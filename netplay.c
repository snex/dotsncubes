#include <windows.h>
#include "netplay.h"

int init_socket(WSADATA *WSAData, SOCKET *sock) {

	if (WSAStartup(MAKEWORD(2,0), WSAData)) {
			
		return ERR_WSASTARTUP;

	}

	*sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (sock == INVALID_SOCKET) {

		WSACleanup();

		return ERR_SOCK_CREATE;

	}
/*
	if (SOCKET_ERROR == WSAAsyncSelect(*sock, hWnd, SOCKET_NOTIFY, FD_READ|FD_ACCEPT|FD_CONNECT|FD_CLOSE)) {

		closesocket(*sock);
		WSACleanup();

		return ERR_SOCK_SELECT;

	}
*/	
	return 0;

}

int init_server(SOCKET *sock, SOCKADDR_IN *sa, HWND hWnd, unsigned short port) {

	if (SOCKET_ERROR == WSAAsyncSelect(*sock, hWnd, SOCKET_NOTIFY, FD_READ|FD_ACCEPT|FD_CLOSE)) {

		closesocket(*sock);
		WSACleanup();

		return ERR_SOCK_SELECT;

	}

	sa->sin_family = AF_INET;
	sa->sin_addr.S_un.S_addr = 0;
	sa->sin_port = htons(port);

	if (SOCKET_ERROR == bind(*sock, (LPSOCKADDR)sa, sizeof(*sa))) {

		closesocket(*sock);
		WSACleanup();

		return ERR_SOCK_BIND;

	}

	if (SOCKET_ERROR == listen(*sock, 1)) {

		closesocket(*sock);
		WSACleanup();

		return ERR_SOCK_LISTEN;

	}

	return 0;

}

int init_client(SOCKET *sock, SOCKADDR_IN *sa, HWND hWnd, char *address, unsigned short port) {

	if (SOCKET_ERROR == WSAAsyncSelect(*sock, hWnd, SOCKET_NOTIFY, FD_READ|FD_CONNECT|FD_CLOSE)) {

		closesocket(*sock);
		WSACleanup();

		return ERR_SOCK_SELECT;

	}
	
	sa->sin_family = AF_INET;
	sa->sin_port = htons(port);
	sa->sin_addr.S_un.S_addr = inet_addr(TEXT(address));

	connect(*sock, (SOCKADDR *)sa, sizeof(*sa));

	if (WSAEWOULDBLOCK != WSAGetLastError()) {

		closesocket(*sock);
		WSACleanup();

		return ERR_SOCK_CONNECT;

	}

	return 0;

}

void destroy_socket(SOCKET *sock) {

	closesocket(*sock);

}