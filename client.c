#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
    if(argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>.\n", argv[0]);
        return 1;
    }

    struct in_addr server_ip;
    if(inet_pton(AF_INET, argv[1], &server_ip) <= 0) {
        fprintf(stderr, "Invalid address: %s.\n", argv[1]);
        return 1;
    }
    int port = atoi(argv[2]);           /* Convert string to int (port).*/



    /* PHASE 1. Create TCP socket. */
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_fd < 0) {
        perror("socket()");
        return 1;
    }

    /* PHASE 2. Fill in the server address. */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr = server_ip;

    /* PHASE 3. Actively open a connection to the server. */
    // 3-way handshake.
    if(connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect()");
        close(sock_fd);
        return 1;
    }

    printf("Connected to %s:%d. Type a message and press Enter (Ctrl+D to quit).\n", argv[1], port);

    char input[BUF_SIZE];
    char reply[BUF_SIZE];

    /* PHASE 4. Send message and receive response */
    while(fgets(input, sizeof(input), stdin) != NULL) {
        size_t len = strlen(input);

        if(send(sock_fd, input, len, 0) < 0) {
            perror("send()");
            break;
        }

        ssize_t n = recv(sock_fd, reply, sizeof(reply) - 1, 0);
        if(n <= 0) {
            printf("Server closed the connection.\n");
            break;
        }
        reply[n] = '\0';
        printf("Response: %s", reply);
    }

    close(sock_fd);
    return 0;
}