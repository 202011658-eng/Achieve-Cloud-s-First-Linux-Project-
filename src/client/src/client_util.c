#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include "../header/client_util.h"

void handleError(const char* message) {
    perror(message);
    exit(1);
}

void getPassword(char* password, int max_len) {
    struct termios oldt, newt;
    int i = 0;
    char ch;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    while (i < max_len - 1) {
        ch = getchar();
        if (ch == '\n' || ch == '\r') {
            break;
        }
        password[i++] = ch;
        printf("*");
    }
    password[i] = '\0';
    printf("\n");

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}