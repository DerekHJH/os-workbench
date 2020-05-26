#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#define MAXPID 1000000 //max number of PIDS or relations
#define MAX 1024    //general bound
uint32_t print_opt = 0;
int Next[MAXPID] = {0}, Last[MAXPID] = {0}, End[MAXPID] = {0}, totline = 0;
void Add(int x, int y)
{
	totline++;
	End[totline] = y;
	Next[totline] = Last[x];
	Last[x] = totline;
}
typedef struct
{
	int PID;
	int PPID;
	char Name[MAX];
}Proc;
Proc proc[MAXPID];
int totproc = 0;
int Map[MAXPID] = {0};
void parse_args(int argc, char *argv[])
{
	while(1)
	{
		static struct option long_options[] = 
		{
			{"show-pids", no_argument, NULL, 'p'},
			{"numeric-sort", no_argument, NULL, 'n'},
			{"version", no_argument, NULL, 'V'},
			{0, 0, 0, 0}
		};
		int o = getopt_long(argc, argv, "pnV", long_options, NULL);
		if(o == -1)break;
		switch(o)
		{
		  case 'p':
			{
				print_opt |= 0x1;
				break;
			}	
		  case 'n':
			{
				print_opt |= 0x2;
				break;
			}	
		  case 'V':
			{
				print_opt |= 0x4;
				break;
			}
			default: 
		  {
				//printf("Usage: %s [-p] [-n] [-v] [--show-pids] [--numeric-sort] [--version]\n", argv[0]);
				//assert(0);
				break;
			}	
		}
	}
	return;
}
void find_relation()
{
	char dirname[MAX] = "//proc/";
	char subdirname[MAX] = "\0";
	
  DIR *dir = opendir(dirname);
	DIR *subdir = NULL;
	if(dir == NULL)
	{
		return;
	}

	struct dirent *ptr = NULL;
	struct dirent *subptr = NULL;

	char *pidstr = NULL;
	char *subpidstr = NULL;

	char filename[MAX] = "\0";
	char subfilename[MAX] = "\0";
	
	FILE *fp = NULL;
	FILE *subfp = NULL; 

  char aa[MAX]= "";
  char Name[MAX]= "";
  char Line = '\0';
  int ppid = 0, pid = 0;
	while((ptr = readdir(dir)) != NULL)
	{
		pidstr = ptr->d_name;
    if(isdigit(*pidstr))
		{
			totproc++;
			filename[0] = '\0';
			sprintf(filename, "%s%s/status", dirname, pidstr);
			fp = fopen(filename, "r");
			if(fp == NULL)
			{
				//printf("file %s cannot be opened\n", filename);
				continue;
			}	
			fscanf(fp, "%s %[^\n]", aa, Name);
			fscanf(fp, "%c", &Line);
			for(int i = 1; i <= 5; i++)
			{
				fscanf(fp, "%[^\n]", aa);
				fscanf(fp, "%c", &Line);
			}
			fscanf(fp, "%s %d\n", aa, &ppid);
      pid = atoi(pidstr);
      proc[totproc].PID = pid;
		  sprintf(proc[totproc].Name, "%s", Name);
			proc[totproc].PPID = ppid;
      fclose(fp);
			 
			ppid = pid;
			//Next we are going through the task directory
			sprintf(subdirname, "%s%s/task", dirname, pidstr);
			subdir = opendir(subdirname);
			if(subdir == NULL)continue;
		  while((subptr = readdir(subdir)) != NULL)
			{
				subpidstr = subptr->d_name;
				if(strcmp(subpidstr, pidstr) == 0)continue;
				if(isdigit(*subpidstr))
				{
					totproc++;
					subfilename[0] = '\0';
					sprintf(subfilename, "%s/%s/status", subdirname, subpidstr);
					subfp = fopen(subfilename, "r");
					if(subfp == NULL)
					{
						//printf("file %s cannot be opened\n", filename);
						continue;
					}
					fscanf(subfp, "%s %[^\n]", aa, Name);
					fscanf(subfp, "%c", &Line);
          pid = atoi(subpidstr);
          proc[totproc].PID = pid;
          sprintf(proc[totproc].Name, "{%s}", Name);
          proc[totproc].PPID = ppid;
					fclose(subfp);
				}
			}	
			closedir(subdir);
		}
	}
	closedir(dir);	
	return;
}

int cmp(const void *A, const void *B)
{
  Proc *a = (Proc *)A;
	Proc *b = (Proc *)B;
	return (a->PID) <= (b->PID);
}
void build_tree()
{
  if(print_opt & 0x2)qsort(proc + 1, totproc, sizeof(Proc), cmp);
	for(int i = 1; i <= totproc; i++)
	{
		Map[proc[i].PID] = i;
		Add(proc[i].PPID, proc[i].PID);
	}
	return;
}
void print_tree(int pid, int len)
{		
	for(int i = 1 ; i <= len; i++)putchar(' ');	
	printf("%s\n", proc[Map[pid]].Name);
	int t = Last[pid], newlen = len + 2;
	while(t > 0)
	{
		print_tree(End[t], newlen);
		t = Next[t];	
	}
	return;
}
void print_outcome()
{
  if(print_opt & 0x4)
  {
  	fprintf(stderr, "pstree (PSmisc) 23.1\n""hjh");
  }
  else 
	{
		if(print_opt & 0x1)
		{
			char pidstr[MAX] = "\0";
			int l = 0;
			int temp = 0;
			char Inverse[MAX] = "";
			char Digit[MAX] = "";
			for(int i = 1; i <= totproc; i++)
			{	
				l = 0;
				Inverse[0] = '\0';
				Digit[0] = '\0';
				pidstr[0] = '(';
				pidstr[1] = '\0';
				temp = proc[i].PID;
				while(temp > 0)
				{
					Inverse[l++] = temp % 10 + '0';
				  temp /= 10;	
				}
				for(int j = 0; j < l; j++)
					Digit[j] = Inverse[l - j - 1];
				Digit[l++] = ')';
				Digit[l] = '\0';
				strcat(pidstr, Digit);
				strcat(proc[i].Name, pidstr);
			}
		}
		print_tree(1, 0);
	}
}
int main(int argc, char *argv[]) 
{
  parse_args(argc, argv);
	find_relation();
  build_tree();
	print_outcome();
  return 0;
}
