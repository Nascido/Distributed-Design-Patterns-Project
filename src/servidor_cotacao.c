#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <poll.h>      // Biblioteca essencial para o poll
#include <errno.h>

// --- CONFIGURAÇÕES ---
#define PORTA_MOCK 8080
#define PORTA_HUB 9090
#define PORTA_SHARD_1 9801
#define PORTA_SHARD_2 9802

#define MAX_CLIENTES 100   // Máximo de conexões simultâneas
#define TIMEOUT_MS 5000    // 5 segundos (Periodicidade do Monitor)

// Estrutura global para monitorar sockets
struct pollfd fds[MAX_CLIENTES + 1]; // +1 para o socket do servidor (listener)
int nfds = 1; // Contador de descritores abertos (começa com 1, o listener)

// --- FUNÇÕES AUXILIARES (Mesmas de antes, simplificadas) ---

float consultar_mock(char *moeda) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    float preco = -1.0;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORTA_MOCK);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) >= 0) {
        send(sock, moeda, strlen(moeda), 0);
        int valread = read(sock, buffer, 1024);
        if (valread > 0) {
            buffer[valread] = '\0';
            char *ptr = strstr(buffer, "price\":");
            if (ptr) preco = atof(ptr + 7);
        }
    }
    close(sock);
    return preco;
}

void salvar_no_sharding(float valor) {
    int porta_shard = ((int)valor % 2 == 0) ? PORTA_SHARD_1 : PORTA_SHARD_2;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(porta_shard);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) >= 0) {
        char comando[50];
        sprintf(comando, "SALVAR %.2f", valor);
        send(sock, comando, strlen(comando), 0);
        printf("[SHARDING] Enviado para porta %d\n", porta_shard);
    }
    close(sock);
}

// Envia mensagem para TODOS os clientes conectados (Broadcast)
void notificar_clientes(char *mensagem) {
    printf("[PUB/SUB] Notificando clientes...\n");
    // Percorre o array de fds, pulando o índice 0 (que é o servidor listener)
    for (int i = 1; i < nfds; i++) {
        if (fds[i].fd != -1) {
            send(fds[i].fd, mensagem, strlen(mensagem), 0);
        }
    }
}

// Lógica periódica (O que era a Thread Monitor)
void executar_ciclo_monitoramento() {
    printf("[MONITOR] Verificando Mock...\n");
    float preco = consultar_mock("BTC");
    
    if (preco > 0) {
        printf("[MONITOR] Nova cotacao: %.2f\n", preco);
        // salvar_no_sharding(preco);
        
        char msg[100];
        sprintf(msg, "[ALERT] BTC Atualizado: %.2f\n", preco);
        notificar_clientes(msg);
    }
}

// --- FUNÇÃO MAIN COM MULTIPLEXAÇÃO ---

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int opt = 1;

    // 1. Configurar Socket Servidor (Listener)
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket falhou");
        exit(EXIT_FAILURE);
    }
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORTA_HUB);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind falhou");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 10) < 0) {
        perror("Listen falhou");
        exit(EXIT_FAILURE);
    }

    // 2. Inicializar estrutura do Poll
    memset(fds, 0, sizeof(fds));
    
    // Configura o primeiro descritor como o Socket do Servidor (Listener)
    fds[0].fd = server_fd;
    fds[0].events = POLLIN; // Queremos saber quando há dados para ler (nova conexão) 
    nfds = 1;

    printf("--- SERVIDOR MULTIPLEXADO (POLL) NA PORTA %d ---\n", PORTA_HUB);

    while(1) {
        // 3. Chamada Bloqueante do Poll (espera TIMEOUT_MS)
        // [cite: 75] poll(fds, numero_fds, timeout)
        int activity = poll(fds, nfds, TIMEOUT_MS);

        if (activity < 0) {
            perror("Poll error");
            break;
        }

        // 4. Se activity == 0, foi Timeout -> Hora de rodar tarefas periódicas
        if (activity == 0) {
            executar_ciclo_monitoramento();
            continue; // Volta para o inicio do loop
        }

        // 5. Verificar atividade no Socket Listener (Nova Conexão?)
        if (fds[0].revents & POLLIN) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                perror("Accept error");
            } else {
                printf("[IO] Novo cliente conectado (FD: %d)\n", new_socket);
                
                // Adiciona novo socket ao array do poll
                if (nfds < MAX_CLIENTES) {
                    fds[nfds].fd = new_socket;
                    fds[nfds].events = POLLIN; // Monitorar leitura
                    nfds++;
                } else {
                    printf("Recusado: Maximo de clientes atingido.\n");
                    close(new_socket);
                }
            }
        }

        // 6. Verificar atividade nos Sockets dos Clientes (Pedidos ou Desconexão)
        for (int i = 1; i < nfds; i++) {
            if (fds[i].fd != -1 && (fds[i].revents & POLLIN)) {
                char buffer[1024] = {0};
                int valread = read(fds[i].fd, buffer, 1024);

                if (valread <= 0) {
                    // Cliente desconectou ou erro
                    printf("[IO] Cliente desconectado (FD: %d)\n", fds[i].fd);
                    close(fds[i].fd);
                    
                    // Remove do array (substitui pelo último da lista para manter compacto)
                    fds[i] = fds[nfds - 1];
                    nfds--;
                    i--; // Reavaliar a posição atual
                } 
                else {
                    // Processar Pedido
                    buffer[strcspn(buffer, "\r\n")] = 0;
                    printf("[REQ] De %d: %s\n", fds[i].fd, buffer);

                    if (strncmp(buffer, "cotacao_atual", 13) == 0) {
                        float preco = consultar_mock("BTC");
                        char resp[100];
                        sprintf(resp, "Preco BTC: %.2f\n", preco);
                        send(fds[i].fd, resp, strlen(resp), 0);
                    } 
                    else {
                        char *msg = "Comando desconhecido.\n";
                        send(fds[i].fd, msg, strlen(msg), 0);
                    }
                }
            }
        }
    }

    return 0;
}