#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define Panic(cond, s) \
  do \
	{ \
    if (cond) \
		{ \
      printf(s); \
    } \
  } while (0) 
#define BOUND 0.5
struct _process
{
	char callname[64];
	double calltime;
}Syscall[1024];
int tot = 0;
double tottime = 0.0;

int cmp(const void *a, const void *b)
{
	return ((struct _process *)a)->calltime < ((struct _process *)b)->calltime;
}

void Display()
{
  qsort(Syscall + 1, tot, sizeof(struct _process), cmp);
  for(int i = 1; i <= tot; i++)
	{
		Syscall[i].calltime /= tottime;
	}
	for(int i = 1; i <= tot; i++)
	{
		printf("%s (%02d%%)\n", Syscall[i].callname, (int)(Syscall[i].calltime * 100.0));
	}

	for(int i = 1; i <= 80; i++)
		printf("%c", 0);

	printf("\n");
	fflush(stdout);
	tot = 0;
	tottime = 0.0;
}
int main(int argc, char *argv[], char *envp[]) 
{
	int pid = fork();
	Panic(pid == -1, "\033[31mfork failed!!\033[0m\n");
	if(pid == 0)
	{
		int haha = open("/dev/null", O_RDWR | O_APPEND);
		dup2(haha, 2);
		dup2(haha, 1);
		char *Path = malloc(4096);
	  Path[0]	= 'P', Path[1] = 'A', Path[2] = 'T', Path[3] = 'H', Path[4] = '=';
		char temp[4096];
		system("echo $PATH > path.txt");
		FILE *fp = fopen("path.txt", "r");
		fscanf(fp, "%s", temp);
		fclose(fp);
		strcat(Path, temp);

		char *exec_argv[argc + 4];
		exec_argv[0] = (char *)malloc(1024);
		exec_argv[1] = "-T";
		exec_argv[2] = "-o";
		exec_argv[3] = "output.txt";
		for(int i = 4; i <= argc + 2; i++)
			exec_argv[i] = argv[i - 3];
		exec_argv[argc + 3] = NULL;
    char *exec_envp[] = { Path, NULL, };	
    int pos = 5, lenpath = strlen(Path), lentemp = 0;
		
		while(pos < lenpath)
		{
			lentemp = 0;
			while(pos < lenpath && Path[pos] != ':')
			{
				temp[lentemp] = Path[pos];
				pos++;
				lentemp++;
			}
			pos++;
			temp[lentemp] = '\0';
			strcat(temp, "/strace");
			lentemp += 7;
			temp[lentemp] = '\0';
			sprintf(exec_argv[0], "%s", temp);
			execve(temp, exec_argv, envp);
		}
    //execve("/bin/strace",     exec_argv, exec_envp);
    //execve("/usr/bin/strace", exec_argv, exec_envp);
    perror(argv[0]);
    exit(EXIT_FAILURE);
	}
	else 
	{
		char *callname = malloc(1024);
		int lenname = 0;
		char *calltime = malloc(1024);
		int lentime = 0;
		int ret = 0;
		char c;
		double Time = 0.0;
		int i = 0;

		FILE *fp = NULL;
		while(fp == NULL)fp = fopen("output.txt", "w+");
		/*while(1)
		{
			c = fgetc(fp);
			if(c != EOF)printf("%c", c);
		}*/
		
		while(1)
		{
			do
			{
				c = fgetc(fp);
			}while(c == EOF || c == '\n');
			for(lenname = 0; c != '(' && lenname < 1024; c = fgetc(fp), lenname++)
			{
				if(c == EOF)lenname--;
				else callname[lenname] = c;
			}
			callname[lenname] = '\0';
			if(strcmp(callname, "exit_group") == 0)
			{
				Display();
				break;
			}

			do
			{
				c = fgetc(fp);
			}while(c == EOF || c != '<');
			
			for(c = fgetc(fp), lentime = 0; c != '>' && lentime < 1024; c = fgetc(fp), lentime++)
      {
				if(c == EOF)lentime--;
				else calltime[lentime] = c;
      }
			calltime[lentime] = '\0';
			Time = atof(calltime);
			tottime += Time;
			for(i = 1; i <= tot; i++)
				if(strcmp(Syscall[i].callname, callname) == 0)
				{
					Syscall[i].calltime += Time;
					break;
				}
			if(i > tot)
			{
				tot++;
				memcpy(Syscall[tot].callname, callname, lenname + 1);
				Syscall[tot].calltime = Time;
			}
			if(tottime > BOUND)Display();
		}
	}

	return 0;
}

