#ifndef FILEH
#define FILEH

typedef struct file 
{
  enum { FD_NONE, FD_INODE  } type;
  int ref; // reference count
  char readable;
  char writable;
  inode_t *ip;
  uint32_t off;
}file_t;

#endif

void fileinit();
file_t *filealloc();
file_t *filedup(file_t *f);
void fileclose(file_t *f);
int filestat(file_t *f, stat_t *st);
int fileread(file_t *f, char *addr, int n);
int filewrite(file_t *f, char *addr, int n);
