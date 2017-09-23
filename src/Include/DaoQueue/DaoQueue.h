#ifndef DaoQueue_h
#define DaoQueue_h

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

#ifndef HAS_DAOLIBS
/*引入DaoLibs库*/
#include "../DaoLibs/DaoLibs.h"
#define HAS_DAOLIBS
#endif // HAS_DAOLIBS

//自定义类库


//定义宏(宏的定义有利于人对于程序的阅读以及理解，在编译时程序会自动替换)


//由于要考虑到其他程序的使用，所以不能用宏简化结构
//#define MBlock struct DaoMPLMBlock
//#define Flag struct struct DaoMPLFlag

//定义各种结构体、共用体、枚举等
//队列结构
struct DaoQueueDate {
    char *data;//队列中的数据指向的内存地址
    int dataSize;//数据大小
};

//队列结构
struct DaoQueue {
    int index;//索引
    struct DaoQueueDate *data;//队列中的数据指向的DaoQueueDate内存地址
    int depth;//搜索引擎的深度（搜索队列的特殊标识，队列可以自定义标识）
    int updateStatus;//sqlite更新queue是否成功处理的状态.值为1和-1
    int totalSize;//队列大小
    /*此为必须申明*/
    struct DaoQueueFlag *queueFlag;//队列结构
};

//队列结构
struct DaoQueueFlag {
    int num;//队列数量
    //队列现在位置
    int nowSetIndex;
    int nowGetIndex;
    int maxIndex;
    struct DaoQueue **startQueue;//起始队列
};

/*
queueFlag:队列管理结构
queueNum:队列数量
queues:队列数组
queueDates：队列数据
*/
int dao_queue_init(struct DaoQueueFlag *queueFlag, int queueNum, struct DaoQueue **queues, struct DaoQueueDate **queueDates);

/*
基于内存池的队列注销
queueFlag:队列管理结构
*/
void dao_queue_exit(struct DaoQueueFlag *queueFlag);

/*
重置队列
queueFlag:队列管理结构
*/
int dao_queue_reset(struct DaoQueueFlag *queueFlag);

/*
线程安全压入数据
queueFlag:队列标识符
depth:深度
obj:所设置的数据到索引的字符串
*/
int dao_queue_set(struct DaoQueueFlag *queueFlag, int depth, char *obj);

/*
线程安全取出数据
queueFlag:队列标识符
queue:获取到的队列数据
*/
int dao_queue_get(struct DaoQueueFlag *queueFlag, struct DaoQueue **nowQueue);

/*
按下标压入数据
queueFlag:队列标识符
index:队列索引
depth:深度
obj:所设置的数据到索引的字符串
*/
int dao_queue_set_index(struct DaoQueueFlag *queueFlag, int index, int depth, char *obj);

/*
按下标取出数据
queueFlag:队列标识符
index:队列索引
queue:获取到的队列数据
*/
int dao_queue_get_index(struct DaoQueueFlag *queueFlag, int index, struct DaoQueue **nowQueue);
#endif
