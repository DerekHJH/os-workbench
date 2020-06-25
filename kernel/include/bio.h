#ifndef BUFH
#define BUFH

#define MAXOPBLOCKS 10
#define NBUF (MAXOPBLOCKS * 3)
typedef struct _buf 
{
  int flags;
  device_t *dev;
  uint blockno;
  sem_t sem;
  uint refcnt;
  struct _buf *prev; // LRU cache list
  struct _buf *next;
  uint8_t data[BSIZE];
}buf_t;
#define B_VALID 0x2  // buffer has been read from disk
#define B_DIRTY 0x4  // buffer needs to be written to disk

#endif
buf_t *bread(device_t *dev, uint32_t blockno);
void bwrite(buf_t *b);
void brelse(buf_t *b);
void binit(void);
