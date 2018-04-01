#include "client.h"
#define baseFilePath "./receivedFiles/" 

TcpChatSocket* Client::connectServer(int port){
    int socketfd;
    int byteLength;
    struct sockaddr_in serverSockAddr;
    TcpChatSocket* newSock;
    memset(&serverSockAddr,0,sizeof(serverSockAddr));
    serverSockAddr.sin_family = AF_INET;
    serverSockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverSockAddr.sin_port = htons(port);

    if ((socketfd = socket(PF_INET,SOCK_STREAM,0)) < 0){
        perror("socket create error");
        return nullptr;
    }   
      
    /*将套接字绑定到服务器的网络地址上*/  
    if (connect(socketfd, (struct sockaddr *)&serverSockAddr, sizeof(struct sockaddr)) < 0){  
        perror("connect error");  
        return nullptr;  
    }  

    newSock = new TcpChatSocket(socketfd);
    newSock->initSocket();

    return newSock;
}

void Client::sendFile(string fileName){ 
    currentFile = fopen(fileName.c_str(),"rb");
    if (currentFile == NULL){
        cout << "invalid filename!" << endl;
        return;
    }
    this->serverSock->sendMsg("STOR "+fileName);

    TcpChatSocket* fileSocket;
    while (true){
        if (this->filePort != -1) {
            fileSocket = connectServer(this->filePort);
            if (fileSocket == nullptr) {
                this->filePort = -1;
                return;
            }
            break;
        }
        if (this->failed) return;
    }

    int seg;
    while ((seg = fread(fileBuf,1,FILEBUFSIZE,currentFile)) > 0){
        fileSocket->sendMsg(fileBuf,seg);
    }
    fileSocket->shutDownSocket();
    delete(fileSocket);
    this->filePort = -1;
}

void Client::recvFile(string fileName){ 
    string fullFileName = baseFilePath+fileName;
    currentFile = fopen(fullFileName.c_str(),"wb");
    this->serverSock->sendMsg("RETR "+fileName);

    TcpChatSocket* fileSocket;
    while (true){
        if (this->filePort != -1) {
            fileSocket = connectServer(this->filePort);
            if (fileSocket == nullptr) {
                this->filePort = -1;
                return;
            }
            break;
        }
        if (this->failed) return;
    }

    while (true){
        inData = fileSocket->recvMsg();
        if (inData.size() == 0) break;
        fwrite(inData.data(),1,inData.size(),currentFile);
    }
    fflush(currentFile);
    fclose(currentFile);
    fileSocket->shutDownSocket();
    delete(fileSocket);
    this->filePort = -1;
}

int Client::startClient(){
    this->filePort = -1;
    this->serverSock = connectServer(SERVER_PORT);
    this->failed = false;

    thread recvThread = thread([=](){
        BinData msgData;
        string msg;
        while (true){
            msgData = serverSock->recvMsg();
            msg = TcpChatSocket::binDataToString(msgData);
            if ((msg.length() > 5) && (msg.substr(0,4) == "PORT")){
                this->filePort = stoi(msg.substr(5));
            } else if (msg == FILE_NOT_FOUND){
                this->filePort = -1;
                this->failed = true;
            } else {
                cout << msgData.data() << endl;
            }
        }
    });

    thread sendThread = thread([=](){
        string cmd,tmp;
        while (true){
            cin.getline(this->cmdBuf,CMDBUFSIZE);
            cmd.assign(this->cmdBuf);
            if (cmd.length() > 4){
                tmp = cmd.substr(0,4);
                if (tmp == "RETR"){
                    recvFile(cmd.substr(5));
                } else if (tmp == "STOR"){
                    sendFile(cmd.substr(5));
                } else {
                    this->serverSock->sendMsg(cmd);
                }
            } else {
                this->serverSock->sendMsg(cmd);
            }
        }
    });

    recvThread.join();
    sendThread.join();
    serverSock->shutDownSocket();
    delete serverSock;

    return 0;
}