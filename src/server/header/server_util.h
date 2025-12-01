#ifndef UTILS_H
#define UTILS_H

// server_util.h : 서버 유틸리티 헤더 파일

void handleError(const char *message);
void sigchld_handler(int sig);
void getCurrentTime(char *buffer);
int containsBadWord(const char* text);
void maskBadWords(char* text);

#endif