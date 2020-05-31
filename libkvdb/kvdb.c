#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <sys/file.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#define min(a, b) (a < b ? a: b)
#define panic_on(cond, s)\
	do\
	{\
		if(cond)\
		{\
			printf(s);\
			assert(0);\
		}\
	}while(0)
#define PGSIZE 4096
#define KSIZE (256 - sizeof(size_t) - sizeof(int))
#define VSIZE (PGSIZE - 256)
#define BIGVSIZE (PGSIZE - sizeof(size_t) - sizeof(int))
typedef struct _kvent
{
	size_t next;
	int order; //0 --- the start of the key-value mapping, and 1, 2, 3...
	union
	{
		struct 
		{
			char key[KSIZE];
			char value[VSIZE];	
		};
		char bigvalue[BIGVSIZE];
	};
}__attribute__((packed)) kvent_t;
typedef struct _log
{	
	kvent_t data[PGSIZE * 2 - 9];
	int addr[PGSIZE * 2];
	int commit;//1 --- committed, 0 --- not committed
  int n;//number of blocks to be wrriten
  char reserved[PGSIZE - 2 * sizeof(int)];
}__attribute__((packed)) log_t;
#define LOGSIZE sizeof(log_t)

typedef struct kvdb 
{
	int fd;
}kvdb_t;

struct kvdb *kvdb_open(const char *filename) 
{
	panic_on(sizeof(kvent_t) != PGSIZE, "\033[31msizeof(kvent_t) != PGSIZE\n\033[0m");
	panic_on(sizeof(log_t) != 2 * PGSIZE * PGSIZE, "\033[31msizeof(log_t) != PGSIZE * PGSIZE\n\033[0m");
	kvdb_t *cur = malloc(sizeof(kvdb_t));
	cur->fd = open(filename, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
	if(cur->fd <= 0)return NULL;
	return NULL;
}

int kvdb_close(struct kvdb *db) 
{
	close(db->fd);
	free(db);
  return 0;
}
void read2(off_t offset, int fd, void *buf, size_t count)
{
	lseek(fd, offset, SEEK_SET);
	read(fd, buf, count);
}
void write2(off_t offset, int fd, void *buf, size_t count)
{
	lseek(fd, offset, SEEK_SET);
	write(fd, buf, count);
	fsync(fd);
}
void check_log(struct kvdb *db)
{
	log_t *log = malloc(sizeof(log_t));
	read2(0, db->fd, log, sizeof(log_t));
	if(log->commit == 0)return;
	for(int i = 0; i < log->n; i++)
	{
		write2(log->addr[i], db->fd, &log->data[i], sizeof(PGSIZE));
	}
	log->commit = 0;	
	write2(0, db->fd, log, sizeof(log_t));
	free(log);
	return;
}
int kvdb_put(struct kvdb *db, const char *key, const char *value) 
{
	/*
	log_t *log = malloc(sizeof(log_t));
	log->commit = 0;
	log->n = 1;
	sprintf(log->data[0].key, key, strlen(key));
	sprintf(log->data[0].value, value, min(BIGVSIZE, strlen(value)));
	*/
  return 0;
}

char *kvdb_get(struct kvdb *db, const char *key) 
{
	flock(db->fd, LOCK_EX);
	check_log(db);		
	lseek(db->fd, LOGSIZE, SEEK_SET);
	int Flag = 0;
	kvent_t *cur = malloc(sizeof(kvent_t));
	while(read(db->fd, cur, PGSIZE) > 0)
	{
		if(strcmp(cur->key, key) == 0)
		{
			Flag = 1;
			break;
		}
	}
	if(Flag == 0)return NULL;
	char *ret = malloc(PGSIZE * PGSIZE + 1);
	char *temp = ret;
	int len = 0;
	sprintf(temp, cur->value, VSIZE);
	if(cur->next == 0)len = strlen(temp);
	else len = VSIZE;	
	while(cur->next > 0)
	{
		read2(cur->next * PGSIZE, db->fd, cur, PGSIZE);
		temp = (char *)((size_t)ret + len);
		sprintf(temp, cur->value, BIGVSIZE);
		if(cur->next == 0)len += strlen(temp);
		else len += BIGVSIZE;	
	}
	ret = realloc(ret, len + 1);
	flock(db->fd, LOCK_UN);
  return ret;
}
