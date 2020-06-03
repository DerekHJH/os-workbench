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

char haha[1024 * 1024 * 15];
char xixi[4100];
int main() 
{	
  struct kvdb *db;
  const char *key = "operating-systems";
	const char *key2 = "hjh";
	for(int i = 0; i < 1024 * 1024 * 15; i++)
		haha[i] = 'A' + i % 26;
	haha[1024 * 1024 * 15] = '\0';
	for(int i = 0; i < 4096; i++)
		xixi[i] = 'A' + i % 26;
	xixi[4096] = '\0';
  char *value;  
	panic_on(!(db = kvdb_open("a.db")), "cannot open db");
  kvdb_put(db, key, "three-easy-pieces");
	//kvdb_put(db, key2, haha);
  value = kvdb_get(db, key); 
  //printf("[%s]: [%s]\n", key, value);
	value = kvdb_get(db, key2);
	//printf("[%s]: [%s]\n", key2, value);
	//kvdb_put(db, key2, "three-easy-pieces");
	//value = kvdb_get(db, key2);
	//printf("[%s]: [%s]\n", key2, value);
  free(value);
  kvdb_close(db);
  return 0;
}
