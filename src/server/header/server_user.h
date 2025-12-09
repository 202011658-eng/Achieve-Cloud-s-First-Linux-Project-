#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include "common.h"

// server_user_manager.h : 서버 사용자 관리 헤더 파일

int readUsers(User users[]);
void saveUser(User* user);
int userExists(const char* username);
int authenticateUser(const char* username, const char* password, char* nickname);
int nicknameExists(const char* nickname);

int readOnlineUsersFromFile(OnlineUser users[], int max_users);
void saveOnlineUsersToFile(OnlineUser users[], int count);
void addOnlineUser(const char* username, const char* nickname, const char* ip);
void removeOnlineUser(const char* username);

void registerUser(int client_sock);
int loginUser(int client_sock, char* username, char* nickname, const char* client_ip);
void listOnlineUsers(int client_sock);

#endif