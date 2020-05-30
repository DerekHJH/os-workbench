#include <stdio.h>
#include <assert.h>
#include <pthread.h>
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
#define KSIZE (256 - sizeof(size_t))
#define VSIZE (PGSIZE - 256)
#define BIGVSIZE (PGSIZE - sizeof(size_t))
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

}__attribute__((packed)) log_t;




struct kvdb 
{
	pthread_mutex_t lock;
	int refcnt;

};

struct kvdb *kvdb_open(const char *filename) 
{
  return NULL;
}

int kvdb_close(struct kvdb *db) 
{
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
