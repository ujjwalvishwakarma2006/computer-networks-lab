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

// Function to generate a random string for eof
void generate_random_string(char *str, size_t length) {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                           "abcdefghijklmnopqrstuvwxyz"
                           "0123456789";
    size_t charsetSize = strlen(charset);

    if (length == 0) {
        str[0] = '\0';
        return;
    }

    for (size_t i = 0; i < length; i++) {
        int key = rand() % charsetSize; // Random index
        str[i] = charset[key];
    }
    str[length] = '\0'; // Null-terminate the string
}

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

void message_send(const int connection_socket, char* message) {
    
}

void file_send(const int connection_socket, char* filename) {
    int n;
    char eof[128];
    FILE* fp;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("[FILE OPENING ERROR]");
        return;     // return back to the user
    }

    generate_random_string(eof, 128);

    // Clean filename
    filename[strcspn(filename, "\n")] = '\0';
    filename[strcspn(filename, "\r")] = '\0';

    // Send filename first and sleep for 10ms
    n = send(connection_socket, filename, sizeof(filename), 0);
    if (n == -1) error_and_exit("[FILENAME SEND ERROR]");
    usleep(10000);
    
    // Send eof marker second and sleep again for 10ms
    n = send(connection_socket, eof, sizeof(eof), 0);
    if (n == -1) error_and_exit("[EOF SEND ERROR-1]");
    usleep(10000);

    // Send the file, and close fp
    while ((n = fread(buf_outgoing, 1, SIZE, fp)) > 0) {
        send(connection_socket, buf_outgoing, n, 0);
        memset(buf_outgoing, 0, SIZE);
    }
    fclose(fp);

    // Again send EOF marker to signal end
    n = send(connection_socket, eof, strlen(eof), 0);
    if (n == -1) error_and_exit("[EOF SEND ERROR-2]");
    
    printf("File sent `%s`\n", filename);
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
            file_send(file_socket, filename);
        } else {
            char message[256];
            printf("  Message: ");
            fflush(stdout);
            scanf("%s", message);
            message_send(msg_socket, message);
        }
    }
}

int main() {
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