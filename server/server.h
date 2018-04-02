#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>  
#include <sys/stat.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <memory.h>
#include <signal.h>
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
    queue<function<void()>> tasks;
    queue<int> deadThreads;
    vector<thread> threadPool;
    map<int,thread> threadMap;
    mutex taskLock;
    mutex garbageLock;
    int nextSocketid;
    int nextFileSocketid;

    int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr);

    int recvFileFrom(TcpChatSocket* sock, string filePath);
    int sendFileTo(TcpChatSocket* sock, string filePath);
    TcpChatSocket* genServerSocket(int port, bool useTimeOut);
    TcpChatSocket* waitForSocket();
    TcpChatSocket* waitForFileSocket(TcpChatSocket* fileSocket);
    void catchClientSocket(TcpChatSocket* clientSock);

public:
    int startServer();
};

#endif