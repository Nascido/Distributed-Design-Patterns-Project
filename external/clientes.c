#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1"

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    char moeda[100];

    // 1. Criação do Socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("\n Erro na criação do Socket \n");
        return -1;
    }

    // 2. Definição do Endereço do Servidor
    memset(&serv_addr, '0', sizeof(serv_addr));
    
    serv_addr.sin_family = AF_INET;
    // htons converte a porta para a ordem de bytes da rede (Big Endian)
    serv_addr.sin_port = htons(PORT); 

    // inet_pton converte o IP de texto ("127.0.0.1") para binário
    if(inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        perror("\nEndereço inválido / Endereço não suportado \n");
        return -1;
    }

    // 3. Conectar ao Servidor
    // O connect() realiza o "handshake" TCP (SYN, SYN-ACK, ACK)
    printf("Tentando conectar ao servidor em %s:%d...\n", SERVER_IP, PORT);
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("\nConexão Falhou \n");
        return -1;
    }

    // Interação com o Usuário
    printf("Conectado!\nDigite o código da moeda (ex: BTC, ETH): ");
    fgets(moeda, sizeof(moeda), stdin);
    
    // Remove o \n que o fgets captura ao apertar Enter
    moeda[strcspn(moeda, "\n")] = 0;

    // 4. Enviar Pedido (Send)
    // Envia a string digitada para o servidor
    send(sock, moeda, strlen(moeda), 0);
    printf("Pedido enviado: %s\n", moeda);

    // 5. Receber Resposta (Recv)
    // O programa bloqueia aqui esperando a resposta do servidor
    int valread = recv(sock, buffer, 1024, 0);
    
    if (valread > 0) {
        buffer[valread] = '\0'; // Importante: garante o fim da string
        printf("Resposta do Servidor: %s\n", buffer);
    } else {
        printf("Servidor desconectou ou erro no recebimento.\n");
    }

    // 6. Fechar Socket
    // Libera o recurso no sistema operacional
    close(sock); 
    
    return 0;
}