#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
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
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;


  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

/* 
  检测page table是否被读或写
  传入分别为
  buf（测试地址空间）
  length （页表个数）
  abits（位图）
*/
//#ifdef LAB_PGTBL
int
sys_pgaccess(void)
{
  struct proc *p =myproc();//当前进程
  uint64 usrpg_ptr;//页表起始指针
  int npage;//页表个数
  uint64 usraddr;//用户地址
  uint32 bitmap=0;
  argaddr(0,&usrpg_ptr);
  argint(1,&npage);
  //传入的第一个参数以a1寄存器返回并赋给npage
  argaddr(2,&usraddr);
  if(npage>32)
    return -1;//限制搜索的页表个数
  for(int i=0;i<npage;i++){
    pte_t *pte=walk(p->pagetable,usrpg_ptr+i*PGSIZE,0);
    //pte_t *pte=walk(p->pagetable,i*PGSIZE,0);
    if(*pte & PTE_A){//判断该pte是否被读或写入
    //riscv会自动从处理PTE_A位是否被读或者写入
      bitmap |= (1<<i);//制作位图
      *pte &= ~PTE_A;//重置PTE_A
    }
  }
  
  copyout(p->pagetable,usraddr,(char*)&bitmap,sizeof(bitmap));
  //将生成的bitmap从内核拷贝到用户态
  return 0;
}
//#endif

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
