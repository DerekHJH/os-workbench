#include <common.h>

static int zero_init(device_t *dev)
{
	return 0;
}

static ssize_t zero_write(device_t *dev, off_t offset, const void *buf, size_t count) 
{
	return 0;
}
static ssize_t zero_read(device_t *dev, off_t offset, void *buf, size_t count) 
{
	unsigned char *temp = (unsigned char *)buf;
	for(int i = 0; i < count; i++)
		temp[i] = 0;
	return count;
}

devops_t zero_ops = 
{
  .init  = zero_init,
  .read  = zero_read,
  .write = zero_write,
};

