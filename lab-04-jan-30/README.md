# Client-Server programming with C

## Functions Reference

### `socket()`
Creates an endpoint for communication.

**Syntax:**
```c
int socket(int domain, int type, int protocol);
```

**Parameters:**
- `domain`: Communication domain (e.g., `AF_INET` for IPv4)
- `type`: Communication type (e.g., `SOCK_STREAM` for TCP, `SOCK_DGRAM` for UDP)
- `protocol`: Protocol to use (usually `0` for default)

**Returns:**
- File descriptor for the socket on success
- `-1` on error

**Example from code:**
```c
server_socket = socket(AF_INET, SOCK_STREAM, 0);
```

---

### `bind()`
Binds a socket to a specific IP address and port.

**Syntax:**
```c
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
```

**Parameters:**
- `sockfd`: Socket file descriptor
- `addr`: Pointer to `sockaddr` structure containing address info
- `addrlen`: Size of the address structure

**Returns:**
- `0` on success
- `-1` on error

**Example from code:**
```c
bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
```

---

### `listen()`
Marks the socket as a passive socket that will accept incoming connections.

**Syntax:**
```c
int listen(int sockfd, int backlog);
```

**Parameters:**
- `sockfd`: Socket file descriptor
- `backlog`: Maximum length of the queue for pending connections

**Returns:**
- `0` on success
- `-1` on error

**Example from code:**
```c
listen(server_socket, 1);
```

---

### `accept()`
Accepts an incoming connection on a listening socket.

**Syntax:**
```c
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
```

**Parameters:**
- `sockfd`: Listening socket file descriptor
- `addr`: Pointer to `sockaddr` structure to store client address (can be `NULL`)
- `addrlen`: Pointer to variable containing address structure size (modified to actual size)

**Returns:**
- New socket file descriptor for the accepted connection on success
- `-1` on error

**Example from code:**
```c
connection_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_size);
```

---

### `recv()`
Receives data from a connected socket.

**Syntax:**
```c
ssize_t recv(int sockfd, void *buf, size_t len, int flags);
```

**Parameters:**
- `sockfd`: Socket file descriptor
- `buf`: Buffer to store received data
- `len`: Maximum number of bytes to receive
- `flags`: Flags to modify behavior (usually `0`)

**Returns:**
- **Number of bytes actually received** on success
- `0` if the peer has performed an orderly shutdown
- `-1` on error

**Example from code:**
```c
n = recv(connection_socket, buf, SIZE, 0);
```

**Answer to your question:** Yes! `n` holds the **number of bytes actually received**. This might be less than `SIZE` if the sender sent fewer bytes.

---

### `connect()`
Initiates a connection to a server (client-side function).

**Syntax:**
```c
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
```

**Parameters:**
- `sockfd`: Socket file descriptor (created with `socket()`)
- `addr`: Pointer to `sockaddr` structure containing server address and port
- `addrlen`: Size of the address structure

**Returns:**
- `0` on success (connection established)
- `-1` on error

**Example from client code:**
```c
// From client_tcp.c
connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
```

**Key difference from server:**
- Servers use `bind()` → `listen()` → `accept()`
- Clients use `connect()` to initiate connection to a server

---

### `send()` / `write()`
Sends data through a connected socket.

**Syntax:**
```c
ssize_t send(int sockfd, const void *buf, size_t len, int flags);
ssize_t write(int fd, const void *buf, size_t count);
```

**Parameters:**
- `sockfd`/`fd`: Socket file descriptor
- `buf`: Buffer containing data to send
- `len`/`count`: Number of bytes to send
- `flags`: Flags to modify behavior (only for `send()`, usually `0`)

**Returns:**
- Number of bytes sent on success
- `-1` on error

**Example from code:**
```c
// Server side
write(connection_socket, counts, strlen(counts));

// Client side  
send(sockfd, data, sizeof(data), 0);
```

---

### `sprintf()`
Writes formatted output to a string.

**Syntax:**
```c
int sprintf(char *str, const char *format, ...);
```

**Parameters:**
- `str`: Destination string buffer
- `format`: Format string (like `printf`)
- `...`: Variable arguments based on format string

**Returns:**
- Number of characters written (excluding null terminator)
- Negative value on error

**Example from code:**
```c
sprintf(counts, "Characters: %d \tWords: %d \tSentences: %d\n", chars, words, sentences);
```

---

### `strlen()`
Calculates the length of a string (excluding null terminator).

**Syntax:**
```c
size_t strlen(const char *s);
```

**Parameters:**
- `s`: Pointer to null-terminated string

**Returns:**
- Number of characters in the string (excluding `\0`)

**Example from code:**
```c
char counts[100];
sprintf(counts, "Characters: %d", chars);
write(connection_socket, counts, strlen(counts));  // Send exact length
```

**Common use:**
- Determining how many bytes to send when transmitting strings
- Safer than using `sizeof()` which includes buffer size, not string length

**Important:**
```c
char buf[100] = "Hello";
sizeof(buf)  // Returns 100 (buffer size)
strlen(buf)  // Returns 5 (actual string length)
```

---

### `close()`
Closes a file descriptor (including sockets).

**Syntax:**
```c
int close(int fd);
```

**Parameters:**
- `fd`: File descriptor to close

**Returns:**
- `0` on success
- `-1` on error

**Example from code:**
```c
close(connection_socket);
close(server_socket);
```

---

### `shutdown()`
Shuts down part or all of a full-duplex connection on a socket.

**Syntax:**
```c
int shutdown(int sockfd, int how);
```

**Parameters:**
- `sockfd`: Socket file descriptor
- `how`: Specifies what to shut down:
  - `SHUT_RD` (0) - Further receives are disallowed
  - `SHUT_WR` (1) - Further sends are disallowed
  - `SHUT_RDWR` (2) - Further sends and receives are disallowed

**Returns:**
- `0` on success
- `-1` on error

**Example:**
```c
shutdown(sockfd, SHUT_WR);  // Tell server "I'm done sending"
// Can still receive data from server
```

**Difference from `close()`:**
- `shutdown()`: Affects all processes sharing the socket; can shutdown reading/writing separately
- `close()`: Only closes the file descriptor for current process; fully closes connection if it's the last reference

**Use case:**
```c
// Client sends file, then signals "done sending"
send_file(fp, sockfd);
shutdown(sockfd, SHUT_WR);  // Half-close: "I'm done sending"
// Server now knows all data has been sent
recv(sockfd, response, SIZE, 0);  // Can still receive response
close(sockfd);  // Finally close completely
```

---

### `bzero()` / `memset()`
Fills a block of memory with zeros.

**Syntax:**
```c
void bzero(void *s, size_t n);
void *memset(void *s, int c, size_t n);
```

**Parameters:**
- `s`: Pointer to memory block
- `n`: Number of bytes to zero
- `c`: Value to set (for `memset`, use `0` for zeroing)

**Returns:**
- `void` for `bzero()`
- Pointer to memory block for `memset()`

**Example from client_tcp.c:**
```c
bzero(data, SIZE);  // Clear the buffer after sending
```

**Note:** `bzero()` is deprecated; prefer `memset(s, 0, n)` in modern code.

---

### `fopen()`, `fgets()`, `fclose()`
Standard file I/O operations (used for reading files to send over socket).

**Syntax:**
```c
FILE *fopen(const char *filename, const char *mode);
char *fgets(char *str, int n, FILE *stream);
int fclose(FILE *stream);
```

**Parameters:**
- `filename`: Path to file
- `mode`: "r" for read, "w" for write, "a" for append
- `str`: Buffer to store read data
- `n`: Maximum number of characters to read
- `stream`: File pointer

**Returns:**
- `fopen()`: File pointer on success, `NULL` on error
- `fgets()`: Pointer to string on success, `NULL` on EOF or error
- `fclose()`: `0` on success, `EOF` on error

**Example from client_tcp.c:**

Client vs Server Socket Flow

### Server Flow:
1. `socket()` - Create socket
2. `bind()` - Bind to IP address and port
3. `listen()` - Mark socket as passive (accepting connections)
4. `accept()` - Wait for and accept client connection (blocks until client connects)
5. `recv()` / `send()` - Communicate with client
6. `close()` - Close connection and server socket

### Client Flow:
1. `socket()` - Create socket
2. `connect()` - Connect to server's IP address and port
3. `send()` / `recv()` - Communicate with server
4. `close()` - Close connection

**Key Difference:**
- Server **waits** for connections (passive)
- Client **initiates** connections (active)

**Visual Representation:**
```
Server                          Client
------                          ------
socket()                       socket()
  |                              |
bind()                           |
  |                              |
listen()                         |
  |                              |
accept() [WAITING...]            |
  |                          connect()
  |<---------CONNECTION--------->|
  |                              |
recv() / send()              send() / recv()
  |<--------DATA EXCHANGE------->|
  |                              |
close()                      close()
```
```
fp = fopen("send.txt", "r");
while(fgets(data, SIZE, fp) != NULL) {
    send(sockfd, data, sizeof(data), 0);
    bzero(data, SIZE);
}
fclose(fp);
```

---
**Returns:**
- `0` on success
- `-1` on error

**Example from code:**
```c
close(connection_socket);
close(server_socket);
```

---

### `inet_addr()`
Converts IPv4 address from dotted-decimal string to binary form.

**Syntax:**
```c
in_addr_t inet_addr(const char *cp);
```

**Parameters:**
- `cp`: String containing IPv4 address (e.g., "10.0.2.4")

**Returns:**
- IP address in network byte order
- `INADDR_NONE` on error

**Example from code:**
```c
server_addr.sin_addr.s_addr = inet_addr(server_ip);
```

---

### `htons()` (Host TO Network Short)
Converts a 16-bit number from host byte order to network byte order.

**Syntax:**
```c
uint16_t htons(uint16_t hostshort);
```

**Parameters:**
- `hostshort`: 16-bit value in host byte order

**Returns:**
- Value in network byte order

**Note:** Port numbers must be in network byte order, but not used correctly in [server_task2.c](server_task2.c) - should be `htons(server_port)` instead of just `server_port`.

---

### `setsockopt()`
Allows you to configure various options for a socket to modify its behavior.

**Syntax:**
```c
int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
```

**Parameters:**
- `sockfd`: Socket file descriptor
- `level`: Protocol level at which the option resides
  - `SOL_SOCKET` - Socket level (most common)
  - `IPPROTO_TCP` - TCP level
  - `IPPROTO_IP` - IP level
- `optname`: The option to set (e.g., `SO_REUSEADDR`, `SO_KEEPALIVE`)
- `optval`: Pointer to the value for the option
- `optlen`: Size of the option value

**Returns:**
- `0` on success
- `-1` on error

**Example from code:**
```c
int opt = 1;
setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
```

---

#### Common Socket Options

**`SO_REUSEADDR` - Reuse address/port**

Allows the socket to bind to an address that is already in use. Essential for server development to avoid "Address already in use" errors when restarting the server. When a TCP connection closes, it enters a `TIME_WAIT` state (usually 2 minutes). `SO_REUSEADDR` lets you bind to the port even during `TIME_WAIT`.

```c
int opt = 1;
setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
```

---

**`SO_KEEPALIVE` - Keep connection alive**

Enables periodic testing of the connection to detect if peer is still alive. Useful for long-lived connections where you need to detect if the other side crashed or network disconnected.

```c
int opt = 1;
setsockopt(server_socket, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));
```

---

**`SO_RCVBUF` / `SO_SNDBUF` - Buffer sizes**

Set the size of receive/send buffers. Useful for high-throughput applications and file transfers.

```c
int buffer_size = 65536;  // 64 KB
setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size));
setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &buffer_size, sizeof(buffer_size));
```

---

**`SO_RCVTIMEO` / `SO_SNDTIMEO` - Timeouts**

Set timeout for receive/send operations to prevent `recv()` or `send()` from blocking indefinitely.

```c
struct timeval timeout;
timeout.tv_sec = 5;   // 5 seconds
timeout.tv_usec = 0;
setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
```

---

**`TCP_NODELAY` - Disable Nagle's algorithm**

Send small packets immediately instead of buffering them. Useful for real-time applications (games, VoIP) and interactive applications where latency matters more than throughput.

```c
int opt = 1;
setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
```

**Trade-off:** Lower latency but more packets (slightly less efficient).

---

#### Common Socket Options Summary

| Option | Level | Purpose | Typical Value |
|--------|-------|---------|---------------|
| `SO_REUSEADDR` | `SOL_SOCKET` | Reuse address/port | `1` (enable) |
| `SO_KEEPALIVE` | `SOL_SOCKET` | Detect dead connections | `1` (enable) |
| `SO_RCVBUF` | `SOL_SOCKET` | Receive buffer size | `65536` (64KB) |
| `SO_SNDBUF` | `SOL_SOCKET` | Send buffer size | `65536` (64KB) |
| `SO_RCVTIMEO` | `SOL_SOCKET` | Receive timeout | `struct timeval` |
| `SO_SNDTIMEO` | `SOL_SOCKET` | Send timeout | `struct timeval` |
| `TCP_NODELAY` | `IPPROTO_TCP` | Disable Nagle's algorithm | `1` (disable) |
| `SO_BROADCAST` | `SOL_SOCKET` | Allow broadcast messages | `1` (enable) |

---

### `getsockopt()`
Reads current socket options.

**Syntax:**
```c
int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
```

**Parameters:**
- `sockfd`: Socket file descriptor
- `level`: Protocol level (same as `setsockopt`)
- `optname`: The option to read
- `optval`: Pointer to buffer to store the option value
- `optlen`: Pointer to variable containing buffer size

**Returns:**
- `0` on success
- `-1` on error

**Example:**
```c
int opt;
socklen_t optlen = sizeof(opt);
getsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, &optlen);
printf("SO_REUSEADDR is %s\n", opt ? "enabled" : "disabled");
```

---

## Common Questions Answered

### Q1: What does `n` hold in `n = recv(connection_socket, buf, SIZE, 0);`?

**Answer:** `n` holds the **number of bytes actually received** from the socket. This is important because:
- It might be less than `SIZE` if fewer bytes were sent
- It will be `0` if the connection was closed gracefully
- It will be `-1` if an error occurred

You should always check this value to know how much data was actually received.

---

### Q2: What's the difference between `sockaddr` and `sockaddr_in`?

**Answer:**

**`struct sockaddr`** is a **generic** socket address structure:
```c
struct sockaddr {
    unsigned short sa_family;    // Address family (e.g., AF_INET)
    char sa_data[14];           // Protocol-specific address data
};
```

**`struct sockaddr_in`** is **IPv4-specific** and more convenient:
```c
struct sockaddr_in {
    short sin_family;           // Address family (AF_INET)
    unsigned short sin_port;    // Port number
    struct in_addr sin_addr;    // IP address
    char sin_zero[8];           // Padding to match sockaddr size
};
```

**Why both exist:**
- `sockaddr` is the generic type that socket functions accept
- `sockaddr_in` is easier to work with for IPv4
- You fill in `sockaddr_in`, then **cast** it to `sockaddr*` when calling socket functions

**Example:**
```c
struct sockaddr_in server_addr;  // Easy to fill
server_addr.sin_family = AF_INET;
server_addr.sin_port = htons(8080);
server_addr.sin_addr.s_addr = inet_addr("10.0.2.4");

// Cast to sockaddr* when calling bind
bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
```

---

### Q3: Why do we need to pass `sizeof(server_addr)` as an argument? Can't the function calculate the size itself?

**Great question!** Here's why:

**Short answer:** C doesn't have built-in type reflection. Functions can't determine the size of structures at runtime.

**Detailed explanation:**

1. **C is not Python:** In Python, everything is an object with metadata. In C, a pointer is just a memory address - it doesn't carry information about what it points to or how large that thing is.

2. **Generic functions:** Socket functions like `bind()` accept `struct sockaddr*`, which could point to:
   - `struct sockaddr_in` (IPv4, 16 bytes)
   - `struct sockaddr_in6` (IPv6, 28 bytes)
   - Other address families with different sizes
   
   The function needs to know how many bytes to read!

3. **Performance:** C prioritizes performance. Having the programmer calculate `sizeof()` at compile-time is faster than runtime reflection.

4. **Type safety:** Passing the size explicitly acts as a sanity check.

**Example showing the problem:**
```c
void print_data(void *ptr) {
    // How big is the data at ptr?
    // Could be 1 byte, 100 bytes, 1000 bytes...
    // C has NO WAY to know!
}

void print_data(void *ptr, size_t size) {
    // Now we know! We can safely read 'size' bytes
    for (int i = 0; i < size; i++) {
        printf("%d ", ((char*)ptr)[i]);
    }
}
```

---

## Python vs C Socket Programming

You're right that Python socket programming is easier! Here's why:

### Python Example:
```python
server_socket = socket(AF_INET, SOCK_STREAM)
server_socket.bind(('10.0.2.4', 8080))  # Tuple handles everything!
server_socket.listen(1)
connection_socket, addr = server_socket.accept()  # Returns tuple!
```

### C Equivalent:
```c
int server_socket = socket(AF_INET, SOCK_STREAM, 0);
struct sockaddr_in server_addr;
server_addr.sin_family = AF_INET;
server_addr.sin_port = htons(8080);
server_addr.sin_addr.s_addr = inet_addr("10.0.2.4");
bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
listen(server_socket, 1);
socklen_t addr_size = sizeof(client_addr);
connection_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_size);
```

**Why C is harder:**
- Manual memory management
- Manual byte order conversion (`htons`, `htonl`)
- Manual structure filling
- Manual type casting
- You need to know sizes explicitly

**Why learn C anyway:**
- Understand what Python does under the hood
- Better understanding of networking fundamentals
- Performance-critical applications
- Operating system programming

---
