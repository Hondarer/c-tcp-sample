#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>

#define PORT 8082
#define BUFFER_SIZE 1024
#define BIND_ADDR "127.0.0.1"  // バインドするIPアドレス

void print_timestamp() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm *tm_info = localtime(&tv.tv_sec);
    printf("[%02d:%02d:%02d.%03ld] ",
           tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
           tv.tv_usec / 1000);
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    if (inet_pton(AF_INET, BIND_ADDR, &address.sin_addr) <= 0) {
        perror("inet_pton failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    print_timestamp();
    printf("サーバー起動: ポート %d で待機中\n", PORT);

    client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
    if (client_fd < 0) {
        perror("accept failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    print_timestamp();
    printf("クライアント接続\n");

    ssize_t bytes = read(client_fd, buffer, BUFFER_SIZE);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        print_timestamp();
        printf("受信: %s\n", buffer);

        if (strcmp(buffer, "ABCD") == 0) {
            write(client_fd, "CDEF", 4);
            print_timestamp();
            printf("送信: CDEF\n");
        } else {
            write(client_fd, "ERROR", 5);
            print_timestamp();
            printf("送信: ERROR\n");
        }
    }

    // RST送信による強制切断
    struct linger so_linger;
    so_linger.l_onoff = 1;   // lingerを有効化
    so_linger.l_linger = 0;  // タイムアウト0秒 = RST送信
    setsockopt(client_fd, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger));
    print_timestamp();
    printf("RST送信による強制切断を実行\n");

    close(client_fd);
    close(server_fd);

    return 0;
}
