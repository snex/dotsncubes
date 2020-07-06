#ifndef __NETPLAY_H_
#define __NETPLAY_H_

#include <winsock.h>

#define SOCKET_NOTIFY		WM_USER + 2

#define ERR_WSASTARTUP		1
#define ERR_SOCK_CREATE		2
#define ERR_SOCK_SELECT		3
#define ERR_SOCK_BIND		4
#define ERR_SOCK_LISTEN		5
#define ERR_SOCK_CONNECT	6

#define MSG_HELLO			1
#define MSG_BOARD			2
#define MSG_READY			3
#define MSG_GO				4
#define MSG_NEWGAME			5
#define MSG_MOVE			6
#define MSG_CHAT			7
#define MSG_EXIT			8

#pragma comment( lib, "wsock32.lib" )

int init_socket(WSADATA *WSAData, SOCKET *sock);
int init_server(SOCKET *sock, SOCKADDR_IN *sa, HWND hWnd, unsigned short port);
int init_client(SOCKET *sock, SOCKADDR_IN *sa, HWND hWnd, char *address, unsigned short port);

DWORD WINAPI handle_events(PVOID pvoid);
//int handle_events(SOCKET *sock, WSAEVENT *WSAEvent);

void destroy_socket(SOCKET *sock);

#endif