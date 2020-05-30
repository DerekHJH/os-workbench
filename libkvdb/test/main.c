#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <kvdb.h>

#define panic_on(cond, s)\
	do\
	{\
		if(cond)\
		{\
			printf(s);\
			assert(0);\
		}\
	}while(0)

void print_kvdb(kvdb_t *db)
{
	printf("fd is %d\n", db->fd);
	printf("name is %s\n", db->name);
	printf("refcnt is %d\n", db->refcnt);
	printf("index is %d\n", db->index);
}
int main() 
{
	
  struct kvdb *db;
	struct kvdb *db2;
  const char *key = "operating-systems";
  char *value;

  panic_on(!(db = kvdb_open("a.db")), "cannot open db");
	panic_on(!(db2 = kvdb_open("a.db")), "cannot open db2");
	panic_on(db != db2, "db != db2");
	print_kvdb(db);
	print_kvdb(db2);
	kvdb_close(db);
	kvdb_close(db2);
	panic_on(db != NULL, "db != db2\n");
	

/*
  kvdb_put(db, key, "three-easy-pieces");
  value = kvdb_get(db, key); 
  kvdb_close(db);
  printf("[%s]: [%s]\n", key, value);
  free(value);
	*/
  return 0;
}
