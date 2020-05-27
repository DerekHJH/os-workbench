#include <common.h>
//#define DEBUG
#define PGSIZE (4096 * 16 + 512) 
#define MAXALLOCBIT 13
#ifdef __x86_64__
	#define HDRSIZE 32
#else
	#define HDRSIZE 16
#endif 
uintptr_t FREE = 0;
uintptr_t cnt = 0;
/*===============other functions========*/
int pow2[MAXALLOCBIT] = {0};
int findpow2(int x)
{
	int i = 0;
	for(i = 0; i <= MAXALLOCBIT && pow2[i] < x; i++);
	return pow2[i]; 
}
/*==========Some definitions=============*/
typedef struct Header
{
	struct Header *Next;
	uintptr_t sptr; //Start pointer if it points to NULL, then it is free;
  size_t dummy; //From behind sptr till the start of this block	
	uint8_t start;
}header_t;
#define Hlen (sizeof(header_t) - sizeof(size_t))
typedef union page 
{
	struct
	{
		size_t size;
		union page *Next;
		union page *Prev;
		header_t *freeh;
	};
  struct
	{	
		uint8_t header[HDRSIZE], data[PGSIZE - HDRSIZE];
	};
}__attribute__((packed)) page_t;

page_t *free_head = NULL;
page_t *alloc_head = NULL;
intptr_t free_lock = 0;
page_t *cpu_head[8] = {NULL};
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
void *bigmalloc()
{
	page_t *ret = NULL;

	dolock(&free_lock);
	if(FREE > 4)
	{
		/*=======leave free_head=========*/
		ret = free_head;
		free_head = free_head->Next;
		free_head->Prev = NULL;
		/*==========enter alloc_head==========*/
		ret->size = PGSIZE - HDRSIZE - Hlen;
		ret->Next = alloc_head->Next;
		ret->Prev = alloc_head;
		if(alloc_head->Next != NULL)alloc_head->Next->Prev = ret;
		alloc_head->Next = ret;	
		ret->freeh = (header_t *)((uintptr_t)ret + HDRSIZE);
		/*=========initialize freeh=========*/
		ret->freeh->Next = NULL;
		ret->freeh->sptr = 0;
		FREE--;
	}
	unlock(&free_lock);
#ifdef DEBUG
	printf("from cpu %d\n", _cpu());
	if(ret != NULL)printf("ret is 0x%x and ret->next is 0x%x and ret->freeh is 0x%x and alloc_head is 0x%x and free_head is 0x%x, FREE is %d\n", ret, ret->Next, ret->freeh, alloc_head, free_head, FREE);
#endif
	return ret;
}

void bigfree(page_t *ptr)
{
#ifdef DEBUG
	panic_on(ptr == NULL, "bigfree ptr is NULL!!\n");
	panic_on(ptr->Prev == NULL, "bigfree ptr->Prev is NULL!!\n");
#endif
  /*
	page_t *cur = alloc_head;
  while(cur)
  {
  	printf("cur is 0x%x\n", cur);
  	cur = cur->Next;
  }
	printf("ptr->Prev is 0x%x\n", ptr->Prev);
	*/

	/*====leave alloc_head======*/
	ptr->Prev->Next = ptr->Next;
	if(ptr->Next != NULL)ptr->Next->Prev = ptr->Prev;
	/*======enter free_head======*/
  ptr->Next = free_head;
	ptr->Prev = NULL;
	free_head->Prev = ptr;
	free_head = ptr;
	FREE++;
	/*
	cur = alloc_head;
	while(cur)
	{
		printf("cur is 0x%x\n", cur);
		cur = cur->Next;
	}
	*/
}



static void *kalloc(size_t size) 
{
	int intr = _intr_read();
	_intr_write(0);
/*===============Original code=====================*/
	if(size == 0)return NULL;
	int cpu = _cpu();
	size_t POW = findpow2(size);//Find the bound for the right size; 
	size = (size + 0x7) & (~0x7);//Make sure size is divisible by 8;

	uintptr_t Sptr = 0;
	size_t Dummy = 0;
	size_t Size = 0;	
	header_t *cur = cpu_head[cpu]->freeh;
	Sptr = ((uintptr_t)(&cur->start) + POW - 1) & (~(POW - 1));	 	
	Dummy = Sptr - (uintptr_t)(&cur->start);
	Size = Dummy + size;
	if(Size + Hlen > (cpu_head[cpu]->size))
	{
		page_t *temp = bigmalloc();
		if(temp == NULL)
		{
			return NULL;
		}
		else
		{
			cpu_head[cpu] = temp;
			cur = cpu_head[cpu]->freeh;
			Sptr = ((uintptr_t)(&cur->start) + POW - 1) & (~(POW - 1));	 	
			Dummy = Sptr - (uintptr_t)(&cur->start);
			Size = Dummy + size;
		}
	}
	cpu_head[cpu]->freeh = (header_t *)(Sptr + size);
	cpu_head[cpu]->freeh->sptr = 0;
	cpu_head[cpu]->freeh->Next = NULL;
	cpu_head[cpu]->size = cpu_head[cpu]->size - Hlen - Size; 
#ifdef DEBUG
	if(!cpu_head[cpu] && cpu_head[cpu]->freeh != NULL)printf("from cpu %d, size is %d, Size is %d, Dummy is %d, Sptr is 0x%x, POW is 0x%x, cpu->freeh is 0x%x, cpu->size is 0x%x, cpu->free->sptr is 0x%x, cpu->free->Next is 0x%x\n", _cpu(), size, Size, Dummy, Sptr, POW, cpu_head[cpu]->freeh, cpu_head[cpu]->size, cpu_head[cpu]->freeh->sptr, cpu_head[cpu]->freeh->Next);
#endif

	cur->sptr = Sptr;
	memcpy((void *)(Sptr - sizeof(size_t)), &Dummy, sizeof(size_t));
	cur->Next = cpu_head[cpu]->freeh;
	/*==================Original Code=================*/
	if(intr) _intr_write(1);
	return (void *)cur->sptr;

}

static void kfree(void *ptr) 
{
	int intr = _intr_read();
	_intr_write(0);
	/*====================Original Code================*/
	//printf("free request 0x%x\n", ptr);
	uintptr_t Sptr = (uintptr_t)ptr;	
	size_t Dummy = *((size_t *)(Sptr - sizeof(size_t)));
	header_t *cur = (header_t *)(Sptr - Dummy - Hlen); 
	dolock(&free_lock);
	if(cur->sptr == Sptr)cur->sptr = 0;
	cnt++;
	if(cnt > (PGSIZE >> 3))
	{
		cnt = 0;
		//printf("start one iteration\n");
		page_t *cur = alloc_head->Next;
		//printf("cur is 0x%x\n", cur);
		page_t *tobefree = NULL;
		header_t *p = NULL;
		int isfree = 1;
		while(cur)
		{
			//printf("	cur is 0x%x\n", cur);
			isfree = 1;
			p = (header_t *)((uintptr_t)cur + HDRSIZE);
			while(p)
			{
				//printf("		p is 0x%x\n", p);
				if(p->sptr != 0)
				{
					isfree = 0;
					break;
				}
				p = p->Next;
			}

			if(isfree)
			{
				//printf("===========================free ok======================\n");
				tobefree = cur;
				cur = cur->Prev;
				bigfree(tobefree);
				//printf("freehead is 0x%x, freehead->Next is 0x%x after free\n", free_head, free_head->Next);
#ifdef DEBUG
	printf("==================================FREE============free_head is 0x%x, FREE is %d\n", free_head, FREE);
#endif
			}
			cur = cur->Next;
		}
		//printf("finish one iteration\n");
	}

	unlock(&free_lock);
/*==========================Original Code=================*/
	if(intr)_intr_write(1);


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
	free_head = _heap.start;
	while((uintptr_t)free_head < (uintptr_t)_heap.end)
	{
		FREE++;
		free_head->size = PGSIZE - HDRSIZE - Hlen;
		free_head->Next = (uintptr_t)free_head + PGSIZE < (uintptr_t)_heap.end ? (page_t *)((uintptr_t)free_head + PGSIZE): NULL;
		free_head->Prev = free_head != _heap.start ? (page_t *)((uintptr_t)free_head - PGSIZE): NULL;
		free_head->freeh = NULL;
		free_head++;
	}
	free_head = (page_t *)((uintptr_t)_heap.start + PGSIZE);
	free_lock = 0;
	/*============initialize alloc head========*/
  alloc_head = _heap.start;
  alloc_head->size = 0;
  alloc_head->Next = NULL;
	alloc_head->Prev = NULL;
  alloc_head->freeh = NULL;
	/*===========initialize each cpu page==========*/
	for(int cpu = 0; cpu < _ncpu(); cpu++)
	{
		cpu_head[cpu] = bigmalloc();
	}

}

MODULE_DEF(pmm) = 
{
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
