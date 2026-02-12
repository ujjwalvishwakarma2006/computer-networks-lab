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
    int connection_socket, fd;
    int n;
    struct sockaddr_in server_addr;
    char buf_outgoing[SIZE], buf_incoming[SIZE]; 
    char* filename = "client_task3.c";
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
    
    // Open the file
    fd = open(filename, O_RDONLY);
    if (fd == -1) error_and_exit("[ERROR] opening file");
    printf("[OPENED] file `%s`\n", filename);

    // Send data to the server, max `SIZE` many bytes at a time
    printf("[SENDING...] data to the server\n");
    while ((n = read(fd, buf_outgoing, SIZE)) > 0) {
        write(connection_socket, buf_outgoing, n);

        // Clear buf_incoming, and read the results coming from the server
        memset(buf_incoming, 0, SIZE);
        read(connection_socket, buf_incoming, SIZE);
        printf("[CHUNK STATS]\t %s", buf_incoming);
    }
    printf("[DONE] sending data to the server\n");
    
    // Send shutdown signal and tell the server that no more data is left to write to the pipe
    n = shutdown(connection_socket, SHUT_WR);
    if (n == -1) error_and_exit("[ERROR] shutting down the connection");
    
    // Read and print final stats of the file
    memset(buf_incoming, 0, SIZE);
    if (read(connection_socket, buf_incoming, SIZE) > 0) {
        printf("[FILE STATS]\t %s\n", buf_incoming);
    }

    // Close all opened file descriptors
    close(fd);
    close(connection_socket);

    return 0;
}