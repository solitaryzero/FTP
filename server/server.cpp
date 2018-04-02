#include "server.h"

#define baseFilePath "./fileStorage/" 

int Server::Accept(int fd, struct sockaddr *sa, socklen_t *salenptr){   //防止ECONNABORTED终止程序
    int n;
    while (true){
        if ((n = accept(fd,sa,salenptr)) < 0){
            #ifndef EPROTO
                if ((errno == EPROTO) || (errno == ECONNABORTED))
            #else
                if (errno == ECONNABORTED)
            #endif
                continue;
                else {
                    return n;
                }
        } else {
            return n;
        }
    }
}

//生成服务器对应的socket
TcpChatSocket* Server::genServerSocket(int port, bool useTimeOut){
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
    serverSock->initSocket(useTimeOut);

    if (bind(serverSocketfd,(struct sockaddr*)&serverSockAddr,sizeof(serverSockAddr)) < 0){
        perror("socket bind error");
        return nullptr;
    }

    if (listen(serverSocketfd,MAX_QUEUE) < 0){
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
    if ((clientSocketfd = Accept(serverSock->socketfd, (struct sockaddr*)&clientSockAddr, &sinSize)) < 0){
        perror("accept error");
        return nullptr;
    }

    TcpChatSocket* clientSock = new TcpChatSocket(clientSocketfd,nextSocketid);
    nextSocketid++;
    clientSock->initSocket(true);
    printf("accept client %s @port %d\n",inet_ntoa(clientSockAddr.sin_addr),ntohs(clientSockAddr.sin_port));  

    return clientSock;
}

//在fileSocket上等待用户数据socket的连接
TcpChatSocket* Server::waitForFileSocket(TcpChatSocket* fileSocket){
    int clientSocketfd;
    struct sockaddr_in clientSockAddr;
    socklen_t sinSize = sizeof(struct sockaddr_in);
    if ((clientSocketfd = Accept(fileSocket->socketfd, (struct sockaddr*)&clientSockAddr, &sinSize)) < 0){
        perror("accept error");
        return nullptr;
    }

    TcpChatSocket* clientSock = new TcpChatSocket(clientSocketfd,nextSocketid);
    nextFileSocketid++;
    clientSock->initSocket(true);
    printf("accept file client %s @port %d\n",inet_ntoa(clientSockAddr.sin_addr),ntohs(clientSockAddr.sin_port));  

    return clientSock;
}

/*
    @brief:收取用户上传的文件
    @param:
        sock:用户的指令socket;
        filePath:相对文件路径;
*/
int Server::recvFileFrom(TcpChatSocket* sock, string filePath){
    TcpChatSocket* fileSocket = nullptr;
    int port;
    while (fileSocket == nullptr){
        port = rand()%MAXPORT + 10000;
        fileSocket = genServerSocket(port,true);
    }
    sock->sendMsg("PORT "+to_string(port));

    threadMap[fileSocket->socketfd] = thread([=]{
        TcpChatSocket* clientSock = waitForFileSocket(fileSocket);
        if (clientSock == nullptr){
            perror("accept file socket failed");
            return;
        }

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
            sock->sendMsg(UPLOAD_COMPLETE);
            garbageLock.lock();
            deadThreads.push(fileSocket->socketfd);
            garbageLock.unlock();
        });
        taskLock.unlock();
    });

    return 0;
}

/*
    @brief:向用户发送文件
    @param:
        sock:用户的指令socket;
        filePath:相对文件路径;
*/
int Server::sendFileTo(TcpChatSocket* sock, string filePath){
    FILE* currentFile = fopen(filePath.c_str(),"rb");
    if (currentFile == NULL){
        sock->sendMsg(FILE_NOT_FOUND);
        return 1;
    }

    TcpChatSocket* fileSocket = nullptr;
    int port;
    while (fileSocket == nullptr){
        port = rand()%MAXPORT + 10000;
        fileSocket = genServerSocket(port,true);
    }
    sock->sendMsg("PORT "+to_string(port));

    threadMap[fileSocket->socketfd] = thread([=]{
        TcpChatSocket* clientSock = waitForFileSocket(fileSocket);
        if (clientSock == nullptr){
            perror("accept file socket failed");
            return;
        }
        
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
            sock->sendMsg(DOWNLOAD_COMPLETE);

            garbageLock.lock();
            deadThreads.push(fileSocket->socketfd);
            garbageLock.unlock();
        });
        taskLock.unlock();
    });

    return 0;
}

/*
    @brief:对用户指令socket发来的信息进行处理
    @param:
        clientSock:用户的指令socket;
*/
void Server::catchClientSocket(TcpChatSocket* clientSock){
    threadMap[clientSock->socketfd] = thread([=](){
        BinData inData;
        string msg,tmp;
        vector<string> filePaths;
        mutex pathLock;
        int pos;
        filePaths.clear();
        while (true){
            inData = clientSock->recvMsg();
            if (inData.size() == 0) break;
            msg = TcpChatSocket::binDataToString(inData);
            pos = msg.find('\n');
            if (pos == -1) continue;
            msg = msg.substr(0,pos);
            if (msg.length() > 4){
                tmp = msg.substr(0,4);
                
                if (tmp == "RETR"){                 //下载文件
                    taskLock.lock();
                    tasks.push([=,&pathLock](){
                        string currentFilePath = "";
                        pathLock.lock();
                        for (int i=0;i<filePaths.size();i++){
                            currentFilePath += filePaths[i]+"/";
                        }
                        pathLock.unlock();
                        string filePath = baseFilePath+currentFilePath+msg.substr(5);
                        sendFileTo(clientSock,filePath);                       
                    });
                    taskLock.unlock();
                } else if (tmp == "STOR"){          //上传文件
                    taskLock.lock();
                    tasks.push([=,&pathLock](){
                        string currentFilePath = "";
                        pathLock.lock();
                        for (int i=0;i<filePaths.size();i++){
                            currentFilePath += filePaths[i]+"/";
                        }
                        pathLock.unlock();
                        string filePath = baseFilePath+currentFilePath+msg.substr(5);
                        recvFileFrom(clientSock,filePath);
                    });
                    taskLock.unlock();
                } else if (tmp == "CWD "){          //切换路径
                    taskLock.lock();
                    tasks.push([=,&filePaths,&pathLock](){
                        string newfilePath = msg.substr(4);
                        pathLock.lock();
                        if (newfilePath == ".."){
                            if (filePaths.size() == 0){
                                clientSock->sendMsg(CWD_FAILED);
                            } else {
                                filePaths.pop_back();
                                clientSock->sendMsg(CWD_SUCCESS);
                            }
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
                        pathLock.unlock();
                    });
                    taskLock.unlock();                  
                    
                } else if (msg.substr(0,4) == "MKD "){      //创建文件夹
                    taskLock.lock();
                    tasks.push([=,&pathLock](){
                        string currentFilePath;
                        char absPath[100];
                        getcwd(absPath,sizeof(absPath));
                        currentFilePath.assign(absPath);
                        currentFilePath += "/fileStorage/";
                        pathLock.lock();
                        for (int i=0;i<filePaths.size();i++){
                            currentFilePath += filePaths[i]+"/";
                        }
                        pathLock.unlock();
                        string filePath = currentFilePath+msg.substr(4);
                        mkdir(filePath.c_str(),0777);
                    });
                    taskLock.unlock();
                } else if (msg.substr(0,4) == "RMD "){      //删除文件夹
                    taskLock.lock();
                    tasks.push([=,&pathLock](){
                        string currentFilePath;
                        char absPath[100];
                        getcwd(absPath,sizeof(absPath));
                        currentFilePath.assign(absPath);
                        currentFilePath += "/fileStorage/";
                        pathLock.lock();
                        for (int i=0;i<filePaths.size();i++){
                            currentFilePath += filePaths[i]+"/";
                        }
                        pathLock.unlock();
                        string filePath = currentFilePath+msg.substr(4);
                        rmdir(filePath.c_str());
                    });
                    taskLock.unlock();
                } else if (msg.substr(0,4) == "DELE"){      //删除文件
                    taskLock.lock();
                    tasks.push([=,&pathLock](){
                        string currentFilePath;
                        currentFilePath = "";
                        pathLock.lock();
                        for (int i=0;i<filePaths.size();i++){
                            currentFilePath += filePaths[i]+"/";
                        }
                        pathLock.unlock();
                        string filePath = baseFilePath+currentFilePath+msg.substr(5);
                        if (remove(filePath.c_str()) < 0){
                            clientSock->sendMsg(DELETE_FILE_FAILED);
                        } else {
                            clientSock->sendMsg(DELETE_FILE_SUCCESS);
                        }
                    });
                    taskLock.unlock();
                }
            } else if (msg == "LIST"){              //列出文件列表
                taskLock.lock();
                tasks.push([=,&pathLock](){
                    string currentFilePath = baseFilePath;
                    pathLock.lock();
                    for (int i=0;i<filePaths.size();i++){
                        currentFilePath += filePaths[i]+"/";
                    }  
                    pathLock.unlock(); 
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
            } else if (msg == "PWD"){           //显示当前路径
                taskLock.lock();
                tasks.push([=,&pathLock](){
                    string currentFilePath = "./";
                    pathLock.lock();
                    for (int i=0;i<filePaths.size();i++){
                        currentFilePath += filePaths[i]+"/";
                    }
                    pathLock.unlock();
                    clientSock->sendMsg("Current Path: "+currentFilePath);
                });
                taskLock.unlock();
            } else {
                clientSock->sendMsg(SYNTAX_ERROR);
            }
            
        }
        garbageLock.lock();
        deadThreads.push(clientSock->socketfd);
        garbageLock.unlock();
    });
}

//启动服务器
int Server::startServer(){
    srand(time(NULL));

    //处理SIGPIPE
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, 0);

    while (!tasks.empty()){
        tasks.pop();
    }
    while (!deadThreads.empty()){
        deadThreads.pop();
    }
    threadMap.clear();

    nextSocketid = 0;
    nextFileSocketid = 0;

    this->serverSock = genServerSocket(SERVER_PORT,false);   //生成服务器的指令socket

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

    thread garbageCollectThread = thread([=](){         //过期线程清理线程
        while(true){
            garbageLock.lock();
            if (!deadThreads.empty()){
                int fd = deadThreads.front();
                threadMap[fd].join();
                threadMap.erase(fd);
                deadThreads.pop();
            }
            garbageLock.unlock();
        }
    });
    
    cout << "Server started." << endl;

    waitForSocketThread.join();
    garbageCollectThread.join();
    serverSock->shutDownSocket();
    delete(serverSock);

    return 0;
}