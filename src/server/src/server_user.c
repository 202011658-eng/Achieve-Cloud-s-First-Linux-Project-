#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/file.h>
#include <arpa/inet.h>
#include "../../common.h"
#include "../header/server_user.h"
#include "../header/server_util.h"

// 전역 변수 (접속자 관리용)
OnlineUser online_users[MAX_ONLINE];
int online_count = 0;

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

int readOnlineUsersFromFile(OnlineUser users[], int max_users) {
    FILE *fp = fopen(ONLINE_FILE, "r");
    int count = 0;

    if (fp == NULL) {
        return 0;   // 파일 없으면 0명
    }

    flock(fileno(fp), LOCK_SH);

    while (count < max_users) {
        char uname[50], nick[50], ip[INET_ADDRSTRLEN];
        long t;
        int n = fscanf(fp, "%49[^|]|%49[^|]|%15[^|]|%ld\n",
                       uname, nick, ip, &t);
        if (n != 4) break;

        strncpy(users[count].username, uname, 49);
        users[count].username[49] = '\0';

        strncpy(users[count].nickname, nick, 49);
        users[count].nickname[49] = '\0';

        strncpy(users[count].ip, ip, INET_ADDRSTRLEN - 1);
        users[count].ip[INET_ADDRSTRLEN - 1] = '\0';

        users[count].login_time = (time_t)t;
        count++;
    }

    flock(fileno(fp), LOCK_UN);
    fclose(fp);
    return count;
}

void saveOnlineUsersToFile(OnlineUser users[], int count) {
    FILE *fp = fopen(ONLINE_FILE, "w");
    if (fp == NULL) {
        perror("온라인 유저 파일 열기 실패");
        return;
    }

    flock(fileno(fp), LOCK_EX);

    for (int i = 0; i < count; i++) {
        fprintf(fp, "%s|%s|%s|%ld\n",
                users[i].username,
                users[i].nickname,
                users[i].ip,
                (long)users[i].login_time);
    }

    flock(fileno(fp), LOCK_UN);
    fclose(fp);
}

void addOnlineUser(const char* username, const char* nickname, const char* ip) {
    OnlineUser users[MAX_ONLINE];
    int count = readOnlineUsersFromFile(users, MAX_ONLINE);

    if (count >= MAX_ONLINE) {
        printf("[접속자 추가 실패] 최대 접속자 수 초과\n");
        return;
    }

    for (int i = 0; i < count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            printf("[중복 로그인 시도] %s (%s)\n", nickname, username);
            return;
        }
    }

    strncpy(users[count].username, username, 49);
    users[count].username[49] = '\0';
    strncpy(users[count].nickname, nickname, 49);
    users[count].nickname[49] = '\0';
    strncpy(users[count].ip, ip, INET_ADDRSTRLEN - 1);
    users[count].ip[INET_ADDRSTRLEN - 1] = '\0';
    users[count].login_time = time(NULL);

    count++;
    saveOnlineUsersToFile(users, count);

    printf("[접속자 추가] %s(%s) - 현재 접속자: %d명\n",
           nickname, username, count);
}

void removeOnlineUser(const char* username) {
    OnlineUser users[MAX_ONLINE];
    int count = readOnlineUsersFromFile(users, MAX_ONLINE);
    int newCount = 0;
    char removed_nick[50] = "";
    int removed = 0;

    for (int i = 0; i < count; i++) {
        if (!removed && strcmp(users[i].username, username) == 0) {
            strncpy(removed_nick, users[i].nickname, 49);
            removed_nick[49] = '\0';
            removed = 1;
            continue;   
        }
        users[newCount++] = users[i];
    }

    saveOnlineUsersToFile(users, newCount);

    if (removed) {
        printf("[접속자 제거] %s(%s) - 현재 접속자: %d명\n",
               removed_nick, username, newCount);
    }
}

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
    OnlineUser users[MAX_ONLINE];
    int count = readOnlineUsersFromFile(users, MAX_ONLINE);
    char buffer[MAX_BUFFER];
    int offset = 0;
    time_t now = time(NULL);

    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                       "========================================\n");
    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                       "현재 접속자 목록 (%d명)\n", count);
    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                       "========================================\n");
    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                       "닉네임\t\t접속 시간\n");
    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                       "----------------------------------------\n");

    for (int i = 0; i < count && offset < MAX_BUFFER - 100; i++) {
        int minutes = (int)((now - users[i].login_time) / 60);
        offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                           "%.15s\t%d분 전\n",
                           users[i].nickname, minutes);
    }

    offset += snprintf(buffer + offset, MAX_BUFFER - offset,
                       "========================================\n");

    write(client_sock, buffer, offset);
}