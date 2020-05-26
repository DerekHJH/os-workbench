#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/wait.h>
#include <dlfcn.h>
#define Panic(cond, s)\
		if(!(cond))\
		{\
			printf(s);\
			continue;\
		}\

#define Panic_child(cond, s)\
	do\
	{\
		if(!(cond))\
		{\
			printf(s);\
			exit(EXIT_FAILURE);\
		}\
	}while(0)
int main(int argc, char *argv[]) 
{
  static char line[4096];
	static char prefixfold[] = "/tmp/";
	static char suffixc[] = ".c";
	static char suffixos[] = ".os";
	static char funcname[4096];
	static char filename[4096];
	static char binname[4096];
  int wstatus = 0;
	char *pos = NULL;
	int fd = 0;
	int pid = 0;

	static char prefixwr[4096] = "int wrapper(){return ";
	static char suffixwr[4096] = ";}";
	static char funcdef[4096];

	int (*wrapper)() = NULL;
	void *handle = NULL;
	char *error;
  while (1) 
	{
    printf("crepl> ");
    fflush(stdout);
    if (!fgets(line, sizeof(line), stdin)) 
		{
      break;
    }
		if(line[strlen(line) - 1] == '\n')line[strlen(line) - 1] = '\0';
		if(line[0] == 'i' && line[1] == 'n' && line[2] == 't')
		{
			pos = strchr(line, '(');
			Panic(pos != NULL, "\033[31mUse your brain!!\033[0m\n")

			strncpy(funcname, line + 4, pos - line - 4);
			sprintf(filename, "%s%s%s", prefixfold, funcname, suffixc);
			fd = open(filename, O_CREAT | O_RDWR | O_TRUNC, 0600);
			Panic(fd > 0, "\033[31mfile open failed !!\033[0m\n")
			write(fd, line, strlen(line));
			close(fd);
			
			sprintf(binname, "%s%s%s", prefixfold, funcname, suffixos);
			pid = fork();
			Panic(pid != -1, "\033[31mfork failed!!!\033[0m\n")
			if(pid == 0)
			{	
#if __x86_64__
				execlp("gcc", "gcc", "-w", "-fPIC", "-shared", "-m64", filename, "-o", binname, NULL);
#else
				execlp("gcc", "gcc", "-w", "-fPIC", "-shared", "-m32", filename, "-o", binname, NULL);
#endif
				Panic_child(0, "\033[31mShould not reach here after execlp!!\033[0m\n");
			}
			else
			{
				wait(&wstatus);	
				Panic(wstatus == 0, "\033[31mUse your brain!!\033[0m\n")
				handle = dlopen(binname, RTLD_NOW | RTLD_GLOBAL | RTLD_DEEPBIND);					
				Panic(handle != NULL, "\033[31m dlopen failed !!\033[0m\n")
				printf("\033[32mNow you are using your brain!!\033[0m\n");
			}
		}
		else
		{
      sprintf(funcdef, "%s%s%s", prefixwr, line, suffixwr);
      sprintf(filename, "%sXXXXXX%s", prefixfold, suffixc);
			fd = mkstemps(filename, 2);
      Panic(fd > 0, "\033[31mfile open failed !!\033[0m\n")
      write(fd, funcdef, strlen(funcdef));
      close(fd);
			 
      sprintf(binname, "%s", filename);
			binname[strlen(binname) - 2] = '\0';
			strcat(binname, ".os");

			pid = fork();
  		Panic(pid != -1, "\033[31mfork failed!!!\033[0m\n")
  		if(pid == 0)
  		{	
#if __x86_64__
  			execlp("gcc", "gcc", "-w", "-fPIC", "-shared", "-m64", filename, "-o", binname, NULL);
#else
  			execlp("gcc", "gcc", "-w", "-fPIC", "-shared", "-m32", filename, "-o", binname, NULL);
#endif
  			Panic_child(0, "\033[31mShould not reach here after execlp!!\033[0m\n");
  		}
  		else
  		{
  			wait(&wstatus);
				Panic(wstatus == 0, "\033[31mchild failed!!\033[0m\n")
				handle = dlopen(binname, RTLD_NOW | RTLD_GLOBAL | RTLD_DEEPBIND);	
  			Panic(handle != NULL, "\033[31mdlopen failed !!\033[0m\n")
				dlerror();
				wrapper = (int (*)(int))dlsym(handle, "wrapper");
				error = dlerror();
				Panic(error == NULL, "\033[31mdlsym failed !!\033[0m\n")
				pid = fork();
				if(pid == 0)
				{
					printf("%d\n", (*wrapper)());
				}
				else 
				{
					wait(&wstatus);
					Panic(wstatus == 0, "\033[31mUse your brain!!\033[0m\n")
				}
			}

		}
		
  }
}
