#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <fstream>
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

#include "../socket/TcpChatSocket.h"
#include "../common.h"

#define CMDBUFSIZE 100

using namespace std;

class Client{
private:
    TcpChatSocket* serverSock;
    TcpChatSocket* connectServer(int port);
    TcpChatSocket* connectFileServer(int port);

    int currentPort;
    int filePort;
    char cmdBuf[CMDBUFSIZE];
    char fileBuf[FILEBUFSIZE];
    BinData inData;
    FILE* currentFile;

    void sendFile(string fileName);
    void recvFile(string fileName);

public:
    int startClient();
};

#endif