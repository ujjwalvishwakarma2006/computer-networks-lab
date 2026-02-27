#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ncurses.h>
#include <semaphore.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/wait.h>
#include <locale.h>
#define SIZE 1024

typedef enum {
    SERVER, 
    CLIENT,
    NONE
} AppMode;

AppMode app_mode = NONE;
sem_t printing;
char* server_ip = "10.0.2.4";
char* sender_name = "Server";
int msg_port = 8080;
int file_port = 8081;
int msg_socket, file_socket;
char msg_buf_incoming[SIZE];
char file_buf_incoming[SIZE];
char buf_outgoing[SIZE];

// file names to store intermediate files/messages
// it will make it easier for us when we use encryption using openssl
char* pub_key_filename = "pubkey.pem";
char* priv_key_filename = "privkey.pem";
char* sym_key_filename = "aeskey.bin";
char* sym_key_filename_enc = "aeskey_enc.bin";
char* out_msg_filename = ".msg_out_unencrypted.txt";        // Outgoing unencrypted message's filename
char* out_msg_filename_enc = ".msg_out_encrypted.bin";      // Outgoing (encrypted) message's filename
char* inc_msg_filename_enc = ".msg_inc_encrypted.bin";      // Incoming (encrypted) message's filename
char* inc_msg_filename = ".msg_inc_unencrypted.txt";        // Incoming unencrypted message's filename
char* out_fil_filename_enc = ".file_out_encrypted.bin";     // Outgoing encrypted file's filename
char* inc_fil_filename_enc = ".file_inc_encrypted.bin";     // Incoming encrypted file's filename

// ncurse Windows for boxes
WINDOW *log_win = NULL, *input_win = NULL;

void error_and_exit(char* error) {
    close(msg_socket);
    close(file_socket);
    printw("Some Error Occured. "
        "Press any key to see the error message on regular terminal.");
    getch();
    endwin();
    perror(error);
    exit(1);
}

void init_windows() {
    initscr();
    cbreak();
    noecho();

    setlocale(LC_ALL, "");

    int win_rows, win_cols, input_rows = 5;
    getmaxyx(stdscr, win_rows, win_cols);

    // window boxes
    WINDOW *log_box = newwin(win_rows - input_rows, win_cols, 0, 0);
    WINDOW *input_box = newwin(input_rows, win_cols, win_rows - 5, 0);
    box(log_box, 0, 0);
    box(input_box, 0, 0);

    // actual windows; the trick is to use actual window dimension - 2 for row and cols
    log_win = derwin(log_box, win_rows - input_rows - 2, win_cols - 4, 1, 2);
    input_win = derwin(input_box, input_rows - 2, win_cols - 4, 1, 2);

    scrollok(log_win, TRUE);
    scrollok(input_win, TRUE);
    keypad(log_win, TRUE);
    keypad(input_win, TRUE);
    idlok(input_win, TRUE);

    wrefresh(log_box);
    wrefresh(input_box);
}


// ===============================================================================================
//                                      SERVER SPECIFC FUNCTIONS 
// ===============================================================================================

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
    wprintw(log_win, "Listening on port %d for %s Connection Requests\n", server_port, label);
    wrefresh(log_win);

    return server_socket;
}

int init_channel(const int listen_fd, const char* channel_name) {
    int connection_socket;
    struct sockaddr_in client_addr;
    socklen_t addr_size;
    
    // Accept incoming request
    connection_socket = accept(listen_fd, (struct sockaddr*)& client_addr, &addr_size);
    if (connection_socket == -1) error_and_exit("[CONNECTION ERROR]");
    wprintw(log_win, "Connected to client with file descriptor %d for %s\n", connection_socket, channel_name);
    wrefresh(log_win);
    
    return connection_socket;
}


// ===============================================================================================
//                                      CLIENT SPECIFC FUNCTIONS 
// ===============================================================================================

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
    wprintw(log_win, "Connected to %s:%d for %s\n", server_ip, server_port, label);
    wrefresh(log_win);

    return connection_socket;
}


// ===============================================================================================
//                                      COMMUNICATION FUNCTIONS 
// ===============================================================================================

// Only AES-256-CBC Mode
void encrypt_file(char* file_name, char* enc_file_name, char* key_file) {
    int child_pid = fork(); 
    if (child_pid == 0) {
        // openssl rand -out sym_key_filename 32
        char* args[] = {"openssl", "enc", "-aes-256-cbc", "-salt", "-in", file_name, "-out", enc_file_name, "-pbkdf2", "-kfile", key_file, NULL};
        execvp(args[0], args);
        
        // Exit from the child process. Mandetory. 
        exit(0);
    }
    
    // Wait for child process to finish generating key.
    waitpid(child_pid, NULL, 0);
}

// Only AES-256-CBC Mode
void decrypt_file(char* enc_file_name, char* file_name, char* key_file) {
    int child_pid = fork(); 
    if (child_pid == 0) {
        // openssl rand -out sym_key_filename 32
        char* args[] = {"openssl", "enc", "-aes-256-cbc", "-d", "-in", enc_file_name, "-out", file_name, "-pbkdf2", "-kfile", key_file, NULL};
        execvp(args[0], args);
        
        // Exit from the child process. Mandetory. 
        exit(0);
    }
    
    // Wait for child process to finish generating key.
    waitpid(child_pid, NULL, 0);    
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
        fp = fopen(inc_msg_filename_enc, "wb");
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

        // close file before decrypting, otherwise we'll get error
        fclose(fp);
        
        decrypt_file(inc_msg_filename_enc, inc_msg_filename, sym_key_filename);

        fp = fopen(inc_msg_filename, "r");
        if (fp == NULL) error_and_exit("[RECV_MSG FILE OPENING ERROR]");
        
        sem_wait(&printing);
        wprintw(log_win, "%s: ", sender_name);
        while (fgets(msg_buf_incoming, sizeof(msg_buf_incoming), fp) != NULL) {
            wprintw(log_win, "%s", msg_buf_incoming);    
        }
        wprintw(log_win, "\n");
        wrefresh(log_win);
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
        fp = fopen(inc_fil_filename_enc, "wb");
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

        // close file before decrypting, otherwise we'll get error
        fclose(fp);

        decrypt_file(inc_fil_filename_enc, filename, sym_key_filename);

        sem_wait(&printing);
        wprintw(log_win, "%s sent `%s`\n", sender_name, filename);
        sem_post(&printing);
        wrefresh(log_win);
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
    
    if (!is_msg_file) {
        // First, encrypt the file, and store the encrypted version out_file_filename_enc
        // Message file is already encrypted by message_send() function
        encrypt_file(filename, out_fil_filename_enc, sym_key_filename);
        
        // STEP 1: Send filename length first
        filename_len = strlen(filename);
        n = send(connection_socket, &filename_len, sizeof(filename_len), 0);
        if (n == -1) error_and_exit("[FILENAME LENGTH SEND ERROR]");
        
        // STEP 2: Send the filename
        n = send(connection_socket, filename, filename_len, 0);
        if (n == -1) error_and_exit("[FILENAME SEND ERROR]");
        
        // Open the encrypted file, file may not exist, therefore return in that case
        fp = fopen(out_fil_filename_enc, "rb");
        if (fp == NULL) {
            perror("[FILE OPENING ERROR]");
            return;
        }
    } else {
        // Open the encrypted message file, file may not exist, therefore return in that case
        fp = fopen(out_msg_filename_enc, "rb");
        if (fp == NULL) {
            perror("[FILE OPENING ERROR]");
            return;
        }
    }
    
    // STEP 3: Get file size, and send to the server
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    n = send(connection_socket, &file_size, sizeof(file_size), 0);
    if (n == -1) error_and_exit("[FILE LENGTH SEND ERROR]");

    // STEP 4: Send the file, and close fp
    while ((n = fread(buf_outgoing, 1, SIZE, fp)) > 0) {
        send(connection_socket, buf_outgoing, n, 0);
    }

    fclose(fp);
    if (!is_msg_file) {
        sem_wait(&printing);
        wprintw(log_win, "File sent `%s`\n", filename);
        sem_post(&printing);
        wrefresh(log_win);
    }
}

void message_send(const int connection_socket, char* message) {
    FILE* fp;

    // First write the message into a file, will be useful furing encryption
    fp = fopen(out_msg_filename, "w");
    if (fp == NULL) error_and_exit("[MESSAGE FILE OPENING ERROR]");
    
    fprintf(fp, "%s", message);
    fclose(fp);

    encrypt_file(out_msg_filename, out_msg_filename_enc, sym_key_filename);
    
    file_send(connection_socket, out_msg_filename_enc, true);
}


// ===============================================================================================
//                                     KEY EXCHANGE FUNCTIONS 
// ===============================================================================================

// Server calls the following function to share his public key
void send_server_public_key() {
    // Follow similar step to file_send() to send the public key file to the client
    int n;
    FILE* fp;
    uint32_t file_size;
    int connection_socket = file_socket;

    fp = fopen(pub_key_filename, "rb");
    if (fp == NULL) error_and_exit("[FILE OPENING ERROR - KEY EXCHANGE]");

    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    n = send(connection_socket, &file_size, sizeof(file_size), 0);
    if (n == -1) error_and_exit("[FILE LENGTH SEND ERROR - KEY EXCHANGE]");

    while ((n = fread(buf_outgoing, 1, SIZE, fp)) > 0) {
        send(connection_socket, buf_outgoing, n, 0);
    }

    fclose(fp);
    sem_wait(&printing);
    wprintw(log_win, "Public Key sent\n");
    sem_post(&printing);
    wrefresh(log_win);
}

// Client calls this function to receive server's public key
void recv_server_public_key() {
    int n;
    uint32_t file_size;
    uint32_t bytes_received, to_receive;
    FILE* fp;
    int connection_socket = file_socket;

    fp = fopen(pub_key_filename, "w");
    if (fp == NULL) error_and_exit("[FILE OPENING ERROR - KEY EXCHANGE]");

    n = recv(connection_socket, &file_size, sizeof(file_size), MSG_WAITALL);
    if (n <= 0) error_and_exit("[FILESIZE RECV ERROR]");

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
    wprintw(log_win, "Server Public Key Received\n");
    sem_post(&printing);
    wrefresh(log_win);
}

// Client calls this function to generate a session for every new session
void generate_session_key() {
    int child_pid = fork(); 
    if (child_pid == 0) {
        // openssl rand -out sym_key_filename 32
        char* args[] = {"openssl", "rand", "-out", sym_key_filename, "32", NULL};
        execvp(args[0], args);

        // Exit from the child process. Mandetory. 
        exit(0);
    }

    // Wait for child process to finish generating key.
    waitpid(child_pid, NULL, 0);
}

// Client calls this function to encrypt the session key using server's public key
void encrypt_session_key() {
    int child_pid = fork(); 
    if (child_pid == 0) {
        // openssl rand -out sym_key_filename 32
        char* args[] = {"openssl", "pkeyutl", "-encrypt", "-pubin", "-inkey", pub_key_filename, "-in", sym_key_filename, "-out", sym_key_filename_enc, NULL};
        execvp(args[0], args);

        // Exit from the child process. Mandetory. 
        exit(0);
    }

    // Wait for child process to finish generating key.
    waitpid(child_pid, NULL, 0);
}

// Client calls this function to generate a session key and share with the server
void send_session_key() {
    // Follow similar step to file_send() to send the public key file to the client
    int n;
    FILE* fp;
    uint32_t file_size;
    int connection_socket = file_socket;

    fp = fopen(sym_key_filename_enc, "rb");
    if (fp == NULL) error_and_exit("[FILE OPENING ERROR - KEY EXCHANGE]");

    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    n = send(connection_socket, &file_size, sizeof(file_size), 0);
    if (n == -1) error_and_exit("[FILE LENGTH SEND ERROR - KEY EXCHANGE]");

    while ((n = fread(buf_outgoing, 1, SIZE, fp)) > 0) {
        send(connection_socket, buf_outgoing, n, 0);
    }

    fclose(fp);
    sem_wait(&printing);
    wprintw(log_win, "Session Key sent\n");
    sem_post(&printing);
    wrefresh(log_win);
}

// Server calls this function to receive the session key from client
void recv_session_key() {
    int n;
    uint32_t file_size;
    uint32_t bytes_received, to_receive;
    FILE* fp;
    int connection_socket = file_socket;

    fp = fopen(sym_key_filename_enc, "w");
    if (fp == NULL) error_and_exit("[FILE OPENING ERROR - KEY EXCHANGE]");

    n = recv(connection_socket, &file_size, sizeof(file_size), MSG_WAITALL);
    if (n <= 0) error_and_exit("[FILESIZE RECV ERROR]");

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
    wprintw(log_win, "Session Key Received\n");
    sem_post(&printing);
    wrefresh(log_win);
}

// Server calls this function to decrypt the encrypted session key
void decrypt_session_key() {
    int child_pid = fork(); 
    if (child_pid == 0) {
        // openssl rand -out sym_key_filename 32
        char* args[] = {"openssl", "pkeyutl", "-decrypt", "-inkey", priv_key_filename, "-in", sym_key_filename_enc, "-out", sym_key_filename, NULL};
        execvp(args[0], args);

        // Exit from the child process. Mandetory. 
        exit(0);
    }

    // Wait for child process to finish generating key.
    waitpid(child_pid, NULL, 0);
}


// ===============================================================================================
//                                    INPUT HANDLING FUNCTIONS 
// ===============================================================================================

void print_usage(const char *program_name) {
    printf("Usage: %s --server|--client [OPTIONS]\n", program_name);
    printf("Options:\n");
    printf("  --server          Run in server mode\n");
    printf("  --client          Run in client mode\n");
    printf("  --ip <address>    IP address (default: %s)\n", server_ip);
    printf("  --mp <port>       Message Port number (default: %d)\n", msg_port);
    printf("  --fp <port>       File Port number (default: %d)\n", file_port);
    printf("  --help            Show this help message\n");
}

// Function to setup the environment and some global variables based on command line inputs
// e.g. whether the application is being run as client or server? 
void setup(int argc, char* argv[]) {
    if ((argc < 2) || strcmp(argv[1], "--server") != 0 && strcmp(argv[1], "--client") != 0) {
        print_usage("app");
        exit(1);
    }

    sender_name = strcmp(argv[1], "--server") == 0 ? "Client" : "Server";
    app_mode = strcmp(argv[1], "--server") == 0 ? SERVER : CLIENT;

    for (int i = 2; i < argc; ++i) {
        if (strcmp(argv[i], "--ip") == 0) {
            if (i + 1 < argc) {
                strncpy(server_ip, argv[++i], sizeof(server_ip) - 1);
                server_ip[sizeof(server_ip) - 1] = '\0';
            } else {
                print_usage("app");
                exit(1);
            }
        }
        else if (strcmp(argv[i], "-mp") == 0) {
            if (i + 1 < argc) {
                msg_port = atoi(argv[++i]);
                if (msg_port <= 0 || msg_port > 65535) {
                    print_usage("app");
                    exit(1);
                }
            } else {
                print_usage("app");
                exit(1);
            }
        }
        else if (strcmp(argv[i], "-fp") == 0) {
            if (i + 1 < argc) {
                file_port = atoi(argv[++i]);
                if (file_port <= 0 || file_port > 65535) {
                    print_usage("app");
                    exit(1);
                }
            } else {
                print_usage("app");
                exit(1);
            }
        }
        else if (strcmp(argv[i], "--help") == 0) {
            print_usage("app");
            exit(1);
        }
        else {
            fprintf(stderr, "Error: Unknown option '%s'\n", argv[i]);
            print_usage("app");
            exit(1);
        }
    }

    return;
}

// Function to take input with dynamic string length 
char* wgetstring(WINDOW* window) {
    int capacity = 512;
    char* buffer = malloc(capacity * sizeof(char));
    if (!buffer) return NULL;

    int ch;
    int i;

    for (i = 0; ; ++i) {
        ch = wgetch(window);
        if (ch == '\n') break;

        // Manually handle backspace behaviour
        if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
            if (i > 0) {
                --i;
                int y, x;
                getyx(window, y, x);
                wmove(window, y, x - 1);
                wdelch(window);
                wrefresh(window);
            }
            --i;
            continue;
        }

        // Adjust buffer size based on the input length
        if (i >= capacity - 1) {
            capacity *= 2;
            char* new_buffer = realloc(buffer, capacity * sizeof(char));
            if (!new_buffer) {
                free(buffer);
                return NULL;
            }
            buffer = new_buffer;
        }
        buffer[i] = ch;
        waddch(window, ch);
        wrefresh(window);
    }

    buffer[i] = '\0';
    return buffer;
}

// Function to handle outgoing messages/files
void handle_outgoing(const int msg_socket, const int file_socket) {
    while (1) {
        char* input = wgetstring(input_win);
        wclear(input_win);
        wrefresh(input_win);
        
        if (!input) continue;
        
        if (strncmp(input, "-f ", 3) == 0) {
            file_send(file_socket, input + 3, false);
        } else {
            sem_wait(&printing);
            wprintw(log_win, "You: %s\n", input);
            wrefresh(log_win);
            message_send(msg_socket, input);
            sem_post(&printing);
        }
     
        free(input);
    }
}

int main(int argc, char* argv[]) {
    setup(argc, argv);

    sem_init(&printing, 0, 1);
    pthread_t msg_recv_thread, file_recv_thread, send_thread;
    
    // Init ncurses windows
    init_windows();
    
    if (app_mode == SERVER) {
        // Initialize listening sockets 
        int msg_socket_listen, file_socket_listen;
        msg_socket_listen = start_server(server_ip, msg_port, "Message Transfer");
        file_socket_listen = start_server(server_ip, file_port, "File Transfer");
  
        // Accept first connections
        msg_socket = init_channel(msg_socket_listen, "Message Transfer");
        file_socket = init_channel(file_socket_listen, "File Transfer");

        send_server_public_key();
        recv_session_key();
        decrypt_session_key();
    } else if (app_mode == CLIENT) {
        // Initialize two separate connections with the server
        msg_socket = connect_to_server(server_ip, msg_port, "Message Transfer");
        file_socket = connect_to_server(server_ip, file_port, "File Transfer");

        recv_server_public_key();
        generate_session_key();
        encrypt_session_key();
        send_session_key();
    } else {
        error_and_exit("[ENVIRONMENT INIT FAILED]");
    }

    // Threads handle incoming Messages/Files
    pthread_create(&msg_recv_thread, NULL, message_recv, NULL);
    pthread_create(&file_recv_thread, NULL, file_recv, NULL);

    // Original Process handles outgoing transfers
    handle_outgoing(msg_socket, file_socket);

    // End ncurse session with all its windows and subwindows
    endwin();
    return 0;
}