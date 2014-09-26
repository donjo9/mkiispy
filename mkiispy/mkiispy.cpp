// WinSockmkII.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define DEFAULT_PORT "10794"
#define DEFAULT_IP "192.168.0.106"
#define DB_SERVER 188.179.200.84
#define DB_PORT 80
#define DEFAULT_BUFLEN 2048

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <iostream>

#pragma comment(lib, "Ws2_32.lib")
using namespace std;



int _tmain(int argc, char *argv[])
{
	int i;
	int errorCode;
	char *sendbuf = "this is a test\n\r";
	char ClientRecvbuf[DEFAULT_BUFLEN];
	char ServerRecvbuf[DEFAULT_BUFLEN];
	int ServerRecvLen = 0;
	int ClientRecvLen = 0;
	u_long NonBlock = 1;
	FD_SET WriteSet;
	FD_SET ReadSet;
	WSADATA winSockData;
	bool endloop = false;
	struct addrinfo *resultC = NULL, *ptrC = NULL, hintsC;
	struct addrinfo *resultS = NULL, *ptrS = NULL, hintsS;

	SOCKET DBSocket = INVALID_SOCKET;
	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET BoxSocket = INVALID_SOCKET;
	
	for (i = 1; i < argc; i++)
	{
		switch (i)
		{
		case 1:
			cout << "Server IP: ";
			break;
		case 2: 
			cout << "Server Port: ";
			break;
		case 3:
			cout << "Listening IP: ";
			break;
		case 4:
			cout << "Listening Port: ";
			break;
		default:
			break;
		}
		cout << argv[i] << endl;
	}
	cout << endl;

	#pragma region Shared Functions
	errorCode = WSAStartup(MAKEWORD(2, 2), &winSockData);
	if (errorCode != 0)
	{
		cout << "WSAStartup failed" << endl;
		return 1;
	}

	
	#pragma endregion	

	#pragma region Client Setup

	ZeroMemory(&hintsC, sizeof(hintsC));
	hintsC.ai_family = AF_UNSPEC;
	hintsC.ai_socktype = SOCK_STREAM;
	hintsC.ai_protocol = IPPROTO_TCP;


	errorCode = getaddrinfo(argv[1], argv[2], &hintsC, &resultC);
	if (errorCode != 0)
	{
		cout << "getaddrinfo failed" << endl;
		WSACleanup();
		return 1;
	}
	ptrC = resultC;

	
	#pragma endregion

	#pragma region Server Setup
	ZeroMemory(&hintsS, sizeof (hintsS));
	hintsS.ai_family = AF_INET;
	hintsS.ai_socktype = SOCK_STREAM;
	hintsS.ai_protocol = IPPROTO_TCP;
	hintsS.ai_flags = AI_PASSIVE;

	if (argc == 5)
	{
		errorCode = getaddrinfo(argv[3], argv[4], &hintsS, &resultS);
	}
	else {
		errorCode = getaddrinfo(DEFAULT_IP, DEFAULT_PORT, &hintsS, &resultS);
	}

	if (errorCode != 0) {
		cout << "getaddrinfo failed: " << errorCode << endl;
		WSACleanup();
		return 1;
	}

	ListenSocket = socket(resultS->ai_family, resultS->ai_socktype, resultS->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		cout << "Error at socket(): " << WSAGetLastError() << endl;
		freeaddrinfo(resultS);
		WSACleanup();
		return 1;
	}
	struct sockaddr_in *ipv4 = (struct sockaddr_in *)resultS->ai_addr;
	cout << inet_ntoa(ipv4->sin_addr) << endl;


	errorCode = bind(ListenSocket, resultS->ai_addr, (int)resultS->ai_addrlen);
	if (errorCode == SOCKET_ERROR) {
		cout << "bind failed with error: " << WSAGetLastError() << endl;
		freeaddrinfo(resultS);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(resultS);

	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
		cout << "Listen failed with error: " << WSAGetLastError() << endl;
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	
	
	
	#pragma endregion

	#pragma region Loop
	while(endloop == false)
	{
		if (DBSocket == INVALID_SOCKET)
		{
			DBSocket = socket(ptrC->ai_family, ptrC->ai_socktype, ptrC->ai_protocol);

			if (DBSocket == INVALID_SOCKET) {
				cout << "DBSocket - Error at socket(): " << WSAGetLastError() << endl;
				freeaddrinfo(resultC);
				WSACleanup();
				return 1;
			}

			errorCode = connect(DBSocket, ptrC->ai_addr, (int)ptrC->ai_addrlen);

			if (errorCode == SOCKET_ERROR) {
				closesocket(DBSocket);
				DBSocket = INVALID_SOCKET;
				freeaddrinfo(resultC);
				cout << "Unable to connect to server!" << endl;
				WSACleanup();
				return 1;
			}
			else {
				cout << "Forbindelse til serveren!" << endl;
			}
		}
		FD_ZERO(&WriteSet);
		FD_ZERO(&ReadSet);

		if (DBSocket != INVALID_SOCKET)
		{
			FD_SET(DBSocket, &WriteSet);
			FD_SET(DBSocket, &ReadSet);
		}
		if (BoxSocket != INVALID_SOCKET)
		{
			FD_SET(BoxSocket, &ReadSet);
			FD_SET(BoxSocket, &WriteSet);
		}
		
		if (ListenSocket != INVALID_SOCKET)
		{
			FD_SET(ListenSocket, &ReadSet);
			FD_SET(ListenSocket, &WriteSet);
		}
		

		errorCode = select(0,&ReadSet,&WriteSet,NULL,NULL);
		if (!(errorCode > 0))
		{
			cout << "Select error: " << WSAGetLastError() << endl;
		}

		if (FD_ISSET(DBSocket, &ReadSet))
		{
			ioctlsocket(DBSocket, FIONBIO, &NonBlock);
			ClientRecvLen = recv(DBSocket, ClientRecvbuf, DEFAULT_BUFLEN, 0);
			if (ClientRecvLen > 0)
			{
				cout << "DB - Bytes received: " << ClientRecvLen << endl;
			}
			else if (ClientRecvLen  == 0)
			{
				cout << "DB Connection closed" << endl;
				DBSocket = INVALID_SOCKET;
			}
			else {
				cout << "DB recv failed: " << WSAGetLastError() << endl;
			}
			

		}
		if (FD_ISSET(DBSocket, &WriteSet))
		{
		
			if (ServerRecvLen > 0)
			{
				errorCode = send(DBSocket, ServerRecvbuf, ServerRecvLen, 0);
				if (ServerRecvLen != errorCode)
				{
					cout << "Relay send failed, Box -> DB" << endl;
				}
				ServerRecvLen = 0;
			}
		}
		if (FD_ISSET(ListenSocket, &ReadSet))
		{

			BoxSocket = accept(ListenSocket, NULL, NULL);
			ioctlsocket(BoxSocket, FIONBIO, &NonBlock);
			if (BoxSocket == INVALID_SOCKET) {
				cout << "accept failed: " << WSAGetLastError() << endl;
				closesocket(ListenSocket);
				WSACleanup();
				return 1;
			}
			else {
				cout << "Socket accepted" << endl;
			}
		}

		if (FD_ISSET(BoxSocket, &ReadSet))
		{
			ServerRecvLen = recv(BoxSocket, ServerRecvbuf, DEFAULT_BUFLEN, 0);
			if (ServerRecvLen > 0) {
				cout << "Box - Bytes received: " << ServerRecvLen << endl;

			}
			else if (ServerRecvLen == 0)
			{
			
				cout << "Box Connection closing..." << endl;
				BoxSocket = INVALID_SOCKET;
			}
			else {

			}
			

		}
		if (FD_ISSET(BoxSocket, &WriteSet))
		{
			if (ClientRecvLen > 0)
			{
				errorCode = send(BoxSocket, ClientRecvbuf, ClientRecvLen, 0);
				if (errorCode != ClientRecvLen)
				{
					cout << "Relay send failed, DB -> Box" << endl;
				}
				ClientRecvLen = 0;
			}
		}
		
	}
#pragma endregion	

	closesocket(DBSocket);
	closesocket(BoxSocket);
	WSACleanup();
	return 0;
}


