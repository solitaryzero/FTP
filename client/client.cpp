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
    if (port == SERVER_PORT){
        printf(">> connected to server\n");  
    } else if (port == FILE_SERVER_PORT){
        printf(">> connected to file server\n");  
    }
    newSock = new TcpChatSocket(socketfd);
    newSock->initSocket();
    return newSock;
}

void Client::tryRegister(){
    string name,password;
    printf(">> Trying to register...\n");

    printf(">> Enter your name: ");
    cin.getline(buf,BUFSIZE);
    buf[strlen(buf)] = '\0';
    name.assign(buf);

    printf(">> Enter your password: ");
    cin.getline(buf,BUFSIZE); 
    buf[strlen(buf)] = '\0';
    password.assign(buf);

    Json res = Json::object{
        {"Type",MSG_TYPE_REGISTER},
        {"Name",name},
        {"Password",password}
    };

    serverSock->sendMsg(res.dump());
}

void Client::tryLogin(){
    string name,password;
    printf(">> Trying to login...\n");

    printf(">> Enter your name: ");
    cin.getline(buf,BUFSIZE); 
    buf[strlen(buf)] = '\0';
    name.assign(buf);

    printf(">> Enter your password: ");
    cin.getline(buf,BUFSIZE); 
    buf[strlen(buf)] = '\0';
    password.assign(buf);

    Json res = Json::object{
        {"Type",MSG_TYPE_LOGIN},
        {"Name",name},
        {"Password",password}
    };

    serverFileSock->sendMsg(res.dump());    
    serverSock->sendMsg(res.dump());
}

void Client::tryChat(){
    string name;

    cout << ">> You want to chat with: ";
    cin.getline(buf,BUFSIZE);
    buf[strlen(buf)] = '\0';
    name.assign(buf);

    chatPartner = name;
    cout << ">> You are now chatting with: " << name << endl;

    auto iter = msgBuffer.find(name);
    if (iter == msgBuffer.end()){
        return;
    }

    if (msgBuffer[name].size() == 0) return;

    cout << OFFLINE_MSG_HINT << name << ":" << endl;
    for (int i=0;i<msgBuffer[name].size();i++){
        cout << name << ": " << msgBuffer[name][i] << endl;
    }
    msgBuffer[name].clear();
}

void Client::tryExitChat(){
    if (chatPartner == NO_NAME){
        cout << ">> You are not chatting with others." << endl;
        return;
    }
    cout << ">> exited chat with " << chatPartner << "." << endl;
    chatPartner = NO_NAME;
}

void Client::tryListUsers(){
    Json res = Json::object{
        {"Type",MSG_TYPE_LISTUSERS},
    };

    serverSock->sendMsg(res.dump());
}

void Client::tryListFriends(){
    Json res = Json::object{
        {"Type",MSG_TYPE_LISTFRIENDS},
    };

    serverSock->sendMsg(res.dump());
}

void Client::tryAddFriend(){
    string name; 

    cout << ">> Enter new friend's name: ";
    cin.getline(buf,BUFSIZE);
    buf[strlen(buf)] = '\0';
    name.assign(buf);
    Json res = Json::object{
        {"Type",MSG_TYPE_ADD_FRIEND},
        {"Name",name}
    };

    serverSock->sendMsg(res.dump());
}

void Client::tryProfile(){ 
    Json res = Json::object{
        {"Type",MSG_TYPE_PROFILE}
    };

    serverSock->sendMsg(res.dump());
}

void Client::sendMsg(){
    buf[strlen(buf)] = '\0';
    string msg(buf);

    if (chatPartner == NO_NAME){
        cout << ">> unknown command" << endl;
        return;
    }

    Json res = Json::object{
        {"Type",MSG_TYPE_STRINGMSG},
        {"Name",chatPartner},
        {"Content",msg}
    };

    serverSock->sendMsg(res.dump());
}

void Client::trySendFile(){
    string name; 
    int size;

    if (chatPartner == NO_NAME){
        cout << ">> You are not chatting with anyone." << endl;
        return;
    }

    cout << ">> Enter file name: ";
    
    cin.getline(buf,BUFSIZE);
    buf[strlen(buf)] = '\0';
    name.assign(buf);

    FILE* _file = fopen(buf,"rb");
    if (!_file) return;
    fseek(_file,0,SEEK_END);
    size = ftell(_file);    //确定文件大小
    fclose(_file); 
    
    Json fileHeader = Json::object{
        {"Type",MSG_TYPE_FILE_HEADER},
        {"FileName",name},
        {"Size",size},
        {"Dest",chatPartner}
    };
    serverFileSock->sendMsg(fileHeader.dump());

    _file = fopen(buf,"rb");
    int count = 0;
    while (count < size){
        int seg = fread(fileBuf,1,FILEBUFSIZE,_file);
        string binString;
        binString.assign(fileBuf,seg);
        Json fileBody = Json::object{
            {"Type",MSG_TYPE_FILE_BODY},
            {"FileName",name},
            {"Size",seg},
            {"Content",binString},
            {"Dest",chatPartner}
        };
        serverFileSock->sendMsg(fileBody.dump());
        count += seg;
    }

    Json fileEnd = Json::object{
        {"Type",MSG_TYPE_FILE_END},
        {"FileName",name},
        {"Size",size},
        {"Dest",chatPartner}
    };
    serverFileSock->sendMsg(fileEnd.dump());
}

void Client::tryRecvFile(){
    Json res = Json::object{
        {"Type",MSG_TYPE_GET_BUFFERED_FILE}
    };
    serverFileSock->sendMsg(res.dump());
}

int Client::startClient(){
    chatPartner = NO_NAME;
    this->serverSock = connectServer(SERVER_PORT);
    this->serverFileSock = connectServer(FILE_SERVER_PORT);

    //处理服务器端信息
    thread recvThread = thread([=](){
        bool flag = true;
        while(true){
            inData = serverSock->recvMsg();//接收服务器端信息  
            if (inData.size() == 0) break;

            string err;
            Json msg = Json::parse(inData.data(),err);
            int msgType = msg["Type"].int_value();

            switch(msgType){
                case MSG_TYPE_ERRORMSG:{
                    cout << ">> " << msg["Content"].string_value() << endl;
                    break;
                } 

                case MSG_TYPE_INFOMSG:{
                    cout << ">> " << msg["Content"].string_value() << endl;
                    break;
                } 

                case MSG_TYPE_LOGIN_SUCCESS_MSG:{
                    cout << ">> " << msg["Content"].string_value() << endl;
                    Json res = Json::object{
                        {"Type",MSG_TYPE_GET_BUFFERED_STRINGMSG}
                    };
                    serverSock->sendMsg(res.dump());    //获取离线信息
                    break;
                }

                case MSG_TYPE_STRINGMSG:{
                    string author = msg["Author"].string_value();
                    string content = msg["Content"].string_value();

                    if (author == chatPartner){
                        cout << ">> " << author << ": " << content << endl;
                    } else {
                        auto iter = msgBuffer.find(author);
                        if (iter == msgBuffer.end()){
                            vector<string> newVec;
                            msgBuffer[author] = newVec;
                        }
                        msgBuffer[author].push_back(content);
                    }
                    break;
                }

                case MSG_TYPE_LISTUSERS:{
                    cout << ">> " << "All users: " << endl;

                    for (int i=0;i<msg["Size"].int_value();i++){
                        cout << ">> " << msg["Content"][i]["Name"].string_value() << " ";
                        if (msg["Content"][i]["isOnline"].bool_value()){
                            cout << "(Online)" << endl;
                        } else {
                            cout << "(Offline)" << endl;
                        }
                    }
                    break;
                }

                case MSG_TYPE_LISTFRIENDS:{
                    cout << ">> " << "All friends: " << endl;

                    for (int i=0;i<msg["Size"].int_value();i++){
                        cout << ">> " << msg["Content"][i]["Name"].string_value() << " ";
                        if (msg["Content"][i]["isOnline"].bool_value()){
                            cout << "(Online)" << endl;
                        } else {
                            cout << "(Offline)" << endl;
                        }
                    }
                    break;
                }

                case MSG_TYPE_GET_BUFFERED_STRINGMSG:{
                    Json::array data = msg["Content"].array_items();
                    for (int i=0;i<data.size();i++){
                        string err;
                        string s = data[i].string_value();
                        Json tmp = Json::parse(s,err);

                        string author = tmp["Author"].string_value();
                        string content = tmp["Content"].string_value();

                        if (author == chatPartner){
                            cout << ">> " << author << ": " << content << endl;
                        } else {
                            auto iter = msgBuffer.find(author);
                            if (iter == msgBuffer.end()){
                                vector<string> newVec;
                                msgBuffer[author] = newVec;
                            }
                            msgBuffer[author].push_back(content);
                        }
                    }
                    break;
                }

                case MSG_TYPE_PROFILE:{
                    cout << ">> " << "Your profile: " << endl;
                    cout << ">> " << "Username: " << msg["Username"].string_value() << endl;
                    cout << ">> " << "Password: " << msg["Password"].string_value() << endl;
                    if (chatPartner == NO_NAME){
                        cout << ">> " << "Not chatting with anyone." << endl;
                    } else {
                        cout << ">> " << "Now chatting with: " << chatPartner << endl;
                    }
                    
                    break;
                }

                case MSG_TYPE_END_CONNECTION:{
                    flag = false;
                    break;
                }

                default:
                {
                    break;
                }
            }
        
            fflush(stdout);
            if (!flag) break;
        }
    });
      
    //接收文件服务端口信息
    thread recvFileThread = thread([=](){
        bool flag = true;
        while(true){
            inData = serverFileSock->recvMsg();//接收服务器端信息  
            if (inData.size() == 0) break;

            string err;
            Json msg = Json::parse(inData.data(),err);
            int msgType = msg["Type"].int_value();

            switch(msgType){
                case MSG_TYPE_ERRORMSG:{
                    cout << ">> " << msg["Content"].string_value() << endl;
                    break;
                } 

                case MSG_TYPE_INFOMSG:{
                    cout << ">> " << msg["Content"].string_value() << endl;
                    break;
                } 

                case MSG_TYPE_FILE_HEADER:{
                    string filename = msg["FileName"].string_value();
                    string author = msg["Author"].string_value();
                    int size = msg["Size"].int_value();
                    int count = 0; 

                    //需要修改为本机对应路径
                    string fullFilename = "/home/solitaryzero/Downloads/"+filename;
                    
                    currentFile = fopen(fullFilename.c_str(),"wb");
                    cout << ">> " << "Received file " << filename << " from " << author << "." << endl;
                    break;
                } 

                case MSG_TYPE_FILE_BODY:{
                    int size = msg["Size"].int_value();
                    string binMsg = msg["Content"].string_value();
                    fwrite(binMsg.c_str(),1,size,currentFile);
                    break;
                } 

                case MSG_TYPE_FILE_END:{
                    string filename = msg["FileName"].string_value();
                    cout << ">> " << "File " + filename << " successfully received." << endl;
                    fflush(currentFile);
                    fclose(currentFile);
                    break;
                } 

                case MSG_TYPE_END_CONNECTION:{
                    flag = false;
                    break;
                }

                default:
                {
                    break;
                }
            }
        
            fflush(stdout);
            if (!flag) break;
        }
    });

    //根据指令进行相应操作
    thread sendThread = thread([=](){
        while(true)  
        {  
            cin.getline(buf,BUFSIZE);
            if (strcmp(buf,"quit") == 0){
                break;  
            } else if (strcmp(buf,"register") == 0){
                tryRegister();
            } else if (strcmp(buf,"login") == 0){
                tryLogin();
            } else if (strcmp(buf,"chat") == 0){
                tryChat();
            } else if (strcmp(buf,"senfile") == 0){
                trySendFile();
            } else if (strcmp(buf,"recvfile") == 0){
                tryRecvFile();
            } else if (strcmp(buf,"search") == 0){
                tryListUsers();
            } else if (strcmp(buf,"ls") == 0){
                tryListFriends();
            } else if (strcmp(buf,"add") == 0){
                tryAddFriend();
            } else if (strcmp(buf,"exit") == 0){
                tryExitChat();
            } else if (strcmp(buf,"profile") == 0){
                tryProfile();
            } else {
                sendMsg();
            }
        }  
    });

    sendThread.join();
    serverSock->shutDownSocket();
    serverFileSock->shutDownSocket();
    recvThread.join();
    recvFileThread.join();
    delete serverSock;
    delete serverFileSock;

    return 0;
}