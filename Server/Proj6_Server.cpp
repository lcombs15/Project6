/*
File Name: windows_server.cpp
Author: Lucas Combs
Course: CSC 402
Date: 04/16/2018
*/
#undef UNICODE

#define WIN32_LEAN_AND_MEAN
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "5"

#include <random>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <vector>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
using namespace std;

/*
Evaluate command. Return empty string if command invalid.
*/
string evaluate(string cmd) {
	if (cmd._Equal("move")) {
		return "move complete.";
	}
	else if (cmd._Equal("stats")) {
		return "Here's a stat: " + to_string(rand());
	}
	else if (cmd._Equal("shoot")) {
		return "Shoot for the stars!";
	}

	return string();
}



SOCKET ClientSocket = INVALID_SOCKET;

void handle_client(int pid) {
	int iResult, iSendResult;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;

	//Copy ClientSocket before next Client to connect wipes it out.
	SOCKET mysocket = ClientSocket;

	//Announce new connection!
	cout << "\nClient " << pid << " connected.\n";

	// Receive until the peer shuts down the connection
	do {
		iResult = recv(mysocket, recvbuf, recvbuflen, 0);

		//Handle input
		if (iResult > 0) {
			cout
				<< "Bytes received: " << iResult << "\n"
				<< string(recvbuf) << "\n";

			string result = evaluate(string(recvbuf));

			//Handle exit command
			if (string(recvbuf) == "quit") {
				cout << "exiting server.\n\n";
				break;
			}
			else if (!result.empty()) {
				cout << string(recvbuf) << " command received\n";
			}
			else { //Check for invalid comman
				result = "Command not found.";
			}

			// Echo the buffer back to the sender
			iSendResult = send(mysocket, result.c_str(), result.length() + 1, 0);
			if (iSendResult == SOCKET_ERROR) {
				cout << "send failed with error: " << WSAGetLastError() << "\n";
				closesocket(mysocket);
				WSACleanup();
				return;
			}
			cout << "Bytes sent: " << iSendResult << "\n";
		}
		else if (iResult == 0) {
			cout << "Connection closing...\n";
			send(mysocket, "", 0, 0);
		}
		else {
			cout << "recv failed with error: " << WSAGetLastError() << "\n";
			closesocket(mysocket);
			WSACleanup();
			return;
		}

	} while (iResult > 0);

	// shutdown the connection since we're done
	iResult = shutdown(mysocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		cout << "shutdown failed with error: " << WSAGetLastError() << "\n";
		closesocket(mysocket);
		WSACleanup();
		return;
	}

	// cleanup
	closesocket(mysocket);
	WSACleanup();
}

//Server main method
int __cdecl main(void)
{
	WSADATA wsaData;
	int iResult;

	SOCKET ListenSocket = INVALID_SOCKET;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	//int iSendResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		cout << "WSAStartup failed with error: " << iResult << "\n";
		return 1;
	}

	//Setup hints
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo("localhost", DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		cout << "getaddrinfo failed with error: " << iResult << "\n";
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		cout << "socket failed with error: " << WSAGetLastError() << "\n";
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = ::bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		cout << "bind failed with error: " << WSAGetLastError() << "\n";
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		cout << "listen failed with error: " << WSAGetLastError() << "\n";
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	int client_num = 1;
	vector<thread> threads = vector<thread>();

	cout << "Server Starting.....\nWaiting for clients.....";
	while (true) {
		// Accept a client socket
		ClientSocket = accept(ListenSocket, NULL, NULL);
		if (ClientSocket != INVALID_SOCKET) {
			threads.push_back(thread(handle_client, client_num));
			Sleep(100);
			client_num++;
		}
	}

	for (thread& t : threads) {
		t.join();
	}

	// No longer need server socket
	closesocket(ListenSocket);
	return 0;
}