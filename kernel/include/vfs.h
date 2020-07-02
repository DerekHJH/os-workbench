#ifndef VFSH
#define VFSH

#define BSIZE 512

#define NDIRECT 12
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)


typedef struct _dinode
{
	short type;
	short nlink;
	short major;
	short minor;
	uint32_t size;
	uint32_t addrs[NDIRECT + 1];
}__attribute__((packed))dinode_t;
typedef struct inode 
{
  uint32_t dev;           // Device number
  uint32_t inum;          // Inode number
  int ref;            // Reference count
  sem_t sem; // protects everything below here
  int valid;          // inode has been read from disk?

  short type;         // copy of disk inode
  short major;
  short minor;
  short nlink;
  uint32_t size;
  uint32_t addrs[NDIRECT + 1];
}inode_t;
#define NINODE 50 //maximum nunmber of active inodes

#define MAXPATH 4096




#define NINODES 200 //maximum number of files
//unit --- blocks --- replace superblock
#define FSSTART 0
#define FSSIZE (1024 * 1024 * 255 / BSIZE)
#define INODESTART FSSTART

//middle
#define IPB (BSIZE / sizeof(dinode_t))
#define BPB (BSIZE * 8)
#define IBLOCK(i) ((i) / IPB + INODESTART)
//midlle

#define NINODEBLOCK (NINODES / IPB + 1)
#define BMSTART (INODESTART + NINODEBLOCK) 

#define BBLOCK(b) ((b) / BPB + BMSTART) //middle

#define NBM (FSSIZE / BPB + 1) 
#define DATASTART (BMSTART + NBM)
#define NMETADATA (FSSTART + NINODEBLOCK + NBM)
#define NDATA (FSSIZE - NMETADATA)




#define ROOTINO 1



#define DIRSIZ 28

typedef struct _dirent
{
	uint32_t inode;
  char name[DIRSIZ];
}dirent_t;


#endif
