#ifndef STATH
#define STATH
#define T_DIR  1   // Directory
#define T_FILE 2   // File

typedef struct stat 
{
  short type;  // Type of file
  device_t *dev;     // File system's disk device
  uint32_t ino;    // Inode number
  short nlink; // Number of links to file
  uint32_t size;   // Size of file in bytes
}stat_t;
#endif
