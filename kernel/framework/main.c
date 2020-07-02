#include <klib.h>
#include <klib-macros.h>
#include <am.h>
#include <common.h>
/*
#define MAX 1000000
sem_t mutex, empty, full;
spinlock_t lock;
void producer(void *arg)
{
	char *name = (char *)arg;
	while(1)
	{
		panic_on(_intr_read() == 0, "\033[31mproducer intr = 0\033[0m\n");

		kmt->sem_wait(&mutex);
		printf("%s\n", name);
		kmt->sem_signal(&mutex);
		_yield();
	}
}
void consumer(void *arg)
{
	char *name = (char *)arg;
	while(1)
	{
		panic_on(_intr_read() == 0, "\033[31mconsumer intr = 0\033[0m\n");
		kmt->sem_wait(&mutex);
		printf("%s\n", name);
		kmt->sem_signal(&mutex);
		_yield();
	}
}
*/
void fileoperation(void *arg)
{
	char *filename = (char *)arg;
	printf("before open\n");
	int fd = vfs->open(filename, O_CREAT | O_RDWR);
	printf("filename is %s\n", filename);
	printf("after open\n");
	if(fd < 0)
	{
		printf("shit fd is negative\n");
	}
	printf("before write\n");
	vfs->write(fd, filename, strlen(filename));
	printf("after write\n");
	char ans[128] = "\0";
	printf("before read\n");
	vfs->read(fd, ans, strlen(filename));
	printf("after read\n");
	printf("===================ans is %s\n", ans);
	while(1);
}
void create_threads()
{
	//kmt->create(pmm->alloc(sizeof(task_t)), "producer1", producer, "producer1");
	//kmt->create(pmm->alloc(sizeof(task_t)), "consumer1", consumer, "consumer1");
	//kmt->sem_init(&mutex, "mutex", 1);
	kmt->create(pmm->alloc(sizeof(task_t)), "fileoperation", fileoperation, "fileoperation");
}

int main(const char *args) 
{
	printf("\033[31mCPU reset\033[0m\n");
  _ioe_init();
  _cte_init(os->trap);
  os->init();

	create_threads();
  _mpe_init(os->run);
}
