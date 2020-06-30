#include <common.h>

file_t *ofile[NOFILE];
static void vfs_init()
{
	binit();
	iinit();
	fileinit();
	return;
}
static int vfs_write(int fd, void *buf, int count)
{
	return filewrite(ofile[fd], buf, count);
}
static int vfs_read(int fd, void *buf, int count)
{
	return fileread(ofile[fd], buf, count);
}
static int vfs_close(int fd)
{
	file_t *f = ofile[fd];
	ofile[fd] = NULL;
	fileclose(f);
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
	return filestat(ofile[fd], (stat_t *)buf);
}
static int vfs_mkdir(const char *pathname)
{
	return 0;
}
static int vfs_chdir(const char *path)
{
	return 0;
}
static int fdalloc(file_t *f)
{
	int fd;
	for(fd = 0; fd < NOFILE; fd++)
	{
    if(ofile[fd] == 0)
		{
      ofile[fd] = f;
      return fd;
    }
  }
  return -1;
}
static int vfs_dup(int fd)
{
	int fd2;
	if((fd2 = fdalloc(ofile[fd])) < 0)return -1;
	filedup(ofile[fd]);
	return fd2;
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

