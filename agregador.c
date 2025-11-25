#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

// Função auxiliar para conectar e ler de um servidor específico
float obter_cotacao(int porta) {
    int sockfd;
    struct sockaddr_in address;
    int result;
    char buffer[1024];
    float valor = 0.0;

    // 1. Criar Socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    // 2. Definir endereço (Localhost + Porta recebida por parametro)
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_port = htons(porta);

    // 3. Conectar
    result = connect(sockfd, (struct sockaddr *)&address, sizeof(address));
    
    if(result == -1) {
        printf("[Agregador] Erro: Nao consegui falar com o servidor na porta %d\n", porta);
        return 0.0; // Em caso de erro retorna 0
    }

    // 4. Ler resposta
    int n = read(sockfd, buffer, 1024);
    if (n > 0) {
        buffer[n] = '\0';
        valor = atof(buffer); // Converte string para float
        printf("[Agregador] Recebi %.2f do servidor na porta %d\n", valor, porta);
    }
    
    close(sockfd);
    return valor;
}

int main() {
    printf("--- Iniciando Agregador Scatter/Gather ---\n");

    while(1) {
        printf("\nPressione ENTER para calcular a media das cotacoes...");
        getchar(); // Espera o utilizador dar Enter

        // --- SCATTER (Espalhar os pedidos) ---
        // Numa versão avançada, fariamos isto com Threads ou Fork para ser simultaneo.
        // Aqui fazemos sequencial, que é mais facil de entender e funciona para o exemplo.
        
        printf("Consultando Shard 1 (Porta 9734)...\n");
        float val1 = obter_cotacao(9734);
        
        printf("Consultando Shard 2 (Porta 9735)...\n");
        float val2 = obter_cotacao(9735);

        // --- GATHER (Juntar e Processar) ---
        
        if (val1 > 0 && val2 > 0) {
            float media = (val1 + val2) / 2.0;
            printf(">> RESULTADO UNIFICADO: Media de Mercado = %.2f\n", media);
        } else {
            printf(">> FALHA: Um ou mais servidores nao responderam.\n");
        }
    }

    return 0;
}