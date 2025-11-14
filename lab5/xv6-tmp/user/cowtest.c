// kernel/kalloc.c
#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"

extern char end[]; // first address after kernel
static struct run *freelist;

struct {
  struct spinlock lock;
} kmem;

static struct spinlock pgref_lock;
static int *pageref;   // page reference counts
static uint64 npage;   // number of pages we track

// convert physical address to index in pageref
#define PA2IDX(pa) (((uint64)(pa) - KERNBASE) / PGSIZE)

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&pgref_lock, "pgref");

  // initialize page refcounts array
  // compute maximum physical pages between 'end' and PHYSTOP
  uint64 start_pa = (uint64)PGROUNDUP((uint64)end);
  // number of pages we track from KERNBASE to PHYSTOP
  // note: on many xv6 setups, KERNBASE == the base physical mapping constant
  npage = (PHYSTOP - KERNBASE) / PGSIZE;
  pageref = (int*)kalloc(); // temporarily use kalloc memory to hold array pointer?
  // safer: allocate pageref with a static array? but easier: use kalloc to get one page for pageref storage
  // if you prefer static: static int pageref[MAXPAGES]; but size depends on PHYSTOP.
  // We'll allocate one page to hold the array and clear it.
  if(!pageref) panic("kinit: pageref alloc failed");
  memset(pageref, 0, PGSIZE);
  freerange(start_pa, (void*)PHYSTOP);
}

// helper to increment page refcount for physical page pa
void
page_ref_inc(uint64 pa)
{
  acquire(&pgref_lock);
  uint64 idx = PA2IDX(pa);
  if(idx < npage) pageref[idx]++;
  else panic("page_ref_inc: bad pa");
  release(&pgref_lock);
}

// helper to decrement page refcount for physical page pa; returns new count
int
page_ref_dec(uint64 pa)
{
  acquire(&pgref_lock);
  uint64 idx = PA2IDX(pa);
  if(idx >= npage) panic("page_ref_dec: bad pa");
  if(pageref[idx] <= 0) panic("page_ref_dec: underflow");
  pageref[idx]--;
  int v = pageref[idx];
  release(&pgref_lock);
  return v;
}

// helper to get current refcount
int
page_ref_count(uint64 pa)
{
  acquire(&pgref_lock);
  uint64 idx = PA2IDX(pa);
  if(idx >= npage) panic("page_ref_count: bad pa");
  int v = pageref[idx];
  release(&pgref_lock);
  return v;
}

// modify kalloc: when it returns a fresh page, set its refcount to 1
void*
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = freelist;
  if(r)
    freelist = r->next;
  release(&kmem.lock);

  if(r) {
    // clear page
    memset((char*)r, 5, PGSIZE);
    // compute pa and set refcount = 1
    uint64 pa = (uint64)r;
    acquire(&pgref_lock);
    uint64 idx = PA2IDX(pa);
    if(idx >= npage) panic("kalloc: bad pa");
    pageref[idx] = 1;
    release(&pgref_lock);
  }
  return (void*)r;
}

// modify kfree: only put back onto freelist when refcount becomes zero
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // decrement refcount and only free if it hits 0
  acquire(&pgref_lock);
  uint64 idx = PA2IDX((uint64)pa);
  if(idx >= npage) panic("kfree: bad pa idx");
  if(pageref[idx] <= 0) panic("kfree: freeing free page");
  pageref[idx]--;
  int v = pageref[idx];
  release(&pgref_lock);

  if(v > 0) {
    // still referenced by others — don't free physical page
    return;
  }

  // actually free page
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = freelist;
  freelist = r;
  release(&kmem.lock);
}
