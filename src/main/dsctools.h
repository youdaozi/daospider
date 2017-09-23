#ifndef dsctools_h
#define dsctools_h

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


/*
作用：采集器curl的回掉函数
ptr:获取的数据
size:获取的数据大小
nmemb:获取的数据数量
stream:传入参数
*/
static size_t dsctools_curl_download_file_write_data_callback(void *ptr, size_t nsize, size_t nmemb, void *stream);

/*
作用：采集器curl的回掉函数
ptr:获取的数据
size:获取的数据大小
nmemb:获取的数据数量
stream:传入参数
*/
static size_t dsctools_curl_data_get_write_data_callback(void *ptr, size_t nsize, size_t nmemb, void *stream);

/*
文件下载
L:lua打开的库指针--lua调用c只有此参数，其他的为备忘
url:文件地址
desPath:目标地址
socket_connect_time_out:连接超时
socket_time_out:总连接时间
*/
static int dsctools_curl_download_file(lua_State* L);

/*
内容下载
L:lua打开的库指针--lua调用c只有此参数，其他的为备忘
url:文件地址
cookiePath:cookie路径
socket_connect_time_out:连接超时
socket_time_out:总连接时间
*/
static int dsctools_curl_data_get(lua_State* L);

/*
mysql数据插入
L:lua打开的库指针--lua调用c只有此参数，其他的为备忘
to_db_host:数据库host
to_db_username：数据库用户名
to_db_password：数据库密码
to_db_name：数据库名
to_db_port：数据库端口
sql:sql语句
*/
static int dsctools_mysql_insert(lua_State* L);

/**
 *  function MakeDir
 *  author PengWeizhe
 *  date
 *  param [in] path 待创建的目录路径 可以相对路径和绝对路径
 *  return 0 创建成功 1创建失败
 *  details 创建一个目录(单级、多级)
 L:lua打开的库指针--lua调用c只有此参数，其他的为备忘
 */
static int make_dir(lua_State* L);

/**
该C库的唯一入口函数。其函数签名等同于上面的注册函数。见如下几点说明：
1. 我们可以将该函数简单的理解为模块的工厂函数。
2. 其函数名必须为luaopen_xxx，其中xxx表示library名称。Lua代码require "xxx"需要与之对应。
3. 在luaL_register的调用中，其第一个字符串参数为模块名"xxx"，第二个参数为待注册函数的数组。
4. 需要强调的是，所有需要用到"xxx"的代码，不论C还是Lua，都必须保持一致，这是Lua的约定，否则将无法调用。
5. 此函数必须不能是static类型的，因为static类型的数据是对于外部隐藏的，会导致lua无法访问到（这个特性常常用于保护内部函数不受外部调用，就行add与sub函数一样）
*/
int luaopen_dsctools(lua_State* L);
#endif
