#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <sys/wait.h>
#include "common.h"
#include "server/header/server_util.h"

// 욕설 목록 (내부 전역 변수)
static const char* bad_words[] = {
    "욕설1", "욕설2", "바보",
    NULL
};

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