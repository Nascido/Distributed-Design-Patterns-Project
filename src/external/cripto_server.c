#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080

#define CHANCE_FALHA 20    
#define MAX_LATENCIA 1    


float preco_btc_atual = 60000.00;
float preco_eth_atual = 3000.00;
time_t ultima_atualizacao = 0;
int intervalo_proxima_mudanca = 0;

void atualizar_mercado_se_necessario() {
    time_t agora = time(NULL);
    
    if (ultima_atualizacao == 0 || (agora - ultima_atualizacao) > intervalo_proxima_mudanca) {
        
        float fator_btc = 1.0 + (((float)rand() / RAND_MAX) * 0.05 - 0.025);
        float fator_eth = 1.0 + (((float)rand() / RAND_MAX) * 0.05 - 0.025);
        
        preco_btc_atual *= fator_btc;
        preco_eth_atual *= fator_eth;
        
        ultima_atualizacao = agora;
        intervalo_proxima_mudanca = (rand() % 5) + 1;
        
        printf("[MERCADO] Preços atualizados! Próxima mudança em %ds\n", intervalo_proxima_mudanca);
        printf("          BTC: %.2f | ETH: %.2f\n", preco_btc_atual, preco_eth_atual);
    }
}

float get_price(char *coin) {
    atualizar_mercado_se_necessario();
    
    if (strstr(coin, "BTC") != NULL) return preco_btc_atual;
    if (strstr(coin, "ETH") != NULL) return preco_eth_atual;
    return 0.0; 
}

void handle_client(int socket) {
    char buffer[1024] = {0};
    char response[2048] = {0}; 
    
    // --- SIMULAÇÃO DE LATÊNCIA (Demora para responder) ---
    int latencia = rand() % (MAX_LATENCIA + 1);
    if (latencia > 0) {
        printf("[CAOS] Simulando latência de %ds...\n", latencia);
        sleep(latencia);
    }

    // --- SIMULAÇÃO DE FALHA (Erro na conexão) ---
    int sorteio_falha = rand() % 100;
    if (sorteio_falha < CHANCE_FALHA) {
        printf("[CAOS] Simulando CRASH/ERRO 500 para este cliente!\n");
        close(socket);
        return;
    }

    // --- FLUXO NORMAL ---
    int valread = recv(socket, buffer, 1024, 0);
    
    if (valread > 0) {
        buffer[strcspn(buffer, "\r\n")] = 0;
        printf("-> Pedido: %s\n", buffer);
        
        float price = get_price(buffer);
        
        if (price > 0) {
            snprintf(response, sizeof(response), "{\"coin\": \"%s\", \"price\": %.2f}", buffer, price);
        } else {
            snprintf(response, sizeof(response), "{\"error\": \"Moeda invalida\"}");
        }

        send(socket, response, strlen(response), 0);
        printf("<- Resposta: %s\n", response);
    }
    close(socket);
}

int main() {
    srand(time(NULL));
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }
    
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("=== MOCK CRIPTO EXCHANGE ===\n");
    printf("Porta: %d\n", PORT);
    printf("Chance de Falha: %d%%\n", CHANCE_FALHA);
    printf("Latência Max: %ds\n", MAX_LATENCIA);
    printf("============================\n");

    while(1) {
        int new_socket;
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, 
                           (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }
        handle_client(new_socket);
    }
    return 0;
}