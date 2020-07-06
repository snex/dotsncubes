#include <windows.h> 
#include <process.h>
#include <stdio.h>
#include <time.h>
#include <GL/gl.h> 
#include <GL/glu.h> 
#include "board.h"
#include "camera.h"
#include "player.h"
#include "cpu-ai.h"
#include "resource.h"
#include "netplay.h"
//#include "tga.h"

#define WIDTH           825 
#define HEIGHT          625 

#define NUM_CAMS		2
#define OUTSIDE_CAM		0
#define CENTER_CAM		1

#define IDB_SENDBUTTON	12345

#define GAME_MADE_MOVE WM_USER + 1

#define SWAPBUFFERS SwapBuffers(ghDC);

#pragma comment( lib, "opengl32.lib" )
#pragma comment( lib, "glu32.lib" )
  
struct Board board;
struct Camera cams[NUM_CAMS];
struct Player players[2];

typedef struct {
    HANDLE hEvent;
    int diff[2];
} PARAMS, *PPARAMS;

CHAR szAppName[]="DotsNCubes"; 
HINSTANCE hInst;
HWND  ghWnd; 
HDC   ghDC; 
HGLRC ghRC;
static PARAMS params;
GLdouble mouse_x, mouse_y;
GLfloat cube_width, cube_height, cube_depth;
GLint curLine = -1;
TCHAR buff[1024], buff2[10000], buff3[1024], buff4[1024], buff5[1024];
TCHAR sendBuff[1024];
TCHAR recvBuff[1024];
TCHAR chatBuff[1024];
TCHAR remoteAddr[15];
int len, base;
int curCam = OUTSIDE_CAM;
int curPlayer = PLAYER_ONE;
BOOL canMove = FALSE;
BOOL gameInProgress = FALSE;
BOOL listening = FALSE;
BOOL connecting = FALSE;
BOOL sendMoveToRemote = FALSE;
WNDPROC defEditProc;
WSADATA WSAData;
SOCKET sock;
struct sockaddr_in sa;
int sa_len;
static SOCKET serverSock;
static SOCKADDR sa2;
HWND internetDlg;
//RGBImg *img;
 
LONG WINAPI MainWndProc (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK glWinProc (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK editWinProc (HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK optionsDlgProc (HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK internetDlgProc (HWND, UINT, WPARAM, LPARAM);

BOOL bSetupPixelFormat(HDC); 
GLvoid resize(GLsizei, GLsizei); 
GLvoid initializeGL(GLsizei, GLsizei); 
GLvoid drawScene(GLenum); 
GLint Selection(void);

GLvoid BuildFont(GLvoid);
GLvoid KillFont(GLvoid);
GLvoid glPrint(const char *fmt, ...);

DWORD WINAPI CPUMove(PVOID pvoid);
 
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) 
{ 
    MSG        msg; 
    WNDCLASS   wndclass; 

	hInst = hInstance;
  
    wndclass.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC; 
    wndclass.lpfnWndProc   = (WNDPROC)MainWndProc; 
    wndclass.cbClsExtra    = 0; 
    wndclass.cbWndExtra    = 0; 
    wndclass.hInstance     = hInstance; 
    wndclass.hIcon         = LoadIcon (hInstance, szAppName); 
    wndclass.hCursor       = LoadCursor (NULL,IDC_ARROW); 
    wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW); 
    wndclass.lpszMenuName =  (LPCSTR)IDR_MENU1;
    wndclass.lpszClassName = szAppName; 
 
    if (!RegisterClass (&wndclass) ) 
        return FALSE; 

	wndclass.lpszClassName = TEXT("glWin");
	wndclass.lpfnWndProc = glWinProc;
	wndclass.hIcon = NULL;
	wndclass.lpszMenuName = NULL;

	if (!RegisterClass (&wndclass) )
		return FALSE;

	init_board(&board, 8, 5, 1);
//	img=readTGAImg("galaxy.tga");

    ghWnd = CreateWindow (szAppName, 
             "Dots'N'Cubes", 
             WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 
             CW_USEDEFAULT, 
             CW_USEDEFAULT, 
             WIDTH, 
             HEIGHT, 
             NULL, 
             NULL, 
             hInstance, 
             NULL); 
  
    if (!ghWnd) 
        return FALSE; 
  
    ShowWindow (ghWnd, nCmdShow); 
 
    UpdateWindow (ghWnd); 
  
    while (1) { 
 
        while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE) == TRUE) 
        { 
            if (GetMessage(&msg, NULL, 0, 0) ) 
            { 
                TranslateMessage(&msg); 
                DispatchMessage(&msg); 
            } else { 
                return TRUE; 
            } 
        } 
        drawScene(GL_RENDER); 
    } 
} 
 
LONG WINAPI MainWndProc ( 
    HWND    hWnd, 
    UINT    uMsg, 
    WPARAM  wParam, 
    LPARAM  lParam) 
{ 
    LONG    lRet = 1; 
    PAINTSTRUCT    ps; 
	static HWND textWindow, editWindow, sendButton, glWindow;
	TCHAR tmpBuff[100];
	HDC hdc;
	RECT rect;
	int tmp, i;
	static int expectedMsg;
	WORD wEvent, wError;
	static HANDLE hEvent;
	static HANDLE hThread;
	static HMENU hMenu;
	static HBRUSH hBrushTextWin;
	static BOOL connectionValid = FALSE;

    switch (uMsg) { 
 
    case WM_CREATE: 
		textWindow = CreateWindow(TEXT("edit"),
                                  TEXT(""),
                                  WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
                                  10,
							      460,
							      790,
							      80,
						          hWnd,
							      NULL,
							      hInst,
							      NULL);
		editWindow = CreateWindow(TEXT("edit"),
		                          TEXT(""),
		                          WS_CHILD | WS_VISIBLE | WS_BORDER,
		                          10,
		                          545,
		                          700,
		                          20,
		                          hWnd,
		                          NULL,
		                          hInst,
		                          NULL);
        sendButton = CreateWindow(TEXT("button"),
                                  TEXT("Send"),
                                  WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                                  720,
                                  545,
                                  80,
                                  20,
                                  hWnd,
                                  (HMENU)IDB_SENDBUTTON,
                                  hInst,
                                  NULL);

		glWindow = CreateWindow(  TEXT("glWin"),
                                  TEXT(""),
                                  WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                                  0,
                                  0,
                                  WIDTH,
                                  450,
                                  hWnd,
                                  0,
                                  hInst,
                                  NULL);
		init_player(&players[0], HOTSEAT);
		params.diff[0] = DIFFICULTY_HARD;
		init_player(&players[1], CPU);
		params.diff[1] = DIFFICULTY_MEDIUM;
		hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		params.hEvent = hEvent;
		hThread = CreateThread(NULL, 0, CPUMove, &params, 0, NULL);
		hMenu = GetMenu(hWnd);
		hBrushTextWin = CreateSolidBrush(RGB(0, 0, 0));
		defEditProc = (WNDPROC)SetWindowLong(editWindow, GWL_WNDPROC, (LONG)editWinProc);
        break; 

    case WM_PAINT: 
        BeginPaint(hWnd, &ps); 
        EndPaint(hWnd, &ps); 
        break; 
 
    case WM_SIZE:
        break; 

	case WM_COMMAND:

		switch(LOWORD(wParam)) {
		case IDB_SENDBUTTON:
			
			if (GetWindowTextLength(editWindow) > 0) {
				
				if (GetWindowTextLength(textWindow) + GetWindowTextLength(editWindow) + 3 >= 1024) {

					for (i = 0; i < GetWindowTextLength(textWindow); i++) {

						chatBuff[i] = chatBuff[i+GetWindowTextLength(editWindow)+3];

					}

				}

				GetWindowText(editWindow, buff3, GetWindowTextLength(editWindow) + 1);

				if (connectionValid) {

					memset(sendBuff, 0, sizeof(sendBuff));
					wsprintf(sendBuff, "%d:%s:", MSG_CHAT, buff3);
					send(sock, sendBuff, sizeof(sendBuff), 0);

				}
				
				if (GetWindowTextLength(textWindow) > 0)
					wsprintf(chatBuff, "%s\r\n>%s", chatBuff, buff3);
				else
					wsprintf(chatBuff, ">%s", buff3);

				SetWindowText(textWindow, chatBuff);
				SetWindowText(editWindow, "");
				SetFocus(editWindow);
				SendMessage(textWindow, EM_SETSEL, GetWindowTextLength(textWindow), GetWindowTextLength(textWindow));
				SendMessage(textWindow, EM_SCROLLCARET, 0, 0);

			}

			break;

		case IDM_FILE_NEW:

			if (!gameInProgress && !connecting) {

				gameInProgress = TRUE;
				init_board(&board, board.x, board.y, board.z);
				players[0].score = 0;
				players[1].score = 0;
				curPlayer = PLAYER_ONE;
				cube_width = 6 / (float)board.x;
				cube_height = 3 / (float)board.y;
				cube_depth = 4 / (float)board.z;
				update_camera(&cams[OUTSIDE_CAM], 90.0, 0.0, 0.0, 0.0, -6.8);
				update_camera(&cams[CENTER_CAM], 90.0, 0.0, 0.0, 0.0, 0.0);
				
				if ((players[0].type == CPU) && (players[1].type != REMOTE_USER))
					SetEvent(hEvent);
				
				if (players[1].type == REMOTE_USER) {
	
					memset(sendBuff, 0, sizeof(sendBuff));
					wsprintf(sendBuff, "%d:%d:%d:%d:", MSG_BOARD, board.z, board.y, board.x);
					send(sock, sendBuff, sizeof(sendBuff), 0);
					canMove = TRUE;

				}
				
				if (players[0].type == HOTSEAT);
					canMove = TRUE;

				EnableMenuItem(hMenu, IDM_FILE_OPTIONS, MF_GRAYED);

			}

			break;

		case IDM_FILE_OPTIONS:
			
			if (!gameInProgress)
				
				if (DialogBox(hInst, (LPCSTR)IDD_OPTIONS, hWnd, optionsDlgProc))
					DialogBox(hInst, (LPCSTR)IDD_INTERNET, hWnd, internetDlgProc);

			break;

		case IDM_FILE_EXIT:
			SendMessage(hWnd, WM_CLOSE, 0, 0);
			break;

		}

		break;

	case WM_CTLCOLORSTATIC:
		SetBkColor((HDC)wParam, RGB(0, 0, 0));
		SetTextColor((HDC)wParam, RGB(255, 255, 255));

		return (LRESULT)hBrushTextWin;

	case WM_CTLCOLOREDIT:
		SetBkColor((HDC)wParam, RGB(0, 0, 0));
		SetTextColor((HDC)wParam, RGB(255, 255, 255));

		return (LRESULT)hBrushTextWin;
 
    case WM_CLOSE: 
        DestroyWindow (hWnd); 
        break; 
 
    case WM_DESTROY: 
		destroy_board(&board);
		destroy_socket(&sock);
		destroy_socket(&serverSock);
//		deallocRGBImg(img);
        PostQuitMessage (0); 
        break; 

	case WM_CHAR:
		switch ((TCHAR)wParam) {
		case 'C':
		case 'c':
			curCam = (curCam == OUTSIDE_CAM) ? CENTER_CAM : OUTSIDE_CAM;
			break;

		default:
			break;
		}
		break;
    
    case WM_KEYDOWN: 
        switch (wParam) { 
		case VK_F1:
			SendMessage(hWnd, WM_COMMAND, IDM_FILE_OPTIONS, 0);
			break;
        case VK_F2:
			SendMessage(hWnd, WM_COMMAND, IDM_FILE_NEW, 0);
			break;

        } 

		break;
 
	case GAME_MADE_MOVE:
		
		if (board.lines[wParam].filled != NOT_FILLED)
			break;

		if (((sendMoveToRemote) && (curPlayer == PLAYER_ONE) && (players[1].type == REMOTE_USER)) ||
			((sendMoveToRemote) && (curPlayer == PLAYER_TWO) && (players[0].type == REMOTE_USER)))
		{
		
			memset(sendBuff, 0, sizeof(sendBuff));
			wsprintf(sendBuff, "%d:%d:", MSG_MOVE, (int)wParam);
			send(sock, sendBuff, sizeof(sendBuff), 0);

		}

		tmp = update_board(&board, wParam, curPlayer);
			
		if (tmp == 0) {

			curPlayer = (curPlayer == PLAYER_ONE) ? PLAYER_TWO : PLAYER_ONE;

		}
			
		players[curPlayer-1].score += tmp;
			
		if (game_over(&board)) {

			canMove = FALSE;

			if (players[1].type == REMOTE_USER)
				expectedMsg = MSG_READY;
			if (players[0].type == REMOTE_USER)
				expectedMsg = MSG_BOARD;

			gameInProgress = FALSE;
			curLine = -1;
			curPlayer = PLAYER_ONE;
			EnableMenuItem(hMenu, IDM_FILE_OPTIONS, MF_ENABLED);
			break;

		}

		if (players[curPlayer-1].type == CPU) {
					
			canMove = FALSE;
			SetEvent(hEvent);

		}
		else if (players[curPlayer-1].type == REMOTE_USER) {

			canMove = FALSE;
			expectedMsg = MSG_MOVE;

		}
		else {

			canMove = TRUE;
			expectedMsg = 0;

		}

		break;

	case SOCKET_NOTIFY:

		wEvent = WSAGETSELECTEVENT(lParam);
		wError = WSAGETSELECTERROR(lParam);

		switch (wEvent) {
		case FD_ACCEPT:

			sa_len = sizeof(SOCKADDR_IN);

			sock = accept(serverSock, (SOCKADDR*)&sa, &sa_len);

			if (sock == INVALID_SOCKET) {

				wsprintf(buff, "Error in accept.\r\nError: %d", wError);
				MessageBox(hWnd, buff, "Error", MB_ICONERROR);
				listening = FALSE;
				canMove = FALSE;
				curPlayer = PLAYER_ONE;
				gameInProgress = FALSE;
				curLine = -1;
				EnableMenuItem(hMenu, IDM_FILE_OPTIONS, MF_ENABLED);
				players[1].type = CPU;
				destroy_socket(&sock);
				destroy_socket(&serverSock);
				WSACleanup();
			
				break;

			}

			EndDialog(internetDlg, FALSE);

			if (wError) {

				wsprintf(buff, "Accept error.\r\nError: %d", wError);
				MessageBox(hWnd, buff, "Error", MB_ICONERROR);
				listening = FALSE;
				canMove = FALSE;
				curPlayer = PLAYER_ONE;
				gameInProgress = FALSE;
				curLine = -1;
				EnableMenuItem(hMenu, IDM_FILE_OPTIONS, MF_ENABLED);
				players[1].type = CPU;
				destroy_socket(&sock);
				destroy_socket(&serverSock);
				WSACleanup();

				break;

			}

			wsprintf(remoteAddr, "%s", inet_ntoa(sa.sin_addr));

			if (GetWindowTextLength(textWindow) + strlen(remoteAddr) + 16 >= 1024) {

				for (i = 0; i < GetWindowTextLength(textWindow); i++) {

					chatBuff[i] = chatBuff[i+strlen(remoteAddr)+16];

				}

			}
				
			if (GetWindowTextLength(textWindow) > 0)
				wsprintf(chatBuff, "%s\r\nConnected to: %s", chatBuff, remoteAddr);
			else
				wsprintf(chatBuff, "Connected to: %s", remoteAddr);

			SetWindowText(textWindow, chatBuff);
			SendMessage(textWindow, EM_SETSEL, GetWindowTextLength(textWindow), GetWindowTextLength(textWindow));
			SendMessage(textWindow, EM_SCROLLCARET, 0, 0);
			expectedMsg = MSG_HELLO;
			destroy_socket(&serverSock);

			break;

		case FD_CONNECT:

			EndDialog(internetDlg, FALSE);

			if (wError) {

				wsprintf(buff, "Connect error.\r\nError: %d", wError);
				MessageBox(hWnd, buff, "Error", MB_ICONERROR);
				listening = FALSE;
				canMove = FALSE;
				curPlayer = PLAYER_ONE;
				gameInProgress = FALSE;
				curLine = -1;
				EnableMenuItem(hMenu, IDM_FILE_OPTIONS, MF_ENABLED);
				players[1].type = CPU;
				destroy_socket(&sock);
				destroy_socket(&serverSock);
				WSACleanup();

				break;

			}

			wsprintf(remoteAddr, "%s", inet_ntoa(sa.sin_addr));

			if (GetWindowTextLength(textWindow) + strlen(remoteAddr) + 16 >= 1024) {

				for (i = 0; i < GetWindowTextLength(textWindow); i++) {

					chatBuff[i] = chatBuff[i+strlen(remoteAddr)+16];

				}

			}
				
			if (GetWindowTextLength(textWindow) > 0)
				wsprintf(chatBuff, "%s\r\nConnected to: %s", chatBuff, remoteAddr);
			else
				wsprintf(chatBuff, "Connected to: %s", remoteAddr);

			SetWindowText(textWindow, chatBuff);
			SendMessage(textWindow, EM_SETSEL, GetWindowTextLength(textWindow), GetWindowTextLength(textWindow));
			SendMessage(textWindow, EM_SCROLLCARET, 0, 0);			
			memset(sendBuff, 0, sizeof(sendBuff));
			wsprintf(sendBuff, "%d:", MSG_HELLO);
			send(sock, sendBuff, sizeof(sendBuff), 0);
			players[0].type = REMOTE_USER;
			expectedMsg = MSG_BOARD;
			connectionValid = TRUE;

			break;

		case FD_READ:

			if (listening || connecting) {
				
				memset(recvBuff, 0, sizeof(recvBuff));
				recv(sock, (CHAR *)&recvBuff, sizeof(recvBuff), 0);

				if (wError) {

					wsprintf(buff, "Receive error.\r\nError: %d", wError);
					MessageBox(hWnd, buff, "Error", MB_ICONERROR);
					listening = FALSE;
					canMove = FALSE;
					curPlayer = PLAYER_ONE;
					gameInProgress = FALSE;
					curLine = -1;
					EnableMenuItem(hMenu, IDM_FILE_OPTIONS, MF_ENABLED);
					players[1].type = CPU;
					destroy_socket(&sock);
					destroy_socket(&serverSock);
					WSACleanup();

					break;

				}

				switch (atoi(strtok(recvBuff, ":"))) {
				case MSG_HELLO:
					
					if (expectedMsg != MSG_HELLO) {

						connectionValid = FALSE;
					
						if (GetWindowTextLength(textWindow) + strlen(remoteAddr) + 47 >= 1024) {

							for (i = 0; i < GetWindowTextLength(textWindow); i++) {

								chatBuff[i] = chatBuff[i+strlen(remoteAddr)+47];

							}

						}
				
						if (GetWindowTextLength(textWindow) > 0)
							wsprintf(chatBuff, "%s\r\nDisconnected from: %s: Illegal connection type.", chatBuff, remoteAddr);
						else
							wsprintf(chatBuff, "Disconnected from: %s: Illegal connection type.", remoteAddr);

						SetWindowText(textWindow, chatBuff);
						SendMessage(textWindow, EM_SETSEL, GetWindowTextLength(textWindow), GetWindowTextLength(textWindow));
						SendMessage(textWindow, EM_SCROLLCARET, 0, 0);
						listening = FALSE;
						canMove = FALSE;
						curPlayer = PLAYER_ONE;
						gameInProgress = FALSE;
						curLine = -1;
						EnableMenuItem(hMenu, IDM_FILE_OPTIONS, MF_ENABLED);
						players[0].type = HOTSEAT;
						players[1].type = CPU;
						expectedMsg = 0;
						destroy_socket(&sock);
						destroy_socket(&serverSock);
						WSACleanup();

						break;

					}

					connectionValid = TRUE;
					players[1].type = REMOTE_USER;
					expectedMsg = MSG_READY;
					
					break;

				case MSG_BOARD:

					if (expectedMsg != MSG_BOARD) {

						connectionValid = FALSE;

						if (GetWindowTextLength(textWindow) + strlen(remoteAddr) + 47 >= 1024) {

							for (i = 0; i < GetWindowTextLength(textWindow); i++) {

								chatBuff[i] = chatBuff[i+strlen(remoteAddr)+47];

							}

						}
				
						if (GetWindowTextLength(textWindow) > 0)
							wsprintf(chatBuff, "%s\r\nDisconnected from: %s: Illegal connection type.", chatBuff, remoteAddr);
						else
							wsprintf(chatBuff, "Disconnected from: %s: Illegal connection type.", remoteAddr);

						SetWindowText(textWindow, chatBuff);
						SendMessage(textWindow, EM_SETSEL, GetWindowTextLength(textWindow), GetWindowTextLength(textWindow));
						SendMessage(textWindow, EM_SCROLLCARET, 0, 0);
						connecting = FALSE;
						canMove = FALSE;
						curPlayer = PLAYER_ONE;
						gameInProgress = FALSE;
						curLine = -1;
						EnableMenuItem(hMenu, IDM_FILE_OPTIONS, MF_ENABLED);
						players[0].type = HOTSEAT;
						players[1].type = CPU;
						expectedMsg = 0;
						destroy_socket(&sock);
						WSACleanup();

						break;

					}

					expectedMsg = MSG_NEWGAME;
					init_board(&board, atoi(strtok(NULL, ":")), atoi(strtok(NULL, ":")), atoi(strtok(NULL, ":")));
					gameInProgress = TRUE;
					curPlayer = PLAYER_ONE;
					players[0].score = 0;
					players[1].score = 0;
					cube_width = 6 / (float)board.x;
					cube_height = 3 / (float)board.y;
					cube_depth = 4 / (float)board.z;
					update_camera(&cams[OUTSIDE_CAM], 90.0, 0.0, 0.0, 0.0, -6.8);
					update_camera(&cams[CENTER_CAM], 90.0, 0.0, 0.0, 0.0, 0.0);
					EnableMenuItem(hMenu, IDM_FILE_OPTIONS, MF_GRAYED);
					memset(sendBuff, 0, sizeof(sendBuff));
					wsprintf(sendBuff, "%d:", MSG_READY);
					send(sock, sendBuff, sizeof(sendBuff), 0);

					break;

				case MSG_READY:

					if (expectedMsg != MSG_READY) {

						connectionValid = FALSE;
						
						if (GetWindowTextLength(textWindow) + strlen(remoteAddr) + 47 >= 1024) {

							for (i = 0; i < GetWindowTextLength(textWindow); i++) {

								chatBuff[i] = chatBuff[i+strlen(remoteAddr)+47];

							}

						}
				
						if (GetWindowTextLength(textWindow) > 0)
							wsprintf(chatBuff, "%s\r\nDisconnected from: %s: Illegal connection type.", chatBuff, remoteAddr);
						else
							wsprintf(chatBuff, "Disconnected from: %s: Illegal connection type.", remoteAddr);

						SetWindowText(textWindow, chatBuff);
						SendMessage(textWindow, EM_SETSEL, GetWindowTextLength(textWindow), GetWindowTextLength(textWindow));
						SendMessage(textWindow, EM_SCROLLCARET, 0, 0);
						listening = FALSE;
						canMove = FALSE;
						curPlayer = PLAYER_ONE;
						gameInProgress = FALSE;
						curLine = -1;
						EnableMenuItem(hMenu, IDM_FILE_OPTIONS, MF_ENABLED);
						players[0].type = HOTSEAT;
						players[1].type = CPU;
						expectedMsg = 0;
						destroy_socket(&sock);
						destroy_socket(&serverSock);
						WSACleanup();

						break;

					}

					memset(sendBuff, 0, sizeof(sendBuff));
					wsprintf(sendBuff, "%d:", MSG_NEWGAME);
					send(sock, sendBuff, sizeof(sendBuff), 0);
					expectedMsg = MSG_GO;

					break;

				case MSG_GO:

					if (expectedMsg != MSG_GO) {

						connectionValid = FALSE;
						
						if (GetWindowTextLength(textWindow) + strlen(remoteAddr) + 47 >= 1024) {

							for (i = 0; i < GetWindowTextLength(textWindow); i++) {

								chatBuff[i] = chatBuff[i+strlen(remoteAddr)+47];

							}

						}
				
						if (GetWindowTextLength(textWindow) > 0)
							wsprintf(chatBuff, "%s\r\nDisconnected from: %s: Illegal connection type.", chatBuff, remoteAddr);
						else
							wsprintf(chatBuff, "Disconnected from: %s: Illegal connection type.", remoteAddr);

						SetWindowText(textWindow, chatBuff);
						SendMessage(textWindow, EM_SETSEL, GetWindowTextLength(textWindow), GetWindowTextLength(textWindow));
						SendMessage(textWindow, EM_SCROLLCARET, 0, 0);
						listening = FALSE;
						canMove = FALSE;
						curPlayer = PLAYER_ONE;
						gameInProgress = FALSE;
						curLine = -1;
						EnableMenuItem(hMenu, IDM_FILE_OPTIONS, MF_ENABLED);
						players[0].type = HOTSEAT;
						players[1].type = CPU;
						expectedMsg = 0;
						destroy_socket(&sock);
						destroy_socket(&serverSock);
						WSACleanup();

						break;

					}

					sendMoveToRemote = TRUE;

					if (players[0].type == CPU)
						SetEvent(hEvent);

					expectedMsg = 0;

					break;

				case MSG_NEWGAME:

					if (expectedMsg != MSG_NEWGAME) {

						connectionValid = FALSE;
						
						if (GetWindowTextLength(textWindow) + strlen(remoteAddr) + 47 >= 1024) {

							for (i = 0; i < GetWindowTextLength(textWindow); i++) {

								chatBuff[i] = chatBuff[i+strlen(remoteAddr)+47];

							}

						}
				
						if (GetWindowTextLength(textWindow) > 0)
							wsprintf(chatBuff, "%s\r\nDisconnected from: %s: Illegal connection type.", chatBuff, remoteAddr);
						else
							wsprintf(chatBuff, "Disconnected from: %s: Illegal connection type.", remoteAddr);

						SetWindowText(textWindow, chatBuff);
						SendMessage(textWindow, EM_SETSEL, GetWindowTextLength(textWindow), GetWindowTextLength(textWindow));
						SendMessage(textWindow, EM_SCROLLCARET, 0, 0);
						connecting = FALSE;
						canMove = FALSE;
						curPlayer = PLAYER_ONE;
						gameInProgress = FALSE;
						curLine = -1;
						EnableMenuItem(hMenu, IDM_FILE_OPTIONS, MF_ENABLED);
						players[0].type = HOTSEAT;
						players[1].type = CPU;
						expectedMsg = 0;
						destroy_socket(&sock);
						WSACleanup();

						break;

					}

					expectedMsg = MSG_MOVE;
					sendMoveToRemote = TRUE;
					canMove = FALSE;
					gameInProgress = TRUE;
					memset(sendBuff, 0, sizeof(sendBuff));
					wsprintf(sendBuff, "%d:", MSG_GO);
					send(sock, sendBuff, sizeof(sendBuff), 0);

					break;

				case MSG_MOVE:

					if (expectedMsg != MSG_MOVE) {

						connectionValid = FALSE;
						
						if (GetWindowTextLength(textWindow) + strlen(remoteAddr) + 47 >= 1024) {

							for (i = 0; i < GetWindowTextLength(textWindow); i++) {

								chatBuff[i] = chatBuff[i+strlen(remoteAddr)+47];

							}

						}
				
						if (GetWindowTextLength(textWindow) > 0)
							wsprintf(chatBuff, "%s\r\nDisconnected from: %s: Illegal connection type.", chatBuff, remoteAddr);
						else
							wsprintf(chatBuff, "Disconnected from: %s: Illegal connection type.", remoteAddr);

						SetWindowText(textWindow, chatBuff);
						SendMessage(textWindow, EM_SETSEL, GetWindowTextLength(textWindow), GetWindowTextLength(textWindow));
						SendMessage(textWindow, EM_SCROLLCARET, 0, 0);
						listening = FALSE;
						canMove = FALSE;
						curPlayer = PLAYER_ONE;
						gameInProgress = FALSE;
						curLine = -1;
						EnableMenuItem(hMenu, IDM_FILE_OPTIONS, MF_ENABLED);
						players[0].type = HOTSEAT;
						players[1].type = CPU;
						expectedMsg = 0;
						destroy_socket(&sock);
						destroy_socket(&serverSock);
						WSACleanup();

						break;

					}

					SendMessage(hWnd, GAME_MADE_MOVE, atoi(strtok(NULL, ":")), 0);

					break;

				case MSG_CHAT:

					if (!connectionValid) {
	
						if (GetWindowTextLength(textWindow) + strlen(remoteAddr) + 47 >= 1024) {

							for (i = 0; i < GetWindowTextLength(textWindow); i++) {

								chatBuff[i] = chatBuff[i+strlen(remoteAddr)+47];

							}

						}
				
						if (GetWindowTextLength(textWindow) > 0)
							wsprintf(chatBuff, "%s\r\nDisconnected from: %s: Illegal connection type.", chatBuff, remoteAddr);
						else
							wsprintf(chatBuff, "Disconnected from: %s: Illegal connection type.", remoteAddr);

						SetWindowText(textWindow, chatBuff);
						SendMessage(textWindow, EM_SETSEL, GetWindowTextLength(textWindow), GetWindowTextLength(textWindow));
						SendMessage(textWindow, EM_SCROLLCARET, 0, 0);
						listening = FALSE;
						canMove = FALSE;
						curPlayer = PLAYER_ONE;
						gameInProgress = FALSE;
						curLine = -1;
						EnableMenuItem(hMenu, IDM_FILE_OPTIONS, MF_ENABLED);
						players[0].type = HOTSEAT;
						players[1].type = CPU;
						expectedMsg = 0;
						destroy_socket(&sock);
						destroy_socket(&serverSock);
						WSACleanup();

						break;

					}

					wsprintf(tmpBuff, "%s", strtok(NULL, ":"));

					if (GetWindowTextLength(textWindow) + strlen(tmpBuff) >= 1024) {

						for (i = 0; i < GetWindowTextLength(textWindow); i++) {

							chatBuff[i] = chatBuff[i+strlen(tmpBuff)+3];

						}

					}
				
					if (GetWindowTextLength(textWindow) > 0)
						wsprintf(chatBuff, "%s\r\n-%s", chatBuff, tmpBuff);
					else
						wsprintf(chatBuff, "-%s", tmpBuff);

					SetWindowText(textWindow, chatBuff);
					SendMessage(textWindow, EM_SETSEL, GetWindowTextLength(textWindow), GetWindowTextLength(textWindow));
					SendMessage(textWindow, EM_SCROLLCARET, 0, 0);

					break;

				default:

					if (GetWindowTextLength(textWindow) + strlen(remoteAddr) + 47 >= 1024) {

						for (i = 0; i < GetWindowTextLength(textWindow); i++) {

							chatBuff[i] = chatBuff[i+strlen(remoteAddr)+47];

						}

					}
				
					if (GetWindowTextLength(textWindow) > 0)
						wsprintf(chatBuff, "%s\r\nDisconnected from: %s: Illegal connection type.", chatBuff, remoteAddr);
					else
						wsprintf(chatBuff, "Disconnected from: %s: Illegal connection type.", remoteAddr);

					SetWindowText(textWindow, chatBuff);
					SendMessage(textWindow, EM_SETSEL, GetWindowTextLength(textWindow), GetWindowTextLength(textWindow));
					SendMessage(textWindow, EM_SCROLLCARET, 0, 0);
					listening = FALSE;
					canMove = FALSE;
					curPlayer = PLAYER_ONE;
					gameInProgress = FALSE;
					curLine = -1;
					EnableMenuItem(hMenu, IDM_FILE_OPTIONS, MF_ENABLED);
					players[0].type = HOTSEAT;
					players[1].type = CPU;
					expectedMsg = 0;
					destroy_socket(&sock);
					destroy_socket(&serverSock);
					WSACleanup();

					break;

				}

				break;

			}

			else if (connecting) {

				break;

			}

			else
				break;

		case FD_CLOSE:

			if (GetWindowTextLength(textWindow) + strlen(remoteAddr) + 50 >= 1024) {

				for (i = 0; i < GetWindowTextLength(textWindow); i++) {

					chatBuff[i] = chatBuff[i+strlen(remoteAddr)+50];

				}

			}
				
			if (GetWindowTextLength(textWindow) > 0)
				wsprintf(chatBuff, "%s\r\nDisconnected from: %s: Connection closed remotely.", chatBuff, remoteAddr);
			else
				wsprintf(chatBuff, "Disconnected from: %s: Connection closed remotely.", remoteAddr);

			SetWindowText(textWindow, chatBuff);
			SendMessage(textWindow, EM_SETSEL, GetWindowTextLength(textWindow), GetWindowTextLength(textWindow));
			SendMessage(textWindow, EM_SCROLLCARET, 0, 0);
			connectionValid = FALSE;
			listening = FALSE;
			connecting = FALSE;
			canMove = FALSE;
			curPlayer = PLAYER_ONE;
			gameInProgress = FALSE;
			curLine = -1;
			EnableMenuItem(hMenu, IDM_FILE_OPTIONS, MF_ENABLED);
			players[0].type = HOTSEAT;
			players[1].type = CPU;
			expectedMsg = 0;
			destroy_socket(&sock);
			destroy_socket(&serverSock);
			WSACleanup();

			break;

		default:

			break;

		}

    default: 
        lRet = DefWindowProc (hWnd, uMsg, wParam, lParam); 
        break; 
    } 
 
    return lRet; 
} 

LRESULT CALLBACK glWinProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	LONG    lRet = 1; 
    PAINTSTRUCT    ps;
	RECT rect;
	int tmp;
	static BOOL rmb = FALSE;
	static int oldMouseX, oldMouseY, deltaX, deltaY;

	switch (uMsg) { 
	case WM_CREATE: 
        ghDC = GetDC(hWnd); 
        if (!bSetupPixelFormat(ghDC)) 
            PostQuitMessage (0); 
 
        ghRC = wglCreateContext(ghDC); 
        wglMakeCurrent(ghDC, ghRC); 
        GetClientRect(hWnd, &rect); 
        initializeGL(rect.right, rect.bottom);
        break; 
 
    case WM_PAINT: 
        BeginPaint(hWnd, &ps); 
		drawScene(GL_RENDER);
        EndPaint(hWnd, &ps); 
        break; 
 
    case WM_SIZE: 
        GetClientRect(hWnd, &rect); 
        resize(rect.right, rect.bottom); 
        break; 
 
    case WM_CLOSE: 
        if (ghRC) 
            wglDeleteContext(ghRC); 
        if (ghDC) 
            ReleaseDC(hWnd, ghDC); 
        ghRC = 0; 
        ghDC = 0; 
 
        DestroyWindow (hWnd); 
        break; 
 
    case WM_DESTROY: 
        if (ghRC) 
            wglDeleteContext(ghRC); 
        if (ghDC) 
            ReleaseDC(hWnd, ghDC);  
        break; 

	case WM_CHAR:
		switch ((TCHAR)wParam) {
		case 'C':
		case 'c':
			curCam = (curCam == OUTSIDE_CAM) ? CENTER_CAM : OUTSIDE_CAM;
			break;
		default:
			break;
		}
		break;
     
/*    case WM_KEYDOWN: 
        switch (wParam) { 
        case VK_LEFT: 
            xTrans -= 0.5F; 
            break; 
        case VK_RIGHT: 
            xTrans += 0.5F; 
            break; 
        case VK_UP: 
            yTrans += 0.5F; 
            break; 
        case VK_DOWN: 
            yTrans -= 0.5F; 
            break; 
        } 
*/
	case WM_LBUTTONDOWN:
		if (!canMove)
			break;
		if (!gameInProgress)
			break;
		if (rmb)
			break;
		tmp = Selection();
		
		if (tmp != -1) {

			SendMessage(ghWnd, GAME_MADE_MOVE, (WPARAM)tmp, 0);

		}

		break;
	case WM_RBUTTONDOWN:
		if (!gameInProgress)
			break;
		rmb = TRUE;
		oldMouseX = LOWORD(lParam);
		oldMouseY = HIWORD(lParam);
		break;

	case WM_RBUTTONUP:
		if (!gameInProgress)
			break;
		rmb = FALSE;
		break;

	case WM_MOUSEMOVE:
		if (!gameInProgress)
			break;
		mouse_x = LOWORD(lParam);
		mouse_y = HIWORD(lParam);

		if (rmb) {
			deltaX = LOWORD(lParam) - oldMouseX;
			deltaY = HIWORD(lParam) - oldMouseY;
			oldMouseX = LOWORD(lParam);
			oldMouseY = HIWORD(lParam);
			update_camera(&cams[curCam], cams[curCam].xRot - deltaY / (((float)HEIGHT - 195) / 360), cams[curCam].yRot + deltaX / ((float)WIDTH / 360), cams[curCam].xTrans, cams[curCam].yTrans, cams[curCam].zTrans);
		}

		else {
			curLine = Selection();
		}

		break;
 
    default: 
        lRet = DefWindowProc (hWnd, uMsg, wParam, lParam); 
        break; 
    } 
 
    return lRet; 
}

LRESULT CALLBACK editWinProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	switch (uMsg) {
	case WM_CHAR:

		if (wParam == '\r')
			return 0;
		break;

	case WM_KEYDOWN:
		
		switch(wParam) {
		case VK_RETURN:
			SendMessage(GetParent(hWnd), WM_COMMAND, IDB_SENDBUTTON, 0);
			return 0;
		case VK_F1:
			SendMessage(GetParent(hWnd), WM_COMMAND, IDM_FILE_OPTIONS, 0);
			return 0;;
		case VK_F2:
			SendMessage(GetParent(hWnd), WM_COMMAND, IDM_FILE_NEW, 0);
			return 0;
		default:
			break;

		}

	}

	return CallWindowProc(defEditProc, hWnd, uMsg, wParam, lParam);

}

BOOL CALLBACK optionsDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    
    static HWND xScroll, yScroll, zScroll;
	static HWND xTxt, yTxt, zTxt;
	static HWND p1Human, p2Human, p1Easy, p2Easy, p1Medium, p2Medium, p1Hard, p2Hard;
	static int scrollPos[3];
	int i;
	
	switch (uMsg) {
    case WM_INITDIALOG:
		xScroll = GetDlgItem(hDlg, IDC_SCROLLBAR1);
		yScroll = GetDlgItem(hDlg, IDC_SCROLLBAR2);
		zScroll = GetDlgItem(hDlg, IDC_SCROLLBAR3);
		xTxt = GetDlgItem(hDlg, IDC_BOARD_X);
		yTxt = GetDlgItem(hDlg, IDC_BOARD_Y);
		zTxt = GetDlgItem(hDlg, IDC_BOARD_Z);
		p1Human = GetDlgItem(hDlg, IDC_PLAYER_ONE_HUMAN);
		p2Human = GetDlgItem(hDlg, IDC_PLAYER_TWO_HUMAN);
		p1Easy = GetDlgItem(hDlg, IDC_PLAYER_ONE_CPU_EASY);
		p2Easy = GetDlgItem(hDlg, IDC_PLAYER_TWO_CPU_EASY);
		p1Medium = GetDlgItem(hDlg, IDC_PLAYER_ONE_CPU_MED);
		p2Medium = GetDlgItem(hDlg, IDC_PLAYER_TWO_CPU_MED);
		p1Hard = GetDlgItem(hDlg, IDC_PLAYER_ONE_CPU_HARD);
		p2Hard = GetDlgItem(hDlg, IDC_PLAYER_TWO_CPU_HARD);

		if (connecting || listening)
			EnableWindow(GetDlgItem(hDlg, IDC_TCP), FALSE);

		if (connecting) {

			EnableWindow(p1Human, FALSE);
			EnableWindow(p1Easy, FALSE);
			EnableWindow(p1Medium, FALSE);
			EnableWindow(p1Hard, FALSE);

		}

		if (listening) {

			EnableWindow(p2Human, FALSE);
			EnableWindow(p2Easy, FALSE);
			EnableWindow(p2Medium, FALSE);
			EnableWindow(p2Hard, FALSE);

		}

		wsprintf(buff, "Cubes in X-axis: %d", board.x);
		SetWindowText(xTxt, buff);
		SetScrollRange(xScroll, SB_CTL, 1, MAX_X, FALSE);
		SetScrollPos(xScroll, SB_CTL, board.x, TRUE);
		scrollPos[0] = board.x;
		wsprintf(buff, "Cubes in Y-axis: %d", board.y);
		SetWindowText(yTxt, buff);
		SetScrollRange(yScroll, SB_CTL, 1, MAX_Y, FALSE);
		SetScrollPos(yScroll, SB_CTL, board.y, TRUE);
		scrollPos[1] = board.y;
		wsprintf(buff, "Cubes in Z-axis: %d", board.z);
		SetWindowText(zTxt, buff);
		SetScrollRange(zScroll, SB_CTL, 1, MAX_Z, FALSE);
		SetScrollPos(zScroll, SB_CTL, board.z, TRUE);
		scrollPos[2] = board.z;
		
		switch (players[0].type) {
		case HOTSEAT:
			SendMessage(p1Human, BM_SETCHECK, TRUE, 0);
			break;
		case CPU:
			switch(params.diff[0]) {
			case DIFFICULTY_EASY:
				SendMessage(p1Easy, BM_SETCHECK, TRUE, 0);
				break;
			case DIFFICULTY_MEDIUM:
				SendMessage(p1Medium, BM_SETCHECK, TRUE, 0);
				break;
			case DIFFICULTY_HARD:
				SendMessage(p1Hard, BM_SETCHECK, TRUE, 0);
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}

		switch (players[1].type) {
		case HOTSEAT:
			SendMessage(p2Human, BM_SETCHECK, TRUE, 0);
			break;
		case CPU:
			switch(params.diff[1]) {
			case DIFFICULTY_EASY:
				SendMessage(p2Easy, BM_SETCHECK, TRUE, 0);
				break;
			case DIFFICULTY_MEDIUM:
				SendMessage(p2Medium, BM_SETCHECK, TRUE, 0);
				break;
			case DIFFICULTY_HARD:
				SendMessage(p2Hard, BM_SETCHECK, TRUE, 0);
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}

        return TRUE;

    case WM_COMMAND:
        switch(LOWORD(wParam)) {
			case IDOK:
				init_board(&board, scrollPos[0], scrollPos[1], scrollPos[2]);
				
				if ((BOOL)SendMessage(p1Human, BM_GETCHECK, 0, 0) == TRUE) {
					players[0].type = HOTSEAT;
					params.diff[0] = DIFFICULTY_HARD;
				}

				if ((BOOL)SendMessage(p1Easy, BM_GETCHECK, 0, 0) == TRUE) {
					players[0].type = CPU;
					params.diff[0] = DIFFICULTY_EASY;
				}

				if ((BOOL)SendMessage(p1Medium, BM_GETCHECK, 0, 0) == TRUE) {
					players[0].type = CPU;
					params.diff[0] = DIFFICULTY_MEDIUM;
				}

				if ((BOOL)SendMessage(p1Hard, BM_GETCHECK, 0, 0) == TRUE) {
					players[0].type = CPU;
					params.diff[0] = DIFFICULTY_HARD;
				}

				if ((BOOL)SendMessage(p2Human, BM_GETCHECK, 0, 0) == TRUE) {
					players[1].type = HOTSEAT;
					params.diff[1] = DIFFICULTY_HARD;
				}

				if ((BOOL)SendMessage(p2Easy, BM_GETCHECK, 0, 0) == TRUE) {
					players[1].type = CPU;
					params.diff[1] = DIFFICULTY_EASY;
				}

				if ((BOOL)SendMessage(p2Medium, BM_GETCHECK, 0, 0) == TRUE) {
					players[1].type = CPU;
					params.diff[1] = DIFFICULTY_MEDIUM;
				}

				if ((BOOL)SendMessage(p2Hard, BM_GETCHECK, 0, 0) == TRUE) {
					players[1].type = CPU;
					params.diff[1] = DIFFICULTY_HARD;
				}

				players[0].score = 0;
				players[1].score = 0;
				cube_width = 6 / (float)board.x;
				cube_height = 3 / (float)board.y;
				cube_depth = 4 / (float)board.z;
                EndDialog(hDlg, FALSE);
				return FALSE;
            case IDCANCEL:
                EndDialog(hDlg, FALSE);
				return FALSE;
			case IDC_TCP:

				if (connecting || listening)
					return FALSE;

				init_board(&board, scrollPos[0], scrollPos[1], scrollPos[2]);
				
				if ((BOOL)SendMessage(p1Human, BM_GETCHECK, 0, 0) == TRUE) {
					players[0].type = HOTSEAT;
					params.diff[0] = DIFFICULTY_HARD;
				}

				if ((BOOL)SendMessage(p1Easy, BM_GETCHECK, 0, 0) == TRUE) {
					players[0].type = CPU;
					params.diff[0] = DIFFICULTY_EASY;
				}

				if ((BOOL)SendMessage(p1Medium, BM_GETCHECK, 0, 0) == TRUE) {
					players[0].type = CPU;
					params.diff[0] = DIFFICULTY_MEDIUM;
				}

				if ((BOOL)SendMessage(p1Hard, BM_GETCHECK, 0, 0) == TRUE) {
					players[0].type = CPU;
					params.diff[0] = DIFFICULTY_HARD;
				}
				if ((BOOL)SendMessage(p2Human, BM_GETCHECK, 0, 0) == TRUE) {
					players[1].type = HOTSEAT;
					params.diff[1] = DIFFICULTY_HARD;
				}

				if ((BOOL)SendMessage(p2Easy, BM_GETCHECK, 0, 0) == TRUE) {
					players[1].type = CPU;
					params.diff[1] = DIFFICULTY_EASY;
				}

				if ((BOOL)SendMessage(p2Medium, BM_GETCHECK, 0, 0) == TRUE) {
					players[1].type = CPU;
					params.diff[1] = DIFFICULTY_MEDIUM;
				}

				if ((BOOL)SendMessage(p2Hard, BM_GETCHECK, 0, 0) == TRUE) {
					players[1].type = CPU;
					params.diff[1] = DIFFICULTY_HARD;
				}

				players[0].score = 0;
				players[1].score = 0;
				cube_width = 6 / (float)board.x;
				cube_height = 3 / (float)board.y;
				cube_depth = 4 / (float)board.z;
				EndDialog(hDlg, TRUE);
				return FALSE;
            default:
                return FALSE;
        }
		break;

	case WM_HSCROLL:
		i = GetWindowLong((HWND)lParam, GWL_ID);

		switch(i) {
		case IDC_SCROLLBAR1:
			
			switch(LOWORD(wParam)) {
			case SB_LINELEFT:
			case SB_PAGELEFT:
				if (scrollPos[0] == 1)
					return FALSE;

				wsprintf(buff, "Cubes in X-axis: %d", scrollPos[0] - 1);
				SetWindowText(xTxt, buff);
				SetScrollPos(xScroll, SB_CTL, scrollPos[0] - 1, TRUE);
				scrollPos[0]--;

				return TRUE;

			case SB_LINERIGHT:
			case SB_PAGERIGHT:
				if (scrollPos[0] == MAX_X)
					return FALSE;

				wsprintf(buff, "Cubes in X-axis: %d", scrollPos[0] + 1);
				SetWindowText(xTxt, buff);
				SetScrollPos(xScroll, SB_CTL, scrollPos[0] + 1, TRUE);
				scrollPos[0]++;

				return TRUE;

			case SB_THUMBPOSITION:
			case SB_THUMBTRACK:
				wsprintf(buff, "Cubes in X-axis: %d", HIWORD(wParam));
				SetWindowText(xTxt, buff);
				SetScrollPos(xScroll, SB_CTL, HIWORD(wParam), TRUE);
				scrollPos[0] = HIWORD(wParam);

				return TRUE;

			default:

				return FALSE;

			}

		case IDC_SCROLLBAR2:
			
			switch(LOWORD(wParam)) {
			case SB_LINELEFT:
			case SB_PAGELEFT:
				if (scrollPos[1] == 1)
					return FALSE;

				wsprintf(buff, "Cubes in Y-axis: %d", scrollPos[1] - 1);
				SetWindowText(yTxt, buff);
				SetScrollPos(yScroll, SB_CTL, scrollPos[1] - 1, TRUE);
				scrollPos[1]--;

				return TRUE;

			case SB_LINERIGHT:
			case SB_PAGERIGHT:
				if (scrollPos[1] == MAX_Y)
					return FALSE;

				wsprintf(buff, "Cubes in Y-axis: %d", scrollPos[1] + 1);
				SetWindowText(yTxt, buff);
				SetScrollPos(yScroll, SB_CTL, scrollPos[1] + 1, TRUE);
				scrollPos[1]++;

				return TRUE;

			case SB_THUMBPOSITION:
			case SB_THUMBTRACK:
				wsprintf(buff, "Cubes in Y-axis: %d", HIWORD(wParam));
				SetWindowText(yTxt, buff);
				SetScrollPos(yScroll, SB_CTL, HIWORD(wParam), TRUE);
				scrollPos[1] = HIWORD(wParam);

				return TRUE;

			default:

				return FALSE;

			}

		case IDC_SCROLLBAR3:
			
			switch(LOWORD(wParam)) {
			case SB_LINELEFT:
			case SB_PAGELEFT:
				if (scrollPos[2] == 1)
					return FALSE;

				wsprintf(buff, "Cubes in Z-axis: %d", scrollPos[2] - 1);
				SetWindowText(zTxt, buff);
				SetScrollPos(zScroll, SB_CTL, scrollPos[2] - 1, TRUE);
				scrollPos[2]--;

				return TRUE;

			case SB_LINERIGHT:
			case SB_PAGERIGHT:
				if (scrollPos[2] == MAX_Z)
					return FALSE;

				wsprintf(buff, "Cubes in Z-axis: %d", scrollPos[2] + 1);
				SetWindowText(zTxt, buff);
				SetScrollPos(zScroll, SB_CTL, scrollPos[2] + 1, TRUE);
				scrollPos[2]++;

				return TRUE;

			case SB_THUMBPOSITION:
			case SB_THUMBTRACK:
				wsprintf(buff, "Cubes in Z-axis: %d", HIWORD(wParam));
				SetWindowText(zTxt, buff);
				SetScrollPos(zScroll, SB_CTL, HIWORD(wParam), TRUE);
				scrollPos[2] = HIWORD(wParam);

				return TRUE;

			default:

				return FALSE;

			}

		default:

			return FALSE;

		}

    default:
        return FALSE;

	}

}

BOOL CALLBACK internetDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	static HWND editLocalPort, editRemotePort, editRemoteIP, staticStatus;
	
	switch (uMsg) {
	case WM_INITDIALOG:

		internetDlg = hDlg;
		editLocalPort = GetDlgItem(hDlg, IDC_LOCAL_PORT);
		editRemotePort = GetDlgItem(hDlg, IDC_REMOTE_PORT);
		editRemoteIP = GetDlgItem(hDlg, IDC_IP_ADDRESS);
		staticStatus = GetDlgItem(hDlg, IDC_STATUS);
		SetWindowText(editLocalPort, "12345");
		SetWindowText(editRemotePort, "12345");
		SetWindowText(editRemoteIP, "127.0.0.1");
		SetWindowText(staticStatus, "If you are hosting the game, type in the port number you wish to use and hit Start. If you are connecting to a host, type in the IP address of the host and the port to connect to and hit Connect.");

		return TRUE;

	case WM_COMMAND:

        switch(LOWORD(wParam)) {
		case IDC_LISTEN:

			if (!listening) {

				GetWindowText(editLocalPort, buff, GetWindowTextLength(editLocalPort) + 1);

				switch(init_socket(&WSAData, &serverSock)) {
				case ERR_WSASTARTUP:
					wsprintf(buff, "Error initializing Winsock!\r\nError: %d", WSAGetLastError());
					MessageBox(hDlg, buff, "Error", MB_OK|MB_ICONHAND);
					return FALSE;
				case ERR_SOCK_CREATE:
					wsprintf(buff, "Error creating socket!\r\nError: %d", WSAGetLastError());
					MessageBox(hDlg, buff, "Error", MB_OK|MB_ICONHAND);
					return FALSE;
				case 0:

					switch (init_server(&serverSock, &sa, ghWnd, (unsigned short)atoi(buff))) {
					case ERR_SOCK_SELECT:
						wsprintf(buff, "Error initializing socket!\r\nError: %d", WSAGetLastError());
						MessageBox(hDlg, buff, "Error", MB_OK|MB_ICONHAND);
						return FALSE;
					case ERR_SOCK_BIND:
						wsprintf(buff, "Error initializing socket!\r\nError: %d", WSAGetLastError());
						MessageBox(hDlg, buff, "Error", MB_OK|MB_ICONHAND);
						return FALSE;
					case ERR_SOCK_LISTEN:
						wsprintf(buff, "Error initializing socket!\r\nError: %d", WSAGetLastError());
						MessageBox(hDlg, buff, "Error", MB_OK|MB_ICONHAND);
						return FALSE;
					case 0:
						SetWindowText(staticStatus, "Waiting for incoming connection... (Press Stop to stop waiting.)");
						SetWindowText(GetDlgItem(hDlg, IDC_LISTEN), "Stop");
						EnableWindow(GetDlgItem(hDlg, IDC_CONNECT), FALSE);
						listening = TRUE;
					default:
						return FALSE;
					}

				default:
					return FALSE;

				}

			}
			else {
				
				destroy_socket(&serverSock);
				SetWindowText(staticStatus, "If you are hosting the game, type in the port number you wish to use and hit Start. If you are connecting to a host, type in the IP address of the host and the port to connect to and hit Connect.");
				SetWindowText(GetDlgItem(hDlg, IDC_LISTEN), "Start");
				EnableWindow(GetDlgItem(hDlg, IDC_CONNECT), TRUE);
				listening = FALSE;
				return FALSE;

			}

		case IDC_CONNECT:

			if (!connecting) {

				GetWindowText(editRemoteIP, buff3, GetWindowTextLength(editRemoteIP) + 1);
				GetWindowText(editRemotePort, buff, GetWindowTextLength(editRemotePort) + 1);

				switch(init_socket(&WSAData, &sock)) {
				case ERR_WSASTARTUP:
					wsprintf(buff, "Error initializing Winsock!\r\nError: %d", WSAGetLastError());
					MessageBox(hDlg, buff, "Error", MB_OK|MB_ICONHAND);
					return FALSE;
				case ERR_SOCK_CREATE:
					wsprintf(buff, "Error creating socket!\r\nError: %d", WSAGetLastError());
					MessageBox(hDlg, buff, "Error", MB_OK|MB_ICONHAND);
					return FALSE;
				case 0:

					switch (init_client(&sock, &sa, ghWnd, buff3, (unsigned short)atoi(buff))) {
					case ERR_SOCK_SELECT:
						wsprintf(buff, "Error initializing socket!\r\nError: %d", WSAGetLastError());
						MessageBox(hDlg, buff, "Error", MB_OK|MB_ICONHAND);
						return FALSE;
					case ERR_SOCK_CONNECT:
						wsprintf(buff, "Error initializing socket!\r\nError: %d", WSAGetLastError());
						MessageBox(hDlg, buff, "Error", MB_OK|MB_ICONHAND);
						return FALSE;
					case 0:
						SetWindowText(staticStatus, "Connecting... (Press Stop to stop waiting.)");
						SetWindowText(GetDlgItem(hDlg, IDC_CONNECT), "Stop");
						EnableWindow(GetDlgItem(hDlg, IDC_LISTEN), FALSE);
						connecting = TRUE;
					default:
						return FALSE;
					}

				default:
					return FALSE;

				}

			}
			else {
				
				destroy_socket(&sock);
				SetWindowText(staticStatus, "If you are hosting the game, type in the port number you wish to use and hit Start. If you are connecting to a host, type in the IP address of the host and the port to connect to and hit Connect.");
				SetWindowText(GetDlgItem(hDlg, IDC_CONNECT), "Connect");
				EnableWindow(GetDlgItem(hDlg, IDC_LISTEN), TRUE);
				connecting = FALSE;
				return FALSE;

			}

		case IDCANCEL:
			listening = FALSE;
			connecting = FALSE;
			destroy_socket(&sock);
			destroy_socket(&serverSock);
			EndDialog(hDlg, FALSE);
			return FALSE;
		default:
			return FALSE;

		}

	default:

		return FALSE;

	}

}

BOOL bSetupPixelFormat(HDC hdc) 
{ 
    PIXELFORMATDESCRIPTOR pfd, *ppfd; 
    int pixelformat; 
 
    ppfd = &pfd; 
 
    ppfd->nSize = sizeof(PIXELFORMATDESCRIPTOR); 
    ppfd->nVersion = 1; 
    ppfd->dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL |  
                        PFD_DOUBLEBUFFER; 
    ppfd->dwLayerMask = PFD_MAIN_PLANE; 
    ppfd->iPixelType = PFD_TYPE_COLORINDEX; 
    ppfd->cColorBits = 8; 
    ppfd->cDepthBits = 16; 
    ppfd->cAccumBits = 0; 
    ppfd->cStencilBits = 0; 
 
    pixelformat = ChoosePixelFormat(hdc, ppfd); 
 
    if ( (pixelformat = ChoosePixelFormat(hdc, ppfd)) == 0 ) 
    { 
        MessageBox(NULL, "ChoosePixelFormat failed", "Error", MB_OK); 
        return FALSE; 
    } 
 
    if (SetPixelFormat(hdc, pixelformat, ppfd) == FALSE) 
    { 
        MessageBox(NULL, "SetPixelFormat failed", "Error", MB_OK); 
        return FALSE; 
    } 
 
    return TRUE; 
} 
 
GLvoid resize( GLsizei width, GLsizei height ) 
{ 
    GLfloat aspect; 
 
    glViewport( 0, 0, width, height ); 
 
    aspect = (GLfloat) width / height; 
 
    glMatrixMode( GL_PROJECTION ); 
    glLoadIdentity(); 
    gluPerspective( 45.0, aspect, 0.1, 12.0 ); 
    glMatrixMode( GL_MODELVIEW ); 

	//update_camera(&cams[OUTSIDE_CAM], cams[OUTSIDE_CAM].xRot - deltaY / (((float)HEIGHT - 195) / 360), cams[OUTSIDE_CAM].yRot + deltaX / ((float)WIDTH / 360), cams[OUTSIDE_CAM].xTrans, cams[curCam].yTrans, cams[curCam].zTrans);
	//update_camera(&cams[CENTER_CAM], cams[curCam].xRot - deltaY / (((float)HEIGHT - 195) / 360), cams[curCam].yRot + deltaX / ((float)WIDTH / 360), cams[curCam].xTrans, cams[curCam].yTrans, cams[curCam].zTrans);
	cube_width = 6 / (float)board.x;
	cube_height = 3 / (float)board.y;
	cube_depth = 4 / (float)board.z;
}

GLvoid drawSquares(int cube)
{
	
	int i;
	
	if (board.cubes[cube].filled != NOT_FILLED)	{

		if (board.cubes[cube].filled == PLAYER_ONE) {
		
			glColor3f(1.0f, 0.0f, 0.0f);

		}
		else if (board.cubes[cube].filled == PLAYER_TWO) {

			glColor3f(0.0f, 0.0f, 1.0f);

		}
		else if (board.cubes[cube].filled == NO_OWNER) {

			glColor3f(1.0f, 1.0f, 1.0f);

		}
		else {

			return;

		}

		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(0.0, 1.0);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		glBegin(GL_QUADS);

			glVertex3f(-cube_width / 2, -cube_depth / 2, -cube_height / 2);
			glVertex3f(-cube_width / 2, -cube_depth / 2, cube_height / 2);
			glVertex3f(cube_width / 2, -cube_depth / 2, cube_height / 2);
			glVertex3f(cube_width / 2, -cube_depth / 2, -cube_height / 2);

			glVertex3f(-cube_width / 2, -cube_depth / 2, -cube_height / 2);
			glVertex3f(-cube_width / 2, -cube_depth / 2, cube_height / 2);
			glVertex3f(-cube_width / 2, cube_depth / 2, cube_height / 2);
			glVertex3f(-cube_width / 2, cube_depth / 2, -cube_height / 2);

			glVertex3f(cube_width / 2, -cube_depth / 2, cube_height / 2);
			glVertex3f(cube_width / 2, -cube_depth / 2, -cube_height / 2);
			glVertex3f(cube_width / 2, cube_depth / 2, -cube_height / 2);
			glVertex3f(cube_width / 2, cube_depth / 2, cube_height / 2);

			glVertex3f(-cube_width / 2, cube_depth / 2, -cube_height / 2);
			glVertex3f(-cube_width / 2, cube_depth / 2, cube_height / 2);
			glVertex3f(cube_width / 2, cube_depth / 2, cube_height / 2);
			glVertex3f(cube_width / 2, cube_depth / 2, -cube_height / 2);

			glVertex3f(-cube_width / 2, -cube_depth / 2, -cube_height / 2);
			glVertex3f(-cube_width / 2, cube_depth / 2, -cube_height / 2);
			glVertex3f(cube_width / 2, cube_depth / 2, -cube_height / 2);
			glVertex3f(cube_width / 2, -cube_depth / 2, -cube_height / 2);

			glVertex3f(-cube_width / 2, -cube_depth / 2, cube_height / 2);
			glVertex3f(-cube_width / 2, cube_depth / 2, cube_height / 2);
			glVertex3f(cube_width / 2, cube_depth / 2, cube_height / 2);
			glVertex3f(cube_width / 2, -cube_depth / 2, cube_height / 2);

		glEnd();

	}

	glDisable(GL_POLYGON_OFFSET_FILL);

	for (i = 0; i < 6; i++) {
	
		if (board.squares[board.cubes[cube].squares[i]].filled == NOT_FILLED)
			continue;

		else if (board.squares[board.cubes[cube].squares[i]].filled == PLAYER_ONE) {

			glColor3f(1.0f, 0.0f, 0.0f);
			glLineWidth(3);

		}

		else if (board.squares[board.cubes[cube].squares[i]].filled == PLAYER_TWO) {

			glColor3f(0.0f, 0.0f, 1.0f);
			glLineWidth(3);

		}

		else
			continue;

		glBegin(GL_LINES);

		switch (i) {
		case 0:

			glVertex3f(-cube_width / 2, -cube_depth / 2, -cube_height / 2);
			glVertex3f(cube_width / 2, -cube_depth / 2, cube_height / 2);

			glVertex3f(-cube_width / 2, -cube_depth / 2, cube_height / 2);
			glVertex3f(cube_width / 2, -cube_depth / 2, -cube_height / 2);
			break;

		case 1:
			glVertex3f(-cube_width / 2, cube_depth / 2, -cube_height / 2);
			glVertex3f(cube_width / 2, cube_depth / 2, cube_height / 2);

			glVertex3f(-cube_width / 2, cube_depth / 2, cube_height / 2);
			glVertex3f(cube_width / 2, cube_depth / 2, -cube_height / 2);
			break;

		case 2:
			glVertex3f(-cube_width / 2, -cube_depth / 2, -cube_height / 2);
			glVertex3f(cube_width / 2, cube_depth / 2, -cube_height / 2);

			glVertex3f(-cube_width / 2, cube_depth / 2, -cube_height / 2);
			glVertex3f(cube_width / 2, -cube_depth / 2, -cube_height / 2);
			break;
			
		case 3:
			glVertex3f(-cube_width / 2, -cube_depth / 2, cube_height / 2);
			glVertex3f(cube_width / 2, cube_depth / 2, cube_height / 2);

			glVertex3f(-cube_width / 2, cube_depth / 2, cube_height / 2);
			glVertex3f(cube_width / 2, -cube_depth / 2, cube_height / 2);
			break;

		case 4:
			glVertex3f(-cube_width / 2, -cube_depth / 2, -cube_height / 2);
			glVertex3f(-cube_width / 2, cube_depth / 2, cube_height / 2);

			glVertex3f(-cube_width / 2, -cube_depth / 2, cube_height / 2);
			glVertex3f(-cube_width / 2, cube_depth / 2, -cube_height / 2);
			break;

		case 5:
			glVertex3f(cube_width / 2, -cube_depth / 2, -cube_height / 2);
			glVertex3f(cube_width / 2, cube_depth / 2, cube_height / 2);

			glVertex3f(cube_width / 2, -cube_depth / 2, cube_height / 2);
			glVertex3f(cube_width / 2, cube_depth / 2, -cube_height / 2);
			break;

		default:
			break;

		}

		glEnd();

	}

}

GLvoid drawLines(int cube) 
{
		
	int i;

	for (i = 0; i < 12; i++) {
	
		glLoadName(board.cubes[cube].lines[i]);
		glPushMatrix();
		
		if (board.lines[board.cubes[cube].lines[i]].filled != NOT_FILLED)
		{
			glLineWidth(3);
			glColor3f(0.0f, 1.0f, 0.0f);
		}

		else if (curLine == board.cubes[cube].lines[i])
		{
			glLineWidth(3);
			glColor3f(1.0f, 1.0f, 0.0f);
		}
	
		else {
			glLineWidth(2);
			glColor3f(0.5f, 0.5f, 0.5f);
		}
		if (board.lines[board.cubes[cube].lines[i]].newestLine == TRUE) {
			glLineWidth(3);
			glColor3f(1.0f, 1.0f, 1.0f);
		}
		glBegin(GL_LINES);

			switch(i) {
			case 0:
				glVertex3f(-cube_width / 2, -cube_depth / 2, -cube_height / 2);
				glVertex3f(-cube_width / 2, -cube_depth / 2, cube_height / 2);
				break;
			case 1:
				glVertex3f(cube_width / 2, -cube_depth / 2, -cube_height / 2);
				glVertex3f(cube_width / 2, -cube_depth / 2, cube_height / 2);
				break;
			case 2:
				glVertex3f(-cube_width / 2, cube_depth / 2, -cube_height / 2);
				glVertex3f(-cube_width / 2, cube_depth / 2, cube_height / 2);
				break;
			case 3:
				glVertex3f(cube_width / 2, cube_depth / 2, -cube_height / 2);
				glVertex3f(cube_width / 2, cube_depth / 2, cube_height / 2);
				break;
			case 4:
				glVertex3f(-cube_width / 2, -cube_depth / 2, -cube_height / 2);
				glVertex3f(-cube_width / 2, cube_depth / 2, -cube_height / 2);
				break;
			case 5:
				glVertex3f(-cube_width / 2, -cube_depth / 2, cube_height / 2);
				glVertex3f(-cube_width / 2, cube_depth / 2, cube_height / 2);
				break;
			case 6:
				glVertex3f(cube_width / 2, -cube_depth / 2, -cube_height / 2);
				glVertex3f(cube_width / 2, cube_depth / 2, -cube_height / 2);
				break;
			case 7:
				glVertex3f(cube_width / 2, -cube_depth / 2, cube_height / 2);
				glVertex3f(cube_width / 2, cube_depth / 2, cube_height / 2);
				break;
			case 8:
				glVertex3f(-cube_width / 2, -cube_depth / 2, -cube_height / 2);
				glVertex3f(cube_width / 2, -cube_depth / 2, -cube_height / 2);
				break;
			case 9:
				glVertex3f(-cube_width / 2, -cube_depth / 2, cube_height / 2);
				glVertex3f(cube_width / 2, -cube_depth / 2, cube_height / 2);
				break;
			case 10:
				glVertex3f(-cube_width / 2, cube_depth / 2, -cube_height / 2);
				glVertex3f(cube_width / 2, cube_depth / 2, -cube_height / 2);
				break;
			case 11:
				glVertex3f(-cube_width / 2, cube_depth / 2, cube_height / 2);
				glVertex3f(cube_width / 2, cube_depth / 2, cube_height / 2);
				break;
			default:
				break;
			}

		glEnd();
		glPopMatrix();

	}

}   
 
GLvoid initializeGL(GLsizei width, GLsizei height) 
{ 
    GLfloat     aspect;  
 
    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f ); 
    glClearDepth( 50.0 ); 
 
    glEnable(GL_DEPTH_TEST); 
	glEnable(GL_LINE_SMOOTH);
	glEnable (GL_POINT_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelZoom(1.0, -1.0);
 
    glMatrixMode( GL_PROJECTION ); 
    aspect = (GLfloat) width / height; 
    gluPerspective( 45.0, aspect, 0.1, 12.0 ); 
    glMatrixMode( GL_MODELVIEW );

	BuildFont();
	
	update_camera(&cams[OUTSIDE_CAM], 90.0, 0.0, 0.0, 0.0, -6.8);
	update_camera(&cams[CENTER_CAM], 90.0, 0.0, 0.0, 0.0, 0.0);

	cube_width = 6 / (float)board.x;
	cube_height = 3 / (float)board.y;
	cube_depth = 4 / (float)board.z;

} 

GLint Selection(void)
{
	GLuint	buffer[512];
	GLint	hits;
	GLint	viewport[4];
	int loop, choose, depth;
	
	glGetIntegerv(GL_VIEWPORT, viewport);
	glSelectBuffer(512, buffer);

	glRenderMode(GL_SELECT);

	glInitNames();
	glPushName(0);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	gluPickMatrix((GLdouble) mouse_x, (GLdouble) (viewport[3]-mouse_y), 10.0f, 10.0f, viewport);

	gluPerspective(45.0f, (GLfloat) (viewport[2]-viewport[0])/(GLfloat) (viewport[3]-viewport[1]), 0.1f, 100.0f);
	glMatrixMode(GL_MODELVIEW);
	drawScene(GL_SELECT);
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	hits=glRenderMode(GL_RENDER);
    if (hits > 0)
	{
		choose = buffer[3];
		depth = buffer[1];

		for (loop = 1; loop < hits; loop++)
		{
			if (buffer[loop*4+1] < (GLuint)depth)
			{
				choose = buffer[loop*4+3];
				depth = buffer[loop*4+1];
			}       
		}
	}
	else
		return -1;

	return choose;
} 

GLvoid drawScene(GLenum mode) 
{ 
    int i, j, k;

	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glLoadIdentity();
/* 
    glPushMatrix(); 
	
		glViewport(0, 0, 825, 450);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-img->cols, img->cols, 0.0, img->rows, -1.0, 1.0);
		glMatrixMode(GL_MODELVIEW);

		glRasterPos2i(-img->cols, img->rows);
		glDrawPixels(img->cols, img->rows, GL_BGR_EXT, GL_UNSIGNED_BYTE, img->data[0]);

	glPopMatrix();

	glLoadIdentity();
*/
	glPushMatrix();

/*		glMatrixMode( GL_PROJECTION );  
		gluPerspective( 45.0, (float)825 / (float)450, 0.1, 12.0 ); 
		glMatrixMode( GL_MODELVIEW );
*/		
		if (mode == GL_RENDER) {

			if (!gameInProgress) {
				glColor3f(0.0f, 1.0f, 0.0f);
				glRasterPos3f(-1.0f, 0.0f, -1.5f);
				glPrint("Press F1 to set up options or press F2 to start game.");
			}

			glColor3f(1.0f, 0.5f, 0.5f);
			glRasterPos3f(-1.04f, 0.57f, -1.5f);
			switch(players[0].type) {
			case HOTSEAT:
				glPrint("Player 1: %d", players[0].score);
				break;
			case CPU:
				glPrint("CPU 1: %d", players[0].score);
				break;
			case REMOTE_USER:
				glPrint("%s: %d", remoteAddr, players[0].score);
				break;
			default:
				break;
			}

			glColor3f(0.5f, 0.5f, 1.0f);
			switch(players[1].type) {
			case HOTSEAT:
				wsprintf(buff, "Player 2: %d", players[1].score);
				glRasterPos3f((float)(1.06-0.043*strlen(buff)), 0.57f, -1.5f);
				glPrint(buff);
				break;
			case CPU:
				wsprintf(buff, "CPU 2: %d", players[1].score);
				glRasterPos3f((float)(1.06-0.043*strlen(buff)), 0.57f, -1.5f);
				glPrint(buff);
				break;
			case REMOTE_USER:
				wsprintf(buff3, "%s", inet_ntoa(sa.sin_addr));
				wsprintf(buff, "%s: %d", buff3, players[1].score);
				glRasterPos3f((float)(1.06-0.043*strlen(buff)), 0.57f, -1.5f);
				glPrint(buff);
				break;
			default:
				break;
			}

			glColor3f(0.0f, 1.0f, 0.0f);
			switch(curPlayer) {
			case PLAYER_ONE:
				glBegin(GL_TRIANGLES);

					glVertex3f(-0.075f, 0.041f, -0.1f);
					glVertex3f(-0.071f, 0.039f, -0.1f);
					glVertex3f(-0.075f, 0.037f, -0.1f);

				glEnd();
				break;
			case PLAYER_TWO:
				glBegin(GL_TRIANGLES);

					glVertex3f(0.074f, 0.041f, -0.1f);
					glVertex3f(0.070f, 0.039f, -0.1f);
					glVertex3f(0.074f, 0.037f, -0.1f);

				glEnd();
				break;
			default:
				break;
			}

		}

		glTranslated(cams[curCam].xTrans, cams[curCam].yTrans, cams[curCam].zTrans);

		if (!gameInProgress) {
			cams[curCam].yRot += 3;
		}
		glRotated(-cams[curCam].xRot, 1.0, 0.0, 0.0); 
		glRotated(cams[curCam].yRot, 0.0, 0.0, 1.0);  

		for (i = 0; i < board.z; i++) {
			for (j = 0; j < board.x; j++) {
				for (k = 0; k < board.y; k++) {

					glTranslated(-3.0 + cube_width / 2 + j * cube_width, -2.0 + cube_depth / 2 + i * cube_depth, -1.5 + cube_height / 2 + k * cube_height);
					if (mode == GL_RENDER) {
						drawSquares(board.x * board.y * i + board.y * j + k);
					} 
					drawLines(board.x * board.y * i + board.y * j + k);
					glTranslated(3.0 - cube_width / 2 - j * cube_width, 2.0 - cube_depth / 2 - i * cube_depth, 1.5 - cube_height / 2 - k * cube_height);

				}

			}
		}
 
    glPopMatrix(); 

	glFlush();
 
    SWAPBUFFERS; 
}

GLvoid BuildFont(GLvoid)
{
	HFONT	font;
	HFONT	oldfont;

	base = glGenLists(96);

	font = CreateFont(	-24,
						0,
						0,
						0,
						FW_BOLD,
						FALSE,
						FALSE,
						FALSE,
						ANSI_CHARSET,
						OUT_TT_PRECIS,
						CLIP_DEFAULT_PRECIS,
						ANTIALIASED_QUALITY,
						FF_DONTCARE|DEFAULT_PITCH,
						"Courier New");

	oldfont = (HFONT)SelectObject(ghDC, font);
	wglUseFontBitmaps(ghDC, 32, 96, base);
	SelectObject(ghDC, oldfont);
	DeleteObject(font);
}

GLvoid KillFont(GLvoid)
{
	glDeleteLists(base, 96);
}

GLvoid glPrint(const char *fmt, ...)
{
	char		text[256];
	va_list		ap;

	if (fmt == NULL)
		return;

	va_start(ap, fmt);
	    vsprintf(text, fmt, ap);
	va_end(ap);

	glPushAttrib(GL_LIST_BIT);
	glListBase(base - 32);
	glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);
	glPopAttrib();
}

DWORD WINAPI CPUMove(PVOID pvoid) {

	volatile PPARAMS pparams;
	int tmp;
    
    pparams = (PPARAMS)pvoid;
    while (TRUE) {
        WaitForSingleObject(pparams->hEvent, INFINITE);
		//Sleep(200);
		tmp = cpu_make_move(&board, pparams->diff[curPlayer-1]);
		SendMessage(ghWnd, GAME_MADE_MOVE, (WPARAM)tmp, 0);

	}

}