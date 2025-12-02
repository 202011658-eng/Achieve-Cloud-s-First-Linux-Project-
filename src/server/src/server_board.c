#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/file.h>
#include "../common.h"
#include "server_board_manager.h"
#include "server_noti_manager.h" // likePost의 알림, readPost의 댓글 append 위해
#include "server_util.h"

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

void listPosts(int client_sock) {
    Post posts[MAX_POSTS];
    int count = readPosts(posts);

    char buffer[MAX_BUFFER];
    int offset = 0;

    const char *line = "================================================================================\n";

    offset += snprintf(buffer + offset, MAX_BUFFER - offset, "%s", line);
    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                       "%-4s %-15s %-10s %-8s %-8s %-19s\n",
                       "번호", "제목", "작성자", "         추천수", "  조회수", "작성시간");
    offset += snprintf(buffer + offset, MAX_BUFFER - offset, "%s", line);

    for (int i = 0; i < count && offset < MAX_BUFFER - 100; i++) {
        offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                           "%4d %-15.15s %-10.10s %8d %8d %-19.19s\n",
                           posts[i].id,
                           posts[i].title,
                           posts[i].author,
                           posts[i].likes,   
                           posts[i].views,   
                           posts[i].timestamp);
    }

    offset += snprintf(buffer + offset, MAX_BUFFER - offset, "%s", line);
    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                       "총 %d개의 게시글\n", count);

    write(client_sock, buffer, offset);
}

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

void likePost(int client_sock, int post_id, const char* liker_nickname) {
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

    /* 자기 글을 자기가 추천한 건 알림 안 보냄 */
    if (strcmp(posts[found].author, liker_nickname) != 0) {
        char msg[200];
        snprintf(msg, sizeof(msg),
                 "'%s' 글에 %s님이 추천을 남겼습니다.",
                 posts[found].title, liker_nickname);
        addNotification(posts[found].author, msg);
    }

    write(client_sock, "SUCCESS|추천이 반영되었습니다.\n",
          strlen("SUCCESS|추천이 반영되었습니다.\n"));
}

int cmpViews(const void* a, const void* b) {
    const Post* pa = (const Post*)a;
    const Post* pb = (const Post*)b;
    if (pb->views != pa->views)
        return pb->views - pa->views;
    return pb->likes - pa->likes;  
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
    qsort(tmp, count, sizeof(Post), cmpViews);  

    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                       "===== 조회수순 인기글 =====\n");
    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                       "번호\t제목\t\t조회수\t추천수\n");
    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                       "------------------------\n");

    for (int i = 0; i < count && i < 10 && offset < MAX_BUFFER - 100; i++) {
        offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                           "%d\t%.15s\t%d\t%d\n",
                           tmp[i].id,
                           tmp[i].title,
                           tmp[i].views,   
                           tmp[i].likes);
    }

    write(client_sock, buffer, offset);
}

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