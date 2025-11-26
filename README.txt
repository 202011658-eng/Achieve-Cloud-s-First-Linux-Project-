댓글 게시물 알람 추가하고 비밀번호 보안강화 추가하고
코드 다듬기

🖥 server.c 함수 정리 (간단 버전)


handleError : 에러 메시지 출력하고 서버 바로 종료.

sigchld_handler : 자식 프로세스 좀비 안 남게 회수.

getCurrentTime : 현재 시간을 "YYYY-MM-DD HH:MM:SS" 문자열로 만들어줌.

containsBadWord : 문자열에 욕설(금지어) 있는지 검사.

maskBadWords : 문자열 안 욕설 부분을 *로 가려줌.

readUsers : user_data.txt에서 회원 목록 읽어오기.

saveUser : 새 회원 한 명을 user_data.txt에 추가해서 저장.

userExists : 아이디가 이미 존재하는지 확인.

authenticateUser : 아이디+비번이 맞는 계정인지 확인하고 닉네임 돌려줌.

nicknameExists : 닉네임이 이미 존재하는지 확인.

addOnlineUser : 로그인한 유저를 온라인 사용자 목록에 추가.

removeOnlineUser : 로그아웃/끊긴 유저를 온라인 목록에서 제거.

listOnlineUsers : 현재 접속 중인 유저 목록을 문자열로 만들어 클라이언트에 전송.

readPosts : board_data.txt에서 게시글들을 읽어 배열에 채움.

savePosts : 게시글 배열 전체를 board_data.txt에 저장(덮어쓰기).

readComments (댓글 기능 버전) : comment_data.txt에서 댓글 목록 읽기.

saveComments (댓글 기능 버전) : 댓글 목록 전체를 comment_data.txt에 저장.

writePost : 클라이언트와 주고받으면서 새 글 작성해서 파일에 저장.

listPosts : 게시글 목록(번호/제목/작성자/조회수 등) 만들어서 보내줌.

appendCommentsForPost (댓글 기능) : 특정 게시글의 댓글들을 출력 문자열 뒤에 붙임.

readPost : 게시글 하나 찾아서 조회수 1 올리고, 본문 + 댓글까지 묶어서 보내줌.

deletePost : 본인이 쓴 글인지 확인 후, 해당 게시글 삭제.

updatePost : 본인이 쓴 글인지 확인 후, 제목/내용 새로 받아서 수정.

addComment (댓글 기능) : 댓글 내용 받아서 해당 게시글에 댓글 하나 추가.

likePost : 게시글 추천수 +1 하고 저장.

cmpLikes : 추천순(추천 많고, 조회수 많은 순) 정렬에 쓰이는 비교 함수(qsort용).

rankPosts : 게시글들을 추천수 기준으로 정렬해서 인기글 TOP10 보내줌.

searchPosts : 제목/내용/작성자에 키워드가 들어간 글만 찾아서 목록으로 보내줌.

handleClient : 한 클라이언트랑 전체 통신 처리(로그인/글쓰기/삭제/댓글 등 명령 분기).

main : 서버 소켓 만들고 accept+fork로 클라이언트 받는 메인 루프.


💻 client.c 함수 정리 (간단 버전)


handleError : 에러 메시지 출력하고 클라이언트 종료.

getPassword : 비밀번호 입력받을 때 화면에 *로만 찍히게 입력 처리.

printInitialMenu : 로그인/회원가입/종료 초기 메뉴를 출력.

printMainMenu : 글/댓글/추천 등 메인 메뉴를 출력.

registerUser : 서버와 통신하며 아이디/비번/닉네임 보내서 회원가입.

loginUser : 아이디/비번 보내서 로그인 시도, 성공 여부 리턴.

writePostClient : 제목/내용 입력받아서 서버에 새 글 작성 요청.

listPostsClient : 서버에 목록 요청해서 그대로 출력.

readPostClient : 글 번호 입력→ 서버에 READ 요청→ 글 내용+댓글 출력.

deletePostClient : 글 번호, 삭제 확인 받고 서버에 삭제 요청.

updatePostClient : 글 번호, 새 제목/내용 입력해서 서버에 수정 요청.

listOnlineUsersClient : 현재 접속자 목록 요청해서 출력.

likePostClient : 글 번호 입력해서 추천 요청.

rankPostsClient : 인기글(추천순) 목록 요청해서 출력.

searchPostsClient : 키워드 입력해서 검색 결과 목록 출력.

commentPostClient (댓글 기능) : 게시글 번호 + 댓글 내용 입력해서 서버에 댓글 작성 요청.

main : 서버에 연결 → 로그인/회원가입 루프 → 이후 메인 메뉴 반복 처리.
