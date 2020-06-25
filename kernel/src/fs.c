#include <common.h>
struct 
{
  spinlock_t lock;
  inode_t inode[NINODE];
} icache;

void log_write(buf_t *bp)
{
	bwrite(bp);
}
void iinit()
{
  kmt->spin_init(&icache.lock, "icache");
  for(int i = 0; i < NINODE; i++) 
	{
    kmt->sem_init(&icache.inode[i].sem, "inode", 1);
  }
}

static inode_t *iget(device_t *dev, uint32_t inum)
{
  inode_t *ip, *empty;

  kmt->spin_lock(&icache.lock);

  // Is the inode already cached?
  empty = 0;
  for(ip = &icache.inode[0]; ip < &icache.inode[NINODE]; ip++)
  {
    if(ip->ref > 0 && ip->dev == dev && ip->inum == inum)
    {
      ip->ref++;
      kmt->spin_unlock(&icache.lock);
      return ip;
    }
    if(empty == 0 && ip->ref == 0)    // Remember empty slot.
      empty = ip;
  }

  // Recycle an inode cache entry.
  panic_on(empty == 0, "\033[31m iget: no inodes\n \033[0m");

  ip = empty;
  ip->dev = dev;
  ip->inum = inum;
  ip->ref = 1;
  ip->valid = 0;
  kmt->spin_unlock(&icache.lock);

  return ip;
}

inode_t *ialloc(device_t *dev, short type)
{
  buf_t *bp;
  dinode_t *dip;

  for(int inum = 1; inum < NINODES; inum++)
  {
    bp = bread(dev, IBLOCK(inum));
    dip = (dinode_t *)bp->data + inum % IPB;
    if(dip->type == 0)
    {  // a free inode
      memset(dip, 0, sizeof(*dip));
      dip->type = type;
      log_write(bp);   // mark it allocated on the disk
      brelse(bp);
      return iget(dev, inum);
    }
    brelse(bp);
  }
  panic_on(1, "\033[31m ialloc: no inodes \033[0m\n");
}

void iupdate(inode_t *ip)
{
  buf_t *bp;
  dinode_t *dip;

  bp = bread(ip->dev, IBLOCK(ip->inum));
  dip = (dinode_t *)bp->data + ip->inum % IPB;
  dip->type = ip->type;
  dip->major = ip->major;
  dip->minor = ip->minor;
  dip->nlink = ip->nlink;
  dip->size = ip->size;
  memmove(dip->addrs, ip->addrs, sizeof(ip->addrs));
  log_write(bp);
  brelse(bp);
}
inode_t *idup(inode_t *ip)
{
  kmt->spin_lock(&icache.lock);
  ip->ref++;
  kmt->spin_unlock(&icache.lock);
  return ip;
}
