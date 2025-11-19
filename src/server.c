#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/file.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>

#define PORT 9000
#define MAX_BUFFER 4096
#define DATA_FILE "board_data.txt"
#define MAX_POSTS 100

// 게시글 구조체
typedef struct {
    int id;
    char title[100];
    char author[50];
    char content[500];
    char timestamp[30];
} Post;

void handleError(const char *message) {
    perror(message);
    exit(1);
}

// 좀비 프로세스 방지 (시그널 핸들러)
void sigchld_handler(int sig) {
    // 종료된 자식 프로세스 정리
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

// 현재 시간 가져오기
void getCurrentTime(char *buffer) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, 30, "%Y-%m-%d %H:%M:%S", t);
}

// 게시글 목록 읽기
int readPosts(Post posts[]) {
    FILE *fp = fopen(DATA_FILE, "r");
    int count = 0;
    
    if (fp == NULL) {
        return 0; // 파일이 없으면 0 반환
    }
    
    // 파일 잠금 (공유 잠금 - 읽기용)
    flock(fileno(fp), LOCK_SH);
    
    while (count < MAX_POSTS && 
           fscanf(fp, "%d|%99[^|]|%49[^|]|%499[^|]|%29[^\n]\n",
                  &posts[count].id,
                  posts[count].title,
                  posts[count].author,
                  posts[count].content,
                  posts[count].timestamp) == 5) {
        count++;
    }
    
    flock(fileno(fp), LOCK_UN);
    fclose(fp);
    
    return count;
}

// 게시글 저장
void savePosts(Post posts[], int count) {
    FILE *fp = fopen(DATA_FILE, "w");
    if (fp == NULL) {
        perror("파일 열기 실패");
        return;
    }
    
    // 파일 잠금 (배타적 잠금 - 쓰기용)
    flock(fileno(fp), LOCK_EX);
    
    for (int i = 0; i < count; i++) {
        fprintf(fp, "%d|%s|%s|%s|%s\n",
                posts[i].id,
                posts[i].title,
                posts[i].author,
                posts[i].content,
                posts[i].timestamp);
    }
    
    flock(fileno(fp), LOCK_UN);
    fclose(fp);
}

// 게시글 작성
void writePost(int client_sock) {
    Post posts[MAX_POSTS];
    int count = readPosts(posts);
    
    if (count >= MAX_POSTS) {
        write(client_sock, "ERROR|게시글이 꽉 찼습니다.\n", strlen("ERROR|게시글이 꽉 찼습니다.\n"));
        return;
    }
    
    Post newPost;
    newPost.id = (count > 0) ? posts[count-1].id + 1 : 1;
    
    char buffer[MAX_BUFFER];
    
    // 제목 받기
    write(client_sock, "TITLE", 5);
    int len = read(client_sock, buffer, MAX_BUFFER);
    if (len <= 0) return;
    buffer[len] = '\0';
    strncpy(newPost.title, buffer, 99);
    newPost.title[99] = '\0';
    
    // 작성자 받기
    write(client_sock, "AUTHOR", 6);
    len = read(client_sock, buffer, MAX_BUFFER);
    if (len <= 0) return;
    buffer[len] = '\0';
    strncpy(newPost.author, buffer, 49);
    newPost.author[49] = '\0';
    
    // 내용 받기
    write(client_sock, "CONTENT", 7);
    len = read(client_sock, buffer, MAX_BUFFER);
    if (len <= 0) return;
    buffer[len] = '\0';
    strncpy(newPost.content, buffer, 499);
    newPost.content[499] = '\0';
    
    // 시간 설정
    getCurrentTime(newPost.timestamp);
    
    // 게시글 추가
    posts[count] = newPost;
    savePosts(posts, count + 1);
    
    write(client_sock, "SUCCESS|게시글이 작성되었습니다.\n", strlen("SUCCESS|게시글이 작성되었습니다.\n"));
}

// 게시글 목록 보기
void listPosts(int client_sock) {
    Post posts[MAX_POSTS];
    int count = readPosts(posts);
    
    char buffer[MAX_BUFFER];
    int offset = 0;
    
    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                      "========================================\n");
    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                      "번호\t제목\t\t작성자\t\t작성시간\n");
    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                      "========================================\n");
    
    for (int i = 0; i < count && offset < MAX_BUFFER - 100; i++) {
        offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                          "%d\t%.15s\t%.10s\t%s\n",
                          posts[i].id,
                          posts[i].title,
                          posts[i].author,
                          posts[i].timestamp);
    }
    
    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                      "========================================\n");
    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                      "총 %d개의 게시글\n", count);
    
    write(client_sock, buffer, offset);
}

// 게시글 읽기
void readPost(int client_sock, int post_id) {
    Post posts[MAX_POSTS];
    int count = readPosts(posts);
    
    char buffer[MAX_BUFFER];
    int found = 0;
    
    for (int i = 0; i < count; i++) {
        if (posts[i].id == post_id) {
            snprintf(buffer, MAX_BUFFER,
                    "========================================\n"
                    "번호: %d\n"
                    "제목: %s\n"
                    "작성자: %s\n"
                    "작성시간: %s\n"
                    "========================================\n"
                    "%s\n"
                    "========================================\n",
                    posts[i].id,
                    posts[i].title,
                    posts[i].author,
                    posts[i].timestamp,
                    posts[i].content);
            found = 1;
            break;
        }
    }
    
    if (!found) {
        snprintf(buffer, MAX_BUFFER, "ERROR|해당 게시글을 찾을 수 없습니다.\n");
    }
    
    write(client_sock, buffer, strlen(buffer));
}

// 게시글 삭제
void deletePost(int client_sock, int post_id) {
    Post posts[MAX_POSTS];
    int count = readPosts(posts);
    
    int found = -1;
    for (int i = 0; i < count; i++) {
        if (posts[i].id == post_id) {
            found = i;
            break;
        }
    }
    
    if (found == -1) {
        write(client_sock, "ERROR|해당 게시글을 찾을 수 없습니다.\n",
              strlen("ERROR|해당 게시글을 찾을 수 없습니다.\n"));
        return;
    }
    
    // 게시글 삭제 (배열에서 제거)
    for (int i = found; i < count - 1; i++) {
        posts[i] = posts[i + 1];
    }
    
    savePosts(posts, count - 1);
    write(client_sock, "SUCCESS|게시글이 삭제되었습니다.\n",
          strlen("SUCCESS|게시글이 삭제되었습니다.\n"));
}

// 클라이언트 요청 처리
void handleClient(int client_sock, char *client_ip) {
    char buffer[MAX_BUFFER];
    
    printf("[PID %d] 클라이언트 %s 처리 시작\n", getpid(), client_ip);
    
    while (1) {
        memset(buffer, 0, MAX_BUFFER);
        int len = read(client_sock, buffer, MAX_BUFFER);
        
        if (len <= 0) {
            printf("[PID %d] 클라이언트 %s 연결 종료\n", getpid(), client_ip);
            break;
        }
        
        buffer[len] = '\0';
        printf("[PID %d] %s 요청: %s\n", getpid(), client_ip, buffer);
        
        if (strncmp(buffer, "WRITE", 5) == 0) {
            writePost(client_sock);
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
            deletePost(client_sock, post_id);
        }
        else if (strncmp(buffer, "QUIT", 4) == 0) {
            write(client_sock, "BYE", 3);
            printf("[PID %d] 클라이언트 %s 정상 종료\n", getpid(), client_ip);
            break;
        }
        else {
            write(client_sock, "ERROR|알 수 없는 명령입니다.\n",
                  strlen("ERROR|알 수 없는 명령입니다.\n"));
        }
    }
    
    close(client_sock);
    exit(0); // 자식 프로세스 종료
}

int main(int argc, char *argv[]) {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_size;
    pid_t pid;

    // SIGCHLD 시그널 핸들러 등록 (좀비 프로세스 방지)
    signal(SIGCHLD, sigchld_handler);

    // 서버 소켓 생성
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        handleError("socket() error");
    }

    // 소켓 옵션 설정 (재사용 가능하도록)
    int option = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    // 서버 주소 설정
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    // 바인드
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        handleError("bind() error");
    }

    // 리슨
    if (listen(server_sock, 5) == -1) {
        handleError("listen() error");
    }

    printf("========================================\n");
    printf("온라인 게시판 서버가 시작되었습니다.\n");
    printf("포트: %d\n", PORT);
    printf("다중 클라이언트 지원 (fork 사용)\n");
    printf("========================================\n");

    while (1) {
        // 클라이언트 연결 수락
        client_addr_size = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_size);

        if (client_sock == -1) {
            perror("accept() error");
            continue;
        }

        // 클라이언트 IP 주소 가져오기
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        printf("\n[부모 프로세스] 새 클라이언트 접속: %s\n", client_ip);

        // fork()로 자식 프로세스 생성
        pid = fork();

        if (pid == -1) {
            perror("fork() error");
            close(client_sock);
            continue;
        }
        else if (pid == 0) {
            // ===== 자식 프로세스 =====
            close(server_sock); // 자식은 서버 소켓 필요 없음
            
            // 환영 메시지
            const char *welcome = "온라인 게시판에 오신 것을 환영합니다!\n";
            write(client_sock, welcome, strlen(welcome));
            
            // 클라이언트 요청 처리 (여기서 exit()으로 종료)
            handleClient(client_sock, client_ip);
        }
        else {
            // ===== 부모 프로세스 =====
            close(client_sock); // 부모는 클라이언트 소켓 필요 없음
            printf("[부모 프로세스] 자식 프로세스 생성 (PID: %d)\n", pid);
        }
    }

    close(server_sock);
    return 0;
}
