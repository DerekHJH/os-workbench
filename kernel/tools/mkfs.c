#include <user.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <../include/vfs.h>

#define uint unsigned int
#define ushort unsigned short
#define uchar unsigned char

#define panic_on(cond, s)\
	do\
	{\
		if(cond)\
		{\
			printf(s);\
			assert(0);\
		}\
	}while(0)

#define IMG_SIZE (64 * 1024 * 1024)
int fd;
uint8_t *disk;
int freeinode = 1;
int freeblock = DATASTART;

uint xint(uint x)
{
	return x;
}
ushort xshort(ushort x)
{
	return x;
}

void write_block(uint bnum, void *buf)
{
  panic_on(lseek(fd, bnum * BSIZE, 0) != bnum * BSIZE, "\033[31m In write_block, lseek(fd, bnum * BSIZE, 0) != sec * BSIZE\033[0m\n");
	panic_on(write(fd, buf, BSIZE) != BSIZE, "\033[31m In write_block, write(fd, buf, BSIZE) != BSIZE\033[0m\n");
}
void read_block(uint bnum, void *buf)
{
	panic_on(lseek(fd, bnum * BSIZE, 0) != bnum * BSIZE, "\033[31m In read_block, lseek(fd, bnum * BSIZE, 0) != sec * BSIZE\033[0m\n");
	panic_on(read(fd, buf, BSIZE) != BSIZE, "\033[31m In read_block, read(fd, buf, BSIZE) != BSIZE\033[0m\n");
}

void write_inode(uint inum, dinode_t *ip)
{
  char buf[BSIZE];
  uint bn;
  dinode_t *dip;

  bn = IBLOCK(inum);
  read_block(bn, buf);
  dip = ((dinode_t *)buf) + (inum % IPB);
  *dip = *ip;
  write_block(bn, buf);
}
void read_inode(uint inum, dinode_t *ip)
{
  char buf[BSIZE];
  uint bn;
  dinode_t *dip;

  bn = IBLOCK(inum);
  read_block(bn, buf);
  dip = ((dinode_t *)buf) + (inum % IPB);
  *ip = *dip;
}

uint ialloc(ushort type)
{
  uint inum = freeinode++;
  dinode_t inode;

  bzero(&inode, sizeof(inode));
  inode.type = xshort(type);
  inode.nlink = xshort(1);
  inode.size = xshort(0);
  write_inode(inum, &inode);
  return inum;
}

#define min(a, b) ((a) < (b) ? (a) : (b))

void inode_append(uint inum, void *xp, int n)
{
  char *p = (char*)xp;
  uint fbn, off, n1;
  dinode_t din;
  char buf[BSIZE];
  uint indirect[NINDIRECT];
  uint x;

  read_inode(inum, &din);
  off = xint(din.size);
  // printf("append inum %d at off %d sz %d\n", inum, off, n);
  while(n > 0)
  {
    fbn = off / BSIZE;
		panic_on(fbn < MAXFILE, "\033[31m fbn < MAXFILE \033[0m\n");
    if(fbn < NDIRECT)
    {
      if(xint(din.addrs[fbn]) == 0)
      {
        din.addrs[fbn] = xint(freeblock++);
      }
      x = xint(din.addrs[fbn]);
    } 
    else 
    {
      if(xint(din.addrs[NDIRECT]) == 0)
      {
        din.addrs[NDIRECT] = xint(freeblock++);
      }
      read_block(xint(din.addrs[NDIRECT]), (char*)indirect);
      if(indirect[fbn - NDIRECT] == 0)
      {
        indirect[fbn - NDIRECT] = xint(freeblock++);
        write_block(xint(din.addrs[NDIRECT]), (char*)indirect);
      }
      x = xint(indirect[fbn - NDIRECT]);
    }
    n1 = min(n, (fbn + 1) * BSIZE - off);
    read_block(x, buf);
    bcopy(p, buf + off - (fbn * BSIZE), n1);
    write_block(x, buf);
    n -= n1;
    off += n1;
    p += n1;
  }
  din.size = xint(off);
  write_inode(inum, &din);
}
void bm_alloc(int used)
{
  uchar buf[BSIZE];
  int i;

  //printf("balloc: first %d blocks have been allocated\n", used);
  panic_on(used < BSIZE * 8, "\033[31m used < BSIZE * 8 \033[0m\n");
  bzero(buf, BSIZE);
  for(i = 0; i < used; i++)
  {
    buf[i / 8] = buf[i / 8] | (0x1 << (i % 8));
  }
  //printf("balloc: write bitmap block at sector %d\n", sb.bmapstart);
  write_block(BMSTART, buf);
}

void fill_dirent(int type, uint inum, char *name)
{
	dirent_t de;
	if(inum == 0)inum = inode_alloc(T_DIR);
  bzero(&dirent, sizeof(dirent));
  de.inode = inum;
  strcpy(de.name, name);
  inode_append(inum, &de, sizeof(de));
}

void traverse_dir(char *path, char *pathname, uint curinum, uint previnum)
{
	DIR *dir = NULL;
	struct dirent *de = NULL;
	char nextpath[4096] = "\0";
	char nextpathname[4096] = "\0";	
	panic_on(!(dir = opendir(path)), "\033[31m !(dir = opendir(path))\033[0m\n");
	while(!(de = readdir(dir)))
	{
		if(strncmp(de->d_name, ".", 1) == 0 || strncmp(de->d_name, "..", 2) == 0)
		{
			fill_dirent(T_DIR, 0, ".");			
			fill_dirent(T_DIR, previnum, "..");			
		}
		else
		{
			if(de->d_type == DT_REG)
			{
				fill_dirent(T_FILE, 0, de->d_name);

			}
			else if(de->d_type == DT_DIR)
			{
				fill_dirent(T_DIR, 0, de->d_name);

			}
		}
	}
}
int main(int argc, char *argv[]) 
{
	/*===========data structure check=========*/
	panic_on(BSIZE % sizeof(dinode_t) != 0, "\033[31m BSIZE mod sizeof(dinode_t) != 0\033[0m\n");
	panic_on(BSIZE % sizeof(dirent_t) != 0, "\033[31m BSIZE mod sizeof(dirent_t) != 0\033[0m\n");
	printf("FSSTART is 0x%x\nFSSIZE is 0x%x\nINODESTART is 0x%x\nNINODEBLOCK is 0x%lx\nBMSTART is 0x%lx\nNBM is 0x%x\nDATASTART is 0x%lx\nNDATA is 0x%lx\nNMETADATA is 0x%lx\n", FSSTART, FSSIZE, INODESTART, NINODEBLOCK, BMSTART, NBM, DATASTART, NDATA, NMETADATA);

  assert((fd = open(argv[2], O_RDWR)) > 0);
  assert((ftruncate(fd, IMG_SIZE)) == 0);
  assert((disk = mmap(NULL, IMG_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) != (void *)-1);
  //initialize with zeros
	char zeros[BSIZE] = {0};
	memset(zeros, 0, sizeof(zeros));
	for(int i = FSSTART; i < FSSIZE; i++)
		write_block(i, zeros);
	
	
	char rootname[4096];
	sprintf(rootname, "%s", argv[3]);
	char pathname[4096] = "\";
	traverse_dir(rootname, pathname, ROOTINO, ROOTINO);

  munmap(disk, IMG_SIZE);
  close(fd);
	return 0;
}
