#include "DaoQueue.h"

/*线程锁*/
static pthread_mutex_t mutex;

/*
queueFlag:队列管理结构
queueNum:队列数量
queues:队列数组
queueDates：队列数据
*/
int dao_queue_init(struct DaoQueueFlag *queueFlag, int queueNum, struct DaoQueue **queues, struct DaoQueueDate **queueDates) {
    //初始化线程
    pthread_mutex_init(&mutex, NULL);//锁机制初始化

    //排除无效申请，申请空间为0
    if(queueNum==0) {
        return DAOFAILURE;
    }

    /*初始化所有queue*/
    int i;
    for(i=0; i<queueNum; i++) {
        //清空所有队列数据
        memset((*(queueDates+i))->data, 0, sizeof(char)*(*(queueDates+i))->dataSize);

        (*(queues+i))->data=*(queueDates+i);
        (*(queues+i))->depth=0;
        (*(queues+i))->index=i;
        (*(queues+i))->queueFlag=queueFlag;
    }

    /*队列结构初始化*/
    queueFlag->startQueue=queues;
    queueFlag->num=queueNum;
    queueFlag->nowSetIndex=0;
    queueFlag->nowGetIndex=0;
    queueFlag->maxIndex=0;

    //初始化队列结构
    //返回成功标识
    return DAOSUCCESS;
}

/*
基于内存池的队列注销
queueFlag:队列管理结构
*/
void dao_queue_exit(struct DaoQueueFlag *queueFlag) {
    //锁机制销毁
    pthread_mutex_destroy(&mutex);
}

/*
重置队列
queueFlag:队列管理结构
*/
int dao_queue_reset(struct DaoQueueFlag *queueFlag) {
    //更改下标
    queueFlag->maxIndex=0;
    queueFlag->nowGetIndex=0;
    queueFlag->nowSetIndex=0;

    //初始化空间
    //memset((*(queueFlag->startQueue))->data->data, 0, sizeof(char)*(*(queueFlag->startQueue))->data->dataSize*queueFlag->num);
    //初始化所有queue
    int i;
    struct DaoQueue *queue;
    for(i=0; i<queueFlag->num; i++) {
        queue=*(queueFlag->startQueue+i);
        //清空所有队列数据
        memset(queue->data->data, 0, sizeof(char)*queue->data->dataSize);
        queue->data->dataSize=0;

        queue->depth=0;
    }

    //默认返回成功标识
    return DAOSUCCESS;
}

/*
queueFlag:队列标识符
depth:深度
obj:所设置的数据到索引的字符串
*/
int dao_queue_set(struct DaoQueueFlag *queueFlag, int depth, char *obj){
    //获取set的index
    int index=-1;
    pthread_mutex_lock(&mutex);//阻塞线程
    index=queueFlag->nowSetIndex;//变更状态
    queueFlag->nowSetIndex=queueFlag->nowSetIndex+1;//自增1
    pthread_mutex_unlock(&mutex);//解开阻塞线程
    if(index==-1){
        return DAOFAILURE;
    }

    //超过队列数量限制则返回DAOFAILURE
    if(index>=queueFlag->num) {
        return 2;
    }

    struct DaoQueue *nowQueue;//定义现在所在的队列
    struct DaoQueue *queueTmp;//定义现在所在的队列
    nowQueue=*(queueFlag->startQueue+index);

    //字符串超过大小则返回错误
    if((strlen(obj)+1)>=nowQueue->totalSize){
        return DAOFAILURE;
    }

    //去重处理类似与java机制的set(暂时用BerkeleyDB的机制，这里只做内存数据的中转)
    /*int i, j;
    for(i=0, j=0; i<queueFlag->num; i++){
        //检测到内容为空3次则终止循环
        if(j>=3){
            break;
        }

        queueTmp=*(queueFlag->startQueue+i);

        //内容为空则记录数据
        if(strlen(queueTmp->data->data)==0){
            j++;
        }

        if(strcmp(queueTmp->data->data, obj)==0){
            pthread_mutex_unlock(&mutex);//解开阻塞线程
            return 2;//表示此数据已经存在于队列中，直接返回2
        }
    }*/

    //队列赋值
    strncpy(nowQueue->data->data, obj, strlen(obj)+1);
    nowQueue->depth=depth;

    //设置maxIndex
    if(index>queueFlag->maxIndex) {
        queueFlag->maxIndex=index;//更改最大队列位置
    }

    //返回成功
    return DAOSUCCESS;
}

/*
基于内存池的队列根据下标index取出数据
queueFlag:队列标识符
queue:获取到的队列数据
*/
int dao_queue_get(struct DaoQueueFlag *queueFlag, struct DaoQueue **nowQueue){
    //获取get的index
    int index=-1;
    pthread_mutex_lock(&mutex);//阻塞线程
    index=queueFlag->nowGetIndex;//变更状态
    queueFlag->nowGetIndex=queueFlag->nowGetIndex+1;//自增1
    pthread_mutex_unlock(&mutex);//解开阻塞线程
    if(index==-1){
        return DAOFAILURE;
    }

    *nowQueue=*(queueFlag->startQueue+index);
    //返回成功
    return DAOSUCCESS;
}

/*
queueFlag:队列标识符
index:队列索引
depth:深度
obj:所设置的数据到索引的字符串
*/
int dao_queue_set_index(struct DaoQueueFlag *queueFlag, int index, int depth, char *obj) {
    //超过队列数量限制则返回DAOFAILURE
    if(index>=queueFlag->num) {
        return 2;
    }

    struct DaoQueue *nowQueue;//定义现在所在的队列
    struct DaoQueue *queueTmp;//定义现在所在的队列
    nowQueue=*(queueFlag->startQueue+index);

    //字符串超过大小则返回错误
    if((strlen(obj)+1)>=nowQueue->totalSize){
        return DAOFAILURE;
    }

    //去重处理类似与java机制的set(暂时用BerkeleyDB的机制，这里只做内存数据的中转)
    /*int i, j;
    for(i=0, j=0; i<queueFlag->num; i++){
        //检测到内容为空3次则终止循环
        if(j>=3){
            break;
        }

        queueTmp=*(queueFlag->startQueue+i);

        //内容为空则记录数据
        if(strlen(queueTmp->data->data)==0){
            j++;
        }

        if(strcmp(queueTmp->data->data, obj)==0){
            pthread_mutex_unlock(&mutex);//解开阻塞线程
            return 2;//表示此数据已经存在于队列中，直接返回2
        }
    }*/

    //队列赋值
    strncpy(nowQueue->data->data, obj, strlen(obj)+1);
    nowQueue->depth=depth;

    queueFlag->nowSetIndex=queueFlag->nowSetIndex+1;//nowSetIndex， 自增1

    if(index>queueFlag->maxIndex) {
        queueFlag->maxIndex=index;//更改最大队列位置
    }

    //返回成功
    return DAOSUCCESS;
}

/*
基于内存池的队列根据下标index取出数据
queueFlag:队列标识符
index:队列索引
queue:获取到的队列数据
*/
int dao_queue_get_index(struct DaoQueueFlag *queueFlag, int index, struct DaoQueue **nowQueue) {
    *nowQueue=*(queueFlag->startQueue+index);
    queueFlag->nowGetIndex=queueFlag->nowGetIndex+1;//nowGetIndex, 自增1
    //返回成功
    return DAOSUCCESS;
}
