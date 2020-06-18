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
int freeinode = INODESTART;
int freeblock = DATASTART;
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

void write_inode(uint inum, inode_t *ip)
{
  char buf[BSIZE];
  uint bn;
  inode_t *tempip;

  bn = IBLOCK(inum);
  read_block(bn, buf);
  tempip = ((inode_t *)buf) + (inum % IPB);
  *tempip = *ip;
  write_block(bn, buf);
}
void read_inode(uint inum, inode_t *ip)
{
  char buf[BSIZE];
  uint bn;
  inode_t *tempip;

  bn = IBLOCK(inum);
  read_block(bn, buf);
  tempip = ((inode_t *)buf) + (inum % IPB);
  *ip = *tempip;
}

uint ialloc(ushort type)
{
  uint inum = freeinode++;
  inode_t inode;

  bzero(&inode, sizeof(inode));
  inode.type = type;
  inode.nlink = 1;
  inode.size = 0;
  write_inode(inum, &inode);
  return inum;
}



int main(int argc, char *argv[]) 
{
	/*===========data structure check=========*/
	panic_on(BSIZE % sizeof(inode_t) != 0, "\033[31m BSIZE mod sizeof(inode_t) != 0\033[0m\n");
	panic_on(BSIZE % sizeof(dirent_t) != 0, "\033[31m BSIZE mod sizeof(dirent_t) != 0\033[0m\n");
	printf("FSSTART is 0x%x\nFSSIZE is 0x%x\nINODESTART is 0x%x\nNINODEBLOCK is 0x%lx\nBMSTART is 0x%lx\nNBM is 0x%x\nDATASTART is 0x%lx\nNDATA is 0x%lx\n", FSSTART, FSSIZE, INODESTART, NINODEBLOCK, BMSTART, NBM, DATASTART, NDATA);


  
  
	//char *dirname = argv[3];
  assert((fd = open(argv[2], O_RDWR)) > 0);
  assert((ftruncate(fd, IMG_SIZE)) == 0);
  assert((disk = mmap(NULL, IMG_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) != (void *)-1);
  //initialize with zeros
	char zeros[BSIZE] = {0};
	memset(zeros, 0, sizeof(zeros));
	for(int i = FSSTART; i < FSSIZE; i++)
		write_block(i, zeros);
	













  munmap(disk, IMG_SIZE);
  close(fd);
	return 0;
}
