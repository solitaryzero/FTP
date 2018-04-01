#include "server.h"

#define MAX_USERS 10
#define baseFilePath "./fileStorage/" 

//生成服务器对应的socket
TcpChatSocket* Server::genServerSocket(int port){
    int serverSocketfd;
    struct sockaddr_in serverSockAddr;
    memset(&serverSockAddr,0,sizeof(serverSockAddr));
    serverSockAddr.sin_family = AF_INET;
    serverSockAddr.sin_addr.s_addr = INADDR_ANY;
    serverSockAddr.sin_port = htons(port);

    if ((serverSocketfd = socket(PF_INET,SOCK_STREAM,0)) < 0){
        perror("socket create error");
        return nullptr;
    }

    TcpChatSocket* serverSock = new TcpChatSocket(serverSocketfd);
    serverSock->initSocket();

    if (bind(serverSocketfd,(struct sockaddr*)&serverSockAddr,sizeof(serverSockAddr)) < 0){
        perror("socket bind error");
        return nullptr;
    }

    if (listen(serverSocketfd,MAX_USERS) < 0){
        perror("listen error");
        return nullptr;
    }

    return serverSock;
}

//等待下一个用户socket
TcpChatSocket* Server::waitForSocket(){
    int clientSocketfd;
    struct sockaddr_in clientSockAddr;
    socklen_t sinSize = sizeof(struct sockaddr_in);
    if ((clientSocketfd = accept(serverSock->socketfd, (struct sockaddr*)&clientSockAddr, &sinSize)) < 0){
        perror("accept error");
        return nullptr;
    }

    TcpChatSocket* clientSock = new TcpChatSocket(clientSocketfd,nextSocketid);
    nextSocketid++;
    clientSock->initSocket();
    printf("accept client %s @port %d\n",inet_ntoa(clientSockAddr.sin_addr),ntohs(clientSockAddr.sin_port));  

    return clientSock;
}

TcpChatSocket* Server::waitForFileSocket(TcpChatSocket* fileSocket){
    int clientSocketfd;
    struct sockaddr_in clientSockAddr;
    socklen_t sinSize = sizeof(struct sockaddr_in);
    if ((clientSocketfd = accept(fileSocket->socketfd, (struct sockaddr*)&clientSockAddr, &sinSize)) < 0){
        perror("accept error");
        return nullptr;
    }

    TcpChatSocket* clientSock = new TcpChatSocket(clientSocketfd,nextSocketid);
    nextFileSocketid++;
    clientSock->initSocket();
    printf("accept file client %s @port %d\n",inet_ntoa(clientSockAddr.sin_addr),ntohs(clientSockAddr.sin_port));  

    return clientSock;
}

int Server::recvFileFrom(TcpChatSocket* sock, string filePath){
    TcpChatSocket* fileSocket = nullptr;
    int port;
    while (fileSocket == nullptr){
        port = rand()%MAXPORT + 10000;
        fileSocket = genServerSocket(port);
    }
    sock->sendMsg("PORT "+to_string(port));

    fileThreadMap[fileSocket->socketfd] = thread([=]{
        TcpChatSocket* clientSock = waitForFileSocket(fileSocket);
        tasks.push([=]{
            FILE* currentFile = fopen(filePath.c_str(),"wb");
            BinData inData;
            while (true){
                inData = clientSock->recvMsg();
                if (inData.size() == 0) break;
                fwrite(inData.data(),1,inData.size(),currentFile);
            }

            fileSocket->shutDownSocket();
            clientSock->shutDownSocket();
            fflush(currentFile);
            fclose(currentFile);
            //fileThreadMap.erase(fileSocket->socketfd);
            sock->sendMsg(UPLOAD_COMPLETE);
        });
    });

    return 0;
}

int Server::sendFileTo(TcpChatSocket* sock, string filePath){
    FILE* currentFile = fopen(filePath.c_str(),"rb");
    if (currentFile == NULL){
        perror("invalid filename!");
        sock->sendMsg(FILE_NOT_FOUND);
        return 1;
    }

    TcpChatSocket* fileSocket = nullptr;
    int port;
    while (fileSocket == nullptr){
        port = rand()%MAXPORT + 10000;
        fileSocket = genServerSocket(port);
    }
    sock->sendMsg("PORT "+to_string(port));

    fileThreadMap[fileSocket->socketfd] = thread([=]{
        TcpChatSocket* clientSock = waitForFileSocket(fileSocket);
        tasks.push([=]{
            sock->sendMsg(CONNECTION_SUCCESS);
            int seg;
            char fileBuf[FILEBUFSIZE];
            while ((seg = fread(fileBuf,1,FILEBUFSIZE,currentFile)) > 0){
                clientSock->sendMsg(fileBuf,seg);
            }
            fileSocket->shutDownSocket();
            clientSock->shutDownSocket();
            //fileThreadMap.erase(fileSocket->socketfd);

            sock->sendMsg(DOWNLOAD_COMPLETE);
        });
    });

    return 0;
}

void Server::catchClientSocket(TcpChatSocket* clientSock){
    threadMap[clientSock->socketid] = thread([=](){
        BinData inData;
        string msg,tmp;
        while (true){
            inData = clientSock->recvMsg();
            if (inData.size() == 0) break;
            msg = TcpChatSocket::binDataToString(inData);
            if (msg.length() < 4){
                clientSock->sendMsg("400");
                continue;
            }
            tmp = msg.substr(0,4);
            
            if (tmp == "RETR"){
                tasks.push([=](){
                    string filePath = baseFilePath+msg.substr(5);
                    sendFileTo(clientSock,filePath);
                });
            } else if (tmp == "STOR"){
                tasks.push([=](){
                    string filePath = baseFilePath+msg.substr(5);
                    recvFileFrom(clientSock,filePath);
                });
            }
            
        }
        printf("disconnected\n");
    });
}

//启动服务器
int Server::startServer(){
    srand(time(NULL));

    //清空事件队列
    while (!tasks.empty()){     
        tasks.pop();
    }

    threadMap.clear();
    fileThreadMap.clear();

    nextSocketid = 0;
    nextFileSocketid = 0;

    this->serverSock = genServerSocket(SERVER_PORT);   //生成服务器的socket

    thread taskThread = thread([=](){       //事件处理队列线程
        while(true){
            if (!tasks.empty()){
                taskLock.lock();
                function<void()> func = tasks.front();
                tasks.pop();
                taskLock.unlock();
                func();
            }
        }
    });

    thread waitForSocketThread = thread([=](){          //指令连接处理线程
        while(true){
            TcpChatSocket* clientSock;
            clientSock = waitForSocket();
            if (clientSock != nullptr) {
                catchClientSocket(clientSock);
            }
        }
    });
    
    waitForSocketThread.join();
    serverSock->shutDownSocket();
    delete(serverSock);

    return 0;
}