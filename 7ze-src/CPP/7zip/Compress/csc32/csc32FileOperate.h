#ifndef _FILEOPERATE_H
#define _FILEOPERATE_H
#include "csc32Common.h"

int GetPath(const char *str);
void CreateDir(char* Path);
int strWildCmp(const char *wild, const char *string);
i64 GetFileSize64(char *fileName);
int GetUncompressedFileName(char *dst,const char *src,const char *ref);
int SetCompressedFileName(char *dst,const char *src,const char *ref);
char* SymSbt(char *dst,char from,char to);

#endif
