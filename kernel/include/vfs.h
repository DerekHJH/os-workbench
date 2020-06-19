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
	unsigned int size;
	unsigned int addrs[NDIRECT + 1];
}__attribute__((packed))dinode_t;





#define NINODES 200 //maximum number of files
//unit --- blocks --- replace superblock
#define FSSTART (1024 * 1024 / BSIZE)
#define FSSIZE (1024 * 1024 * 64 / BSIZE)
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




#define T_DIR 1
#define T_FILE 2
#define T_DEV 3

#define ROOTINO 1



#define DIRSIZ 28

typedef struct _dirent
{
	uint32_t inode;
  char name[DIRSIZ];
}dirent_t;

#endif
