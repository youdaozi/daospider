#include "bdb.h"

/*主函数*/
int main() {
    //BerkeleyDB相关
    DB_ENV *myEnv;
    DB *dbp;
    DBC *DBcursor;
    int flag;
    u_int32_t envFlags;
    char path[256];
    DBT key, data;//BerkeleyDB的key和value
    struct BDBData *BDBData;//定义BerkeleyDB中data结构
    char *keyData;
    //创建环境
    flag = db_env_create(&myEnv, 0);
    if (flag != 0) {
        fprintf(stderr, "Error creating env handle: %s\n", db_strerror(flag));
        return -1;
    }
    envFlags = DB_CREATE | DB_INIT_MPOOL;
    //打开环境
    snprintf(path, sizeof(path),  "%stmp/dbs/", ROOTPATH);//设置文件路径
    flag = myEnv->open(myEnv, path, envFlags, 0);
    if (flag != 0) {
        fprintf(stderr, "Environment open failed: %s", db_strerror(flag));
        return -1;
    }
    //创建数据库连接
    if ((flag = db_create(&dbp, myEnv, 0)) != 0) {
        fprintf(stderr, "db_create: %s\n", db_strerror(flag));
        //关闭环境
        if (myEnv != NULL) {
            myEnv->close(myEnv, 0);
        }

        return -1;
    }
    //打开数据库
    if ((flag = dbp->open(dbp, NULL, "to.db", NULL, DB_BTREE, DB_CREATE, 0664)) != 0) {
        dbp->err(dbp, flag, "%s", "to.db");
        //关闭环境
        if (myEnv != NULL) {
            myEnv->close(myEnv, 0);
        }

        return -1;
    }

    //返回一个创建的数据库游标
    flag=dbp->cursor(dbp, NULL, &DBcursor, 0);
    if(flag!=0) {
        //关闭数据库
        if (dbp != NULL) {
            dbp->close(dbp, 0);
        }

        //关闭环境
        if (myEnv != NULL) {
            myEnv->close(myEnv, 0);
        }

        return -1;
    }



    //进行数据取出并赋值
    for(; ; ) {
        //初始化key与data,由于key与data为空，所以遍历全部数据
        memset(&(key), 0, sizeof(DBT));
        memset(&(data), 0, sizeof(DBT));

        flag=DBcursor->get(DBcursor, &key, &data, DB_PREV);//可查找与key或者key和data匹配的记录.获取下一个(DB_PREV key为空，匹配数据，重头开始)
        if(flag!=0) {
            if(flag==DB_NOTFOUND) {
                break;
            } else {
                dbp->err(dbp, flag, "DBcursor->get");
                continue;
            }
        }

        BDBData=data.data;
        keyData=key.data;

        printf("key:{url:%s}, data:{url:%s, depth:%d, status:%d}\n", keyData, BDBData->url, BDBData->depth, BDBData->status);
    }

    //关闭游标cursor
    if(DBcursor!=NULL) {
        DBcursor->close(DBcursor);
    }
    //关闭数据库
    if (dbp != NULL) {
        dbp->close(dbp, 0);
    }
    //关闭环境
    if (myEnv != NULL) {
        myEnv->close(myEnv, 0);
    }

    return 0;
}
