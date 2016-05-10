#include "csc32PackedFile.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys\STAT.h>
#include <io.h>



void PackedFile::SetHead(char* fileName,i64 RawSize,u32 dictSize,u16 blockSize)
{
	packedFileHead.fileNameLen=(u16)strlen(fileName);
	//packedFileHead.fileName=new char[packedFileHead.fileNameLen+1];
	sprintf(packedFileHead.fileName,"%s",fileName);

	packedFileHead.dictSize=dictSize;
	packedFileHead.rawSize=RawSize;
	packedFileHead.blockSize=blockSize;

}

PackedFile::PackedFile()
{
	sprintf(packedFileHead.magicBytes,"%c%c%c",'f','s','y');
	packedFileHead.version=4;
	packedFileHead.blockSize=256;
	packedFileHead.fileNameLen=0;
	//m_f=NULL;
	m_fd=0;
}

int PackedFile::Create(char *fileName)
{
	if (m_fd!=0)
		Close();

	m_fd=open(fileName,_O_WRONLY|_O_TRUNC|_O_BINARY|_O_CREAT,_S_IREAD | _S_IWRITE );
	if (m_fd==-1)
	{
		//printf("%d",errno);
		return CANT_CREATE_FILE;
	}

	//m_fd=fileno(m_f);
	WriteHead();
	return NO_ERROR;
}


int PackedFile::Open(char *fileName)
{
	if (m_fd!=0)
		Close();

	m_fd=open(fileName,_O_RDONLY|_O_BINARY,0);
	if (m_fd==-1)
		return CANT_OPEN_FILE;

	//m_fd=fileno(m_f);
	return ReadHead();;
}


void PackedFile::Close()
{
	if (m_fd!=0)
	{
		close(m_fd);
		m_fd=0;
		//m_f=NULL;
		//if (packedFileHead.fileNameLen!=0)
		//	delete []packedFileHead.fileName;
	}
}


int PackedFile::ReadHead()
{
	char magicBytes[3];
	u8 version;
	read(m_fd,magicBytes,3);
	if (magicBytes[0]!='f'||
		magicBytes[1]!='s'||
		magicBytes[2]!='y')
		return NOT_CSC_FILE;
	read(m_fd,&version,1);
	if (version!=4)
		return VERSION_INVALID;

	read(m_fd,&packedFileHead.packedSize,8);
	read(m_fd,&packedFileHead.rawSize,8);
	read(m_fd,&packedFileHead.dictSize,4);
	read(m_fd,&packedFileHead.blockSize,2);
	read(m_fd,&packedFileHead.fileNameLen,2);

	//packedFileHead.fileName=new char[packedFileHead.fileNameLen+1];
	read(m_fd,packedFileHead.fileName,packedFileHead.fileNameLen+1);

	m_RCFPos=m_BCFPos=m_basicFPos=_telli64(m_fd);
	return NO_ERROR;
}


void PackedFile::WriteHead()
{
	char tmpBuffer[28]={0};

	memcpy(tmpBuffer,packedFileHead.magicBytes,3);
	memcpy(tmpBuffer+3,&packedFileHead.version,1);
	memcpy(tmpBuffer+4,&packedFileHead.packedSize,8);
	memcpy(tmpBuffer+12,&packedFileHead.rawSize,8);
	memcpy(tmpBuffer+20,&packedFileHead.dictSize,4);
	memcpy(tmpBuffer+24,&packedFileHead.blockSize,2);
	memcpy(tmpBuffer+26,&packedFileHead.fileNameLen,2);
	
	write(m_fd,tmpBuffer,28);
	write(m_fd,packedFileHead.fileName,packedFileHead.fileNameLen+1);

	m_RCFPos=m_BCFPos=m_basicFPos=_telli64(m_fd);
}


/*
  BLOCK STRUCTURE:  tag(1B),[size(4B)],data

				tag: 0 0 0 0  0 0 0 0
                     | | | |  | | | |
           RC 1/BC 0 - | - reserved -
      FULL SIZE BLOCK 1-
		if it is not a full size (blocksize==size), there should
		be another 4Byte integer after "tag",indicates the actually
		data size.

	  reserved: maybe for encryption/data recovery/Checksum and so on.



	  *	Writing procedure writes range coded or bit coded blocks 
		sequentially.
	  *	And the reading procedure works separately.
	  *	Each reading procedure reads blocks sequentially but skips 
		the other type of blocks.Two procedure keeps each's io pointer(int64).

*/




void PackedFile::WriteRCBlock(u8* buffer,u32 size)
{
	u8 tag=0;

	tag|=(1<<7);
	if (size==packedFileHead.blockSize*KB)
		tag|=(1<<6);

	write(m_fd,&tag,1);

	if (size!=packedFileHead.blockSize*KB)
	{
		write(m_fd,&size,4);
	}
	write(m_fd,buffer,size);
}


void PackedFile::WriteBCBlock(u8* buffer,u32 size)
{
	u8 tag=0;

	tag|=(0<<7);
	if (size==packedFileHead.blockSize*KB)
		tag|=(1<<6);

	write(m_fd,&tag,1);

	if (size!=packedFileHead.blockSize*KB)
	{
		write(m_fd,&size,4);
	}
	write(m_fd,buffer,size);
}



void PackedFile::ReadRCBlock(u8* buffer,u32& size)
{
	u8 tag;
	u32 currSize;

  for (;;)
	{
		_lseeki64(m_fd,m_RCFPos,SEEK_SET);
		read(m_fd,&tag,1);
		m_RCFPos++;
		if (tag&(1<<6))
		{
			//full size block
			currSize=packedFileHead.blockSize*KB;
		}
		else
		{
			currSize=0;
			read(m_fd,&currSize,4);
			m_RCFPos+=4;
		}
		m_RCFPos+=currSize;
		if (tag&(1<<7))
		{
			//is Range coded block
			read(m_fd,buffer,currSize);
			size=currSize;
			break;
		}
	}
	//basic file pointer should after all read blocks
	m_basicFPos=m_BCFPos>m_RCFPos?m_BCFPos:m_RCFPos;
}

void PackedFile::ReadBCBlock(u8* buffer,u32& size)
{
	u8 tag;
	u32 currSize;

  for (;;)
	{
		_lseeki64(m_fd,m_BCFPos,SEEK_SET);
		read(m_fd,&tag,1);
		m_BCFPos++;
		if (tag&(1<<6))
		{
			//full size block
			currSize=packedFileHead.blockSize*KB;
		}
		else
		{
			currSize=0;
			read(m_fd,&currSize,4);
			m_BCFPos+=4;
		}
		m_BCFPos+=currSize;
		if (!(tag&(1<<7)))
		{
			//is bit coded block
			read(m_fd,buffer,currSize);
			size=currSize;
			break;
		}
	}
	//basic file pointer should after all read blocks
	m_basicFPos=m_BCFPos>m_RCFPos?m_BCFPos:m_RCFPos;
}

i64 PackedFile::GetCurrentSize(void)
{
	return _telli64(m_fd);
}