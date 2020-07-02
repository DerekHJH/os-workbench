#ifndef OSH
#define OSH
/*==========cpu============*/
#define MAXNCPU 8


/*=============task============*/
#define TASKSIZE 4096
#define READY 1
#define HUNG 2
#define HEADLEN (5 * sizeof(size_t) + 4 * sizeof(int))
 /*===========sem==============*/
#define QLEN 128

/*===========time==========*/
#define MAXTIME 10

struct task
{
	struct 
  {
    const char *name;
		int cpu;
		int stat;
		int last;
		int id;
		void *cwd;//inode
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
	int idcnt;
	spinlock_t lock;
}taskop_t;
#endif




