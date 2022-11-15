#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sysinfo.h"

uint64 knproc();
uint64 kfree_memory();

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if (growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if (n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n)
  {
    if (killed(myproc()))
    {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_trace(void)
{
  int n;
  argint(0, &n); //取出a0寄存器中的值，即sys_exec的返回值（掩码）
  if (n < 0)
    return -1;
  struct proc *p =myproc();
  p->trace_mask = n;
  return 0;
}

uint64
sys_sysinfo(void)
{
  struct sysinfo info;
  uint64 addr;
  struct proc *p=myproc();
  info.freemem=kfree_memory();
  info.nproc=knproc();
  argaddr(0, &addr);//将addr转换为a0寄存器的指针地址（存放syscall返回值）
  if(copyout(p->pagetable, addr, (char *)&info, sizeof(info)) < 0)
  //在给定的pagetable中拷贝info大小字节，将info拷贝到addr中
      return -1;
  printf("sysinfo : freemem is %d, num proc is %d\n",info.freemem,info.nproc);
  return 0;
}