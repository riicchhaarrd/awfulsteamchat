#include "stdafx.h"
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include "Win32Project1.h"
#include "steam.h"
#include <time.h>

/* net stuff*/

#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
/* end */

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

static bool steam_init = false;

class MsgType {
public:
	std::string msg;
	CSteamID steamid;
	std::string sFriendName;
	time_t time;
};

std::vector<MsgType> v_friendMessages;

void exitproc() {
	SteamAPI_Shutdown();

	ExitProcess(0);
}

char *va(char *format, ...) {
	va_list argptr;
#define MAX_VA_STRING   32000
	static char temp_buffer[MAX_VA_STRING];
	static char string[MAX_VA_STRING];      // in case va is called by nested functions
	static int index = 0;
	char    *buf;
	int len;


	va_start(argptr, format);
	vsprintf(temp_buffer, format, argptr);
	va_end(argptr);

	if ((len = strlen(temp_buffer)) >= MAX_VA_STRING) {
		MessageBoxA(NULL, "va buffer overrun", "", 0);
		exitproc();
	}

	if (len + index >= MAX_VA_STRING - 1) {
		index = 0;
	}

	buf = &string[index];
	memcpy(buf, temp_buffer, len + 1);

	index += len + 1;

	return buf;
}

//#define WELCOME_MSG "[ 46100 Chat Version 1.0 ] Just talk back to me to start receiving messages and It'll get send to the other people which are also in 46100"
#define WELCOME_MSG "{ Type /num for active chatters }"

class u_SPlayer {
public:
	std::string name;
	CSteamID steamid;
	bool active;
};

std::vector<u_SPlayer> u_players;

class ChatCallback {
public:
	STEAM_CALLBACK(ChatCallback, FriendChatCallback, GameConnectedFriendChatMsg_t, inf);

	ChatCallback() : inf(this, &ChatCallback::FriendChatCallback) { }
};

void FriendChatHandler(std::string msg, CSteamID id) {
	static int last_sent_time = time(NULL);

	const char *sFriendName = SteamFriends()->GetFriendPersonaName(id);
	static int once = 0;
	if (!once) {
		//MsgBox(va("%s from %s", msg.c_str(), sFriendName));
		once = 1;
	}

	//if (strstr(msg.c_str(), "/join")) {
		for (std::vector<u_SPlayer>::iterator it = u_players.begin(); it != u_players.end(); ++it) {
			if (id.GetAccountID() == it->steamid.GetAccountID())
				it->active = true;
		}
	//} else
	if (strstr(msg.c_str(), "/num")) {

		size_t chatters_num = 0;
		for (std::vector<u_SPlayer>::iterator it = u_players.begin(); it != u_players.end(); ++it) {
			if (it->active)
				chatters_num++;
		}
		SteamFriends()->ReplyToFriendMessage(id, va("Active chatters: %d\n", chatters_num));
		return;
	}

	MsgType mt;
	mt.msg = msg;
	mt.sFriendName = sFriendName;
	mt.steamid = id;
	mt.time = time(NULL);
	bool found = false;
	for (std::vector<MsgType>::iterator it = v_friendMessages.begin(); it != v_friendMessages.end(); ++it) {
		if (it->steamid.GetAccountID() == id.GetAccountID()) {
			found = true;
			break;
		}
	}
	if (!found)
		v_friendMessages.push_back(mt);

	if (v_friendMessages.size() != 0) {
		std::string big = "";
		for (std::vector<MsgType>::iterator it = v_friendMessages.begin(); it != v_friendMessages.end(); ++it) {
			big += it->sFriendName;
			big += ": ";
			big += it->msg;
			big.push_back('\n');
		}

		for (std::vector<u_SPlayer>::iterator it = u_players.begin(); it != u_players.end(); ++it) {
			if (!it->active)
				continue;
			if(SteamFriends()->GetFriendPersonaState(it->steamid) == k_EPersonaStateOnline)
				SteamFriends()->ReplyToFriendMessage(it->steamid, big.c_str());
		}


		v_friendMessages.clear();
	}
}

void ChatCallback::FriendChatCallback(GameConnectedFriendChatMsg_t* info) {
	char buffer[2048] = { 0 };
	EChatEntryType entry;
	SteamFriends()->GetFriendMessage(info->m_steamIDUser, info->m_iMessageID, buffer, sizeof(buffer), &entry);
	static bool once = false;
	if (!once) {
		//MsgBox("test");
		once = true;
	}
	if (entry == EChatEntryType::k_EChatEntryTypeChatMsg) {
		FriendChatHandler(std::string(buffer), info->m_steamIDUser);
	}
}

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	srand(time(NULL));
	WSADATA wsaData;

	if (WSAStartup(0x202, &wsaData) != 0) {
		MsgBox("fail wsastartup\n");
		ExitProcess(0);
		return 0;
	}
#if 1
	SetEnvironmentVariableA("SteamGameId", va("%d", STEAM_APPID));
	SetEnvironmentVariableA("SteamAppId", va("%d", STEAM_APPID));
#endif
	if (!SteamAPI_Init()) {
		MessageBoxW(NULL, L"fail load steam", L"",MB_OK|MB_ICONERROR);
		exitproc();
	}

	steam_init = true;

	SteamFriends()->SetListenForFriendsMessages(true);
	ChatCallback c;

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_WIN32PROJECT1, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WIN32PROJECT1));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		SteamAPI_RunCallbacks();
	}
	SteamAPI_Shutdown();
	WSACleanup();
	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WIN32PROJECT1));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCE(IDC_WIN32PROJECT1);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

#define DEF_HTTP_PORT 80
#define RECV_BUFFERSIZE 4096
std::string get_site_result(std::string url){
	std::string request;
	char buffer[RECV_BUFFERSIZE];
	struct sockaddr_in serveraddr;
	int sock;

	if (url.find("http://") != -1) {
		std::string temp = url;
		url = temp.substr(url.find("http://") + 7);
	}

	int dm = url.find("/");
	std::string file = url.substr(dm);
	std::string shost = url.substr(0, dm);

	request += "GET " + file + " HTTP/1.0\r\n";
	request += "Host: " + shost + "\r\n";
	request += "\r\n";

	if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		return "";

	memset(&serveraddr, 0, sizeof(serveraddr));

	hostent *record = gethostbyname(shost.c_str());
	if (record == NULL)
		return "";
	in_addr *address = (in_addr *)record->h_addr;
	std::string ipd = inet_ntoa(*address);
	const char *ipaddr = ipd.c_str();

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(ipaddr);
	serveraddr.sin_port = htons(DEF_HTTP_PORT);

	if (connect(sock, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
		return "";

	if (send(sock, request.c_str(), request.length(), 0) != request.length())
		return "";

	int nRecv;
	std::string result = "";

	while ((nRecv = recv(sock, (char*)&buffer, RECV_BUFFERSIZE, 0))>0) {
		int i = 0;
		for (; i != nRecv; ++i)
			result += buffer[i];
	}
	int pos;
	if ((pos = result.find("\r\n\r\n"))>0) {
		result = result.substr(pos + 4);
	}
	closesocket(sock);
	return result;
}

std::vector<std::string> split_string(const std::string& str,
	const std::string& delimiter)
{
	std::vector<std::string> strings;

	std::string::size_type pos = 0;
	std::string::size_type prev = 0;
	while ((pos = str.find(delimiter, prev)) != std::string::npos)
	{
		strings.push_back(str.substr(prev, pos - prev));
		prev = pos + 1;
	}

	// To get the last substring (or only, if delimiter is not found)
	strings.push_back(str.substr(prev));

	return strings;
}

void read_u_players(std::string& content) {
	std::vector<std::string> lines = split_string(content, "\n");
	
	int bracket = 0;
	
	u_SPlayer player;

	static int msgbox_once = 0;

	for (std::vector<std::string>::iterator it = lines.begin(); it != lines.end(); ++it) {

		if (it->empty())
			continue;

		if (!strcmp(it->c_str(), "{"))
			bracket++;
		else if (!strcmp(it->c_str(), "}"))
			bracket--;
		else {
#define ACCOUNT_ID_STR "\"accountid\":"
#define NAME_STR "\"name\":"
			size_t f_idx = std::string::npos;
			if ((f_idx = it->find(ACCOUNT_ID_STR)) != std::string::npos) {
				f_idx += strlen(ACCOUNT_ID_STR);
				if (f_idx < it->length()) {

					std::string sAccID = it->substr(f_idx + 1, it->length() - 2 - f_idx);
					
#if 0
					if (!msgbox_once) {
						MsgBox(
							va("accid=%s\n",

							it->substr(f_idx + 1, it->length() - 2 - f_idx).c_str()
							),
							"",
							0);
						msgbox_once = 1;
					}
#endif

					uint32 ui_steamid = (uint32)strtoull(sAccID.c_str(), NULL, 10);
					CSteamID csteamid = CSteamID(ui_steamid, k_EUniversePublic, k_EAccountTypeIndividual);
					if (ui_steamid > 0 && SteamFriends()->GetFriendPersonaState(csteamid) == k_EPersonaStateOnline) {
						player.steamid = csteamid;
						size_t chatters_num = 0;
						for (std::vector<u_SPlayer>::iterator it = u_players.begin(); it != u_players.end(); ++it) {
							if (it->active)
								chatters_num++;
						}
						SteamFriends()->ReplyToFriendMessage(player.steamid, va("%s (Active chatters: %d)", WELCOME_MSG, chatters_num));
						//SteamFriends()->GetFriendMessage
					}
				}
			} else if ((f_idx = it->find(NAME_STR)) != std::string::npos) {
				f_idx += strlen(NAME_STR);
				if (f_idx < it->length()) {

					std::string playername = it->substr(f_idx + 2, it->length() - 3 - f_idx);
					if (!msgbox_once) {
						MsgBox(
							va("accid=%s\n",

							playername.c_str()
							),
							"",
							0);
						msgbox_once = 1;
					}
					if (player.steamid.GetAccountID() != 0) {
						player.name = playername;
						player.active = false;
						u_players.push_back(player);
						player.name = "";
						player.steamid = CSteamID();
					}
				}
			}

		}

	}

	if (bracket != 0) {
		MsgBox("missing ending bracket\n");
	}
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   if (SteamUser() != nullptr)
	   MsgBox(va("Welcome %s", SteamFriends()->GetPersonaName()));

   std::string s_res = get_site_result("http://steamapi-a.akamaihd.net/ITowerAttackMiniGameService/GetPlayerNames/v0001/?gameid=46100");

   if (!s_res.empty()) {
	   read_u_players(s_res);
   }
   
   ISteamFriends *fr = SteamFriends();
   FILE *log = fopen("./proj_log.txt", "w");

   if (!log)
	   return TRUE;
#if 0
   for (int i = 0; i < fr->GetClanCount(); i++) {
	   CSteamID cid = fr->GetClanByIndex(i);
	   const char *sClanName = fr->GetClanName(cid);
	   if (strstr(sClanName, "Elite")) {
		   MessageBoxA(NULL,
			   va("group name = %s\n", sClanName),
			   "",
			   MB_OK);
		   MsgBox(va("officers = %d\n", fr->GetClanOfficerCount(cid)));
		   for (int j = 0; j < fr->GetClanOfficerCount(cid); j++) {
			   CSteamID officer = fr->GetClanOfficerByIndex(cid, j);

			   const char *officer_nickname = fr->GetPlayerNickname(officer);
			   fprintf(log, "nick: %s\n", officer_nickname);
#if 0
			   for (std::vector<u_SPlayer>::iterator it = u_players.begin(); it != u_players.end(); ++it) {
				   if (it->steamid.GetAccountID() == officer.GetAccountID()) {
					   fprintf(log, "player %s\n", officer_nickname);
				   }
			   }
#endif
		   }
	   }
   }
   #endif

   fclose(log);

#if 0
   int group_num = SteamFriends()->GetFriendsGroupCount();

   for (int i = 0; i < group_num; i++) {
	   FriendsGroupID_t fgi = SteamFriends()->GetFriendsGroupIDByIndex(group_num);
	   MessageBoxA(hWnd, SteamFriends()->GetFriendsGroupName(fgi), "", 0);

   }
#endif

   //SteamFriends()->SendClanChatMessage()
   //MessageBoxA(hWnd, , "", 0);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
