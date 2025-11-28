#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <termios.h>

#define PORT 9000
#define MAX_BUFFER 4096

void handleError(const char* message) {
    perror(message);
    exit(1);
}

void getPassword(char* password, int max_len) {
    struct termios oldt, newt;
    int i = 0;
    char ch;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    while (i < max_len - 1) {
        ch = getchar();
        if (ch == '\n' || ch == '\r') {
            break;
        }
        password[i++] = ch;
        printf("*");
    }
    password[i] = '\0';
    printf("\n");

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

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
    printf("7. 로그아웃\n");
    printf("8. 글 추천\n");
    printf("9. 인기글 보기(추천순)\n");
    printf("10. 글 검색\n");
    printf("11. 댓글 달기\n");
    printf("========================================\n");
    printf("선택: ");
}

/* ===========================
 * 회원가입
 * =========================== */

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

/* ===========================
 * 로그인
 * =========================== */

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

/* ===========================
 * 글 작성
 * =========================== */

void writePostClient(int sock) {
    char buffer[MAX_BUFFER];

    write(sock, "WRITE", 5);

    read(sock, buffer, MAX_BUFFER);
    printf("\n=== 글 작성 ===\n");
    printf("제목을 입력하세요: ");
    fgets(buffer, 100, stdin);
    buffer[strcspn(buffer, "\n")] = '\0';
    write(sock, buffer, strlen(buffer));

    int len = read(sock, buffer, MAX_BUFFER);
    buffer[len] = '\0';

    if (strncmp(buffer, "ERROR", 5) == 0) {
        printf("\n✗ %s", buffer + 6);
        return;
    }

    printf("내용을 입력하세요 (최대 500자):\n");
    fgets(buffer, 500, stdin);
    buffer[strcspn(buffer, "\n")] = '\0';
    write(sock, buffer, strlen(buffer));

    len = read(sock, buffer, MAX_BUFFER);
    buffer[len] = '\0';

    if (strncmp(buffer, "SUCCESS", 7) == 0) {
        printf("\n✓ %s", buffer + 8);
    } else {
        printf("\n✗ %s", buffer + 6);
    }
}

/* ===========================
 * 글 목록
 * =========================== */

void listPostsClient(int sock) {
    char buffer[MAX_BUFFER];

    write(sock, "LIST", 4);

    int len = read(sock, buffer, MAX_BUFFER);
    buffer[len] = '\0';

    printf("\n%s\n", buffer);
}

/* ===========================
 * 글 읽기 (댓글 포함)
 * =========================== */

void readPostClient(int sock) {
    char buffer[MAX_BUFFER];
    int post_id;

    printf("\n읽을 게시글 번호를 입력하세요: ");
    scanf("%d", &post_id);
    getchar();

    snprintf(buffer, MAX_BUFFER, "READ:%d", post_id);
    write(sock, buffer, strlen(buffer));

    int len = read(sock, buffer, MAX_BUFFER);
    buffer[len] = '\0';

    printf("\n%s\n", buffer);
}

/* ===========================
 * 글 삭제
 * =========================== */

void deletePostClient(int sock) {
    char buffer[MAX_BUFFER];
    int post_id;

    printf("\n삭제할 게시글 번호를 입력하세요: ");
    scanf("%d", &post_id);
    getchar();

    printf("정말 삭제하시겠습니까? (y/n): ");
    char confirm;
    scanf("%c", &confirm);
    getchar();

    if (confirm != 'y' && confirm != 'Y') {
        printf("삭제가 취소되었습니다.\n");
        return;
    }

    snprintf(buffer, MAX_BUFFER, "DELETE:%d", post_id);
    write(sock, buffer, strlen(buffer));

    int len = read(sock, buffer, MAX_BUFFER);
    buffer[len] = '\0';

    if (strncmp(buffer, "SUCCESS", 7) == 0) {
        printf("\n✓ %s", buffer + 8);
    } else {
        printf("\n✗ %s", buffer + 6);
    }
}

/* ===========================
 * 글 수정
 * =========================== */

void updatePostClient(int sock) {
    char buffer[MAX_BUFFER];
    int post_id;

    printf("\n수정할 게시글 번호를 입력하세요: ");
    scanf("%d", &post_id);
    getchar();

    snprintf(buffer, MAX_BUFFER, "UPDATE:%d", post_id);
    write(sock, buffer, strlen(buffer));

    int len = read(sock, buffer, MAX_BUFFER);
    buffer[len] = '\0';

    if (strncmp(buffer, "ERROR", 5) == 0) {
        printf("\n✗ %s", buffer + 6);
        return;
    }

    printf("\n=== 글 수정 ===\n");
    printf("새로운 제목을 입력하세요: ");
    fgets(buffer, 100, stdin);
    buffer[strcspn(buffer, "\n")] = '\0';
    write(sock, buffer, strlen(buffer));

    len = read(sock, buffer, MAX_BUFFER);
    buffer[len] = '\0';
    if (strncmp(buffer, "ERROR", 5) == 0) {
        printf("\n✗ %s", buffer + 6);
        return;
    }

    printf("새로운 내용을 입력하세요 (최대 500자):\n");
    fgets(buffer, 500, stdin);
    buffer[strcspn(buffer, "\n")] = '\0';
    write(sock, buffer, strlen(buffer));

    len = read(sock, buffer, MAX_BUFFER);
    buffer[len] = '\0';

    if (strncmp(buffer, "SUCCESS", 7) == 0) {
        printf("\n✓ %s", buffer + 8);
    } else {
        printf("\n✗ %s", buffer + 6);
    }
}

/* ===========================
 * 접속자 목록
 * =========================== */

void listOnlineUsersClient(int sock) {
    char buffer[MAX_BUFFER];

    write(sock, "ONLINE", 6);

    int len = read(sock, buffer, MAX_BUFFER);
    buffer[len] = '\0';

    printf("\n%s\n", buffer);
}

/* ===========================
 * 추천
 * =========================== */

void likePostClient(int sock) {
    char buffer[MAX_BUFFER];
    int post_id;

    printf("\n추천할 게시글 번호를 입력하세요: ");
    scanf("%d", &post_id);
    getchar();

    snprintf(buffer, MAX_BUFFER, "LIKE:%d", post_id);
    write(sock, buffer, strlen(buffer));

    int len = read(sock, buffer, MAX_BUFFER);
    buffer[len] = '\0';

    if (strncmp(buffer, "SUCCESS", 7) == 0) {
        printf("\n✓ %s", buffer + 8);
    } else {
        printf("\n✗ %s", buffer + 6);
    }
}

/* ===========================
 * 인기글
 * =========================== */

void rankPostsClient(int sock) {
    char buffer[MAX_BUFFER];

    write(sock, "RANK", 4);

    int len = read(sock, buffer, MAX_BUFFER);
    buffer[len] = '\0';

    printf("\n%s\n", buffer);
}

/* ===========================
 * 검색
 * =========================== */

void searchPostsClient(int sock) {
    char buffer[MAX_BUFFER];

    printf("\n검색할 키워드를 입력하세요: ");
    fgets(buffer, 100, stdin);
    buffer[strcspn(buffer, "\n")] = '\0';

    char sendbuf[MAX_BUFFER];
    snprintf(sendbuf, MAX_BUFFER, "SEARCH:%s", buffer);
    write(sock, sendbuf, strlen(sendbuf));

    int len = read(sock, buffer, MAX_BUFFER);
    buffer[len] = '\0';

    printf("\n%s\n", buffer);
}

/* ===========================
 * 댓글 달기
 * =========================== */

void commentPostClient(int sock) {
    char buffer[MAX_BUFFER];
    int post_id;

    printf("\n댓글을 달 게시글 번호를 입력하세요: ");
    scanf("%d", &post_id);
    getchar();

    snprintf(buffer, MAX_BUFFER, "COMMENT:%d", post_id);
    write(sock, buffer, strlen(buffer));

    int len = read(sock, buffer, MAX_BUFFER);
    buffer[len] = '\0';

    if (strncmp(buffer, "ERROR", 5) == 0) {
        printf("\n✗ %s", buffer + 6);
        return;
    }

    if (strncmp(buffer, "CONTENT", 7) != 0) {
        printf("\n서버 응답이 올바르지 않습니다.\n");
        return;
    }

    printf("\n=== 댓글 작성 ===\n");
    printf("댓글 내용을 입력하세요 (최대 300자):\n");
    fgets(buffer, 300, stdin);
    buffer[strcspn(buffer, "\n")] = '\0';
    write(sock, buffer, strlen(buffer));

    len = read(sock, buffer, MAX_BUFFER);
    buffer[len] = '\0';

    if (strncmp(buffer, "SUCCESS", 7) == 0) {
        printf("\n✓ %s", buffer + 8);
    } else if (strncmp(buffer, "ERROR", 5) == 0) {
        printf("\n✗ %s", buffer + 6);
    } else {
        printf("\n알 수 없는 서버 응답: %s\n", buffer);
    }
}

/* ===========================
 * main
 * =========================== */

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
            write(client_sock, "QUIT", 4);
            read(client_sock, buffer, MAX_BUFFER);
            printf("\n로그아웃되었습니다. 안녕히 가세요!\n");
            close(client_sock);
            return 0;
        case 8:
            likePostClient(client_sock);
            break;
        case 9:
            rankPostsClient(client_sock);
            break;
        case 10:
            searchPostsClient(client_sock);
            break;
        case 11:
            commentPostClient(client_sock);
            break;
        default:
            printf("\n잘못된 선택입니다. 다시 선택해주세요.\n");
        }
    }

    close(client_sock);
    return 0;
}
