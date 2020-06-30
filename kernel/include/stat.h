#ifndef STATH
#define STATH
#define T_DIR  1   // Directory
#define T_FILE 2   // File

#define SEEK_CUR  0
#define SEEK_SET  1
#define SEEK_END  2

#define O_RDONLY  00000000
#define O_WRONLY  00000001
#define O_RDWR    00000002
#define O_CREAT   00000100

typedef struct stat 
{
  short type;  // Type of file
  uint32_t dev;     // File system's disk device
  uint32_t ino;    // Inode number
  short nlink; // Number of links to file
  uint32_t size;   // Size of file in bytes
}stat_t;
#endif
