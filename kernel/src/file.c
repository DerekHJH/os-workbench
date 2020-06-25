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


