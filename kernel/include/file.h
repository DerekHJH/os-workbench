#ifndef FILEH
#define FILEH

typedef struct file 
{
  enum { FD_NONE, FD_INODE } type;
  int ref; // reference count
  char readable;
  char writable;
  inode_t *ip;
  uint32_t off;
}file_t;

#endif
