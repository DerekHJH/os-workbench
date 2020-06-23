#ifndef BUFH
#define BUFH

#define MAXOPBLOCK 10
#define NBUF (MAXOPBLOCK * 3)

typedef struct _buf 
{
  int flags;
  uint dev;
  uint blockno;
  struct semaphore sem;
  uint refcnt;
  struct buf *prev; // LRU cache list
  struct buf *next;
  struct buf *qnext; // disk queue
  uint8_t data[BSIZE];
}buf_t;
#define B_VALID 0x2  // buffer has been read from disk
#define B_DIRTY 0x4  // buffer needs to be written to disk

#endif
