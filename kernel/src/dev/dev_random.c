#include <common.h>

static int random_init(device_t *dev)
{
	return 0;
}

static ssize_t random_write(device_t *dev, off_t offset, const void *buf, size_t count) 
{
	return 0;
}
static ssize_t random_read(device_t *dev, off_t offset, void *buf, size_t count) 
{
	unsigned char *temp = (unsigned char *)buf;
	for(int i = 0; i < count; i++)
		temp[i] = uptime() % 256;
	return count;
}

devops_t random_ops = 
{
  .init  = random_init,
  .read  = random_read,
  .write = random_write,
};


