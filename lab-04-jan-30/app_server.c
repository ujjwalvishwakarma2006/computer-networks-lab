#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#define SIZE 1024

void error_and_exit(char* error) {
    perror(error);
    exit(1);
}

const char* server_ip = "10.0.2.4";
const int msg_port = 8080;
const int file_port = 8081;
char buf_incoming[SIZE];
char buf_outgoing[SIZE];

// file names to store intermediate files/messages
// it will make it easier for us when we use encryption using openssl
const char* outgoing_message_fname = "temp_msg_out.txt";
const char* incoming_message_fname = "temp_msg_in.txt";
const char* outgoing_file_fname = "temp_file_out.txt";
const char* incoming_file_fname = "temp_file_in.txt";

int start_server(const char* server_ip, const int server_port, const char* label) {
    int server_socket;
    int n;
    struct sockaddr_in server_addr;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) error_and_exit("[SOCKET INIT ERROR]");
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = server_port;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    // Bind socket to the port
    n = bind(server_socket, (struct sockaddr*)& server_addr, sizeof(server_addr));
    if (n == -1) error_and_exit("[SOCKET BIND ERROR]");

    // Start listening on the port
    n = listen(server_socket, 2);
    if (n == -1) error_and_exit("[PORT LISTEN ERROR]");
    printf("Listening on port %d for %s Connection Requests\n", server_port, label);

    return server_socket;
}

int init_channel(const int listen_fd, const char* channel_name) {
    int connection_socket;
    struct sockaddr_in client_addr;
    socklen_t addr_size;
    
    // Accept incoming request
    connection_socket = accept(listen_fd, (struct sockaddr*)& client_addr, &addr_size);
    if (connection_socket == -1) error_and_exit("[CONNECTION ERROR]");
    printf("Connected to client with file descriptor %d for %s\n", connection_socket, channel_name);
    
    return connection_socket;
}

void message_recv(const int connection_socket) {
    return;
}

void file_recv(const int connection_socket) {
    int n;
    char filename[128];
    char eof[128];
    FILE* fp;
    
    while (1) {
        // Clear buffers before receiving any object
        memset(filename, 0, sizeof(filename));
        memset(eof, 0, sizeof(eof));

        // Receive filename first
        n = recv(connection_socket, filename, sizeof(filename), 0);
        if (n <= 0) error_and_exit("[FILENAME RECV ERROR]");
        filename[n] = '\0';     // Ensure null termination

        // Receive EOF marker
        n = recv(connection_socket, eof, 128, 0);
        if (n <= 0) error_and_exit("[EOF RECV ERROR]");
        eof[n] = '\0';

        // Validate filename
        if (strlen(filename) == 0) {
            perror("[EMPTY FILENAME ERROR]");
            continue;
        }

        // Open file for writing
        fp = fopen(filename, "w");
        if (fp == NULL) {
            perror("[FILE OPENING ERROR]");
            continue;   // Continue to receive other files
        }

        while (1) {
            memset(buf_incoming, 0, SIZE);
            n = recv(connection_socket, buf_incoming, SIZE, 0);
            
            if (n <= 0) {
                printf("[WARNING] Connection closed while receiving file\n");
                break;
            }

            // Break if reached EOF
            if (strncmp(buf_incoming, eof, strlen(eof)) == 0) break;

            // Write to file
            fwrite(buf_incoming, 1, n, fp);
        }

        fclose(fp);
        printf("Recieved `%s` from client\n", filename);
    }
}

void message_send(const int connection_socket, const char* message) {
    return;
}

void file_send(const int connection_socket, const char* filename) {
    return;
}

int main() {
    int msg_socket_listen, file_socket_listen;
    int msg_socket, file_socket;

    msg_socket_listen = start_server(server_ip, msg_port, "Message Transfer");
    file_socket_listen = start_server(server_ip, file_port, "File Transfer");
    msg_socket = init_channel(msg_socket_listen, "Message Transfer");
    file_socket = init_channel(file_socket_listen, "File Transfer");

    if (fork() == 0) {
        // Child Processes (receive messages and files)
        if (fork == 0) {
            // receive messages
            message_recv(msg_socket);
        } else {
            // receive files
            file_recv(file_socket);
        }
    } else {
        // Original Process (sends messages and files)
    }

    return 0;
}