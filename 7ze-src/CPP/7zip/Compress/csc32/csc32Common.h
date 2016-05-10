#ifndef _CSC32_COMMON_H
#define _CSC32_COMMON_H

#define CSA_VERSION 8


typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef int i32;
typedef long long i64;

const u32 KB=1024;
const u32 MB=1048576;
const u32 MinBlockSize=8*KB;


const u32 MaxChunkBits=21;
const u32 MaxChunkSize=(1<<(MaxChunkBits-1));
const u32 MaxDictSize=512*MB;//Don't change
const u32 DefaultOutStreamBlockSize=128*KB;
const u32 DefaultInBufferSize=MaxChunkSize;  //Should >=MaxChunkSize
#define DLT_CHANNEL_MAX 5
const u32 DltIndex[DLT_CHANNEL_MAX]={1,2,3,4,8};


#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define SAFEFREE(x) {if ((x)!=NULL) free(x);x=NULL;}


#define ENCODE 1
#define	DECODE 2


/*****ERRORS*****************/
#ifndef NO_ERROR
#define NO_ERROR 0L
#endif
#define CANT_OPEN_FILE (-100)
#define CANT_CREATE_FILE (-99)
#define NOT_CSC_FILE (-98)
#define VERSION_INVALID (-97)
#define CSC_FILE_INVALID (-95)
#define DECODE_ERROR (-96)
#define CANT_ALLOC_MEM (-94)
#define ALREADY_INITIALIZED (-93)
#define OPERATION_ERROR (-92)
#define FILE_DIDNT_OPEN (-91)
/*****ERRORS*****************/

/******Block Type*************/
#define DT_NONE 0
#define DT_HARD 0x05
#define DT_EXE 0x04
#define DT_BAD 0x03
#define DT_NORMAL 0x02
#define DT_SKIP 0x01
#define DT_AUDIO 0x06
#define DT_RGB 0x07
#define DT_FAST 0x08
#define SIG_EOF 0x09
#define DT_ENGTXT 0x0A
#define DT_DLT 0x10
#define DT_MAXINDEX 0x1F
/******Block Type*************/


#endif