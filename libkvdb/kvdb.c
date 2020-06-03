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
#define KSIZE 128
#define KNUM 256
#define PGSIZE 4096
typedef struct _keytable
{
	char key[KNUM][KSIZE];
	long long start[KNUM];//start address in bytes
	long long len[KNUM - 1];//number of blocks
	long long keytot;	
}__attribute__((packed)) keytable_t;
keytable_t keytable;
#define KEYTABLESIZE sizeof(keytable_t)
#define LOGDATASIZE (PGSIZE * 2 - 1 - sizeof(long long) * 2)
#define DATAEND (LOGDATASIZE * PGSIZE)
#define ADDREND (DATAEND + 2 * PGSIZE * sizeof(long long))
typedef struct _log
{	
	char data[LOGDATASIZE][PGSIZE];
	long long addr[PGSIZE * 2];
	int commit;//1 --- committed, 0 --- not committed
  int n;//number of blocks to be wrriten
  char reserved[PGSIZE - 2 * sizeof(int)];
}__attribute__((packed)) log_t;
log_t Log;
#define LOGSIZE sizeof(log_t)

typedef struct kvdb 
{
	int fd;
}kvdb_t;

void read2(off_t offset, int fd, void *buf, size_t count)
{
	lseek(fd, offset, SEEK_SET);
	read(fd, buf, count);
}
void write2(off_t offset, int fd, void *buf, size_t count)
{
	lseek(fd, offset, SEEK_SET);
	write(fd, buf, count);
}
struct kvdb *kvdb_open(const char *filename) 
{
	panic_on(sizeof(log_t) != 2 * PGSIZE * PGSIZE, "\033[31msizeof(log_t) != PGSIZE * PGSIZE\n\033[0m");
	panic_on(LOGSIZE - PGSIZE != ADDREND, "\033[31mLOGSIZE - PGSIZE != ADDREND\n\033[0m");
	panic_on(KEYTABLESIZE % PGSIZE != 0, "\033[31mKEYTABLESIZE mod PGSIZE != 0\n\033[0m");
	kvdb_t *cur = malloc(sizeof(kvdb_t));
	cur->fd = open(filename, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
	int a = 0; 
	write2(LOGSIZE - 4, cur->fd, &a, 4);
	fsync(cur->fd);
	if(cur->fd <= 0)return NULL;
	return cur;
}

int kvdb_close(struct kvdb *db) 
{
	close(db->fd);
	free(db);
  return 0;
}

void check_log(struct kvdb *db)
{
	read2(ADDREND, db->fd, &Log.commit, PGSIZE);
	//printf("commit is %d, n is %d\n", log->commit, log->n);
	if(Log.commit == 0)
	{
		//printf("no need to check log\n");
		return;
	}
	read2(0, db->fd, &Log, Log.n * PGSIZE);
	read2(DATAEND, db->fd, Log.addr, ADDREND - DATAEND);
	for(int i = 0; i < Log.n; i++)
	{
		write2(Log.addr[i], db->fd, &Log.data[i], PGSIZE);
	}
	fsync(db->fd);
	Log.commit = 0;	
	write2(ADDREND, db->fd, &Log.commit, sizeof(int));
	fsync(db->fd);
	return;
}
int find_key(struct kvdb *db, const char *key)
{
	read2(LOGSIZE, db->fd, &keytable, KEYTABLESIZE);
	int i = 0;
	for(i = 0; i < keytable.keytot; i++)
		if(strcmp(keytable.key[i], key) == 0)break;
	if(i >= keytable.keytot)return -1;
	else return i;
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
	
	Log.commit = 1;
	int len = strlen(value);
	Log.n = (len + PGSIZE - 1) / PGSIZE;
	int Check = charmove(&Log.data[0][0], value, len + 1);
	panic_on(Check != len, "\033[31mCheck != len\033[0m\n");
	
	int k = find_key(db, key);	

	/*
	if(k == -1)
	{
		size_t pos = lseek(db->fd, 0, SEEK_END);	
		panic_on(pos % PGSIZE != 0 || pos < LOGSIZE, "\033[31mpos mod PGSIZE != 0 || pos < LOGSIZE\n\033[0m");
		for(int i = 0; i < log->n; i++, pos += PGSIZE)
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
		int i = 0;
		log->addr[0] = lseek(db->fd, 0, SEEK_CUR) - PGSIZE;
		panic_on(log->addr[0] % PGSIZE != 0, "\033[31mlog->addr[0] mod PGSIZE != 0\n\033[0m");
		while(cur->next > 0 && i < log->n)
		{
			log->data[i].next = cur->next;
			i++;
			log->addr[i] = cur->next;
			read2(cur->next, db->fd, cur, PGSIZE);
		}
		if(cur->next == 0 && i == log->n - 1)log->data[i].next = 0;
		else if(cur->next == 0)
		{
			size_t pos = lseek(db->fd, 0, SEEK_END);	
      panic_on(pos % PGSIZE != 0 || pos < LOGSIZE, "\033[31mpos mod PGSIZE != 0 || pos < LOGSIZE\n\033[0m");
			log->data[i].next = pos;
      for(i++; i < log->n; i++, pos += PGSIZE)
      {
      	log->addr[i] = pos;
      	if(i <= log->n - 2)
      	{
      		log->data[i].next = pos + PGSIZE;
      	}
      	else log->data[i].next = 0;
      }
		}
	}

	write2(0, db->fd, log, PGSIZE * log->n);
	write2(DATAEND, db->fd, &log->addr, ADDREND - DATAEND);
	fsync(db->fd);
	write2(ADDREND, db->fd, &log->commit, PGSIZE);
	fsync(db->fd);
	*/
	flock(db->fd, LOCK_UN);
  return 0;
}

char *kvdb_get(struct kvdb *db, const char *key) 
{
	flock(db->fd, LOCK_EX);
	check_log(db);		
	int k = find_key(db, key);	
	if(k == -1)
	{
		flock(db->fd, LOCK_UN);
		return NULL;
	}
	char *ret = malloc(keytable.len[k] * PGSIZE);
	read2(keytable.start[k], db->fd, ret, keytable.len[k] * PGSIZE);
	flock(db->fd, LOCK_UN);
  return ret;
}
