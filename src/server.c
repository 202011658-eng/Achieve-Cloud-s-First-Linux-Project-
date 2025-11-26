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
#include <ctype.h>

#define PORT 9000
#define MAX_BUFFER 4096
#define DATA_FILE "board_data.txt"
#define USER_FILE "user_data.txt"
#define COMMENT_FILE "comment_data.txt"

#define MAX_POSTS 100
#define MAX_USERS 1000
#define MAX_ONLINE 50
#define MAX_COMMENTS 2000

/* ===========================
 * 구조체 정의
 * =========================== */

typedef struct {
    int id;
    char title[100];
    char author[50];
    char content[500];
    char timestamp[30];
    int views;
    int likes;
} Post;

typedef struct {
    char username[50];
    char password[50];
    char nickname[50];
} User;

typedef struct {
    char username[50];
    char nickname[50];
    char ip[INET_ADDRSTRLEN];
    time_t login_time;
} OnlineUser;

typedef struct {
    int id;
    int post_id;
    char author[50];
    char content[300];
    char timestamp[30];
} Comment;

/* ===========================
 * 전역 변수
 * =========================== */

OnlineUser online_users[MAX_ONLINE];
int online_count = 0;

const char* bad_words[] = {
    "욕설1", "욕설2", "바보", "멍청이", "병신", "시발", "개새끼", "fuck", "김성운", "shit",
    NULL
};

/* ===========================
 * 공통 유틸리티
 * =========================== */

void handleError(const char *message) {
    perror(message);
    exit(1);
}

void sigchld_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

void getCurrentTime(char *buffer) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, 30, "%Y-%m-%d %H:%M:%S", t);
}

/* 욕설 필터 */

int containsBadWord(const char* text) {
    char lower_text[1000];

    strncpy(lower_text, text, sizeof(lower_text) - 1);
    lower_text[sizeof(lower_text) - 1] = '\0';

    for (int i = 0; lower_text[i]; i++) {
        lower_text[i] = tolower((unsigned char)lower_text[i]);
    }

    for (int i = 0; bad_words[i] != NULL; i++) {
        if (strstr(lower_text, bad_words[i]) != NULL) {
            return 1;
        }
    }
    return 0;
}

void maskBadWords(char* text) {
    for (int i = 0; bad_words[i] != NULL; i++) {
        char* pos = strstr(text, bad_words[i]);
        while (pos != NULL) {
            int len = (int)strlen(bad_words[i]);
            for (int j = 0; j < len; j++) {
                pos[j] = '*';
            }
            pos = strstr(pos + len, bad_words[i]);
        }
    }
}

/* ===========================
 * 사용자 관리
 * =========================== */

int readUsers(User users[]) {
    FILE *fp = fopen(USER_FILE, "r");
    int count = 0;

    if (fp == NULL) {
        return 0;
    }

    flock(fileno(fp), LOCK_SH);

    while (count < MAX_USERS &&
           fscanf(fp, "%49[^|]|%49[^|]|%49[^\n]\n",
                  users[count].username,
                  users[count].password,
                  users[count].nickname) == 3) {
        count++;
    }

    flock(fileno(fp), LOCK_UN);
    fclose(fp);
    return count;
}

void saveUser(User* user) {
    FILE *fp = fopen(USER_FILE, "a");
    if (fp == NULL) {
        perror("사용자 파일 열기 실패");
        return;
    }

    flock(fileno(fp), LOCK_EX);
    fprintf(fp, "%s|%s|%s\n", user->username, user->password, user->nickname);
    flock(fileno(fp), LOCK_UN);
    fclose(fp);
}

int userExists(const char* username) {
    User users[MAX_USERS];
    int count = readUsers(users);

    for (int i = 0; i < count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            return 1;
        }
    }
    return 0;
}

int authenticateUser(const char* username, const char* password, char* nickname) {
    User users[MAX_USERS];
    int count = readUsers(users);

    for (int i = 0; i < count; i++) {
        if (strcmp(users[i].username, username) == 0 &&
            strcmp(users[i].password, password) == 0) {
            strcpy(nickname, users[i].nickname);
            return 1;
        }
    }
    return 0;
}

int nicknameExists(const char* nickname) {
    User users[MAX_USERS];
    int count = readUsers(users);

    for (int i = 0; i < count; i++) {
        if (strcmp(users[i].nickname, nickname) == 0) {
            return 1;
        }
    }
    return 0;
}

/* ===========================
 * 접속자 관리
 * =========================== */

void addOnlineUser(const char* username, const char* nickname, const char* ip) {
    if (online_count >= MAX_ONLINE) return;

    strcpy(online_users[online_count].username, username);
    strcpy(online_users[online_count].nickname, nickname);
    strcpy(online_users[online_count].ip, ip);
    online_users[online_count].login_time = time(NULL);
    online_count++;

    printf("[접속자 추가] %s(%s) - 현재 접속자: %d명\n",
           nickname, username, online_count);
}

void removeOnlineUser(const char* username) {
    for (int i = 0; i < online_count; i++) {
        if (strcmp(online_users[i].username, username) == 0) {
            printf("[접속자 제거] %s(%s)\n",
                   online_users[i].nickname, online_users[i].username);

            for (int j = i; j < online_count - 1; j++) {
                online_users[j] = online_users[j + 1];
            }
            online_count--;
            printf("현재 접속자: %d명\n", online_count);
            break;
        }
    }
}

/* ===========================
 * 회원가입 / 로그인 / 온라인 목록
 * =========================== */

void registerUser(int client_sock) {
    char buffer[MAX_BUFFER];
    User newUser;

    write(client_sock, "USERNAME", 8);
    int len = read(client_sock, buffer, MAX_BUFFER);
    if (len <= 0) return;
    buffer[len] = '\0';

    if (userExists(buffer)) {
        write(client_sock, "ERROR|이미 존재하는 아이디입니다.\n",
              strlen("ERROR|이미 존재하는 아이디입니다.\n"));
        return;
    }

    strncpy(newUser.username, buffer, 49);
    newUser.username[49] = '\0';

    write(client_sock, "PASSWORD", 8);
    len = read(client_sock, buffer, MAX_BUFFER);
    if (len <= 0) return;
    buffer[len] = '\0';
    strncpy(newUser.password, buffer, 49);
    newUser.password[49] = '\0';

    write(client_sock, "NICKNAME", 8);
    len = read(client_sock, buffer, MAX_BUFFER);
    if (len <= 0) return;
    buffer[len] = '\0';

    if (nicknameExists(buffer)) {
        write(client_sock, "ERROR|이미 존재하는 닉네임입니다.\n",
              strlen("ERROR|이미 존재하는 닉네임입니다.\n"));
        return;
    }

    if (containsBadWord(buffer)) {
        write(client_sock, "ERROR|닉네임에 부적절한 단어가 포함되어 있습니다.\n",
              strlen("ERROR|닉네임에 부적절한 단어가 포함되어 있습니다.\n"));
        return;
    }

    strncpy(newUser.nickname, buffer, 49);
    newUser.nickname[49] = '\0';

    saveUser(&newUser);

    write(client_sock,
          "SUCCESS|회원가입이 완료되었습니다. 로그인해주세요.\n",
          strlen("SUCCESS|회원가입이 완료되었습니다. 로그인해주세요.\n"));
}

int loginUser(int client_sock, char* username, char* nickname, const char* client_ip) {
    char buffer[MAX_BUFFER];
    char input_username[50];
    char input_password[50];

    write(client_sock, "USERNAME", 8);
    int len = read(client_sock, buffer, MAX_BUFFER);
    if (len <= 0) return 0;
    buffer[len] = '\0';
    strncpy(input_username, buffer, 49);
    input_username[49] = '\0';

    write(client_sock, "PASSWORD", 8);
    len = read(client_sock, buffer, MAX_BUFFER);
    if (len <= 0) return 0;
    buffer[len] = '\0';
    strncpy(input_password, buffer, 49);
    input_password[49] = '\0';

    if (authenticateUser(input_username, input_password, nickname)) {
        strcpy(username, input_username);
        addOnlineUser(username, nickname, client_ip);

        snprintf(buffer, MAX_BUFFER, "SUCCESS|%s님 환영합니다!\n", nickname);
        write(client_sock, buffer, strlen(buffer));
        return 1;
    } else {
        write(client_sock, "ERROR|아이디 또는 비밀번호가 올바르지 않습니다.\n",
              strlen("ERROR|아이디 또는 비밀번호가 올바르지 않습니다.\n"));
        return 0;
    }
}

void listOnlineUsers(int client_sock) {
    char buffer[MAX_BUFFER];
    int offset = 0;
    time_t now = time(NULL);

    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                       "========================================\n");
    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                       "현재 접속자 목록 (%d명)\n", online_count);
    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                       "========================================\n");
    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                       "닉네임\t\t접속 시간\n");
    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                       "----------------------------------------\n");

    for (int i = 0; i < online_count && offset < MAX_BUFFER - 100; i++) {
        int minutes = (int)((now - online_users[i].login_time) / 60);
        offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                           "%.15s\t%d분 전\n",
                           online_users[i].nickname, minutes);
    }

    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                       "========================================\n");

    write(client_sock, buffer, offset);
}

/* ===========================
 * 게시글 파일 I/O
 * 포맷: id|title|author|content|timestamp|views|likes\n
 * =========================== */

int readPosts(Post posts[]) {
    FILE *fp = fopen(DATA_FILE, "r");
    int count = 0;

    if (fp == NULL) {
        return 0;
    }

    flock(fileno(fp), LOCK_SH);

    while (count < MAX_POSTS &&
           fscanf(fp, "%d|%99[^|]|%49[^|]|%499[^|]|%29[^|]|%d|%d\n",
                  &posts[count].id,
                  posts[count].title,
                  posts[count].author,
                  posts[count].content,
                  posts[count].timestamp,
                  &posts[count].views,
                  &posts[count].likes) == 7) {
        count++;
    }

    flock(fileno(fp), LOCK_UN);
    fclose(fp);
    return count;
}

void savePosts(Post posts[], int count) {
    FILE *fp = fopen(DATA_FILE, "w");
    if (fp == NULL) {
        perror("파일 열기 실패");
        return;
    }

    flock(fileno(fp), LOCK_EX);

    for (int i = 0; i < count; i++) {
        fprintf(fp, "%d|%s|%s|%s|%s|%d|%d\n",
                posts[i].id,
                posts[i].title,
                posts[i].author,
                posts[i].content,
                posts[i].timestamp,
                posts[i].views,
                posts[i].likes);
    }

    flock(fileno(fp), LOCK_UN);
    fclose(fp);
}

/* ===========================
 * 댓글 파일 I/O
 * 포맷: id|post_id|author|content|timestamp\n
 * =========================== */

int readComments(Comment comments[]) {
    FILE *fp = fopen(COMMENT_FILE, "r");
    int count = 0;

    if (fp == NULL) {
        return 0;
    }

    flock(fileno(fp), LOCK_SH);

    while (count < MAX_COMMENTS &&
           fscanf(fp, "%d|%d|%49[^|]|%299[^|]|%29[^\n]\n",
                  &comments[count].id,
                  &comments[count].post_id,
                  comments[count].author,
                  comments[count].content,
                  comments[count].timestamp) == 5) {
        count++;
    }

    flock(fileno(fp), LOCK_UN);
    fclose(fp);
    return count;
}

void saveComments(Comment comments[], int count) {
    FILE *fp = fopen(COMMENT_FILE, "w");
    if (fp == NULL) {
        perror("댓글 파일 열기 실패");
        return;
    }

    flock(fileno(fp), LOCK_EX);

    for (int i = 0; i < count; i++) {
        fprintf(fp, "%d|%d|%s|%s|%s\n",
                comments[i].id,
                comments[i].post_id,
                comments[i].author,
                comments[i].content,
                comments[i].timestamp);
    }

    flock(fileno(fp), LOCK_UN);
    fclose(fp);
}

/* ===========================
 * 글 작성
 * =========================== */

void writePost(int client_sock, const char* nickname) {
    Post posts[MAX_POSTS];
    int count = readPosts(posts);

    if (count >= MAX_POSTS) {
        write(client_sock, "ERROR|게시글이 꽉 찼습니다.\n",
              strlen("ERROR|게시글이 꽉 찼습니다.\n"));
        return;
    }

    Post newPost;
    newPost.id = (count > 0) ? posts[count - 1].id + 1 : 1;
    newPost.views = 0;
    newPost.likes = 0;

    char buffer[MAX_BUFFER];

    write(client_sock, "TITLE", 5);
    int len = read(client_sock, buffer, MAX_BUFFER);
    if (len <= 0) return;
    buffer[len] = '\0';

    if (containsBadWord(buffer)) {
        write(client_sock,
              "ERROR|제목에 부적절한 단어가 포함되어 있습니다.\n",
              strlen("ERROR|제목에 부적절한 단어가 포함되어 있습니다.\n"));
        return;
    }

    strncpy(newPost.title, buffer, 99);
    newPost.title[99] = '\0';

    strncpy(newPost.author, nickname, 49);
    newPost.author[49] = '\0';

    write(client_sock, "CONTENT", 7);
    len = read(client_sock, buffer, MAX_BUFFER);
    if (len <= 0) return;
    buffer[len] = '\0';

    if (containsBadWord(buffer)) {
        maskBadWords(buffer);
    }

    strncpy(newPost.content, buffer, 499);
    newPost.content[499] = '\0';

    getCurrentTime(newPost.timestamp);

    posts[count] = newPost;
    savePosts(posts, count + 1);

    write(client_sock, "SUCCESS|게시글이 작성되었습니다.\n",
          strlen("SUCCESS|게시글이 작성되었습니다.\n"));
}

/* ===========================
 * 글 목록
 * =========================== */

void listPosts(int client_sock) {
    Post posts[MAX_POSTS];
    int count = readPosts(posts);

    char buffer[MAX_BUFFER];
    int offset = 0;

    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                       "========================================\n");
    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                       "번호\t제목\t\t작성자\t\t추천수\t조회수\t작성시간\n");
    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                       "========================================\n");

    for (int i = 0; i < count && offset < MAX_BUFFER - 100; i++) {
        offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                           "%d\t%.15s\t%.10s\t%d\t%d\t%s\n",
                           posts[i].id,
                           posts[i].title,
                           posts[i].author,
                           posts[i].likes,
                           posts[i].views,
                           posts[i].timestamp);
    }

    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                       "========================================\n");
    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                       "총 %d개의 게시글\n", count);

    write(client_sock, buffer, offset);
}

/* ===========================
 * 댓글 출력 (특정 게시글)
 * =========================== */

int appendCommentsForPost(char *buffer, int offset, int max_len, int post_id) {
    Comment comments[MAX_COMMENTS];
    int count = readComments(comments);
    int has_comment = 0;

    if (offset > max_len - 50) return offset;

    offset += snprintf(buffer + offset, max_len - offset,
                       "------------- 댓글 -------------\n");

    for (int i = 0; i < count && offset < max_len - 100; i++) {
        if (comments[i].post_id == post_id) {
            has_comment = 1;
            offset += snprintf(buffer + offset, max_len - offset,
                               "#%d %s (%s)\n%s\n------------------------------\n",
                               comments[i].id,
                               comments[i].author,
                               comments[i].timestamp,
                               comments[i].content);
        }
    }

    if (!has_comment && offset < max_len - 50) {
        offset += snprintf(buffer + offset, max_len - offset,
                           "등록된 댓글이 없습니다.\n");
    }

    if (offset < max_len - 50) {
        offset += snprintf(buffer + offset, max_len - offset,
                           "========================================\n");
    }

    return offset;
}

/* ===========================
 * 글 읽기 (조회수 증가 + 댓글 포함)
 * =========================== */

void readPost(int client_sock, int post_id) {
    Post posts[MAX_POSTS];
    int count = readPosts(posts);

    char buffer[MAX_BUFFER];
    int offset = 0;
    int found = -1;

    for (int i = 0; i < count; i++) {
        if (posts[i].id == post_id) {
            found = i;
            break;
        }
    }

    if (found == -1) {
        snprintf(buffer, MAX_BUFFER,
                 "ERROR|해당 게시글을 찾을 수 없습니다.\n");
        write(client_sock, buffer, strlen(buffer));
        return;
    }

    posts[found].views++;
    savePosts(posts, count);

    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                       "========================================\n");
    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                       "번호: %d\n", posts[found].id);
    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                       "제목: %s\n", posts[found].title);
    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                       "작성자: %s\n", posts[found].author);
    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                       "작성시간: %s\n", posts[found].timestamp);
    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                       "추천수: %d\n", posts[found].likes);
    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                       "조회수: %d\n", posts[found].views);
    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                       "========================================\n");
    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                       "%s\n", posts[found].content);
    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                       "========================================\n");

    offset = appendCommentsForPost(buffer, offset, MAX_BUFFER, posts[found].id);

    write(client_sock, buffer, offset);
}

/* ===========================
 * 글 삭제
 * =========================== */

void deletePost(int client_sock, int post_id, const char* nickname) {
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

    if (strcmp(posts[found].author, nickname) != 0) {
        write(client_sock,
              "ERROR|본인이 작성한 글만 삭제할 수 있습니다.\n",
              strlen("ERROR|본인이 작성한 글만 삭제할 수 있습니다.\n"));
        return;
    }

    for (int i = found; i < count - 1; i++) {
        posts[i] = posts[i + 1];
    }

    savePosts(posts, count - 1);
    write(client_sock, "SUCCESS|게시글이 삭제되었습니다.\n",
          strlen("SUCCESS|게시글이 삭제되었습니다.\n"));
}

/* ===========================
 * 글 수정
 * =========================== */

void updatePost(int client_sock, int post_id, const char* nickname) {
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

    if (strcmp(posts[found].author, nickname) != 0) {
        write(client_sock, "ERROR|본인이 작성한 글만 수정할 수 있습니다.\n",
              strlen("ERROR|본인이 작성한 글만 수정할 수 있습니다.\n"));
        return;
    }

    char buffer[MAX_BUFFER];

    write(client_sock, "TITLE", 5);
    int len = read(client_sock, buffer, MAX_BUFFER);
    if (len <= 0) return;
    buffer[len] = '\0';

    if (containsBadWord(buffer)) {
        write(client_sock,
              "ERROR|제목에 부적절한 단어가 포함되어 있습니다.\n",
              strlen("ERROR|제목에 부적절한 단어가 포함되어 있습니다.\n"));
        return;
    }
    strncpy(posts[found].title, buffer, 99);
    posts[found].title[99] = '\0';

    write(client_sock, "CONTENT", 7);
    len = read(client_sock, buffer, MAX_BUFFER);
    if (len <= 0) return;
    buffer[len] = '\0';

    if (containsBadWord(buffer)) {
        maskBadWords(buffer);
    }
    strncpy(posts[found].content, buffer, 499);
    posts[found].content[499] = '\0';

    getCurrentTime(posts[found].timestamp);
    savePosts(posts, count);

    write(client_sock, "SUCCESS|게시글이 수정되었습니다.\n",
          strlen("SUCCESS|게시글이 수정되었습니다.\n"));
}

/* ===========================
 * 댓글 작성
 * 명령: COMMENT:<post_id>
 * 프로토콜:
 *   서버: (post 확인 후) "CONTENT" 또는 "ERROR|..."
 *   클라: 댓글 내용 전송
 *   서버: SUCCESS / ERROR
 * =========================== */

void addComment(int client_sock, int post_id, const char* nickname) {
    char buffer[MAX_BUFFER];

    /* 게시글 존재 여부 확인 */
    Post posts[MAX_POSTS];
    int pcount = readPosts(posts);
    int found = -1;
    for (int i = 0; i < pcount; i++) {
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

    /* 댓글 배열 로드 */
    Comment comments[MAX_COMMENTS];
    int ccount = readComments(comments);

    if (ccount >= MAX_COMMENTS) {
        write(client_sock, "ERROR|댓글이 너무 많습니다. 더 이상 작성할 수 없습니다.\n",
              strlen("ERROR|댓글이 너무 많습니다. 더 이상 작성할 수 없습니다.\n"));
        return;
    }

    /* 클라이언트에 내용 요청 */
    write(client_sock, "CONTENT", 7);

    int len = read(client_sock, buffer, MAX_BUFFER);
    if (len <= 0) return;
    buffer[len] = '\0';

    if (containsBadWord(buffer)) {
        maskBadWords(buffer);
    }

    Comment newComment;
    newComment.id = (ccount > 0) ? comments[ccount - 1].id + 1 : 1;
    newComment.post_id = post_id;

    strncpy(newComment.author, nickname, 49);
    newComment.author[49] = '\0';

    strncpy(newComment.content, buffer, 299);
    newComment.content[299] = '\0';

    getCurrentTime(newComment.timestamp);

    comments[ccount] = newComment;
    saveComments(comments, ccount + 1);

    write(client_sock, "SUCCESS|댓글이 등록되었습니다.\n",
          strlen("SUCCESS|댓글이 등록되었습니다.\n"));
}

/* ===========================
 * 추천 기능
 * =========================== */

void likePost(int client_sock, int post_id) {
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

    posts[found].likes++;
    savePosts(posts, count);

    write(client_sock, "SUCCESS|추천이 반영되었습니다.\n",
          strlen("SUCCESS|추천이 반영되었습니다.\n"));
}

/* ===========================
 * 인기글 정렬
 * =========================== */

int cmpLikes(const void* a, const void* b) {
    const Post* pa = (const Post*)a;
    const Post* pb = (const Post*)b;
    if (pb->likes != pa->likes)
        return pb->likes - pa->likes;
    return pb->views - pa->views;
}

void rankPosts(int client_sock) {
    Post posts[MAX_POSTS];
    int count = readPosts(posts);

    char buffer[MAX_BUFFER];
    int offset = 0;

    if (count == 0) {
        write(client_sock, "게시글이 없습니다.\n",
              strlen("게시글이 없습니다.\n"));
        return;
    }

    Post tmp[MAX_POSTS];
    for (int i = 0; i < count; i++) tmp[i] = posts[i];
    qsort(tmp, count, sizeof(Post), cmpLikes);

    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                       "===== 추천순 인기글 =====\n");
    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                       "번호\t제목\t\t추천수\t조회수\n");
    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                       "------------------------\n");

    for (int i = 0; i < count && i < 10 && offset < MAX_BUFFER - 100; i++) {
        offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                           "%d\t%.15s\t%d\t%d\n",
                           tmp[i].id,
                           tmp[i].title,
                           tmp[i].likes,
                           tmp[i].views);
    }

    write(client_sock, buffer, offset);
}

/* ===========================
 * 검색 기능
 * =========================== */

void searchPosts(int client_sock, const char* keyword) {
    Post posts[MAX_POSTS];
    int count = readPosts(posts);

    char buffer[MAX_BUFFER];
    int offset = 0;
    int found = 0;

    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                       "===== 검색 결과: \"%s\" =====\n", keyword);

    for (int i = 0; i < count && offset < MAX_BUFFER - 100; i++) {
        if (strstr(posts[i].title, keyword) ||
            strstr(posts[i].content, keyword) ||
            strstr(posts[i].author, keyword)) {
            found = 1;
            offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                               "%d\t%.20s\tby %.10s\t(추천:%d 조회:%d)\n",
                               posts[i].id,
                               posts[i].title,
                               posts[i].author,
                               posts[i].likes,
                               posts[i].views);
        }
    }

    if (!found) {
        offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                           "일치하는 게시글이 없습니다.\n");
    }

    write(client_sock, buffer, offset);
}

/* ===========================
 * 클라이언트 처리
 * =========================== */

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
        else if (strncmp(buffer, "UPDATE:", 7) == 0) {
            int post_id = atoi(buffer + 7);
            updatePost(client_sock, post_id, nickname);
        }
        else if (strncmp(buffer, "LIKE:", 5) == 0) {
            int post_id = atoi(buffer + 5);
            likePost(client_sock, post_id);
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

/* ===========================
 * main
 * =========================== */

int main(int argc, char *argv[]) {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_size;
    pid_t pid;

    signal(SIGCHLD, sigchld_handler);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        handleError("socket() error");
    }

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
