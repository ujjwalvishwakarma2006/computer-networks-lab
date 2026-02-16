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

- [ ] Encryption
- [ ] Bug Fixes and UI
- [ ] Multi-chat support

## 3. Encrypted Client-Server Chat Application with File Transfer Support

[**Feb 06 Lab Session**](./lab-05-feb-06/) extends the capabilities of chat application built previously to end-to-end encryption. 