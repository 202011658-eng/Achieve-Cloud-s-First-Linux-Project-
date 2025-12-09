# 컴파일러 설정
CC = gcc
CFLAGS = -Wall -g
INCLUDES = -I.

# 서버 소스 파일
SERVER_SRC = server/server_main.c \
             server/src/server_util.c \
             server/src/server_user.c \
             server/src/server_board.c \
             server/src/server_noti.c

# 클라이언트 소스 파일
CLIENT_SRC = client/client_main.c \
             client/src/client_util.c \
             client/src/client_auth.c \
             client/src/client_board.c

# 빌드
all: board_server board_client

board_server: $(SERVER_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o board_server $(SERVER_SRC)

board_client: $(CLIENT_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o board_client $(CLIENT_SRC)

# 삭제
clean:
	rm -f board_server board_client
