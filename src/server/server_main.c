#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include "../common.h"
#include "header/server_util.h"
#include "header/server_user.h"
#include "header/server_board.h"
#include "header/server_noti.h"

int g_server_sock = -1;

void sigint_handler(int sig) {
    printf("\n[서버] SIGINT 수신, 서버를 종료합니다...\n");
    if (g_server_sock != -1) {
        close(g_server_sock);
        g_server_sock = -1;
    }
    exit(0);
}

void handleClient(int client_sock, char *client_ip) {
    char buffer[MAX_BUFFER];
    char username[50] = "";
    char nickname[50] = "";
    int logged_in = 0;

    printf("[PID %d] 클라이언트 %s 처리 시작\n", getpid(), client_ip);

    const char *welcome = "온라인 게시판에 오신 것을 환영합니다!\n";
    write(client_sock, welcome, strlen(welcome));

    while (1) {
        memset(buffer, 0, MAX_BUFFER);
        int len = read(client_sock, buffer, MAX_BUFFER);

        if (len <= 0) {
            if (logged_in) {
                removeOnlineUser(username);
            }
            printf("[PID %d] 클라이언트 %s 연결 종료\n", getpid(), client_ip);
            break;
        }

        buffer[len] = '\0';
        printf("[PID %d] %s 요청: %s\n", getpid(), client_ip, buffer);

        if (strncmp(buffer, "REGISTER", 8) == 0) {
            registerUser(client_sock);
        }
        else if (strncmp(buffer, "LOGIN", 5) == 0) {
            if (loginUser(client_sock, username, nickname, client_ip)) {
                logged_in = 1;
            }
        }
        else if (strncmp(buffer, "QUIT", 4) == 0) {
            if (logged_in) {
                removeOnlineUser(username);
            }
            write(client_sock, "BYE", 3);
            printf("[PID %d] 클라이언트 %s 정상 종료\n", getpid(), client_ip);
            break;
        }
        else if (!logged_in) {
            write(client_sock, "ERROR|로그인이 필요합니다.\n",
                  strlen("ERROR|로그인이 필요합니다.\n"));
        }
        else if (strncmp(buffer, "WRITE", 5) == 0) {
            writePost(client_sock, nickname);
        }
        else if (strncmp(buffer, "LIST", 4) == 0) {
            listPosts(client_sock);
        }
        else if (strncmp(buffer, "READ:", 5) == 0) {
            int post_id = atoi(buffer + 5);
            readPost(client_sock, post_id);
        }
        else if (strncmp(buffer, "DELETE:", 7) == 0) {
            int post_id = atoi(buffer + 7);
            deletePost(client_sock, post_id, nickname);
        }
        else if (strncmp(buffer, "ONLINE", 6) == 0) {
            listOnlineUsers(client_sock);
        }
        else if (strncmp(buffer, "NOTI", 4) == 0) {
            showNotifications(client_sock, nickname);
        }
        else if (strncmp(buffer, "UPDATE:", 7) == 0) {
            int post_id = atoi(buffer + 7);
            updatePost(client_sock, post_id, nickname);
        }
        else if (strncmp(buffer, "LIKE:", 5) == 0) {
            int post_id = atoi(buffer + 5);
            likePost(client_sock, post_id, nickname);
        }
        else if (strncmp(buffer, "RANK", 4) == 0) {
            rankPosts(client_sock);
        }
        else if (strncmp(buffer, "SEARCH:", 7) == 0) {
            char *keyword = buffer + 7;
            while (*keyword == ' ') keyword++;
            searchPosts(client_sock, keyword);
        }
        else if (strncmp(buffer, "COMMENT:", 8) == 0) {
            int post_id = atoi(buffer + 8);
            addComment(client_sock, post_id, nickname);
        }
        else {
            write(client_sock, "ERROR|알 수 없는 명령입니다.\n",
                  strlen("ERROR|알 수 없는 명령입니다.\n"));
        }
    }

    close(client_sock);
    exit(0);
}

int main(int argc, char *argv[]) {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_size;
    pid_t pid;
    
    FILE *fp = fopen(ONLINE_FILE, "w");
    if (fp != NULL) {
        fclose(fp);
    }

    signal(SIGINT, sigint_handler);
    signal(SIGCHLD, sigchld_handler);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        handleError("socket() error");
    }

    g_server_sock = server_sock;

    int option = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        handleError("bind() error");
    }

    if (listen(server_sock, 5) == -1) {
        handleError("listen() error");
    }

    printf("========================================\n");
    printf("온라인 게시판 서버가 시작되었습니다.\n");
    printf("포트: %d\n", PORT);
    printf("회원가입/로그인 시스템 활성화\n");
    printf("욕설 필터링 활성화\n");
    printf("추천/조회수/검색/인기글/댓글 기능 활성화\n");
    printf("========================================\n");

    while (1) {
        client_addr_size = sizeof(client_addr);
        client_sock = accept(server_sock,
                             (struct sockaddr *)&client_addr,
                             &client_addr_size);

        if (client_sock == -1) {
            perror("accept() error");
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        printf("\n[부모 프로세스] 새 클라이언트 접속: %s\n", client_ip);

        pid = fork();

        if (pid == -1) {
            perror("fork() error");
            close(client_sock);
            continue;
        } else if (pid == 0) {
            signal(SIGINT, SIG_DFL);

            close(server_sock);
            handleClient(client_sock, client_ip);
        } else {
            close(client_sock);
            printf("[부모 프로세스] 자식 프로세스 생성 (PID: %d)\n", pid);
        }
    }

    close(server_sock);
    return 0;
}