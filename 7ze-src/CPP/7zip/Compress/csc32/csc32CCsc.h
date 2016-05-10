#ifndef _CCSC_H
#define _CCSC_H

#include "csc32Coder.h"
#include "csc32LZ.h"
#include "csc32Model.h"
#include "csc32Analyzer.h"
#include "csc32Common.h"
#include "csc32Filters.h"
#include "csc32IOCallBack.h"

#pragma pack(push, 1)

#define CSC32_DLT_FILTER_BIT  1
#define CSC32_TXT_FILTER_BIT  2
#define CSC32_EXE_FILTER_BIT  4

struct CSCSettings
{
  u8 lzMode;
  u8 Filter;

	u32 wndSize;
	u32 outStreamBlockSize;

  u32 hashWidth;
  u32 hashBits;
  u32 maxSuccBlockSize;
  u32 InBufferSize;

  IOCallBack *io;

  CSCSettings();
  void SetDefaultMethod(u8 method);
	void Refresh();
  bool DLTFilter() { return (Filter & CSC32_DLT_FILTER_BIT) != 0; }
  bool TXTFilter() { return (Filter & CSC32_TXT_FILTER_BIT) != 0; }
  bool EXEFilter() { return (Filter & CSC32_EXE_FILTER_BIT) != 0; }
  void setDLTFilter(bool val) {
    if (val)
      Filter |= CSC32_DLT_FILTER_BIT;
    else
      Filter &= !CSC32_DLT_FILTER_BIT;
  }
  void setTXTFilter(bool val) {
    if (val)
      Filter |= CSC32_TXT_FILTER_BIT;
    else
      Filter &= !CSC32_TXT_FILTER_BIT;
  }
  void setEXEFilter(bool val) {
    if (val)
      Filter |= CSC32_EXE_FILTER_BIT;
    else
      Filter &= !CSC32_EXE_FILTER_BIT;
  }
};

#pragma pack(pop)

#define CSC32_SETTING_LENGTH 	((size_t)(&((CSCSettings *)0)->hashWidth))

class CCsc
{
public:
	CCsc();
	~CCsc();

	int Init(u32 operation,CSCSettings setting);
	//operation should be ENCODE | DECODE
	

	void WriteEOF();
	//Should be called when finished compression of one part.

	void Flush();
	//Should be called when finished the whole compression.

	void Destroy();

	void Compress(u8 *src,u32 size);

	int Decompress(u8 *src,u32 *size);
	//*size==0 means meets the EOF in raw stream.

	void CheckFileType(u8 *src,u32 size);
	//Should be called before compress a file.src points
	//to first several bytes of file.
	
	void EncodeInt(u32 num,u32 bits);
	u32 DecodeInt(u32 bits);
	//Put/Get num on CSC Stream


	i64 GetCompressedSize();
	//Get current compressed size.
	

private:
	CSCSettings m_setting;
	bool m_initialized;

	u32 m_operation;
	u8 *m_rcbuffer;
	u8 *m_bcbuffer;
	u32 fixedDataType; //
	u32 typeArg1,typeArg2,typeArg3;

	Filters m_filters;
	Coder m_coder;
	Model m_model;
	
	LZ m_lz;
	Analyzer m_analyzer;

	u32 m_succBlockSize;
	//This determines how much maximumly the CCsc:Decompress can decompress
	// in one time. 

	bool m_useFilters;
	void InternalCompress(u8 *src,u32 size,u32 type);
	//compress the buffer and treat them in one type.It's called after analyze the data.

};

#endif