#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h> 
int main()
{
	int fd = open("haha", O_CREAT | O_TRUNC | O_RDWR, 777);
	
	close(fd);
	return 0;
}
