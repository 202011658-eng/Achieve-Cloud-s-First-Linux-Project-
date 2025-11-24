#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <termios.h>

#define PORT 9000
#define MAX_BUFFER 4096

/* ============================================================
 * 1. 유틸리티 함수
 * ============================================================ */

void handleError(const char* message) {
    perror(message);
    exit(1);
}

// 비밀번호 입력 (화면에 표시 안 함)
void getPassword(char* password, int max_len) {
    struct termios oldt, newt;
    int i = 0;
    char ch;

    // 터미널 설정 백업
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO); // ECHO 끄기
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    // 비밀번호 입력
    while (i < max_len - 1) {
        ch = getchar();
        if (ch == '\n' || ch == '\r') {
            break;
        }
        password[i++] = ch;
        printf("*"); // 별표 출력
    }
    password[i] = '\0';
    printf("\n");

    // 터미널 설정 복원
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

/* ============================================================
 * 2. 메뉴 출력 함수
 * ============================================================ */

// 초기 메뉴 (로그인 전)
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

// 메인 메뉴 (로그인 후)
void printMainMenu() {
    printf("\n========================================\n");
    printf("       온라인 게시판 시스템\n");
    printf("========================================\n");
    printf("1. 글 작성\n");
    printf("2. 글 목록 보기\n");
    printf("3. 글 읽기\n");
    printf("4. 글 수정\n");  // [추가]
    printf("5. 글 삭제\n");  // [번호 밀림]
    printf("6. 접속자 목록\n");
    printf("7. 로그아웃\n");
    printf("========================================\n");
    printf("선택: ");
}

/* ============================================================
 * 3. 서버 통신 함수
 * ============================================================ */

// 회원가입
void registerUser(int sock) {
    char buffer[MAX_BUFFER];

    write(sock, "REGISTER", 8);

    // 아이디 입력
    read(sock, buffer, MAX_BUFFER); // "USERNAME" 수신
    printf("\n=== 회원가입 ===\n");
    printf("아이디를 입력하세요 (영문, 숫자): ");
    fgets(buffer, 50, stdin);
    buffer[strcspn(buffer, "\n")] = '\0';
    write(sock, buffer, strlen(buffer));

    // 비밀번호 입력
    read(sock, buffer, MAX_BUFFER); // "PASSWORD" 수신
    printf("비밀번호를 입력하세요: ");
    getPassword(buffer, 50);
    write(sock, buffer, strlen(buffer));

    // 닉네임 입력
    read(sock, buffer, MAX_BUFFER); // "NICKNAME" 수신
    printf("닉네임을 입력하세요: ");
    fgets(buffer, 50, stdin);
    buffer[strcspn(buffer, "\n")] = '\0';
    write(sock, buffer, strlen(buffer));

    // 결과 수신
    int len = read(sock, buffer, MAX_BUFFER);
    buffer[len] = '\0';
    
    if (strncmp(buffer, "SUCCESS", 7) == 0) {
        printf("\n✓ %s", buffer + 8);
    } else {
        printf("\n✗ %s", buffer + 6);
    }
}

// 로그인
int loginUser(int sock) {
    char buffer[MAX_BUFFER];

    write(sock, "LOGIN", 5);

    // 아이디 입력
    read(sock, buffer, MAX_BUFFER); // "USERNAME" 수신
    printf("\n=== 로그인 ===\n");
    printf("아이디: ");
    fgets(buffer, 50, stdin);
    buffer[strcspn(buffer, "\n")] = '\0';
    write(sock, buffer, strlen(buffer));

    // 비밀번호 입력
    read(sock, buffer, MAX_BUFFER); // "PASSWORD" 수신
    printf("비밀번호: ");
    getPassword(buffer, 50);
    write(sock, buffer, strlen(buffer));

    // 결과 수신
    int len = read(sock, buffer, MAX_BUFFER);
    buffer[len] = '\0';

    if (strncmp(buffer, "SUCCESS", 7) == 0) {
        printf("\n✓ %s", buffer + 8);
        return 1; // 로그인 성공
    } else {
        printf("\n✗ %s", buffer + 6);
        return 0; // 로그인 실패
    }
}

// 게시글 작성
void writePost(int sock) {
    char buffer[MAX_BUFFER];

    write(sock, "WRITE", 5);

    // 제목 입력
    read(sock, buffer, MAX_BUFFER); // "TITLE" 수신
    printf("\n=== 글 작성 ===\n");
    printf("제목을 입력하세요: ");
    fgets(buffer, 100, stdin);
    buffer[strcspn(buffer, "\n")] = '\0';
    write(sock, buffer, strlen(buffer));

    // 서버 응답 체크 (제목 욕설 필터링)
    int len = read(sock, buffer, MAX_BUFFER);
    buffer[len] = '\0';
    
    if (strncmp(buffer, "ERROR", 5) == 0) {
        printf("\n✗ %s", buffer + 6);
        return;
    }

    // 내용 입력
    printf("내용을 입력하세요 (최대 500자):\n");
    fgets(buffer, 500, stdin);
    buffer[strcspn(buffer, "\n")] = '\0';
    write(sock, buffer, strlen(buffer));

    // 결과 수신
    len = read(sock, buffer, MAX_BUFFER);
    buffer[len] = '\0';
    
    if (strncmp(buffer, "SUCCESS", 7) == 0) {
        printf("\n✓ %s", buffer + 8);
    } else {
        printf("\n✗ %s", buffer + 6);
    }
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
    getchar();

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

void updatePost(int sock) {
    char buffer[MAX_BUFFER];
    int post_id;

    printf("\n수정할 게시글 번호를 입력하세요: ");
    scanf("%d", &post_id);
    getchar(); // 버퍼 비우기

    // UPDATE 명령 전송
    snprintf(buffer, MAX_BUFFER, "UPDATE:%d", post_id);
    write(sock, buffer, strlen(buffer));

    // 수정 가능 여부 확인 (작성자 확인 등)
    int len = read(sock, buffer, MAX_BUFFER);
    buffer[len] = '\0';

    if (strncmp(buffer, "ERROR", 5) == 0) {
        printf("\n✗ %s", buffer + 6);
        return;
    }

    // 여기서부터는 writePost와 유사한 로직
    // 제목 입력 요청 수신 ("TITLE")
    read(sock, buffer, MAX_BUFFER); 
    printf("\n=== 글 수정 ===\n");
    printf("새로운 제목을 입력하세요: ");
    fgets(buffer, 100, stdin);
    buffer[strcspn(buffer, "\n")] = '\0';
    write(sock, buffer, strlen(buffer));

    // 제목 에러 체크
    len = read(sock, buffer, MAX_BUFFER);
    buffer[len] = '\0';
    if (strncmp(buffer, "ERROR", 5) == 0) {
        printf("\n✗ %s", buffer + 6);
        return;
    }

    // 내용 입력 ("CONTENT" 수신 후)
    printf("새로운 내용을 입력하세요 (최대 500자):\n");
    fgets(buffer, 500, stdin);
    buffer[strcspn(buffer, "\n")] = '\0';
    write(sock, buffer, strlen(buffer));

    // 최종 결과 수신
    len = read(sock, buffer, MAX_BUFFER);
    buffer[len] = '\0';
    
    if (strncmp(buffer, "SUCCESS", 7) == 0) {
        printf("\n✓ %s", buffer + 8);
    } else {
        printf("\n✗ %s", buffer + 6);
    }
}

// 접속자 목록
void listOnlineUsers(int sock) {
    char buffer[MAX_BUFFER];

    write(sock, "ONLINE", 6);

    int len = read(sock, buffer, MAX_BUFFER);
    buffer[len] = '\0';

    printf("\n%s\n", buffer);
}



/* ============================================================
 * 4. 메인 함수
 * ============================================================ */

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

    // 로그인 전 루프
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

    // 로그인 후 메인 루프
    while (1) {
        printMainMenu();
        scanf("%d", &choice);
        getchar();

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
        case 4: // [추가]
            updatePost(client_sock);
            break;
        case 5: // [번호 밀림]
            deletePost(client_sock);
            break;
        case 6: // [번호 밀림]
            listOnlineUsers(client_sock);
            break;
        case 7: // [번호 밀림]
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
