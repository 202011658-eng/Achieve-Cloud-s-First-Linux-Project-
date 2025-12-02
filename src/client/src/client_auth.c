#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../common.h"
#include "../header/client_auth.h"
#include "../header/client_util.h"

void registerUser(int sock) {
    char buffer[MAX_BUFFER];

    write(sock, "REGISTER", 8);

    read(sock, buffer, MAX_BUFFER);
    printf("\n=== 회원가입 ===\n");
    printf("아이디를 입력하세요 (영문, 숫자): ");
    fgets(buffer, 50, stdin);
    buffer[strcspn(buffer, "\n")] = '\0';
    write(sock, buffer, strlen(buffer));

    read(sock, buffer, MAX_BUFFER);
    printf("비밀번호를 입력하세요: ");
    getPassword(buffer, 50);
    write(sock, buffer, strlen(buffer));

    read(sock, buffer, MAX_BUFFER);
    printf("닉네임을 입력하세요: ");
    fgets(buffer, 50, stdin);
    buffer[strcspn(buffer, "\n")] = '\0';
    write(sock, buffer, strlen(buffer));

    int len = read(sock, buffer, MAX_BUFFER);
    buffer[len] = '\0';

    if (strncmp(buffer, "SUCCESS", 7) == 0) {
        printf("\n✓ %s", buffer + 8);
    } else {
        printf("\n✗ %s", buffer + 6);
    }
}

int loginUser(int sock) {
    char buffer[MAX_BUFFER];

    write(sock, "LOGIN", 5);

    read(sock, buffer, MAX_BUFFER);
    printf("\n=== 로그인 ===\n");
    printf("아이디: ");
    fgets(buffer, 50, stdin);
    buffer[strcspn(buffer, "\n")] = '\0';
    write(sock, buffer, strlen(buffer));

    read(sock, buffer, MAX_BUFFER);
    printf("비밀번호: ");
    getPassword(buffer, 50);
    write(sock, buffer, strlen(buffer));

    int len = read(sock, buffer, MAX_BUFFER);
    buffer[len] = '\0';

    if (strncmp(buffer, "SUCCESS", 7) == 0) {
        printf("\n✓ %s", buffer + 8);
        return 1;
    } else {
        printf("\n✗ %s", buffer + 6);
        return 0;
    }
}