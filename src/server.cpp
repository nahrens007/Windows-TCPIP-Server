/*
 * server.cpp
 *
 *  Created on: Mar 4, 2017
 *      Author: nahrens
 */

#include <winsock2.h>	// needed for socket
#include <ws2tcpip.h>	//needed for socket
#include <windows.h> //needed for thread
#include <list>
#include <iostream>

using namespace std;

#ifdef __cplusplus
extern "C"
{
#endif
void WSAAPI freeaddrinfo(struct addrinfo*);

int WSAAPI getaddrinfo(const char*, const char*, const struct addrinfo*,
		struct addrinfo**);

int WSAAPI getnameinfo(const struct sockaddr*, socklen_t, char*, DWORD, char*,
		DWORD, int);
#ifdef __cplusplus
}
#endif

int handle_client(SOCKET client);
void broadcast(char msg[], int buflen);

DWORD WINAPI tclient(LPVOID lpParameter);

list<SOCKET> clients;

int main(int argc, char *argv[])
{
	WSADATA wsaData;

	int iResult;
	const char* port;

	/*
	 * If the port number was included as a command line
	 * argument, then use the provided port. Otherwise, use
	 * the default.
	 */
	if (argc == 1)
		port = "8029";
	else
		port = argv[1];

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		cout << "WSAStartup failed: %d\n" << iResult << endl;
		return 1;
	}

	// The addrinfo structure is used by the getaddrinfo funciton
	struct addrinfo *result = NULL, *prt = NULL, hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET; // specify the IPv4 family
	hints.ai_socktype = SOCK_STREAM; // Specify stream sock
	hints.ai_protocol = IPPROTO_TCP; // Specify TCP protocol
	//caller intends to use the returned socket address structure in a call to the bind function.
	hints.ai_flags = AI_PASSIVE;

	// Resolve the local address and port to be used by the server
	iResult = getaddrinfo(NULL, port, &hints, &result);
	if (iResult != 0)
	{
		cout << "getaddrinfo failed: %d\n" << iResult << endl;
		WSACleanup();
		return 1;
	}

	SOCKET ListenSocket = INVALID_SOCKET;

	// Create a SOCKET for the server to listen for client connections
	ListenSocket = socket(result->ai_family, result->ai_socktype,
			result->ai_protocol);

	if (ListenSocket == INVALID_SOCKET)
	{
		cout << "Error at socket(): " << WSAGetLastError() << endl;
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int) result->ai_addrlen);
	if (iResult == SOCKET_ERROR)
	{
		cout << "bind failed with error: " << WSAGetLastError() << endl;
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);

	// Handle the connections
	SOCKET ClientSocket;
	do
	{

		// Listen for a connection
		if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR)
		{
			cout << "Listen failed with error: " << WSAGetLastError() << endl;
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}

		// Accepting a connection
		ClientSocket = INVALID_SOCKET;
		cout << "Listening for new client..." << endl;
		// Accept a client socket
		ClientSocket = accept(ListenSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET)
		{
			cout << "accept failed: " << WSAGetLastError() << endl;
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}
		cout << "Client accepted." << endl;
		// ClientSocket is the socket which is connected to the client
		// Create a new thread for the client
		// Create a thread to handle the new client
		SOCKET* sock = new SOCKET(ClientSocket);
		clients.push_back(*sock);

		CreateThread(0, 0, tclient, sock, 0, NULL);
	} while (true); // never stop the server (use CTR+C to force stop)
}

/*
 * This function will broadcast the received message to
 * all the SOCKET connections in the clients list.
 * If there is an error sending the message to any client,
 * the client will be removed from the clients list.
 */
void broadcast(char msg[], int buflen)
{
	/*
	 * In order for broadcast to work, I would have to create a
	 * list that contained all of the SOCKET's of the clients and
	 * iterate over them all in order to send the message to all clients.
	 */
	int iSendResult;
	cout << "broadcast: ";
	for (int i = 0; i < buflen-2; i++)
		cout << msg[i];
	cout << endl;
	for (list<SOCKET>::iterator it = clients.begin(); it != clients.end(); ++it)
	{
		// Iterate through clients, sending the message to everyone.

		// Echo the buffer back to the sender
		iSendResult = send(*it, msg, buflen, 0);
		if (iSendResult == SOCKET_ERROR)
		{
			// If there was an error sending the message, remove the SOCKET from the list
			cout << "send failed: " << WSAGetLastError() << endl;
			clients.remove(*it); // remove client from list
			closesocket(*it);
			WSACleanup();
		}
	}
}

/*
 * This function is meant to be called by "CreateThread".
 * It's purpose is to create a new thread for each new
 * SOCKET connection. That way, this server will be able
 * to listen to multiple different clients (SOCKETs) at once.
 */
DWORD WINAPI tclient(LPVOID lpParameter)
{
	SOCKET* client = ((SOCKET*) lpParameter);
	if (handle_client(*client) != 0)
	{
		cout << "Error with handling client..." << endl;
		return 1;
	}
	// Delete the SOCKET, since we used "new" to create it.
	delete client;
	return 0;
}

/*
 * This function is used to do the actual handling of each client.
 * It will listen to each client, and when something is received,
 * it will call broadcast. It will also remove clients' SOCKET
 * from the list clients whenever there is a disconnection or an error.
 */
#define DEFAULT_BUFLEN 512
int handle_client(SOCKET client)
{
	// send and receive data
	char recvbuf[DEFAULT_BUFLEN];
	int iResult;
	int recvbuflen = DEFAULT_BUFLEN;

	char msg[6] = "Hello";
	send(client, msg, 6, 0);

	// Receive until the peer shuts down the connection
	do
	{

		iResult = recv(client, recvbuf, recvbuflen, 0);
		if (iResult > 0)
		{
			// Echo the buffer back to the sender
			broadcast(recvbuf, iResult);
		}
		else if (iResult == 0)
			cout << "Connection closing..." << endl;
		else
		{
			cout << "handle_client() recv failed: " << WSAGetLastError()
					<< endl;
			clients.remove(client); // remove client from list
			closesocket(client);
			WSACleanup();
			return 1;
		}

	} while (iResult > 0);
	clients.remove(client); // remove client from list
	return 0;
}
