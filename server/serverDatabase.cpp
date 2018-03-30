#include "serverDatabase.h"

ServerDatabase::ServerDatabase(){

}

ServerDatabase::~ServerDatabase(){
    
}

void ServerDatabase::init(){
    ifstream in(DB_FILENAME);
    if (!in) return;

    int len;
    string name, password;
    in >> len;
    for (int i=0;i<len;i++){
        in >> name;
        in >> password;
        userData[name] = password;
        isOnline[name] = false;
    }
    in.close();

    ifstream fr_in(FRIENDSHIP_DB_FILENAME);
    if (!fr_in) return;

    int friendNum;
    string name1, name2;
    fr_in >> len;
    for (int i=0;i<len;i++){
        fr_in >> name1;
        fr_in >> friendNum;
        vector<string> newVec;
        for (int j=0;j<friendNum;j++){
            fr_in >> name2;
            newVec.push_back(name2);
        }
        friendshipData[name1] = newVec;
    }
    fr_in.close();
}

void ServerDatabase::save(){
    ofstream out(DB_FILENAME);
    int len = userData.size();
    out << len << endl;
    for (auto i=userData.begin();i!=userData.end();++i){
        out << i->first << endl;
        out << i->second << endl;
    }
    out.close();

    ofstream fr_out(FRIENDSHIP_DB_FILENAME);
    len = friendshipData.size();
    fr_out << len << endl;
    for (auto i=friendshipData.begin();i!=friendshipData.end();++i){
        fr_out << i->first << endl;
        fr_out << i->second.size() << endl;
        for (int j=0;j<i->second.size();j++){
            fr_out << i->second[j] << endl;
        }
    }
    fr_out.close();
}

int ServerDatabase::createUser(string name, string password){
    auto iter = userData.find(name);
    if (iter != userData.end()){
        return 1;
    }

    userData[name] = password;
    isOnline[name] = false;
    return 0;
}

int ServerDatabase::checkUser(string name, string password){
    auto iter = userData.find(name);
    if (iter == userData.end()){
        return 1;
    }

    if (userData[name] != password){
        return 1;
    }
    
    return 0;
}

vector<string> ServerDatabase::findAllUsers(){
    vector<string> res;
    for (auto i=userData.begin();i!=userData.end();++i){
        res.push_back(i->first);
    }
    return res;
}

int ServerDatabase::addFriendship(string name1, string name2){
    auto iter = friendshipData.find(name1);
    if (iter == friendshipData.end()){
        vector<string> newVec;
        friendshipData[name1] = newVec;
    }
    friendshipData[name1].push_back(name2);

    iter = friendshipData.find(name2);
    if (iter == friendshipData.end()){
        vector<string> newVec;
        friendshipData[name2] = newVec;
    }
    friendshipData[name2].push_back(name1);
}

bool ServerDatabase::isFriend(string name1, string name2){
    auto iter = friendshipData.find(name1);
    if (iter == friendshipData.end()){
        return false;
    }
    for (int i=0;i<friendshipData[name1].size();i++){
        if (friendshipData[name1][i] == name2){
            return true;
        }
    }
    return false;
}

vector<string> ServerDatabase::findAllFriends(string name){
    vector<string> res;
    auto iter = friendshipData.find(name);
    if (iter == friendshipData.end()){
        return res;
    }

    return friendshipData[name];
}

bool ServerDatabase::doesUserExist(string name){
    auto iter = userData.find(name);
    if (iter == userData.end()){
        return false;
    }
    
    return true;
}

void ServerDatabase::setOnlineState(string name, bool flag){
    isOnline[name] = flag;
}

bool ServerDatabase::getOnlineState(string name){
    return(isOnline[name]);
}