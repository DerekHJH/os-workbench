#include "co.h"
#include <stdlib.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#define STACK_SIZE 64 * 1024
#if __x86_64__
#define MASK 7
#else
#define MASK 3
#endif
enum co_status
{
	CO_NEW = 1,
	CO_RUNNING,
	CO_WAITING,
	CO_DEAD,
};
struct co 
{
	const char *Name;
	void (*func)(void *);
	void *arg;
	enum co_status Status;
	struct co *Waiter;
	jmp_buf Context;
	struct co *Next;
	uint8_t Stack[STACK_SIZE]__attribute__((aligned(16)));
};

struct co *Current = NULL;

struct co *Head = NULL;
int tot = 0;
void Add(struct co *temp)
{
	temp->Next = Head->Next;
	Head->Next = temp;
	tot++;
}

void Delete(struct co *val)
{
	struct co *temp = Head;
	while(temp->Next != NULL)
	{
		if((temp->Next) == val)
		{
			temp->Next = val->Next;
			free(val);
			tot--;
			break;
		}
		temp = temp->Next;
	}
}

struct co *Random()
{
	if(tot > 0)
	{	
		struct co *temp = Head;
		int Choose = rand() % tot + 1;
		while(Choose > 0)
		{
			temp = temp->Next;
			Choose--;
		}
		if(temp->Status == CO_DEAD)
		{
			temp = Head->Next;
			while(temp != NULL && temp->Status == CO_DEAD)temp = temp->Next; 
		}
		return temp;
	}
	else return NULL;
}

static inline void stack_switch_call(void *sp, void *entry, uintptr_t arg) 
{
  asm volatile (
#if __x86_64__
    "movq %0, %%rsp; movq %2, %%rdi; jmp *%1" : : "b"((uintptr_t)sp),     "d"(entry), "a"(arg)
#else
    "movl %0, %%esp; movl %2, 4(%0); jmp *%1" : : "b"((uintptr_t)sp), "d"(entry), "a"(arg)
#endif
  );
}

__attribute__((constructor))static void Initiate()
{
	srand(time(NULL));
	Head = malloc(sizeof(struct co));
	Head->Next = NULL;
	Current = co_start("main", NULL, NULL);	
	Current->Status = CO_RUNNING; 
}

__attribute__((destructor))static void End()
{
	while(Head->Next != NULL)Delete(Head->Next);
}

struct co *co_start(const char *name, void (*func)(void *), void *arg) 
{
	struct co *thd = malloc(sizeof(struct co)); //Already freed it
	thd->Name = name;
	thd->func = func;
	thd->arg = arg;
	thd->Status = CO_NEW;
	thd->Waiter = NULL;
	Add(thd);
  return thd;
}

void co_wait(struct co *co) 
{
	while(co->Status != CO_DEAD)co_yield();
	Delete(co);
}

void Finish()
{
	Current->Status = CO_DEAD;
	struct co *temp = Head;
	while(temp->Next != NULL)temp = temp->Next;
	Current = temp;
	longjmp(Current->Context, 1);
}
void co_yield() 
{
	int val = setjmp(Current->Context);
	//printf("Current is %s\n", Current->Name);
	if(val == 0)
	{
	  Current = Random();	
		//printf("Next is %s func is 0x%p stack is %p\n", Current->Name, Current->func, Current->Stack);
    if(Current->Status == CO_NEW)
    {	
    	Current->Status = CO_RUNNING;
			uintptr_t temp = (uintptr_t)Current->Stack + STACK_SIZE - 1;
			temp = temp - MASK;
			uintptr_t retq = (uintptr_t)Finish;
			memcpy((char *)temp, &retq, MASK + 1);
    	stack_switch_call((void *)temp, Current->func, (uintptr_t)Current->arg);
    }
		else longjmp(Current->Context, 1);	
	}
}
