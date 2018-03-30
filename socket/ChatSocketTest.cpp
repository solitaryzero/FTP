#include <iostream>
#include <cstdlib>
#include "TcpChatSocket.h"

using namespace std;

int main(){
    TcpChatSocket tcs;
    tcs.initSocket();
    cout << "hello world!" << endl;
    return 0;
}