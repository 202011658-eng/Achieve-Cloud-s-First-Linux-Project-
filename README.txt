----추가 기능 넣은것들----

파일 기반 데이터 저장

board_data.txt 파일에 게시글 영구 저장
구조화된 형식으로 저장 (ID|제목|작성자|내용|시간)


파일 잠금 (flock)

동시 접근 시 충돌 방지
읽기 잠금(LOCK_SH), 쓰기 잠금(LOCK_EX) 사용


타임스탬프

게시글 작성 시 자동으로 현재 시간 기록


게시글 최대 개수 제한

최대 100개의 게시글 제한


server.c의 주요 함수:

handleError(const Char *message) : 서버 통신 과정 중 에러 처리 함수
sigchld_handler(int sig) : 좀비 프로세스 방지
getCurrentTime(char *buffer) : 현재 시간 가져옴 (time_t 구조체, strftime() 함수 이용)
containsBadWord(const char* text) : 욕설 필터링 함수 // 전역변수 bad_words[] 에서 해당 원소 포함하는지 strstr() 함수로 비교
maskBadWords(char* text) : 게시글이나 댓글에 포함된 욕설을 필터링하는 함수
readUsers(User users[]) : 유저 파일(USER_FILE - > 텍스트?)을 읽어 type_User 배열에 username/password/nickname 형태로 저장 (읽기 잠금 사용)
-> 잠금 형태 교재에 나온 형태로 바꾸는게 나을듯?
saveUser(User* user) : 유저 파일(USER_FILE) 을 연 뒤 해당 파일에 인자로 받은 USER의 정보를 씀. (쓰기 잠금 사용)




추가 기능 넣은것들----

파일 기반 데이터 저장

board_data.txt 파일에 게시글 영구 저장
구조화된 형식으로 저장 (ID|제목|작성자|내용|시간)


파일 잠금 (flock)

동시 접근 시 충돌 방지
읽기 잠금(LOCK_SH), 쓰기 잠금(LOCK_EX) 사용


타임스탬프

게시글 작성 시 자동으로 현재 시간 기록


게시글 최대 개수 제한

최대 100개의 게시글 제한


server.c의 주요 함수:

handleError(const Char *message) : 서버 통신 과정 중 에러 처리 함수
sigchld_handler(int sig) : 좀비 프로세스 방지
getCurrentTime(char *buffer) : 현재 시간 가져옴 (time_t 구조체, strftime() 함수 이용)
containsBadWord(const char* text) : 욕설 필터링 함수 // 전역변수 bad_words[] 에서 해당 원소 포함하는지 strstr() 함수로 비교
maskBadWords(char* text) : 게시글이나 댓글에 포함된 욕설을 필터링하는 함수




client.c의 주요 함수:

printMenu(): 메뉴 출력
writePost(): 게시글 작성 인터페이스
listPosts(): 목록 보기
readPost(): 게시글 읽기
deletePost(): 게시글 삭제

