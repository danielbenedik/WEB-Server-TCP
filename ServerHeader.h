#pragma once
#ifndef HTTP_PARSER
#define HTTP_PARSER

#include <stdio.h>
#include <winsock2.h>
#include <string.h>
#include <time.h>
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <fstream>
using namespace std;

const int TIME_PORT = 27015;
const int MAX_SOCKETS = 60;
const int EMPTY = 0;
const int LISTEN = 1;
const int RECEIVE = 2;
const int IDLE = 3;
const int SEND = 4;
const int GET = 5;
const int PUT = 6;
const int OPTIONS = 7;
const int TRACE = 8;
const int HEAD = 9;
const int _DELETE = 10;
const int EXIT = 11;
const int ERROR_REQUEST = 12;

struct SocketState
{
	SOCKET id;			// Socket handle
	int	recv;			// Receiving?
	int	send;			// Sending?
	int sendSubType;	// Sending sub-type
	char buffer[512];
	int len;
};

bool addSocket(SOCKET id, int what);
void removeSocket(int index);
void acceptConnection(int index);
void receiveMessage(int index);
void sendMessage(int index);

void getMethodTypeFromHttpRequest(string& recvRequest, int& subType);
void ConvertHttpRequest(char* recvBuff, string& request);
void MakeTheAnswer(int type, string& answer, string& request, char* content);
string GetFileContent(string request);
string GET_f(string request);
string MakeHeaderSend();
string HEAD_f(string request);
string OPTIONS_f();
string GetFileName(string request);
string TRACE_f(char* allContent);
string PUT_f(string request, char * allContent);
string deleteFile(string fileName);
string DELETE_f(string request);
string putToFile(string fileName, string Content);
string Make_start_header(const string& body);
string Make_sizeOfBodyMessage(const string& body);

#endif
