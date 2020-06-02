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

int main() 
{	
  struct kvdb *db;
  const char *key = "operating-systems";
	const char *key2 = "hjh";
	const char haha[4096 * 2];
	for(int i = 0; i < 5000; i++)
		haha[i] = 'H';
	haha[5000] = '\0';
  char *value;  
	panic_on(!(db = kvdb_open("a.db")), "cannot open db");
  kvdb_put(db, key, "three-easy-pieces");
	kvdb_put(db, key2, haha);
  value = kvdb_get(db, key); 
  printf("[%s]: [%s]\n", key, value);
	value = kvdb_get(db, key2);
	printf("[%s]: [%s]\n", key2, value);
  free(value);
  kvdb_close(db);
  return 0;
}
