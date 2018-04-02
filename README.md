# FTP
Try to implement FTP protocol with linux socket

# How to use
cmake ./
make
mkdir fileStorage
mkdir receivedFiles

Then you get ./Server and ./Client

# Result
Implements active mode of FTP protocol, available commands are:
RETR, STOR, CWD, PWD, LIST, MKD, RMD, DELE.
Uses std::thread to support multithread requirement, thread pool implemented.
Miscs that have been dealt with:
SIGPIPE, ECONNABORTED in accept(), SO_REUSEADDR, SO_LINGER, garbage thread collecting, killing idle sockets. 
Uploaded files are stored in ./fileStorage;
Downloaded files are stored in ./receivedFiles. 