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
char msg_buf_incoming[SIZE];
char file_buf_incoming[SIZE];
char buf_outgoing[SIZE];

// file names to store intermediate files/messages
// it will make it easier for us when we use encryption using openssl
char* outgoing_message_fname = ".temp_msg_out.txt";
char* incoming_message_fname = ".temp_msg_in.txt";
char* outgoing_file_fname = ".temp_file_out.txt";
char* incoming_file_fname = ".temp_file_in.txt";

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
        printf("Server: ");
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
        printf("Recieved `%s` from server\n", filename);
        sem_post(&printing);
        usleep(100000);
    }

    return NULL;
}

void file_send(const int connection_socket, char* filename, const bool is_msg_file) {
    int n;
    FILE* fp;
    uint32_t filename_len;
    uint32_t file_size;

    // Clean filename [Skipping for now, causes seg fault when sending message]
    // filename[strcspn(filename, "\n")] = '\0';
    // filename[strcspn(filename, "\r")] = '\0';

    // Open the file, file may not exist, therefore return in that case
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        perror("[FILE OPENING ERROR]");
        return;
    }

    if (!is_msg_file) {
        // STEP 1: Send filename length first
        filename_len = strlen(filename);
        n = send(connection_socket, &filename_len, sizeof(filename_len), 0);
        if (n == -1) error_and_exit("[FILENAME LENGTH SEND ERROR]");

        // STEP 2: Send the filename
        n = send(connection_socket, filename, filename_len, 0);
        if (n == -1) error_and_exit("[FILENAME SEND ERROR]");
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
    pthread_t msg_recv_thread, file_recv_thread;

    // Initialize two separate connections with the server
    msg_socket = connect_to_server(server_ip, msg_port, "Message Transfer");
    file_socket = connect_to_server(server_ip, file_port, "File Transfer");

    // Threads handle incoming Messages/Files
    pthread_create(&msg_recv_thread, NULL, message_recv, NULL);
    pthread_create(&file_recv_thread, NULL, file_recv, NULL);

    // Original Provess handles outgoing transfers
    handle_outgoing(msg_socket, file_socket);

    // Close all opened sockets
    close(msg_socket);
    close(file_socket);
    return 0;
}