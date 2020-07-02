#include <common.h>
#define MAXHANDLE 100
typedef struct _ehandle
{
	int seq;
	int event;
	handler_t handler;
}ehandle_t;
int handletot = 0;
ehandle_t handles[MAXHANDLE];

void sort_handles()
{
	ehandle_t t;
	for(int i = 0; i < handletot - 1; i++)
	{
		for(int j = 0; j < handletot - i - 1; j++)
			if(handles[j].seq > handles[j + 1].seq)
			{
				t = handles[j + 1];
				handles[j + 1] = handles[j];
				handles[j] = t;
			}
	}	
}


void fileoperation(void *arg)
{
	char *filename = (char *)arg;

	printf("filename is %s\n", filename);
	printf("before open\n");
	int fd = vfs->open(filename, O_CREAT | O_RDWR);
	printf("after open\n");
	if(fd < 0)
	{
		printf("shit fd is negative\n");
	}
	printf("before write\n");
	vfs->write(fd, filename, strlen(filename));
	printf("after write\n");
	char ans[128] = "\0";
	printf("before lseek\n");
  vfs->lseek(fd, 0, SEEK_SET);
  printf("after lseek\n");


	printf("before read\n");
	vfs->read(fd, ans, strlen(filename));
	printf("after read\n");
	printf("===================ans is %s\n", ans);
	while(1);
}
static void os_init() 
{
	//mpe still not on, no need to worry about mpe
  pmm->init();
	kmt->init();
	dev->init();
	vfs->init();
	sort_handles();
	kmt->create(pmm->alloc(sizeof(task_t)), "fileoperation", fileoperation, "fileoperation");
}

static void os_run() 
{
#ifdef DEBUG	
	printf("Hello from CPU %d\n", _cpu());
#endif
	_intr_write(1);
	while(1)_yield();
}

static void os_on_irq(int seq, int event, handler_t handler)
{
	panic_on(handletot == MAXHANDLE, "\033[31mtoo many handlers!! \033[0n\n");
	handles[handletot].seq = seq;
	handles[handletot].event = event;	
	handles[handletot].handler = handler;
	handletot++;
}

_Context *kmt_schedule(_Event ev, _Context *ctx);
static _Context *os_trap(_Event ev, _Context *ctx)
{	
	_Context *next = NULL;
  for (int i = 0; i < handletot; i++) 
	{
    if(handles[i].event == _EVENT_NULL || handles[i].event == ev.event) 
		{
      _Context *r = handles[i].handler(ev, ctx);
      panic_on(r && next, "\033[31mreturning multiple contexts\n\033[0m");
      if(r)next = r;
    }
  }
  panic_on(!next, "\033[31mreturning NULL context\033[0m\n");
  return next;
	//return kmt_schedule(ev, ctx);	
}  

MODULE_DEF(os) = 
{
  .init = os_init,
  .run  = os_run,
	.trap = os_trap,
	.on_irq = os_on_irq,
};
