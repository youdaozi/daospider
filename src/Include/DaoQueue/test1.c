#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <pcre.h>

/*struct pregAllStr{
        int num;
        struct pregStr ovector[256];
};*/
struct pregStr {
    int num;
    int ovector[16];
};

int preg_match(char *pattern, char *srcStr, struct pregStr *ret, int getWhich) {
    //参数分别是正则表达式， 要匹配的字符串， 要返回的正则后的偏移算法
    pcre  *re;
    int matchNum=512;
    int i;
    struct pregStr matchs[matchNum];
    //初始化num=0
    for(i=0; i<matchNum; i++) {
        matchs[i].num=0;
    }

    const char *error;
    int oveccount=30;
    int  erroffset;
    int  ovector[oveccount];
    int p, q;
    int  rc;
    int exec_offset=0;
    //char  src [] = "111 <title>Hello World</title> 222<title>PCRE</title>";   // 要被用来匹配的字符串
    //char  pattern [] = "<title>(.*)</(tit)le>";    // 将要被编译的字符串形式的正则表达式
    //printf("String : %s\n", srcStr);
    //printf("Pattern: \"%s\"\n", pattern);
    re = pcre_compile(pattern,     // pattern, 输入参数，将要被编译的字符串形式的正则表达式
                      0,             // options, 输入参数，用来指定编译时的一些选项
                      &error,         // errptr, 输出参数，用来输出错误信息
                      &erroffset,     // erroffset, 输出参数，pattern中出错位置的偏移量
                      NULL);         // tableptr, 输入参数，用来指定字符表，一般情况用NULL
    // 返回值：被编译好的正则表达式的pcre内部表示结构
    if (re == NULL) {
        //如果编译失败，返回错误信息
        //printf("PCRE compilation failed at offset %d: %s\n", erroffset, error);
        pcre_free(re);
        return 0;
    }
    //初始化二维数组
    p=0;
    do {
        rc = pcre_exec(re,    // code, 输入参数，用pcre_compile编译好的正则表达结构的指针
                       NULL,  // extra, 输入参数，用来向pcre_exec传一些额外的数据信息的结构的指针
                       srcStr,   // subject, 输入参数，要被用来匹配的字符串
                       strlen(srcStr),  // length, 输入参数，要被用来匹配的字符串的指针
                       exec_offset,     // startoffset, 输入参数，用来指定subject从什么位置开始被匹配的偏移量
                       0,     // options, 输入参数，用来指定匹配过程中的一些选项
                       ovector,        // ovector, 输出参数，用来返回匹配位置偏移量的数组
                       oveccount);    // ovecsize, 输入参数， 用来返回匹配位置偏移量的数组的最大大小
        // 返回值：匹配成功返回非负数，没有匹配返回负数
        if (rc < 0) {
            //如果没有匹配，返回错误信息
            //if (rc == PCRE_ERROR_NOMATCH) printf("Sorry, no match ...\n");
            //else printf("Matching error %d\n", rc);
            //pcre_free(re);
            //printf("HelloWorld\n");
            break;
            //return 0;
        }
        //printf("\nOK, has matched ...\n\n");   //没有出错，已经匹配
        matchs[p].num=rc;
        printf("%d:%d\n", p, matchs[p].num);
        for (i = 0, q=0; i < rc; i++) {
            //分别取出捕获分组 $0整个正则公式 $1第一个()
            matchs[p].ovector[q]=ovector[2*i];
            q++;
            matchs[p].ovector[q]=ovector[2*i+1];
            q++;
        }
        exec_offset=ovector[i];
        //printf("%d\n", matchs[p].num);
        p++;
    } while(rc>0);

    pcre_free(re);   // 编译正则表达式re 释放内存
    //返回结果数组
    //得出返回数组是哪一个并进行赋值
    *ret=matchs[getWhich];

    return 0;
}

int main() {
    struct pregStr ovector;
    //不能乱用malloc，malloc毕竟效率等不高
    //ovector=(char *)malloc(sizeof(char)*100);
    int i, j;
    char  srcStr[] = "111 <title>Hello World</title> 222<title>PCRE</title>";   // 要被用来匹配的字符串
    char  pattern[] = "<title>(.*?)</(tit)le>";    // 将要被编译的字符串形式的正则表达式
    int getWhich=0;

    preg_match(pattern, srcStr, &ovector, getWhich);

    //printf("%d\n", ovector.num);
    for(i=0; i<ovector.num; i++) {
        char *substring_start = srcStr + ovector.ovector[2*i];
        int substring_length =ovector.ovector[2*i+1] - ovector.ovector[2*i];
        printf("$%2d: %.*s\n", j, substring_length, substring_start);
    }


    //free(ovector);
    return 0;
}

