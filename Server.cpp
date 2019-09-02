#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "ServerHeader.h"

struct SocketState sockets[MAX_SOCKETS] = { 0 };
struct timeval timeout;
int socketsCount = 0;

void main()
{

	WSAData wsaData;

	if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		cout << "Web Server: Error at WSAStartup()\n";
		return;
	}


	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (INVALID_SOCKET == listenSocket)
	{
		cout << "Web Server: Error at socket(): " << WSAGetLastError() << endl;
		WSACleanup();
		return;
	}

	// Create a sockaddr_in object called serverService.
	sockaddr_in serverService;

	// Address family (must be AF_INET - Internet address family).
	serverService.sin_family = AF_INET;


	serverService.sin_addr.s_addr = INADDR_ANY;

	serverService.sin_port = htons(TIME_PORT);


	if (SOCKET_ERROR == bind(listenSocket, (SOCKADDR *)&serverService, sizeof(serverService)))
	{
		cout << "Web Server: Error at bind(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return;
	}


	if (SOCKET_ERROR == listen(listenSocket, 5))
	{
		cout << "Time Server: Error at listen(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return;
	}
	addSocket(listenSocket, LISTEN);

	timeout.tv_sec = 120;
	timeout.tv_usec = 0;

	// Accept connections and handles them one by one.
	while (true)
	{

		fd_set waitRecv;
		FD_ZERO(&waitRecv);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if ((sockets[i].recv == LISTEN) || (sockets[i].recv == RECEIVE))
				FD_SET(sockets[i].id, &waitRecv);
		}

		fd_set waitSend;
		FD_ZERO(&waitSend);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if (sockets[i].send == SEND)
				FD_SET(sockets[i].id, &waitSend);
		}

		int nfd;
		nfd = select(0, &waitRecv, &waitSend, NULL, NULL);
		if (nfd == SOCKET_ERROR)
		{
			cout << "Web Server: Error at select(): " << WSAGetLastError() << endl;
			WSACleanup();
			return;
		}

		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitRecv))
			{
				nfd--;
				switch (sockets[i].recv)
				{
				case LISTEN:
					acceptConnection(i);
					break;

				case RECEIVE:
					receiveMessage(i);
					break;
				}
			}
		}

		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitSend))
			{
				nfd--;
				switch (sockets[i].send)
				{
				case SEND:
					sendMessage(i);
					break;
				}
			}
		}
	}

	// Closing connections and Winsock.
	cout << "Web Server: Closing Connection.\n";
	closesocket(listenSocket);
	WSACleanup();
}

bool addSocket(SOCKET id, int what)
{
	//
	// Set the socket to be in non-blocking mode.
	//
	unsigned long flag = 1;
	if (ioctlsocket(id, FIONBIO, &flag) != 0)
	{
		cout << "Time Server: Error at ioctlsocket(): " << WSAGetLastError() << endl;
	}

	for (int i = 0; i < MAX_SOCKETS; i++)
	{
		if (sockets[i].recv == EMPTY)
		{
			sockets[i].id = id;
			sockets[i].recv = what;
			sockets[i].send = IDLE;
			sockets[i].len = 0;
			socketsCount++;
			if (setsockopt(id, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) == SOCKET_ERROR)
			{
				cout << "Web Server: Error at setsockopt: " << WSAGetLastError() << endl;
				return false;
			}
			return (true);
		}
	}
	return (false);
}

void removeSocket(int index)
{
	sockets[index].recv = EMPTY;
	sockets[index].send = EMPTY;
	socketsCount--;
}

void acceptConnection(int index)
{
	SOCKET id = sockets[index].id;
	struct sockaddr_in from;		// Address of sending partner
	int fromLen = sizeof(from);

	SOCKET msgSocket = accept(id, (struct sockaddr *)&from, &fromLen);
	if (INVALID_SOCKET == msgSocket)
	{
		cout << "Time Server: Error at accept(): " << WSAGetLastError() << endl;
		return;
	}
	cout << "Time Server: Client " << inet_ntoa(from.sin_addr) << ":" << ntohs(from.sin_port) << " is connected." << endl;

	


	if (addSocket(msgSocket, RECEIVE) == false)
	{
		cout << "\t\tToo many connections, dropped!\n";
		closesocket(id);
	}
	return;
}

void receiveMessage(int index)
{
	SOCKET msgSocket = sockets[index].id;

	int len = sockets[index].len;
	int bytesRecv = recv(msgSocket, &sockets[index].buffer[len], sizeof(sockets[index].buffer) - len, 0);

	if (SOCKET_ERROR == bytesRecv)
	{
		if (WSAGetLastError() == WSAETIMEDOUT) // hendel at over time
		{
			cout << "Web Server: Error at recv(): Too long to receive, time out triggered." << endl;
		}
		else {
			cout << "Web Server: Error at recv(): " << WSAGetLastError() << endl;
		}
		closesocket(msgSocket);
		removeSocket(index);
		return;
	}
	if (bytesRecv == 0)
	{
		closesocket(msgSocket);
		removeSocket(index);
		return;
	}
	else
	{
		sockets[index].buffer[len + bytesRecv] = '\0'; //add the null-terminating to make it a string
		cout << "Web Server: Recieved: " << bytesRecv << " bytes of \"" << &sockets[index].buffer[len] << "\" \n";
		sockets[index].len += bytesRecv;
		string request;

		if (sockets[index].len > 0)
		{
			ConvertHttpRequest(sockets[index].buffer, request);
			sockets[index].len = 0;
			sockets[index].send = SEND;
			string answer;

			getMethodTypeFromHttpRequest(request, sockets[index].sendSubType);

			if (sockets[index].sendSubType == EXIT)
			{
				closesocket(msgSocket);
				removeSocket(index);
				return;
			}
			else
			{
				MakeTheAnswer(sockets[index].sendSubType, answer, request, sockets[index].buffer);
			}

			for (int i = 0; i < answer.size(); ++i)
				sockets[index].buffer[i] = answer[i];

			sockets[index].buffer[answer.size()] = 0;
			return;
		}
	}
}

void sendMessage(int index)
{
	int bytesSent = 0;
	char sendBuff[400];

	SOCKET msgSocket = sockets[index].id;

	strcpy(sendBuff, sockets[index].buffer);
	bytesSent = send(msgSocket, sendBuff, (int)strlen(sendBuff), 0);
	if (SOCKET_ERROR == bytesSent)
	{
		cout << "Web Server: Error at send(): " << WSAGetLastError() << endl;
		return;
	}

	cout << "Web Server: Sent: " << bytesSent << "\\" << strlen(sendBuff) << " bytes of \"" << sendBuff << "\" \n";

	sockets[index].send = IDLE;
}

string Make_start_header(const string& body)
{
	string messageStatus = "HTTP/1.1 ";
	if (body.compare("File not found") == 0)
		messageStatus += "404 Not Found\r\n";
	else
		messageStatus += "200 OK\r\n";

	return messageStatus;
}

string Make_sizeOfBodyMessage(const string& body)
{
	size_t size = body.length();

	string length = to_string(size);

	return length;
}

string GET_f(string request)
{
	string body = GetFileContent(request);
	string messageStatus = Make_start_header(body);

	messageStatus += MakeHeaderSend();
	messageStatus += Make_sizeOfBodyMessage(body);

	return messageStatus + "\r\n\n" + body;
}

string MakeHeaderSend()
{
	time_t currentTime;
	string response;
	
	response.append("Date: ");
	time(&currentTime);
	response.append(ctime(&currentTime));
	response += "Server: Nely And Daniel's Server\r\nContent-Type: text/html\r\nContent-length: ";
	
	return response;
}

string GetFileName(string request)
{
	request = request.substr(request.find(' ') + 1, request.size());
	string fileName = request.substr(1, request.find(' '));
	fileName[fileName.length()-1] = '\0';
	return fileName;
}

string OPTIONS_f()
{
	string returnMessage = "HTTP/1.1 200 OK\r\n";
	returnMessage += MakeHeaderSend();
	returnMessage += "0\r\nAllow: GET, PUT, OPTIONS, TRACE, HEAD, DELETE, Exit\r\n\n";
	return returnMessage;
}

string GetFileContent(string request)
{
	string fileName = GetFileName(request);

	string content;
	if (fileName.size() > 0)
	{
		ifstream file;
		file.open(fileName);
		if (file.fail())
			return "File not found";
		
		char singelChar = file.get();
		while (singelChar != EOF)
		{
			content.push_back(singelChar);
			singelChar = file.get();
		}
	}
	else content = "<!DOCTYPE HTML>\n<html><div><h3>Input is invalid</h3></div>\n<div><h4>Nely&&Daniel SERVER</h4></div></html>";
	return content;
}

void MakeTheAnswer(int type, string& answer, string& request, char* allContent)
{
	if (type == GET)
		answer = GET_f(request);
	else if (type == HEAD)
		answer = HEAD_f(request);
	else if (type == OPTIONS)
		answer = OPTIONS_f();
	else if (type == TRACE)
		answer = TRACE_f(allContent);
	else if (type == PUT)
		answer = PUT_f(request, allContent);
	else if (type == _DELETE)
		answer = DELETE_f(request);
	else
		answer = request + " method not supported.";
}

string putToFile(string fileName, string Content)
{
	
	FILE* file = fopen(fileName.c_str(), "r");

	if (file != nullptr)
	{
		fclose(file);
		ofstream outputFile;
		outputFile.open(fileName);
		if (outputFile.is_open())
		{
			outputFile << Content;
			outputFile.close();
			return "HTTP/1.1 201 Created\r\n";
		}
		else
		{
			return "HTTP/1.1 500 Internal Server Error\r\n";
		}
	}
	else
	{
		ofstream outputFile(fileName);
		outputFile << Content;
		return "HTTP/1.1 201 Created\r\n";
	}
}

string PUT_f(string request ,char * allContent)
{
	string fileName = GetFileName(request);
	string temp(allContent);
	int len = temp.length();
	string body = temp.substr(temp.find("\n\r\n")+3, temp.length());
	
	string answer = putToFile(fileName, body);
	answer += MakeHeaderSend();

	if (answer.find("201") != string::npos)
	{
		answer += Make_sizeOfBodyMessage(body);
		answer += "\r\n\n";
		answer += body;
	}
	else
	{
		answer += "0\r\n\n";
	}

	return answer;
}

string DELETE_f(string request)
{
	string fileName = GetFileName(request);
	string deleteAnswer = deleteFile(fileName);
	deleteAnswer += MakeHeaderSend();

	string body;

	if (deleteAnswer.find("200") != string::npos) // ??
		body = "<html><h1>URL deleted.</h1></html>";
	else
		body = "<html><h1>URL not deleted.</h1></html>";

	deleteAnswer += Make_sizeOfBodyMessage(body);
	deleteAnswer += "\r\n\n";
	deleteAnswer += body;

	return deleteAnswer;

}

string deleteFile(string fileName)
{
	FILE* file = fopen(fileName.c_str(), "r");
	if (file != nullptr)
	{
		fclose(file);
		if (remove(fileName.c_str()) == 0)
		{
			return "HTTP/1.1 200 OK\r\n";
		}
		else
		{
			return "HTTP/1.1 500 Internal Server Error\r\n";
		}
	}
	else
	{
		return "HTTP/1.1 404 Not Found\r\n";
	}
}

string TRACE_f(char* allContent)
{
	string returnMessage = "HTTP/1.1 200 OK\r\n";
	returnMessage +=MakeHeaderSend();
	
	string strContent(allContent);

	returnMessage += Make_sizeOfBodyMessage(strContent);
	returnMessage += "\r\n\n";

	returnMessage += strContent;
	
	return returnMessage;
}

void ConvertHttpRequest(char* recvBuff, string& request)
{
	string tempBuff(recvBuff);

	if (tempBuff.find('\n') == string::npos)
	{
		tempBuff = "\0";
		request = tempBuff;
		return;
	}

	request = tempBuff.substr(0, tempBuff.find('\n'));
}

string HEAD_f(string request)
{
	string body = GetFileContent(request);
	string messageStatus = Make_start_header(body);

	messageStatus += MakeHeaderSend();

	return messageStatus + "0\r\n\n";

}

void getMethodTypeFromHttpRequest(string& recvRequest, int& subType)
{
	try
	{

	int res = -1;
	if (recvRequest.compare("Exit") == 0)
		res = EXIT;

	string requestType = recvRequest.substr(0, recvRequest.find(' '));
	
	if (requestType.compare("GET") == 0)
		res = GET;

	else if (requestType.compare("PUT") == 0)
		res = PUT;

	else if (requestType.compare("DELETE") == 0)
		res = _DELETE;

	else if (requestType.compare("OPTIONS") == 0)
		res = OPTIONS;

	else if (requestType.compare("HEAD") == 0)
		res = HEAD;

	else if (requestType.compare("TRACE") == 0)
		res = TRACE;

	subType = res;

	}
	catch (const char* exp)
	{
		subType = ERROR_REQUEST;
	}
}

