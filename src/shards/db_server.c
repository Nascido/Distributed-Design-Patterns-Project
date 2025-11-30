#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define MAX_HISTORICO 100  
#define BUFFER_SIZE 1024

float historico[MAX_HISTORICO];
int total_registros = 0;

// Função: Adiciona valor ao histórico (FIFO - Remove o mais antigo se encher)
void salvar_no_banco(float valor) {
    if (total_registros < MAX_HISTORICO) {
        historico[total_registros] = valor;
        total_registros++;
    } else {
        for (int i = 0; i < MAX_HISTORICO - 1; i++) {
            historico[i] = historico[i+1];
        }
        historico[MAX_HISTORICO - 1] = valor;
    }
    printf("   [DB] Valor salvo: %.2f (Total: %d)\n", valor, total_registros);
}

void ler_do_banco(int quantidade, char *resposta) {
    char temp[32];
    strcpy(resposta, ""); 
    
    int inicio = total_registros - quantidade;
    if (inicio < 0) inicio = 0;

    for (int i = inicio; i < total_registros; i++) {
        sprintf(temp, "%.2f;", historico[i]); 
        strcat(resposta, temp);
    }
    strcat(resposta, "\n"); 
}

int main(int argc, char *argv[]) {
    int server_sockfd, client_sockfd;
    struct sockaddr_in server_address, client_address;
    socklen_t client_len;
    char buffer_in[BUFFER_SIZE];
    char buffer_out[BUFFER_SIZE];

    if (argc < 2) {
        fprintf(stderr, "Uso: %s <porta>\n", argv[0]);
        fprintf(stderr, "Exemplo: %s 9801\n", argv[0]);
        exit(1);
    }
    int porta = atoi(argv[1]);

    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    int opt = 1;
    setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(porta);

    if (bind(server_sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Erro no Bind");
        exit(1);
    }

    listen(server_sockfd, 5);
    printf("--- SHARD STORAGE rodando na porta %d ---\n", porta);

    while(1) {
        client_len = sizeof(client_address);
        client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_address, &client_len);
        
        memset(buffer_in, 0, BUFFER_SIZE);
        memset(buffer_out, 0, BUFFER_SIZE);

        if (read(client_sockfd, buffer_in, BUFFER_SIZE) > 0) {
            
            if (strncmp(buffer_in, "SALVAR", 6) == 0) {
                float valor = atof(buffer_in + 7);
                salvar_no_banco(valor);
                strcpy(buffer_out, "OK");
            } 
            else if (strncmp(buffer_in, "LER", 3) == 0) {
                int qtd = atoi(buffer_in + 4);
                printf("   [DB] Pedido de leitura: ultimos %d\n", qtd);
                ler_do_banco(qtd, buffer_out);
            } 
            else {
                strcpy(buffer_out, "ERRO");
            }

            write(client_sockfd, buffer_out, strlen(buffer_out));
        }
        close(client_sockfd);
    }
    close(server_sockfd);
    return 0;
}