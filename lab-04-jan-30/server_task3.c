#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#define SIZE 100

void error_and_exit(char* error) {
    perror(error);
    exit(1);
}

int main(){

    // Define variables
    char *server_ip = "10.0.2.4";
    int server_port = 8080;
    int server_socket, connection_socket;
    int n;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
    char buf[SIZE]; 
    int total_chars = 0, total_words = 0, total_sentences = 0;

    // Initialize server_socket and server_addr objects
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) error_and_exit("[ERROR] Socket Initialization Error");
    printf("[CREATED SOCKET]\n");
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = server_port;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    printf("[INITIALIZED PORT]\n");
    
    // Modify sever_socket's SO_REUSEADDR option to use the socket immediately
    int opt = 1;
    n = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (server_socket == -1) error_and_exit("[ERROR] Socket option error");
    
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

    // Read and processing the incoming data buffer-by-buffer
    printf("[RECEIVING...] data from the client\n");
    while (1) {
        memset(buf, 0, SIZE);
        n = read(connection_socket, buf, SIZE);
        if(n <= 0) break;

        int chars = 0, words = 0, sentences = 0;
        for(int i=0; i<n; i++){
            chars++;
            if(buf[i] == ' ' || buf[i] == '\n') words++;
            if(buf[i] == '.') sentences++;
        }
        total_chars += chars; 
        total_words += words; 
        total_sentences += sentences;

        char buf_counts[SIZE];
        sprintf(buf_counts, "Characters: %d \tWords: %d \tSentences: %d\n", chars, words, sentences);
        write(connection_socket, buf_counts, strlen(buf_counts));
    }

    // In case there is only one word in the file with no spaces and new lines
    if(total_chars > 0) total_words++; 

    // Send back the results to the client
    char final_counts[SIZE];
    sprintf(final_counts, "Characters: %d \tWords: %d \tSentences: %d\n", total_chars, total_words, total_sentences);
    write(connection_socket, final_counts, strlen(final_counts));
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