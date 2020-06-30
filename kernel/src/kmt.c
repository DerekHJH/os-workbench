#include <common.h>
#define CPU cpuinfo[_cpu()]
#define Current cpuinfo[_cpu()].current
#define Head cpuinfo[_cpu()].head
#define Tail cpuinfo[_cpu()].tail
#define Idle &(cpuinfo[_cpu()].idle)
cpu_t cpuinfo[MAXNCPU];
taskop_t taskop;
int idcnt = 0;
void insert_task(int cpu, task_t *task)
{
	task->next = NULL;
  task->prev = cpuinfo[cpu].tail;
  cpuinfo[cpu].tail->next = task;
  cpuinfo[cpu].tail = task;
}
void delete_task(int cpu, task_t *task)
{
	if(task->next != NULL)
  {
  	task->next->prev = task->prev;
  	task->prev->next = task->next;
  }
  else
  {
  	cpuinfo[cpu].tail = task->prev;
  	cpuinfo[cpu].tail->next = NULL;
  }
}

static void kmt_spin_init(spinlock_t *lk, const char *name)
{
	lk->locked = 0;
	lk->intr = 0;
	lk->name = (char *)name;
	lk->cpu = 0;
}
static void kmt_spin_lock(spinlock_t *lk)
{
	int intr = _intr_read();
  _intr_write(0);
	while(_atomic_xchg(&(lk->locked), 1));

	lk->intr = intr;
	lk->cpu = _cpu();//debug
	panic_on(lk->locked != 1, "\033[31mlock != 1\033[0m\n");
  panic_on(_intr_read() != 0, "\033[31m_init_read() != 0\033[0m\n");
}
static void kmt_spin_unlock(spinlock_t *lk)
{	
	int intr = lk->intr;
	_atomic_xchg(&(lk->locked), 0);
	if(intr)_intr_write(1);
}
static void kmt_sem_init(sem_t *sem, const char *name, int value)
{
	sem->value = value;
	kmt_spin_init(&(sem->lock), name);
	sem->head = 0;
	sem->tail = 0;
	sem->name = (char *)name;
}

static void kmt_sem_wait(sem_t *sem)
{
	int Flag = 0;
	kmt_spin_lock(&(sem->lock));
	sem->value--;
	if(sem->value < 0)
	{
		Flag = 1;
		Current->stat = HUNG;	
		sem->queue[sem->tail] = Current;
		sem->tail = (sem->tail + 1) % QLEN;
		panic_on(sem->tail == sem->head, "\033\31mIn kmt_sem_wait, sem->tail == sem->head\033[0m\n");
	}
	kmt_spin_unlock(&(sem->lock));
	if(Flag)_yield();//might be buggy
}
static void kmt_sem_signal(sem_t *sem)
{
	kmt_spin_lock(&(sem->lock));
	sem->value++;
	if(sem->head != sem->tail)
	{
		sem->queue[sem->head]->stat = READY;
		sem->head = (sem->head + 1) % QLEN;
	}	                                 
  kmt_spin_unlock(&(sem->lock));
}

static int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg)
{
	kmt_spin_lock(&taskop.lock);
  _Area stack   = (_Area) {&task->context + 1, task + 1};
  panic_on(stack.end - stack.start != TASKSIZE - HEADLEN, "\033[31mstack size is wrong at kmt_create\033[0m\n");
  task->name = (char *)name;
  task->context = _kcontext(stack, entry, arg);
	task->stat = READY;
	task->last = 0;
	task->cwd = namei("/");
	task->id = ++idcnt;

	task->cpu = taskop.cpu;

	insert_task(taskop.cpu, task);

	taskop.cpu = (taskop.cpu + 1) % _ncpu();
	kmt_spin_unlock(&taskop.lock);
#ifdef DEBUG
	printf("Create at 0x%x, task->name is %s, task->stack is %x - %x\n", task, task->name, stack.start, stack.end);
#endif
	return 0;
}
static void kmt_teardown(task_t *task)
{
	kmt_spin_lock(&taskop.lock);
	delete_task(task->cpu, task);	
	kmt_spin_unlock(&taskop.lock);
	pmm->free(task);
}

static _Context *kmt_context_save(_Event ev, _Context *ctx)
{
	kmt->spin_lock(&taskop.lock); 
	Current->context = ctx;       
	kmt->spin_unlock(&taskop.lock); 
	return NULL;
}

static _Context *kmt_schedule(_Event ev, _Context *ctx)
{
	kmt->spin_lock(&taskop.lock); 
  Current->context = ctx;

	if(Current != Idle)
	{
		delete_task(Current->cpu, Current);
		panic_on(Current->cpu != _cpu(), "\033[31mIn kmt_schdule, Current->cpu != _cpu()\033[0m\n");
		//if(_ncpu() == 6)Current->cpu = (Current->cpu + 1) % _ncpu();
		insert_task(Current->cpu, Current);
	}

	task_t *cur = Head->next;
	while(cur)
	{
		//if(_ncpu() == 6)
		//{
		//	if(cur->stat == READY && uptime() - cur->last > MAXTIME)break;
		//}
		//else 
		if(cur->stat == READY)break;
		cur = cur->next;
	}
	
	Current->last = uptime();	
	if(cur)Current = cur; 
	else Current = Idle;	
	
  kmt->spin_unlock(&taskop.lock);
  return Current->context;
}
static void kmt_init()
{
	/*==========initialize taskop===========*/
	kmt_spin_init(&taskop.lock, "taskop.lock");
	taskop.cpu = 0;
	/*==========initialize cpu=========*/
	for(int i = 0; i < _ncpu(); i++)
	{
		cpuinfo[i].current = &cpuinfo[i].idle;
		cpuinfo[i].head = &cpuinfo[i].idle;
		cpuinfo[i].tail = &cpuinfo[i].idle;
		
		cpuinfo[i].idle.name = "idle";
		cpuinfo[i].idle.cpu = i;
		cpuinfo[i].idle.stat = READY;
		cpuinfo[i].idle.last = i;
		cpuinfo[i].idle.prev = NULL;
		cpuinfo[i].idle.next = NULL;
		cpuinfo[i].idle.context = NULL;	
		cpuinfo[i].idle.cwd = NULL;
	}
	/*==========on_orq=================*/
	os->on_irq(-999999999, _EVENT_NULL, kmt_context_save);
	os->on_irq(999999999, _EVENT_NULL, kmt_schedule);
}



MODULE_DEF(kmt) = 
{
  .init = kmt_init,
  .create  = kmt_create,
	.teardown = kmt_teardown,
	.spin_init = kmt_spin_init,
	.spin_lock = kmt_spin_lock,
	.spin_unlock = kmt_spin_unlock,
	.sem_init = kmt_sem_init,
	.sem_wait = kmt_sem_wait,
	.sem_signal = kmt_sem_signal,
};


