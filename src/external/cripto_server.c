#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080

// Lógica Mock
float get_mock_price(char *coin) {
    float random_factor = ((float)rand()/(float)(RAND_MAX)) * 500.0; 
    if (strstr(coin, "BTC") != NULL) return 60000.0 + random_factor;
    if (strstr(coin, "ETH") != NULL) return 3000.0 + (random_factor / 10.0);
    return 0.0; 
}

void handle_client(int socket) {
    char buffer[1024] = {0};
    char response[1024] = {0};
    
    // recv() é a chamada padrão para receber dados em sockets TCP
    int valread = recv(socket, buffer, 1024, 0);
    
    if (valread > 0) {
        // Remove quebra de linha que pode vir do telnet/nc
        buffer[strcspn(buffer, "\r\n")] = 0;
        
        printf("-> Pedido: %s\n", buffer);
        float price = get_mock_price(buffer);
        
        if (price > 0) {
            sprintf(response, "{\"coin\": \"%s\", \"price\": %.2f}", buffer, price);
        } else {
            sprintf(response, "{\"error\": \"Moeda invalida\"}");
        }

        send(socket, response, strlen(response), 0);
        printf("<- Resposta: %s\n", response);
    }
    close(socket);
}

int main() {
    srand(time(NULL)); // Inicializa semente randomica
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // 1. Socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }
    
    // Opção para evitar erro de porta presa ao reiniciar
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 2. Bind
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // 3. Listen
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Servidor Mock Cripto ouvindo na porta %d...\n", PORT);

    while(1) {
        int new_socket;
        // 4. Accept
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, 
                           (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }
        handle_client(new_socket);
    }
    return 0;
}