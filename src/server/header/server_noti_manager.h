#ifndef NOTI_MANAGER_H
#define NOTI_MANAGER_H

#include "../common.h"

// server_noti_manager.h : 서버 알림 관리 헤더 파일

int readComments(Comment comments[]);
void saveComments(Comment comments[], int count);
int readNotifications(Notification notis[]);
void saveNotifications(Notification notis[], int count);
void addNotification(const char* owner, const char* message);
void showNotifications(int client_sock, const char* nickname);
void addComment(int client_sock, int post_id, const char* nickname);
int appendCommentsForPost(char *buffer, int offset, int max_len, int post_id);

#endif