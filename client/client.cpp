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

/*
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
*/

int Client::startClient(){
    this->serverSock = connectServer(SERVER_PORT);
    string cmd;
    while (true){
        cin >> cmd;
        this->serverSock->sendMsg(cmd);
    }
    serverSock->shutDownSocket();
    delete serverSock;

    return 0;
}