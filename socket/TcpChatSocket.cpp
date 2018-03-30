#include "TcpChatSocket.h"

#define BUFSIZE 65535

using namespace std;

string TcpChatSocket::binDataToString(BinData input){
    string s = input.data();
    return s;
}

TcpChatSocket::TcpChatSocket(int sfd){
    socketfd = sfd;
    socketid = -1;
    name = NO_NAME;
}

TcpChatSocket::TcpChatSocket(int sfd, int sid){
    socketfd = sfd;
    socketid = sid;
    name = NO_NAME;
}

int TcpChatSocket::initSocket(){
    bool flag = true;
    setsockopt(socketfd,SOL_SOCKET ,SO_REUSEADDR,(const char*)&flag,sizeof(bool)); 
    return 0;
}

int TcpChatSocket::sendMsg(string s){
    int len = s.size()+1;
    write(socketfd,&len,sizeof(int));

    BinData dataOut;
    dataOut.resize(s.size()+1);
    char* pDst = &dataOut[0];
    memcpy(pDst,s.data(),s.size());
    dataOut.at(s.size()) = '\0';
    if (write(socketfd,dataOut.data(),dataOut.size()) < 0){
        perror("send error");
        return 1;
    }
    return 0;
}

int TcpChatSocket::sendMsg(void* p, int len){
    write(socketfd,&len,sizeof(int));

    BinData dataOut;
    dataOut.resize(len);
    char* pDst = &dataOut[0];
    memcpy(pDst,p,len);
    if (write(socketfd,dataOut.data(),dataOut.size()) < 0){
        perror("send error");
        return 1;
    }
    return 0;
}

int TcpChatSocket::sendMsg(BinData dataOut){
    int len = dataOut.size();
    write(socketfd,&len,sizeof(int));
    if (write(socketfd,dataOut.data(),dataOut.size()) < 0){
        perror("send error");
        return 1;
    }
    return 0;
}

BinData TcpChatSocket::recvMsg(){
    BinData res;

    int byteLength;
    char lengthbuf[4];
    byteLength = recv(socketfd,lengthbuf,sizeof(int),MSG_WAITALL);
    if (byteLength <= 0){
        res.resize(0);
        return res;
    }
    int length = *((int*)lengthbuf);    //确认实际数据长度

    char buf[length];
    byteLength = recv(socketfd,buf,length,MSG_WAITALL);
    if (byteLength <= 0){
        perror("receive error");
        res.resize(0);
        return res;
    }

    res.resize(byteLength);
    memcpy(res.data(),buf,byteLength);  //接收实际数据
    return res;
}

int TcpChatSocket::shutDownSocket(){
    shutdown(socketfd,2);
    return 0;
}