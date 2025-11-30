🖥 server.c 함수 목록
공통 유틸 / 필터

handleError(const char *message)
→ 시스템 콜 에러가 났을 때 perror 찍고 서버 프로세스를 종료한다.

sigchld_handler(int sig)
→ fork로 만든 자식 프로세스가 끝났을 때 좀비 프로세스를 정리해준다.

getCurrentTime(char *buffer)
→ 현재 시간을 "YYYY-MM-DD HH:MM:SS" 문자열로 만들어 넣어준다.

containsBadWord(const char* text)
→ 문자열에 욕설(금지어) 리스트에 있는 단어가 포함되어 있는지 검사해서 있으면 1을 리턴한다.

maskBadWords(char* text)
→ 문자열 안에 있는 금지어 부분을 * 문자로 모두 가린다.

사용자 관리 (회원가입/로그인)

int readUsers(User users[])
→ user_data.txt에서 모든 사용자 정보를 읽어 배열에 채우고 사용자 수를 돌려준다.

void saveUser(User* user)
→ 새 사용자 정보를 user_data.txt 마지막 줄에 추가 저장한다.

int userExists(const char* username)
→ 같은 아이디(username)를 가진 사용자가 이미 존재하는지 검사한다.

int authenticateUser(const char* username, const char* password, char* nickname)
→ 아이디/비밀번호가 일치하는 사용자를 찾아 로그인 검증하고, 성공 시 그 닉네임을 돌려준다.

int nicknameExists(const char* nickname)
→ 같은 닉네임을 가진 사용자가 이미 있는지 검사한다.

파일 기반 접속자 관리

int readOnlineUsersFromFile(OnlineUser users[], int max_users)
→ online_users.txt에서 현재 접속자 목록을 읽어 배열에 채우고 인원 수를 리턴한다.

void saveOnlineUsersToFile(OnlineUser users[], int count)
→ 메모리에 있는 접속자 배열을 online_users.txt에 모두 덮어쓴다.

void addOnlineUser(const char* username, const char* nickname, const char* ip)
→ 중복 로그인 여부를 확인한 뒤, 새 접속자를 메모리 배열과 파일에 추가한다.

void removeOnlineUser(const char* username)
→ 해당 아이디를 가진 접속자를 목록에서 제거하고 파일도 업데이트한다.

회원가입 / 로그인 / 접속자 목록

void registerUser(int client_sock)
→ 클라이언트와 통신하며 아이디·비밀번호·닉네임을 입력받아 유효성 검사 후 회원가입을 처리한다.

int loginUser(int client_sock, char* username, char* nickname, const char* client_ip)
→ 클라이언트로부터 아이디/비밀번호를 받아 인증하고, 성공 시 접속자 목록에 추가하며 1을 리턴한다.

void listOnlineUsers(int client_sock)
→ 파일 기반 접속자 목록을 읽어서 현재 접속자 닉네임과 접속 경과 시간을 클라이언트에 출력해 준다.

게시글 파일 I/O

int readPosts(Post posts[])
→ board_data.txt에서 모든 게시글을 읽어 배열에 채우고 게시글 개수를 리턴한다.

void savePosts(Post posts[], int count)
→ 게시글 배열 전체를 board_data.txt에 다시 써서 저장한다(쓰기 락으로 동시접근 보호).

댓글 파일 I/O

int readComments(Comment comments[])
→ comment_data.txt에서 모든 댓글을 읽어 배열에 채우고 댓글 개수를 리턴한다.

void saveComments(Comment comments[], int count)
→ 댓글 배열 전체를 comment_data.txt에 덮어써서 저장한다.

알림 파일 I/O / 알림 기능

int readNotifications(Notification notis[])
→ notification_data.txt에서 모든 알림을 읽어 배열에 채우고 알림 개수를 리턴한다.

void saveNotifications(Notification notis[], int count)
→ 알림 배열 전체를 notification_data.txt에 덮어써서 저장한다.

void addNotification(const char* owner, const char* message)
→ 특정 유저(owner)에게 보낼 새 알림 한 개를 만들어 파일에 추가한다.

void showNotifications(int client_sock, const char* nickname)
→ 해당 유저에게 도착한 알림들을 모두 출력해 주고, 출력된 알림은 파일에서 삭제한다.

게시글 관련 기능

void writePost(int client_sock, const char* nickname)
→ 클라이언트에서 제목·내용을 받아 욕설 필터링 후 새 게시글을 생성하고 저장한다.

void listPosts(int client_sock)
→ 모든 게시글의 번호·제목·작성자·추천수·조회수·작성시간을 표 형식으로 클라이언트에 보내준다.

int appendCommentsForPost(char *buffer, int offset, int max_len, int post_id)
→ 특정 게시글 번호에 해당하는 댓글들을 버퍼에 이어 붙여 출력 형식으로 만들어 준다.

void readPost(int client_sock, int post_id)
→ 지정한 게시글 번호를 찾아 조회수를 1 증가시키고, 게시글 내용과 댓글 목록까지 한 번에 보여준다.

void deletePost(int client_sock, int post_id, const char* nickname)
→ 작성자 본인인지 확인한 후 해당 게시글을 목록에서 삭제한다.

void updatePost(int client_sock, int post_id, const char* nickname)
→ 작성자 본인 확인과 욕설 필터링을 거쳐 제목/내용을 수정하고 수정 시간을 갱신한다.

댓글 / 추천 / 인기글 / 검색

void addComment(int client_sock, int post_id, const char* nickname)
→ 게시글 존재 여부 확인 후 댓글 내용을 받아 저장하고, 원글 작성자에게 알림을 추가한다.

void likePost(int client_sock, int post_id, const char* liker_nickname)
→ 해당 게시글의 추천수를 1 증가시키고, 본인이 아닌 다른 사람 글이면 작성자에게 추천 알림을 보낸다.

int cmpViews(const void* a, const void* b)
→ 게시글을 정렬할 때 조회수 기준으로 내림차순, 같으면 추천수 기준으로 비교하는 비교 함수다.

void rankPosts(int client_sock)
→ 게시글을 조회수 기준으로 정렬해서 상위 인기글 목록을 클라이언트에 보여준다.

void searchPosts(int client_sock, const char* keyword)
→ 제목/내용/작성자에 키워드가 포함된 게시글만 찾아 간단한 정보 목록으로 보여준다.

클라이언트 처리 / 메인

void handleClient(int client_sock, char *client_ip)
→ 하나의 클라이언트 소켓에 대해 로그인 상태를 관리하며, 각종 명령(WRITE/READ/UPDATE/…/COMMENT)을 분기 처리한다.

int main(int argc, char *argv[])
→ 서버 소켓을 열고 fork()로 클라이언트를 각각 자식 프로세스에서 처리하는 메인 루프를 수행한다.

💻 client.c 함수 목록
유틸 / 메뉴

handleError(const char* message)
→ 클라이언트 쪽에서 소켓 관련 에러가 나면 메시지 출력 후 프로그램을 종료한다.

getPassword(char* password, int max_len)
→ 터미널 에코를 끄고 사용자가 비밀번호를 입력하면 *만 표시하면서 안전하게 읽어온다.

printInitialMenu()
→ 로그인/회원가입/종료로 구성된 초기 메뉴를 화면에 출력한다.

printMainMenu()
→ 로그인 후 사용할 게시판 메인 메뉴(글쓰기, 목록, 읽기, 수정, 삭제, 추천, 검색, 댓글, 알림, 로그아웃)를 출력한다.

인증 / 회원가입

void registerUser(int sock)
→ 서버와 통신하면서 아이디·비밀번호·닉네임을 입력받아 회원가입 요청을 보내고 결과를 출력한다.

int loginUser(int sock)
→ 아이디와 비밀번호를 서버로 보내 로그인 요청을 하고, 성공 여부에 따라 1 또는 0을 리턴한다.

게시글 관련 클라이언트 기능

void writePostClient(int sock)
→ 제목과 내용을 입력받아 서버에 새 글 작성 요청을 보내고 성공/실패 메시지를 출력한다.

void listPostsClient(int sock)
→ 서버에 글 목록 요청(LIST)을 보내고 전달받은 목록 문자열을 그대로 출력한다.

void readPostClient(int sock)
→ 읽을 게시글 번호를 입력받아 서버에 READ:<id> 요청을 보내고 본문+댓글을 화면에 보여준다.

void deletePostClient(int sock)
→ 삭제할 게시글 번호와 삭제 여부(y/n)를 확인한 뒤 서버에 삭제 요청을 보내고 결과를 출력한다.

void updatePostClient(int sock)
→ 수정할 게시글 번호를 입력받고, 서버가 허용하면 새 제목·내용을 보내 글 수정 요청을 처리한다.

접속자 / 추천 / 인기글 / 검색 / 알림 / 댓글

void listOnlineUsersClient(int sock)
→ 서버에 ONLINE 명령을 보내 현재 접속자 목록을 받아 출력한다.

void likePostClient(int sock)
→ 추천할 게시글 번호를 입력받아 LIKE:<id> 요청을 보내고 결과 메시지를 출력한다.

void rankPostsClient(int sock)
→ 서버에 RANK 명령을 보내 조회수순 인기글 목록을 받아 출력한다.

void searchPostsClient(int sock)
→ 검색 키워드를 입력받아 SEARCH:<keyword> 요청을 보내고 검색 결과를 출력한다.

void showNotificationsClient(int sock)
→ 서버에 NOTI 명령을 보내 현재 로그인한 사용자에게 온 알림 목록을 받아 출력한다.

void commentPostClient(int sock)
→ 댓글을 달 게시글 번호를 입력한 뒤 서버와 COMMENT 프로토콜로 통신하여 댓글 내용을 보내고 결과를 출력한다.

메인 함수

int main(int argc, char* argv[])
→ 서버 IP를 인자로 받아 접속을 맺고, 초기 메뉴(로그인/회원가입)와 메인 메뉴를 반복 출력하면서 사용자의 선택에 따라 각 기능 함수를 호출한다.
