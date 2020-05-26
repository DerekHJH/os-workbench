#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>
#include <macro.h>
struct task
{
	struct 
  {
    const char *name;
		int cpu;
		int stat;
		unsigned long long last;
		struct task *prev;
    struct task *next;
    _Context   *context;
  };
  uint8_t stack[TASKSIZE - HEADLEN];
};

typedef struct __cpu 
{
	task_t *current;
	task_t *head;
	task_t *tail;
	task_t idle;
}cpu_t;



struct spinlock
{
	intptr_t locked;	
	int intr;
	//for debugging
	char *name;
	int cpu;
};
struct semaphore
{
	intptr_t value;
	spinlock_t lock;
	task_t *queue[128];
	int head, tail;
	char *name;
};

typedef struct __taskop
{
	int cpu;
	spinlock_t lock;
}taskop_t;
