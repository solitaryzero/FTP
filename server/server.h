#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <stdio.h>
#include <unistd.h>
#include <memory.h>
#include <string>
#include <vector>
#include <thread>
#include <queue>
#include <map>
#include <mutex>

#include "../socket/TcpChatSocket.h"
#include <json11.hpp>
#include "../common.h"
#include "serverDatabase.h"

using namespace std;
using namespace json11;

class Server{
private:
    TcpChatSocket* serverSock;
    TcpChatSocket* fileSock;
    queue<function<void()>> tasks;
    map<int,thread> threadMap;
    map<int,thread> fileThreadMap;
    map<string,TcpChatSocket*> clientSocketMap;
    map<string,TcpChatSocket*> clientFileSocketMap;
    map<string,vector<string>> msgBuffer;
    map<string,vector<string>> fileBuffer;
    ServerDatabase db; 
    mutex taskLock;
    int nextSocketid;
    int nextFileSocketid;

    int sendMessageTo(string name, string content);
    int sendFileTo(string name, string content);
    //int sendFileTo(string name, BinData data);
    TcpChatSocket* genServerSocket(int port);
    TcpChatSocket* waitForSocket();
    TcpChatSocket* waitForFileSocket();
    void catchClientSocket(TcpChatSocket* clientSock);
    void catchClientFileSocket(TcpChatSocket* clientSock);

public:
    int startServer();
};

#endif