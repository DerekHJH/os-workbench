#include "klib.h"
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

#define Max_fmt 1024
int tag[128]=
{
/*0x00*/	0,0,0,0,
/*0x04*/  0,0,0,0,
/*0x08*/  0,0,0,0,
/*0x0c*/  0,0,0,0,
/*0x10*/	0,0,0,0,
/*0x14*/  0,0,0,0,
/*0x18*/  0,0,0,0,
/*0x1c*/  0,0,0,0,
/*0x20*/	0,0,0,0,
/*0x24*/  0,0,0,0,
/*0x28*/  0,0,0,0,
/*0x2c*/  0,0,0,0,
/*0x30*/	0,0,0,0,
/*0x34*/  0,0,0,0,
/*0x38*/  0,0,0,0,
/*0x3c*/  0,0,0,0,
/*0x40*/	0,0,0,0,
/*0x44*/  0,0,0,0,
/*0x48*/  0,0,0,0,
/*0x4c*/  0,0,0,0,
/*0x50*/	0,0,0,0,
/*0x54*/  0,0,0,0,
/*0x58*/  0,0,0,0,
/*0x5c*/  0,0,0,0,
/*0x60*/	0,0,0,1,
/*0x64*/  1,0,0,0,
/*0x68*/  0,0,0,0,
/*0x6c*/  0,0,0,0,
/*0x70*/	1,0,0,1,
/*0x74*/  0,0,0,0,
/*0x78*/  1,0,0,0,
/*0x7c*/  0,0,0,0

};
//0 stands for symbols not defined
//1 stands for specifiers
//2 stands for 
 
int printf(const char *fmt, ...) 
{
  char buffer[Max_fmt]={'\0'};
 	int len=0;
  va_list ap;
  va_start(ap,fmt);
 	len=vsprintf(buffer,fmt,ap);
 	va_end(ap);
	for(int i=0;i<len;i++)
		_putc(buffer[i]);
  return len;
}

int vsprintf(char *out, const char *fmt, va_list ap) 
{
  //begin{hjh}
	int len=0;
  while((*fmt)!='\0')
	{
    while((*fmt)!='\0'&&(*fmt)!='%')
		{
			*(out+len)=*fmt;
			fmt++;
      len++;
		}
		if(*(fmt)=='\0')break;
		int l=0;
		while(*(fmt+l)!='\0'&&((int)(*(fmt+l))>127||tag[(int)(*(fmt+l))]!=1))l++;//*(fmt+l)=the spefifier
    //////////////////////////////////////////////////////////////////
    switch (*(fmt+l))//fmt+0---fmt+l, this whole range of length l+1;
    {
    	case 'c':
    	{
				char temp;
        temp=(char)va_arg(ap,int);
        *(out+len)=temp;
        len++;
        break;				
    	}
    	case 's':
    	{
				char * temp=NULL;
        temp=va_arg(ap,char*);
    		while(*temp!='\0')
    		{
    			*(out+len)=*temp;
    			temp++;
    			len++;
    		}
        break;
    	}
			case 'd':
			case 'x':
			case 'p':
			{
				int Scale=0;
				if(*(fmt+l)=='d')Scale=10;
				else if(*(fmt+l)=='x'||*(fmt+l)=='p')Scale=16;
				char buffer[Max_fmt];
				int lbuffer=0;
				int ZE=0;
				int width=0;
				long long temp=0;
        if(*(fmt+l)=='p')temp=va_arg(ap,uint32_t);
				else if(*(fmt+l-1)=='l')
				{
					temp=va_arg(ap,long long);
					l--;
				}
				else temp=va_arg(ap,int);
				for(int i=1;i<l;i++)
				{
					if(*(fmt+i)=='0')ZE=1;
					width=width*10+(*(fmt+i))-'0';
				}
        while(temp)
				{
					long long tt=temp%Scale;
					if(tt<10)buffer[lbuffer++]=tt+'0';
					else buffer[lbuffer++]=tt-10+'a';
					temp/=Scale;
				}
        while(lbuffer<width)
				{
					if(ZE==1)buffer[lbuffer++]='0';
					else buffer[lbuffer++]=' ';
				}
				lbuffer--;
				while(lbuffer>=0)
				{
					*(out+len)=buffer[lbuffer];
					len++;
					lbuffer--;
				}
        break;
			}
    }

		/////////////////////////////////////////////////////////////////////
		fmt+=l+1;
	}
	*(out+len)='\0';
  //end{hjh}
	return len;
}

int sprintf(char *out, const char *fmt, ...) 
{
	int len=0;
  va_list ap;
  va_start(ap,fmt);
	len=vsprintf(out,fmt,ap);
	va_end(ap);
  return len;
}

int snprintf(char *out, size_t n, const char *fmt, ...) 
{
  return 0;
}

#endif
