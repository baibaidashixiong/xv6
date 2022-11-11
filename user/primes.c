#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
void prime(int rd)
{
    int n;
    read(rd, &n, 2);//从管道读端读入一个buf
    printf("prime %d\n", n);
    int created = 0;
    int p[2];
    int num;
    while (read(rd, &num, 2) != 0)
    {
        if (created == 0)//每个管道的首位数都是prime
        {
            pipe(p);
            created = 1;
            int pid = fork();
            if (pid == 0)
            {
                close(p[1]);//关闭管道写端
                prime(p[0]);//子进程递归该过程
                return;
            }
            else
            {
                close(p[0]);
            }
        }
        if (num % n != 0)
        {
            write(p[1], &num, 2);//每次将过滤掉n的倍数的结果写入管道写端
        }
    }
    close(rd);
    close(p[1]);
    wait(0);
}

int main(int argc, char *argv[])
{
    int init[2];
    pipe(init);
    int pid = fork();
    if (pid != 0)//父进程
    {
        close(init[0]);
        for (int i = 2; i <= 35; i++)
        {
            write(init[1], &i, 2);//向管道写端写入2-35
        }
        close(init[1]);
        wait(0);
    }
    else
    {
        close(init[1]);//关闭子进程写端
        prime(init[0]);
        close(init[0]);
    }
    exit(0);
}