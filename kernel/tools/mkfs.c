#include <user.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include "vfs.h"
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#define uint unsigned int
#define ushort unsigned short
#define uchar unsigned char

#define FSOFFSET (1024 * 1024 / BSIZE)
#define panic_on(cond, s)\
	do\
	{\
		if(cond)\
		{\
			printf(s);\
			assert(0);\
		}\
	}while(0)

//#define IMG_SIZE (64 * 1024 * 1024)
int fd;
//uint8_t *disk;
int freeinode = 1;
int freeblock = DATASTART;

uint xint(uint x)
{
	uint y;
  uchar *a = (uchar *)&y;
  a[0] = x;
  a[1] = x >> 8;
  a[2] = x >> 16;
  a[3] = x >> 24;
  return y;
}
ushort xshort(ushort x)
{
	ushort y;
  uchar *a = (uchar *)&y;
  a[0] = x;
  a[1] = x >> 8;
  return y;
}

void write_block(uint bnum, void *buf)
{
	bnum += FSOFFSET;
	panic_on(bnum * BSIZE < 1024 * 1024, "\033[31mfuck\n\033[0m");
  panic_on(lseek(fd, bnum * BSIZE, 0) != bnum * BSIZE, "\033[31m In write_block, lseek(fd, bnum * BSIZE, 0) != sec * BSIZE\033[0m\n");
	panic_on(write(fd, buf, BSIZE) != BSIZE, "\033[31m In write_block, write(fd, buf, BSIZE) != BSIZE\033[0m\n");
}
void read_block(uint bnum, void *buf)
{
	bnum += FSOFFSET;
	panic_on(bnum * BSIZE < 1024 * 1024, "\033[31mfuck\n\033[0m");
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
	//printf("in write inode bn is %d and inum is %u and IPB is %ld and sizeoof(dinode) is %zd\n", bn, inum, IPB, sizeof(dinode_t));
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

uint inode_alloc(ushort type)
{
  uint inum = freeinode++;
  dinode_t inode;

  bzero(&inode, sizeof(inode));
  inode.type = xshort(type);
  inode.nlink = xshort(1);
  inode.size = xint(0);
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
		panic_on(fbn >= MAXFILE, "\033[31m fbn >= MAXFILE \033[0m\n");
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
  int i, cnt = 0;

  //printf("balloc: first %d blocks have been allocated\n", used);
	while(used > 0)
	{
		bzero(buf, BSIZE);
		for(i = 0; i < min(used, BSIZE); i++)
		{
			buf[i / 8] = buf[i / 8] | (0x1 << (i % 8));
		}
		//printf("balloc: write bitmap block at sector %d\n", sb.bmapstart);
		write_block(BMSTART + cnt, buf);
		used -= BSIZE;
		cnt++;
	}
}

void fill_dirent(uint curinum, uint inum, char *name)
{
	dirent_t de;
  bzero(&de, sizeof(de));
  de.inode = inum;
  strcpy(de.name, name);
  inode_append(curinum, &de, sizeof(de));
}
uint8_t *filecopy = NULL;
void traverse_dir(char *path, uint curinum, uint previnum)
{
	DIR *dir = NULL;
	struct dirent *de = NULL;
	char nextpath[MAXPATH] = "\0";
	panic_on(!(dir = opendir(path)), "\033[31m !(dir = opendir(path))\033[0m\n");

	fill_dirent(curinum, curinum, ".");			
	fill_dirent(curinum, previnum, "..");
	while((de = readdir(dir)))
	{
		if(strcmp(de->d_name, ".") == 0)
		{
			//printf(". --- %s\n", path);
		}
		else if(strcmp(de->d_name, "..") == 0)
		{
			//printf(".. --- hahhahahah\n");
		}
		else
		{
			if(de->d_type == DT_REG)
			{
				uint tempinum = inode_alloc(T_FILE);
				fill_dirent(curinum, tempinum, de->d_name);
				sprintf(nextpath, "%s%s", path, de->d_name);	

				//printf("%s\n", nextpath);
				int ffd = 0;
				panic_on((ffd = open(nextpath, O_RDWR)) < 0, "\033[31m (ffd = open(nextpath, O_RDWR)) < 0 \033[0m\n");
				panic_on((filecopy = mmap(NULL, lseek(ffd, 0, SEEK_END), PROT_READ | PROT_WRITE, MAP_SHARED, ffd, 0)) == (void *)-1, "\033[31m mmmap crash \033[0m\n");
				inode_append(tempinum, filecopy, lseek(ffd, 0, SEEK_END));
				munmap(filecopy, lseek(ffd, 0, SEEK_END));
				close(ffd);
			}
			else if(de->d_type == DT_DIR)
			{
				uint tempinum = inode_alloc(T_DIR);
				fill_dirent(curinum, tempinum, de->d_name);
				sprintf(nextpath, "%s%s/", path, de->d_name);
				
				//printf("%s\n", nextpath);
				traverse_dir(nextpath, tempinum, curinum);
			}
		}
	}
}
int main(int argc, char *argv[]) 
{
	/*===========data structure check=========*/
	panic_on(BSIZE % sizeof(dinode_t) != 0, "\033[31m BSIZE mod sizeof(dinode_t) != 0\033[0m\n");
	panic_on(BSIZE % sizeof(dirent_t) != 0, "\033[31m BSIZE mod sizeof(dirent_t) != 0\033[0m\n");
	//printf("FSSTART is 0x%x\nFSSIZE is 0x%x\nINODESTART is 0x%x\nNINODEBLOCK is 0x%lx\nBMSTART is 0x%lx\nNBM is 0x%x\nDATASTART is 0x%lx\nNDATA is 0x%lx\nNMETADATA is 0x%lx\n", FSSTART, FSSIZE, INODESTART, NINODEBLOCK, BMSTART, NBM, DATASTART, NDATA, NMETADATA);

	//uint32_t imgsize = atoi(argv[1]) * 1024 * 1024;
  assert((fd = open(argv[2], O_RDWR)) > 0);
  assert((ftruncate(fd, FSSIZE + FSOFFSET)) == 0);
  //assert((disk = mmap(NULL, IMG_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) != (void *)-1);
  //initialize with zeros
	char zeros[BSIZE] = {0};
	memset(zeros, 0, sizeof(zeros));
	for(int i = FSSTART; i < FSSIZE; i++)
		write_block(i, zeros);
	
	
	char rootname[MAXPATH];
	sprintf(rootname, "%s", argv[3]);

	/*===========create proc and dev===============*/
	char temppath[MAXPATH];
	int tempfd = 0;
	sprintf(temppath, "%sproc/", argv[3]);	
  mkdir(temppath, 0777);
	sprintf(temppath, "%sdev/", argv[3]);	
	mkdir(temppath, 0777);                	
	sprintf(temppath, "%sdev/null", argv[3]);
	tempfd = open(temppath, O_CREAT | O_RDWR | O_TRUNC, 0777);	
	printf("%ld\n", write(tempfd, "dev", 3));
	close(tempfd);
	
	sprintf(temppath, "%sdev/zero", argv[3]);
  tempfd = open(temppath, O_CREAT | O_RDWR | O_TRUNC, 0777);	
  printf("%ld\n", write(tempfd, "dev", 3));
  close(tempfd);

	sprintf(temppath, "%sdev/random", argv[3]);
  tempfd = open(temppath, O_CREAT | O_RDWR | O_TRUNC, 0777);	
  printf("%ld\n", write(tempfd, "dev", 3));
  close(tempfd);
	/*============================*/

	uint rootino = inode_alloc(T_DIR);
	panic_on(rootino != ROOTINO, "\033[31m rootino != ROOTINO \033[0m\n");
	traverse_dir(rootname, ROOTINO, ROOTINO);

	bm_alloc(freeblock);

  //munmap(disk, IMG_SIZE);
  close(fd);
	return 0;
}
