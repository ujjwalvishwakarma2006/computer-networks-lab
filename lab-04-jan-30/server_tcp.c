#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#define SIZE 1024

void write_file(int sockfd) {
    // Initialize variables
    int n;
    FILE *fp;
    char *filename = "recv.txt";
    char buffer[SIZE];

    fp = fopen(filename, "w");
    while(1) {
	n = recv(sockfd, buffer, SIZE, 0);
	if (n <= 0) {
	    break;
	    return;
	}
	fprintf(fp, "%s", buffer);
	bzero(buffer, SIZE);
    }
    return;
}

int main () {
    char* server_ip = "10.0.2.4";
    const int server_port = 8080;
    int e;

    // Initialize variables
    int sockfd, conn_sock;
    struct sockaddr_in server_addr, conn_addr;
    socklen_t addr_size;
    char buffer[SIZE];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
	perror("[ERROR] socket initialization error\n");
	exit(1);
    }
    printf("[INIT] Server Socket created successfully\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = server_port;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    e = bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (e < 0) {
	perror("[ERROR] Socket bind error\n");
	exit(1);
    }
    printf("[BIND] Binding Successful\n");

    if (listen(sockfd, 10) == 0) {
	printf("[LISTEN] Listening...\n");
    } else {
	perror("[ERROR] Listening Error\n");
	exit(1);
    }

    addr_size = sizeof(conn_addr);
    conn_sock = accept(sockfd, (struct sockaddr*)&conn_addr, &addr_size);
    write_file(conn_sock);
    printf("[DONE] Data written successfully\n");

    return 0;
}
