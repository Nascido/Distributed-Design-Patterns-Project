#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

int main()
{
    int server_sockfd, client_sockfd;
    int server_len, client_len;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
    char str_out[1024];
    
    // Configuração para gerar números aleatórios diferentes a cada execução
    srand(time(NULL)); 

    // 1. Criar o socket
    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // Configuração extra: Permite reutilizar a porta imediatamente após fechar o servidor
    // Útil para evitar o erro "Address already in use" durante os testes
    int opt = 1;
    setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 2. Definir o endereço do servidor
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); // Aceita conexões de qualquer IP
    server_address.sin_port = htons(9734); // Porta definida
    server_len = sizeof(server_address);

    // 3. Ligar o socket ao endereço (Bind)
    if (bind(server_sockfd, (struct sockaddr *)&server_address, server_len) < 0) {
        perror("Erro no Bind");
        exit(1);
    }

    // 4. Ouvir conexões (Listen)
    listen(server_sockfd, 5);
    printf("Servidor Externo de Cotacoes rodando na porta 9734...\n");

    while(1) {
        // 5. Aceitar conexão
        client_len = sizeof(client_address);
        client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_address, &client_len);
        
        printf("Cliente conectado!\n");

        // Simulação: Gerar um preço aleatório entre 100.00 e 200.00
        float price = 100.0 + (float)(rand() % 10000) / 100.0;
        
        // Formatar a mensagem de resposta
        sprintf(str_out, "%.2f", price);
        
        // Simular um pequeno tempo de processamento (opcional, ajuda a testar timeouts depois)
        // sleep(1); 

        // 6. Enviar dados
        write(client_sockfd, str_out, strlen(str_out));
        printf("Cotacao enviada: %s\n", str_out);

        // 7. Fechar conexão com este cliente específico
        close(client_sockfd);
    }
    
    // O servidor principal nunca chega aqui no while(1), mas por boa prática:
    close(server_sockfd);
    return 0;
}