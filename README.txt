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

readPosts(): 파일에서 게시글 읽기
savePosts(): 게시글을 파일에 저장
writePost(): 새 게시글 작성
listPosts(): 게시글 목록 전송
readPost(): 특정 게시글 읽기
deletePost(): 게시글 삭제
handleClient(): 클라이언트 요청 처리

client.c의 주요 함수:

printMenu(): 메뉴 출력
writePost(): 게시글 작성 인터페이스
listPosts(): 목록 보기
readPost(): 게시글 읽기
deletePost(): 게시글 삭제

