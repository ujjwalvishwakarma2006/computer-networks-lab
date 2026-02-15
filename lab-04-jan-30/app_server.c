#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <semaphore.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <unistd.h>
#define SIZE 1024

void error_and_exit(char* error) {
    perror(error);
    exit(1);
}

sem_t printing;
const char* server_ip = "10.0.2.4";
const int msg_port = 8080;
const int file_port = 8081;
int msg_socket, file_socket;
char file_buf_incoming[SIZE];
char msg_buf_incoming[SIZE];
char file_buf_outgoing[SIZE];
char msg_buf_outgoing[SIZE];

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

void* message_recv() {
    const int connection_socket = msg_socket;
    int n;
    uint32_t msg_len;
    uint32_t bytes_received, to_receive;
    FILE* fp;
    
    while (1) {
        // Clear buffer before receiving any message
        memset(msg_buf_incoming, 0, SIZE);

        // STEP 1: Receive message length
        n = recv(connection_socket, &msg_len, sizeof(msg_len), MSG_WAITALL);
        if (n <= 0) error_and_exit("[MESSAGE LENGTH RECV ERROR]");

        // STEP 2: Open temp file to store the incoming message
        fp = fopen(incoming_message_fname, "w");
        if (fp == NULL) error_and_exit("[MESSAGE FILE OPENING ERROR]");

        // STEP 3: Receive EXACT MESSAGE
        bytes_received = 0;
        while (bytes_received < msg_len) {
            to_receive = (msg_len-bytes_received > SIZE) ?
                        SIZE : (msg_len-bytes_received);
            n = recv(connection_socket, msg_buf_incoming, to_receive, MSG_WAITALL);
            if (n <= 0) {
                printf("[WARNING] Connection closed while receiving file\n");
                break;
            }

            // Write message chunk to the file
            fwrite(msg_buf_incoming, n, 1, fp);
            bytes_received += n;
        }

        // Print the message to show the user
        fclose(fp);
        fp = fopen(incoming_message_fname, "r");
        if (fp == NULL) error_and_exit("[RECV_MSG FILE OPENING ERROR]");
        
        sem_wait(&printing);
        while (fgets(msg_buf_incoming, sizeof(msg_buf_incoming), fp) != NULL) {
            printf("%s", msg_buf_incoming);    
        }
        sem_post(&printing);

        fclose(fp);
        usleep(100000);
    }
    return NULL;
}

void* file_recv() {
    const int connection_socket = file_socket;
    int n;
    char filename[128];
    FILE* fp;
    uint32_t filename_len;
    uint32_t file_size;
    uint32_t bytes_received, to_receive;
    
    while (1) {
        // Clear buffers before receiving any object
        memset(filename, 0, sizeof(filename));

        // STEP 1: Receive length of the filename first
        n = recv(connection_socket, &filename_len, sizeof(filename_len), MSG_WAITALL);
        if (n <= 0) error_and_exit("[FILENAME LENGTH RECV ERROR]");

        // STEP 2: Receive EXACT filename based on the length
        n = recv(connection_socket, filename, filename_len, MSG_WAITALL);
        if (n <= 0) error_and_exit("[FILENAME RECV ERROR]");
        filename[filename_len] = '\0';     // Ensure null termination

        // STEP 3: Receive file size
        n = recv(connection_socket, &file_size, sizeof(file_size), MSG_WAITALL);
        if (n <= 0) error_and_exit("[FILESIZE RECV ERROR]");

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

        // STEP 4: Receive exact file_size many bytes
        bytes_received = 0;
        while (bytes_received < file_size) {
            to_receive = (file_size-bytes_received > SIZE) ? 
                        SIZE : (file_size-bytes_received);
                        
            n = recv(connection_socket, file_buf_incoming, to_receive, 0);
            if (n <= 0) {
                printf("[WARNING] Connection closed while receiving file\n");
                break;
            }
            
            // Write to file
            fwrite(file_buf_incoming, 1, n, fp);
            bytes_received += n;
        }

        fclose(fp);
        sem_wait(&printing);
        printf("Recieved `%s` from client\n", filename);
        sem_post(&printing);
        usleep(100000);
    }

    return NULL;
}

void message_send(const int connection_socket, const char* message) {
    return;
}

void file_send(const int connection_socket, const char* filename) {
    return;
}

int main() {
    sem_init(&printing, 0, 1);
    int msg_socket_listen, file_socket_listen;
    pthread_t msg_recv_thread, file_recv_thread, send_thread;

    msg_socket_listen = start_server(server_ip, msg_port, "Message Transfer");
    file_socket_listen = start_server(server_ip, file_port, "File Transfer");
    msg_socket = init_channel(msg_socket_listen, "Message Transfer");
    file_socket = init_channel(file_socket_listen, "File Transfer");

    pthread_create(&msg_recv_thread, NULL, message_recv, NULL);
    pthread_create(&file_recv_thread, NULL, file_recv, NULL);

    while(1) {
        usleep(100000);
    }

    return 0;
}