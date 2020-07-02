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
char ss[8194];
void openclose_test(void* s)
{
  int fd=vfs->open("b.c",O_RDONLY),fd2=vfs->open("copy.doc",O_RDWR|O_CREAT);
  int fd3=vfs->dup(fd2),fd4=vfs->open("copy2.doc",O_WRONLY|O_CREAT);
  for(int i=6;i<128;i++)
  {
  ret=vfs->lseek(fd,i*4096,SEEK_SET);
  ret=vfs->read(fd,ss,4100);
  ret=vfs->write(fd2,ss,4096);
  }
  ret=vfs->lseek(fd3,9*4096,SEEK_SET);
  ret=vfs->read(fd3,ss,8192);
	for(int i = 0; i <= 100; i++)
	printf("%d", ss[i]);
	printf("\n");
  ret=vfs->write(fd2,ss,4096);
  ret=vfs->read(fd3,ss,8192);
	printf("============ss is %s\n", ss);
  while ((ret=vfs->read(fd3,ss,4100))>0)
  {
    ++yaq;
    ret=vfs->write(fd4,ss,ret);
    ++yaq;
  }
  ret=vfs->close(fd);
  ret=vfs->close(fd2);
  ret=vfs->close(fd3);
  ret=vfs->close(fd4);
  //printf("%d\n",vfs->unlink("/dev/null"));
  printf("mkdir=%d\n",vfs->mkdir("home"));
  ret=vfs->chdir("home");
  printf("chdir answer=%d\n",ret);
  printf("mkdir=%d\n",vfs->mkdir("zhn"));
  fd=vfs->open("zhn/a.txt",O_CREAT|O_RDWR);
  printf("a.txt open to %d\n",fd);
  printf("write=%d\n",vfs->write(fd,"0123456789",10));
  fd2=vfs->dup(fd);
  vfs->lseek(fd,5,SEEK_SET);
  printf("read=%d\n",vfs->read(fd2,ss,6));
  printf("%s\n",ss);
  vfs->lseek(fd,9,SEEK_SET);
  printf("read=%d\n",vfs->read(fd2,ss,1));
  printf("%s\n",ss);
  vfs->close(fd);vfs->close(fd2);
  printf("link answer is %d\n",vfs->link("/home/zhn/a.txt","/home/b.txt"));
  ret=vfs->chdir("/home");
  printf("ret=%d\n",ret);
  fd3=vfs->open("b.txt",O_RDONLY);
  printf("b.txt open in %d\n",fd3);
  int test1=vfs->read(fd3,ss,5);
  printf("%d\n",test1);
  printf("%s\n",ss);
  printf("unlink=%d\n",vfs->unlink("/home/b.txt"));
  ret=vfs->lseek(fd3,11*4096,SEEK_SET);
  printf("read=%d\n",vfs->read(fd3,ss,5));
  printf("b.txt read in %d\n",vfs->open("b.txt",O_RDONLY));
  printf("%s\n",ss);
	printf("=====================================================\n");
	//assert(0);
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
