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
char buf_incoming[SIZE];
char buf_outgoing[SIZE];

// file names to store intermediate files/messages
// it will make it easier for us when we use encryption using openssl
char* outgoing_message_fname = "temp_msg_out.txt";
char* incoming_message_fname = "temp_msg_in.txt";
char* outgoing_file_fname = "temp_file_out.txt";
char* incoming_file_fname = "temp_file_in.txt";

int connect_to_server(const char* server_ip, const int server_port, const char* label) {
    int connection_socket;
    int n;
    struct sockaddr_in server_addr;

    connection_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (connection_socket == -1) error_and_exit("[SOCKET INIT ERROR]");
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = server_port;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    
    n = connect(connection_socket, (struct sockaddr*)& server_addr, sizeof(server_addr));
    if (n == -1) error_and_exit("[CONNECTION ERROR]");
    printf("Connected to %s:%d for %s\n", server_ip, server_port, label);

    return connection_socket;
}

void message_recv(const int connection_socket) {

}

void file_recv(const int connection_socket) {

}

void file_send(const int connection_socket, char* filename, const bool is_msg_file) {
    int n;
    FILE* fp;
    uint32_t filename_len;
    uint32_t file_size;

    if (!is_msg_file) {
        // Clean filename
        filename[strcspn(filename, "\n")] = '\0';
        filename[strcspn(filename, "\r")] = '\0';

        // STEP 1: Send filename length first
        filename_len = strlen(filename);
        n = send(connection_socket, &filename_len, sizeof(filename_len), 0);
        if (n == -1) error_and_exit("[FILENAME LENGTH SEND ERROR]");

        // STEP 2: Send the filename
        n = send(connection_socket, filename, filename_len, 0);
        if (n == -1) error_and_exit("[FILENAME SEND ERROR]");
    }

    // Open the file
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        perror("[FILE OPENING ERROR]");
        return;     // return back to the user
    }
    
    // STEP 3: Get file size, and send to the server
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    n = send(connection_socket, &file_size, sizeof(file_size), 0);
    if (n == -1) error_and_exit("[FILENAME LENGTH SEND ERROR]");

    // STEP 4: Send the file, and close fp
    while ((n = fread(buf_outgoing, 1, SIZE, fp)) > 0) {
        send(connection_socket, buf_outgoing, n, 0);
    }

    fclose(fp);
    if (!is_msg_file) {
        sem_wait(&printing);
        printf("  File sent `%s`\n", filename);
        sem_post(&printing);
    }
}

void message_send(const int connection_socket, char* message) {
    FILE* fp;

    // First write the message into a file, will be useful furing encryption
    fp = fopen(outgoing_message_fname, "w");
    if (fp == NULL) error_and_exit("[MESSAGE FILE OPENING ERROR]");
    
    fprintf(fp, "%s", message);
    fclose(fp);
    
    file_send(connection_socket, outgoing_message_fname, true);
}

// Function to handle outgoing messages/files
void handle_outgoing(const int msg_socket, const int file_socket) {
    while (1) {
        int object_type;
        printf("You [ file(1) message(2) ]: ");
        fflush(stdout);  // Force output to display immediately
        scanf("%d", &object_type);
    
        if (object_type != 1 && object_type != 2) {
            printf("  [INVALID OPTION] %d\n", object_type);
            continue;
        }
        if (object_type == 1) {
            char filename[128];
            printf("  Filename: ");
            fflush(stdout);
            scanf("%s", filename);
            file_send(file_socket, filename, false);
        } else {
            char* message = NULL;
            size_t msg_len = 0;
            getchar();
            printf("  Message: ");
            fflush(stdout);
            getline(&message, &msg_len, stdin);
            message_send(msg_socket, message);
            free(message);
        }
    }
}

int main() {
    sem_init(&printing, 0, 1);
    int msg_socket, file_socket;

    msg_socket = connect_to_server(server_ip, msg_port, "Message Transfer");
    file_socket = connect_to_server(server_ip, file_port, "File Transfer");

    if (fork() == 0) {
        // Child Process (prints incoming messages and filenames)
        exit(1);
    } else {
        // Original Process (sends messages and files)
        handle_outgoing(msg_socket, file_socket);
    }

    // Close all opened sockets
    close(msg_socket);
    close(file_socket);
    return 0;
}