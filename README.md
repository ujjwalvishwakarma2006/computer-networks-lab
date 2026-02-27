# Computer Networks Laboratory

Writing codes to practice computer networking. 
These codes are intentionally over-engineered to follow best practices.

Here are brief descriptions of each folder in the order. 
Please follow the recommended order to get the best experience. 
I am using the Top-Down approach based on the Kurose and Ross book. 

Initial lab sessions were utilized to setup 2 VirtualBox VMs so that experiments can be performed. 
Therefore, the description starts with the 3rd lab session.

> **Note**: Codes are not written from a performance point of view. They are solely written from an understanding point of view. Error handling is minimal or non-existent - this is intentional to keep things simple and focused on networking concepts.

## 1. DNS Server Implementation

[**Jan 26 Lab Session**](./lab-03-jan-26/) focuses on implementing a simple DNS server using socket programming in Python. I chose Python for better understanding and ease of implementation. This lab demonstrates both TCP and UDP server implementations for DNS resolution.

**Files Overview:**

**Client Side (runs on client VM):**
- **`client.py`** - A client program that can query either the TCP or UDP DNS server to resolve hostnames to IP addresses, then make requests to the resolved IP

**Server Side (runs on server VM):**
- **`dns_init.py`** - Shared configuration module that loads the `ip_addr` dictionary from CSV for fast hostname lookups and defines DNS port constants (`DNS_PORT_TCP` and `DNS_PORT_UDP`)
- **`dns_server_tcp.py`** - TCP-based DNS server listening on port `11999`, handles `ip-resolve-req` requests using connection-oriented communication
- **`dns_server_udp.py`** - UDP-based DNS server listening on port `12000`, handles `ip-resolve-req` requests using connectionless communication
- **`ip_addresses.csv`** - Simple CSV file containing `hostname,ip_address` pairs for DNS resolution
- **`unnecessarysite.py`** - A completely unnecessary server for a completely unnecessary site that demonstrates what happens after DNS resolution. Handles only one type of request as of now because why not

**How it works:**
1. Client requests hostname resolution from DNS server (TCP or UDP)
2. DNS server looks up hostname in the loaded dictionary
3. Server responds with IP address
4. Client can then connect to the resolved IP address

Fun fact: Both servers can run simultaneously since they use different ports!

## 2. Client-Server Chat Application with File Transfer Support

[**Jan 30 Lab Session**](./lab-04-jan-30/) focuses on implementing a client-server chat application with support for both message and file transfers. The server maintains two listening ports for incoming client connections. Two threads are created on both client and server sides - one for receiving messages and another for receiving files - while the main process handles outgoing transfers. To ease development, the application was built incrementally using knowledge gained from simpler preliminary tasks.

>**Note**: Apart from the functions mentioned here, the code also contains some functions related to the `ncurses` library (for example; `init_windows()` and `wgetstring()`). 

**Server Side** (`app_server.c`):
- **`int start_server(const char* server_ip, const int server_port, const char* label)`** - Creates a socket, binds it to the server address (`server_ip:server_port`), and starts listening for connection requests. Returns the file descriptor of the listening socket.

- **`int init_channel(const int listen_fd, const char* channel_name)`** - Accepts the first connection request on the listening socket (`listen_fd`) and returns the connected socket's file descriptor. Logs which file descriptor is being used for the specified `channel_name`.

- **`void* message_recv()`** - Thread function that continuously monitors `msg_socket` for incoming messages and prints them. Uses a `while` loop with a 100ms sleep after each message to prevent CPU blocking.

- **`void* file_recv()`** - Thread function that continuously monitors `file_socket` for incoming files, similar to `message_recv()`.

- **`void file_send(const int connection_socket, char* filename, const bool is_msg_file)`** - Sends a file over the specified socket. The `is_msg_file` flag indicates whether the file represents a message, as this function is also used internally by `message_send()`.

- **`void message_send(const int connection_socket, char* message)`** - Sends a message by first storing it in a temporary file and then calling `file_send()`. This approach will simplify future encryption/decryption using libraries like OpenSSL.

- **`void handle_outgoing(const int msg_socket, const int file_socket)`** - Main loop that prompts the user to choose between sending a file or message, then requests the appropriate input and initiates the transfer.

**Client Side** (`app_client.c`):
Contains the same functions as the server except `start_server()` and `init_channel()`. Instead, it includes:

- **`int connect_to_server(const char* server_ip, const int server_port, const char* label)`** - Establishes a TCP connection to the server at `server_ip:server_port`. Returns the connected socket's file descriptor and logs the successful connection for the specified `label` purpose.

**Upcoming Features**:

- [x] Encryption (Available in [3](#3-encrypted-client-server-chat-application-with-file-transfer-support))
- [x] Bug Fixes and TUI (minimal using `ncurses` library, requires installing)
- [ ] Multi-chat support

// ...existing code...
## 3. Encrypted Client-Server Chat Application with File Transfer Support

[**Feb 06 Lab Session**](./lab-05-feb-06/) extends the capabilities of the chat application built previously with end-to-end encryption. The server and client code have been unified into a single `app.c` file, reducing redundancy and making feature/protocol changes easier. The application mode (server or client) is selected via command-line arguments.

### Requirements
- `ncurses` - `sudo apt-get install libncurses-dev`
- `openssl` - Used via `execvp()` for all cryptographic operations

### Features
- End-to-end encrypted using **AES-256-CBC** for message/file encryption and **RSA** for session key exchange.
- Per-session symmetric key: the client generates a fresh AES key for every session, encrypts it with the server's RSA public key, and sends it to the server.
- Supports both message and file transfers over separate TCP connections (separate ports for each).
- TUI built with `ncurses` — a scrollable log window for incoming/outgoing messages and a dedicated input window.
- Threaded architecture: two threads handle incoming messages and files respectively, while the main process handles outgoing transfers.
- Dynamic input buffer that grows as needed via `realloc()`.

### Encryption Protocol
1. **Server** sends its RSA public key (`pubkey.pem`) to the client over the file transfer socket.
2. **Client** generates a 256-bit AES session key using `openssl rand`.
3. **Client** encrypts the session key with the server's public key using `openssl pkeyutl`.
4. **Client** sends the encrypted session key to the server.
5. **Server** decrypts the session key using its private key (`privkey.pem`) via `openssl pkeyutl`.
6. All subsequent messages and files are encrypted/decrypted using `openssl enc -aes-256-cbc` with the shared session key.

### Code Structure

**Server & Client Modes** (`app.c`):

- **`void setup(int argc, char* argv[])`** - Parses command-line arguments to determine application mode (`--server` or `--client`), IP address, message port, and file port. Sets global configuration accordingly.

- **`void init_windows()`** - Initializes the `ncurses` TUI with a scrollable log window and an input window, separated by bordered boxes.

**Server-Specific Functions:**
- **`int start_server(const char* server_ip, const int server_port, const char* label)`** - Creates, binds, and listens on a TCP socket. Returns the listening socket file descriptor.
- **`int init_channel(const int listen_fd, const char* channel_name)`** - Accepts the first incoming connection on a listening socket. Returns the connected socket file descriptor.

**Client-Specific Functions:**
- **`int connect_to_server(const char* server_ip, const int server_port, const char* label)`** - Establishes a TCP connection to the server. Returns the connected socket file descriptor.

**Communication Functions:**
- **`void encrypt_file(char* file_name, char* enc_file_name, char* key_file)`** - Encrypts a file using AES-256-CBC via `openssl enc` (forks a child process).
- **`void decrypt_file(char* enc_file_name, char* file_name, char* key_file)`** - Decrypts a file using AES-256-CBC via `openssl enc -d` (forks a child process).
- **`void* message_recv()`** - Thread function that continuously receives encrypted messages on `msg_socket`, decrypts them, and displays in the log window.
- **`void* file_recv()`** - Thread function that continuously receives encrypted files on `file_socket`, decrypts them, and saves with the original filename.
- **`void file_send(const int connection_socket, char* filename, const bool is_msg_file)`** - Encrypts and sends a file over the specified socket. Sends filename length, filename, file size, then file data. When `is_msg_file` is true, skips filename transmission and uses the pre-encrypted message file.
- **`void message_send(const int connection_socket, char* message)`** - Writes the message to a temp file, encrypts it, then delegates to `file_send()`.

**Key Exchange Functions:**
- **`void send_server_public_key()` / `void recv_server_public_key()`** - Server sends and client receives the RSA public key over the file socket.
- **`void generate_session_key()`** - Client generates a 32-byte random AES key via `openssl rand`.
- **`void encrypt_session_key()`** - Client encrypts the AES key with the server's public key via `openssl pkeyutl`.
- **`void send_session_key()` / `void recv_session_key()`** - Client sends and server receives the encrypted session key.
- **`void decrypt_session_key()`** - Server decrypts the session key using its private key via `openssl pkeyutl`.

**Input Handling:**
- **`char* wgetstring(WINDOW* window)`** - Reads a dynamically-sized string from an `ncurses` window with manual backspace handling.
- **`void handle_outgoing(const int msg_socket, const int file_socket)`** - Main loop that reads user input. Prefix `-f <filename>` sends a file; otherwise the input is sent as a message.

### Limitations
- 🔴 **Security**: Prone to man-in-the-middle (MITM) attack — the server's public key is assumed to be pre-shared securely; there is no certificate verification.
- 🔴 **Security**: Uses RSA for key exchange, therefore not forward secret; compromise of the server's private key exposes all past session keys.
- 🔴 **Security**: Filenames and their lengths are not transmitted encrypted.
- 🟡 **Feature**: Doesn't support multiple simultaneous client connections.
- 🔵 **Dependency**: Cryptographic operations are performed by forking `openssl` as child processes, requiring `openssl` to be installed and on `PATH`.

### How to run
- Generate the server's RSA key pair (run once on the server):
    ```
    openssl genpkey -algorithm RSA -out privkey.pem -pkeyopt rsa_keygen_bits:2048
    openssl pkey -in privkey.pem -pubout -out pubkey.pem
    ```
- Compile the code by linking with the required libraries:
    ```
    gcc app.c -lpthread -lncurses -o app
    ```
- Usage:
    ```
    Usage: ./app --server|--client [OPTIONS]
    ```
- Options:
    ```
    Options:
    --server          Run in server mode
    --client          Run in client mode
    --ip <address>    IP address (default: 10.0.2.4)
    --mp <port>       Message Port number (default: 8080)
    --fp <port>       File Port number (default: 8081)
    --help            Show this help message
    ```
- Sending files from within the app:
    ```
    -f <filename>
    ```

The organization of the code is also improved to make it manageable (especially combining the server and client codes removes redundancy, and makes it easier to make any changes in a feature/protocol or UI).

A standalone repository for this version with further enhancements can be found [here](https://github.com/ujjwalvishwakarma2006/saffron-c).