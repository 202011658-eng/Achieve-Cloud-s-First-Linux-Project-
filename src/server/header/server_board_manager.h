#ifndef BOARD_MANAGER_H
#define BOARD_MANAGER_H

#include "../common.h"

// server_board_manager.h : 서버 게시판 관리 헤더 파일

int readPosts(Post posts[]);
void savePosts(Post posts[], int count);
void writePost(int client_sock, const char* nickname);
void listPosts(int client_sock);
void readPost(int client_sock, int post_id);
void deletePost(int client_sock, int post_id, const char* nickname);
void updatePost(int client_sock, int post_id, const char* nickname);
void likePost(int client_sock, int post_id, const char* liker_nickname);
void rankPosts(int client_sock);
void searchPosts(int client_sock, const char* keyword);

#endif