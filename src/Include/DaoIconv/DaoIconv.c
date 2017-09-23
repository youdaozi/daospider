#include "DaoIconv.h"
/*
作用：编码转换主要函数
from_charset:现在的编码
to_charset：转换为的编码
inbuf：输入字符串
inlen:inbuf中还需要编码转换的字节数
outbuf：输出字符串
outlen:outbuf中还可以存放转码的字节数，也就是outbuf中的剩余空间
*/
int code_convert(char *from_charset, char *to_charset, char *inbuf, size_t inlen, char *outbuf, size_t outlen) {
    iconv_t cd;
    int rc;
    int status;

    char **pin = &inbuf;
    char **pout = &outbuf;

    cd = iconv_open(to_charset, from_charset);

    if (cd==0) {
        return -1;
    }

    /*初始化输出内存块（默认自动会初始化）*/
    status=iconv(cd, pin, &inlen, pout, &outlen);
    if ( status== -1) {
        return -1;
    }

    iconv_close(cd);
    return 0;
}
/*
作用：编码转换utf8到gbk
inbuf：输入字符串
inlen:inbuf中还需要编码转换的字节数
outbuf：输出字符串
outlen:outbuf中还可以存放转码的字节数，也就是outbuf中的剩余空间
*/
int u2g(char *inbuf, size_t inlen, char *outbuf, size_t outlen) {
    /*utf-8//IGNORE用于函数转换是避免报错程序自动停止*/
    return code_convert("UTF-8", "GBK//IGNORE", inbuf, inlen, outbuf, outlen);
}
/*
作用：编码转换gbk到utf8
inbuf：输入字符串
inlen:inbuf中还需要编码转换的字节数
outbuf：输出字符串
outlen:outbuf中还可以存放转码的字节数，也就是outbuf中的剩余空间
*/
int g2u(char *inbuf, size_t inlen, char *outbuf, size_t outlen) {
    /*utf-8//IGNORE用于函数转换是避免报错程序自动停止*/
    return code_convert("GBK", "UTF-8//IGNORE", inbuf, inlen, outbuf, outlen);
}
