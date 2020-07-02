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

int yaq;
int ret1;
int ret;
uint8_t ss[8194], ans[8194];
void openclose_test(void* s)
{
	int fd = vfs->open("a.c", O_RDWR);
	int fd2 = vfs->dup(fd);
	panic_on(fd < 0 || fd2 < 0, "\033[31m fuck read\n\033[0m");
	for(int i = 0; i < 8194; i++)
		ss[i] = i % 26 + 1;
	vfs->write(fd, ss, 8194);
	vfs->lseek(fd, 10000, SEEK_END);
	vfs->lseek(fd2, 10000, SEEK_SET);
	vfs->write(fd, ss, 8194);
	vfs->lseek(fd, 26 * 200, SEEK_SET);
	vfs->read(fd2, ans, 8194);
	for(int i = 0; i < 8194; i++)
		printf("%d ", ans[i]);
	printf("\n");
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
	kmt->create(pmm->alloc(sizeof(task_t)), "openclose_test", openclose_test, "openclose_test");
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
