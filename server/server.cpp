#include "server.h"

#define BUFSIZE 100
#define MAX_USERS 10

int Server::sendMessageTo(string name, string content){
    if (!db.doesUserExist(name)){
        return USER_NOT_FOUND_ERROR;
    }

    auto iter = clientSocketMap.find(name);
    if ((iter == clientSocketMap.end()) || (clientSocketMap[name] == nullptr)){
        auto iter1 = msgBuffer.find(name);
        if (iter1 == msgBuffer.end()){
            vector<string> newVec;
            msgBuffer[name] = newVec;
        }
        msgBuffer[name].push_back(content);
    } else {
        clientSocketMap[name]->sendMsg(content);
    }
    return 0;
}

int Server::sendFileTo(string name, string content){
    if (!db.doesUserExist(name)){
        return USER_NOT_FOUND_ERROR;
    }

    auto iter = clientFileSocketMap.find(name);
    if ((iter == clientFileSocketMap.end()) || (clientFileSocketMap[name] == nullptr)){
        auto iter1 = fileBuffer.find(name);
        if (iter1 == fileBuffer.end()){
            vector<string> newVec;
            fileBuffer[name] = newVec;
        }
        fileBuffer[name].push_back(content);
    } else {
        clientFileSocketMap[name]->sendMsg(content);
    }
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
        //接收客户端的数据
        BinData inData;
        while (1){  
            inData = clientSock->recvMsg();
            if (inData.size() == 0) break;
            printf("%s\n",inData.data());

            taskLock.lock();            //加入处理信息事件
            tasks.push([=](){
                string err;
                Json msg = Json::parse(inData.data(),err);
                int msgType = msg["Type"].int_value();

                switch(msgType){
                    case MSG_TYPE_REGISTER:{
                        string name = msg["Name"].string_value();
                        string password = msg["Password"].string_value();
                        int res = db.createUser(name,password);
                        if (res == 1){
                            Json cont = Json::object{
                                {"Type", MSG_TYPE_ERRORMSG},
                                {"Content", USER_ALREADY_EXIST_ERROR_STRING}
                            };
                            clientSock->sendMsg(cont.dump());
                        } else {
                            Json cont = Json::object{
                                {"Type", MSG_TYPE_INFOMSG},
                                {"Content", REGISTER_SUCCESS_MSG}
                            };
                            clientSock->sendMsg(cont.dump());
                            db.save();
                        }
                        break;
                    } 

                    case MSG_TYPE_LOGIN:{
                        string name = msg["Name"].string_value();
                        string password = msg["Password"].string_value();
                        int res = db.checkUser(name,password);
                        if (res == 1){
                            Json cont = Json::object{
                                {"Type", MSG_TYPE_ERRORMSG},
                                {"Content", LOGIN_FAILED_MSG}
                            };
                            clientSock->sendMsg(cont.dump());
                        } else {
                            Json cont = Json::object{
                                {"Type", MSG_TYPE_LOGIN_SUCCESS_MSG},
                                {"Content", LOGIN_SUCCESS_MSG}
                            };
                            clientSock->sendMsg(cont.dump());
                            clientSock->name = name;
                            clientSocketMap[name] = clientSock;
                            db.setOnlineState(name,true);
                        }
                        break;
                    } 

                    case MSG_TYPE_STRINGMSG:{  
                        if (clientSock->name == NO_NAME){   //是否登录
                            Json cont = Json::object{
                                {"Type",MSG_TYPE_ERRORMSG},
                                {"Content",NOT_LOGINED_ERROR_STRING}
                            };
                            clientSock->sendMsg(cont.dump());
                            break;
                        }

                        string name = msg["Name"].string_value();
                        string content = msg["Content"].string_value();

                        if (!db.isFriend(clientSock->name,name)){   //是否为好友
                            Json cont = Json::object{
                                {"Type",MSG_TYPE_ERRORMSG},
                                {"Content",name+USER_NOT_FRIEND_ERROR_STRING}
                            };
                            clientSock->sendMsg(cont.dump());
                            break;
                        }

                        Json cont = Json::object{
                            {"Type",MSG_TYPE_STRINGMSG},
                            {"Author",clientSock->name},
                            {"Content",content}
                        };
                        int res = sendMessageTo(name,cont.dump());
                        if (res == USER_NOT_FOUND_ERROR){
                            cont = Json::object{
                                {"Type",MSG_TYPE_ERRORMSG},
                                {"Content",USER_NOT_FOUND_ERROR_STRING}
                            };
                            clientSock->sendMsg(cont.dump());
                        }
                        break;
                    }

                    case MSG_TYPE_LISTUSERS:{
                        vector<string> res = db.findAllUsers();
                        int len = res.size();
                        Json::array users;
                        for (int i=0;i<res.size();i++){
                            Json tmp = Json::object{
                                {"Name",res[i]},
                                {"isOnline",db.getOnlineState(res[i])}
                            };
                            users.push_back(tmp);
                        }
                        Json cont = Json::object{
                            {"Type",MSG_TYPE_LISTUSERS},
                            {"Size",len},
                            {"Content",users}
                        };
                        clientSock->sendMsg(cont.dump());
                        break;
                    }

                    case MSG_TYPE_LISTFRIENDS:{
                        if (clientSock->name == NO_NAME){
                            Json cont = Json::object{
                                {"Type",MSG_TYPE_ERRORMSG},
                                {"Content",NOT_LOGINED_ERROR_STRING}
                            };
                            clientSock->sendMsg(cont.dump());
                            break;
                        }

                        vector<string> res = db.findAllFriends(clientSock->name);
                        int len = res.size();
                        Json::array users;
                        for (int i=0;i<res.size();i++){
                            Json tmp = Json::object{
                                {"Name",res[i]},
                                {"isOnline",db.getOnlineState(res[i])}
                            };
                            users.push_back(tmp);
                        }
                        Json cont = Json::object{
                            {"Type",MSG_TYPE_LISTFRIENDS},
                            {"Size",len},
                            {"Content",users}
                        };
                        clientSock->sendMsg(cont.dump());
                        break;
                    }

                    case MSG_TYPE_ADD_FRIEND:{
                        if (clientSock->name == NO_NAME){
                            Json cont = Json::object{
                                {"Type",MSG_TYPE_ERRORMSG},
                                {"Content",NOT_LOGINED_ERROR_STRING}
                            };
                            clientSock->sendMsg(cont.dump());
                            break;
                        }

                        string friendName = msg["Name"].string_value();
                        if (!db.doesUserExist(friendName)){
                            Json cont = Json::object{
                                {"Type",MSG_TYPE_ERRORMSG},
                                {"Content",USER_NOT_FOUND_ERROR_STRING}
                            };
                            clientSock->sendMsg(cont.dump());
                        } else {
                            db.addFriendship(clientSock->name,friendName);
                            Json cont = Json::object{
                                {"Type",MSG_TYPE_INFOMSG},
                                {"Content",ADD_FRIEND_SUCCESS_MSG+friendName}
                            };
                            clientSock->sendMsg(cont.dump());
                            db.save();
                        }
                        
                        break;
                    }

                    case MSG_TYPE_GET_BUFFERED_STRINGMSG:{
                        vector<string> bufferedMsg = msgBuffer[clientSock->name];
                        msgBuffer[clientSock->name].clear();
                        Json cont = Json::object{
                            {"Type",MSG_TYPE_GET_BUFFERED_STRINGMSG},
                            {"Content",bufferedMsg}
                        };
                        clientSock->sendMsg(cont.dump());
                        break;
                    }
                    
                    case MSG_TYPE_PROFILE:{
                        if (clientSock->name == NO_NAME){
                            Json cont = Json::object{
                                {"Type",MSG_TYPE_ERRORMSG},
                                {"Content",NOT_LOGINED_ERROR_STRING}
                            };
                            clientSock->sendMsg(cont.dump());
                            break;
                        }

                        Json cont = Json::object{
                            {"Type",MSG_TYPE_PROFILE},
                            {"Username",clientSock->name},
                            {"Password",db.userData[clientSock->name]}
                        };
                        cout << cont.dump() << endl;
                        clientSock->sendMsg(cont.dump());
                        break;
                    }

                    default:
                    {
                        perror("unknown command");
                        break;
                    }
                }
            });
            taskLock.unlock();

        } 

        taskLock.lock();        //用户下线
        printf("socket No.%d closed\n", clientSock->socketid);
        tasks.push([=](){
            db.setOnlineState(clientSock->name,false);
            threadMap[clientSock->socketid].join();
            if (clientSock->name != NO_NAME){
                clientSocketMap.erase(clientSock->name);
            }
            threadMap.erase(clientSock->socketid);
            clientSock->shutDownSocket();
            delete clientSock;
        });
        taskLock.unlock();
    });
}

void Server::catchClientFileSocket(TcpChatSocket* clientSock){
    fileThreadMap[clientSock->socketid] = thread([=](){
        //接收客户端的文件数据
        BinData inData;
        while (1){  
            inData = clientSock->recvMsg();
            if (inData.size() == 0) break;

            taskLock.lock();            //加入处理信息事件
            tasks.push([=](){
                string err;
                Json msg = Json::parse(inData.data(),err);
                int msgType = msg["Type"].int_value();
                cout << "file socket msgType " << msgType << endl;

                switch(msgType){
                    case MSG_TYPE_LOGIN:{
                        string name = msg["Name"].string_value();
                        string password = msg["Password"].string_value();
                        int res = db.checkUser(name,password);
                        if (res == 0){
                            clientSock->name = name;
                            clientFileSocketMap[name] = clientSock;
                        }
                        break;
                    } 

                    case MSG_TYPE_FILE_HEADER:{
                        string filename = msg["FileName"].string_value();
                        int size = msg["Size"].int_value();
                        string dest = msg["Dest"].string_value();

                        Json fileHeader = Json::object{
                            {"Type",MSG_TYPE_FILE_HEADER},
                            {"Author",clientSock->name},
                            {"FileName",filename},
                            {"Size",size}
                        };
                        sendFileTo(dest,fileHeader.dump());
                        break;
                    }

                    case MSG_TYPE_FILE_BODY:{
                        string filename = msg["FileName"].string_value();
                        int size = msg["Size"].int_value();
                        string dest = msg["Dest"].string_value();
                        string binMsg = msg["Content"].string_value();

                        Json fileBody = Json::object{
                            {"Type",MSG_TYPE_FILE_BODY},
                            {"Author",clientSock->name},
                            {"FileName",filename},
                            {"Size",size},
                            {"Content",binMsg}
                        };
                        sendFileTo(dest,fileBody.dump());
                        break;
                    }

                    case MSG_TYPE_FILE_END:{
                        string filename = msg["FileName"].string_value();
                        string dest = msg["Dest"].string_value();
                        int size = msg["Size"].int_value();
                        Json fileEnd = Json::object{
                            {"Type",MSG_TYPE_FILE_END},
                            {"FileName",filename},
                            {"Size",size}
                        };
                        sendFileTo(dest,fileEnd.dump());
                        break;
                    }

                    case MSG_TYPE_GET_BUFFERED_FILE:{
                        vector<string> bufferedFile = fileBuffer[clientSock->name];
                        fileBuffer[clientSock->name].clear();
                        for (int i=0;i<bufferedFile.size();i++){
                            clientSock->sendMsg(bufferedFile[i]);
                        }
                        break;
                    }

                    default:
                    {
                        perror("unknown command");
                        break;
                    }
                }
            });
            taskLock.unlock();

        } 

        taskLock.lock();        //用户下线
        printf("file socket No.%d closed\n", clientSock->socketid);
        tasks.push([=](){
            db.setOnlineState(clientSock->name,false);
            fileThreadMap[clientSock->socketid].join();
            if (clientSock->name != NO_NAME){
                clientFileSocketMap.erase(clientSock->name);
            }
            fileThreadMap.erase(clientSock->socketid);
            clientSock->shutDownSocket();
            delete clientSock;
        });
        taskLock.unlock();
    });
}

//启动服务器
int Server::startServer(){
    //清空事件队列
    while (!tasks.empty()){     
        tasks.pop();
    }

    db.init();    //初始化数据库

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

    thread waitForSocketThread = thread([=](){          //聊天连接处理线程
        while(true){
            TcpChatSocket* clientSock;
            clientSock = waitForSocket();
            if (clientSock != nullptr) {
                taskLock.lock();
                tasks.push([=](){
                    catchClientSocket(clientSock);
                });
                taskLock.unlock();
            }
        }
    });

    thread waitForFileSocketThread = thread([=](){          //文件连接处理线程
        while(true){
            TcpChatSocket* clientSock;
            clientSock = waitForFileSocket();
            if (clientSock != nullptr) {
                taskLock.lock();
                tasks.push([=](){
                    catchClientFileSocket(clientSock);
                });
                taskLock.unlock();
            }
        }
    });
    
    waitForSocketThread.join();
    waitForFileSocketThread.join();

    serverSock->shutDownSocket();
    fileSock->shutDownSocket();
    delete(serverSock);
    delete(fileSock);

    return 0;
}