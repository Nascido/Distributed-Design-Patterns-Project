// servidor_cotacao.c (Atualizado)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <poll.h>
#include <errno.h>
#include "circuit_breaker.h"

// --- CONFIGURAÇÕES ---
#define PORTA_MOCK 8080
#define PORTA_HUB 9090
#define PORTA_SHARD_1 9801
#define PORTA_SHARD_2 9802

#define MAX_CLIENTES 100
#define TIMEOUT_MS 5000

// Globais
struct pollfd fds[MAX_CLIENTES + 1];
int nfds = 1;
CircuitBreaker cb_mock; // <--- Instância global do Circuit Breaker

// --- FUNÇÕES AUXILIARES ---

float consultar_mock(char *moeda) {
    // 1. PERGUNTA AO CIRCUIT BREAKER SE PODE TENTAR
    if (!cb_allow_request(&cb_mock)) {
        printf("[MOCK] Circuito ABERTO. Retornando erro imediato (Fail Fast).\n");
        return -1.0; // Retorna erro sem nem tentar conectar
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    float preco = -1.0;
    int sucesso = 0; // Flag para indicar sucesso da operação

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORTA_MOCK);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    // Tenta conectar
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) >= 0) {
        send(sock, moeda, strlen(moeda), 0);
        int valread = read(sock, buffer, 1024);
        
        if (valread > 0) {
            buffer[valread] = '\0';
            
            // Verifica se o servidor mandou um erro explícito (ex: JSON de erro)
            if (strstr(buffer, "error") != NULL) {
                printf("[MOCK] Servidor retornou erro lógico: %s\n", buffer);
                sucesso = 0;
            } else {
                char *ptr = strstr(buffer, "price\":");
                if (ptr) {
                    preco = atof(ptr + 7);
                    sucesso = 1;
                }
            }
        } else {
             // Leu 0 bytes (conexão fechada prematuramente)
             sucesso = 0;
        }
    } else {
        // Falha no connect()
        perror("[MOCK] Falha ao conectar");
        sucesso = 0;
    }

    close(sock);

    // 2. ATUALIZA O CIRCUIT BREAKER COM O RESULTADO
    if (sucesso) {
        cb_record_success(&cb_mock);
    } else {
        cb_record_failure(&cb_mock);
    }

    return preco;
}

// ... (Manter funções salvar_no_sharding e notificar_clientes iguais ao seu original) ...
void salvar_no_sharding(float valor) {
    // (Código original mantido para brevidade)
}

void notificar_clientes(char *mensagem) {
    printf("[PUB/SUB] Notificando clientes...\n");
    for (int i = 1; i < nfds; i++) {
        if (fds[i].fd != -1) {
            send(fds[i].fd, mensagem, strlen(mensagem), 0);
        }
    }
}

void executar_ciclo_monitoramento() {
    printf("\n[MONITOR] Estado do Disjuntor: %s\n", cb_state_to_string(cb_mock.state));
    printf("[MONITOR] Verificando Mock...\n");
    
    float preco = consultar_mock("BTC");
    
    if (preco > 0) {
        printf("[MONITOR] Nova cotacao: %.2f\n", preco);
        
        char msg[100];
        sprintf(msg, "[ALERT] BTC Atualizado: %.2f\n", preco);
        notificar_clientes(msg);
    } else {
        printf("[MONITOR] Falha ao obter cotacao.\n");
    }
}

// --- MAIN ---

int main() {
    // ... (Configuração de socket listener igual ao seu original) ...
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int opt = 1;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) exit(EXIT_FAILURE);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORTA_HUB);
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 10);

    // -------------------------------------------------
    // INICIALIZA O CIRCUIT BREAKER
    // 3 falhas consecutivas abrem o circuito.
    // 10 segundos de timeout para tentar novamente.
    // -------------------------------------------------
    cb_init(&cb_mock, 3, 10); 

    memset(fds, 0, sizeof(fds));
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;
    nfds = 1;

    printf("--- SERVIDOR COTACAO (COM CIRCUIT BREAKER) PORTA %d ---\n", PORTA_HUB);

    while(1) {
        int activity = poll(fds, nfds, TIMEOUT_MS);

        if (activity < 0) break;

        if (activity == 0) {
            executar_ciclo_monitoramento();
            continue;
        }

        // ... (Restante da lógica do loop principal igual ao seu original) ...
        if (fds[0].revents & POLLIN) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) >= 0) {
                printf("[IO] Novo cliente conectado\n");
                if (nfds < MAX_CLIENTES) {
                    fds[nfds].fd = new_socket;
                    fds[nfds].events = POLLIN;
                    nfds++;
                }
            }
        }
        // Loop de clientes... (igual ao seu original)
         for (int i = 1; i < nfds; i++) {
            if (fds[i].fd != -1 && (fds[i].revents & POLLIN)) {
                char buffer[1024] = {0};
                if (read(fds[i].fd, buffer, 1024) <= 0) {
                    close(fds[i].fd);
                    fds[i] = fds[nfds - 1];
                    nfds--;
                    i--;
                } else {
                    // Lógica simples de resposta
                    if (strncmp(buffer, "cotacao_atual", 13) == 0) {
                         float p = consultar_mock("BTC");
                         char resp[50];
                         if (p > 0) sprintf(resp, "BTC: %.2f\n", p);
                         else sprintf(resp, "Servico Indisponivel (CB Open)\n");
                         send(fds[i].fd, resp, strlen(resp), 0);
                    }
                }
            }
        }
    }
    return 0;
}