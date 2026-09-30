#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[]) {

    if(argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    /* The second parameter pass to the terminal is server port. */
    /* Need to convert it to int (string by default). */
    int port = atoi(argv[1]);

    /* PHASE 1. */
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_fd < 0) {
        perror("socket()");
        return -1;
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* PHASE 2. */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;   /* 0.0.0.0 */
    server_addr.sin_port = htons(port);         /* host-to-network byte */

    /* PHASE 3. */
    if(bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind()");
        return 1;
    }

    /* PHASE 4. */
    if(listen(listen_fd, 5) < 0) {
        perror("listen()");
        return 1;
    }

    printf("Listening on port %d...\n", port);

    while(1) {
        /* Buffer to storage client properties. */
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        char client_ip[INET_ADDRSTRLEN];

        /* PHASE 5. */
        int client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);
        // Fail thì bỏ connection này.
        if(client_fd < 0) {
            perror("accept()");
            continue;
        }

        // Chỉ để in ra thôi.
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        printf("Client connected: %s:%d\n", client_ip, ntohs(client_addr.sin_port));

        /* Handle data from client. */
        char buf[BUF_SIZE];     /* Storage data from connection. */
        ssize_t n;
        while((n = recv(client_fd, buf, sizeof(buf) - 1, 0)) > 0) {
            buf[n] = '\0';
            printf("Received: %s", buf);

            const char *resp = "Done.\n";
            send(client_fd, resp, strlen(resp), 0);
        }

        if(n == 0) {
            printf("Client disconnected.\n");
        } else {
            perror("recv()");
        }

        close(client_fd);
    }

    close(listen_fd);
    return 0;
}