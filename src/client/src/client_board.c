#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "common.h"
#include "client/header/client_board.h"

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

void listPostsClient(int sock) {
    char buffer[MAX_BUFFER];

    write(sock, "LIST", 4);

    int len = read(sock, buffer, MAX_BUFFER);
    buffer[len] = '\0';

    printf("\n%s\n", buffer);
}

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

void listOnlineUsersClient(int sock) {
    char buffer[MAX_BUFFER];

    write(sock, "ONLINE", 6);

    int len = read(sock, buffer, MAX_BUFFER);
    buffer[len] = '\0';

    printf("\n%s\n", buffer);
}

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

void rankPostsClient(int sock) {
    char buffer[MAX_BUFFER];

    write(sock, "RANK", 4);

    int len = read(sock, buffer, MAX_BUFFER);
    buffer[len] = '\0';

    printf("\n%s\n", buffer);
}

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

void showNotificationsClient(int sock) {
    char buffer[MAX_BUFFER];

    write(sock, "NOTI", 4);

    int len = read(sock, buffer, MAX_BUFFER);
    buffer[len] = '\0';

    printf("\n%s\n", buffer);
}

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