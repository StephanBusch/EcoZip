#include "csc32IOCallBack.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys\STAT.h>
#include <io.h>




FileDirectIO::FileDirectIO()
{
	m_stream = NULL;
	m_blockSize = 0;

	m_nRCHead = m_nRCTail = -1;
	m_nBCHead = m_nBCTail = -1;
}



void FileDirectIO::SetStream(void *stream)
{
	m_stream = stream;
}

void FileDirectIO::SetBlockSize(int size)
{
	m_blockSize=size;
}


void FileDirectIO::SetInitialFilePos(i64 /*filePos*/)
{
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


void FileDirectIO::WriteRCProc(u8 *buffer, u32 size)
{
	u32 processedSize;
	u8 tag=0;

	if (m_stream == NULL) return;
	ISequentialOutStream *outStream = (ISequentialOutStream *)m_stream;

	tag|=(1<<7);
	if ((int)size==m_blockSize)
		tag|=(1<<6);

	outStream->Write(&tag, 1, &processedSize);

	if ((int)size!=m_blockSize)
	{
		outStream->Write(&size, 4, &processedSize);
	}
	outStream->Write(buffer, size, &processedSize);
}


void FileDirectIO::WriteBCProc(u8 *buffer, u32 size)
{
	u32 processedSize;
	u8 tag = 0;

	if (m_stream == NULL) return;
	ISequentialOutStream *outStream = (ISequentialOutStream *)m_stream;

	tag |= (0 << 7);
	if ((int)size==m_blockSize)
		tag|=(1<<6);

	outStream->Write(&tag, 1, &processedSize);

	if ((int)size!=m_blockSize)
	{
		outStream->Write(&size, 4, &processedSize);
	}
	outStream->Write(buffer, size, &processedSize);
}

void FileDirectIO::ReadBlock(u8 *buffer, u32 &size, bool rc)
{
	u32 processedSize;
	u8 tag;
	u32 currSize = 0;

	int nBlock;
	if (rc) {
		if ((nBlock = dequeue(m_rcQueue, m_nRCHead, m_nRCTail)) >= 0) {
			CSC32_BLOCK &block = m_rcQueue[nBlock];
			memcpy(buffer, block.buffer, block.nDataSize);
			size = (u32)block.nDataSize;
			return;
		}
	}
	else if ((nBlock = dequeue(m_bcQueue, m_nBCHead, m_nBCTail)) >= 0) {
		CSC32_BLOCK &block = m_bcQueue[nBlock];
		memcpy(buffer, block.buffer, block.nDataSize);
		size = (u32)block.nDataSize;
		return;
	}

	if (m_stream == NULL) {
		size = 0;
		return;
	}

  for (;;)
	{
		if (read(&tag, 1, &processedSize) != S_OK) 
		{
			size=currSize;
			return;
		}
		if (tag & (1 << 6))
		{
			//full size block
			currSize = m_blockSize;
		}
		else
		{
			currSize = 0;
			read(&currSize, 4, &processedSize);
		}
		if ((rc && (tag & (1 << 7))) || (!rc && !(tag & (1 << 7)))) {
			read(buffer, currSize, &processedSize);
			size = currSize;
			break;
		}
		else {
			void *newBuffer = (tag & (1 << 7)) ?
				enqueue(m_rcQueue, m_nRCHead, m_nRCTail, currSize) :
				enqueue(m_bcQueue, m_nBCHead, m_nBCTail, currSize);
			read(newBuffer, currSize, &processedSize);
		}
	}
}


void FileDirectIO::ReadRCProc(u8 *buffer, u32 &size)
{
	ReadBlock(buffer, size, true);
}


void FileDirectIO::ReadBCProc(u8 *buffer, u32 &size)
{
	ReadBlock(buffer, size, false);
}


void FileDirectIO::ReadBuf(void *buf, u32 size)
{
	u32 processedSize;
	if (m_stream == NULL) return;
	ISequentialInStream *inStream = (ISequentialInStream *)m_stream;

	inStream->Read(buf, size, &processedSize);
}


void FileDirectIO::WriteBuf(void *buf, u32 size)
{
	u32 processedSize;
	if (m_stream == NULL) return;
	ISequentialOutStream *outStream = (ISequentialOutStream *)m_stream;

	outStream->Write(buf, size, &processedSize);
}


void FileDirectIO::WriteInt(i64 value, u32 bytes)
{
	u32 processedSize;
	u8 numBuf[8];

	if (m_stream == NULL) return;
	ISequentialOutStream *outStream = (ISequentialOutStream *)m_stream;


	for (u32 i = 0; i<bytes; i++)
	{
		numBuf[i]=value&0xFF;
		value>>=8;
	}

	outStream->Write(numBuf, bytes, &processedSize);
}


i64 FileDirectIO::ReadInt(u32 bytes)
{
	u32 processedSize;
	u8 numBuf[8];
	i64 result=0;
	
	if (m_stream == NULL) return 0;
	ISequentialInStream *inStream = (ISequentialInStream *)m_stream;

	inStream->Read(numBuf, bytes, &processedSize);
	for(int i=bytes-1;i>=0;i--)
	{
		result<<=8;
		result|=numBuf[i];
	}
	return result;
}


void * FileDirectIO::enqueue(CObjectVector<CSC32_BLOCK> &queue, int &head, int &tail, int size)
{
	if ((head == 0 && tail == (int)queue.Size() - 1) || (head == tail + 1) || queue.Size() == 0) {
		CSC32_BLOCK block;
		block.alloc(size);
    if (tail == (int)queue.Size() - 1)
			queue.Add(block);
		else {
			queue.Insert(tail, block);
			head++;
		}
		if (head < 0)
			head = 0;
		tail++;
		return block.buffer;
	}
	if (head == -1) {
		head = 0;
		tail = 0;
	}
	else {
    if (tail == (int)queue.Size() - 1)
			tail = 0;
		else
			tail = tail + 1;
	}
	CSC32_BLOCK &block = queue[tail];
	if (!block.fit(size)) {
		block.free();
		block.alloc(size);
	}
	return block.buffer;
}


void FileDirectIO::enqueue(CObjectVector<CSC32_BLOCK> &queue, int &head, int &tail, int size, void *buffer)
{
  if ((head == 0 && tail == (int)queue.Size() - 1) || (head == tail + 1) || queue.Size() == 0) {
		CSC32_BLOCK block;
		block.nBlockSize = block.nDataSize = size;
		block.buffer = buffer;
    if (tail == (int)queue.Size() - 1)
			queue.Add(block);
		else {
			queue.Insert(tail, block);
			head++;
		}
		if (head < 0)
			head = 0;
		tail++;
		return;
	}
	if (head == -1) {
		head = 0;
		tail = 0;
	}
	else {
    if (tail == (int)queue.Size() - 1)
			tail = 0;
		else
			tail = tail + 1;
	}
	CSC32_BLOCK &block = queue[tail];
	block.nBlockSize = block.nDataSize = size;
	block.buffer = buffer;
}


int FileDirectIO::dequeue(CObjectVector<CSC32_BLOCK> &queue, int &head, int &tail)
{
	int rlt = -1;
	if (head >= 0) {
		rlt = head;
		if (head == tail) {
			head = -1;
			tail = -1;
		}
		else {
      if (head == (int)queue.Size() - 1)
				head = 0;
			else
				head = head + 1;
		}
	}
	return rlt;
}


bool FileDirectIO::is_empty(CObjectVector<CSC32_BLOCK> &queue, int &head, int &tail)
{
  return (head == tail || head == tail + (int)queue.Size());
}


HRESULT FileDirectIO::read(void *data, UInt32 size, UInt32 *processedSize)
{
	if (processedSize != NULL)
		*processedSize = 0;

	if (m_stream == NULL) {
		size = 0;
		return E_FAIL;
	}
	ISequentialInStream *inStream = (ISequentialInStream *)m_stream;

	UInt32 inSize;
	while (size > 0) {
		RINOK(inStream->Read(data, size, &inSize));
		if (inSize == 0)
			return E_FAIL;
		data = (unsigned char *)data + inSize;
		size -= inSize;
		if (processedSize != NULL)
			*processedSize += inSize;
	}
	return S_OK;
}

