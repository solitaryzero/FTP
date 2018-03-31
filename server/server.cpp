#include "server.h"

#define BUFSIZE 100
#define MAX_USERS 10

int Server::sendMessageTo(TcpChatSocket* sock, string content){

    return 0;
}

int Server::sendFileTo(TcpChatSocket* sock, string content){

    return 0;
}

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
    printf("accept client %s\n",inet_ntoa(clientSockAddr.sin_addr));  

    return clientSock;
}

//等待下一个用户文件socket
TcpChatSocket* Server::waitForFileSocket(){
    int clientSocketfd;
    struct sockaddr_in clientSockAddr;
    socklen_t sinSize = sizeof(struct sockaddr_in);
    if ((clientSocketfd = accept(fileSock->socketfd, (struct sockaddr*)&clientSockAddr, &sinSize)) < 0){
        perror("accept error");
        return nullptr;
    }

    TcpChatSocket* clientSock = new TcpChatSocket(clientSocketfd,nextFileSocketid);
    nextFileSocketid++;
    clientSock->initSocket();
    printf("accept file client %s\n",inet_ntoa(clientSockAddr.sin_addr));  

    return clientSock;
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
                tmp = msg.substr(4);
                cout << "R " << tmp << endl;
            } else if (tmp == "STOR"){
                tmp = msg.substr(4);
                cout << "S " << tmp << endl;
            }
        }
        printf("disconnected\n");
    });
}

//启动服务器
int Server::startServer(){
    //清空事件队列
    while (!tasks.empty()){     
        tasks.pop();
    }

    threadMap.clear();
    fileThreadMap.clear();

    nextSocketid = 0;
    nextFileSocketid = 0;

    this->serverSock = genServerSocket(SERVER_PORT);   //生成服务器的socket
    this->fileSock = genServerSocket(FILE_SERVER_PORT);//生成文件服务器socket

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
    fileSock->shutDownSocket();
    delete(serverSock);
    delete(fileSock);

    return 0;
}