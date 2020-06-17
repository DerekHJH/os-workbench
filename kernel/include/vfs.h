#ifndef VFSH
#define VFSH

#define BSIZE 512

typedef struct _inode
{

}__attribute__((packed))inode_t;
#define IPB (BSIZE / sizeof(inode_t))
#define BPB (BSIZE * 8)



#define NINODES 200 //maximum number of files
//unit --- blocks --- replace superblock
#define FSSTART (1024 * 1024 / BSIZE)
#define FSSIZE (1024 * 1024 * 64 / BSIZE)
#define INODESTART FSSTART
#define NINODEBLOCK (NINODES / IBP + 1)
#define BMSTART (INODESTART + NINODEBLOCK) 
#define NBM (FSSIZE / BPB + 1) 
#define DATASTART (BMSTART + NBM)
#define NMETADATA (FSSTART + NINODEBLOCK + NBM)
#define NDATA (FSSIZE - NMETADATA)

#define NDIRECT 12
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)




#define DIRSIZ 28

typedef struct _dirent
{
	uint32_t inode;
  char name[DIRSIZ];
}dirent_t;

#endif
