#ifndef BOARD_CLIENT_H
#define BOARD_CLIENT_H

void writePostClient(int sock);
void listPostsClient(int sock);
void readPostClient(int sock);
void deletePostClient(int sock);
void updatePostClient(int sock);
void listOnlineUsersClient(int sock);
void likePostClient(int sock);
void rankPostsClient(int sock);
void searchPostsClient(int sock);
void showNotificationsClient(int sock);
void commentPostClient(int sock);

#endif