#include "client.h"

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

    struct sockaddr_in localAddr;
    unsigned int len = sizeof(localAddr);
    if (getsockname(socketfd, (struct sockaddr *)&localAddr, &len) < 0){
        perror("sockname error");  
        return nullptr;  
    }
    this->currentPort = ntohs(localAddr.sin_port);
    this->filePort = this->currentPort+1;

    if (port == SERVER_PORT){
        printf(">> connected to server\n");  
    } else if (port == FILE_SERVER_PORT){
        printf(">> connected to file server\n");  
    }
    newSock = new TcpChatSocket(socketfd);
    newSock->initSocket();

    return newSock;
}

TcpChatSocket* Client::connectFileServer(int port){
    int socketfd;
    int byteLength;
    struct sockaddr_in serverSockAddr;
    TcpChatSocket* newSock;
    memset(&serverSockAddr,0,sizeof(serverSockAddr));
    serverSockAddr.sin_family = AF_INET;
    serverSockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverSockAddr.sin_port = htons(port);

    struct sockaddr_in localAddr;
    memset(&localAddr,0,sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY;
    localAddr.sin_port = htons(this->filePort);

    if ((socketfd = socket(PF_INET,SOCK_STREAM,0)) < 0){
        perror("socket create error");
        return nullptr;
    }   

    /* 文件传输端口为连接端口 +1 */
    if (bind(socketfd,(struct sockaddr*)&localAddr,sizeof(localAddr)) < 0){
        perror("socket bind error");
        return nullptr;
    }

    /*将套接字绑定到服务器的网络地址上*/  
    if (connect(socketfd, (struct sockaddr *)&serverSockAddr, sizeof(struct sockaddr)) < 0){  
        perror("connect error");  
        return nullptr;  
    }  

    if (port == SERVER_PORT){
        printf(">> connected to server\n");  
    } else if (port == FILE_SERVER_PORT){
        printf(">> connected to file server\n");  
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
    TcpChatSocket* fileSocket = connectFileServer(FILE_SERVER_PORT);
    if (fileSocket == nullptr) return;

    int seg;
    string binString;
    while ((seg = fread(fileBuf,1,FILEBUFSIZE,currentFile)) > 0){
        binString.assign(fileBuf,seg);
        fileSocket->sendMsg(binString);
    }
}

int Client::startClient(){
    this->serverSock = connectServer(SERVER_PORT);
    string cmd,tmp;
    while (true){
        cin.getline(cmdBuf,CMDBUFSIZE);
        cmd.assign(cmdBuf);
        if (cmd.length() > 4){
            tmp = cmd.substr(0,4);
            if (tmp == "RETR"){

            } else if (tmp == "STOR"){
                sendFile(cmd.substr(5));
            }
        }
        this->serverSock->sendMsg(cmd);
    }
    serverSock->shutDownSocket();
    delete serverSock;

    return 0;
}