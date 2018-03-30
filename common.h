#ifndef COMMON_H
#define COMMON_H

#define SERVER_PORT 8001
#define FILE_SERVER_PORT 8002
#define FILEBUFSIZE 500

#define DB_FILENAME "db.txt"
#define FRIENDSHIP_DB_FILENAME "friendshipdb.txt"
#define NO_NAME "__undefined"
#define USER_NOT_FOUND_ERROR_STRING "user does not exist"
#define USER_NOT_FRIEND_ERROR_STRING " is not your friend."
#define NOT_LOGINED_ERROR_STRING "You are not logined!"
#define USER_ALREADY_EXIST_ERROR_STRING "user already exists!"
#define REGISTER_SUCCESS_MSG "register success!"
#define LOGIN_FAILED_MSG "login failed!"
#define LOGIN_SUCCESS_MSG "login success!"
#define ADD_FRIEND_SUCCESS_MSG "Successfully added friend: "
#define OFFLINE_MSG_HINT "History message from "

#define MSG_TYPE_LOGIN 1
#define MSG_TYPE_REGISTER 2
#define MSG_TYPE_STRINGMSG 3
#define MSG_TYPE_ERRORMSG 4
#define MSG_TYPE_INFOMSG 5
#define MSG_TYPE_LISTUSERS 6
#define MSG_TYPE_LISTFRIENDS 7
#define MSG_TYPE_GET_BUFFERED_STRINGMSG 8
#define MSG_TYPE_LOGIN_SUCCESS_MSG 9
#define MSG_TYPE_ADD_FRIEND 10
#define MSG_TYPE_END_CONNECTION 11
#define MSG_TYPE_PROFILE 12
#define MSG_TYPE_FILE_HEADER 13
#define MSG_TYPE_FILE_BODY 14
#define MSG_TYPE_FILE_END 15
#define MSG_TYPE_GET_BUFFERED_FILE 16

#define USER_NOT_FOUND_ERROR 1

#endif