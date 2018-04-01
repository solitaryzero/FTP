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
        taskLock.lock();
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
        taskLock.unlock();
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
        taskLock.lock();
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
        taskLock.unlock();
    });

    return 0;
}

void Server::catchClientSocket(TcpChatSocket* clientSock){
    threadMap[clientSock->socketid] = thread([=](){
        BinData inData;
        string msg,tmp;
        vector<string> filePaths;
        filePaths.clear();
        while (true){
            inData = clientSock->recvMsg();
            if (inData.size() == 0) break;
            msg = TcpChatSocket::binDataToString(inData);
            if (msg.length() > 4){
                tmp = msg.substr(0,4);
                
                if (tmp == "RETR"){
                    taskLock.lock();
                    tasks.push([=](){
                        string currentFilePath = "";
                        for (int i=0;i<filePaths.size();i++){
                            currentFilePath += filePaths[i]+"/";
                        }
                        string filePath = baseFilePath+currentFilePath+msg.substr(5);
                        sendFileTo(clientSock,filePath);
                    });
                    taskLock.unlock();
                } else if (tmp == "STOR"){
                    taskLock.lock();
                    tasks.push([=](){
                        string currentFilePath = "";
                        for (int i=0;i<filePaths.size();i++){
                            currentFilePath += filePaths[i]+"/";
                        }
                        string filePath = baseFilePath+currentFilePath+msg.substr(5);
                        cout << filePath << endl;
                        recvFileFrom(clientSock,filePath);
                    });
                    taskLock.unlock();
                } else if (tmp == "CWD "){
                    
                    string newfilePath = msg.substr(4);
                    if (newfilePath == ".."){
                        filePaths.pop_back();
                        clientSock->sendMsg(CWD_SUCCESS);
                    } else {
                        string currentFilePath = "";
                        for (int i=0;i<filePaths.size();i++){
                            currentFilePath += filePaths[i]+"/";
                        }
                        string filePath = baseFilePath+currentFilePath+newfilePath;
                        if (opendir(filePath.c_str())) {
                            filePaths.push_back(newfilePath);
                            clientSock->sendMsg(CWD_SUCCESS);
                        } else {
                            clientSock->sendMsg(CWD_FAILED);
                        }
                    }                      
                    
                } else if ((msg.length() > 5) && (msg.substr(0,5) == "MKDIR")){
                    taskLock.lock();
                    tasks.push([=](){
                        string currentFilePath;
                        char absPath[100];
                        getcwd(absPath,sizeof(absPath));
                        currentFilePath.assign(absPath);
                        currentFilePath += "/fileStorage/";
                        for (int i=0;i<filePaths.size();i++){
                            currentFilePath += filePaths[i]+"/";
                        }
                        string filePath = currentFilePath+msg.substr(6);
                        mkdir(filePath.c_str(),0777);
                    });
                    taskLock.unlock();
                }
            } else if (msg == "LIST"){
                taskLock.lock();
                tasks.push([=](){
                    string currentFilePath = baseFilePath;
                    for (int i=0;i<filePaths.size();i++){
                        currentFilePath += filePaths[i]+"/";
                    }  
                    cout << currentFilePath << endl; 
                    string result = "File list:\n";
                    DIR *dir;
                    struct dirent *ptr;
                    dir = opendir(currentFilePath.c_str());
                    while ((ptr=readdir(dir)) != NULL){
                        if (strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0) {
                            continue;
                        } else if(ptr->d_type == 4) {
                            result += "/"+std::string(ptr->d_name)+"\n";
                        } else {
                            result += std::string(ptr->d_name)+"\n";
                        }
                    }     
                    clientSock->sendMsg(result);
                });
                taskLock.unlock();
            } else if (msg == "PWD"){
                taskLock.lock();
                tasks.push([=](){
                    string currentFilePath = "./";
                    for (int i=0;i<filePaths.size();i++){
                        currentFilePath += filePaths[i]+"/";
                    }
                    clientSock->sendMsg("Current Path: "+currentFilePath);
                });
                taskLock.unlock();
            } else {
                clientSock->sendMsg(SYNTAX_ERROR);
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

    for (int i=0;i<MAXTHREADPOOL;i++){                  //构造线程池
        threadPool.push_back(thread([=](){
            while(true){
                taskLock.lock();
                if (!tasks.empty()){
                    function<void()> func = tasks.front();
                    tasks.pop();
                    func();
                }
                taskLock.unlock();
            }
        }));       
    }

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