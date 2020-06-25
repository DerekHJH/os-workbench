#include <common.h>

struct 
{
  spinlock_t lock;
  file_t file[NFILE];
} ftable;

void fileinit()
{
	kmt->spin_init(&ftable.lock, "ftable");
}

file_t *filealloc()
{
  file_t *f;

  kmt->spin_lock(&ftable.lock);
  for(f = ftable.file; f < ftable.file + NFILE; f++)
  {
    if(f->ref == 0)
    {
      f->ref = 1;
      kmt->spin_unlock(&ftable.lock);
      return f;
    }
  }
  kmt->spin_unlock(&ftable.lock);
  return 0;
}
file_t *filedup(file_t *f)
{
	kmt->spin_lock(&ftable.lock);
  panic_on(f->ref < 1, "\033[31m filedup\n \033[0m");
  f->ref++;
	kmt->spin_unlock(&ftable.lock);
  return f;
}
void fileclose(file_t *f)
{
  file_t ff;

	kmt->spin_lock(&ftable.lock);
	panic_on(f->ref < 1, "\033[31m filedclose\n \033[0m");

  if(--f->ref > 0)
  {
		kmt->spin_unlock(&ftable.lock);
    return;
  }
  ff = *f;
  f->ref = 0;
  f->type = FD_NONE;

	kmt->spin_unlock(&ftable.lock);

  if(ff.type == FD_INODE)iput(ff.ip);
}

int filestat(file_t *f, stat_t *st)
{
  if(f->type == FD_INODE)
  {
    ilock(f->ip);
    stati(f->ip, st);
    iunlock(f->ip);
    return 0;
  }
  return -1;
}
int fileread(file_t *f, char *addr, int n)
{
  int r;

  if(f->readable == 0)return -1;
  if(f->type == FD_INODE)
  {
    ilock(f->ip);
    if((r = readi(f->ip, addr, f->off, n)) > 0)f->off += r;
    iunlock(f->ip);
    return r;
  }
  panic_on(1, "\033[31m fileread reached the end\n\33[0m");
}
int filewrite(file_t *f, char *addr, int n)
{
  int r;

  if(f->writable == 0)return -1;
  if(f->type == FD_INODE)
  {
    int max = ((MAXOPBLOCKS - 1 - 1 - 2) / 2) * 512;
    int i = 0;
    while(i < n)
    {
      int n1 = n - i;
      if(n1 > max)n1 = max;

      ilock(f->ip);
      if ((r = writei(f->ip, addr + i, f->off, n1)) > 0)f->off += r;
      iunlock(f->ip);

      if(r < 0)break;
      panic_on(r != n1, "\033[31m short filewrite\n \033[0m");
      i += r;
    }
    return i == n ? n : -1;
  }
	panic_on(1, "\033[31m filewrite reached the end\n\33[0m");
}
