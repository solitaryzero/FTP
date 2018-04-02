#include "TcpChatSocket.h"

using namespace std;

string TcpChatSocket::binDataToString(BinData input){
    string s(input.begin(),input.end());
    return s;
}

TcpChatSocket::TcpChatSocket(int sfd){
    socketfd = sfd;
    socketid = -1;
}

TcpChatSocket::TcpChatSocket(int sfd, int sid){
    socketfd = sfd;
    socketid = sid;
}

int TcpChatSocket::initSocket(bool useTimeOut){
    int reuseFlag = 1;
    setsockopt(socketfd,SOL_SOCKET,SO_REUSEADDR,&reuseFlag,sizeof(reuseFlag)); 

    struct linger lg;
    lg.l_onoff = true;
    lg.l_linger = 30;   //超时时间为30s
    setsockopt(socketfd, SOL_SOCKET, SO_LINGER, &lg, sizeof(lg));

    if (useTimeOut){
        int timeOutLimit = 120;
        setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, &timeOutLimit, sizeof(timeOutLimit));
        setsockopt(socketfd, SOL_SOCKET, SO_SNDTIMEO, &timeOutLimit, sizeof(timeOutLimit));
    }

    return 0;
}

int TcpChatSocket::sendMsg(string s){
    BinData dataOut;
    dataOut.resize(s.size()+1);
    char* pDst = &dataOut[0];
    memcpy(pDst,s.data(),s.size());
    pDst[s.size()] = '\0';
    if (send(socketfd,dataOut.data(),dataOut.size(),0) < 0){
        perror("send error");
        return 1;
    }
    return 0;
}

int TcpChatSocket::sendMsg(void* p, int len){
    BinData dataOut;
    dataOut.resize(len);
    char* pDst = &dataOut[0];
    memcpy(pDst,p,len);
    if (send(socketfd,dataOut.data(),dataOut.size(),0) < 0){
        perror("send error");
        return 1;
    }
    return 0;
}

int TcpChatSocket::sendMsg(BinData dataOut){
    if (send(socketfd,dataOut.data(),dataOut.size(),0) < 0){
        perror("send error");
        return 1;
    }
    return 0;
}

BinData TcpChatSocket::recvMsg(){
    BinData res;
    char buf[FILEBUFSIZE];
    int dataLength = recv(socketfd,buf,FILEBUFSIZE,0);
    if (dataLength <= 0){
        res.resize(0);
        return res;
    }

    res.resize(dataLength);
    memcpy(res.data(),buf,dataLength); 
    return res;
}

int TcpChatSocket::shutDownSocket(){
    shutdown(socketfd,2);
    return 0;
}