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

typedef struct {
    int fd;
    char topico_interesse[10]; // Ex: "BTC", "ETH" ou "TODOS"
} ClienteSubscriber;

typedef struct {
    char chave[10];
    int porta;
} RotaShard;

RotaShard tabela_roteamento[] = {
    {"BTC", 9801},
    {"ETH", 9802}
};

// --- CONFIGURAÇÕES ---
#define PORTA_MOCK 8080
#define PORTA_HUB 9090
#define PORTA_SHARD_BTC 9801
#define PORTA_SHARD_ETH 9802

#define MAX_CLIENTES 100
#define TIMEOUT_MS 5000

// Globais
struct pollfd fds[MAX_CLIENTES + 1];
int nfds = 1;
CircuitBreaker cb_mock; 
ClienteSubscriber clientes[MAX_CLIENTES]; 

void processar_assinatura(int index_cliente, char *moeda) {
    strcpy(clientes[index_cliente].topico_interesse, moeda);
    printf("[BROKER] Cliente %d inscreveu-se no topico: %s\n", 
           clientes[index_cliente].fd, moeda);
    
    char msg[50];
    sprintf(msg, "Confirmado: Assinado no topico %s\n", moeda);
    send(clientes[index_cliente].fd, msg, strlen(msg), 0);
}

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

void notificar_clientes(char *moeda_atualizada, float valor) {
    char mensagem[100];
    sprintf(mensagem, "[ALERT] %s Atualizado: %.2f\n", moeda_atualizada, valor);

    printf("[PUB/SUB] Publicando no topico '%s'...\n", moeda_atualizada);

    for (int i = 1; i < nfds; i++) {
        // Lógica de Filtro:
        // Envia SE: Tópico do cliente for "TODOS" OU for igual à moeda atualizada
        if (fds[i].fd != -1) {
             // Precisamos mapear o fd do poll com o nosso array de clientes
             // (Simplificação: assumindo indices sincronizados ou busca simples)
             if (strcmp(clientes[i].topico_interesse, "TODOS") == 0 ||
                 strcmp(clientes[i].topico_interesse, moeda_atualizada) == 0) {
                 
                 send(fds[i].fd, mensagem, strlen(mensagem), 0);
             }
        }
    }
}

int obter_porta_shard(char *chave) {
    int total_shards = sizeof(tabela_roteamento) / sizeof(RotaShard);
    
    for(int i=0; i < total_shards; i++) {
        if (strcmp(chave, tabela_roteamento[i].chave) == 0) {
            return tabela_roteamento[i].porta;
        }
    }
    return -1; // Não encontrado (Rota default ou erro)
}

void salvar_no_sharding(char *moeda, float valor) {
    int porta = obter_porta_shard(moeda); 
    
    if (porta == -1) {
        printf("[ROTEADOR] Erro: Sem rota para a chave %s\n", moeda);
        return;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(porta);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) >= 0) {
        char comando[50];
        sprintf(comando, "SALVAR %.2f", valor);
        send(sock, comando, strlen(comando), 0);
        printf("[SHARDING] Saved %s (%.2f) on Port %d\n", moeda, valor, porta);
    }
    close(sock);
}

//HELPER
float calcular_media_de_lista(char *lista_str) {
    if (strlen(lista_str) == 0) return 0.0;
    
    float soma = 0.0;
    int conta = 0;
    char *token = strtok(lista_str, ";"); // Quebra a string nos ';'
    
    while(token != NULL) {
        float valor = atof(token);
        if(valor > 0) {
            soma += valor;
            conta++;
        }
        token = strtok(NULL, ";");
    }
    
    if (conta == 0) return 0.0;
    return soma / conta;
}

void realizar_scatter_gather(char *resposta_final) {
    int sock;
    struct sockaddr_in serv_addr;
    char buffer_btc[1024] = {0};
    char buffer_eth[1024] = {0};

    printf("[SCATTER] Iniciando consultas paralelas aos Shards...\n");

    // 1. SCATTER: Consultar Shard BTC
    sock = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORTA_SHARD_BTC);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) >= 0) {
        send(sock, "LER 10", 6, 0); // Pede ultimos 10
        read(sock, buffer_btc, 1024);
    }
    close(sock);

    // 2. SCATTER: Consultar Shard ETH
    sock = socket(AF_INET, SOCK_STREAM, 0); // Novo socket
    serv_addr.sin_port = htons(PORTA_SHARD_ETH); // Muda porta
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) >= 0) {
        send(sock, "LER 10", 6, 0); // Pede ultimos 10
        read(sock, buffer_eth, 1024);
    }
    close(sock);

    // 3. GATHER (Processamento Local)
    printf("[GATHER] Calculando medias e unificando resposta...\n");
    float media_btc = calcular_media_de_lista(buffer_btc);
    float media_eth = calcular_media_de_lista(buffer_eth);

    // Monta a resposta unificada
    sprintf(resposta_final, 
            "=== RELATORIO DE MERCADO ===\n"
            "Media BTC (Ultimos 10): %.2f\n"
            "Media ETH (Ultimos 10): %.2f\n"
            "Tendencia: %s\n",
            media_btc, media_eth, 
            (media_btc > media_eth * 10) ? "BTC Dominante" : "Normal");
}

void executar_ciclo_monitoramento() {
    printf("[MONITOR] Ciclo de atualizacao...\n");
    
    float p_btc = consultar_mock("BTC");
    if (p_btc > 0) {
        salvar_no_sharding("BTC", p_btc);
        notificar_clientes("BTC", p_btc);
    }

    float p_eth = consultar_mock("ETH");
    if (p_eth > 0) {
        salvar_no_sharding("ETH", p_eth);
        notificar_clientes("ETH", p_eth);
    }
}

// --- NOVA FUNÇÃO: LER DO SHARD (FALLBACK) ---
float ler_ultimo_preco_shard(char *moeda) {
    int porta = obter_porta_shard(moeda);
    if (porta == -1) return -1.0;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    char buffer[64] = {0};
    float preco_cache = -1.0;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(porta);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) >= 0) {
        send(sock, "LER 1", 5, 0);
        
        if (read(sock, buffer, 64) > 0) {
            preco_cache = atof(buffer); 
        }
    }
    close(sock);
    return preco_cache;
}

int main() {
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

                    clientes[nfds].fd = new_socket;
                    strcpy(clientes[nfds].topico_interesse, "TODOS");

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
                    clientes[i] = clientes[nfds - 1];
                    nfds--;
                    i--;
                } else {
                    if (strncmp(buffer, "SUBSCRIBE", 9) == 0) {
                        char moeda[10];
                        if (sscanf(buffer, "SUBSCRIBE %s", moeda) == 1) {
                            strcpy(clientes[i].topico_interesse, moeda);
                            char msg[50];
                            sprintf(msg, "OK: Assinado no topico %s\n", moeda);
                            send(fds[i].fd, msg, strlen(msg), 0);
                            printf("[BROKER] Cliente %d assinou: %s\n", fds[i].fd, moeda);
                        }
                    }
                    else if (strncmp(buffer, "cotacao_atual", 13) == 0) {
                        char moeda[10] = {0}; 

                        if (sscanf(buffer, "cotacao_atual(%[^)])", moeda) == 1) {
                            
                            printf("[DEBUG] Cliente pediu cotacao de: %s\n", moeda);

                            float p = consultar_mock(moeda);
                            
                            char resp[100];
                            if (p > 0) {
                                sprintf(resp, "%s: %.2f\n", moeda, p);
                            } else {
                                printf("[FALLBACK] Servico indisponivel. Buscando cache no Shard...\n");
                                            
                                float p_cache = ler_ultimo_preco_shard(moeda);
                                
                                if (p_cache > 0) {
                                    sprintf(resp, "%s: %.2f (Cache - Servico Off)\n", moeda, p_cache);
                                } else {
                                    sprintf(resp, "Erro: Servico Indisponivel e sem dados historicos para '%s'\n", moeda);
                                }                            }
                            send(fds[i].fd, resp, strlen(resp), 0);
                        }
                    }
                    else if (strncmp(buffer, "media_historica", 15) == 0) {
                        char relatorio[1024];
                        realizar_scatter_gather(relatorio);
                        send(fds[i].fd, relatorio, strlen(relatorio), 0);
                    }
                }
            }
        }
    }
    return 0;
}

