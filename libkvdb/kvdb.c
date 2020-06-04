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
#define KNUM 256  //actually there are only 255 keys
#define PGSIZE 4096
#define KEYEND (KNUM * KSIZE)
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
	long long a = 0; 
	write2(LOGSIZE + KEYTABLESIZE- 8, cur->fd, &a, 8);
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
		write2(Log.addr[i], db->fd, &Log.data[i][0], PGSIZE);
	}
	fsync(db->fd);
	Log.commit = 0;	
	write2(ADDREND, db->fd, &Log.commit, PGSIZE);
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
int kvdb_put(struct kvdb *db, const char *key, const char *value) 
{
	flock(db->fd, LOCK_EX);
	check_log(db);
	
	Log.commit = 1;
	int len = strlen(value);
	Log.n = (len + PGSIZE - 1) / PGSIZE;
	memmove(&Log.data[0][0], value, len + 1);
	
	int k = find_key(db, key);	
	
	if(k > 0 && keytable.len[k] >= Log.n)
	{
		long long pos = keytable.start[k];
		for(int i = 0; i < Log.n; i++, pos += PGSIZE)
		  Log.addr[i] = pos;	
	}
	else
	{
		long long pos = lseek(db->fd, 0, SEEK_END);	
		long long start = pos;
    panic_on(pos % PGSIZE != 0 || pos < LOGSIZE + KEYTABLESIZE, "\033[31mpos mod PGSIZE != 0 || pos < LOGSIZE + KEYTABLESIZE\n\033[0m");
    for(int i = 0; i < Log.n; i++, pos += PGSIZE)
    	Log.addr[i] = pos;
		if(k > 0)
		{
			keytable.len[k] = Log.n;	
			keytable.start[k] = start;
			memmove(&Log.data[Log.n][0], &keytable.start[0], PGSIZE);	
			Log.addr[Log.n] = KEYEND + LOGSIZE; 
			Log.n++;
		}
		else
		{
			k = keytable.keytot;
			keytable.keytot++;
			keytable.len[k] = Log.n;	
      keytable.start[k] = start;
			memmove(keytable.key[k], key, strlen(key));

			memmove(&Log.data[Log.n][0], &keytable.start[0], PGSIZE);	
			Log.addr[Log.n] = KEYEND + LOGSIZE; 
			Log.n++;
			long long temp = k - k % (PGSIZE / KSIZE);
			memmove(&Log.data[Log.n][0], keytable.key[temp], PGSIZE);	
			Log.addr[Log.n] = temp * KSIZE + LOGSIZE; 
			Log.n++;
		}
	}

	write2(0, db->fd, &Log, PGSIZE * Log.n);
	write2(DATAEND, db->fd, &Log.addr[0], ADDREND - DATAEND);
	fsync(db->fd);
	write2(ADDREND, db->fd, &Log.commit, PGSIZE);
	fsync(db->fd);
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
