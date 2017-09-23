#include "DaoLibs.h"

/*
作用：内存链初始化
memLinkFlag:内存链
num:内存链节点数量
*/
int dao_memory_link_init(struct MemLink *memLinkFlag, int num){
    if(num<1){
        return DAOFAILURE;
    }
    //内存链申请
    struct MemLinkNode *memLinkNodes=(struct MemLinkNode *)malloc(sizeof(struct MemLinkNode)*num);
    if(memLinkNodes==NULL){
        return DAOFAILURE;
    }
    memset(memLinkNodes, 0, sizeof(struct MemLinkNode)*num);
    //内存链赋值
    memLinkFlag->memLinkNodes=memLinkNodes;
    memLinkFlag->num=num;
    memLinkFlag->index=0;

    return DAOSUCCESS;
}

/*
作用：内存链增加节点
memLinkFlag:内存链
p:新增的内存指针
*/
void dao_memory_link_add(struct MemLink *memLinkFlag, void *p){
    struct MemLinkNode *nowMemLinkNode;//此时的内存链节点

    //存储的空间
    nowMemLinkNode=memLinkFlag->memLinkNodes+memLinkFlag->index;
    //赋值
    nowMemLinkNode->p=p;

    //更改内存链数据
    memLinkFlag->index=memLinkFlag->index+1;
}

/*
作用：内存链退出
memLinkFlag:内存链
*/
void dao_memory_link_exit(struct MemLink *memLinkFlag){
    int i;
    struct MemLinkNode *nowMemLinkNode;//此时的内存链节点
    for(i=0; i<memLinkFlag->index; i++){
        nowMemLinkNode=memLinkFlag->memLinkNodes+i;
        free(nowMemLinkNode->p);
    }
    free(memLinkFlag->memLinkNodes);
}
