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
	struct kvdb *db2;
  const char *key = "operating-systems";
  char *value;
  kvdb_put(db, key, "three-easy-pieces");
  value = kvdb_get(db, key); 
  kvdb_close(db);
  printf("[%s]: [%s]\n", key, value);
  free(value);
  return 0;
}
