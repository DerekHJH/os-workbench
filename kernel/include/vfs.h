#ifndef VFSH
#define VFSH

#define BSISE 512

typedef struct _inode
{

}inode_t;
//unit --- blocks
#define FSSTART (1024 * 1024 / BSIZE)
#define INODESTART FSSTART
#define NINODE 111
#define BMSTART (INODESTART + NINODE) 
#define NBM 111
#define FILESTART (BMSTART + NBM)

#define NDIRECT 12
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)

#define IPB (BSIZE / sizeof(inode_t))
#define BPB (BSIZE * 8)

#define DIRSIZ 28

typedef struct _dirent
{
	uint32_t inode;
  char name[DIRSIZ];
}dirent_t;

#endif
