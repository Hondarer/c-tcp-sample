#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>

#define PORT 8082
#define BUFFER_SIZE 1024
#define SERVER_ADDR "127.0.0.1"  // 接続先IPアドレス

void print_timestamp() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm *tm_info = localtime(&tv.tv_sec);
    printf("[%02d:%02d:%02d.%03ld] ",
           tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
           tv.tv_usec / 1000);
}

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

    if (inet_pton(AF_INET, SERVER_ADDR, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    print_timestamp();
    printf("サーバーに接続\n");

    write(sock, "ABCD", 4);
    print_timestamp();
    printf("送信: ABCD\n");

    fd_set readfds;
    struct timeval timeout;

    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);

    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    int result = select(sock + 1, &readfds, NULL, NULL, &timeout);
    int select_errno = errno;
    print_timestamp();
    printf("[DEBUG] select() result=%d, errno=%d (%s)\n",
           result, select_errno, strerror(select_errno));

    if (result > 0) {
        errno = 0;
        ssize_t bytes = read(sock, buffer, BUFFER_SIZE);
        int read_errno = errno;
        print_timestamp();
        printf("[DEBUG] read() bytes=%zd, errno=%d (%s)\n",
               bytes, read_errno, strerror(read_errno));

        if (bytes > 0) {
            buffer[bytes] = '\0';
            print_timestamp();
            printf("受信: %s\n", buffer);
        } else if (bytes == 0) {
            print_timestamp();
            printf("接続が閉じられました (EOF)\n");
        } else {
            print_timestamp();
            printf("read() エラー: %s\n", strerror(read_errno));
        }
    } else if (result == 0) {
        print_timestamp();
        printf("タイムアウト: 5秒以内に応答なし\n");
    } else {
        print_timestamp();
        printf("select() エラー: %s\n", strerror(select_errno));
    }

    close(sock);

    return 0;
}
