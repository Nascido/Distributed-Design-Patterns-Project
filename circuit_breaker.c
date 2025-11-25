#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

// Definição dos Estados do Circuit Breaker
#define ESTADO_FECHADO 0     // Tudo normal
#define ESTADO_ABERTO 1      // Bloqueando conexões
#define ESTADO_MEIO_ABERTO 2 // Tentativa de recuperação

int main()
{
    int sockfd;
    int len;
    struct sockaddr_in address;
    int result;
    char buffer[1024];

    // Configurações do Circuit Breaker
    int estado_atual = ESTADO_FECHADO;
    int falhas_consecutivas = 0;
    int limite_falhas = 3;         // Abre o circuito após 3 erros
    time_t ultima_falha = 0;
    int tempo_timeout = 10;        // Tempo em segundos para tentar reconectar (timeout de reset)

    // Endereço do Servidor Externo
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1"); // Localhost
    address.sin_port = htons(9734);
    len = sizeof(address);

    printf("--- Cliente com Circuit Breaker Iniciado ---\n");

    while(1) {
        printf("\n[Tentativa de Requisicao] Estado atual: ");
        if(estado_atual == ESTADO_FECHADO) printf("FECHADO (Normal)\n");
        else if(estado_atual == ESTADO_ABERTO) printf("ABERTO (Bloqueado)\n");
        else printf("MEIO-ABERTO (Testando)\n");

        // --- LÓGICA DO CIRCUIT BREAKER (ANTES DA CONEXÃO) ---
        
        if (estado_atual == ESTADO_ABERTO) {
            // Verifica se já passou o tempo de espera
            if ((time(NULL) - ultima_falha) > tempo_timeout) {
                printf(">> Timeout expirou! Mudando para MEIO-ABERTO para testar.\n");
                estado_atual = ESTADO_MEIO_ABERTO;
            } else {
                printf(">> Circuito ABERTO. Bloqueando requisicao localmente (sem gastar rede).\n");
                sleep(2); // Simula espera antes da próxima tentativa do loop
                continue; // Pula para a próxima iteração do while
            }
        }

        // --- TENTATIVA DE CONEXÃO (Se FECHADO ou MEIO-ABERTO) ---

        // Cria o socket (novo a cada tentativa)
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        
        // Tenta conectar
        result = connect(sockfd, (struct sockaddr *)&address, len);

        if(result == -1) {
            // --- FALHA NA CONEXÃO ---
            printf(">> Erro ao conectar ao servidor!\n");
            
            falhas_consecutivas++;
            ultima_falha = time(NULL);

            if (estado_atual == ESTADO_MEIO_ABERTO) {
                printf(">> Teste falhou. Voltando para ABERTO.\n");
                estado_atual = ESTADO_ABERTO;
            } 
            else if (falhas_consecutivas >= limite_falhas) {
                printf(">> Limite de falhas atingido (%d). Abrindo o Circuito!\n", falhas_consecutivas);
                estado_atual = ESTADO_ABERTO;
            }
            
            close(sockfd);
        } 
        else {
            // --- SUCESSO NA CONEXÃO ---
            printf(">> Conectado com sucesso!\n");

            // Lê a resposta do servidor (cotação)
            int n = read(sockfd, buffer, 1024);
            if (n > 0) {
                buffer[n] = '\0';
                printf(">> Cotacao recebida: %s\n", buffer);
            }

            // Reseta o Circuit Breaker pois o sistema está saudável
            if (estado_atual != ESTADO_FECHADO) {
                printf(">> Sistema recuperado! Fechando o circuito.\n");
            }
            estado_atual = ESTADO_FECHADO;
            falhas_consecutivas = 0;

            close(sockfd);
        }

        sleep(2); // Espera 2 segundos entre requisições para podermos ler o log
    }
    
    return 0;
}