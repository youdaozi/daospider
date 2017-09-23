#ifndef DaoIconv_h
#define DaoIconv_h

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
//#include <pcre.h>
//#include <sqlite3.h>
//#include <iconv.h>
//#include <mysql.h>
#include <math.h>
#include <fcntl.h>

#define HAS_LIBS
#endif

#ifndef HAS_DAOLIBS
/*引入DaoLibs库*/
#include "../DaoLibs/DaoLibs.h"
#define HAS_DAOLIBS
#endif // HAS_DAOLIBS

#ifndef HAS_ICONV
/*引入iconv库*/
#include <iconv.h>
#define HAS_ICONV
#endif // HAS_ICONV
//自定义类库


//定义宏(宏的定义有利于人对于程序的阅读以及理解，在编译时程序会自动替换)


//由于要考虑到其他程序的使用，所以不能用宏简化结构
//#define MBlock struct DaoMPLMBlock
//#define Flag struct struct DaoMPLFlag

//定义各种结构体、共用体、枚举等


/*
作用：编码转换主要函数
from_charset:现在的编码
to_charset：转换为的编码
inbuf：输入字符串
inlen:inbuf中还需要编码转换的字节数
outbuf：输出字符串
outlen:outbuf中还可以存放转码的字节数，也就是outbuf中的剩余空间
*/
int code_convert(char *from_charset, char *to_charset, char *inbuf, size_t inlen, char *outbuf, size_t outlen);
/*
作用：编码转换utf8到gbk
inbuf：输入字符串
inlen:inbuf中还需要编码转换的字节数
outbuf：输出字符串
outlen:outbuf中还可以存放转码的字节数，也就是outbuf中的剩余空间
*/
int u2g(char *inbuf, size_t inlen, char *outbuf, size_t outlen);
/*
作用：编码转换gbk到utf8
inbuf：输入字符串
inlen:inbuf中还需要编码转换的字节数
outbuf：输出字符串
outlen:outbuf中还可以存放转码的字节数，也就是outbuf中的剩余空间
*/
int g2u(char *inbuf, size_t inlen, char *outbuf, size_t outlen);
#endif
