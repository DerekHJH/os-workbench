#include <common.h>
#define MAXALLOCBIT 13

int pow2[MAXALLOCBIT] = {0};
int findpow2(int x)
{
	int i = 0;
	for(i = 0; i <= MAXALLOCBIT && pow2[i] < x; i++);
	return pow2[i]; 
}

uintptr_t free_head = 0;
intptr_t free_lock = 0;
/*functions for lock_t===============*/
void dolock(intptr_t *lock)
{
	while(_atomic_xchg(lock, 1));
}

void unlock(intptr_t *lock)
{
	_atomic_xchg(lock, 0);
}
/*==========functions for memory allocation and free===========*/

static void *kalloc(size_t size) 
{
	int intr = _intr_read();
	_intr_write(0);
	
	dolock(&free_lock);
	if(size == 0)return NULL;
	uintptr_t cur = free_head;
	size_t POW = findpow2(size);//Find the bound for the right size; 
	cur	= (cur + POW - 1) & (~(POW - 1));
	free_head = cur + size;
	unlock(&free_lock);

	if(intr) _intr_write(1);

	return (void *)cur;
}

static void kfree(void *ptr) 
{
}

static void pmm_init() 
{
  uintptr_t pmsize = ((uintptr_t)_heap.end - (uintptr_t)_heap.start);
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, _heap.start, _heap.end);

  /*==============initialize pow2=================*/
	pow2[0] = 1;
	for(int i = 1; i < MAXALLOCBIT; i++)
		pow2[i] = pow2[i - 1] * 2;
	/*===============initialize free_page============*/
	free_head = (uintptr_t)_heap.start;
	free_lock = 0;
}

MODULE_DEF(pmm) = 
{
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
