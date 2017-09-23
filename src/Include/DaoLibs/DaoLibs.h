#ifndef DaoLibs_h
#define DaoLibs_h

//系统常用类库
//检测系统是否导入常见类库
#ifndef HAS_LIBS

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
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
#include <fcntl.h>
#include <pthread.h>

#define HAS_LIBS
#endif

//自定义类库

//定义宏(宏的定义有利于人对于程序的阅读以及理解，在编译时程序会自动替换)
#define DAOTRUE 1
#define DAOFALSE 0
#define DAOSUCCESS 0
#define DAOFAILURE -1

//定义各种结构体、共用体、枚举等
//内存链结构
struct MemLink {
    int num;//内存链数量
    int index;//内存链索引
    struct MemLinkNode *memLinkNodes;//内存链节点
};

//内存链节点
struct MemLinkNode {
    void *p;//申请的变量指针
};

/*
作用：内存链初始化
memLinkFlag:内存链
num:内存链节点数量
*/
int dao_memory_link_init(struct MemLink *memLinkFlag, int num);

/*
作用：内存链增加节点
memLinkFlag:内存链
p:新增的内存指针
*/
void dao_memory_link_add(struct MemLink *memLinkFlag, void *p);

/*
作用：内存链退出
memLinkFlag:内存链
*/
void dao_memory_link_exit(struct MemLink *memLinkFlag);

#endif
