#ifndef DaoSpider_h
#define DaoSpider_h

//系统常用类库
//检测系统是否导入常见类库
#ifndef HAS_LIBS

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <math.h>

#define HAS_LIBS
#endif

#ifndef HAS_PCRE
/*引入HAS_PCRE库*/
#include <pcre.h>
#define HAS_DAOPCRE
#endif // HAS_PCRE

#ifndef HAS_SQLITE
/*引入sqlite3库*/
#include <sqlite3.h>
#define HAS_SQLITE
#endif // HAS_SQLITE

#ifndef HAS_MYSQL
/*引入mysql库*/
#include <mysql/mysql.h>
#define HAS_MYSQL
#endif // HAS_MYSQL

#ifndef HAS_CURL
/*引入curl库*/
#include <curl/curl.h>
#define HAS_CURL
#endif // HAS_CURL

#ifndef HAS_BDB
/*引入BerkeleyDB库*/
#include <db.h>
#define HAS_BDB
#endif // HAS_BDB

//自定义类库


//定义宏(宏的定义有利于人对于程序的阅读以及理解，在编译时程序会自动替换)
#define SUCCESS 0
#define FAILURE -1
#define TRUE 1
#define FALSE 0

//定义根目录(用于配置文件等的读取)
#define ROOTPATH "/usr/local/soft/daoscollection/"

//DBOperationQueue结构体-用于BerkeleyDB保存数据
struct BDBData {
    char url[1024];//url
    int depth;//深度
    int status;//状态 0未完成 1已完成
};

#endif
