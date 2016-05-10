#include <string.h>
#include <windows.h>
#include <fcntl.h>
#include <sys\STAT.h>
#include <io.h>

#include "csc32Common.h"
#include "csc32FileOperate.h"

#define MAX_PATH 260

//// GetPath and CreateDir is directly copied from Nania's YZX 0.01
int GetPath(const char *str)
{
	unsigned int i=0;

	if (str[1]==0x3A)
		i=2;
	for(;i<strlen(str);i++)
		if (str[i]!=0x2F && str[i]!=0x5C)
			break;
	return i;
}



char* SymSbt(char *dst,char from,char to)
{
	int i=0;
	int len=strlen(dst);

	for (i=0;i<len;i++)
		if (dst[i]==from)
			dst[i]=to;

	return dst;
}


void CreateDir(char* Path)///////////////////////////////
{
	char DirName[MAX_PATH];
	char* p = Path;
	char* q = DirName;
	while(*p)
	{
		if (('\\' == *p) || ('/' == *p))
		{
			if (':' != *(p-1))
			{
				CreateDirectory(DirName, NULL);
			}
		}
		*q++ = *p++;
		*q = '\0';
	}
	CreateDirectory(DirName, NULL);
}///////////////////////////////////////////////////


int strWildCmp(const char *wild, const char *string) {
	// Written by Jack Handy - jakkhandy@hotmail.com
	const char *cp = NULL, *mp = NULL;

	while ((*string) && (*wild != '*')) 
	{
		if ((*wild != *string) && (*wild != '?')) 
		{
			return 0;
		}
		wild++;
		string++;
	}

	while (*string) 
	{
		if (*wild == '*') 
		{
			if (!*++wild) 
			{
				return 1;
			}
			mp = wild;
			cp = string+1;
		} 
		else if ((*wild == *string) || (*wild == '?') 
			|| (*wild=='\\' && *string=='/') || (*wild=='/' && *string=='\\')) 
		{
			wild++;
			string++;
		} 
		else 
		{
			wild = mp;
			string = cp++;
		}
	}

	while (*wild == '*') 
	{
		wild++;
	}
	return !*wild;
}


i64 GetFileSize64(char *fileName)
{
	i64 size;
	int fd=open(fileName,_O_RDONLY|_O_BINARY,0);
	if (fd==-1) return 0;
	_lseeki64(fd,0,SEEK_END);
	size=_telli64(fd);
	_lseeki64(fd,0,SEEK_SET);
	_close(fd);
	return size;
}

int SetCompressedFileName(char *dst,const char *src,const char *ref)
{
	int len1=strlen(src)-1;
	int len2=strlen(ref)-1;
	int suffixLen=0;
	
	
	dst[0]=0;
	while(src[dst[0]]!=0 && ref[dst[0]]!=0 && 
		src[dst[0]]==ref[dst[0]] && dst[0]<127) 
		dst[0]++;  //Prefix len

	
	strcpy(dst+1,src+dst[0]);

	while(src[len1]==ref[len2] && len1>dst[0] && len2>dst[0])
	{
		suffixLen++;
		if (src[len1]=='.')
		{
			dst[0]|=0x80;
			dst[strlen(dst+1)+1-suffixLen]=0;
			break;
		}
		len1--;
		len2--;
	}

	return strlen(dst+1)+1;
}

int GetUncompressedFileName(char *dst,const char *src,const char *ref)
{
	int prefixLen=src[0]&0x7F;
	int i,j;
	
	for(i=0;i<prefixLen;i++)
		dst[i]=ref[i];
	dst[i]=0;

	strcpy(dst+i,src+1);

	if (src[0]&0x80)
	{
		for (j=strlen(ref)-1;ref[j]!='.';j--);
		strcpy(dst+strlen(dst),ref+j);
	}

	return 0;
}
/*
u32 CommonPrefixLen(const char *a,const char *b)
{
	len=0;
	while(a[len]!=0 && b[len]!=0 && a[len]=b[len]) len++;
	if (len>255) len=255;
	return len;
}*/