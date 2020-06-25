#include <common.h>
struct 
{
  spinlock_t lock;
  inode_t inode[NINODE];
} icache;

void iinit()
{
  kmt->spin_init(&icache.lock, "icache");
  for(int i = 0; i < NINODE; i++) 
	{
    kmt->sem_init(&icache.inode[i].sem, "inode", 1);
  }
}

/*
inode_t *ialloc(device_t *dev, short type)
{
  int inum;
  struct buf *bp;
  struct dinode *dip;

  for(inum = 1; inum < sb.ninodes; inum++)
  {
    bp = bread(dev, IBLOCK(inum, sb));
    dip = (struct dinode*)bp->data + inum%IPB;
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
  panic("ialloc: no inodes");
}
*/
