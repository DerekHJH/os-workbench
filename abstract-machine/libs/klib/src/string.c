#include "klib.h"

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) 
{
	int p=0;
	for(;s[p]!='\0';p++){}
  return p;
}

char *strcpy(char* dst,const char* src) 
{
	int p=0;
	for(;src[p]!='\0';p++)
	{
    dst[p]=src[p];
	}
	dst[p]='\0';
  return dst;
}

char* strncpy(char* dst, const char* src, size_t n) 
{
	int p=0;
	for(;p<n&&src[p]!='\0';p++)
	{
		dst[p]=src[p];
	}
	for(;p<n;p++)
	{
		dst[p]='\0';
	}
  return dst;
}

char* strcat(char* dst, const char* src) 
{
  int p=0,l=0;
  for(;dst[p]!='\0';p++){}
	l=p;
  for(;src[p-l]!='\0';p++)
	{
	  dst[p]=src[p-l];
	}
  dst[p]=src[p-l];	
  return dst;
}

int strcmp(const char* s1, const char* s2) 
{
	int p=0;
	for(;s1[p]!='\0'&&s2[p]!='\0';p++)
	{
		if((int)s1[p]>(int)s2[p])return 1;
		else if((int)s1[p]<(int)s2[p])return -1;
	}
	if(s1[p]=='\0'&&s2[p]=='\0')return 0;
	else if(s1[p]!='\0'&&s2[p]=='\0')return 1;
	else if(s1[p]=='\0'&&s2[p]!='\0')return -1;

	return 0;
}

int strncmp(const char* s1, const char* s2, size_t n) 
{
	int p=0;
	for(;p<n&&s1[p]!='\0'&&s2[p]!='\0';p++)
	{
		if((int)s1[p]>(int)s2[p])return 1;
		else if((int)s1[p]<(int)s2[p])return -1;
	}
	if(p==n)return 0;

	if(s1[p]=='\0'&&s2[p]=='\0')return 0;
	else if(s1[p]!='\0'&&s2[p]=='\0')return 1;
	else if(s1[p]=='\0'&&s2[p]!='\0')return -1;

  return 0;
}

void* memset(void* v,int c,size_t n) 
{
	unsigned char *t1=(unsigned char *)v;
	int p=0;
	for(;p<n;p++)
	{
		t1[p]=c;
	}
  return v;
}

void* memcpy(void* out, const void* in, size_t n) 
{
	unsigned char *t1=(unsigned char *)out;
	unsigned char *t2=(unsigned char *)in;
	int p=0;
	for(;p<n;p++)
	{
		t1[p]=t2[p];
	}
  return out;
}

int memcmp(const void* s1, const void* s2, size_t n)
{
	unsigned char *t1=(unsigned char *)s1;
	unsigned char *t2=(unsigned char *)s2;
	int p=0;
	for(;p<n;p++)
	{
    if(t1[p]>t2[p])return 1;
		else if(t1[p]<t2[p])return -1; 
	}
	if(p==n)return 0;
  return 0;
}

#endif
