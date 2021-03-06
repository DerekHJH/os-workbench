#include <common.h>

static int null_init(device_t *dev)
{
	return 0;
}

static ssize_t null_write(device_t *dev, off_t offset, const void *buf, size_t count) 
{
	return 0;
}
static ssize_t null_read(device_t *dev, off_t offset, void *buf, size_t count) 
{
	unsigned char *temp = (unsigned char *)buf;
	for(int i = 0; i < count; i++)
		temp[i] = 0xff;
	return count;
}

devops_t null_ops = 
{
  .init  = null_init,
  .read  = null_read,
  .write = null_write,
};
