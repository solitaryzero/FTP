#ifndef COMMON_H
#define COMMON_H

#define SERVER_PORT 8001
#define FILE_SERVER_PORT 8002
#define FILEBUFSIZE 4*1024
#define MAXPORT 30000
#define MAXTHREADPOOL 10
#define MAX_QUEUE 100

#define CONNECTION_ERROR "Error building connection!\n"
#define CONNECTION_SUCCESS "File transfer started...\n"
#define UPLOAD_COMPLETE "Upload Complete!\n"
#define DOWNLOAD_COMPLETE "Download Complete!\n"
#define FILE_NOT_FOUND "Requested file not found...\n"
#define SYNTAX_ERROR "Unknown command.\n"
#define CWD_SUCCESS "Directory changed successfully!\n"
#define CWD_FAILED "Directory change failed...\n"
#define DELETE_FILE_SUCCESS "Deleted file successfully!\n"
#define DELETE_FILE_FAILED "Delete file failed...\n"

#endif