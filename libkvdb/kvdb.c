#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
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
	struct _kvent *next;
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
	int commit;//1 --- committed, 0 --- not committed
	int n;//number of blocks to be wrriten
	int pid;//which process or thread is using the log;
	char reserved[PGSIZE - 3 * sizeof(int)];
	kvent_t data[PGSIZE + 1];
}__attribute__((packed)) log_t;
#define LOGSIZE sizeof(log_t)



typedef struct kvdb 
{
	pthread_mutex_t lock;
	int refcnt;
	int fd;
	int index;
	char name[4096];
}kvdb_t;
#define DBSIZE 1024
int dbtot = 0;
kvdb_t *kvdbp[DBSIZE] = {0};
pthread_mutex_t openlock = PTHREAD_MUTEX_INITIALIZER;
struct kvdb *kvdb_open(const char *filename) 
{
	panic_on(sizeof(kvent_t) != PGSIZE, "\033[31msizeof(kvent_t) != PGSIZE\n\033[0m");
	pthread_mutex_lock(&openlock);
	for(int i = 0; i < dbtot; i++)                        	
	if(kvdbp[i] != NULL)
  {
  	if(strcmp(filename, kvdbp[i]->name) == 0)
		{
			pthread_mutex_unlock(&openlock);
			return kvdbp[i];
		}
  }
	int k = -1;
	for(int i = 0; i < dbtot; i++)
		if(kvdbp[i] == NULL)k = i;
	if(k == -1)
	{	
		k = dbtot;
		dbtot++;
	}
	kvdbp[k] = malloc(sizeof(kvdb_t));
	sprintf(kvdbp[k]->name, "%s", filename);
	kvdbp[k]->refcnt = 1;
	kvdbp[k]->fd = open(filename, O_CREAT | O_WRONLY);
	kvdbp[k]->index = k;
	pthread_mutex_init(&kvdbp[k]->lock, NULL);
	pthread_mutex_unlock(&openlock);
  return kvdbp[k];
}

int kvdb_close(struct kvdb *db) 
{
	pthread_mutex_lock(&openlock);
	db->refcnt--;
	if(db->refcnt <= 0)
	{
		close(db->fd);
		kvdbp[db->index] = NULL;
		free(db);
	}
	pthread_mutex_unlock(&openlock);
  return -1;
}

int kvdb_put(struct kvdb *db, const char *key, const char *value) 
{
  return -1;
}

char *kvdb_get(struct kvdb *db, const char *key) 
{
  return NULL;
}
