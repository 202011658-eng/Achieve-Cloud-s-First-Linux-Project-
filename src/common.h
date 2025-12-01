#ifndef COMMON_H
#define COMMON_H

// common.h : 공통 헤더 파일, 게시물, 유저, 댓글, 알림 구조체 정의
// server와 client 모두에서 사용하는 구조체와 상수를 포함

#include <time.h>
#include <arpa/inet.h>

#define PORT 9000
#define MAX_BUFFER 4096

// 파일 경로 상수 (서버용)
#define DATA_FILE "board_data.txt"
#define USER_FILE "user_data.txt"
#define COMMENT_FILE "comment_data.txt"
#define ONLINE_FILE "online_users.txt"
#define NOTIFICATION_FILE "notification_data.txt"

// 제한 상수
#define MAX_NOTIFICATIONS 3000
#define MAX_POSTS 100
#define MAX_USERS 1000
#define MAX_ONLINE 50
#define MAX_COMMENTS 2000

// 구조체 정의
typedef struct {
    int id;
    char title[100];
    char author[50];
    char content[500];
    char timestamp[30];
    int views;
    int likes;
} Post;
// Post 구조체 : 게시물 ID, 제목, 작성자, 내용, 작성 시간, 조회수, 좋아요 수 포함

typedef struct {
    char username[50];
    char password[50];
    char nickname[50];
} User;
// User 구조체 : 사용자 이름, 비밀번호, 닉네임 포함

typedef struct {
    char username[50];
    char nickname[50];
    char ip[INET_ADDRSTRLEN];
    time_t login_time;
} OnlineUser;
// OnlineUser 구조체 : 온라인 사용자 이름, 닉네임, IP 주소, 로그인 시간 포함

typedef struct {
    int id;
    int post_id;
    char author[50];
    char content[300];
    char timestamp[30];
} Comment;
// Comment 구조체 : 댓글 ID, 게시물 ID, 작성자, 내용, 작성 시간 포함

typedef struct {
    int id;
    char owner[50];      
    char message[200];   
    char timestamp[30];  
} Notification;
// Notification 구조체 : 알림 ID, 소유자, 메시지, 생성 시간 포함

#endif