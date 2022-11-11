#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
int main(int argc, char *argv[])
{
    int parent[2], child[2];
    if (pipe(parent) < 0)
    { //创建读写端文件描述符数组
        printf("wrong pipe");
        exit(-1);
    }
    if (pipe(child) < 0)
    {
        printf("wrong pipe");
        exit(-1);
    }
    int pid = fork();
    if (pid == 0)
    { //子进程
        char buf[10];
        read(parent[0], buf, 4);//子进程从parent读端读入
        printf("%d:received %s\n", getpid(),buf);
        write(child[1], "pong", 4);//子进程将pong写入写端
    }
    else if (pid > 0)
    { //父进程
        write(parent[1], "ping", 4);//父进程写入ping至写端
        char buf[10];
        read(child[0], buf, 4);//父进程从子进程读端读入
        printf("%d: received %s\n", getpid(),buf);
    }
    close(parent[0]);
    close(parent[1]);
    close(child[0]);
    close(child[1]);
    exit(0);
}