#ifndef Main_h
#define Main_h

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
#include "../Include/DaoLibs/DaoLibs.h"
#define HAS_DAOLIBS
#endif // HAS_DAOLIBS

#ifndef HAS_DAOICONV
/*引入DaoIconv库*/
#include "../Include/DaoIconv/DaoIconv.h"
#define HAS_DAOICONV
#endif // HAS_DAOICONV

#ifndef HAS_DAOQUEUE
/*引入DaoQueue库*/
#include "../Include/DaoQueue/DaoQueue.h"
#define HAS_DAOQUEUE
#endif // HAS_DAOLIBS

/*#ifndef HAS_DAOPCRE
引入HAS_DAOPCRE库
#include "../Include/DaoPCRE/DaoPCRE.h"
#define HAS_DAOPCRE
#endif // HAS_DAOPCRE*/

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

#ifndef HAS_DAOJSON
/*引入json库*/
#include "../Include/DaoJSON/cJSON.h"
#define HAS_DAOJSON
#endif // HAS_DAOJSON

#ifndef HAS_BDB
/*引入BerkeleyDB库*/
#include <db.h>
#define HAS_BDB
#endif // HAS_BDB

#ifndef HAS_DAOLUA
/*引入LUA库*/
#include "../Include/DaoLua/lua.h"
#include "../Include/DaoLua/lauxlib.h"
#include "../Include/DaoLua/lualib.h"
#define HAS_DAOLUA
#endif // HAS_DAOLUA

//自定义类库

//定义宏(宏的定义有利于人对于程序的阅读以及理解，在编译时程序会自动替换)
#define SUCCESS 0
#define FAILURE -1
#define TRUE 1
#define FALSE 0

//定义申请的固定堆空间大小
#define BIG_DATA_SIZE 1024*1024*10
#define SMALL_DATA_SIZE 1024*1
#define URL_DATA_SIZE 1024*1

//定义各种结构体、共用体、枚举等

//全局配置文件结构体
struct GlobalConfigData {
    char root_path[64];//根路径
    char db_config_file[32];//数据库配置文件路径
    char data_path[64];//数据路径
    int max_num_per_group;//一个分组中最多的元素个数，用于分组采集数据
    int thread_num;//程序启动的线程
    int queue_set_num;//程序启动的队列set数目
    int queue_get_num;//程序启动的队列get数目
    int to_set_num;//程序启动的目标URL set入库数目
    int to_get_num;//程序启动的目标URL get入库数目
};

//网络蜘蛛结构体
struct SpiderData {
    //id
    int id;
    //分组id
    int group_id;
    //规则名
    char rule_name[32];
    //编码
    char charset[32];
    //网址
    char start_url[URL_DATA_SIZE];
    //爬行深度
    int depth;
    //是否锁定起始url的域名，0不开启，1开启。开启后，只会获取起始url的域名的url而忽略其他任何url
    int is_lock_domain;
    //域名
    char domain[URL_DATA_SIZE];
    //每一次执行采集间隔时长us
    int per_interval_time;
    //socket连接超时
    int socket_connect_time_out;
    //socket数据获取超时s
    int socket_time_out;
    //url队列过滤正则
    char queue_url_filter_pattern[256];
    //目标url 过滤正则
    char to_url_filter_pattern[256];
    //采集器配置中处理后期大部分逻辑的lua脚本
    char scollection_config_lua_script[BIG_DATA_SIZE];
};

//配置文件共用体
union DBConfData{
    struct SpiderData SpiderData;
    //struct CollectionData CollectionData;
};

//配置文件结构体
struct CData {
    //全局配置
    struct GlobalConfigData GlobalConfigData;
    //确定类型的详细配置
    union DBConfData DBConfData;
};

//uri解析结构体-由于很小，所以直接用栈空间即可（栈空间速度快）
struct URIDetail {
    char protocol[8];
    char host[64];
    int port;
    char path[256];
    char query[512];
    char fragment[64];
};

//GRoupIdS-分组-获取各id辅助结构体
struct GRoupIdS {
    int *ids;//配置id存储的数组指针
    int i;//此时的id大小
};

//urlDo结构体-用于多线程获取内容
struct URLDo {
    int id;//顺序id,用于选择存储于什么全局变量
    int isBusy;//线程是否正在处理
    char url[URL_DATA_SIZE];//url
    int depth;//深度
    int *rt;//函数返回值
    pthread_t *pt;//线程返回值
    int isPthread;//是否是线程 1是 0不是
};

//curlUrlDo结构体-curl用于多线程获取内容
struct CURLURLDo {
    int dataSize;//数据大小
    struct URLDo *URLDo;//urlDo结构体-用于多线程获取内容
};

//ToDo结构体-用于多线程调用脚本采集内容
struct ToDo {
    int id;//顺序id,用于选择存储于什么全局变量
    int isBusy;//线程是否正在处理
    char url[URL_DATA_SIZE];//url
    int *rt;//函数返回值
    pthread_t *pt;//线程返回值
};

//curlUrlDo结构体-curl用于多线程获取内容
struct CURLToDo {
    int dataSize;//数据大小
    struct ToDo *ToDo;//ToDo结构体-用于多线程调用脚本采集内容
};

//DBOperationQueue结构体-用于执行队列式的数据库操作（主要是BerkeleyDB）
struct DBOperationQueue {
    int setFlag;//是否可以set
    int getFlag;//是否可以get
    int getDepth;//get时的深度
    int type;//0未开始 1url 2初始化完成 3初始化错误，退出 4重置并初始化
    int isEnd;//是否结束标识符
    int *ret;//返回值
};

//DBOperationQueue结构体-用于BerkeleyDB保存数据
struct BDBData {
    char url[URL_DATA_SIZE];//url
    int depth;//深度
    int status;//状态 0未完成 1已完成
};

//DBOperationQueue结构体-用于执行队列式的数据库操作（主要是BerkeleyDB）
struct DBOperationToDB {
    int setFlag;//是否可以set
    int getFlag;//是否可以get
    int type;//0未开始 1url 2初始化完成 3初始化错误，退出 4重置并初始化
    int isEnd;//是否结束标识符
    int *ret;//返回值
};

//DBOperationQueue结构体-用于BerkeleyDB保存数据
struct BDBTData {
    char url[URL_DATA_SIZE];//url
    int status;//状态 0未获取 1已获取
};

/*
作用：配置读取-全局配置文件
*/
static int global_config_get();

/*
作用：读取数据回调函数
data:回调传入参数
col_count:有多少字段
col_values:字段值
col_name:字段名
*/
static int select_spider_get_db_config_callback(void *data, int col_count, char **col_values, char **col_name);

/*
作用：配置读取-数据库配置读取
id：数据库配置id
*/
static int spider_db_config_get_by_id(int id);

/*
作用：采集器程序重置，用于采集分组
*/
int spider_reset();

/*
作用：网络蜘蛛程序初始化，申请各种所需空间
*/
static int spider_init();

/*
作用：网络蜘蛛程序结束，释放所有可用空间
*/
static int spider_destroy();

/*
作用：网络蜘蛛程序启动
*/
static int spider_start();

/*
作用：网络蜘蛛起始url的完整采集
url:要采集的网址
*/
static int spider_start_url_do(char *url);

/**
 *  function MakeDir
 *  author PengWeizhe
 *  date
 *  param [in] path 待创建的目录路径 可以相对路径和绝对路径
 *  return 0 创建成功 1创建失败
 *  details 创建一个目录(单级、多级)
 */
int make_dir(const char* path);

/*
作用：网络蜘蛛数据库操作队列
DBOperationQueueTmp:队列式数据库操作
*/
static void *spider_db_operation_queue(void *DBOperationQueueTmp);

/*
作用：网络蜘蛛数据库操作队列
DBOperationToDBTmp:队列式数据库操作
*/
static void *spider_db_operation_to_db(void *DBOperationToDBTmp);

/*
作用：网络蜘蛛获取某个深度的全部url并用回调函数启动url采集
depth:深度
*/
static int spider_queue_url_do(int depth);

/*
作用：取出url，脚本调用采集器
*/
static int spider_to_url_do();

/*
作用：网络蜘蛛启动一个url的完整采集
URLDoTmp:要采集的网址等结构体
*/
static void *spider_url_do(void *URLDoTmp);

/**
 * 初始化lua库
 */
static int lua_init(lua_State **L);

/**
 * 退出lua库
 */
static void lua_exit(lua_State *L);

/*
作用：采集器-内容处理by lua脚本
ToDoIn:要采集的网址等结构体
*/
static int spider_content_do_by_lua(struct ToDo *ToDoIn);

/*
作用：多线程采集器-调用脚本采集
ToDoTmp:要采集的网址等结构体
*/
static void *spider_to_do(void *ToDoTmp);

/*
作用：网络蜘蛛格式化网址使没有"/"后缀的url加上"/"后缀
url:要格式化的网址
*/
static int spider_format_url(char *url);

/*
作用：网址格式化，把大写变为小写,仅仅只是域名解析时使用
url:要格式化的网址
*/
static int url_to_lower(char *url);

/*
作用：解析url
url:要解析的网址
URI:URI对象
*/
static int parse_url(char *url, struct URIDetail *uri);

/*
作用：网络蜘蛛curl的回掉函数
ptr:获取的数据
size:获取的数据大小
nmemb:获取的数据数量
stream:传入参数
*/
static size_t spider_curl_write_data_callback(void *ptr, size_t nsize, size_t nmemb, void *stream);

/*
作用：网络蜘蛛从网络获取内容--curl库的调用，获取更加高效，且支持多协议不会报错
bOrl:判断是queue-url还是o-url，0是queue，1是to
DoIn:传入的对象，存储各种获取内容所需参数
*/
static int spider_curl_data_get(int bOrl, void *DoIn);

/*
作用：网络蜘蛛从网络获取内容
URLDoIn:urlDo对象，存储各种获取内容所需参数
*/
static int spider_data_get(struct URLDo *URLDoIn);

/*
获取url上一级目录
url:待获取url上一级目录的url
k:获取上一级目录层数
*/
static int get_dirname(char *url, int k);

/*
网络蜘蛛格式化入queue的url,检测匹配的链接是"/","a/index.html"还是"http://www.a.com/"，并把网址格式化为标准地址，如把"a/index.html"格式化为"http://www.b.com/a/index.html"
url:待格式化的queue_url
baseUrl：队列此时的基本url
*/
static int spider_format_queue_url(char *url, char *baseURL);

/*
网络蜘蛛检测url与start_url的domain是否相同
url:队列此时的基本url
*/
static int spider_url_is_include_domain(char *url);

/*
网络蜘蛛过滤出内容中的队列url并且url入库（入队列）
baseUrl:队列此时的基本url
URLDoIn:URLDoIn对象，存储各种获取内容所需参数
*/
static int spider_queue_set(char *baseURL, struct URLDo *URLDoIn);

/*
网络蜘蛛队列url的推理
queueUrl:判断的url
pattern:正则表达式
*/
static int spider_queue_url_filter(char *queueURL, char *pattern);

/*
网络蜘蛛queue_url入库
url:待入库的url
depth:队列深度
*/
static int spider_queue_url_to_queue(char *url, int depth);

/*
网络蜘蛛编码转换
bOrl:判断是queue-url还是o-url，0是queue，1是to
DoIn:DoIn对象，存储各种获取内容所需参数
*/
static int spider_charset_transfer(int bOrl, void *DoIn);

/*
网络蜘蛛目标url的推理
queueUrl:判断的url
pattern:正则表达式
*/
static int spider_to_url_filter(char *queueURL, char *pattern);

/*
网络蜘蛛目标url数据入库
url:目标入库url
*/
static int spider_to_url_to_db(char *url);

/*
作用：读取数据回调函数-gid
data:回调传入参数
col_count:有多少字段
col_values:字段值
col_name:字段名
*/
static int select_spider_get_db_by_gid_config_callback(void *data, int col_count, char **col_values, char **col_name);

/*
作用：采集器配置读取-数据库配置读取-gid
id：数据库配置id
gids：GRoupIdS-分组-获取各id辅助结构体的指针
*/
static int spider_db_config_get_by_gid(int id, struct GRoupIdS *gids);

/*
作用：网络蜘蛛执行主程序
type:类型，包括sid
id：具体的sid
返回值：成功success， 失败failure
*/
static int spider_main(char *type, int id);

#endif
