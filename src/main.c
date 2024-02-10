#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 42069
#define BUFFER_LEN 1024

int main(int argc, char *argv[]) {

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_in host_addr;
    socklen_t host_addr_len = sizeof(host_addr);

    host_addr.sin_family = AF_INET;
    host_addr.sin_port = htons(PORT);
    host_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(socket_fd, (struct sockaddr *)&host_addr, host_addr_len) == -1) {
        perror("bind");
        return EXIT_FAILURE;
    }

    if (listen(socket_fd, 128) == -1) {
        perror("listen");
        return EXIT_FAILURE;
    }

    printf("listening on port %d...\n", PORT);

    char buffer[BUFFER_LEN];
    char resp[] = "HTTP/1.0 200 OK\r\n"
                    "Server: webserver-c\r\n"
                    "Content-type: text/html\r\n\r\n"
                    "<html>hello, world</html>\r\n";

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    while (1) {
        int conn_fd = accept(socket_fd, (struct sockaddr *)&host_addr, &host_addr_len);
        if (conn_fd == -1) {
            perror("accept");
            continue;
        }

        // getpeername?
        if (getsockname(conn_fd, (struct sockaddr *)&client_addr, &client_addr_len) == -1) {
            perror("client name");
            continue;
        }

        printf("[%s:%u] - connection accepted\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        ssize_t num_read = read(conn_fd, (void*)buffer, BUFFER_LEN);
        if (num_read == -1) {
            perror("read");
            // close?
            continue;
        }

        printf("[%s:%u] - %s\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), buffer);

        ssize_t num_write = write(conn_fd, resp, strlen(resp));
        if (num_write == -1) {
            perror("write");
            // close?
            continue;
        }

        close(conn_fd);
    }

    return EXIT_SUCCESS;
}
