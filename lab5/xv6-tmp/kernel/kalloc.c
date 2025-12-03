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
} kmem;

// Reference counting
struct {
  struct spinlock lock;
  int refcount[PHYSTOP/PGSIZE];
} pageref;

#define PA2IDX(pa) (((uint64)(pa)) / PGSIZE)

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&pageref.lock, "pageref");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE) {
    // Initialize refcount to 1 BEFORE calling kfree
    pageref.refcount[PA2IDX((uint64)p)] = 1;
    kfree(p);
  }
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

  // Decrement reference count
  acquire(&pageref.lock);
  int idx = PA2IDX((uint64)pa);
  if(pageref.refcount[idx] < 1)
    panic("kfree: refcount < 1");
  
  pageref.refcount[idx]--;
  // printf("kfree: pa=%p idx=%d ref=%d\n", pa, idx, pageref.refcount[idx]);
  if(pageref.refcount[idx] > 0) {
    release(&pageref.lock);
    return;  // Still references, don't free
  }
  release(&pageref.lock);

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r) {
    memset((char*)r, 5, PGSIZE); // fill with junk

    // Set reference count to 1
    acquire(&pageref.lock);
    pageref.refcount[PA2IDX((uint64)r)] = 1;
    release(&pageref.lock);

    // printf("kalloc: pa=%p\n", r);
  }

  return (void*)r;
}

// Increment reference count for a page
void
krefpage(void *pa)
{
  acquire(&pageref.lock);
  pageref.refcount[PA2IDX((uint64)pa)]++;
  release(&pageref.lock);
}

// Get reference count for a page
int
kgetref(void *pa)
{
  int ref;
  acquire(&pageref.lock);
  ref = pageref.refcount[PA2IDX((uint64)pa)];
  release(&pageref.lock);
  return ref;
}