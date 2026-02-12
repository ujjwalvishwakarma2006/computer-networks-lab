#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

#define SIZE 1024

void send_file(FILE* fp, int sockfd) {
    int n;
    char data[SIZE] = {0};

    while(fgets(data, SIZE, fp) != NULL) {
        if (send(sockfd, data, sizeof(data), 0) == -1) {
            perror("[ERROR] Sending file");
            exit(1);
        }
        bzero(data, SIZE);
    }
}

int main(void) {
    char* server_ip = "10.0.2.4";
    const int server_port = 8080;
    int e;

    // Define variables
    int sockfd;
    struct sockaddr_in server_addr;
    FILE* fp;
    char* filename = "send.txt";

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
	perror("[ERROR] Socket Initialization Error\n");
	exit(1);
    }
    printf("[INIT] Client Socket Initialized\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = server_port;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    e = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (e == -1) {
	perror("[ERROR] Server Connection Error\n");
	exit(1);
    }
    printf("[CONNECTED] Connection Established with server\n");

    fp = fopen(filename, "r");
    if (fp == NULL) {
	perror("[ERROR] File to be sent not found\n");
	exit(1);
    }

    send_file(fp, sockfd);
    printf("[DONE] File data sent successfully\n");
    return 0;
}
