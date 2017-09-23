#include <mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static char* server_groups[]= {"embedded","server","this_program_server",(char*)NULL};
int main() {

    MYSQL mysql;
    MYSQL_RES *res;
    MYSQL_ROW row;
    char sqlcmd[200];
    int t,r;
    mysql_library_init(0,NULL,server_groups);//初始化MYSQL数据库
    mysql_init(&mysql);//初始化MYSQL标识符，用于连接
    if(!mysql_real_connect(&mysql,"localhost","root","159126","tjuhe",0,NULL,0)) {
        fprintf(stderr,"error:%s\n",mysql_error(&mysql));

    } else {
        puts("success");
//首先向数据库中插入一条记录
        //sprintf(sqlcmd,"%s","insert into test (id, name, value) values (NULL, 'TEST', 'HAHA')");
        //mysql_query(&mysql,sqlcmd);
        sprintf(sqlcmd,"%s","select * from test");
        t=mysql_real_query(&mysql,sqlcmd,(unsigned int)strlen(sqlcmd));

        if(t) {
            printf("faliure%s\n",mysql_error(&mysql));
        } else {
            res=mysql_store_result(&mysql);//返回查询的全部结果集
            while(row=mysql_fetch_row(res)) { //mysql_fetch_row取结果集的下一行
                for(t=0; t<mysql_num_fields(res); t++) { //结果集的列的数量
                    printf("%s\t",row[t]);
                }
                printf("\n");
            }

            mysql_free_result(res);//操作完毕，查询结果集
        }
        mysql_close(&mysql);//关闭数据库连接

    }
    mysql_library_end();//关闭MySQL库

    return EXIT_SUCCESS;
}
