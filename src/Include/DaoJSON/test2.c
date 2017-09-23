#include <stdio.h>
#include <stdlib.h>
#include "cJSON.h"
int main (int argc, const char * argv[])
{
    // 创建JSON Object
    cJSON *root = cJSON_CreateObject();
    // 加入节点（键值对），节点名称为value，节点值为123.4
    cJSON_AddNumberToObject(root,"value",123.4);
    // 打印JSON数据包
    char *out = cJSON_Print(root);
    printf("%s\n",out);
    // 释放内存
    cJSON_Delete(root);
    free(out);
    return 0;
}
