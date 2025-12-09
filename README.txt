==============================25/12/02 수정사항==============================

server.c client.c 두개 파일이 너무 길어져서 server / client 의 소스 파일과 헤더 파일로 분리

server 측과 client 측 디렉토리로 분리

server/server_main.c -> 서버 측 메인 함수
server/header/ -> board, noti, user, util 소스 파일을 위한 헤더 파일들 저장 디렉토리
server/src -> board, noti, user, util 소스 파일

client/server_main.c -> 클라이언트 측 메인 함수
client/header/ -> auth, board, util 소스 파일을 위한 헤더 파일 저장 디렉토리
client/src -> auth, board, util 소스 파일


server 측 소스 파일

server_main.c 
다중 프로세스 TCP 서버 엔트리 포인트, 클라이언트 연결 후 socket에서 명령 받아 handleClient() 에서 명령 분기
(REGISTER/LOGIN/WRITE/LIST/READ/DELETE/ONLINE/NOTI/UPDATE/LIKE/RANK/SEARCH/COMMENT/QUIT)
소켓 초기화 및 fork, 시그널 처리

server_util.c
서버 유틸을 담당하는 소스 파일
handleError() -> 예외 / 에러 종료 함수
sigchld_handler() -> 좀비 프로세스 처리 함수
getCurrentTime() -> 현재 시각 포맷
containsBadWord(), maskBadWords() -> 금칙어 (욕설) 검사 함수, 마스킹 함수

server_user.c
사용자 접속 / 관리 소스 파일
readUsers(), saveUser() -> 유저 데이터베이스에서 사용자 읽기 / 저장 함수
userExists(), authenticateUser(), nicknameExists() -> 유저 존재 / 인증 / 닉네임 중복 검사 관련 함수
readOnlineUsersFromFile(), saveOnlineUsersToFile() -> 온라인 접속 유저 목록 / 저장 함수
addOnlineUser(), removeOnlineUser(), listOnlineUsers -> 온라인 접속자 목록 추가 / 삭제 (온라인 접속 유저는 텍스트 DB로 서버측에서 따로 관리) / 온라인 접속자 목록 출력
registerUser(), loginUser() -> 회원가입 / 로그인 프로토콜 처리

server_board.c
게시글 CRUD, 검색, 통계, 게시글 로드/저장 관련 소스 파일
readPost(), savePost() -> 게시글 로드 / 저장
writePost(), listPosts(), readPost() -> 글 작성 / 글 목록 / 글 조회
deletePost(), updatePost() -> 글 삭제 / 수정
likePost() -> 게시글 추천
cmpViews(), rankPosts() -> 조회수 / 추천수 기반 정렬 (qsort 사용, cmpViews 비교 함수 이용)
searchPosts -> 키워드 검색


server_noti.c
댓글 / 알림 관리 소스 파일 -> 댓글 데이터는 comment_data.txt 에서 텍스트 DB로 따로 관리 (Comment Struct) -> post_id로 어떤 post에 달린 댓글인지 구분 
readComments(), saveComments(), readNotifications(), saveNotifications() -> 댓글/알림 로드/저장
addNotification(), readNotifications() -> 알림 추가 / 표시
addComment() -> 댓글 작성 프로토콜 (댓글 배열 로드 -> 클라이언트 측 콘텐츠 요청 -> 댓글 구조체 생성 -> 알림 전송 -> 댓글 DB에 저장)
appendCommentsForPost() -> 게시글 상세에 댓글 덧붙이기


client 측 소스 파일

client_main.c
콘솔 클라이언트 엔트리. 사용자 입력에 따라 각 기능 함수 호출 -> 서버 측과 socket 통신, 종료 처리
printInitialMenu(), printMainMenu() -> 서버 연결 후 초기 (비로그인) / 메인 (로그인) 메뉴 출력

client_util.c
클라이언트 유틸 담당 소스 파일
handleError() -> 예외 / 에러 종료 함수
getPassword() -> 입력 비-ECHO 비밀번호 받기

client_auth.c
회원가입 / 로그인 클라이언트 관련 소스 파일
registerUser(), loginUser() -> 서버와 REGISTER/LOGIN 프로토콜 주고 받으며 입력 처리 (유저 생성 / 로그인)

client_board.c
게시판 기능 클라이언트 관련 소스 파일
writePostClient(), listPostsClient(), readPostClient(), updatePostClient(), deletePostClient() -> 게시판 CRUD, 서버와 통신하며 각각 CRUD 기능 수행
listOnlineUsersClient() -> 온라인 유저 목록 서버에서 받아서 클라이언트 측에 출력
likePostClient() -> 게시글 추천 기능 함수
rankPostsClient() -> 조회수 순 랭킹으로 출력 (UI) (서버 측에서 정렬 후 출력)
searchPostsClient() -> 게시글 검색
showNotificationsClient() -> 알림 확인
commentPostClient() -> 댓글 작성


==============================25/12/09 수정사항==============================
sigint_handler -> server_main.c 파일에 추가
-> 서버 종료 시 서버가 종료되었음을 표시

==============================MakeFile==============================
MakeFile 내용 :

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

빌드하는법 :
# 전체 빌드
make

# 서버만 빌드
make board_server

# 클라이언트만 빌드
make board_client

# 실행 파일 삭제
make clean

실행:
# 서버 실행
./board_server

# 클라이언트 실행 (다른 터미널)
./board_client 127.0.0.1


==============================12/10 추가사항==============================

게시판 데이터는 src/server/data에 따로 보관하게 함.
시뮬레이션 확인 -> 잘 돌아가므로 main branch로 승격.







