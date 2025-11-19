#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 9000
#define MAX_BUFFER 4096

void handleError(const char* message) {
    perror(message);
    exit(1);
}

// 메뉴 출력
void printMenu() {
    printf("\n========================================\n");
    printf("       온라인 게시판 시스템\n");
    printf("========================================\n");
    printf("1. 글 작성\n");
    printf("2. 글 목록 보기\n");
    printf("3. 글 읽기\n");
    printf("4. 글 삭제\n");
    printf("5. 종료\n");
    printf("========================================\n");
    printf("선택: ");
}

// 게시글 작성
void writePost(int sock) {
    char buffer[MAX_BUFFER];

    // 서버에 작성 요청
    write(sock, "WRITE", 5);

    // 제목 입력
    read(sock, buffer, MAX_BUFFER); // "TITLE" 수신
    printf("\n제목을 입력하세요: ");
    fgets(buffer, 100, stdin);
    buffer[strcspn(buffer, "\n")] = '\0'; // 개행 제거
    write(sock, buffer, strlen(buffer));

    // 작성자 입력
    read(sock, buffer, MAX_BUFFER); // "AUTHOR" 수신
    printf("작성자를 입력하세요: ");
    fgets(buffer, 50, stdin);
    buffer[strcspn(buffer, "\n")] = '\0';
    write(sock, buffer, strlen(buffer));

    // 내용 입력
    read(sock, buffer, MAX_BUFFER); // "CONTENT" 수신
    printf("내용을 입력하세요 (최대 500자):\n");
    fgets(buffer, 500, stdin);
    buffer[strcspn(buffer, "\n")] = '\0';
    write(sock, buffer, strlen(buffer));

    // 결과 수신
    int len = read(sock, buffer, MAX_BUFFER);
    buffer[len] = '\0';
    printf("\n%s\n", buffer);
}

// 게시글 목록 보기
void listPosts(int sock) {
    char buffer[MAX_BUFFER];

    write(sock, "LIST", 4);

    int len = read(sock, buffer, MAX_BUFFER);
    buffer[len] = '\0';

    printf("\n%s\n", buffer);
}

// 게시글 읽기
void readPost(int sock) {
    char buffer[MAX_BUFFER];
    int post_id;

    printf("\n읽을 게시글 번호를 입력하세요: ");
    scanf("%d", &post_id);
    getchar(); // 버퍼 비우기

    snprintf(buffer, MAX_BUFFER, "READ:%d", post_id);
    write(sock, buffer, strlen(buffer));

    int len = read(sock, buffer, MAX_BUFFER);
    buffer[len] = '\0';

    printf("\n%s\n", buffer);
}

// 게시글 삭제
void deletePost(int sock) {
    char buffer[MAX_BUFFER];
    int post_id;

    printf("\n삭제할 게시글 번호를 입력하세요: ");
    scanf("%d", &post_id);
    getchar(); // 버퍼 비우기

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

    printf("\n%s\n", buffer);
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

    // 클라이언트 소켓 생성
    client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock == -1) {
        handleError("socket() error");
    }

    // 서버 주소 설정
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        handleError("inet_pton() error (잘못된 IP 주소)");
    }

    // 서버에 연결
    if (connect(client_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        handleError("connect() error");
    }

    printf("서버(%s:%d)에 연결되었습니다.\n", server_ip, PORT);

    // 환영 메시지 수신
    int len = read(client_sock, buffer, MAX_BUFFER);
    buffer[len] = '\0';
    printf("%s\n", buffer);

    // 메인 루프
    int choice;
    while (1) {
        printMenu();
        scanf("%d", &choice);
        getchar(); // 버퍼 비우기

        switch (choice) {
        case 1:
            writePost(client_sock);
            break;
        case 2:
            listPosts(client_sock);
            break;
        case 3:
            readPost(client_sock);
            break;
        case 4:
            deletePost(client_sock);
            break;
        case 5:
            write(client_sock, "QUIT", 4);
            read(client_sock, buffer, MAX_BUFFER);
            printf("\n게시판을 종료합니다. 안녕히 가세요!\n");
            close(client_sock);
            return 0;
        default:
            printf("\n잘못된 선택입니다. 다시 선택해주세요.\n");
        }
    }

    close(client_sock);
    return 0;
}