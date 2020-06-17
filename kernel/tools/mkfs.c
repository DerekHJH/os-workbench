#include <user.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <../include/vfs.h>

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

void write_block(int num, void *buf)
{
  panic_on(lseek(fsfd, sec * BSIZE, 0) != sec * BSIZE, "\033[31m In write_block, lseek(fsfd, sec * BSIZE, 0) != sec * BSIZE\033[0m\n");
	panic_on(write(fsfd, buf, BSIZE) != BSIZE, "\033[31m In write_block, write(fsfd, buf, BSIZE) != BSIZE\033[0m\n");
}













int main(int argc, char *argv[]) 
{
  int fd;
  uint8_t *disk;
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
