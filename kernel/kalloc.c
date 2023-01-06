// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmems[NCPU]; 
// 为每个 CPU 分配独立的 freelist，并用独立的锁保护它

const uint name_sz = sizeof("kmem cpu 0");//锁名长度
char kmem_lock_names[NCPU][sizeof("kmem cpu 0")];//定义锁名数组

void
kinit()
{
  for(int i = 0; i < NCPU; i++){
    snprintf(kmem_lock_names[i], name_sz, "kmem cpu %d", i);// 为每个锁命名
    initlock(&kmems[i].lock, kmem_lock_names[i]);//初始化每个锁
  }
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  push_off();
  uint id = cpuid();//获取cpu id
  pop_off();//acquire时候会自动关中断，所以在这里打开中断即可

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmems[id].lock);
  r->next = kmems[id].freelist;
  kmems[id].freelist = r;
  release(&kmems[id].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  push_off();//关中断
  uint id=cpuid();//获取cpuid
  acquire(&kmems[id].lock);
  if(!kmems[id].freelist){//该cpu空闲列表已耗尽
    int steal_left = 16;//从其它cpu偷的页表数量
    for(int i=0;i<NCPU;i++) {
      if(i == id) continue; // 只抢其它cpu的空闲列表
      acquire(&kmems[i].lock);
      struct run *rr = kmems[i].freelist;
      //rr表示其它cpu中的空闲列表
      while(rr && steal_left) {
        kmems[i].freelist = rr->next;
        rr->next = kmems[id].freelist;//偷出的空闲页表插入表头
        kmems[id].freelist = rr;
        rr = kmems[i].freelist;
        steal_left--;
      }
      release(&kmems[i].lock);
      if(steal_left == 0) break; // done stealing
    }
  }

  r = kmems[id].freelist;
  //r是之后返回的页
  if(r){
    kmems[id].freelist = r->next;
  }
  release(&kmems[id].lock);
  pop_off();
  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
