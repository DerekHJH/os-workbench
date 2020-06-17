#include <common.h>

static void vfs_init()
{
	return;
}
static int vfs_write(int fd, void *buf, int count)
{
	return 0;
}
static int vfs_read(int fd, void *buf, int count)
{
	return 0;
}
static int vfs_close(int fd)
{
	return 0;
}
static int vfs_open(const char *pathname, int flags)
{
	return 0;
}
static int vfs_lseek(int fd, int offset, int whence)
{
	return 0;
}
static int vfs_link(const char *oldpath, const char *newpath)
{
	return 0;
}
static int vfs_unlink(const char *pathname)
{
	return 0;
}
static int vfs_fstat(int fd, struct ufs_stat *buf)
{
	return 0;
}
static int vfs_mkdir(const char *pathname)
{
	return 0;
}
static int vfs_chdir(const char *path)
{
	return 0;
}
static int vfs_dup(int fd)
{
	uint a = 2;
	printf("%d", a);
	return 0;
}


MODULE_DEF(vfs) = 
{
	.init = vfs_init,
  .write = vfs_write,
  .read = vfs_read,
  .close = vfs_close,
  .open = vfs_open,
  .lseek = vfs_lseek,
  .link = vfs_link,
  .unlink = vfs_unlink,
  .fstat = vfs_fstat,
  .mkdir = vfs_mkdir,
  .chdir = vfs_chdir,
  .dup = vfs_dup,
};

