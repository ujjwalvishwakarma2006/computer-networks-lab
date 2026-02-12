#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
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
    int connection_socket;
    int n;
    struct sockaddr_in server_addr;
    char buf[SIZE];
    char* filename = "send.txt";
    FILE* fp;

    // Fill server details in server_addr
    connection_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (connection_socket == -1) error_and_exit("[ERROR] Socket Initialization Error");
    printf("[CREATED SOCKET]\n");
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = server_port;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    printf("[INITIALIZED PORT]\n");
    
    // Send connection request to the server
    printf("[SENDING...] Connection Request\n");
    n = connect(connection_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (n == -1) error_and_exit("[ERROR] Connecting the Server");
    printf("[CONNECTED] to the server\n");
    
    // Open file that needs to be processed on the server
    fp = fopen(filename, "r");
    if (fp == NULL) error_and_exit("[ERROR] File opening error");
    printf("[OPENED] file `%s`\n", filename);
    
    // Send the data to the server line by line, max `SIZE` many bytes at a time
    printf("[SENDING...] data to the server\n");
    while (fgets(buf, SIZE, fp) != NULL) {
        n = send(connection_socket, buf, strlen(buf), 0);
        if (n == -1) error_and_exit("[ERROR] sending data to the server");
    }
    printf("[DONE] sending data to the server\n");
    
    // Send shutdown signal; we may send our custom signal as well
    // Tell the server that I'm shutting down writing
    n = shutdown(connection_socket, SHUT_WR);
    if (n == -1) error_and_exit("[ERROR] shutting down the connection");
    
    // Receive and print counts from the server
    char counts[100];
    recv(connection_socket, counts, 100, 0);
    if (n == -1) error_and_exit("[ERROR] retrieving results from the server");
    printf("[RETRIEVED] results from the server:\n");
    printf("\t%s", counts);
    
    // Close the connection
    printf("[CLOSING...] connection with the server\n");
    n = close(connection_socket);
    if (n == -1) error_and_exit("[ERROR] closing the connection with the server");
    printf("[CLOSED] connection with the server\n");
    
    return 0;    
}