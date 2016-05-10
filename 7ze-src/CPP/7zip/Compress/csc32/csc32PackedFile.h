#ifndef _PACKEDFILE_H
#define _PACKEDFILE_H

#include "csc32Common.h"




class PackedFile
{
public:
	//Create a new archive
	int Create(char *fileName);
	//Open an existing archive
	int Open(char *fileName);
	//close an archive
	void Close(void);

	//callback function: read a range coded block
	void ReadRCBlock(u8* buffer,u32& size);
	//callback function: read a bit coded block
	void ReadBCBlock(u8* buffer,u32& size);
	
	//callback function: write a range coded block
	void WriteRCBlock(u8* buffer,u32 size);
	//callback function: write a bit coded block
	void WriteBCBlock(u8* buffer,u32 size);
	


	//Read archive head
	int ReadHead(void);
	//Write archive head. Should after SetHead()
	void WriteHead(void);

	//Set head of archive
	void SetHead(char *fileName,i64 RawSize,u32 dictSize,u16 blockSize);
	void GetCompressProperty(u32 &dictSize,u16 &blockSize) {dictSize=packedFileHead.dictSize;blockSize=packedFileHead.blockSize;}
	//After opening an archive,get the raw filename
	char *GetRawFileName(){return packedFileHead.fileName;};
	//After opening an archive,get the raw filesize
	i64 GetRawFileSize(){return packedFileHead.rawSize;};
	//return wrote size
	i64 GetCurrentSize(void);

	PackedFile(); 
	~PackedFile() {/*if (!packedFileHead.fileNameLen) delete []packedFileHead.fileName;*/};

	

private:
	//file structure
	//FILE *m_f;
	
	//integer file descriptor
	int m_fd;

	//file pointer of next i/o request
	i64 m_basicFPos;
	//file pointers of range/bit coder i/o
	i64 m_RCFPos,m_BCFPos;

	struct _head {
		//Currently must be "fsy"
		char magicBytes[3];	
		//Currently must be 4
		u8 version;
		//size of packed file
		i64 packedSize;
		//size of raw file
		i64 rawSize;

		//compression dictionary size
		u32 dictSize;
		//bitcoded/rangecoded block size divided by KB (256 means 256 kb)
		u16 blockSize; 

		//raw fileName
		u16 fileNameLen;
		char fileName[260];
	} packedFileHead;
};


#endif
