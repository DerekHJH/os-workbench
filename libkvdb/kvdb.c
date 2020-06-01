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
	int len; //>0 --- the start of the key-value mapping;
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
#define LOGDATASIZE (PGSIZE * 2 - 1 - sizeof(size_t) * 2)
#define DATAEND (LOGDATASIZE * PGSIZE)
#define ADDREND (DATAEND + 2 * PGSIZE * sizeof(size_t))
typedef struct _log
{	
	kvent_t data[LOGDATASIZE];
	size_t addr[PGSIZE * 2];
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
	panic_on(LOGSIZE - PGSIZE != ADDREND, "\033[31mLOGSIZE - PGSIZE != ADDREND\n\033[0m");
	kvdb_t *cur = malloc(sizeof(kvdb_t));
	cur->fd = open(filename, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
	if(cur->fd <= 0)return NULL;
	return cur;
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
	read2(ADDREND, db->fd, &log->commit, PGSIZE);
	//printf("commit is %d, n is %d\n", log->commit, log->n);
	if(log->commit == 0)
	{
		//printf("no need to check log\n");
		return;
	}
	read2(0, db->fd, log, log->n * PGSIZE);
	read2(DATAEND, db->fd, log->addr, PGSIZE * 2);
	for(int i = 0; i < log->n; i++)
	{
		write2(log->addr[i], db->fd, &log->data[i], PGSIZE);
	}
	log->commit = 0;	
	write2(ADDREND, db->fd, &log->commit, PGSIZE);
	free(log);
	return;
}
kvent_t *find_key(struct kvdb *db, const char *key)
{
	lseek(db->fd, LOGSIZE, SEEK_SET);
  int Flag = 0;
  kvent_t *cur = malloc(sizeof(kvent_t));
  while(read(db->fd, cur, PGSIZE) > 0)
  {
  	if(cur->len > 0 && strcmp(cur->key, key) == 0)
  	{
  		Flag = 1;
  		break;
  	}
  }
	if(Flag == 0)return NULL;
	else return cur;
}
int charmove(char *dest, const char *src, size_t n)
{
	int l = 0;
  for(l = 0; l < n; l++)
	{
		dest[l] = src[l];
		if(dest[l] == '\0')break;
	}
  return min(n, l);
}
int kvdb_put(struct kvdb *db, const char *key, const char *value) 
{
	flock(db->fd, LOCK_EX);
	check_log(db);
	
	log_t *log = malloc(sizeof(log_t));
	log->commit = 1;
	log->n = 1;
	log->data[0].len = strlen(value);
	const char *temp = value;
	int len = 0;
	charmove(log->data[0].key, key, KSIZE);
	len += charmove(log->data[0].value, temp, VSIZE);
	while(len < log->data[0].len)
	{
		temp = (char *)((size_t)value + len);
		len += charmove(log->data[log->n].value, temp, BIGVSIZE);	
		log->data[log->n].len = 0;
		(log->n)++;
	}
	panic_on(log->data[0].len != len, "\033[31mlog->data[0].len != len\n\033[0m");
	kvent_t *cur = find_key(db, key);	

	if(cur == NULL)
	{
		size_t pos = lseek(db->fd, 0, SEEK_END);	
		panic_on(pos % PGSIZE != 0, "\033[31mpos mod PGSIZE != 0\n\033[0m");
		for(int i = 0; i < log->n; i++)
		{
			log->addr[i] = pos;
			if(i <= log->n - 2)
			{
				log->data[i].next = pos + PGSIZE;
			}
			else log->data[i].next = 0;
		}
	}
	else
	{


	}

	write2(0, db->fd, log, PGSIZE * log->n);
	write2(DATAEND, db->fd, &log->addr, PGSIZE * 2);
	write2(ADDREND, db->fd, &log->commit, PGSIZE);
	free(log);
	flock(db->fd, LOCK_UN);
  return 0;
}

char *kvdb_get(struct kvdb *db, const char *key) 
{
	flock(db->fd, LOCK_EX);
	check_log(db);		
	fsync(db->fd);
	kvent_t *cur = find_key(db, key);	
	if(cur == NULL)
	{
		printf("no such key\n");
		return NULL;
	}
	char *ret = malloc(cur->len + 1);
	char *temp = ret;
	int len = 0;
	charmove(temp, cur->value, VSIZE);
	if(cur->next == 0)len = strlen(temp);
	else len = VSIZE;	
	while(cur->next > 0)
	{
		read2(cur->next, db->fd, cur, PGSIZE);
		temp = (char *)((size_t)ret + len);
		charmove(temp, cur->value, BIGVSIZE);
		if(cur->next == 0)len += strlen(temp);
		else len += BIGVSIZE;	
	}
	panic_on(cur->len != len, "\033[31mcur->len != len\n\033[0m");
	free(cur);
	flock(db->fd, LOCK_UN);
  return ret;
}
