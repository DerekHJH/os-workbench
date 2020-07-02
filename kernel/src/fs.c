#include <common.h>
extern device_t *devices[];
struct 
{
  spinlock_t lock;
  inode_t inode[NINODE];
} icache;
static inode_t *iget(uint32_t dev, uint32_t inum);
static void itrunc(inode_t *ip);
static void bzero(uint32_t dev, int bno);
static uint32_t balloc(uint32_t dev);
static void bfree(uint32_t dev, uint32_t b);
static uint32_t bmap(inode_t *ip, uint32_t bn);
static char *skipelem(char *path, char *name);
static inode_t *namex(char *path, int nameiparent, char *name);
#define min(a, b) ((a) < (b) ? (a) : (b))
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

inode_t *ialloc(uint32_t dev, short type)
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
void ilock(inode_t *ip)
{
  buf_t *bp;
  dinode_t *dip;

  panic_on(ip == 0 || ip->ref < 1, "\033[31m ilock\n \033[0m");

  kmt->sem_wait(&ip->sem);

  if(ip->valid == 0)
  {
    bp = bread(ip->dev, IBLOCK(ip->inum));
    dip = (dinode_t *)bp->data + ip->inum % IPB;
    ip->type = dip->type;
    ip->major = dip->major;
    ip->minor = dip->minor;
    ip->nlink = dip->nlink;
    ip->size = dip->size;
    memmove(ip->addrs, dip->addrs, sizeof(ip->addrs));
    brelse(bp);
    ip->valid = 1;
    panic_on(ip->type == 0, "\033[31m ilock: no type\n \033[0m");
  }
}
void iunlock(inode_t *ip)
{
  kmt->sem_signal(&ip->sem);
}

void iput(inode_t *ip)
{

	printf("askhkcvasijcbaskjcasj\n");
	kmt->sem_wait(&ip->sem);
  if(ip->valid && ip->nlink == 0)
  {
    kmt->spin_lock(&icache.lock);
    int r = ip->ref;
    kmt->spin_unlock(&icache.lock);
    if(r == 1)
    {
      // inode has no links and no other references: truncate and free.
      itrunc(ip);
      ip->type = 0;
      iupdate(ip);
      ip->valid = 0;
    }
  }
	kmt->sem_signal(&ip->sem);

	kmt->spin_lock(&icache.lock);
  ip->ref--;
  kmt->spin_unlock(&icache.lock);

	return;
}
void iunlockput(inode_t *ip)
{
  iunlock(ip);
  iput(ip);
	return;
}
void stati(inode_t *ip, stat_t *st)
{
  st->id = ip->inum;
  st->type = ip->type;
  st->size = ip->size;
	return;
}
int readi(inode_t *ip, char *dst, uint32_t off, uint32_t n)
{
  uint32_t tot, m;
  buf_t *bp;

	
	if(ip->major > 0)
	{
		if(ip->major < 0 || ip->major >= NDEV || !devices[ip->major]->ops->read)return -1;
		return devices[ip->major]->ops->read(devices[ip->major], off, dst, n);  
	}
	

  if(off > ip->size || off + n < off)return -1;
  if(off + n > ip->size)n = ip->size - off;

  for(tot = 0; tot < n; tot += m, off += m, dst += m)
  {
    bp = bread(ip->dev, bmap(ip, off / BSIZE));
    m = min(n - tot, BSIZE - off % BSIZE);
    memmove(dst, bp->data + off % BSIZE, m);
    brelse(bp);
  }
  return n;
}
int writei(inode_t *ip, char *src, uint32_t off, uint32_t n)
{
  uint32_t tot, m;
  buf_t *bp;

	if(ip->major > 0)
  {
  	if(ip->major < 0 || ip->major >= NDEV || !devices[ip->major]->ops->write)return -1;
  	return devices[ip->major]->ops->write(devices[ip->major], off, src, n);  
  }

  if(off > ip->size || off + n < off)return -1;
  if(off + n > MAXFILE * BSIZE)return -1;

  for(tot = 0; tot < n; tot += m, off += m, src += m)
  {
    bp = bread(ip->dev, bmap(ip, off / BSIZE));
    m = min(n - tot, BSIZE - off % BSIZE);
    memmove(bp->data + off % BSIZE, src, m);
    log_write(bp);
    brelse(bp);
  }

  if(n > 0 && off > ip->size)
  {
    ip->size = off;
    iupdate(ip);
  }
  return n;
}

int namecmp(const char *s, const char *t)
{
  return strncmp(s, t, DIRSIZ);
}

inode_t *dirlookup(inode_t *dp, char *name, uint32_t *poff)
{
  uint32_t off, inum;
  dirent_t de;

  panic_on(dp->type != T_DIR, "\033[31m dirlookup not DIR\n \033[0m");

  for(off = 0; off < dp->size; off += sizeof(de))
  {
    panic_on(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de), "\033[31m dirlookup read\n \033[0m");
    if(de.inode == 0)continue;
    if(namecmp(name, de.name) == 0)
    {
      // entry matches path element
      if(poff)*poff = off;
      inum = de.inode;
      return iget(dp->dev, inum);
    }
  }
  return 0;
}

int dirlink(inode_t *dp, char *name, uint32_t inum)
{
  int off;
  dirent_t de;
  inode_t *ip;

  // Check that name is not present.
  if((ip = dirlookup(dp, name, 0)) != 0)
  {
    iput(ip);
    return -1;
  }

  // Look for an empty dirent.
  for(off = 0; off < dp->size; off += sizeof(de))
  {
    panic_on(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de), "\033[31m dirlink read\n \033[0m");
    if(de.inode == 0)break;
  }

  strncpy(de.name, name, DIRSIZ);
  de.inode = inum;
  panic_on(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de), "\033[31m dirlink\n \033[0m");

  return 0;
}

inode_t *namei(char *path)
{
  char name[DIRSIZ];
  return namex(path, 0, name);
}

inode_t *nameiparent(char *path, char *name)
{
  return namex(path, 1, name);
}

/*===================static function==================*/

static inode_t *iget(uint32_t dev, uint32_t inum)
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

static void itrunc(inode_t *ip)
{
  int i, j;
  buf_t *bp;
  uint32_t *a;

  for(i = 0; i < NDIRECT; i++)
  {
    if(ip->addrs[i])
    {
      bfree(ip->dev, ip->addrs[i]);
      ip->addrs[i] = 0;
    }
  }

  if(ip->addrs[NDIRECT])
  {
    bp = bread(ip->dev, ip->addrs[NDIRECT]);
    a = (uint32_t *)bp->data;
    for(j = 0; j < NINDIRECT; j++)
    {
      if(a[j])bfree(ip->dev, a[j]);
		}
    brelse(bp);
    bfree(ip->dev, ip->addrs[NDIRECT]);
    ip->addrs[NDIRECT] = 0;
  }

  ip->size = 0;
  iupdate(ip);
}

static void bzero(uint32_t dev, int bno)
{
  buf_t *bp;

  bp = bread(dev, bno);
  memset(bp->data, 0, BSIZE);
  log_write(bp);
  brelse(bp);
}

static uint32_t balloc(uint32_t dev)
{
  int b, bi, m;
  buf_t *bp = NULL;

  for(b = 0; b < FSSIZE; b += BPB)
  {
    bp = bread(dev, BBLOCK(b));
    for(bi = 0; bi < BPB && b + bi < FSSIZE; bi++)
    {
      m = 1 << (bi % 8);
      if((bp->data[bi / 8] & m) == 0)
      {  // Is block free?
        bp->data[bi / 8] |= m;  // Mark block in use.
        log_write(bp);
        brelse(bp);
        bzero(dev, b + bi);
        return b + bi;
      }
    }
    brelse(bp);
  }
  panic_on(1, "\033[31m balloc: out of blocks\n \033[0m");
}

static void bfree(uint32_t dev, uint32_t b)
{
  buf_t *bp;
  int bi, m;

  bp = bread(dev, BBLOCK(b));
  bi = b % BPB;
  m = 1 << (bi % 8);
  panic_on((bp->data[bi / 8] & m) == 0, "\033[31m freeing free block\n \033[0m");
  bp->data[bi / 8] &= ~m;
  log_write(bp);
  brelse(bp);
}
static uint32_t bmap(inode_t *ip, uint32_t bn)
{
  uint32_t addr, *a;
  buf_t *bp;

  if(bn < NDIRECT)
  {
    if((addr = ip->addrs[bn]) == 0)ip->addrs[bn] = addr = balloc(ip->dev);
    return addr;
  }
  bn -= NDIRECT;

  if(bn < NINDIRECT)
  {
    // Load indirect block, allocating if necessary.
    if((addr = ip->addrs[NDIRECT]) == 0)ip->addrs[NDIRECT] = addr = balloc(ip->dev);
    bp = bread(ip->dev, addr);
    a = (uint32_t *)bp->data;
    if((addr = a[bn]) == 0)
    {
      a[bn] = addr = balloc(ip->dev);
      log_write(bp);
    }
    brelse(bp);
    return addr;
  }

  panic_on(1, "\033[31m bmap: out of range\n \033[0m");
}

static char *skipelem(char *path, char *name)
{
  char *s;
  int len;

  while(*path == '/')path++;
  if(*path == 0)return 0;
  s = path;
  while(*path != '/' && *path != 0)path++;
  len = path - s;
  if(len >= DIRSIZ)memmove(name, s, DIRSIZ);
  else 
  {
    memmove(name, s, len);
    name[len] = 0;
  }
  while(*path == '/')path++;
  return path;
}
extern cpu_t cpuinfo[MAXNCPU];
static inode_t *namex(char *path, int nameiparent, char *name)
{
	
  inode_t *ip, *next;

  if(*path == '/')ip = iget(ROOTDEV, ROOTINO);
  else ip = idup(cpuinfo[_cpu()].current->cwd);

  while((path = skipelem(path, name)) != 0)
  {
    ilock(ip);
    if(ip->type != T_DIR)
    {
      iunlockput(ip);
      return 0;
    }
    if(nameiparent && *path == '\0')
    {
      // Stop one level early.
      iunlock(ip);
      return ip;
    }
    if((next = dirlookup(ip, name, 0)) == 0)
    {
      iunlockput(ip);
      return 0;
    }
    iunlockput(ip);
    ip = next;
  }

		printf("askhjavhucakabkcs\n");
  if(nameiparent)
  {
    iput(ip);
    return 0;
  }
  return ip;
	
}
