#include <common.h>
static void os_init() 
{
	//mpe still not on, no need to worry about mpe
  pmm->init();
	kmt->init();
	dev->init();
}

static void os_run() 
{
#ifdef DEBUG
	printf("Hello from CPU %d\n", _cpu());
#endif
	_intr_write(1);
	while(1)_yield();
}
_Context *kmt_schedule(_Event ev, _Context *ctx);
static _Context *os_trap(_Event ev, _Context *ctx)
{	
	return kmt_schedule(ev, ctx);	
}  
static void os_on_irq(int seq, int event, handler_t handler)
{
}
MODULE_DEF(os) = 
{
  .init = os_init,
  .run  = os_run,
	.trap = os_trap,
	.on_irq = os_on_irq,
};
