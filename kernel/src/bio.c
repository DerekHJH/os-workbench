#include <common.h>

struct 
{
  struct spinlock lock;
  buf_t buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // head.next is most recently used.
  buf_t head;
} bcache;
