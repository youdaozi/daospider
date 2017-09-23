#include "dsctools.h"
#define LUA51

/*
作用：采集器curl的回掉函数
ptr:获取的数据
size:获取的数据大小
nmemb:获取的数据数量
stream:传入参数
*/
static size_t dsctools_curl_download_file_write_data_callback(void *ptr, size_t nsize, size_t nmemb, void *stream) {
    FILE *file = (FILE *)stream;
    if (!file) {
        printf("!!! No stream\n");
        return -1;
    }

    size_t written = fwrite((FILE*)ptr, nsize, nmemb, file);
    return written;
}

/*
作用：采集器curl的回掉函数
ptr:获取的数据
size:获取的数据大小
nmemb:获取的数据数量
stream:传入参数
*/
static size_t dsctools_curl_data_get_write_data_callback(void *ptr, size_t nsize, size_t nmemb, void *stream) {
    int flag;//标识符
    char *data=(char *)stream;

    memcpy(data, ptr, nsize*nmemb);

    return nsize*nmemb;//必须返回这个大小, 否则只回调一次, 不清楚为何.
}

/*
文件下载
L:lua打开的库指针--lua调用c只有此参数，其他的为备忘
url:文件地址
desPath:目标地址
socket_connect_time_out:连接超时
socket_time_out:总连接时间
*/
static int dsctools_curl_download_file(lua_State* L) {
    CURL *curl;
    CURLcode res;
    int flag;//标识符
    long httpCode = 0;

    //接收参数
    const char *url;
    const char *desPath;
    int socket_connect_time_out;
    int socket_time_out;
    int is_https;//是否是https 0不是 1是

    url = luaL_checkstring(L, 1);
    desPath = luaL_checkstring(L, 2);
    socket_connect_time_out = luaL_checknumber(L, 3);
    socket_time_out = luaL_checknumber(L, 4);
    is_https = luaL_checknumber(L, 5);

    FILE* fp = fopen(desPath, "wb");
    if (!fp) {
        printf("!!! Failed to create file on the disk\n");
        lua_pushnumber(L, -1);
        return 1;//必须是返回值的个数
    }

    //初始化curl库
    //curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if(!curl) {
        fclose(fp);//关闭文件
        //curl_global_cleanup();
        lua_pushnumber(L, -1);
        return 1;//必须是返回值的个数
    }

    curl_easy_setopt(curl, CURLOPT_URL, url); //设置下载地址
    if(is_https==1){
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    }
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L); //libcurl将不会产生任何信号,防止libcurl在多线程中crash
    curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1);//如果没有新的TCP请求来重用这个连接，那么只能等到CLOSE_WAIT超时，这个时间默认在7200秒甚至更高，太多的CLOSE_WAIT连接会导致性能问题
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)socket_time_out);//设置超时时间，获取数据等待
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, (long)socket_connect_time_out);//设置超时时间，连接等待
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dsctools_curl_download_file_write_data_callback);//设置写数据的函数
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);//设置写数据的变量

    res = curl_easy_perform(curl);//执行下载
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);//获取状态码

    if(res!=CURLE_OK) {
        printf("Download>>[%s]GETTING ERROR, res:%d!\n", url, res);
        fclose(fp);//关闭文件
        //清理url
        curl_easy_cleanup(curl);//释放curl资源
        //释放curl库
        //curl_global_cleanup();
        lua_pushnumber(L, -1);
        return 1;//必须是返回值的个数
    }

    if(httpCode!=200) {
        printf("Download>>[%s]%ld ERROR!\n", url, httpCode);
        fclose(fp);//关闭文件
        //清理url
        curl_easy_cleanup(curl);//释放curl资源
        //释放curl库
        //curl_global_cleanup();
        lua_pushnumber(L, -1);
        return 1;//必须是返回值的个数
    } else {
        fclose(fp);//关闭文件
        //关闭curl
        curl_easy_cleanup(curl);//释放curl资源
        //释放curl库
        //curl_global_cleanup();
        lua_pushnumber(L, 0);
        return 1;//必须是返回值的个数
    }
}

/*
内容下载
L:lua打开的库指针--lua调用c只有此参数，其他的为备忘
url:文件地址
cookiePath:cookie路径
socket_connect_time_out:连接超时
socket_time_out:总连接时间
*/
static int dsctools_curl_data_get(lua_State* L) {
    CURL *curl;
    CURLcode res;
    int flag;//标识符
    long httpCode = 0;

    //接收参数
    const char *url;
    const char *cookiePath;
    int socket_connect_time_out;
    int socket_time_out;
    int is_https;//是否是https 0不是 1是

    url = luaL_checkstring(L, 1);
    cookiePath = luaL_checkstring(L, 2);
    socket_connect_time_out = luaL_checknumber(L, 3);
    socket_time_out = luaL_checknumber(L, 4);
    is_https = luaL_checknumber(L, 5);

    char *data;
    data=(char *)malloc(sizeof(char)*BIG_DATA_SIZE);
    if(data==NULL) {
        for(; ; ) {
            data=(char *)malloc(sizeof(char)*BIG_DATA_SIZE);
            if(data!=NULL) {
                break;
            }
        }
    }

    struct curl_slist *cookiesList = NULL;//cookie
    struct curl_slist *nc;

    //初始化curl库
    //curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if(!curl) {
        free(data);
        //curl_global_cleanup();
        lua_pushnumber(L, -1);
        return 1;//必须是返回值的个数
    }

    curl_easy_setopt(curl, CURLOPT_URL, url); //设置下载地址
    //不兼容CURLOPT_COOKIEFILE， CURLOPT_COOKIEJAR等
    struct curl_slist *headers = NULL;
    //headers = curl_slist_append(headers,"User-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64; rv:48.0) Gecko/20100101 Firefox/48.0");
    //headers = curl_slist_append(headers,"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
    //headers = curl_slist_append(headers,"Accept-Language: zh-CN,zh;q=0.8,en-US;q=0.5,en;q=0.3");
    //headers = curl_slist_append(headers,"Connection: keep-alive");
    //headers = curl_slist_append(headers,"Cache-Control: max-age=0");

    headers = curl_slist_append(headers,"User-Agent: Googlebot/2.1 (+http://www.googlebot.com/bot.html)");

    if(is_https==1){
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L); //libcurl将不会产生任何信号,防止libcurl在多线程中crash
    curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1);//如果没有新的TCP请求来重用这个连接，那么只能等到CLOSE_WAIT超时，这个时间默认在7200秒甚至更高，太多的CLOSE_WAIT连接会导致性能问题
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);//抓取跳转页面
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)socket_time_out);//设置超时时间，获取数据等待
    curl_easy_setopt(curl, CURLOPT_COOKIEJAR, cookiePath);//设置cookie默认存储位置-存储
    curl_easy_setopt(curl, CURLOPT_COOKIEFILE, cookiePath);//设置cookie默认存储位置-取出使用
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, (long)socket_connect_time_out);//设置超时时间，连接等待
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dsctools_curl_data_get_write_data_callback);//设置写数据的函数
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);//设置写数据的变量

    res = curl_easy_perform(curl);//执行下载
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);//获取状态码
    curl_slist_free_all(headers);//释放headers

    if(res!=CURLE_OK) {
        printf("URL>>[%s]GETTING ERROR, res:%d!\n", url, res);
        //清理url
        curl_easy_cleanup(curl);//释放curl资源
        //curl_global_cleanup();
        free(data);
        lua_pushnumber(L, -1);
        return 1;//必须是返回值的个数
    }

    if(httpCode!=200) {
        printf("URL>>[%s]%ld ERROR!\n", url, httpCode);
        //清理url
        curl_easy_cleanup(curl);//释放curl资源
        //curl_global_cleanup();
        free(data);
        lua_pushnumber(L, -1);
        return 1;//必须是返回值的个数
    } else {
        //关闭curl
        curl_easy_cleanup(curl);//释放curl资源
        //curl_global_cleanup();
        lua_pushnumber(L, 0);
        lua_pushstring(L, data);
        free(data);
        return 2;//必须是返回值的个数
    }
}

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
static int dsctools_mysql_insert(lua_State* L) {
    MYSQL *mysql;
    MYSQL_RES *res;
    MYSQL_ROW row;
    int flag;//标识符
    char sqlLastInsertId[64];
    int lastInsertId;

    //接收参数
    const char *to_db_host;
    const char *to_db_username;
    const char *to_db_password;
    const char *to_db_name;
    int to_db_port;
    const char *sql;

    to_db_host = luaL_checkstring(L, 1);
    to_db_username = luaL_checkstring(L, 2);
    to_db_password = luaL_checkstring(L, 3);
    to_db_name = luaL_checkstring(L, 4);
    to_db_port = luaL_checknumber(L, 5);
    sql = luaL_checkstring(L, 6);

    int t,r;
    /*运行时间*/
    time_t timeNow;
    struct tm local;

    //初始化mysql
    mysql_thread_init();
    mysql=mysql_init(NULL);//初始化MYSQL标识符，用于连接
    if(mysql== NULL) {
        fprintf(stderr,"Mysql Error1:%s\n",mysql_error(mysql));
        mysql_thread_end();
        lua_pushnumber(L, -1);
        return 1;//必须是返回值的个数
    }

    //数据库连接
    if(!mysql_real_connect(mysql, to_db_host, to_db_username, to_db_password, to_db_name, to_db_port, NULL, 0)) {
        fprintf(stderr,"Mysql Error2:%s\n",mysql_error(mysql));
        mysql_close(mysql);//关闭数据库连接
        mysql_thread_end();
        lua_pushnumber(L, -1);
        return 1;//必须是返回值的个数
    }

    //时间获取
    timeNow=time( NULL );
    localtime_r(&timeNow, &local);//开始运行时间
	
	flag=mysql_query(mysql, "set names utf8");
    if(flag!=0) {
        fprintf(stderr,"Mysql Error4:%s\n",mysql_error(mysql));

        mysql_close(mysql);//关闭数据库连接
        mysql_thread_end();
        lua_pushnumber(L, -1);
        return 1;//必须是返回值的个数
    }
	
	flag=mysql_query(mysql, sql);
    if(flag!=0) {
        fprintf(stderr,"Mysql Error3:%s\n",mysql_error(mysql));

        mysql_close(mysql);//关闭数据库连接
        mysql_thread_end();
        lua_pushnumber(L, -1);
        return 1;//必须是返回值的个数
    }

    //获取插入id
    memset(sqlLastInsertId, 0, sizeof(sqlLastInsertId));
    snprintf(sqlLastInsertId, sizeof(sqlLastInsertId),  "%s", "select LAST_INSERT_ID();");
    flag=mysql_query(mysql, sqlLastInsertId);
    if(flag!=0) {
        fprintf(stderr,"Mysql Error5:%s\n",mysql_error(mysql));

        mysql_close(mysql);//关闭数据库连接
        mysql_thread_end();
        lua_pushnumber(L, -1);
        return 1;//必须是返回值的个数
    }
    res=mysql_store_result(mysql);
    row=mysql_fetch_row(res);
    lastInsertId=atoi(row[0]);
	mysql_free_result(res);

    mysql_close(mysql);//关闭数据库连接
    mysql_thread_end();
    lua_pushnumber(L, 0);
    lua_pushnumber(L, lastInsertId);
    return 2;//必须是返回值的个数
}

/**
 *  function MakeDir
 *  author PengWeizhe
 *  date
 *  param [in] path 待创建的目录路径 可以相对路径和绝对路径
 *  return 0 创建成功 1创建失败
 *  details 创建一个目录(单级、多级)
 L:lua打开的库指针--lua调用c只有此参数，其他的为备忘
 */
static int make_dir(lua_State* L) {
    //接收参数
    const char *path;
    path = luaL_checkstring(L, 1);

    int beginCmpPath;
    int endCmpPath;
    int fullPathLen;
    int pathLen = strlen(path);
    char currentPath[SMALL_DATA_SIZE] = {0};
    char fullPath[SMALL_DATA_SIZE] = {0};

    //相对路径
    if('/' != path[0]) {
        //获取当前路径
        getcwd(currentPath, sizeof(currentPath));
        strcat(currentPath, "/");
        //printf("currentPath = %s\n", currentPath);
        beginCmpPath = strlen(currentPath);
        strcat(currentPath, path);
        if(path[pathLen] != '/') {
            strcat(currentPath, "/");
        }
        endCmpPath = strlen(currentPath);

    } else {
        //绝对路径
        int pathLen = strlen(path);
        strcpy(currentPath, path);
        if(path[pathLen] != '/') {
            strcat(currentPath, "/");
        }
        beginCmpPath = 1;
        endCmpPath = strlen(currentPath);
    }
    //创建各级目录
    int i;
    for(i= beginCmpPath; i < endCmpPath ; i++ ) {
        if('/' == currentPath[i]) {
            currentPath[i] = '\0';
            if(access(currentPath, F_OK) != 0) {
                if(mkdir(currentPath, 0755) == -1) {
                    printf("currentPath = %s\n", currentPath);
                    perror("mkdir error %s\n");
                    lua_pushnumber(L, -1);
                    return 1;//必须是返回值的个数
                }
            }
            currentPath[i] = '/';
        }
    }
    lua_pushnumber(L, 0);
    return 1;//必须是返回值的个数
}

/**
该C库的唯一入口函数。其函数签名等同于上面的注册函数。见如下几点说明：
1. 我们可以将该函数简单的理解为模块的工厂函数。
2. 其函数名必须为luaopen_xxx，其中xxx表示library名称。Lua代码require "xxx"需要与之对应。
3. 在luaL_register的调用中，其第一个字符串参数为模块名"xxx"，第二个参数为待注册函数的数组。
4. 需要强调的是，所有需要用到"xxx"的代码，不论C还是Lua，都必须保持一致，这是Lua的约定，否则将无法调用。
5. 此函数必须不能是static类型的，因为static类型的数据是对于外部隐藏的，会导致lua无法访问到（这个特性常常用于保护内部函数不受外部调用，就行add与sub函数一样）
*/
int luaopen_dsctools(lua_State* L) {
    //const char *libName = "dsctools";
	/**
	luaL_Reg结构体的第一个字段为字符串，在注册时用于通知Lua该函数的名字。
	第一个字段为C函数指针。
	结构体数组中的最后一个元素的两个字段均为NULL，用于提示Lua注册函数已经到达数组的末尾。
	*/
	static const luaL_Reg methods[] = {
		{"make_dir", make_dir},
		{"dsctools_mysql_insert", dsctools_mysql_insert},
		{"dsctools_curl_data_get", dsctools_curl_data_get},
		{"dsctools_curl_download_file", dsctools_curl_download_file},
		{NULL, NULL}
	};
    //luaL_register(L, libName, methods); //第一种写法（已经不常用，有些地方已经作废）
    //luaL_openlib(L, libName, methods, 0); //第二种写法
	
#ifdef LUA51  
	//5.1 下直接使用luaL_register 就好  
    luaL_register(L, "dsctools", methods); 
#else //lua5.2  
    //先把一个table压入VS，然后在调用luaL_setfuncs就会把所以的func存到table中   
    //注意不像luaL_register这个table是个无名table，可以在的使用只用一个变量来存入这个table。   
    //e.g local clib = require "libname". 这样就不会污染全局环境。比luaL_register更好。
	lua_newtable(L);
	luaL_setfuncs(L, methods, 0);//第三种方法（lua5.1不可用） 
#endif 
    return 1;
}
