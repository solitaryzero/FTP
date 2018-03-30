#ifndef SERVERDATABASE_H
#define SERVERDATABASE_H

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include "../common.h"

using namespace std;

class ServerDatabase{
public:
    ServerDatabase();
    ~ServerDatabase();

    map<string, string> userData;
    map<string, vector<string>> friendshipData;
    map<string, bool> isOnline;

    void init();
    void save();
    int createUser(string name, string password);
    int checkUser(string name, string password);
    vector<string> findAllUsers();
    int addFriendship(string name1, string name2);
    bool isFriend(string name1, string name2);
    vector<string> findAllFriends(string name);
    bool doesUserExist(string name);
    void setOnlineState(string name, bool flag);
    bool getOnlineState(string name);
};

#endif