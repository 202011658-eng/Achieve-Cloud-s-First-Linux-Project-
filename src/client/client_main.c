#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "../common.h"
#include "header/client_auth.h"
#include "header/client_board.h"
#include "header/client_util.h"

void printInitialMenu() {
    printf("\n========================================\n");
    printf("       온라인 게시판 시스템\n");
    printf("========================================\n");
    printf("1. 로그인\n");
    printf("2. 회원가입\n");
    printf("3. 종료\n");
    printf("========================================\n");
    printf("선택: ");
}

void printMainMenu() {
    printf("\n========================================\n");
    printf("       온라인 게시판 시스템\n");
    printf("========================================\n");
    printf("1. 글 작성\n");
    printf("2. 글 목록 보기\n");
    printf("3. 글 읽기\n");
    printf("4. 글 수정\n");
    printf("5. 글 삭제\n");
    printf("6. 접속자 목록\n");
    printf("7. 글 추천\n");
    printf("8. 인기글 보기(조회수순)\n");
    printf("9. 글 검색\n");
    printf("10. 댓글 달기\n");
    printf("11. 알림 확인\n");
    printf("12. 로그아웃\n");   
    printf("========================================\n");
    printf("선택: ");
}

int main(int argc, char* argv[]) {
    int client_sock;
    struct sockaddr_in server_addr;
    char buffer[MAX_BUFFER];

    if (argc != 2) {
        fprintf(stderr, "사용법: %s <Server IP>\n", argv[0]);
        exit(1);
    }

    char* server_ip = argv[1];

    client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock == -1) {
        handleError("socket() error");
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        handleError("inet_pton() error (잘못된 IP 주소)");
    }

    if (connect(client_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        handleError("connect() error");
    }

    printf("서버(%s:%d)에 연결되었습니다.\n", server_ip, PORT);

    int len = read(client_sock, buffer, MAX_BUFFER);
    buffer[len] = '\0';
    printf("%s\n", buffer);

    int logged_in = 0;
    int choice;

    while (!logged_in) {
        printInitialMenu();
        scanf("%d", &choice);
        getchar();

        switch (choice) {
        case 1:
            if (loginUser(client_sock)) {
                logged_in = 1;
            }
            break;
        case 2:
            registerUser(client_sock);
            break;
        case 3:
            write(client_sock, "QUIT", 4);
            read(client_sock, buffer, MAX_BUFFER);
            printf("\n게시판을 종료합니다. 안녕히 가세요!\n");
            close(client_sock);
            return 0;
        default:
            printf("\n잘못된 선택입니다. 다시 선택해주세요.\n");
        }
    }

    while (1) {
        printMainMenu();
        scanf("%d", &choice);
        getchar();

        switch (choice) {
        case 1:
            writePostClient(client_sock);
            break;
        case 2:
            listPostsClient(client_sock);
            break;
        case 3:
            readPostClient(client_sock);
            break;
        case 4:
            updatePostClient(client_sock);
            break;
        case 5:
            deletePostClient(client_sock);
            break;
        case 6:
            listOnlineUsersClient(client_sock);
            break;
        case 7:
            likePostClient(client_sock);
            break;
        case 8:
            rankPostsClient(client_sock);
            break;
        case 9:
            searchPostsClient(client_sock);
            break;
        case 10:
            commentPostClient(client_sock);
            break;
        case 11:
            showNotificationsClient(client_sock);
            break;
        case 12:  
            write(client_sock, "QUIT", 4);
            read(client_sock, buffer, MAX_BUFFER);
            printf("\n로그아웃되었습니다. 안녕히 가세요!\n");
            close(client_sock);
            return 0;
        default:
            printf("\n잘못된 선택입니다. 다시 선택해주세요.\n");
        }
    }

    close(client_sock);
    return 0;
}