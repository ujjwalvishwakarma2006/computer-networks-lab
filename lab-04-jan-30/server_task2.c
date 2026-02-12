#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SIZE 1024

void error_and_exit(char* error) {
    perror(error);
    exit(1);
}

int main() {

    // Define variables
    char* server_ip = "10.0.2.4";
    int server_port = 8080;
    int server_socket, connection_socket;
    int n;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
    char buf[SIZE];
    int chars = 0, words = 0, sentences = 0;
    
    // Initialize server_addr and server_socket objects
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) error_and_exit("[ERROR] Socket Initialization Error");
    printf("[CREATED SOCKET]\n");
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = server_port;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    printf("[INITIALIZED PORT]\n");
    
    // Bind socket to the port, and start listening
    n = bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (n == -1) error_and_exit("[ERROR] Socket Binding Error");
    printf("[SOCKET BINDED]\n");
    n = listen(server_socket, 1);
    if (n == -1) error_and_exit("[ERROR] Port Listening Error");
    printf("[LISTENING...] for connection requests on port %d\n", server_port);
    
    // Accept the first incoming request and assign a connection_socket
    addr_size = sizeof(client_addr);
    connection_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_size);
    if (connection_socket == -1) error_and_exit("[ERROR] Connection Acceptence Error");
    printf("[CONNECTION] request received and accepted\n");
    
    // Keep receiving data from the client until no more data is left
    printf("[RECEIVING...] data from the client\n");
    while (1) {
        n = recv(connection_socket, buf, SIZE, 0);
        if (n <= 0) break;
        
        for (int i = 0; i < n; ++i) {
            chars++;
            if (buf[i] == ' ' || buf[i] == '\n') words++;
            if (buf[i] == '.') sentences++;
        }
    }
    printf("[DONE] receiving data from the client\n");
    
    // In case there is only one word in the file with no spaces and new lines
    if (chars > 0) words++;
    
    // Send back the results to the client
    char counts[100];
    sprintf(counts, "Characters: %d \tWords: %d \tSentences: %d\n", chars, words, sentences);
    write(connection_socket, counts, strlen(counts));
    printf("[FINISHED] request and sent the results\n");
    
    // Close opened sockets, here 2 sockets were opened
    printf("[CLOSING] connections...\n");
    n = close(connection_socket);
    if (n == -1) error_and_exit("[ERROR] Closing Connection with the Client");
    printf("[CLOSED] connection with client\n");
    n = close(server_socket);
    if (n == -1) error_and_exit("[ERROR] Closing Server");
    printf("[CLOSED] server down\n");
    return 0;
}