#ifndef _IOCALLBACK_H
#define _IOCALLBACK_H
#include "csc32Common.h"

#include "../../../Common/MyVector.h"
#include "../../IStream.h"

class IOCallBack
{
public:
	virtual void ReadRCProc(u8 *buffer,u32& size)=0;
	virtual void WriteRCProc(u8 *buffer,u32 size)=0;
	virtual void ReadBCProc(u8 *buffer,u32& size)=0;
	virtual void WriteBCProc(u8 *buffer,u32 size)=0;
};



////////////////////////////////////////////////////////////////////////

struct CSC32_BLOCK {
	int nBlockSize;
	int nDataSize;
	void *buffer;

	CSC32_BLOCK() {
		nBlockSize = 0;
		nDataSize = 0;
		buffer = NULL;
	}
	void alloc(int size) {
		buffer = ::malloc(size);
		nBlockSize = nDataSize = size;
	}
	void free() {
		::free(buffer);
	}
	bool fit(int size) {
		return nBlockSize >= size;
	}
};

class FileDirectIO : public IOCallBack
{
private:
	void *m_stream;
	int m_blockSize;

	CObjectVector<CSC32_BLOCK> m_rcQueue;
	CObjectVector<CSC32_BLOCK> m_bcQueue;
	int m_nRCHead, m_nRCTail;
	int m_nBCHead, m_nBCTail;

	static void * enqueue(CObjectVector<CSC32_BLOCK> &queue, int &head, int &tail, int size);
	static void enqueue(CObjectVector<CSC32_BLOCK> &queue, int &head, int &tail, int size, void *buffer);
	static int dequeue(CObjectVector<CSC32_BLOCK> &queue, int &head, int &tail);
	static bool is_empty(CObjectVector<CSC32_BLOCK> &queue, int &head, int &tail);

	HRESULT read(void *data, UInt32 size, UInt32 *processedSize);

	void ReadBlock(u8 *buffer, u32 &size, bool rc);

public:
	
	FileDirectIO();
	void SetStream(void *stream);
	void SetBlockSize(int size);
	void SetInitialFilePos(i64 filePos);


	void ReadRCProc(u8 *buffer,u32& size);
	void ReadBCProc(u8 *buffer,u32& size);
	void WriteRCProc(u8 *buffer,u32 size);
	void WriteBCProc(u8 *buffer,u32 size);


	void ReadBuf(void *buf,u32 size);
	void WriteBuf(void *buf,u32 size);
	
	void WriteInt(i64 value,u32 bytes);
	i64 ReadInt(u32 bytes);
};

#endif
