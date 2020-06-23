#include <common.h>
extern device_t *devices[];
struct 
{
  spinlock_t lock;
  buf_t buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // head.next is most recently used.
  buf_t head;
} bcache;

spinlock_t idelock;
void binit(void)
{
  buf_t *b;

  kmt->spin_init(&bcache.lock, "bcache");
	kmt->spin_init(&idelock, "idelock");
  bcache.head.prev = &bcache.head;
  bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf + NBUF; b++)
  {
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    kmt->sem_init(&b->sem, "buffer", 1);
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
}

static buf_t *bget(device_t *dev, uint32_t blockno)
{
  buf_t *b;

  kmt->spin_lock(&bcache.lock);

  for(b = bcache.head.next; b != &bcache.head; b = b->next)
  {
    if(b->dev == dev && b->blockno == blockno)
    {
      b->refcnt++;
      kmt->spin_unlock(&bcache.lock);
      kmt->sem_wait(&b->sem);
      return b;
    }
  }

  for(b = bcache.head.prev; b != &bcache.head; b = b->prev)
  {
    if(b->refcnt == 0 && (b->flags & B_DIRTY) == 0) 
    {
      b->dev = dev;
      b->blockno = blockno;
      b->flags = 0;
      b->refcnt = 1;
      kmt->spin_unlock(&bcache.lock);
      kmt->sem_wait(&b->sem);
      return b;
    }
  }
  panic_on(1, "\033[31mbget: no buffers \033[0m\n");
}

void iderw(buf_t *b)
{
  panic_on((b->flags & (B_VALID|B_DIRTY)) == B_VALID, "\033[31m iderw: nothing to do\033[0m\n");
  kmt->spin_lock(&idelock);
	if(b->flags & B_DIRTY)
	{
		b->dev->ops->write(b->dev, b->blockno * BSIZE, b->data, BSIZE);
		b->flags ^= B_DIRTY;
	}
	if((b->flags & B_VALID) == 0)
	{
		b->dev->ops->read(b->dev, b->blockno * BSIZE, b->data, BSIZE);
		b->flags |= B_VALID;
	}
  kmt->spin_unlock(&idelock);
}

buf_t *bread(device_t *dev, uint32_t blockno)
{
  buf_t *b;

  b = bget(dev, blockno);
  if((b->flags & B_VALID) == 0) 
  {
    iderw(b);
  }
  return b;
}

void bwrite(buf_t *b)
{
  b->flags |= B_DIRTY;
  iderw(b);
	return;
}
void brelse(buf_t *b)
{

  kmt->sem_signal(&b->sem);

  kmt->spin_lock(&bcache.lock);
  b->refcnt--;
  if (b->refcnt == 0) 
  {
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
  
  kmt->spin_unlock(&bcache.lock);
}
