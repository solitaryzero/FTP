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
#include "../common.h"

using namespace std;

class Server{
private:
    TcpChatSocket* serverSock;
    TcpChatSocket* fileSock;
    queue<function<void()>> tasks;
    map<int,thread> threadMap;
    map<int,thread> fileThreadMap;
    mutex taskLock;
    int nextSocketid;
    int nextFileSocketid;

    int sendMessageTo(TcpChatSocket* sock, string content);
    int sendFileTo(TcpChatSocket* sock, string content);
    TcpChatSocket* genServerSocket(int port);
    TcpChatSocket* waitForSocket();
    TcpChatSocket* waitForFileSocket();
    void catchClientSocket(TcpChatSocket* clientSock);
    void catchClientFileSocket(TcpChatSocket* clientSock);

public:
    int startServer();
};

#endif