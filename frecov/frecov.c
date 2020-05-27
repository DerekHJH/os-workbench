//#define DEBUG
#include <stdio.h>
#include <assert.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int flag = 0;
#define panic_on(cond, s)\
	do\
	{\
		if(cond)\
		{\
			printf(s);\
			assert(0);\
		}\
	}while(0)
#define SecPerClus 8
#define BytsPerSec 512
#define BytsPerClus 4096
#define MAXCLUSTER 32768
#define BOUND 100
enum {UNLABEL = 0, DIRENT, BMPHEAD, BMPDATA, UNUSED};

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef struct _BPB
{
	u8 BS_jmpBoot[3];
	u8 OEMNName[8];
	u16 BPB_BytsPerSec;//512
	u8 BPB_SecPerClus;//8
	u16 BPB_ResvdSecCnt;//32
	u8 BPB_NumFATs;//2
	u16 BPB_RootEntCnt;//0
	u16 BPB_TotSec16;//0
	u8 BPB_Media;
	u16 BPB_FATSz16;//0
	u16 BPB_SecPerTrk;
	u16 BPB_NumHeads;
	u32 BPB_HiddSec;
	u32 BPB_TotSec32;
	u32 BPB_FATSz32;//128 sectors
	u16 BPB_ExtFlags;
	u16 BPB_FSVer;
	u32 BPB_RootClus;//cluster 2
	u16 BPB_FSInfo;//sector 1
	u16 BPB_BkBootSec;//sector 6
	u8 BPB_Reserved[12];
	u8 BS_DrvNum;
	u8 BS_Reserved;
	u8 BS_BootSig;
	u32 BS_VollD;
	u8 BS_VolLab[11];
	u8 BS_FilSysType[8];
	u8 padding[420];
	u16 Signature_word;
}__attribute__((packed)) BPB_t;

typedef union _DIR
{
	struct
	{
		u8 DIR_Name[11];
		u8 DIR_Attr;
		u8 DIR_NTRes;
		u8 DIR_CrtTimeTenth;
		u16 DIR_CrtTime;
		u16 DIR_CrtDate;
		u16 DIR_LstAccDate;
		u16 DIR_FstClusHI;
		u16 DIR_WrtTime;
		u16 DIR_WrtDate;
		u16 DIR_FstClusLO;
		u32 DIR_FIleSize;
	};
	struct
	{
		u8 LDIR_Ord;
		u8 LDIR_Name1[10];
		u8 LDIR_Attr;
		u8 LDIR_Type;
		u8 LDIR_Chksum;
		u8 LDIR_Name2[12];
		u16 LDIR_FstClusLO;
		u8 LDIR_Name3[4];
	};
}__attribute__((packed)) DIR_t;

typedef struct _BMP
{
	struct
	{
		u16 Signature;
		u32 FileSize;
		u8 Reserved[4];
		u32 DataOffset;
	}__attribute__((packed));
	struct 
	{
		u32 Size;
		u32 Width;
		u32 Height;
		u16 Planes;
		u16 BitsPerPix;
		u32 Compression;
		u32 ImageSize;
		u32 XpixelsPerM;
		u32 YpixelsPerM;
		u32 ColorsUsed;
		u32 ImportantColors;
	}__attribute__((packed));
}__attribute__((packed)) BMP_t;

typedef struct _cluster
{
	char sectors[SecPerClus][BytsPerSec];
}cluster_t; 
BPB_t BPB;
cluster_t clusters[MAXCLUSTER];
int Label[MAXCLUSTER] = {0};
int tot = 0;
int DataClus = 0;
void print_dummy()
{
	for(int i = 0; i < 40; i++)
		printf("0");
}

int ref[2][40960];
int Cheat[250] = {0};
void color_test(size_t *Left, size_t Start, size_t End, BMP_t *BMPhead, int *Choice)
{
	//printf("Choice is %d\n", *Choice);
	size_t w = BMPhead->Width * 3;
	size_t Zero = w % 4 == 0 ? 0 : 4 - w % 4;
	w = w + Zero; 
	size_t Remain = (End - Start + (*Left)) % w;
	*Left = Remain;
	size_t temp = End - Remain - w;
	for(int i = 0; i < w; i++, temp++)
	{
		ref[0][i] = (int)(*((unsigned char *)temp));
		panic_on(ref[0][i] < 0, "\033[31mref[0][i] < 0!!\n\033[0m");
		//printf("%x ", ref[0][i]);
	}
	//printf("\n\n");
	for(int i = 0; i < Remain; i++, temp++)
	{
		ref[1][i] = (int)(*((unsigned char *)temp)); 
		panic_on(ref[1][i] < 0, "\033[31mref[1][i] < 0!!\n\033[0m");
		//printf("%x ", ref[1][i]);
	}
	//printf("\n\n\n\n");
	w = w - Zero;
	size_t p = 0;
	int cnt = 0;
	int t;
	int choice = (*Choice) + 1;
	p = (size_t)&clusters[choice].sectors[0][0];                                      	
  for(int i = 0; i < w; i++)
  {
  	t = (int)(*((unsigned char *)(p + i)));
  	panic_on(t < 0, "\033[31mt < 0!!\n\033[0m");
  	if(i + Remain < w)
  	{
  		cnt = cnt + (abs(t - ref[0][i + Remain]) < BOUND ? 0 : 1);
  	}
  	else
  	{
  		cnt = cnt + (abs(t - ref[1][i - (w - Remain)]) < BOUND ? 0 : 1); 
  	}
  	panic_on(cnt < 0, "\033[31mcnt < 0!!\n\033[0m");
	}
	if(2 * cnt < w)
	{
		*Choice = choice;
	  return;
	}
		for(int k = 1; k < tot; k++)
		if(Label[k] != DIRENT && Label[k] != BMPHEAD)
		{
			cnt = 0;
			p = (size_t)&clusters[k].sectors[0][0];
			for(int i = 0; i < w; i++)
			{
				t = (int)(*((unsigned char *)(p + i)));
				panic_on(t < 0, "\033[31mt < 0!!\n\033[0m");
				if(i + Remain < w)
				{
					cnt = cnt + (abs(t - ref[0][i + Remain]) < BOUND ? 0 : 1);
				}
				else
				{
					cnt = cnt + (abs(t - ref[1][i - (w - Remain)]) < BOUND ? 0 : 1); 
				}
				panic_on(cnt < 0, "\033[31mcnt < 0!!\n\033[0m");
			}	
			if(15 * cnt < w)
			{
				choice = k;
				break;
				//printf("choice is %d, cnt is %d, w is %zd, Width is %d\n", choice, cnt, w, BMPhead->Width);
			}
		}
	//if(clusters[choice].sectors[0][0] == 0 && clusters[choice].sectors[0][1] == 0)printf("choice is %d, cnt is %d\n", choice, cnt);
	*Choice = choice;
}
void print_checknum2(DIR_t *obj)
{
	char buf[55];
	long long clusternum = obj->DIR_FstClusHI;
	clusternum = ((clusternum << 16) | obj->DIR_FstClusLO);
	clusternum -= 2;
	if(clusternum < 0 || clusternum >= MAXCLUSTER)
	{
		print_dummy();
		return;
	}
	BMP_t *BMPhead = (BMP_t *)&clusters[clusternum].sectors[0][0];
	/*printf("FileSize is %d, Offset is %d, FileSize - offset is %d, size is %d\n", BMPhead->FileSize, BMPhead->DataOffset, BMPhead->FileSize - BMPhead->DataOffset, BMPhead->Width * BMPhead->Height * 3);*/
	Label[clusternum] = BMPHEAD;
	if(BMPhead->Signature != 0x4d42)
	{
		print_dummy();
		return;
	}	
	long long Filesize = BMPhead->FileSize;
	if((size_t)BMPhead + (size_t)Filesize > (size_t)&clusters[MAXCLUSTER - 1].sectors[7][511])
	{
		print_dummy();
		return;
	}


	//color_test
	char filename[30] = "/tmp/XXXXXX";
	//int fd = open("/tmp/haha", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
	int fd = mkstemp(filename);
	panic_on(fd < 0, "\033[31mfile haha open failed in checknum!!\033[0m\n");
	write(fd, BMPhead, Filesize < BytsPerClus ? Filesize : BytsPerClus);
	Filesize -= BytsPerClus;
	size_t Start = (size_t)BMPhead + (size_t)BMPhead->DataOffset, End = (size_t)BMPhead + BytsPerClus;

	int Choice = clusternum;
	size_t Left = 0;
	while(Filesize > 0)
	{
		color_test(&Left, Start, End, BMPhead, &Choice); 
		if(Choice < 0)
		{
			print_dummy();
			return;
		}	
		Start = (size_t)&clusters[Choice].sectors[0][0];
		End = Start + BytsPerClus;
		write(fd, (void *)&clusters[Choice].sectors[0][0], Filesize < BytsPerClus ? Filesize : BytsPerClus);
		Filesize -=	BytsPerClus;
	}
	close(fd);
	char command[30];
	sprintf(command, "sha1sum %s", filename);	
	FILE *fp = popen(command, "r");
	panic_on(!fp, "\033[31mpopen failed!!\033[0m\n");
	fscanf(fp, "%s", buf);
	printf("%s", buf);
	pclose(fp);
}

void print_checknum(DIR_t *obj)
{
	char buf[55];
	long long clusternum = obj->DIR_FstClusHI;
	clusternum = ((clusternum << 16) | obj->DIR_FstClusLO);
	clusternum -= 2;
	if(clusternum < 0 || clusternum >= MAXCLUSTER)
	{
		print_dummy();
		return;
	}
	BMP_t *BMPhead = (BMP_t *)&clusters[clusternum].sectors[0][0];
	Label[clusternum] = BMPHEAD;
	if(BMPhead->Signature != 0x4d42)
	{
		print_dummy();
		return;
	}	
	long long Filesize = BMPhead->FileSize;
	if((size_t)BMPhead + (size_t)Filesize > (size_t)&clusters[MAXCLUSTER - 1].sectors[7][511])
	{
		print_dummy();
		return;
	}
	char filename[30] = "/tmp/XXXXXX";
  int fd = mkstemp(filename);
  panic_on(fd < 0, "\033[31mfile haha open failed in checknum!!\033[0m\n");
  write(fd, BMPhead, Filesize);
  close(fd);
  char command[30];
  sprintf(command, "sha1sum %s", filename);	
  FILE *fp = popen(command, "r");
  panic_on(!fp, "\033[31mpopen failed!!\033[0m\n");
  fscanf(fp, "%s", buf);
  printf("%s", buf);
  pclose(fp);
}

void print_name(DIR_t *obj)
{
	int Flag = 0;
	DIR_t *prev = obj - 1;
	panic_on((size_t)obj - (size_t)prev != sizeof(DIR_t), "\033[31mprev - obj != sizeof(DIR_t)\033[0m\n");
	if(prev->LDIR_Attr != 0x0f)
	{
		print_dummy();		
	}
	else
	{
		do
		{
			for(int i = 0; i < 10; i += 2)
				if(!Flag && prev->LDIR_Name1[i] != '\0')printf("%c", prev->LDIR_Name1[i]);
				else 
				{
					Flag = 1;
					break;
				}
			for(int i = 0; i < 12; i += 2)
      	if(!Flag && prev->LDIR_Name2[i] != '\0')printf("%c", prev->LDIR_Name2[i]);
      	else 
      	{
      		Flag = 1;
      		break;
      	}
			for(int i = 0; i < 4; i += 2)
      	if(!Flag && prev->LDIR_Name3[i] != '\0')printf("%c", prev->LDIR_Name3[i]);
      	else 
      	{
      		Flag = 1;
      		break;
      	}
			prev = prev - 1;
		}while(prev->LDIR_Attr == 0x0f);
	}
}

int main(int argc, char *argv[])
{
	/*===================read in the BPB==================*/
	panic_on(sizeof(BPB_t) != 512, "\033[31mThe fat_header needs 512 bytes!!\033[0m\n");
	int fd = open(argv[1], O_RDONLY, 0);
	panic_on(fd < 0, "\033[31mimage file open failed!!\033[0m\n");
	read(fd, &BPB, sizeof(BPB_t));
	panic_on(BPB.Signature_word != 0xaa55, "\033[31msignature is not 0xaa55\033[0m\n");
	panic_on(BPB.BPB_SecPerClus != SecPerClus || BPB.BPB_BytsPerSec != BytsPerSec , "\033[31mdefault setting is violated!!\033[0m\n");
	
	
/*==============read in the whole file =================*/
	DataClus = (BPB.BPB_ResvdSecCnt + BPB.BPB_NumFATs * BPB.BPB_FATSz32) / SecPerClus;
	lseek(fd, (BPB.BPB_ResvdSecCnt + BPB.BPB_NumFATs * BPB.BPB_FATSz32) * BytsPerSec, SEEK_SET);
	int readbytes = 0;
	DIR_t *temp;
	for(tot = 0, readbytes = read(fd, &clusters[tot], BytsPerClus); readbytes > 0; tot++, readbytes = read(fd, &clusters[tot], BytsPerClus))
	{	
	}
	close(fd);

	/*============label the directory entry=================*/
	int cntbmp = 0;
	for(int k = 0; k < tot; k++)
	{
		cntbmp = 0;
		Label[k] = UNLABEL;
    for(int i = 0; i < SecPerClus; i++)
    {
    	for(int j = 0; j < BytsPerSec; j += sizeof(DIR_t))
    	{
    		temp = (DIR_t *)(&clusters[k].sectors[i][j]);
    		if(temp->DIR_Name[8] == 'B' && temp->DIR_Name[9] == 'M' && temp->DIR_Name[10] == 'P')cntbmp++;
    	}
    }
		if(cntbmp >= 1)
		{
			Label[k] = DIRENT;
		}
	}
	/*===============label the BMP header==========*/
	for(int k = 0; k < tot; k++)
		if(Label[k] == UNLABEL && clusters[k].sectors[0][0] == 'B' && clusters[k].sectors[0][1] == 'M')
		{
			Label[k] = BMPHEAD; 
		}

	/*=================search for files=================*/
	for(int k = 0; k < tot; k++)
		if(Label[k] == DIRENT)
		{
			for(int i = 0; i < SecPerClus; i++)                                                               	
      {
      	for(int j = 0; j < BytsPerSec; j += sizeof(DIR_t))
      	{
      		temp = (DIR_t *)(&clusters[k].sectors[i][j]);
      		if(temp->DIR_Name[8] == 'B' && temp->DIR_Name[9] == 'M' && temp->DIR_Name[10] == 'P')
					{
						print_checknum(temp);
						printf(" ");
						print_name(temp);
						printf("\n");
						print_checknum2(temp);
            printf(" ");
            print_name(temp);
            printf("\n");
					}
      	}
      }
		}
	return 0;
}	
