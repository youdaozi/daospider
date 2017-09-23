#include "main.h"

//申请配置读取
struct CData *CData;

struct MemLink memLinkFlag;//内存链结构

//各种存储所需的大内存
char **data;
char **dataTmp;

/*网络蜘蛛-queue-get队列相关*/
char **queueGetObjs;//queue字符串数组指针
struct DaoQueueDate **queueGetDates;//队列所对应的数目
struct DaoQueue **queuesGet;//队列所对应的数目
struct DaoQueueFlag queueFlagGet;//队列管理结构

/*网络蜘蛛-queue-set队列相关*/
char **queueSetObjs;//queue字符串数组指针
struct DaoQueueDate **queueSetDates;//队列所对应的数目
struct DaoQueue **queuesSet;//队列所对应的数目
struct DaoQueueFlag queueFlagSet;//队列管理结构

/*网络蜘蛛-to-set队列相关*/
char **toSetObjs;//queue字符串数组指针
struct DaoQueueDate **toSetDates;//队列所对应的数目
struct DaoQueue **tosSet;//队列所对应的数目
struct DaoQueueFlag toFlagSet;//队列管理结构

/*网络蜘蛛-to-get队列相关*/
char **toGetObjs;//queue字符串数组指针
struct DaoQueueDate **toGetDates;//队列所对应的数目
struct DaoQueue **tosGet;//队列所对应的数目
struct DaoQueueFlag toFlagGet;//队列管理结构

/*运行时间*/
time_t timeStart;
time_t timeNow;
struct tm local;
time_t timeStop;
time_t time1;
time_t time2;

/*线程锁*/
pthread_mutex_t mutexURLDo;

pthread_t *pt;//定义线程数组
int *rt;//调用url_do函数后的返回值所在空间
struct URLDo *URLDo;//定义线程参数数组

pthread_t *toPt;//定义线程数组
int *toRt;//调用to_do函数后的返回值所在空间
struct ToDo *ToDo;//定义线程参数数组

struct DBOperationQueue DBOperationQueue;//定义队列式的数据库操作对象
pthread_t DBPt;//队列式数据库操作线程,线程handler
int DBRet;//返回值

struct DBOperationToDB DBOperationToDB;//定义队列式的数据库操作对象
pthread_t DBTPt;//队列式数据库操作线程,线程handler
int DBTRet;//返回值

//lua虚拟机池
lua_State **L;

/*
作用：配置读取-全局配置文件
*/
static int global_config_get() {
    FILE *stream;
    char buf[1024];
    char rootPath[128];//定义根目录

    //初始化字符串数组
    memset(buf, 0, sizeof(buf));
    memset(rootPath, 0, sizeof(rootPath));

    //赋值CData->GlobalConfigData.root_path,如果编译条件中赋值则按照编译中的编译路径来，否则使用默认路径
#ifdef	DSCROOTPATH
	snprintf(CData->GlobalConfigData.root_path, sizeof(CData->GlobalConfigData.root_path),  "%s", DSCROOTPATH);
#else
	snprintf(CData->GlobalConfigData.root_path, sizeof(CData->GlobalConfigData.root_path),  "%s", "/usr/local/soft/daoscollection/");
#endif
    
    snprintf(rootPath, sizeof(rootPath),  "%sconf/main.conf", CData->GlobalConfigData.root_path);
    if ((stream = fopen(rootPath, "r"))==NULL) {
        printf("ConfFile is not exist!\n");
        return FAILURE;
    }

    /*读取配置文件*/
    fread(buf, 1, 1024, stream);
    fclose(stream);

    cJSON *json;
    cJSON *data[8];

    // 解析数据包
    json = cJSON_Parse(buf);

    if (!json) {
        printf("Error before: [%s]\n",cJSON_GetErrorPtr());
        return FAILURE;
    } else {
        // 解析开关值
        data[0] = cJSON_GetObjectItem( json, "db_config_file");
        data[1] = cJSON_GetObjectItem( json, "data_path");
        data[2] = cJSON_GetObjectItem( json, "max_num_per_group");
        data[3] = cJSON_GetObjectItem( json, "thread_num");
        data[4] = cJSON_GetObjectItem( json, "queue_set_num");
        data[5] = cJSON_GetObjectItem( json, "queue_get_num");
        data[6] = cJSON_GetObjectItem( json, "to_set_num");
        data[7] = cJSON_GetObjectItem( json, "to_get_num");

        if( data[0]->type == cJSON_String ) {
            if(strlen(data[0]->valuestring)>=sizeof(CData->GlobalConfigData.db_config_file)) {
                return FAILURE;
            }
            snprintf(CData->GlobalConfigData.db_config_file, sizeof(CData->GlobalConfigData.db_config_file), "%s", data[0]->valuestring);
        }
        if( data[1]->type == cJSON_String ) {
            if(strlen(data[1]->valuestring)>=sizeof(CData->GlobalConfigData.data_path)) {
                return FAILURE;
            }
            snprintf(CData->GlobalConfigData.data_path, sizeof(CData->GlobalConfigData.data_path), "%s", data[1]->valuestring);
        }
        if( data[2]->type == cJSON_Number ) {
            if(data[2]->valueint>1024) {
                return FAILURE;
            }
            CData->GlobalConfigData.max_num_per_group=data[2]->valueint;
        }
        if( data[3]->type == cJSON_Number ) {
            if(data[3]->valueint<1) {
                return FAILURE;
            }
            CData->GlobalConfigData.thread_num=data[3]->valueint;
        }
        if( data[4]->type == cJSON_Number ) {
            if(data[4]->valueint<1) {
                return FAILURE;
            }
            CData->GlobalConfigData.queue_set_num=data[4]->valueint;
        }
        if( data[5]->type == cJSON_Number ) {
            if(data[5]->valueint<1) {
                return FAILURE;
            }
            CData->GlobalConfigData.queue_get_num=data[5]->valueint;
        }
        if( data[6]->type == cJSON_Number ) {
            if(data[6]->valueint<1) {
                return FAILURE;
            }
            CData->GlobalConfigData.to_set_num=data[6]->valueint;
        }
        if( data[7]->type == cJSON_Number ) {
            if(data[7]->valueint<1) {
                return FAILURE;
            }
            CData->GlobalConfigData.to_get_num=data[7]->valueint;
        }

        // 释放内存空间
        cJSON_Delete(json);
    }

    return SUCCESS;
}

/*
作用：读取数据回调函数
data:回调传入参数
col_count:有多少字段
col_values:字段值
col_name:字段名
*/
static int select_spider_get_db_config_callback(void *data, int col_count, char **col_values, char **col_name) {
    int i=0;
    int flag;//标识

    CData->DBConfData.SpiderData.id=atoi(col_values[i+0]);
    CData->DBConfData.SpiderData.group_id=atoi(col_values[i+1]);
    if(strlen(col_values[i+2])>=sizeof(CData->DBConfData.SpiderData.rule_name)) {
        return FAILURE;
    }
    snprintf(CData->DBConfData.SpiderData.rule_name, sizeof(CData->DBConfData.SpiderData.rule_name),  "%s", col_values[i+2]);
    if(strlen(col_values[i+3])>=sizeof(CData->DBConfData.SpiderData.start_url)) {
        return FAILURE;
    }
    snprintf(CData->DBConfData.SpiderData.start_url, sizeof(CData->DBConfData.SpiderData.start_url),  "%s", col_values[i+3]);
    if(strlen(col_values[i+4])>=sizeof(CData->DBConfData.SpiderData.charset)) {
        return FAILURE;
    }
    snprintf(CData->DBConfData.SpiderData.charset, sizeof(CData->DBConfData.SpiderData.charset),  "%s", col_values[i+4]);
    CData->DBConfData.SpiderData.depth=atoi(col_values[i+5]);
    CData->DBConfData.SpiderData.is_lock_domain=atoi(col_values[i+6]);
    if(strlen(col_values[i+7])>=sizeof(CData->DBConfData.SpiderData.domain)) {
        return FAILURE;
    }
    snprintf(CData->DBConfData.SpiderData.domain, sizeof(CData->DBConfData.SpiderData.domain),  "%s", col_values[i+7]);
    CData->DBConfData.SpiderData.per_interval_time=atoi(col_values[i+8]);
    CData->DBConfData.SpiderData.socket_connect_time_out=atoi(col_values[i+9]);
    CData->DBConfData.SpiderData.socket_time_out=atoi(col_values[i+10]);
    if(strlen(col_values[i+11])>=sizeof(CData->DBConfData.SpiderData.queue_url_filter_pattern)) {
        return FAILURE;
    }
    snprintf(CData->DBConfData.SpiderData.queue_url_filter_pattern, sizeof(CData->DBConfData.SpiderData.queue_url_filter_pattern),  "%s", col_values[i+11]);
    if(strlen(col_values[i+12])>=sizeof(CData->DBConfData.SpiderData.to_url_filter_pattern)) {
        return FAILURE;
    }
    snprintf(CData->DBConfData.SpiderData.to_url_filter_pattern, sizeof(CData->DBConfData.SpiderData.to_url_filter_pattern),  "%s", col_values[i+12]);
    if(strlen(col_values[i+13])>=sizeof(CData->DBConfData.SpiderData.scollection_config_lua_script)) {
        return FAILURE;
    }
    snprintf(CData->DBConfData.SpiderData.scollection_config_lua_script, sizeof(CData->DBConfData.SpiderData.scollection_config_lua_script),  "%s", col_values[i+13]);

    //return SUCCESS;
    return 0;
}

/*
作用：网络蜘蛛配置读取-数据库配置读取
id：数据库配置id
*/
static int spider_db_config_get_by_id(int id) {
    //新建sqlite结构体指针
    sqlite3 *db;
    int flag;//返回值标识
    char *errorMsg;//数据库错误信息

    //打开数据库
    char dbFile[256];
    memset(dbFile, 0, sizeof(dbFile));
    if(strlen(CData->GlobalConfigData.root_path)+strlen(CData->GlobalConfigData.db_config_file)+strlen("db/")+1>sizeof(dbFile)) {
        return FAILURE;
    }
    snprintf(dbFile, sizeof(dbFile),  "%sdb/%s", CData->GlobalConfigData.root_path, CData->GlobalConfigData.db_config_file);//设置文件路径
    flag=sqlite3_open(dbFile, &db);
    //是否打开判断
    if(flag != SQLITE_OK) {
        fprintf(stderr,"Can not open database\n:%s", sqlite3_errmsg(db));
        sqlite3_close(db);

        return FAILURE;
    }

    /*确定队列数目*/
    //sql语句
    char sql[256];
    //默认初始化
    memset(sql, 0, 256);
    snprintf(sql, 256,  "select * from config_spider where id=%d", id);//拼接sql字符串
    if(strlen(sql)>=256) {
        sqlite3_close(db);
        return FAILURE;
    }

    flag=sqlite3_exec(db, sql, select_spider_get_db_config_callback, NULL, &errorMsg);//执行查询
    if(flag != SQLITE_OK) {
        fprintf(stderr,"SQL Error:%s\n", errorMsg);
        sqlite3_free(errorMsg);//默认会申请内存，此处应该释放，否则会有内存泄露
        sqlite3_close(db);

        return FAILURE;
    }

    //关闭数据库释放资源
    sqlite3_close(db);
    return SUCCESS;
}

/*
作用：采集器程序重置，用于采集分组
*/
int spider_reset() {
    int flag;//标识符
    /*初始化各种数据*/
    int i;
    for(i=0; i<CData->GlobalConfigData.thread_num; i++) {
        memset(*(data+i), 0, sizeof(char)*BIG_DATA_SIZE);
        memset(*(dataTmp+i), 0, sizeof(char)*BIG_DATA_SIZE);
    }
    //把lua脚本写入lua文件
    char rootPath[128];
    char path[128];
    memset(rootPath, 0, sizeof(rootPath));
    memset(path, 0, sizeof(path));
    if(strlen(CData->GlobalConfigData.root_path)+strlen("tmp/luas/spider/")+12+1>sizeof(path)) {
        dao_memory_link_exit(&memLinkFlag);
        return FAILURE;
    }
    snprintf(path, sizeof(path),  "%stmp/luas/spider/%d/", CData->GlobalConfigData.root_path, CData->DBConfData.SpiderData.id);
    //创建路径
    if(access(path, F_OK)<0) {
        flag=make_dir(path);
        if(flag<0) {
            dao_memory_link_exit(&memLinkFlag);
            return FAILURE;
        }
    }
    snprintf(rootPath, sizeof(rootPath),  "%stmp/luas/spider/%d/lua_func_main.lua", CData->GlobalConfigData.root_path, CData->DBConfData.SpiderData.id);
    //生成文件
    FILE *fp;
    fp=fopen(rootPath, "w");
    fwrite(CData->DBConfData.SpiderData.scollection_config_lua_script, strlen(CData->DBConfData.SpiderData.scollection_config_lua_script), 1, fp);//生成lua文件
    fclose(fp);

    //队列式数据库操作线程初始化-queue
    DBOperationQueue.getDepth=1;
    DBOperationQueue.getFlag=0;
    DBOperationQueue.setFlag=0;
    DBOperationQueue.type=4;
    //等到数据库操作线程初始化完成
    for(; ; ) {
        if(DBOperationQueue.type==2) {
            DBOperationQueue.type=1;
            break;
        }
        if(DBOperationQueue.type==3) {
            printf ("db_operation_queue init error!\n");
            return FAILURE;
        }
    }

    //队列式数据库操作线程初始化-to
    DBOperationToDB.getFlag=0;
    DBOperationToDB.setFlag=0;
    DBOperationToDB.type=4;
    //等到数据库操作线程初始化完成
    for(; ; ) {
        if(DBOperationToDB.type==2) {
            DBOperationToDB.type=1;
            break;
        }
        if(DBOperationToDB.type==3) {
            printf ("db_operation_to_db init error!\n");
            return FAILURE;
        }
    }

    //初始化线程URLDo
    int k;
    for(k=0; k<CData->GlobalConfigData.thread_num; k++) {
        (URLDo+k)->isBusy=-1;
    }

    //初始化线程ToDo
    for(k=0; k<CData->GlobalConfigData.thread_num; k++) {
        (ToDo+k)->isBusy=-1;
    }

    return SUCCESS;
}

/*
作用：网络蜘蛛程序初始化，申请各种所需空间
*/
static int spider_init() {
    int flag;//标识符
    int num;

    //初始化内存链
    num=CData->GlobalConfigData.thread_num*6+6+CData->GlobalConfigData.queue_get_num*3+CData->GlobalConfigData.queue_set_num*3+CData->GlobalConfigData.to_get_num*3+CData->GlobalConfigData.to_set_num*3+1024;
    flag=dao_memory_link_init(&memLinkFlag, num);
    if(flag==DAOFAILURE) {
        return FAILURE;
    }

    /*初始化各种数据*/
    //线程数量的堆空间指针初始化
    data=(char **)malloc(sizeof(char *)*CData->GlobalConfigData.thread_num);
    if(data==NULL) {
        dao_memory_link_exit(&memLinkFlag);
        return FAILURE;
    }
    dao_memory_link_add(&memLinkFlag, data);//加载到内存链

    dataTmp=(char **)malloc(sizeof(char *)*CData->GlobalConfigData.thread_num);
    if(dataTmp==NULL) {
        dao_memory_link_exit(&memLinkFlag);
        return FAILURE;
    }
    dao_memory_link_add(&memLinkFlag, dataTmp);//加载到内存链

    memset(data, 0, sizeof(char *)*CData->GlobalConfigData.thread_num);
    memset(dataTmp, 0, sizeof(char *)*CData->GlobalConfigData.thread_num);

    int i;
    for(i=0; i<CData->GlobalConfigData.thread_num; i++) {
        *(data+i)=(char *)malloc(sizeof(char)*BIG_DATA_SIZE);
        if((data+i)==NULL) {
            dao_memory_link_exit(&memLinkFlag);
            return FAILURE;
        }
        dao_memory_link_add(&memLinkFlag, *(data+i));//加载到内存链

        *(dataTmp+i)=(char *)malloc(sizeof(char)*BIG_DATA_SIZE);
        if((dataTmp+i)==NULL) {
            dao_memory_link_exit(&memLinkFlag);
            return FAILURE;
        }
        dao_memory_link_add(&memLinkFlag, *(dataTmp+i));//加载到内存链

        memset(*(data+i), 0, sizeof(char)*BIG_DATA_SIZE);
        memset(*(dataTmp+i), 0, sizeof(char)*BIG_DATA_SIZE);
    }

    /*初始化queue-get队列*/
    queueGetObjs=(char **)malloc(sizeof(char *)*CData->GlobalConfigData.queue_get_num);
    if(queueGetObjs==NULL) {
        dao_memory_link_exit(&memLinkFlag);
        return FAILURE;
    }
    dao_memory_link_add(&memLinkFlag, queueGetObjs);//加载到内存链

    queueGetDates=(struct DaoQueueDate **)malloc(sizeof(struct DaoQueueDate *)*CData->GlobalConfigData.queue_get_num);
    if(queueGetDates==NULL) {
        dao_memory_link_exit(&memLinkFlag);
        return FAILURE;
    }
    dao_memory_link_add(&memLinkFlag, queueGetDates);//加载到内存链

    queuesGet=(struct DaoQueue **)malloc(sizeof(struct DaoQueue *)*CData->GlobalConfigData.queue_get_num);
    if(queuesGet==NULL) {
        dao_memory_link_exit(&memLinkFlag);
        return FAILURE;
    }
    dao_memory_link_add(&memLinkFlag, queuesGet);//加载到内存链

    for(i=0; i<CData->GlobalConfigData.queue_get_num; i++) {
        *(queueGetObjs+i)=(char *)malloc(sizeof(char)*SMALL_DATA_SIZE);
        if((queueGetObjs+i)==NULL) {
            dao_memory_link_exit(&memLinkFlag);
            return FAILURE;
        }
        dao_memory_link_add(&memLinkFlag, *(queueGetObjs+i));//加载到内存链

        *(queueGetDates+i)=(struct DaoQueueDate *)malloc(sizeof(struct DaoQueueDate)*1);
        if((queueGetDates+i)==NULL) {
            dao_memory_link_exit(&memLinkFlag);
            return FAILURE;
        }
        dao_memory_link_add(&memLinkFlag, *(queueGetDates+i));//加载到内存链

        *(queuesGet+i)=(struct DaoQueue *)malloc(sizeof(struct DaoQueue)*1);
        if((queuesGet+i)==NULL) {
            dao_memory_link_exit(&memLinkFlag);
            return FAILURE;
        }
        dao_memory_link_add(&memLinkFlag, *(queuesGet+i));//加载到内存链

        memset(*(queueGetObjs+i), 0, sizeof(char)*SMALL_DATA_SIZE);
        memset(*(queueGetDates+i), 0, sizeof(struct DaoQueueDate)*1);
        memset(*(queuesGet+i), 0, sizeof(struct DaoQueue)*1);

        //字符串存储空间赋值并绑定队列
        (*(queuesGet+i))->totalSize=SMALL_DATA_SIZE;
        (*(queueGetDates+i))->data=*(queueGetObjs+i);
        (*(queueGetDates+i))->dataSize=0;
    }
    //启动队列
    flag=dao_queue_init(&queueFlagGet, CData->GlobalConfigData.queue_get_num, queuesGet, queueGetDates);
    if(flag==DAOFAILURE) {
        dao_memory_link_exit(&memLinkFlag);
        return FAILURE;
    }

    /*初始化queue-set队列*/
    queueSetObjs=(char **)malloc(sizeof(char *)*CData->GlobalConfigData.queue_set_num);
    if(queueSetObjs==NULL) {
        dao_memory_link_exit(&memLinkFlag);
        return FAILURE;
    }
    dao_memory_link_add(&memLinkFlag, queueSetObjs);//加载到内存链

    queueSetDates=(struct DaoQueueDate **)malloc(sizeof(struct DaoQueueDate *)*CData->GlobalConfigData.queue_set_num);
    if(queueSetDates==NULL) {
        dao_memory_link_exit(&memLinkFlag);
        return FAILURE;
    }
    dao_memory_link_add(&memLinkFlag, queueSetDates);//加载到内存链

    queuesSet=(struct DaoQueue **)malloc(sizeof(struct DaoQueue *)*CData->GlobalConfigData.queue_set_num);
    if(queuesSet==NULL) {
        dao_memory_link_exit(&memLinkFlag);
        return FAILURE;
    }
    dao_memory_link_add(&memLinkFlag, queuesSet);//加载到内存链

    for(i=0; i<CData->GlobalConfigData.queue_set_num; i++) {
        *(queueSetObjs+i)=(char *)malloc(sizeof(char)*SMALL_DATA_SIZE);
        if((queueSetObjs+i)==NULL) {
            dao_memory_link_exit(&memLinkFlag);
            return FAILURE;
        }
        dao_memory_link_add(&memLinkFlag, *(queueSetObjs+i));//加载到内存链

        *(queueSetDates+i)=(struct DaoQueueDate *)malloc(sizeof(struct DaoQueueDate)*1);
        if((queueSetDates+i)==NULL) {
            dao_memory_link_exit(&memLinkFlag);
            return FAILURE;
        }
        dao_memory_link_add(&memLinkFlag, *(queueSetDates+i));//加载到内存链

        *(queuesSet+i)=(struct DaoQueue *)malloc(sizeof(struct DaoQueue)*1);
        if((queuesSet+i)==NULL) {
            dao_memory_link_exit(&memLinkFlag);
            return FAILURE;
        }
        dao_memory_link_add(&memLinkFlag, *(queuesSet+i));//加载到内存链

        memset(*(queueSetObjs+i), 0, sizeof(char)*SMALL_DATA_SIZE);
        memset(*(queueSetDates+i), 0, sizeof(struct DaoQueueDate)*1);
        memset(*(queuesSet+i), 0, sizeof(struct DaoQueue)*1);

        //字符串存储空间赋值并绑定队列
        (*(queuesSet+i))->totalSize=SMALL_DATA_SIZE;
        (*(queueSetDates+i))->data=*(queueSetObjs+i);
        (*(queueSetDates+i))->dataSize=0;
    }
    //启动队列
    flag=dao_queue_init(&queueFlagSet, CData->GlobalConfigData.queue_set_num, queuesSet, queueSetDates);
    if(flag==DAOFAILURE) {
        dao_memory_link_exit(&memLinkFlag);
        return FAILURE;
    }

    /*初始化to-set队列*/
    toSetObjs=(char **)malloc(sizeof(char *)*CData->GlobalConfigData.to_set_num);
    if(toSetObjs==NULL) {
        dao_memory_link_exit(&memLinkFlag);
        return FAILURE;
    }
    dao_memory_link_add(&memLinkFlag, toSetObjs);//加载到内存链

    toSetDates=(struct DaoQueueDate **)malloc(sizeof(struct DaoQueueDate *)*CData->GlobalConfigData.to_set_num);
    if(toSetDates==NULL) {
        dao_memory_link_exit(&memLinkFlag);
        return FAILURE;
    }
    dao_memory_link_add(&memLinkFlag, toSetDates);//加载到内存链

    tosSet=(struct DaoQueue **)malloc(sizeof(struct DaoQueue *)*CData->GlobalConfigData.to_set_num);
    if(tosSet==NULL) {
        dao_memory_link_exit(&memLinkFlag);
        return FAILURE;
    }
    dao_memory_link_add(&memLinkFlag, tosSet);//加载到内存链

    for(i=0; i<CData->GlobalConfigData.to_set_num; i++) {
        *(toSetObjs+i)=(char *)malloc(sizeof(char)*SMALL_DATA_SIZE);
        if((toSetObjs+i)==NULL) {
            dao_memory_link_exit(&memLinkFlag);
            return FAILURE;
        }
        dao_memory_link_add(&memLinkFlag, *(toSetObjs+i));//加载到内存链

        *(toSetDates+i)=(struct DaoQueueDate *)malloc(sizeof(struct DaoQueueDate)*1);
        if((toSetDates+i)==NULL) {
            dao_memory_link_exit(&memLinkFlag);
            return FAILURE;
        }
        dao_memory_link_add(&memLinkFlag, *(toSetDates+i));//加载到内存链

        *(tosSet+i)=(struct DaoQueue *)malloc(sizeof(struct DaoQueue)*1);
        if((tosSet+i)==NULL) {
            dao_memory_link_exit(&memLinkFlag);
            return FAILURE;
        }
        dao_memory_link_add(&memLinkFlag, *(tosSet+i));//加载到内存链

        memset(*(toSetObjs+i), 0, sizeof(char)*SMALL_DATA_SIZE);
        memset(*(toSetDates+i), 0, sizeof(struct DaoQueueDate)*1);
        memset(*(tosSet+i), 0, sizeof(struct DaoQueue)*1);

        //字符串存储空间赋值并绑定队列
        (*(tosSet+i))->totalSize=SMALL_DATA_SIZE;
        (*(toSetDates+i))->data=*(toSetObjs+i);
        (*(toSetDates+i))->dataSize=0;
    }
    //启动队列
    flag=dao_queue_init(&toFlagSet, CData->GlobalConfigData.to_set_num, tosSet, toSetDates);
    if(flag==DAOFAILURE) {
        dao_memory_link_exit(&memLinkFlag);
        return FAILURE;
    }

    /*初始化to-get队列*/
    toGetObjs=(char **)malloc(sizeof(char *)*CData->GlobalConfigData.to_get_num);
    if(toGetObjs==NULL) {
        dao_memory_link_exit(&memLinkFlag);
        return FAILURE;
    }
    dao_memory_link_add(&memLinkFlag, toGetObjs);//加载到内存链

    toGetDates=(struct DaoQueueDate **)malloc(sizeof(struct DaoQueueDate *)*CData->GlobalConfigData.to_get_num);
    if(toGetDates==NULL) {
        dao_memory_link_exit(&memLinkFlag);
        return FAILURE;
    }
    dao_memory_link_add(&memLinkFlag, toGetDates);//加载到内存链

    tosGet=(struct DaoQueue **)malloc(sizeof(struct DaoQueue *)*CData->GlobalConfigData.to_get_num);
    if(tosGet==NULL) {
        dao_memory_link_exit(&memLinkFlag);
        return FAILURE;
    }
    dao_memory_link_add(&memLinkFlag, tosGet);//加载到内存链

    for(i=0; i<CData->GlobalConfigData.to_get_num; i++) {
        *(toGetObjs+i)=(char *)malloc(sizeof(char)*SMALL_DATA_SIZE);
        if((toGetObjs+i)==NULL) {
            dao_memory_link_exit(&memLinkFlag);
            return FAILURE;
        }
        dao_memory_link_add(&memLinkFlag, *(toGetObjs+i));//加载到内存链

        *(toGetDates+i)=(struct DaoQueueDate *)malloc(sizeof(struct DaoQueueDate)*1);
        if((toGetDates+i)==NULL) {
            dao_memory_link_exit(&memLinkFlag);
            return FAILURE;
        }
        dao_memory_link_add(&memLinkFlag, *(toGetDates+i));//加载到内存链

        *(tosGet+i)=(struct DaoQueue *)malloc(sizeof(struct DaoQueue)*1);
        if((tosGet+i)==NULL) {
            dao_memory_link_exit(&memLinkFlag);
            return FAILURE;
        }
        dao_memory_link_add(&memLinkFlag, *(tosGet+i));//加载到内存链

        memset(*(toGetObjs+i), 0, sizeof(char)*SMALL_DATA_SIZE);
        memset(*(toGetDates+i), 0, sizeof(struct DaoQueueDate)*1);
        memset(*(tosGet+i), 0, sizeof(struct DaoQueue)*1);

        //字符串存储空间赋值并绑定队列
        (*(tosGet+i))->totalSize=SMALL_DATA_SIZE;
        (*(toGetDates+i))->data=*(toGetObjs+i);
        (*(toGetDates+i))->dataSize=0;
    }
    //启动队列
    flag=dao_queue_init(&toFlagGet, CData->GlobalConfigData.to_get_num, tosGet, toGetDates);
    if(flag==DAOFAILURE) {
        dao_memory_link_exit(&memLinkFlag);
        return FAILURE;
    }

    //初始化线程-urldo
    //申请空间
    pt=(pthread_t *)malloc(sizeof(pthread_t)*CData->GlobalConfigData.thread_num);
    if(pt==NULL) {
        dao_memory_link_exit(&memLinkFlag);
        return FAILURE;
    }
    dao_memory_link_add(&memLinkFlag, pt);//加载到内存链

    rt=(int *)malloc(sizeof(int)*CData->GlobalConfigData.thread_num);
    if(rt==NULL) {
        dao_memory_link_exit(&memLinkFlag);
        return FAILURE;
    }
    dao_memory_link_add(&memLinkFlag, rt);//加载到内存链

    URLDo=(struct URLDo *)malloc(sizeof(struct URLDo)*CData->GlobalConfigData.thread_num);
    if(URLDo==NULL) {
        dao_memory_link_exit(&memLinkFlag);
        return FAILURE;
    }
    dao_memory_link_add(&memLinkFlag, URLDo);//加载到内存链

    memset(pt, 0, sizeof(pthread_t)*CData->GlobalConfigData.thread_num);
    memset(rt, 0, sizeof(int)*CData->GlobalConfigData.thread_num);
    memset(URLDo, 0, sizeof(struct URLDo)*CData->GlobalConfigData.thread_num);

    int k;
    for(k=0; k<CData->GlobalConfigData.thread_num; k++) {
        //(URLDo+k)->id=k;
        (URLDo+k)->id=k;
        (URLDo+k)->isBusy=-1;
        (URLDo+k)->depth=0;
        memset((URLDo+k)->url, 0, sizeof((URLDo+k)->url));
        (URLDo+k)->rt=(rt+k);//借助此值作为返回值
        (URLDo+k)->pt=(pt+k);//线程返回值
        (URLDo+k)->isPthread=1;//是线程
        flag=pthread_create((pt+k), NULL, spider_url_do, (URLDo+k));
        if(flag!=0) {
            printf ("Create pthread error!\n");
            dao_memory_link_exit(&memLinkFlag);
            return FAILURE;
        }
    }

    //初始化线程-todo
    //申请空间
    toPt=(pthread_t *)malloc(sizeof(pthread_t)*CData->GlobalConfigData.thread_num);
    if(toPt==NULL) {
        dao_memory_link_exit(&memLinkFlag);
        return FAILURE;
    }
    dao_memory_link_add(&memLinkFlag, toPt);//加载到内存链

    toRt=(int *)malloc(sizeof(int)*CData->GlobalConfigData.thread_num);
    if(toRt==NULL) {
        dao_memory_link_exit(&memLinkFlag);
        return FAILURE;
    }
    dao_memory_link_add(&memLinkFlag, toRt);//加载到内存链

    ToDo=(struct ToDo *)malloc(sizeof(struct ToDo)*CData->GlobalConfigData.thread_num);
    if(ToDo==NULL) {
        dao_memory_link_exit(&memLinkFlag);
        return FAILURE;
    }
    dao_memory_link_add(&memLinkFlag, ToDo);//加载到内存链

    memset(toPt, 0, sizeof(pthread_t)*CData->GlobalConfigData.thread_num);
    memset(toRt, 0, sizeof(int)*CData->GlobalConfigData.thread_num);
    memset(ToDo, 0, sizeof(struct ToDo)*CData->GlobalConfigData.thread_num);

    for(k=0; k<CData->GlobalConfigData.thread_num; k++) {
        (ToDo+k)->id=k;
        (ToDo+k)->isBusy=-1;
        memset((ToDo+k)->url, 0, sizeof((ToDo+k)->url));
        (ToDo+k)->rt=(toRt+k);//借助此值作为返回值
        (ToDo+k)->pt=(toPt+k);//线程返回值
        flag=pthread_create((toPt+k), NULL, spider_to_do, (ToDo+k));
        if(flag!=0) {
            printf ("Create toPthread error!\n");
            dao_memory_link_exit(&memLinkFlag);
            return FAILURE;
        }
    }

    //队列式数据库操作线程-queue
    DBOperationQueue.getDepth=1;
    DBOperationQueue.getFlag=0;
    DBOperationQueue.setFlag=0;
    DBOperationQueue.type=0;
    DBOperationQueue.isEnd=0;
    DBOperationQueue.ret=&DBRet;
    flag=pthread_create(&DBPt, NULL, spider_db_operation_queue, &DBOperationQueue);
    if(flag!=0) {
        printf ("Create pthread error!\n");
        dao_memory_link_exit(&memLinkFlag);
        return FAILURE;
    }
    //等到数据库操作线程初始化完成
    for(; ; ) {
        if(DBOperationQueue.type==2) {
            DBOperationQueue.type=1;
            break;
        }
        if(DBOperationQueue.type==3) {
            printf ("db_operation_queue init error!\n");
            dao_memory_link_exit(&memLinkFlag);
            return FAILURE;
        }
    }

    //队列式数据库操作线程-to_db
    DBOperationToDB.getFlag=0;
    DBOperationToDB.setFlag=0;
    DBOperationToDB.type=0;
    DBOperationToDB.isEnd=0;
    DBOperationToDB.ret=&DBTRet;
    flag=pthread_create(&DBTPt, NULL, spider_db_operation_to_db, &DBOperationToDB);
    if(flag!=0) {
        printf ("Create pthread error!\n");
        dao_memory_link_exit(&memLinkFlag);
        return FAILURE;
    }
    //等到数据库操作线程初始化完成
    for(; ; ) {
        if(DBOperationToDB.type==2) {
            DBOperationToDB.type=1;
            break;
        }
        if(DBOperationToDB.type==3) {
            printf ("db_operation_to_db init error!\n");
            dao_memory_link_exit(&memLinkFlag);
            return FAILURE;
        }
    }

    //初始化线程
    pthread_mutex_init(&mutexURLDo, NULL);//锁机制初始化

    //初始化curl库
    curl_global_init(CURL_GLOBAL_ALL);

    //mysql
    mysql_library_init(0, NULL, NULL);

    //初始化lua
    L=(lua_State **)malloc(sizeof(lua_State *)*CData->GlobalConfigData.thread_num);
    dao_memory_link_add(&memLinkFlag, L);//加载到内存链

    for(k=0; k<CData->GlobalConfigData.thread_num; k++) {
        flag=lua_init((L+k));
        if(flag==FAILURE) {
            printf("lua_init Error!");
            dao_memory_link_exit(&memLinkFlag);
            return FAILURE;
        }
    }

    //生成lua文件
    char rootPath[128];
    char path[128];
    memset(rootPath, 0, sizeof(rootPath));
    memset(path, 0, sizeof(path));
    if(strlen(CData->GlobalConfigData.root_path)+strlen("tmp/luas/spider/")+12+1>sizeof(path)) {
        dao_memory_link_exit(&memLinkFlag);
        return FAILURE;
    }
    snprintf(path, sizeof(path),  "%stmp/luas/spider/%d/", CData->GlobalConfigData.root_path, CData->DBConfData.SpiderData.id);
    //创建路径
    if(access(path, F_OK)<0) {
        flag=make_dir(path);
        if(flag<0) {
            dao_memory_link_exit(&memLinkFlag);
            return FAILURE;
        }
    }
    snprintf(rootPath, sizeof(rootPath),  "%stmp/luas/spider/%d/lua_func_main.lua", CData->GlobalConfigData.root_path, CData->DBConfData.SpiderData.id);
    //生成文件
    FILE *fp;
    fp=fopen(rootPath, "wb+");
    fwrite(CData->DBConfData.SpiderData.scollection_config_lua_script, strlen(CData->DBConfData.SpiderData.scollection_config_lua_script), 1, fp);//生成lua文件
    fclose(fp);

    return SUCCESS;
}

/*
作用：网络蜘蛛程序结束，释放所有可用空间
*/
static int spider_destroy() {
    int i;
    int flag;//标识符

    //结束线程-urldo
    for(i=0; i<CData->GlobalConfigData.thread_num; i++) {
        for(; ; ) {
            if((URLDo+i)->isBusy==0) {
                //等待线程
                (URLDo+i)->isBusy=2;
                pthread_join(*(pt+i), NULL);
                if(*((URLDo+i)->rt)==FAILURE) {
                    printf("Pthread %u Ending ERROR!\n", *(pt+i));
                }

                break;
            }
        }
    }

    //结束线程-todo
    for(i=0; i<CData->GlobalConfigData.thread_num; i++) {
        for(; ; ) {
            if((ToDo+i)->isBusy==0) {
                //等待线程
                (ToDo+i)->isBusy=2;
                pthread_join(*(toPt+i), NULL);
                if(*((ToDo+i)->rt)==FAILURE) {
                    printf("toPthread %u Ending ERROR!\n", *(toPt+i));
                }

                break;
            }
        }
    }

    //结束db_operation_queue线程
    DBOperationQueue.isEnd=1;
    pthread_join(DBPt, NULL);
    if(*(DBOperationQueue.ret)==FAILURE) {
        printf("DBOperationQueue ERROR!\n");
    }

    //结束db_operation_to_db线程
    DBOperationToDB.isEnd=1;
    pthread_join(DBTPt, NULL);
    if(*(DBOperationToDB.ret)==FAILURE) {
        printf("DBOperationToDB ERROR!\n");
    }

    /*退出queue-get队列*/
    dao_queue_exit(&queueFlagGet);

    /*退出queue-set队列*/
    dao_queue_exit(&queueFlagSet);

    /*退出to-get队列*/
    dao_queue_exit(&toFlagGet);

    /*退出to-set队列*/
    dao_queue_exit(&toFlagSet);

    //退出lua虚拟机
    for(i=0; i<CData->GlobalConfigData.thread_num; i++) {
        lua_exit(*(L+i));
    }

    //锁机制销毁
    pthread_mutex_destroy(&mutexURLDo);

    //释放curl库
    curl_global_cleanup();

    //释放mysql
    mysql_library_end();

    /*释放内存*/
    dao_memory_link_exit(&memLinkFlag);

    return SUCCESS;
}

/*
作用：网络蜘蛛程序启动
*/
static int spider_start() {
    int flag;

    //初始地址获取内容
    flag=spider_start_url_do(CData->DBConfData.SpiderData.start_url);
    if(flag == FAILURE) {
        return FAILURE;
    }

    //设置线程可用,原来为-1，现在为0
    int i;
    for(i=0; i<CData->GlobalConfigData.thread_num; i++) {
        (URLDo+i)->isBusy=0;
    }

    //循环获取内容
    for(i=1; i<CData->DBConfData.SpiderData.depth+1; i++) {
        flag=spider_queue_url_do(i);
        if(flag == FAILURE) {
			printf("[%d]FAILURE>>Depth[%d] Has Completed!\n", CData->DBConfData.SpiderData.id, i);
            continue;
        }
		printf("[%d]SUCCESS>>Depth[%d] Has Completed!\n", CData->DBConfData.SpiderData.id, i);
    }

    //等待数据队列操作线程结束
    DBOperationToDB.setFlag=1;
    for(; ; ) {
        if(DBOperationToDB.setFlag==0) {
            break;
        }
    }

    //设置线程可用,原来为-1，现在为0
    for(i=0; i<CData->GlobalConfigData.thread_num; i++) {
        (ToDo+i)->isBusy=0;
    }

    //取出url，脚本调用采集器
    flag=spider_to_url_do();
    if(flag == FAILURE) {
        return FAILURE;
    }

    return SUCCESS;
}

/*
作用：网络蜘蛛起始url的完整采集
url:要采集的网址
*/
static int spider_start_url_do(char *url) {
    int *flag;//标识符
    int rt;//url_do的返回值存放在此变量中
    //void *urlDoRet;//url_do函数返回值存入此变量

    struct URLDo URLDo;
    URLDo.id=0;
    URLDo.depth=0;
    memset(URLDo.url, 0, sizeof(URLDo.url));
    strncpy(URLDo.url, url, strlen(url)+1);
    URLDo.rt=&rt;
    URLDo.isPthread=0;

    spider_url_do(&URLDo);
    if(*(URLDo.rt) == FAILURE) {
        return FAILURE;
    }

    //第一次获取，无论有多少数据都变更队列状态
    //等待数据队列操作线程结束
    DBOperationQueue.setFlag=1;
    for(; ; ) {
        if(DBOperationQueue.setFlag==0) {
            break;
        }
    }

    return SUCCESS;
}

/**
 *  function MakeDir
 *  author PengWeizhe
 *  date
 *  param [in] path 待创建的目录路径 可以相对路径和绝对路径
 *  return 0 创建成功 1创建失败
 *  details 创建一个目录(单级、多级)
 */
int make_dir(const char* path) {
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
                    return FAILURE;
                }
            }
            currentPath[i] = '/';
        }
    }
    return SUCCESS;
}

/*
作用：网络蜘蛛数据库操作队列
DBOperationQueueTmp:队列式数据库操作
*/
static void *spider_db_operation_queue(void *DBOperationQueueTmp) {
    //时间获取
    timeNow=time( NULL );
    localtime_r(&timeNow, &local);//开始运行时间
    struct DBOperationQueue *DBOperationQueue;
    DBOperationQueue=(struct DBOperationQueue *)DBOperationQueueTmp;

    //BerkeleyDB相关
    DB_ENV *myEnv;
    DB *dbp;
    DBC *DBcursor;
    int flag;
    u_int32_t envFlags;
    char path[128];

    for(; ; ) {
        //未开始
        if(DBOperationQueue->type==0) {
            //初始化字符串数组
            memset(path, 0, sizeof(path));
            //创建环境
            flag = db_env_create(&myEnv, 0);
            if (flag != 0) {
                fprintf(stderr, "Error creating env handle, queue: %s\n", db_strerror(flag));

                DBOperationQueue->type=3;//初始化错误3
                continue;
            }
            envFlags = DB_CREATE | DB_INIT_MPOOL;
            //打开环境
            if(strlen(CData->GlobalConfigData.data_path)+strlen("tmp/dbs/spider/queue/")+12+1>sizeof(path)) {
                //关闭环境
                if (myEnv != NULL) {
                    myEnv->close(myEnv, 0);
                    myEnv=NULL;
                }
                DBOperationQueue->type=3;//初始化错误3
                continue;
            }
            snprintf(path, sizeof(path),  "%stmp/dbs/spider/queue/%d/", CData->GlobalConfigData.data_path, CData->DBConfData.SpiderData.id);//设置文件路径
            //创建path
            //判断目录是否存在,不存在则建立此目录
            if(access(path, F_OK)<0) {
                flag=make_dir(path);//失败与成功都继续运行
                if(flag<0) {
                    //关闭环境
                    if (myEnv != NULL) {
                        myEnv->close(myEnv, 0);
                        myEnv=NULL;
                    }

                    DBOperationQueue->type=3;//初始化错误3
                    continue;
                }
            }
            flag = myEnv->open(myEnv, path, envFlags, 0);
            if (flag != 0) {
                fprintf(stderr, "Environment open failed, queue: %s\n", db_strerror(flag));
                //关闭环境
                if (myEnv != NULL) {
                    myEnv->close(myEnv, 0);
                    myEnv=NULL;
                }

                DBOperationQueue->type=3;//初始化错误3
                continue;
            }
            //创建数据库连接
            if ((flag = db_create(&dbp, myEnv, 0)) != 0) {
                fprintf(stderr, "db_create, queue: %s\n", db_strerror(flag));
                //关闭环境
                if (myEnv != NULL) {
                    myEnv->close(myEnv, 0);
                    myEnv=NULL;
                }

                DBOperationQueue->type=3;//初始化错误3
                continue;
            }
            //打开数据库
            if ((flag = dbp->open(dbp, NULL, "queue.db", NULL, DB_BTREE, DB_CREATE, 0664)) != 0) {
                dbp->err(dbp, flag, "%s", "queue.db");
                //关闭环境
                if (myEnv != NULL) {
                    myEnv->close(myEnv, 0);
                    myEnv=NULL;
                }

                DBOperationQueue->type=3;//初始化错误3
                continue;
            }
            DBOperationQueue->type=2;//初始化完成为2
        }
        //开始
        if(DBOperationQueue->type==1) {
            for(; ; ) {
                //程序退出
                if(DBOperationQueue->isEnd==1) {
                    //关闭数据库
                    if (dbp != NULL) {
                        dbp->close(dbp, 0);
                        dbp=NULL;
                    }
                    //关闭环境
                    if (myEnv != NULL) {
                        myEnv->close(myEnv, 0);
                        myEnv=NULL;
                    }

                    *(DBOperationQueue->ret)=SUCCESS;
                    return (void *)DBOperationQueue->ret;
                }

                //type不为1时退出此循环
                if(DBOperationQueue->type!=1) {
                    break;
                }

                //是否可以执行get
                if(DBOperationQueue->getFlag==1) {
                    struct DaoQueue *nowQueue;//定义存储队列数据的结构
                    struct BDBData *BDBData;//定义BerkeleyDB中data结构
                    struct BDBData BDBDataTmp;//定义BerkeleyDB中data结构
                    DBT key, data;//BerkeleyDB的key和value
                    int i, j;

                    int depth;//depth，程序此时的深度
                    depth=DBOperationQueue->getDepth;

                    //返回一个创建的数据库游标
                    flag=dbp->cursor(dbp, NULL, &DBcursor, 0);
                    if(flag!=0) {
                        continue;
                    }

                    //进行数据取出并赋值
                    j=0;//判断游标获取是否结束
                    for(; ; ) {
                        //初始化key与data,由于key与data为空，所以遍历全部数据
                        memset(&(key), 0, sizeof(DBT));
                        memset(&(data), 0, sizeof(DBT));

                        flag=DBcursor->get(DBcursor, &key, &data, DB_NEXT);//可查找与key或者key和data匹配的记录.获取下一个(DB_PREV key为空，匹配数据，重头开始).由于删除后又插入，所以直接用DB_NEXT才能保证数据尽量不会重复获取
                        if(flag!=0) {
                            if(flag==DB_NOTFOUND) {
                                j=1;
                                break;
                            } else {
                                dbp->err(dbp, flag, "DBcursor->get");
                                continue;
                            }
                        }

                        BDBData=data.data;
                        if(BDBData->status!=0) {
                            continue;
                        }

                        if(BDBData->depth!=(depth-1)) {
                            continue;
                        }

                        //获取的url为空时跳过此条数据
                        if(strlen(BDBData->url)==0) {
                            continue;
                        }

                        //队列set
                        flag=dao_queue_set(&queueFlagGet, BDBData->depth, BDBData->url);
                        //队列已满，终止赋值
                        if(flag==2) {
                            break;
                        }
                        //队列报错，终止赋值
                        if(flag==DAOFAILURE) {
                            break;
                        }
                    }

                    //关闭游标cursor
                    if(DBcursor!=NULL) {
                        DBcursor->close(DBcursor);
                        DBcursor=NULL;
                    }

                    //数据已经获取过，所以update
                    for(i=0; i<=queueFlagGet.maxIndex; i++) {
                        flag=dao_queue_get_index(&queueFlagGet, i, &nowQueue);
                        if(flag==DAOFAILURE) {
                            printf ("Queue get error!\n");
                            continue;
                        }

                        //对列中的url为空时跳过此条数据
                        if(strlen(nowQueue->data->data)==0) {
                            continue;
                        }

                        //初始化BDBData
                        memset(&(BDBDataTmp), 0, sizeof(struct BDBData));
                        //BDBData赋值
                        strncpy(BDBDataTmp.url, nowQueue->data->data, strlen(nowQueue->data->data)+1);
                        BDBDataTmp.depth=nowQueue->depth;
                        BDBDataTmp.status=1;

                        //初始化key与data
                        memset(&(key), 0, sizeof(DBT));
                        memset(&(data), 0, sizeof(DBT));

                        key.data = nowQueue->data->data;
                        key.size = strlen(nowQueue->data->data)+1;
                        data.data = &(BDBDataTmp);
                        data.size = sizeof(struct BDBData);

                        //删除数据
                        flag = dbp->del(dbp, NULL, &key, 0);
                        //删除数据失败说明数据不需要更新了
                        if(flag!=0) {
                            if(flag!=DB_NOTFOUND) {
                                dbp->err(dbp, flag, "DB->del");
                            }
                        }

                        //更新数据
                        flag = dbp->put(dbp, NULL, &key, &data, DB_NOOVERWRITE);
                        if (flag!= 0) {
                            dbp->err(dbp, flag, "DB->put");
                            continue;
                        }
                    }

                    //数据写入硬盘
                    /*flag=dbp->sync(dbp, 0);
                    if (flag!= 0) {
                        *(DBOperationQueue->ret)=FAILURE;
                        //get状态变更为0
                        DBOperationQueue->getFlag=0;
                        continue;
                    }*/

                    if(j==1) {
                        DBOperationQueue->getFlag=3;//返回3，代表此depth数据获取完成
                    } else {
                        //get状态变更为0
                        DBOperationQueue->getFlag=0;
                    }

                    continue;
                }

                //是否可以执行set
                if(DBOperationQueue->setFlag==1) {
                    struct DaoQueue *nowQueue;//定义存储队列数据的结构
                    struct BDBData BDBData;//定义BerkeleyDB中data结构
                    DBT key, data;//BerkeleyDB的key和value
                    int i;

                    //赋值URLDO到线程
                    for(i=0; i<=queueFlagSet.maxIndex; i++) {
                        flag=dao_queue_get_index(&queueFlagSet, i, &nowQueue);
                        if(flag==DAOFAILURE) {
                            printf ("Queue get error!\n");
                            DBOperationQueue->setFlag=0;//变更状态
                            //清空队列
                            flag=dao_queue_reset(&queueFlagSet);
                            if(flag==DAOFAILURE) {
                                *(DBOperationQueue->ret)=FAILURE;
                                DBOperationQueue->setFlag=0;//变更状态
                                //return (void *)DBOperationQueue->ret;
                                continue;
                            }
                            continue;
                        }

                        //对列中的url为空时跳过此条数据
                        if(strlen(nowQueue->data->data)==0) {
                            continue;
                        }

                        //初始化BDBData
                        memset(&(BDBData), 0, sizeof(struct BDBData));
                        //BDBData赋值
                        strncpy(BDBData.url, nowQueue->data->data, strlen(nowQueue->data->data)+1);
                        BDBData.depth=nowQueue->depth;
                        BDBData.status=0;

                        //初始化key与data
                        memset(&(key), 0, sizeof(DBT));
                        memset(&(data), 0, sizeof(DBT));

                        key.data = nowQueue->data->data;
                        key.size = strlen(nowQueue->data->data)+1;//字符串大小，否则会出现为key的size为1，存储进去的key不完整
                        data.data = &(BDBData);
                        data.size = sizeof(struct BDBData);

                        //数据传入BerkeleyDB
                        flag=dbp->exists(dbp, NULL, &key, 0);
                        if(flag==DB_NOTFOUND) {
                            flag = dbp->put(dbp, NULL, &key, &data, DB_NOOVERWRITE);
                            if (flag!= 0) {
                                if(flag!=DB_KEYEXIST) {
                                    dbp->err(dbp, flag, "DB->put");
                                    continue;
                                }
                            }
                        }
                    }

                    //数据写入硬盘
                    /*flag=dbp->sync(dbp, 0);
                    if (flag!= 0) {
                        *(DBOperationQueue->ret)=FAILURE;
                        DBOperationQueue->setFlag=0;//变更状态
                        //return (void *)DBOperationQueue->ret;
                        continue;
                    }*/

                    //重置队列，否则会出现队列的queueFlagGet->maxIndex!=0;queueFlagSet->nowGetIndex!=0;queueFlagSet->nowSetIndex!=0;从而最终造成程序报错（内存过界）
                    flag=dao_queue_reset(&queueFlagSet);
                    if(flag==DAOFAILURE) {
                        *(DBOperationQueue->ret)=FAILURE;
                        DBOperationQueue->setFlag=0;//变更状态
                        //return (void *)DBOperationQueue->ret;
                        continue;
                    }

                    //pthread_mutex_unlock(&mutexURLDo);//阻塞解锁
                    DBOperationQueue->setFlag=0;//变更状态
                    continue;
                }
            }
        }
        //重置并初始化
        if(DBOperationQueue->type==4) {
            //关闭数据库
            if (dbp != NULL) {
                dbp->close(dbp, 0);
                dbp=NULL;
            }
            //关闭环境
            if (myEnv != NULL) {
                myEnv->close(myEnv, 0);
                myEnv=NULL;
            }

            //初始化字符串数组
            memset(path, 0, sizeof(path));
            //创建环境
            flag = db_env_create(&myEnv, 0);
            if (flag != 0) {
                fprintf(stderr, "Error creating env handle, queue: %s\n", db_strerror(flag));

                DBOperationQueue->type=3;//初始化错误3
                continue;
            }
            envFlags = DB_CREATE | DB_INIT_MPOOL;
            //打开环境
            if(strlen(CData->GlobalConfigData.data_path)+strlen("tmp/dbs/spider/queue/")+12+1>sizeof(path)) {
                DBOperationQueue->type=3;//初始化错误3
                continue;
            }
            snprintf(path, sizeof(path),  "%stmp/dbs/spider/queue/%d/", CData->GlobalConfigData.data_path, CData->DBConfData.SpiderData.id);//设置文件路径
            //创建path
            //判断目录是否存在,不存在则建立此目录
            if(access(path, F_OK)<0) {
                flag=make_dir(path);
                if(flag<0) {
                    //关闭环境
                    if (myEnv != NULL) {
                        myEnv->close(myEnv, 0);
                        myEnv=NULL;
                    }

                    DBOperationQueue->type=3;//初始化错误3
                    continue;
                }
            }
            flag = myEnv->open(myEnv, path, envFlags, 0);
            if (flag != 0) {
                fprintf(stderr, "Environment open failed, queue: %s\n", db_strerror(flag));
                //关闭环境
                if (myEnv != NULL) {
                    myEnv->close(myEnv, 0);
                    myEnv=NULL;
                }

                DBOperationQueue->type=3;//初始化错误3
                continue;
            }
            //创建数据库连接
            if ((flag = db_create(&dbp, myEnv, 0)) != 0) {
                fprintf(stderr, "db_create, queue: %s\n", db_strerror(flag));
                //关闭环境
                if (myEnv != NULL) {
                    myEnv->close(myEnv, 0);
                    myEnv=NULL;
                }

                DBOperationQueue->type=3;//初始化错误3
                continue;
            }
            //打开数据库
            if ((flag = dbp->open(dbp, NULL, "queue.db", NULL, DB_BTREE, DB_CREATE, 0664)) != 0) {
                dbp->err(dbp, flag, "%s", "queue.db");
                //关闭环境
                if (myEnv != NULL) {
                    myEnv->close(myEnv, 0);
                    myEnv=NULL;
                }

                DBOperationQueue->type=3;//初始化错误3
                continue;
            }
            DBOperationQueue->type=2;//初始化完成为2
        }
    }
}

/*
作用：网络蜘蛛数据库操作队列
DBOperationToDBTmp:队列式数据库操作
*/
static void *spider_db_operation_to_db(void *DBOperationToDBTmp) {
    //时间获取
    timeNow=time( NULL );
    localtime_r(&timeNow, &local);//开始运行时间
    struct DBOperationToDB *DBOperationToDB;
    DBOperationToDB=(struct DBOperationToDB *)DBOperationToDBTmp;

    //BerkeleyDB相关
    DB_ENV *myEnv;
    DB *dbp;
    DBC *DBcursor;
    int flag;
    u_int32_t envFlags;
    char path[128];

    for(; ; ) {
        //未开始
        if(DBOperationToDB->type==0) {
            //初始化字符串数组
            memset(path, 0, sizeof(path));
            //创建环境
            flag = db_env_create(&myEnv, 0);
            if (flag != 0) {
                fprintf(stderr, "Error creating env handle, to: %s\n", db_strerror(flag));

                DBOperationToDB->type=3;//初始化错误3
                continue;
            }
            envFlags = DB_CREATE | DB_INIT_MPOOL;
            //打开环境
            if(strlen(CData->GlobalConfigData.data_path)+strlen("tmp/dbs/spider/to/")+12+1>sizeof(path)) {
                //关闭环境
                if (myEnv != NULL) {
                    myEnv->close(myEnv, 0);
                    myEnv=NULL;
                }
                DBOperationToDB->type=3;//初始化错误3
                continue;
            }
            snprintf(path, sizeof(path),  "%stmp/dbs/spider/to/%d/", CData->GlobalConfigData.data_path, CData->DBConfData.SpiderData.id);//设置文件路径
            //创建path
            //判断目录是否存在,不存在则建立此目录
            if(access(path, F_OK)<0) {
                flag=make_dir(path);
                if(flag<0) {
                    //关闭环境
                    if (myEnv != NULL) {
                        myEnv->close(myEnv, 0);
                        myEnv=NULL;
                    }

                    DBOperationToDB->type=3;//初始化错误3
                    continue;
                }
            }
            flag = myEnv->open(myEnv, path, envFlags, 0);
            if (flag != 0) {
                fprintf(stderr, "Environment open failed, to: %s\n", db_strerror(flag));
                //关闭环境
                if (myEnv != NULL) {
                    myEnv->close(myEnv, 0);
                    myEnv=NULL;
                }

                DBOperationToDB->type=3;//初始化错误3
                continue;
            }
            //创建数据库连接
            if ((flag = db_create(&dbp, myEnv, 0)) != 0) {
                fprintf(stderr, "db_create, to: %s\n", db_strerror(flag));
                //关闭环境
                if (myEnv != NULL) {
                    myEnv->close(myEnv, 0);
                    myEnv=NULL;
                }

                DBOperationToDB->type=3;//初始化错误3
                continue;
            }
            //打开数据库
            if ((flag = dbp->open(dbp, NULL, "to_db.db", NULL, DB_BTREE, DB_CREATE, 0664)) != 0) {
                dbp->err(dbp, flag, "%s", "to_db.db");
                //关闭环境
                if (myEnv != NULL) {
                    myEnv->close(myEnv, 0);
                    myEnv=NULL;
                }

                DBOperationToDB->type=3;//初始化错误3
                continue;
            }
            DBOperationToDB->type=2;
        }
        //开始
        if(DBOperationToDB->type==1) {
            for(; ; ) {
                //程序退出
                if(DBOperationToDB->isEnd==1) {
                    //关闭数据库
                    if (dbp != NULL) {
                        dbp->close(dbp, 0);
                        dbp=NULL;
                    }
                    //关闭环境
                    if (myEnv != NULL) {
                        myEnv->close(myEnv, 0);
                        myEnv=NULL;
                    }

                    *(DBOperationToDB->ret)=SUCCESS;
                    return (void *)DBOperationToDB->ret;
                }

                //type不为1时退出此循环
                if(DBOperationToDB->type!=1) {
                    break;
                }
                //是否可以执行get
                if(DBOperationToDB->getFlag==1) {
                    struct DaoQueue *nowQueue;//定义存储队列数据的结构
                    struct BDBTData *BDBTData;//定义BerkeleyDB中data结构
                    struct BDBTData BDBTDataTmp;//定义BerkeleyDB中data结构
                    DBT key, data;//BerkeleyDB的key和value
                    int i, j;

                    //返回一个创建的数据库游标
                    flag=dbp->cursor(dbp, NULL, &DBcursor, 0);
                    if(flag!=0) {
                        continue;
                    }

                    //进行数据取出并赋值
                    j=0;//判断游标获取是否结束
                    for(; ; ) {
                        //初始化key与data,由于key与data为空，所以遍历全部数据
                        memset(&(key), 0, sizeof(DBT));
                        memset(&(data), 0, sizeof(DBT));

                        flag=DBcursor->get(DBcursor, &key, &data, DB_NEXT);//可查找与key或者key和data匹配的记录.获取下一个(DB_PREV key为空，匹配数据，重头开始).由于删除后又插入，所以直接用DB_NEXT才能保证数据尽量不会重复获取
                        if(flag!=0) {
                            if(flag==DB_NOTFOUND) {
                                j=1;
                                break;
                            } else {
                                dbp->err(dbp, flag, "DBcursor->get");
                                continue;
                            }
                        }

                        BDBTData=data.data;
                        if(BDBTData->status!=0) {
                            continue;
                        }

                        //获取的url为空时跳过此条数据
                        if(strlen(BDBTData->url)==0) {
                            continue;
                        }

                        //队列set
                        flag=dao_queue_set(&toFlagGet, 0, BDBTData->url);
                        //队列已满，终止赋值
                        if(flag==2) {
                            break;
                        }

                        //队列报错，终止赋值
                        if(flag==DAOFAILURE) {
                            break;
                        }
                    }

                    //关闭游标cursor
                    if(DBcursor!=NULL) {
                        DBcursor->close(DBcursor);
                        DBcursor=NULL;
                    }

                    //数据已经获取过，所以update
                    for(i=0; i<=toFlagGet.maxIndex; i++) {
                        flag=dao_queue_get_index(&toFlagGet, i, &nowQueue);
                        if(flag==DAOFAILURE) {
                            printf ("Queue get error!\n");
                            continue;
                        }

                        //对列中的url为空时跳过此条数据
                        if(strlen(nowQueue->data->data)==0) {
                            continue;
                        }

                        //初始化BDBTData
                        memset(&(BDBTDataTmp), 0, sizeof(struct BDBTData));
                        //BDBTData赋值
                        strncpy(BDBTDataTmp.url, nowQueue->data->data, strlen(nowQueue->data->data)+1);
                        BDBTDataTmp.status=1;

                        //初始化key与data
                        memset(&(key), 0, sizeof(DBT));
                        memset(&(data), 0, sizeof(DBT));

                        key.data = nowQueue->data->data;
                        key.size = strlen(nowQueue->data->data)+1;
                        data.data = &(BDBTDataTmp);
                        data.size = sizeof(struct BDBTData);

                        //删除数据
                        flag = dbp->del(dbp, NULL, &key, 0);
                        //删除数据失败说明数据不需要更新了
                        if(flag!=0) {
                            if(flag!=DB_NOTFOUND) {
                                dbp->err(dbp, flag, "DB->del");
                            }
                        }
                        //更新数据
                        flag = dbp->put(dbp, NULL, &key, &data, DB_NOOVERWRITE);
                        if (flag!= 0) {
                            dbp->err(dbp, flag, "DB->put");
                            continue;
                        }
                    }

                    //数据写入硬盘
                    /*flag=dbp->sync(dbp, 0);
                    if (flag!= 0) {
                        *(DBOperationToDB->ret)=FAILURE;
                        //get状态变更为0
                        DBOperationToDB->getFlag=0;
                        continue;
                    }*/

                    if(j==1) {
                        DBOperationToDB->getFlag=3;//返回3，代表此depth数据获取完成
                    } else {
                        //get状态变更为0
                        DBOperationToDB->getFlag=0;
                    }

                    continue;
                }

                //是否可以执行set
                if(DBOperationToDB->setFlag==1) {
                    struct DaoQueue *nowQueue;//定义存储队列数据的结构
                    struct BDBTData BDBTData;//定义BerkeleyDB中data结构
                    DBT key, data;//BerkeleyDB的key和value
                    int i;

                    //赋值URLDO到线程
                    for(i=0; i<=toFlagSet.maxIndex; i++) {
                        flag=dao_queue_get_index(&toFlagSet, i, &nowQueue);
                        if(flag==DAOFAILURE) {
                            printf ("Queue get error!\n");
                            DBOperationToDB->setFlag=0;//变更状态
                            //清空队列
                            flag=dao_queue_reset(&toFlagSet);
                            if(flag==DAOFAILURE) {
                                *(DBOperationToDB->ret)=FAILURE;
                                DBOperationToDB->setFlag=0;//变更状态
                                //return (void *)DBOperationToDB->ret;
                                continue;
                            }
                            continue;
                        }

                        //对列中的url为空时跳过此条数据
                        if(strlen(nowQueue->data->data)==0) {
                            continue;
                        }

                        //初始化BDBTData
                        memset(&(BDBTData), 0, sizeof(struct BDBTData));
                        //BDBTData赋值
                        strncpy(BDBTData.url, nowQueue->data->data, strlen(nowQueue->data->data)+1);
                        BDBTData.status=0;

                        //初始化key与data
                        memset(&(key), 0, sizeof(DBT));
                        memset(&(data), 0, sizeof(DBT));

                        key.data = nowQueue->data->data;
                        key.size = strlen(nowQueue->data->data)+1;//字符串大小，否则会出现为key的size为1，存储进去的key不完整
                        data.data = &(BDBTData);
                        data.size = sizeof(struct BDBTData);

                        //数据传入BerkeleyDB
                        flag=dbp->exists(dbp, NULL, &key, 0);
                        if(flag==DB_NOTFOUND) {
                            flag = dbp->put(dbp, NULL, &key, &data, DB_NOOVERWRITE);
                            if (flag!= 0) {
                                if(flag!=DB_KEYEXIST) {
                                    dbp->err(dbp, flag, "DB->put");
                                    continue;
                                }
                            } else {
                                //时间获取
                                timeNow=time( NULL );
                                localtime_r(&timeNow, &local);//开始运行时间

                                printf("[%d]To>>URL>>[%s]Has Inserted, Time:[%d-%d-%d %d:%d:%d]!\n", CData->DBConfData.SpiderData.id, BDBTData.url, (local.tm_year+1900), (local.tm_mon+1), local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec);
                            }
                        }
                    }

                    //数据写入硬盘
                    /*flag=dbp->sync(dbp, 0);
                    if (flag!= 0) {
                        *(DBOperationToDB->ret)=FAILURE;
                        DBOperationToDB->setFlag=0;//变更状态
                        //return (void *)DBOperationToDB->ret;
                        continue;
                    }*/

                    //重置队列，否则会出现队列的toFlagSetGet->maxIndex!=0;toFlagSet->nowGetIndex!=0;toFlagSet->nowSetIndex!=0;从而最终造成程序报错（内存过界）
                    flag=dao_queue_reset(&toFlagSet);
                    if(flag==DAOFAILURE) {
                        *(DBOperationToDB->ret)=FAILURE;
                        DBOperationToDB->setFlag=0;//变更状态
                        //return (void *)DBOperationToDB->ret;
                        continue;
                    }

                    //pthread_mutex_unlock(&mutexURLDo);//阻塞解锁
                    DBOperationToDB->setFlag=0;//变更状态
                    continue;
                }
            }
        }
        //重置并初始化
        if(DBOperationToDB->type==4) {
            //关闭数据库
            if (dbp != NULL) {
                dbp->close(dbp, 0);
                dbp=NULL;
            }
            //关闭环境
            if (myEnv != NULL) {
                myEnv->close(myEnv, 0);
                myEnv=NULL;
            }

            //初始化字符串数组
            memset(path, 0, sizeof(path));
            //创建环境
            flag = db_env_create(&myEnv, 0);
            if (flag != 0) {
                fprintf(stderr, "Error creating env handle, to: %s\n", db_strerror(flag));

                DBOperationToDB->type=3;//初始化错误3
                continue;
            }
            envFlags = DB_CREATE | DB_INIT_MPOOL;
            //打开环境
            if(strlen(CData->GlobalConfigData.data_path)+strlen("tmp/dbs/spider/to/")+12+1>sizeof(path)) {
                //关闭环境
                if (myEnv != NULL) {
                    myEnv->close(myEnv, 0);
                    myEnv=NULL;
                }
                DBOperationToDB->type=3;//初始化错误3
                continue;
            }
            snprintf(path, sizeof(path),  "%stmp/dbs/spider/to/%d/", CData->GlobalConfigData.data_path, CData->DBConfData.SpiderData.id);//设置文件路径
            //创建path
            //判断目录是否存在,不存在则建立此目录
            if(access(path, F_OK)<0) {
                flag=make_dir(path);
                if(flag<0) {
                    DBOperationToDB->type=3;//初始化错误3
                    continue;
                }
            }

            flag = myEnv->open(myEnv, path, envFlags, 0);
            if (flag != 0) {
                fprintf(stderr, "Environment open failed, to: %s\n", db_strerror(flag));
                //关闭环境
                if (myEnv != NULL) {
                    myEnv->close(myEnv, 0);
                    myEnv=NULL;
                }

                DBOperationToDB->type=3;//初始化错误3
                continue;
            }

            //创建数据库连接
            if ((flag = db_create(&dbp, myEnv, 0)) != 0) {
                fprintf(stderr, "db_create, to: %s\n", db_strerror(flag));
                //关闭环境
                if (myEnv != NULL) {
                    myEnv->close(myEnv, 0);
                    myEnv=NULL;
                }

                DBOperationToDB->type=3;//初始化错误3
                continue;
            }

            //打开数据库
            if ((flag = dbp->open(dbp, NULL, "to_db.db", NULL, DB_BTREE, DB_CREATE, 0664)) != 0) {
                dbp->err(dbp, flag, "%s", "to_db.db");
                //关闭环境
                if (myEnv != NULL) {
                    myEnv->close(myEnv, 0);
                    myEnv=NULL;
                }

                DBOperationToDB->type=3;//初始化错误3
                continue;
            }
            DBOperationToDB->type=2;
        }
    }
}

/*
作用：网络蜘蛛获取某个深度的全部url并用回调函数启动url采集
depth:深度
*/
static int spider_queue_url_do(int depth) {
    //深度赋值
    DBOperationQueue.getDepth=depth;

    //队列不足，但是数据已经取完则set队列默认结束，开始启动入库DBOperationQueue.setFlag==1
    if(depth>1) {
        DBOperationQueue.setFlag=1;//设置set为可用，把（非完全（由于队列很多但是数据很少，于是造成队列未完成））队列的值进行插入
        //阻塞，直到队列值插入完成
        for(; ; ) {
            if(DBOperationQueue.setFlag==0) {
                break;
            }
        }

        DBOperationQueue.getFlag=1;//设置get为可用
    } else {
        //第一次队列获取，初始化
        DBOperationQueue.getFlag=1;
    }

    //阻塞，直到第一次数据获取
    for(; ; ) {
        if(DBOperationQueue.getFlag==0 || DBOperationQueue.getFlag==3) {
            break;
        }
    }

    for(; ; ) {
        int flag;//返回值标识
        int p, q, i, j;
        int maxIndex;
        struct DaoQueue *nowQueue;//定义存储队列数据的结构

        //此depth结束
        if(DBOperationQueue.getFlag==3) {
            //获取队列最多的数据量
            if(queueFlagGet.maxIndex>=CData->GlobalConfigData.queue_get_num) {
                maxIndex=CData->GlobalConfigData.queue_get_num;
            } else {
                maxIndex=queueFlagGet.maxIndex;
            }
            for(p=0; p<maxIndex; p++) {
                //找到空闲线程
                for(; ; ) {
                    j=0;//标志是否找到空闲线程
                    for(i=0; i<CData->GlobalConfigData.thread_num; i++) {
                        if((URLDo+i)->isBusy==0) {
                            j=1;//找到空闲的线程j=1
                            q=i;//线程编号赋值
                            break;
                        }
                    }
                    //找到空闲的线程j=1
                    if(j==1) {
                        break;
                    }
                }

                //获取即将获取的队列数据
                flag=dao_queue_get_index(&queueFlagGet, p, &nowQueue);
                if(flag==DAOFAILURE) {
                    printf ("Queue get error!\n");
                    continue;
                }
                //数据赋值到线程
                (URLDo+q)->depth=depth;
                memset((URLDo+q)->url, 0, URL_DATA_SIZE);
                strncpy((URLDo+q)->url, nowQueue->data->data, strlen(nowQueue->data->data)+1);
                (URLDo+q)->isBusy=1;//最后来赋值
                //线程暂停时间
                usleep(CData->DBConfData.SpiderData.per_interval_time);
                //下一个线程
            }

            //重置队列，否则会出现队列的queueFlagGet->maxIndex!=0;queueFlagGet->nowGetIndex!=0;queueFlagGet->nowSetIndex!=0;从而最终造成程序报错（内存过界）
            flag=dao_queue_reset(&queueFlagGet);
            if(flag==DAOFAILURE) {
                return FAILURE;
            }

            //等待URLDo线程结束
            for(i=0; i<CData->GlobalConfigData.thread_num; ) {
                if((URLDo+i)->isBusy==0) {
                    i++;
                }
            }

            //等待数据队列操作线程结束
            DBOperationQueue.setFlag=1;
            for(; ; ) {
                if(DBOperationQueue.setFlag==0) {
                    break;
                }
            }

            return SUCCESS;
        } else if(DBOperationQueue.getFlag==-1) {
            return FAILURE;
        } else if(DBOperationQueue.getFlag==1) {
            continue;
        } else if(DBOperationQueue.getFlag==0)  {
            if(queueFlagGet.nowGetIndex<=CData->GlobalConfigData.queue_get_num) {
                //获取队列最多的数据量
                if(queueFlagGet.maxIndex>=CData->GlobalConfigData.queue_get_num) {
                    maxIndex=CData->GlobalConfigData.queue_get_num;
                } else {
                    maxIndex=queueFlagGet.maxIndex;
                }
                for(p=0; p<maxIndex; p++) {
                    //找到空闲线程
                    for(; ; ) {
                        j=0;//标志是否找到空闲线程
                        for(i=0; i<CData->GlobalConfigData.thread_num; i++) {
                            if((URLDo+i)->isBusy==0) {
                                j=1;//找到空闲的线程j=1
                                q=i;//线程编号赋值
                                break;
                            }
                        }
                        //找到空闲的线程j=1
                        if(j==1) {
                            break;
                        }
                    }

                    //获取即将获取的队列数据
                    flag=dao_queue_get_index(&queueFlagGet, p, &nowQueue);
                    if(flag==DAOFAILURE) {
                        printf ("Queue get error!\n");
                        continue;
                    }
                    //数据赋值到线程
                    (URLDo+q)->depth=nowQueue->depth+1;
                    memset((URLDo+q)->url, 0, URL_DATA_SIZE);
                    strncpy((URLDo+q)->url, nowQueue->data->data, strlen(nowQueue->data->data)+1);
                    (URLDo+q)->isBusy=1;//最后来赋值

                    //线程暂停时间
                    usleep(CData->DBConfData.SpiderData.per_interval_time);
                    //下一个线程
                }

                //重置队列，否则会出现队列的queueFlagGet->maxIndex!=0;queueFlagGet->nowGetIndex!=0;queueFlagGet->nowSetIndex!=0;从而最终造成程序报错（内存过界）
                flag=dao_queue_reset(&queueFlagGet);
                if(flag==DAOFAILURE) {
                    return FAILURE;
                }

                DBOperationQueue.getFlag=1;
            } else {
                //重置队列，否则会出现队列的queueFlagGet->maxIndex!=0;queueFlagGet->nowGetIndex!=0;queueFlagGet->nowSetIndex!=0;从而最终造成程序报错（内存过界）
                flag=dao_queue_reset(&queueFlagGet);
                if(flag==DAOFAILURE) {
                    return FAILURE;
                }
                DBOperationQueue.getFlag=1;
            }
        } else {
            return FAILURE;
        }
    }
}

/*
作用：取出url，脚本调用采集器
*/
static int spider_to_url_do() {
    DBOperationToDB.getFlag=1;//设置get为可用

    //阻塞，直到第一次数据获取
    for(; ; ) {
        if(DBOperationToDB.getFlag==0 || DBOperationToDB.getFlag==3) {
            break;
        }
    }

    for(; ; ) {
        int flag;//返回值标识
        int p, q, i, j;
        int maxIndex;
        struct DaoQueue *nowQueue;//定义存储队列数据的结构

        //结束
        if(DBOperationToDB.getFlag==3) {
            //获取队列最多的数据量
            if(toFlagGet.maxIndex>=CData->GlobalConfigData.to_get_num) {
                maxIndex=CData->GlobalConfigData.to_get_num;
            } else {
                maxIndex=toFlagGet.maxIndex;
            }
            for(p=0; p<maxIndex; p++) {
                //找到空闲线程
                for(; ; ) {
                    j=0;//标志是否找到空闲线程
                    for(i=0; i<CData->GlobalConfigData.thread_num; i++) {
                        if((ToDo+i)->isBusy==0) {
                            j=1;//找到空闲的线程j=1
                            q=i;//线程编号赋值
                            break;
                        }
                    }
                    //找到空闲的线程j=1
                    if(j==1) {
                        break;
                    }
                }

                //获取即将获取的队列数据
                flag=dao_queue_get_index(&toFlagGet, p, &nowQueue);
                if(flag==DAOFAILURE) {
                    printf ("Queue get error!\n");
                    continue;
                }
                //数据赋值到线程
                memset((ToDo+q)->url, 0, URL_DATA_SIZE);
                strncpy((ToDo+q)->url, nowQueue->data->data, strlen(nowQueue->data->data)+1);
                (ToDo+q)->isBusy=1;//最后来赋值
                //线程暂停时间
                usleep(CData->DBConfData.SpiderData.per_interval_time);
                //下一个线程
            }

            //重置队列，否则会出现队列的toFlagGet->maxIndex!=0;toFlagGet->nowGetIndex!=0;toFlagGet->nowSetIndex!=0;从而最终造成程序报错（内存过界）
            flag=dao_queue_reset(&toFlagGet);
            if(flag==DAOFAILURE) {
                return FAILURE;
            }

            //等待ToDo线程结束
            for(i=0; i<CData->GlobalConfigData.thread_num; ) {
                if((ToDo+i)->isBusy==0) {
                    i++;
                }
            }

            return SUCCESS;
        } else if(DBOperationToDB.getFlag==-1) {
            return FAILURE;
        } else if(DBOperationToDB.getFlag==1) {
            continue;
        } else if(DBOperationToDB.getFlag==0)  {
            if(toFlagGet.nowGetIndex<=CData->GlobalConfigData.to_get_num) {
                //获取队列最多的数据量
                if(toFlagGet.maxIndex>=CData->GlobalConfigData.to_get_num) {
                    maxIndex=CData->GlobalConfigData.to_get_num;
                } else {
                    maxIndex=toFlagGet.maxIndex;
                }
                for(p=0; p<maxIndex; p++) {
                    //找到空闲线程
                    for(; ; ) {
                        j=0;//标志是否找到空闲线程
                        for(i=0; i<CData->GlobalConfigData.thread_num; i++) {
                            if((ToDo+i)->isBusy==0) {
                                j=1;//找到空闲的线程j=1
                                q=i;//线程编号赋值
                                break;
                            }
                        }
                        //找到空闲的线程j=1
                        if(j==1) {
                            break;
                        }
                    }

                    //获取即将获取的队列数据
                    flag=dao_queue_get_index(&toFlagGet, p, &nowQueue);
                    if(flag==DAOFAILURE) {
                        printf ("Queue get error!\n");
                        continue;
                    }
                    //数据赋值到线程
                    memset((ToDo+q)->url, 0, URL_DATA_SIZE);
                    strncpy((ToDo+q)->url, nowQueue->data->data, strlen(nowQueue->data->data)+1);
                    (ToDo+q)->isBusy=1;//最后来赋值

                    //线程暂停时间
                    usleep(CData->DBConfData.SpiderData.per_interval_time);
                    //下一个线程
                }

                //重置队列，否则会出现队列的toFlagGet->maxIndex!=0;toFlagGet->nowGetIndex!=0;toFlagGet->nowSetIndex!=0;从而最终造成程序报错（内存过界）
                flag=dao_queue_reset(&toFlagGet);
                if(flag==DAOFAILURE) {
                    return FAILURE;
                }

                DBOperationToDB.getFlag=1;
            } else {
                //重置队列，否则会出现队列的toFlagGet->maxIndex!=0;toFlagGet->nowGetIndex!=0;toFlagGet->nowSetIndex!=0;从而最终造成程序报错（内存过界）
                flag=dao_queue_reset(&toFlagGet);
                if(flag==DAOFAILURE) {
                    return FAILURE;
                }
                DBOperationToDB.getFlag=1;
            }
        } else {
            return FAILURE;
        }
    }
}

/*
作用：网络蜘蛛启动一个url的完整采集
URLDoTmp:要采集的网址等结构体
*/
static void *spider_url_do(void *URLDoTmp) {
    struct URLDo *URLDoIn;
    URLDoIn=(struct URLDo *)URLDoTmp;
    //时间获取
    timeNow=time( NULL );
    localtime_r(&timeNow, &local);//开始运行时间

    /*区别start_url_do*/
    if(URLDoIn->isPthread==0) {
        int flag;//标识
        char *url;
        int depth;

        //初始化各种变量
        memset(*(data+URLDoIn->id), 0, BIG_DATA_SIZE);
        memset(*(dataTmp+URLDoIn->id), 0, BIG_DATA_SIZE);

        //传入值赋值
        depth=URLDoIn->depth;
        url=URLDoIn->url;

        //格式化网址
        flag=spider_format_url(URLDoIn->url);
        if(flag==FAILURE) {
            *(URLDoIn->rt)=FAILURE;
            return (void *)URLDoIn->rt;
        }

        //执行数据获取
        //flag=spider_data_get(URLDoIn);
        flag=spider_curl_data_get(0, URLDoIn);
        if(flag == FAILURE) {
            *(URLDoIn->rt)=FAILURE;
            return (void *)URLDoIn->rt;
        }

        //编码转换
        flag=spider_charset_transfer(0, URLDoIn);
        if(flag == FAILURE) {
            *(URLDoIn->rt)=FAILURE;
            return (void *)URLDoIn->rt;
        }

        //printf("queue_set Begin!\n");
        //新增队列（使用sqlite）
        //重新定义baseUrl，否则会出现baseUrl:http://www.cnbeta.com/topics/379.htm
        char baseURL[URL_DATA_SIZE];
        //初始化字符串数组
        memset(baseURL, 0, sizeof(baseURL));

        //格式化baseUrl
        int i, pEnd=0;
        for(i=0; i<strlen(url); i++) {
            if(*(url+(strlen(url)-i))=='/') {
                pEnd=(strlen(url)-i)+1;
                break;
            }
        }
        //没有取得pEnd值时
        if(pEnd==0) {
            snprintf(baseURL, strlen(url)+1,  "%s", url);//拼接sql字符串
        } else {
            snprintf(baseURL, pEnd+1,  "%s", url);//拼接sql字符串
        }

        flag=spider_queue_set(baseURL, URLDoIn);
        if(flag == FAILURE) {
            *(URLDoIn->rt)=FAILURE;
            return (void *)URLDoIn->rt;
        }

        //返回成功
        *(URLDoIn->rt)=SUCCESS;
        return (void *)URLDoIn->rt;
    } else if(URLDoIn->isPthread==1) {
        for(; ; ) {
            //初始化状态
            if(URLDoIn->isBusy==-1) {
                continue;
            }

            //等待状态
            if(URLDoIn->isBusy==0) {
                continue;
            }

            //结束状态
            if(URLDoIn->isBusy==2) {
                *(URLDoIn->rt)=SUCCESS;
                return (void *)URLDoIn->rt;
            }

            //激活状态
            int flag, flag1;//标识
            char *url;
            int depth;

            //初始化各种变量
            memset(*(data+URLDoIn->id), 0, BIG_DATA_SIZE);
            memset(*(dataTmp+URLDoIn->id), 0, BIG_DATA_SIZE);

            //传入值赋值
            depth=URLDoIn->depth;
            url=URLDoIn->url;

            //格式化网址
            flag=spider_format_url(URLDoIn->url);
            if(flag==FAILURE) {
                *(URLDoIn->rt)=FAILURE;
                URLDoIn->isBusy=0;//把线程更改为可用
                continue;
            }

            //printf("rt:%p, format_url is over!\n", URLDoIn->pt);

            //数据获取
            //flag=spider_data_get(URLDoIn);
            flag=spider_curl_data_get(0, URLDoIn);
            if(flag == FAILURE) {
                *(URLDoIn->rt)=FAILURE;
                URLDoIn->isBusy=0;//把线程更改为可用
                continue;
            }

            //编码转换
            flag=spider_charset_transfer(0, URLDoIn);
            if(flag == FAILURE) {
                *(URLDoIn->rt)=FAILURE;
                URLDoIn->isBusy=0;//把线程更改为可用
                continue;
            }

            //pthread_mutex_lock(&mutexURLDoIn);//线程加锁

            //printf("queue_set Begin!\n");
            //新增队列（使用sqlite）
            //重新定义baseUrl，否则会出现baseUrl:http://www.cnbeta.com/topics/379.htm
            char baseURL[URL_DATA_SIZE];
            //初始化字符串数组
            memset(baseURL, 0, sizeof(baseURL));

            //格式化baseUrl
            int i, pEnd=0;
            for(i=0; i<strlen(url); i++) {
                if(*(url+(strlen(url)-i))=='/') {
                    pEnd=(strlen(url)-i)+1;
                    break;
                }
            }
            //没有取得pEnd值时
            if(pEnd==0) {
                snprintf(baseURL, strlen(url)+1,  "%s", url);//拼接sql字符串
            } else {
                snprintf(baseURL, pEnd+1,  "%s", url);//拼接sql字符串
            }

            flag=spider_queue_set(baseURL, URLDoIn);
            if(flag == FAILURE) {
                *(URLDoIn->rt)=FAILURE;
                URLDoIn->isBusy=0;//把线程更改为可用
                continue;
            }

            *(URLDoIn->rt)=SUCCESS;
            URLDoIn->isBusy=0;//把线程更改为可用
        }
    }
}

/**
 * 初始化lua库
 */
static int lua_init(lua_State **L) {
    *L=NULL;//清空指针
    *L = luaL_newstate();
    if(*L == NULL) {
        printf("Init the lua library error\n");
        return FAILURE;
    }
    luaL_openlibs(*L);
    return SUCCESS;
}

/**
 * 退出lua库
 */
static void lua_exit(lua_State *L) {
    lua_close(L);
}

/*
作用：采集器-内容处理by lua脚本
ToDoIn:要采集的网址等结构体
*/
static int spider_content_do_by_lua(struct ToDo *ToDoIn) {
    int flag;
    int ret;//lua运行后的返回值-数字
    char CDataTmp[256];//lua运行后的返回值-字符串
    char rootPath[128];

    //把lua脚本写入lua文件
    memset(rootPath, 0, sizeof(rootPath));
    snprintf(rootPath, sizeof(rootPath),  "%stmp/luas/spider/%d/lua_func_main.lua", CData->GlobalConfigData.root_path, CData->DBConfData.SpiderData.id);

    //执行lua文件
    if (luaL_dofile(*(L+ToDoIn->id), rootPath)==1) {
        printf("cannot run configuration file: %s\n", lua_tostring(*(L+ToDoIn->id), -1));
        return FAILURE;
    }

    lua_getglobal(*(L+ToDoIn->id), "_main");

    lua_pushstring(*(L+ToDoIn->id), CData->GlobalConfigData.root_path);//参数1-配置
    //拼装组合CData并传参数
    memset(CDataTmp, 0, sizeof(CDataTmp));
    snprintf(CDataTmp, sizeof(CDataTmp),  "{\"%s\":%d, \"%s\":\"%s\"}", "collection_id", CData->DBConfData.SpiderData.id, "root_path", CData->GlobalConfigData.root_path);
    lua_pushstring(*(L+ToDoIn->id), CDataTmp);//参数2-配置
    lua_pushstring(*(L+ToDoIn->id), ToDoIn->url);//参数3-页面数据
    lua_pushstring(*(L+ToDoIn->id), *(data+ToDoIn->id));//参数4-页面数据

    if(flag = lua_pcall(*(L+ToDoIn->id), 4, 1, 0)) {
        printf("lua_pcall error flag %d error '%s'\n", flag, lua_tostring(*(L+ToDoIn->id), -1));
        lua_settop(*(L+ToDoIn->id), 0);
        return FAILURE;
    }

    // 判读返回参数合法性
    if(lua_gettop(*(L+ToDoIn->id)) != 1) {
        printf("Incorrect return number %d\n", lua_gettop(*(L+ToDoIn->id)));
        lua_settop(*(L+ToDoIn->id), 0);
        return FAILURE;
    }
    if(lua_isnumber(*(L+ToDoIn->id), 1)!=1) {
        printf("Incorrect return %d\n", lua_gettop(*(L+ToDoIn->id)));
        lua_settop(*(L+ToDoIn->id), 0);
        return FAILURE;
    }

    //输出返回值
    ret=lua_tonumber(*(L+ToDoIn->id), 1);//第一个返回值，返回状态

    lua_settop(*(L+ToDoIn->id), 0);

    if(ret==0) {
        return SUCCESS;
    } else {
        return FAILURE;
    }
}

/*
作用：多线程采集器-调用脚本采集
ToDoTmp:要采集的网址等结构体
*/
static void *spider_to_do(void *ToDoTmp) {
    struct ToDo *ToDoIn;
    ToDoIn=(struct ToDo *)ToDoTmp;
    //时间获取
    timeNow=time( NULL );
    localtime_r(&timeNow, &local);//开始运行时间

    for(; ; ) {
        //初始化状态
        if(ToDoIn->isBusy==-1) {
            continue;
        }

        //等待状态
        if(ToDoIn->isBusy==0) {
            continue;
        }

        //结束状态
        if(ToDoIn->isBusy==2) {
            *(ToDoIn->rt)=SUCCESS;
            return (void *)ToDoIn->rt;
        }

        //激活状态
        int flag, flag1;//标识
        char *url;
        int depth;

        //初始化各种变量
        memset(*(data+ToDoIn->id), 0, BIG_DATA_SIZE);
        memset(*(dataTmp+ToDoIn->id), 0, BIG_DATA_SIZE);

        //传入值赋值
        url=ToDoIn->url;

        //格式化网址
        flag=spider_format_url(ToDoIn->url);
        if(flag==FAILURE) {
            *(ToDoIn->rt)=FAILURE;
            ToDoIn->isBusy=0;//把线程更改为可用
            continue;
        }

        //数据获取
        flag=spider_curl_data_get(1, ToDoIn);
        if(flag == FAILURE) {
            *(ToDoIn->rt)=FAILURE;
            ToDoIn->isBusy=0;//把线程更改为可用
            continue;
        }

        //编码转换
        flag=spider_charset_transfer(1, ToDoIn);
        if(flag == FAILURE) {
            *(ToDoIn->rt)=FAILURE;
            ToDoIn->isBusy=0;//把线程更改为可用
            continue;
        }

        //采集器-内容处理by lua脚本
        flag=spider_content_do_by_lua(ToDoIn);
        if(flag == FAILURE) {
            *(ToDoIn->rt)=FAILURE;
            ToDoIn->isBusy=0;//把线程更改为可用
            continue;
        }

        *(ToDoIn->rt)=SUCCESS;
        ToDoIn->isBusy=0;//把线程更改为可用
    }
}

/*
作用：网络蜘蛛格式化网址使没有"/"后缀的url加上"/"后缀
url:要格式化的网址
*/
static int spider_format_url(char *url) {
    if(strlen(url)+16>=URL_DATA_SIZE) {
        return FAILURE;
    }

    char *parseptr1;
    char *parseptr2;
    int len, i, flag=0;

    parseptr2 = url;
    //得出协议
    //去除url中不规范的字符串存在某些情况如：http://www.pcunions.com/#, http://www.pcunions.com/soft/#
    //strlen(url)以及包括字符串结束符'\0'
    if(*(url+strlen(url)-1)=='#') {
        *(url+strlen(url)-1)='\0';
    }

    parseptr1 = strchr(parseptr2, ':');
    if ( NULL == parseptr1) {
        printf("format_url ERROR!, url:%s\n", url);
        return FAILURE;
    }
    len = parseptr1 - parseptr2;
    for ( i = 0; i < len; i++) {
        if ( !isalpha(parseptr2[i]) ) {
            printf("format_url ERROR!, url:%s\n", url);
            return FAILURE;
        }
    }
    parseptr1++;
    parseptr2 = parseptr1;
    for ( i = 0; i < 2; i++ ) {
        if ( '/' != *parseptr2 ) {
            printf("format_url ERROR!, url:%s\n", url);
            return FAILURE;
        }
        parseptr2++;
    }

    while(*parseptr2!='\0') {
        if(*parseptr2=='/') {
            //出特殊情况下的所有不需要做任何url操作的情况
            flag=1;
            //排除http://www.pcunions.com/a/和http://www.pcunions.com/时flag==1，但是却不需要做任何操作的情况
            if(*(url+strlen(url)-1)=='/') {
                flag=3;
                break;
            }

            //strcat(url, "/");
            parseptr1=parseptr2;
            while(*parseptr1!='\0') {
                //判断是否有.，通过此来判断是否是类似http://www.pcunions.com/index.html的网址
                if(*parseptr1=='.') {
                    flag=2;
                    //strcat(url, "/");
                }
                //排除类似http://www.pcunions.com/e/web/?type=rss2&classid=0的url
                if(*parseptr1=='?' || *parseptr1=='#') {
                    flag=4;
                }
                parseptr1++;
            }
            break;
        }
        parseptr2++;
    }
    switch(flag) {
    case 0:
        strcat(url, "/");
        break;
    case 1:
        strcat(url, "/");
        break;
    }

    return SUCCESS;
}

/*
作用：网址格式化，把大写变为小写,仅仅只是域名解析时使用
url:要格式化的网址
*/
static int url_to_lower(char *url) {
    char *tmpUrl;
    //把网址的大写都变为小写
    tmpUrl=url;
    while(*tmpUrl!='\0') {
        if(isupper(*tmpUrl)) {
            *tmpUrl=tolower(*tmpUrl);
        }
        tmpUrl++;
    }
    return SUCCESS;
}

/*
作用：解析url
url:要解析的网址
uri:uri对象
*/
static int parse_url(char *url, struct URIDetail *uri) {
    char *parseptr1;
    char *parseptr2;
    char tmpPort[8];
    int len;
    int i, j;
    int flag;//标识

    //初始化操作，一些赋初值操作
    memset(tmpPort, 0, sizeof(tmpPort));
    //memset(tmpUrl, 0, sizeof(tmpUrl));
    //初始化uri结构体
    memset(uri->protocol, 0, sizeof(uri->protocol));
    memset(uri->host, 0, sizeof(uri->host));
    uri->port=0;
    memset(uri->path, 0, sizeof(uri->path));
    memset(uri->query, 0, sizeof(uri->query));
    memset(uri->fragment, 0, sizeof(uri->fragment));

    //纠正网址的格式,最开始已经执行过一次
    /*flag=format_url(url);
    if(flag==FAILURE) {
        printf("URL ERROR!, url:%s\n", url);
        return FAILURE;
    }*/

    //解析
    parseptr2 = url;
    //得出协议
    parseptr1 = strchr(parseptr2, ':');
    if ( NULL == parseptr1) {
        printf("URL ERROR!, url:%s\n", url);
        return FAILURE;
    }
    len = parseptr1 - parseptr2;
    if(len>=sizeof(uri->protocol)) {
        return FAILURE;
    }
    for ( i = 0, j=0; i < len; i++, j++) {
        //字符串过长输出错误,造成字符数组内存撑破，然后占用到len，改变了len的值
        if(j>=sizeof(uri->protocol)-1) {
            printf("URL ERROR!, url:%s\n", url);
            return FAILURE;
        }

        if ( !isalpha(parseptr2[i]) ) {
            printf("URL ERROR!, url:%s\n", url);
            return FAILURE;
        }
        uri->protocol[j]=parseptr2[i];
    }
    parseptr1++;
    parseptr2 = parseptr1;
    for ( i = 0; i < 2; i++ ) {
        if ( '/' != *parseptr2 ) {
            printf("URL ERROR!, url:%s\n", url);
            return FAILURE;
        }
        parseptr2++;
    }
    //得出域名
    parseptr1 = strchr(parseptr2, ':');
    //端口默认时候
    if ( NULL == parseptr1 ) {
        parseptr1 = strchr(parseptr2, '/');
        if ( NULL == parseptr1 ) {
            printf("URL ERROR!, url:%s\n", url);
            return FAILURE;
        }
        len = parseptr1 - parseptr2;
        if(len>=sizeof(uri->host)) {
            printf("URL ERROR!, url:%s\n", url);
            return FAILURE;
        }
        for(i=0, j=0; i<len; i++, j++) {
            //字符串过长输出错误,造成字符数组内存撑破，然后占用到len，改变了len的值
            if(j>=sizeof(uri->host)-1) {
                printf("URL ERROR!, url:%s\n", url);
                return FAILURE;
            }
            uri->host[j]=parseptr2[i];
        }
        //把域名中大写变小写,仅仅只是用于域名
        flag=url_to_lower(uri->host);
        if(flag==FAILURE) {
            printf("URL ERROR!, url:%s\n", url);
            return FAILURE;
        }
        //端口赋值
        uri->port=80;
    } else { //指定端口的时候
        //域名获取
        len = parseptr1 - parseptr2;
        if(len>=sizeof(uri->host)) {
            printf("URL ERROR!, url:%s\n", url);
            return FAILURE;
        }
        for(i=0, j=0; i<len; i++, j++) {
            //字符串过长输出错误,造成字符数组内存撑破，然后占用到len，改变了len的值
            if(j>=sizeof(uri->host)-1) {
                printf("URL ERROR!, url:%s\n", url);
                return FAILURE;
            }
            uri->host[j]=parseptr2[i];
        }
        //把域名中大写变小写,仅仅只是用于域名
        flag=url_to_lower(uri->host);
        if(flag==FAILURE) {
            printf("URL ERROR!, url:%s\n", url);
            return FAILURE;
        }
        //端口号获取
        parseptr1++;
        parseptr2 = parseptr1;
        parseptr1 = strchr(parseptr2, '/');
        if ( NULL == parseptr1 ) {
            printf("URL ERROR!, url:%s\n", url);
            return FAILURE;
        }
        len = parseptr1 - parseptr2;
        for(i=0, j=0; i<len; i++, j++) {
            //字符串过长输出错误,造成字符数组内存撑破，然后占用到len，改变了len的值
            if(j>=sizeof(tmpPort)-1) {
                printf("URL ERROR!, url:%s\n", url);
                return FAILURE;
            }
            //端口不是数字则返回false
            if(parseptr2[i]<48 || parseptr2[i]>57) {
                printf("URL ERROR!, url:%s\n", url);
                return FAILURE;
            }
            tmpPort[j]=parseptr2[i];
        }
        //得到端口号
        uri->port=atoi(tmpPort);
    }

    //得到path
    parseptr1++;
    parseptr2 = parseptr1;
    //循环检测?出现的情况在什么时候
    while ( '\0' != *parseptr1 && '?' != *parseptr1  && '#' != *parseptr1 ) {
        parseptr1++;
    }
    len = parseptr1 - parseptr2;
    if(len>=sizeof(uri->path)) {
        printf("URL ERROR!, url:%s\n", url);
        return FAILURE;
    }
    for(i=0, j=0; i<len; i++, j++) {
        //字符串过长输出错误,造成字符数组内存撑破，然后占用到len，改变了len的值
        if(j>=sizeof(uri->path)-1) {
            printf("URL ERROR!, url:%s\n", url);
            return FAILURE;
        }
        //得到path
        uri->path[j]=parseptr2[i];
    }
    //得到query
    parseptr2=parseptr1;
    //一般只有?号的情况
    if ( '?' == *parseptr1 ) {
        parseptr2++;
        parseptr1 = parseptr2;
        while ( '\0' != *parseptr1 && '#' != *parseptr1 ) {
            parseptr1++;
        }
        len = parseptr1 - parseptr2;
        if(len>=sizeof(uri->query)) {
            printf("URL ERROR!, url:%s\n", url);
            return FAILURE;
        }
        for(i=0, j=0; i<len; i++, j++) {
            //字符串过长输出错误,造成字符数组内存撑破，然后占用到len，改变了len的值
            if(j>=sizeof(uri->query)-1) {
                printf("URL ERROR!, url:%s\n", url);
                return FAILURE;
            }
            //得到query
            uri->query[j]=parseptr2[i];
        }
    }
    //得到fragment
    parseptr2=parseptr1;
    //一般只有#号的情况
    if ( '#' == *parseptr1 ) {
        parseptr2++;
        parseptr1 = parseptr2;
        //'\0'代表结束符，比如网址结束
        while ( '\0' != *parseptr1 ) {
            parseptr1++;
        }
        len = parseptr1 - parseptr2;
        if(len>=sizeof(uri->fragment)) {
            printf("URL ERROR!, url:%s\n", url);
            return FAILURE;
        }
        for(i=0, j=0; i<len; i++, j++) {
            //字符串过长输出错误,造成字符数组内存撑破，然后占用到len，改变了len的值
            if(j>=sizeof(uri->fragment)-1) {
                printf("URL ERROR!, url:%s\n", url);
                return FAILURE;
            }
            //得到fragment
            uri->fragment[j]=parseptr2[i];
        }
    }

    return SUCCESS;
}

/*
作用：网络蜘蛛从网络获取内容
URLDoIn:urlDo对象，存储各种获取内容所需参数
*/
static int spider_data_get(struct URLDo *URLDoIn) {
    //recv
    //由于接受大数据的时候需要一点一点的接收，所以需要一个循环
    //请求超时则退出
    struct timeval timeOld;
    struct timeval timeNew;
    gettimeofday(&timeOld, NULL);

    int ret, sfp, flag, j;
    struct sockaddr_in s_addr;
    struct hostent *host;
    int dataSize;//字符串偏移量，用于memcpy
    //结构体初始化，不初始化的话会乱码
    memset(&s_addr, 0, sizeof(s_addr));

    //临时字符串存储数组，recv遇到大数据是一点一点传过来的，传过来的数据暂时存储在这里
    char tmpStr[1024];
    //初始化字符串数组
    memset(tmpStr, 0, sizeof(tmpStr));

    //通过baseurl得到uri
    struct URIDetail uri;
    flag=parse_url(URLDoIn->url, &uri);
    if(flag==FAILURE) {
        return FAILURE;
    }

    //查询DNS,会出现超时现象，超时或者返回错误则返回failure
    for(; ; ) {
        //超时则返回failure
        gettimeofday(&timeNew, NULL);
        if((timeNew.tv_sec-timeOld.tv_sec)>CData->DBConfData.SpiderData.socket_connect_time_out) {
            return FAILURE;
        }
        //返回null则继续无限循环获取
        host = gethostbyname(uri.host);
        if(host==NULL) {
            continue;
        } else {
            break;
        }
    }

    //socket部分开始
    //socket
    if ((sfp = socket(AF_INET, SOCK_STREAM, 0))==-1) {
        printf("socket error!\n");
        return FAILURE;
    }

    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(uri.port);
    s_addr.sin_addr = *((struct in_addr*)host->h_addr);

    int flags = fcntl(sfp,F_GETFL,0);
    fcntl(sfp, F_SETFL, flags|O_NONBLOCK);
    for(; ; ) {
        //超时则返回failure
        gettimeofday(&timeNew, NULL);
        if((timeNew.tv_sec-timeOld.tv_sec)>CData->DBConfData.SpiderData.socket_connect_time_out) {
            return FAILURE;
        }

        ret=connect(sfp, (struct sockaddr *)&s_addr, sizeof(s_addr));
        if(ret==0) {
            break;
        }
    }

    //header请求
    //格式化header
    char header[1024];
    //初始化字符串数组
    memset(header, 0, sizeof(header));
    strcat(header, "GET ");
    strcat(header, URLDoIn->url);
    //strcat(header, "\r\n");
    strcat(header, " HTTP/1.1\r\n");
    strcat(header, "HOST: ");
    strcat(header, uri.host);
    strcat(header, "\r\nAccept:*/*");
    //strcat(header, "\r\n");
    //strcat(header, "Pragma:no-cache\r\n");
    //strcat(header, "Referer:http://");
    //strcat(header, uri.host);
    //strcat(header, "\r\n");
    strcat(header, "\r\nUser-Agent: DaoSpider/4.0");
    //strcat(header, "\r\nUser-Agent: Mozilla/5.0 (Windows NT 5.2) AppleWebKit/534.30 (KHTML, like Gecko) Chrome/12.0.742.122 Safari/534.30");
    //strcat(header, "\r\nUser-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64; rv:44.0) Gecko/20100101 Firefox/44.0");
    //strcat(header, "\r\nUser-Agent: curl/7.35.0");
    //strcat(header, "Range:bytes");
    strcat(header, "\r\nConnection: keep-alive");
    //strcat(header, "\r\nConnection: close");
    strcat(header, "\r\n\r\n");

    //sprintf(header, "GET %s\r\nUser-Agent: daospider/1.0\r\nHost: %s\r\nAccept: text/html; */*\r\nConnection: Keep-Alive\r\n\r\n", url, server.host);
    //send
    ret = send(sfp, header, strlen(header)+1, 0);
    if (ret <= 0) {
        printf("send error!\n");
        return FAILURE;
    }

    for(; ; ) {
        //超时则返回failure
        gettimeofday(&timeNew, NULL);
        if((timeNew.tv_sec-timeOld.tv_sec)>CData->DBConfData.SpiderData.socket_connect_time_out) {
            return FAILURE;
        }

        ret = send(sfp, header, strlen(header)+1, 0);
        if(ret>0) {
            break;
        }
    }

    dataSize=0;
    j=0;
    memset(*(data+URLDoIn->id), 0, BIG_DATA_SIZE);//初始化
    while(1) {
        //数据量太大则放弃数据
        if(dataSize>=(BIG_DATA_SIZE-1)) {
            return FAILURE;
        }

        gettimeofday(&timeNew, NULL);
        if((timeNew.tv_sec-timeOld.tv_sec)<=CData->DBConfData.SpiderData.socket_time_out) {
            if(j==1) {
                break;
            }

            for(; ; ) {
                //初始化字符串数组
                memset(tmpStr, 0, sizeof(tmpStr));
                //tmpStr必须强制转换为void *类型，否则会多出一些字符串，因为本质上来说，接收的是无类型的数据，而不是字符串。
                ret=recv(sfp, (void *)tmpStr, 1024, 0);
                if(ret>0) {
                    memcpy(*(data+URLDoIn->id)+dataSize, tmpStr, ret);
                    dataSize=dataSize+ret;
                    break;
                }

                if(ret==0) {
                    j=1;
                    break;
                }
            }
        } else {
            //初始化字符串数组
            memset(tmpStr, 0, sizeof(tmpStr));
            return FAILURE;
        }
    }
    //close
    close(sfp);

    //转换为字符串,加上'\0'结束符
    *(*(data+URLDoIn->id)+dataSize+1)='\0';

    //假设获取数据为空则返回failure
    if(strlen(*(data+URLDoIn->id))==0) {
        return FAILURE;
    } else {
        return SUCCESS;
    }
}

/*
作用：网络蜘蛛curl的回掉函数
ptr:获取的数据
size:获取的数据大小
nmemb:获取的数据数量
stream:传入参数
*/
static size_t spider_curl_write_data_callback(void *ptr, size_t nsize, size_t nmemb, void *stream) {
    int flag;//标识符
    //得到URLDoObject
    struct CURLURLDo *CURLURLDo=(struct CURLURLDo *)stream;

    if(CURLURLDo->dataSize+nsize*nmemb>(BIG_DATA_SIZE-1)) {
        return FAILURE;
    }

    memcpy(*(data+CURLURLDo->URLDo->id)+CURLURLDo->dataSize, ptr, nsize*nmemb);

    //更改数据大小
    CURLURLDo->dataSize=CURLURLDo->dataSize+nsize*nmemb;

    return nsize*nmemb;//必须返回这个大小, 否则只回调一次, 不清楚为何.
}

/*
作用：网络蜘蛛从网络获取内容--curl库的调用，获取更加高效，且支持多协议不会报错
bOrl:判断是queue-url还是o-url，0是queue，1是to
DoIn:传入的对象，存储各种获取内容所需参数
*/
static int spider_curl_data_get(int bOrl, void *DoIn) {
    if(bOrl==0) {
        struct URLDo *URLDoIn=(struct URLDo *)DoIn;

        /*curl*/
        CURL *curl;
        CURLcode res;
        int flag;//标识符
        long httpCode = 0;
        char cookiePath[256];//定义cookie文件路径
        char path[256];//cookie所在目录

        struct CURLURLDo CURLURLDo;//curl传递参数
        CURLURLDo.dataSize=0;//socket复制的数据大小
        CURLURLDo.URLDo=URLDoIn;//URLDo赋值
        struct curl_slist *cookiesList = NULL;//cookie
        struct curl_slist *nc;
        char *urlTmp;

        //初始化字符串数组
        memset(cookiePath, 0, sizeof(cookiePath));
        memset(path, 0, sizeof(path));

        //通过baseurl得到rooturl
        struct URIDetail uri;
        flag=parse_url(URLDoIn->url, &uri);
        if(flag==FAILURE) {
            return FAILURE;
        }
        if(strlen(CData->GlobalConfigData.root_path)+strlen("tmp/cookie/spider/")+12+1>sizeof(path)) {
            return FAILURE;
        }
        snprintf(path, sizeof(path),  "%stmp/cookie/spider/%d/", CData->GlobalConfigData.root_path, CData->DBConfData.SpiderData.id);//设置默认cookie文件
        //创建path
        //判断目录是否存在,不存在则建立此目录
        if(access(path, F_OK)<0) {
            flag=make_dir(path);
            if(flag<0) {
                return FAILURE;
            }
        }
        if(strlen(CData->GlobalConfigData.root_path)+strlen("tmp/cookie/spider/")+12+strlen(uri.host)+strlen(".txt")+1>sizeof(cookiePath)) {
            return FAILURE;
        }
        snprintf(cookiePath, sizeof(cookiePath),  "%stmp/cookie/spider/%d/%s.txt", CData->GlobalConfigData.root_path, CData->DBConfData.SpiderData.id, uri.host);//设置默认cookie文件

        curl = curl_easy_init();
        if(!curl) {
            return FAILURE;
        }

        curl_easy_setopt(curl, CURLOPT_URL, URLDoIn->url); //设置下载地址
        //不兼容CURLOPT_COOKIEFILE， CURLOPT_COOKIEJAR等
        struct curl_slist *headers = NULL;
        //headers = curl_slist_append(headers,"User-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64; rv:48.0) Gecko/20100101 Firefox/48.0");
        //headers = curl_slist_append(headers,"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
        //headers = curl_slist_append(headers,"Accept-Language: zh-CN,zh;q=0.8,en-US;q=0.5,en;q=0.3");
        //headers = curl_slist_append(headers,"Connection: keep-alive");
        //headers = curl_slist_append(headers,"Cache-Control: max-age=0");

        headers = curl_slist_append(headers,"User-Agent: Googlebot/2.1 (+http://www.googlebot.com/bot.html)");

        //headers = curl_slist_append(headers, "Upgrade-Insecure-Requests: 1");
        if(strcmp(uri.protocol, "https")==0) {
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L); //libcurl将不会产生任何信号,防止libcurl在多线程中crash
        curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1);//如果没有新的TCP请求来重用这个连接，那么只能等到CLOSE_WAIT超时，这个时间默认在7200秒甚至更高，太多的CLOSE_WAIT连接会导致性能问题
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);//抓取跳转页面
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)CData->DBConfData.SpiderData.socket_time_out);//设置超时时间，获取数据等待
        curl_easy_setopt(curl, CURLOPT_COOKIEJAR, cookiePath);//设置cookie默认存储位置-存储
        curl_easy_setopt(curl, CURLOPT_COOKIEFILE, cookiePath);//设置cookie默认存储位置-取出使用
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, (long)CData->DBConfData.SpiderData.socket_connect_time_out);//设置超时时间，连接等待
        //curl_easy_setopt(curl, CURLOPT_USERAGENT, "DaoSpider/4.0");
        //curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; WOW64; rv:48.0) Gecko/20100101 Firefox/48.0");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, spider_curl_write_data_callback);//设置写数据的函数
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &CURLURLDo);//设置写数据的变量

        res = curl_easy_perform(curl);//执行下载

        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);//获取状态码
        curl_slist_free_all(headers);//释放headers

        if(res!=CURLE_OK) {
            printf("URL>>[%s]GETTING ERROR, res:%d!\n", URLDoIn->url, res);
            //清理url
            curl_easy_cleanup(curl);//释放curl资源
            return FAILURE;
        }

        if(httpCode!=200) {
            printf("URL>>[%s]%ld ERROR!\n", URLDoIn->url, httpCode);
            //清理url
            curl_easy_cleanup(curl);//释放curl资源
            return FAILURE;
        }

        //关闭curl
        curl_easy_cleanup(curl);//释放curl资源
        //转换为字符串,加上'\0'结束符
        *(*(data+CURLURLDo.URLDo->id)+CURLURLDo.dataSize+1)='\0';

        if(strlen(*(data+CURLURLDo.URLDo->id))==0) {
            return FAILURE;
        } else {
            return SUCCESS;
        }
    } else if(bOrl==1) {
        struct ToDo *ToDoIn=(struct ToDo *)DoIn;

        /*curl*/
        CURL *curl;
        CURLcode res;
        int flag;//标识符
        long httpCode = 0;
        char cookiePath[256];//定义cookie文件路径
        char path[256];//cookie所在目录

        struct CURLToDo CURLToDo;//curl传递参数
        CURLToDo.dataSize=0;//socket复制的数据大小
        CURLToDo.ToDo=ToDoIn;//ToDo赋值
        struct curl_slist *cookiesList = NULL;//cookie
        struct curl_slist *nc;
        char *urlTmp;

        //初始化字符串数组
        memset(cookiePath, 0, sizeof(cookiePath));
        memset(path, 0, sizeof(path));

        //通过baseurl得到rooturl
        struct URIDetail uri;
        flag=parse_url(ToDoIn->url, &uri);
        if(flag==FAILURE) {
            return FAILURE;
        }
        if(strlen(CData->GlobalConfigData.root_path)+strlen("tmp/cookie/spider/")+12+1>sizeof(path)) {
            return FAILURE;
        }
        snprintf(path, sizeof(path),  "%stmp/cookie/spider/%d/", CData->GlobalConfigData.root_path, CData->DBConfData.SpiderData.id);//设置默认cookie文件
        //创建path
        //判断目录是否存在,不存在则建立此目录
        if(access(path, F_OK)<0) {
            flag=make_dir(path);
            if(flag<0) {
                return FAILURE;
            }
        }
        if(strlen(CData->GlobalConfigData.root_path)+strlen("tmp/cookie/spider/")+12+strlen(uri.host)+strlen(".txt")+1>sizeof(cookiePath)) {
            return FAILURE;
        }
        snprintf(cookiePath, sizeof(cookiePath),  "%stmp/cookie/spider/%d/%s.txt", CData->GlobalConfigData.root_path, CData->DBConfData.SpiderData.id, uri.host);//设置默认cookie文件

        curl = curl_easy_init();
        if(!curl) {
            return FAILURE;
        }

        curl_easy_setopt(curl, CURLOPT_URL, ToDoIn->url); //设置下载地址
        //不兼容CURLOPT_COOKIEFILE， CURLOPT_COOKIEJAR等
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers,"User-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64; rv:48.0) Gecko/20100101 Firefox/48.0");
        headers = curl_slist_append(headers,"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
        headers = curl_slist_append(headers,"Accept-Language: zh-CN,zh;q=0.8,en-US;q=0.5,en;q=0.3");
        headers = curl_slist_append(headers,"Connection: keep-alive");
        headers = curl_slist_append(headers,"Cache-Control: max-age=0");
        //headers = curl_slist_append(headers, "Upgrade-Insecure-Requests: 1");
        if(strcmp(uri.protocol, "https")==0) {
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L); //libcurl将不会产生任何信号,防止libcurl在多线程中crash
        curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1);//如果没有新的TCP请求来重用这个连接，那么只能等到CLOSE_WAIT超时，这个时间默认在7200秒甚至更高，太多的CLOSE_WAIT连接会导致性能问题
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);//抓取跳转页面
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)CData->DBConfData.SpiderData.socket_time_out);//设置超时时间，获取数据等待
        curl_easy_setopt(curl, CURLOPT_COOKIEJAR, cookiePath);//设置cookie默认存储位置-存储
        curl_easy_setopt(curl, CURLOPT_COOKIEFILE, cookiePath);//设置cookie默认存储位置-取出使用
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, (long)CData->DBConfData.SpiderData.socket_connect_time_out);//设置超时时间，连接等待
        //curl_easy_setopt(curl, CURLOPT_USERAGENT, "DaoSpider/4.0");
        //curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; WOW64; rv:48.0) Gecko/20100101 Firefox/48.0");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, spider_curl_write_data_callback);//设置写数据的函数
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &CURLToDo);//设置写数据的变量

        res = curl_easy_perform(curl);//执行下载

        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);//获取状态码
        curl_slist_free_all(headers);//释放headers

        if(res!=CURLE_OK) {
            printf("URL>>[%s]GETTING ERROR, res:%d!\n", ToDoIn->url, res);
            //清理url
            curl_easy_cleanup(curl);//释放curl资源
            return FAILURE;
        }

        if(httpCode!=200) {
            printf("URL>>[%s]%ld ERROR!\n", ToDoIn->url, httpCode);
            //清理url
            curl_easy_cleanup(curl);//释放curl资源
            return FAILURE;
        }

        //关闭curl
        curl_easy_cleanup(curl);//释放curl资源
        //转换为字符串,加上'\0'结束符
        *(*(data+CURLToDo.ToDo->id)+CURLToDo.dataSize+1)='\0';

        if(strlen(*(data+CURLToDo.ToDo->id))==0) {
            return FAILURE;
        } else {
            return SUCCESS;
        }
    } else {
        return FAILURE;
    }
}

/*
获取url上一级目录
url:待获取url上一级目录的url
k:获取上一级目录层数
*/
static int get_dirname(char *url, int k) {
    int i, j, p;
    char urlTmp[URL_DATA_SIZE];

    if(strlen(url)+1>URL_DATA_SIZE){
        return FAILURE;
    }

    //如果上一级的层数过多，超过url本身的最大值则退出
    for(i=0, j=0; i<strlen(url)+1; i++){
        if(*(url+i)=='/'){
            j++;
        }
    }
    if(k>(j-3)){
        return FAILURE;
    }


    for(p=0; p<k; p++) {
        for(i=0, j=0; i<strlen(url)+1; i++) {
            if(*(url+strlen(url)-1-i)=='/') {
                j++;
            }
            if(j==2){
                //初始化字符串数组
                memset(urlTmp, 0, sizeof(urlTmp));
                strncpy(urlTmp, url, strlen(url)-i);
                strncpy(url, urlTmp, strlen(urlTmp)+1);
                break;
            }
        }
    }

    return SUCCESS;
}

/*
网络蜘蛛格式化入queue的url,检测匹配的链接是"/","a/index.html"还是"http://www.a.com/"，并把网址格式化为标准地址，如把"a/index.html"格式化为"http://www.b.com/a/index.html"
url:待格式化的queue_url
baseUrl：队列此时的基本url
*/
static int spider_format_queue_url(char *url, char *baseURL) {
    int flag;//标识符
    char urlTmp[URL_DATA_SIZE];
    char rootURL[URL_DATA_SIZE];
    char *parseptr1;
    char *parseptr2;
    int i, j, k;

    //初始化字符串数组
    memset(rootURL, 0, sizeof(rootURL));

    //通过baseurl得到rooturl
    struct URIDetail uri;
    flag=parse_url(baseURL, &uri);
    if(flag==FAILURE) {
        return FAILURE;
    }
    //获取rooturl
    if(strlen(uri.protocol)+strlen(uri.host)+16>=URL_DATA_SIZE) {
        return FAILURE;
    }
    if(uri.port==80) {
        snprintf(rootURL, URL_DATA_SIZE, "%s://%s", uri.protocol, uri.host);
    } else {
        snprintf(rootURL, URL_DATA_SIZE, "%s://%s:%d", uri.protocol, uri.host, uri.port);
    }

    if(strlen(url)+strlen(baseURL)+strlen(rootURL)+16>=URL_DATA_SIZE) {
        return FAILURE;
    }

    //格式化上一级目录../, 必须在格式化./之前，否则会出现codeblocks/tickets/285/.././././a.html格式化出来是codeblocks/tickets/285/.a.html
    //初始化字符串数组
    memset(urlTmp, 0, sizeof(urlTmp));
    for(i=0, j=0, k=0; i<strlen(url)+1; i++) {
        if(*(url+i)=='.' && *(url+i+1)=='.' && *(url+i+1+1)=='/') {
            k++;
            i+=2;
            continue;
        } else {
            urlTmp[j]=*(url+i);
            j++;
        }
    }
    urlTmp[strlen(urlTmp)]='\0';//加结束符
    strncpy(url, urlTmp, strlen(urlTmp)+1);
    //获取上k级目录
    flag=get_dirname(baseURL, k);
    if(flag==FAILURE) {
        return FAILURE;
    }

    //格式化此级目录./
    //初始化字符串数组
    memset(urlTmp, 0, sizeof(urlTmp));
    for(i=0, j=0; i<strlen(url)+1; i++) {
        if(*(url+i)=='.' && *(url+i+1)!='.' && *(url+i+1)=='/') {
            i++;
            continue;
        } else {
            urlTmp[j]=*(url+i);
            j++;
        }
    }
    urlTmp[strlen(urlTmp)]='\0';//加结束符
    strncpy(url, urlTmp, strlen(urlTmp)+1);

    //初始化字符串数组
    memset(urlTmp, 0, sizeof(urlTmp));
    //检测url是否入库,排除非http以及https的url, 排除JavaScript:test(a);
    parseptr2 = url;
    //得出协议
    parseptr1 = strchr(parseptr2, ':');
    if(parseptr1!=NULL) {
        //检测如果不是http://或者:8080等类似情况时返回failure
        if(*(parseptr1+1)!='/' && isdigit(*(parseptr1+1))==FALSE) {
            return FAILURE;
        }
    }
    //现在只支持http:,其他的都不支持如：ed2k:等
    if(parseptr1!=NULL) {
        //检测如果不是http://或者:8080等类似情况时返回failure
        if(*(parseptr1-1)!='p' && *(parseptr1-1)!='P') {
            return FAILURE;
        }
    }

    //判断字符串开头是否是'#'
    if(*url=='#') {
        snprintf(url, URL_DATA_SIZE, "%s", baseURL);
    }

    if(*url=='/') {
        //strlen(rootURL)要加1，因为要一个'\0'结束符
        snprintf(urlTmp, strlen(rootURL)+1, "%s", rootURL);
        //url+1是因为要除去里面的"/"
        strncat(urlTmp, url, strlen(url)+1);
        snprintf(url, URL_DATA_SIZE, "%s", urlTmp);
    } else {
        int flag=0;

        parseptr2 = url;
        //得出协议
        parseptr1 = strchr(parseptr2, ':');
        if(parseptr1!=NULL) {
            flag=1;
            if(*(parseptr1+1)=='/') {
                flag=2;
            } else {
                flag=0;
            }
        }
        //标志flag==2代表url具有字符串"://",表明url是完整的url
        if(flag==0) {
            snprintf(urlTmp, strlen(baseURL)+1, "%s", baseURL);
            //url+1是因为要除去里面的"/",只要是snprintf之类的函数一定要注意加1，因为它会自动添加一个'\0'
            strncat(urlTmp, url, strlen(url)+1);
            snprintf(url, URL_DATA_SIZE, "%s", urlTmp);
        }

        //flag==1恰好是javascript:这一种情况
        if(flag==1) {
            snprintf(url, URL_DATA_SIZE, "%s", baseURL);
        }
    }

    //格式化网址
    flag=spider_format_url(url);
    if(flag==FAILURE) {
        return FAILURE;
    }

    //初始化字符串数组
    memset(urlTmp, 0, sizeof(urlTmp));
    for(i=0, j=0; i<strlen(url)+1; i++) {
        //超过urlTmp的存储空间则返回failure
        if(j>=(sizeof(urlTmp)-1)) {
            return FAILURE;
        }

        if(*(url+i)!=' ') {
            urlTmp[j]=*(url+i);
            j++;
        }
    }
    urlTmp[strlen(urlTmp)]='\0';//加结束符
    strncpy(url, urlTmp, strlen(urlTmp)+1);

    return SUCCESS;
}

/*
网络蜘蛛检测url与start_url的domain是否相同
url:队列此时的基本url
*/
static int spider_url_is_include_domain(char *url) {
    struct URIDetail uriDomain;
    struct URIDetail uriURL;
    int flag;//标识符

    //得到url的URI结构
    flag=parse_url(url, &uriURL);
    if(flag==FAILURE) {
        return FAILURE;
    }

    //判断域名是否符合条件
    char *p;
    p=strstr(uriURL.host, CData->DBConfData.SpiderData.domain);
    if(p!=NULL) {
        return SUCCESS;
    } else {
        return FAILURE;
    }
    /*if(strcmp(uriURL.host, uriDomain.host)==0) {
        return SUCCESS;
    } else {
        return FAILURE;
    }*/
}

/*
网络蜘蛛过滤出内容中的队列url并且url入库（入队列）
baseUrl:队列此时的基本url
URLDoIn:URLDoIn对象，存储各种获取内容所需参数
*/
static int spider_queue_set(char *baseURL, struct URLDo *URLDoIn) {
    //参数分别是正则表达式， 要匹配的字符串， 要返回的正则后的偏移算法
    pcre *re;
    const char *error;
    int erroffset;
    int ovector[6];
    int rc, i, j;
    int exec_offset=0;
    char queueURL[URL_DATA_SIZE];//queueURL存储
    char queueURLTmp[URL_DATA_SIZE];//queueURL存储
    int flag;//标识符

    re = pcre_compile("<a.*href=.*[\",'](.*)[\",'].*>", PCRE_CASELESS|PCRE_UNGREEDY|PCRE_DOTALL, &error, &erroffset, NULL);
    if (re == NULL) {
        return FAILURE;
    }

    do {
        //初始化字符串数组
        memset(queueURL, 0, sizeof(queueURL));
        memset(queueURLTmp, 0, sizeof(queueURLTmp));

        rc = pcre_exec(re,    // code, 输入参数，用pcre_compile编译好的正则表达结构的指针
                       NULL,  // extra, 输入参数，用来向pcre_exec传一些额外的数据信息的结构的指针
                       *(data+URLDoIn->id),   // subject, 输入参数，要被用来匹配的字符串
                       strlen(*(data+URLDoIn->id))+1,  // length, 输入参数，要被用来匹配的字符串的指针
                       exec_offset,     // startoffset, 输入参数，用来指定subject从什么位置开始被匹配的偏移量
                       0,     // options, 输入参数，用来指定匹配过程中的一些选项
                       ovector,        // ovector, 输出参数，用来返回匹配位置偏移量的数组
                       6);    // ovecsize, 输入参数， 用来返回匹配位置偏移量的数组的最大大小
        // 返回值：匹配成功返回非负数，没有匹配返回负数
        if (rc < 0) {
            pcre_free(re);   // 编译正则表达式re 释放内存

            //时间获取
            timeNow=time( NULL );
            localtime_r(&timeNow, &local);//开始运行时间

            printf("[%d][%d]Queue>>URL>>[%s]Has Getted, Time:[%d-%d-%d %d:%d:%d]!\n", CData->DBConfData.SpiderData.id, URLDoIn->depth, URLDoIn->url, (local.tm_year+1900), (local.tm_mon+1), local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec);

            //返回值
            return SUCCESS;
        }

        //赋值到charset
        if(ovector[3]-ovector[2]+1>URL_DATA_SIZE) {
            return FAILURE;
        }
        snprintf(queueURLTmp, ovector[3]-ovector[2]+1, "%s", *(data+URLDoIn->id)+ovector[2]);

        //去除数组中的字符空白
        for(i=0, j=0; i<strlen(queueURLTmp); i++) {
            //字符串过长输出错误,造成字符数组内存撑破
            if(j>=sizeof(queueURLTmp)-1) {
                return FAILURE;
            }
            if(queueURLTmp[i]!=' ') {
                queueURL[j]=queueURLTmp[i];
                j++;
            }
        }

        //格式化url
        flag=spider_format_queue_url(queueURL, baseURL);
        if(flag==FAILURE) {
            //改变位移量
            exec_offset=ovector[1];
            continue;
        }

        //判断是否is_lock_start_url_domain==1（是否锁定起始url的域名）
        if(CData->DBConfData.SpiderData.is_lock_domain==1) {
            flag=spider_url_is_include_domain(queueURL);
            if(flag==FAILURE) {
                //改变位移量
                exec_offset=ovector[1];
                continue;
            }
        }

        //推理是否符合条件,用于排除一些特殊URL，如http://www.xxx.com/a.zip
        flag=spider_queue_url_filter(queueURL, CData->DBConfData.SpiderData.queue_url_filter_pattern);
        if(flag==SUCCESS) {
            //改变位移量
            exec_offset=ovector[1];
            continue;
        }

        //存储到队列中
        flag=spider_queue_url_to_queue(queueURL, URLDoIn->depth);
        if(flag==FAILURE) {
            //改变位移量
            exec_offset=ovector[1];
            continue;
        }

        //推理是否符合条件
        flag=spider_to_url_filter(queueURL, CData->DBConfData.SpiderData.to_url_filter_pattern);
        if(flag==FAILURE) {
            //改变位移量
            exec_offset=ovector[1];
            continue;
        }

        //数据入库
        flag=spider_to_url_to_db(queueURL);
        //数据入库失败
        if(flag == FAILURE) {
            //改变位移量
            exec_offset=ovector[1];
            continue;
        }

        printf("[%d][%d]To>>URL>>[%s]Has Founded!\n", CData->DBConfData.SpiderData.id, URLDoIn->depth, queueURL);

        //改变位移量
        exec_offset=ovector[1];
    } while(rc>0);

    pcre_free(re);   // 编译正则表达式re 释放内存

    //返回值
    return SUCCESS;
}

/*
网络蜘蛛队列url的推理
queueUrl:判断的url
pattern:正则表达式
*/
static int spider_queue_url_filter(char *queueURL, char *pattern) {
    //参数分别是正则表达式， 要匹配的字符串， 要返回的正则后的偏移算法
    pcre *re;
    const char *error;
    int erroffset;
    int ovector[4];
    int rc;
    int exec_offset=0;
    re = pcre_compile(pattern, PCRE_CASELESS|PCRE_UNGREEDY|PCRE_DOTALL, &error, &erroffset, NULL);
    if (re == NULL) {
        return FAILURE;
    }

    rc = pcre_exec(re,    // code, 输入参数，用pcre_compile编译好的正则表达结构的指针
                   NULL,  // extra, 输入参数，用来向pcre_exec传一些额外的数据信息的结构的指针
                   queueURL,   // subject, 输入参数，要被用来匹配的字符串
                   strlen(queueURL)+1,  // length, 输入参数，要被用来匹配的字符串的指针
                   exec_offset,     // startoffset, 输入参数，用来指定subject从什么位置开始被匹配的偏移量
                   0,     // options, 输入参数，用来指定匹配过程中的一些选项
                   ovector,        // ovector, 输出参数，用来返回匹配位置偏移量的数组
                   4);    // ovecsize, 输入参数， 用来返回匹配位置偏移量的数组的最大大小
    // 返回值：匹配成功返回非负数，没有匹配返回负数
    if (rc < 0) {
        pcre_free(re);   // 编译正则表达式re 释放内存
        return FAILURE;
    }

    pcre_free(re);   // 编译正则表达式re 释放内存
    return SUCCESS;
}

/*
网络蜘蛛queue_url入库
url:待入库的url
depth:队列深度
*/
static int spider_queue_url_to_queue(char *url, int depth) {
    int flag;//标识

    //队列set
    flag=dao_queue_set(&queueFlagSet, depth, url);
    //队列报错，返回false
    if(flag==DAOFAILURE) {
        return FAILURE;
    }
    //队列已满，返回2
    if(flag==2) {
        //goto跳转
queueFlagSetSetFlag:

        pthread_mutex_lock(&mutexURLDo);//阻塞线程
        DBOperationQueue.setFlag=1;//变更状态
        pthread_mutex_unlock(&mutexURLDo);//解开阻塞线程

        //在队列没有处理完时阻塞
        for(; ; ) {
            if(DBOperationQueue.setFlag==0) {
                break;
            }
        }
        //补录此数据
        flag=dao_queue_set(&queueFlagSet, depth, url);
        //队列报错，返回false
        if(flag==DAOFAILURE) {
            return FAILURE;
        }
        //队列已满，返回2
        if(flag==2) {
            goto queueFlagSetSetFlag;
        }

        return 2;
    }

    return SUCCESS;
}

/*
网络蜘蛛编码转换
bOrl:判断是queue-url还是o-url，0是queue，1是to
DoIn:DoIn对象，存储各种获取内容所需参数
*/
static int spider_charset_transfer(int bOrl, void *DoIn) {
    if(bOrl==0) {
        struct URLDo *URLDoIn=(struct URLDo *)DoIn;
        //参数分别是正则表达式， 要匹配的字符串， 要返回的正则后的偏移算法
        pcre *re;
        const char *error;
        int erroffset;
        int ovector[6];
        int rc, i, j;
        int exec_offset=0;
        char charset[16];//页面编码
        char charsetTmp[16];//页面编码临时存储
        int flag;//标识符

        //初始化字符串数组
        memset(charset, 0, sizeof(charset));
        memset(charsetTmp, 0, sizeof(charsetTmp));

        re = pcre_compile("<meta.*charset=(.*)>", PCRE_CASELESS|PCRE_UNGREEDY|PCRE_DOTALL, &error, &erroffset, NULL);
        if (re == NULL) {
            return FAILURE;
        }

        rc = pcre_exec(re,    // code, 输入参数，用pcre_compile编译好的正则表达结构的指针
                       NULL,  // extra, 输入参数，用来向pcre_exec传一些额外的数据信息的结构的指针
                       *(data+URLDoIn->id),   // subject, 输入参数，要被用来匹配的字符串
                       strlen(*(data+URLDoIn->id))+1,  // length, 输入参数，要被用来匹配的字符串的指针
                       exec_offset,     // startoffset, 输入参数，用来指定subject从什么位置开始被匹配的偏移量
                       0,     // options, 输入参数，用来指定匹配过程中的一些选项
                       ovector,        // ovector, 输出参数，用来返回匹配位置偏移量的数组
                       6);    // ovecsize, 输入参数， 用来返回匹配位置偏移量的数组的最大大小
        // 返回值：匹配成功返回非负数，没有匹配返回负数
        if (rc < 0) {
            flag=0;
        } else {
            flag=1;
        }
        if(flag==1) {
            if(ovector[3]-ovector[2]+1>=16) {
                return FAILURE;
            }
            //赋值到charset
            snprintf(charsetTmp, ovector[3]-ovector[2]+1, "%s", *(data+URLDoIn->id)+ovector[2]);

            //去除数组中的字符空白
            for(i=0, j=0; i<strlen(charsetTmp); i++) {
                //字符串过长输出错误,造成字符数组内存撑破
                if(j>=sizeof(charset)-1) {
                    return FAILURE;
                }
                if(charsetTmp[i]!=' ' && charsetTmp[i]!='"'  && charsetTmp[i]!='/') {
                    charset[j]=charsetTmp[i];
                    j++;
                }
            }

            //无法识别编码则不收录
            if(strlen(charset)<=0) {
                return FAILURE;
            }
        } else {
            strncpy(charset, CData->DBConfData.SpiderData.charset, strlen(CData->DBConfData.SpiderData.charset)+1);
        }

        pcre_free(re);   // 编译正则表达式re 释放内存

        //c为gbk时候转码，其他都不转
        if(charset[0]=='G' || charset[0]=='g') {
            if(2*strlen(*(data+URLDoIn->id))>=(BIG_DATA_SIZE-1)) {
                return FAILURE;
            }
            /*格式转换*/
            flag=g2u(*(data+URLDoIn->id), strlen(*(data+URLDoIn->id))+1, *(dataTmp+URLDoIn->id), BIG_DATA_SIZE);
            //status=u2g(&obj, &obj1);
            if(flag!=0) {
                return FAILURE;
            }
            memset(*(data+URLDoIn->id), 0, BIG_DATA_SIZE);

            //重新赋值
            strncpy(*(data+URLDoIn->id), *(dataTmp+URLDoIn->id), strlen(*(dataTmp+URLDoIn->id))+1);
            memset(*(dataTmp+URLDoIn->id), 0, BIG_DATA_SIZE);
        }

        return SUCCESS;
    } else if(bOrl==1) {
        struct ToDo *ToDoIn=(struct ToDo *)DoIn;
        //参数分别是正则表达式， 要匹配的字符串， 要返回的正则后的偏移算法
        pcre *re;
        const char *error;
        int erroffset;
        int ovector[6];
        int rc, i, j;
        int exec_offset=0;
        char charset[16];//页面编码
        char charsetTmp[16];//页面编码临时存储
        int flag;//标识符

        //初始化字符串数组
        memset(charset, 0, sizeof(charset));
        memset(charsetTmp, 0, sizeof(charsetTmp));

        re = pcre_compile("<meta.*charset=(.*)>", PCRE_CASELESS|PCRE_UNGREEDY|PCRE_DOTALL, &error, &erroffset, NULL);
        if (re == NULL) {
            return FAILURE;
        }

        rc = pcre_exec(re,    // code, 输入参数，用pcre_compile编译好的正则表达结构的指针
                       NULL,  // extra, 输入参数，用来向pcre_exec传一些额外的数据信息的结构的指针
                       *(data+ToDoIn->id),   // subject, 输入参数，要被用来匹配的字符串
                       strlen(*(data+ToDoIn->id))+1,  // length, 输入参数，要被用来匹配的字符串的指针
                       exec_offset,     // startoffset, 输入参数，用来指定subject从什么位置开始被匹配的偏移量
                       0,     // options, 输入参数，用来指定匹配过程中的一些选项
                       ovector,        // ovector, 输出参数，用来返回匹配位置偏移量的数组
                       6);    // ovecsize, 输入参数， 用来返回匹配位置偏移量的数组的最大大小
        // 返回值：匹配成功返回非负数，没有匹配返回负数
        if (rc < 0) {
            flag=0;
        } else {
            flag=1;
        }
        if(flag==1) {
            if(ovector[3]-ovector[2]+1>=16) {
                return FAILURE;
            }
            //赋值到charset
            snprintf(charsetTmp, ovector[3]-ovector[2]+1, "%s", *(data+ToDoIn->id)+ovector[2]);

            //去除数组中的字符空白
            for(i=0, j=0; i<strlen(charsetTmp); i++) {
                //字符串过长输出错误,造成字符数组内存撑破
                if(j>=sizeof(charset)-1) {
                    return FAILURE;
                }
                if(charsetTmp[i]!=' ' && charsetTmp[i]!='"'  && charsetTmp[i]!='/') {
                    charset[j]=charsetTmp[i];
                    j++;
                }
            }

            //无法识别编码则不收录
            if(strlen(charset)<=0) {
                return FAILURE;
            }
        } else {
            strncpy(charset, CData->DBConfData.SpiderData.charset, strlen(CData->DBConfData.SpiderData.charset)+1);
        }

        pcre_free(re);   // 编译正则表达式re 释放内存

        //c为gbk时候转码，其他都不转
        if(charset[0]=='G' || charset[0]=='g') {
            if(2*strlen(*(data+ToDoIn->id))>=(BIG_DATA_SIZE-1)) {
                return FAILURE;
            }
            /*格式转换*/
            flag=g2u(*(data+ToDoIn->id), strlen(*(data+ToDoIn->id))+1, *(dataTmp+ToDoIn->id), BIG_DATA_SIZE);
            //status=u2g(&obj, &obj1);
            if(flag!=0) {
                return FAILURE;
            }
            memset(*(data+ToDoIn->id), 0, BIG_DATA_SIZE);

            //重新赋值
            strncpy(*(data+ToDoIn->id), *(dataTmp+ToDoIn->id), strlen(*(dataTmp+ToDoIn->id))+1);
            memset(*(dataTmp+ToDoIn->id), 0, BIG_DATA_SIZE);
        }

        return SUCCESS;
    } else {
        return FAILURE;
    }
}

/*
网络蜘蛛目标url的推理
queueUrl:判断的url
pattern:正则表达式
*/
static int spider_to_url_filter(char *queueURL, char *pattern) {
    //参数分别是正则表达式， 要匹配的字符串， 要返回的正则后的偏移算法
    pcre *re;
    const char *error;
    int erroffset;
    int ovector[4];
    int rc;
    int exec_offset=0;
    char queueURLTmp[URL_DATA_SIZE];
    re = pcre_compile(pattern, PCRE_CASELESS|PCRE_UNGREEDY|PCRE_DOTALL, &error, &erroffset, NULL);
    if (re == NULL) {
        return FAILURE;
    }

    rc = pcre_exec(re,    // code, 输入参数，用pcre_compile编译好的正则表达结构的指针
                   NULL,  // extra, 输入参数，用来向pcre_exec传一些额外的数据信息的结构的指针
                   queueURL,   // subject, 输入参数，要被用来匹配的字符串
                   strlen(queueURL)+1,  // length, 输入参数，要被用来匹配的字符串的指针
                   exec_offset,     // startoffset, 输入参数，用来指定subject从什么位置开始被匹配的偏移量
                   0,     // options, 输入参数，用来指定匹配过程中的一些选项
                   ovector,        // ovector, 输出参数，用来返回匹配位置偏移量的数组
                   4);    // ovecsize, 输入参数， 用来返回匹配位置偏移量的数组的最大大小
    // 返回值：匹配成功返回非负数，没有匹配返回负数
    if (rc < 0) {
        pcre_free(re);   // 编译正则表达式re 释放内存
        return FAILURE;
    }

    memset(queueURLTmp, 0, URL_DATA_SIZE);
    //赋值到charset
    if(ovector[1]-ovector[0]+1>URL_DATA_SIZE) {
        return FAILURE;
    }
    snprintf(queueURLTmp, ovector[1]-ovector[0]+1, "%s", queueURL);

	//无论是否相同都可以入库，但是这里做一些区分
    if(strcmp(queueURLTmp, queueURL)==0) {
        pcre_free(re);   // 编译正则表达式re 释放内存
        return SUCCESS;
    } else {
        pcre_free(re);   // 编译正则表达式re 释放内存
        return SUCCESS;
    }
}

/*
网络蜘蛛目标url数据入库
url:目标入库url
*/
static int spider_to_url_to_db(char *url) {
    int flag;//标识

    //队列set
    flag=dao_queue_set(&toFlagSet, 0, url);
    //队列报错，返回false
    if(flag==DAOFAILURE) {
        return FAILURE;
    }
    //队列已满，返回2
    if(flag==2) {
        //goto跳转
toFlagSetFlag:

        pthread_mutex_lock(&mutexURLDo);//阻塞线程
        DBOperationToDB.setFlag=1;//变更状态
        pthread_mutex_unlock(&mutexURLDo);//解开阻塞线程

        //在队列没有处理完时阻塞
        for(; ; ) {
            if(DBOperationToDB.setFlag==0) {
                break;
            }
        }
        //补录此数据
        flag=dao_queue_set(&toFlagSet, 0, url);
        //队列报错，返回false
        if(flag==DAOFAILURE) {
            return FAILURE;
        }
        //队列已满，返回2
        if(flag==2) {
            goto toFlagSetFlag;
        }

        return 2;
    }

    return SUCCESS;
}

/*
作用：读取数据回调函数-gid
data:回调传入参数
col_count:有多少字段
col_values:字段值
col_name:字段名
*/
static int select_spider_get_db_by_gid_config_callback(void *data, int col_count, char **col_values, char **col_name) {
    int i=0;
    int flag;//标识
    struct GRoupIdS *gids=(struct GRoupIdS *)data;

    gids->ids[gids->i]=atoi(col_values[i+0]);
    gids->i=gids->i+1;

    //return SUCCESS;
    return 0;
}

/*
作用：采集器配置读取-数据库配置读取-gid
id：数据库配置id
gids：GRoupIdS-分组-获取各id辅助结构体的指针
*/
static int spider_db_config_get_by_gid(int id, struct GRoupIdS *gids) {
    //新建sqlite结构体指针
    sqlite3 *db;
    int flag;//返回值标识
    char *errorMsg;//数据库错误信息

    //打开数据库
    char dbFile[256];
    memset(dbFile, 0, sizeof(dbFile));
    if(strlen(CData->GlobalConfigData.root_path)+strlen(CData->GlobalConfigData.db_config_file)+strlen("db/")+1>sizeof(dbFile)) {
        return FAILURE;
    }
    snprintf(dbFile, sizeof(dbFile),  "%sdb/%s", CData->GlobalConfigData.root_path, CData->GlobalConfigData.db_config_file);//设置文件路径
    //打开数据库
    flag=sqlite3_open(dbFile, &db);
    //是否打开判断
    if(flag != SQLITE_OK) {
        fprintf(stderr,"Can not open database\n:%s", sqlite3_errmsg(db));
        sqlite3_close(db);

        return FAILURE;
    }

    /*确定队列数目*/
    //sql语句
    char sql[256];
    //默认初始化
    memset(sql, 0, 256);
    snprintf(sql, 256,  "select id from config_spider where group_id=%d order by id", id);//拼接sql字符串
    if(strlen(sql)>=256) {
        sqlite3_close(db);
        return FAILURE;
    }

    flag=sqlite3_exec(db, sql, select_spider_get_db_by_gid_config_callback, gids, &errorMsg);//执行查询
    if(flag != SQLITE_OK) {
        fprintf(stderr,"SQL Error:%s\n", errorMsg);
        sqlite3_free(errorMsg);//默认会申请内存，此处应该释放，否则会有内存泄露
        sqlite3_close(db);

        return FAILURE;
    }

    //关闭数据库释放资源
    sqlite3_close(db);
    return SUCCESS;
}

/*
作用：网络蜘蛛执行主程序
type:类型，包括sid
id：具体的sid
返回值：成功success， 失败failure
*/
static int spider_main(char *type, int id) {
    int i;//用于循环
    int flag;//标识符
    if(strcmp(type, "cid") != 0 && strcmp(type, "gid") != 0) {
        return FAILURE;
    }

    //申请配置变量数据空间
    CData=(struct CData *)malloc(sizeof(struct CData)*1);
    if(CData==NULL) {
        return FAILURE;
    }
    //初始化配置变量数据空间
    memset(CData, 0, sizeof(CData));

    //读取根配置-全局配置文件
    flag=global_config_get();
    if(flag==FAILURE) {
        //释放配置变量数据空间
        free(CData);
        return FAILURE;
    }

    if(strcmp(type, "cid") == 0) {
        //单个执行

        //数据库配置文件读取
        flag=spider_db_config_get_by_id(id);
        if(flag==FAILURE) {
            //释放配置变量数据空间
            free(CData);
            return FAILURE;
        }

        //程序核心逻辑部分
        //调用初始化函数
        flag=spider_init();
        if(flag==FAILURE) {
            //释放配置变量数据空间
            free(CData);
            return FAILURE;
        }

        //开始执行
        flag=spider_start();
        if(flag==FAILURE) {
            //释放配置变量数据空间
            free(CData);
            return FAILURE;
        }

        //销毁函数
        flag=spider_destroy();
        if(flag==FAILURE) {
            //释放配置变量数据空间
            free(CData);
            return FAILURE;
        }
    } else if(strcmp(type, "gid") == 0) {
        struct GRoupIdS gids;
        //分组执行
        int ids[CData->GlobalConfigData.max_num_per_group]; //gid个数不超过100个

        gids.ids=ids;
        gids.i=0;
        flag=spider_db_config_get_by_gid(id, &gids);
        if(flag==FAILURE || gids.i==0) {
            //释放配置变量数据空间
            free(CData);
            return FAILURE;
        }

        for(i=0; i<gids.i; i++) {
            //数据库配置文件读取
            flag=spider_db_config_get_by_id(ids[i]);
            if(flag==FAILURE) {
                continue;
            }

            if(i==0) {
                //第一次初始化
                //程序核心逻辑部分
                flag=spider_init();
                if(flag==FAILURE) {
                    continue;
                }

                //开始执行
                flag=spider_start();
                if(flag==FAILURE) {
                    continue;
                }
            } else if(i==gids.i-1) {
                //最后一次释放空间
                //程序核心逻辑部分
                //调用初始化函数
                flag=spider_reset();
                if(flag==FAILURE) {
                    continue;
                }

                //开始执行
                flag=spider_start();
                if(flag==FAILURE) {
                    continue;
                }

                //销毁函数
                flag=spider_destroy();
                if(flag==FAILURE) {
                    continue;
                }
            } else {
                //正常情况，从第二次起重置
                //程序核心逻辑部分
                flag=spider_reset();
                if(flag==FAILURE) {
                    continue;
                }

                //开始执行
                flag=spider_start();
                if(flag==FAILURE) {
                    continue;
                }
            }
        }
    } else {
        //释放配置变量数据空间
        free(CData);
        return FAILURE;
    }

    //释放配置变量数据空间
    free(CData);
    return SUCCESS;
}

/*主函数*/
int main(int argc,  char* argv[]) {
    struct tm localTimeStart;
    struct tm localTimeStop;
    int flag;//标识符

    //输入参数处理
    if(argc!=3) {
        printf("param error!\n");
        return FAILURE;
    }
    //第二个参数必须是cid或者gid
    if(strcmp(argv[1], "cid") != 0 && strcmp(argv[1], "gid") != 0) {
        printf("param must be cid or gid!\n");
        return FAILURE;
    }
    //最后一个参数如果不是数字进行提醒并退出
    if(atoi(argv[2])==0) {
        printf("last param must be num!\n");
        return FAILURE;
    }

    timeStart=time( NULL );//开始运行时间
    localtime_r(&timeStart, &localTimeStart);

    //根据数据选择不同的处理方法
    if(strcmp(argv[1], "cid") == 0) {
        flag=spider_main("cid", atoi(argv[2]));
        if(flag==FAILURE) {
            return FAILURE;
        }
    } else {
        flag=spider_main("gid", atoi(argv[2]));
        if(flag==FAILURE) {
            return FAILURE;
        }
    }

    timeStop=time( NULL );//结束运行时间
    localtime_r(&timeStop, &localTimeStop);

    printf("Finished! TimeStart:[%d-%d-%d %d:%d:%d]-TimeStop:[%d-%d-%d %d:%d:%d], Time Used:%f mins!\n", (localTimeStart.tm_year+1900), (localTimeStart.tm_mon+1), localTimeStart.tm_mday, localTimeStart.tm_hour, localTimeStart.tm_min, localTimeStart.tm_sec, (localTimeStop.tm_year+1900), (localTimeStop.tm_mon+1), localTimeStop.tm_mday, localTimeStop.tm_hour, localTimeStop.tm_min, localTimeStop.tm_sec, (double)((timeStop-timeStart)/(60)));

    return SUCCESS;
}
