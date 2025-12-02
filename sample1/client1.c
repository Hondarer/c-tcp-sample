#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("inet_pton failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("サーバーに接続\n");

    write(sock, "ABCD", 4);
    printf("送信: ABCD\n");

    fd_set readfds;
    struct timeval timeout;

    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);

    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    int result = select(sock + 1, &readfds, NULL, NULL, &timeout);

    if (result > 0) {
        ssize_t bytes = read(sock, buffer, BUFFER_SIZE);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            printf("受信: %s\n", buffer);
        } else {
            printf("接続が閉じられました\n");
        }
    } else if (result == 0) {
        printf("タイムアウト: 5秒以内に応答なし\n");
    } else {
        perror("select failed");
    }

    close(sock);

    return 0;
}
