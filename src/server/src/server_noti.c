#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/file.h>
#include "../common.h"
#include "../header/server_noti.h"
#include "../header/server_board.h" // addComment에서 readPosts 사용 위함
#include "../header/server_util.h"

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

int readNotifications(Notification notis[]) {
    FILE *fp = fopen(NOTIFICATION_FILE, "r");
    int count = 0;

    if (fp == NULL) return 0;

    flock(fileno(fp), LOCK_SH);
    while (count < MAX_NOTIFICATIONS &&
           fscanf(fp, "%d|%49[^|]|%199[^|]|%29[^\n]\n",
                  &notis[count].id,
                  notis[count].owner,
                  notis[count].message,
                  notis[count].timestamp) == 4) {
        count++;
    }
    flock(fileno(fp), LOCK_UN);
    fclose(fp);
    return count;
}

void saveNotifications(Notification notis[], int count) {
    FILE *fp = fopen(NOTIFICATION_FILE, "w");
    if (fp == NULL) {
        perror("알림 파일 열기 실패");
        return;
    }

    flock(fileno(fp), LOCK_EX);
    for (int i = 0; i < count; i++) {
        fprintf(fp, "%d|%s|%s|%s\n",
                notis[i].id,
                notis[i].owner,
                notis[i].message,
                notis[i].timestamp);
    }
    flock(fileno(fp), LOCK_UN);
    fclose(fp);
}

void addNotification(const char* owner, const char* message) {
    Notification notis[MAX_NOTIFICATIONS];
    int count = readNotifications(notis);
    if (count >= MAX_NOTIFICATIONS) return;

    Notification n;
    n.id = (count > 0) ? notis[count - 1].id + 1 : 1;

    strncpy(n.owner, owner, 49);
    n.owner[49] = '\0';

    strncpy(n.message, message, 199);
    n.message[199] = '\0';

    getCurrentTime(n.timestamp);

    notis[count] = n;
    saveNotifications(notis, count + 1);
}

void showNotifications(int client_sock, const char* nickname) {
    Notification notis[MAX_NOTIFICATIONS];
    int count = readNotifications(notis);

    char buffer[MAX_BUFFER];
    int offset = 0;
    int found = 0;

    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                       "===== 알림 목록 =====\n");

    for (int i = 0; i < count && offset < MAX_BUFFER - 100; i++) {
        if (strcmp(notis[i].owner, nickname) == 0) {
            found = 1;
            offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                               "[%s]\n- %s\n\n",
                               notis[i].timestamp,
                               notis[i].message);
        }
    }

    if (!found) {
        offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                           "새 알림이 없습니다.\n");
    }

    write(client_sock, buffer, offset);

    // 확인한 알림은 파일에서 제거
    if (found) {
        int newCount = 0;
        for (int i = 0; i < count; i++) {
            if (strcmp(notis[i].owner, nickname) != 0) {
                notis[newCount++] = notis[i];
            }
        }
        saveNotifications(notis, newCount);
    }
}

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

    // 댓글 배열 로드
    Comment comments[MAX_COMMENTS];
    int ccount = readComments(comments);

    if (ccount >= MAX_COMMENTS) {
        write(client_sock, "ERROR|댓글이 너무 많습니다. 더 이상 작성할 수 없습니다.\n",
              strlen("ERROR|댓글이 너무 많습니다. 더 이상 작성할 수 없습니다.\n"));
        return;
    }

    // 클라이언트에 내용 요청
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

    // 원글 작성자에게 알림 (자기 댓글은 제외)
    if (strcmp(posts[found].author, nickname) != 0) {
        char msg[200];
        snprintf(msg, sizeof(msg),
                 "'%s' 글에 %s님이 댓글을 남겼습니다.",
                 posts[found].title, nickname);
        addNotification(posts[found].author, msg);
    }

    write(client_sock, "SUCCESS|댓글이 등록되었습니다.\n",
          strlen("SUCCESS|댓글이 등록되었습니다.\n"));
}

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