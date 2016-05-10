#include "csc32CCsc.h"
#include "csc32Coder.h"
#include "csc32LZ.h"
#include "csc32Model.h"
#include "csc32Analyzer.h"
#include "csc32Common.h"
#include "csc32Filters.h"
#include <stdlib.h>

CSCSettings::CSCSettings()
{
	wndSize=32*MB;
	//outStreamBlockSize=DefaultOutStreamBlockSize;
	maxSuccBlockSize=MaxChunkSize;
	InBufferSize=DefaultInBufferSize;
	SetDefaultMethod(2);
}

void CSCSettings::Refresh()
{
	if (wndSize<=64*KB)
		hashBits=16;
	else if (wndSize<=128*KB)
		hashBits=17;
	else if (wndSize<=256*KB)
		hashBits=18;
	else if (wndSize<=512*KB)
		hashBits=19;
	else if (wndSize<=2*MB)
		hashBits=21;
	else if (wndSize<=8*MB)
		hashBits=22;
	else if (wndSize<=32*MB)
		hashBits=23;
	else if (wndSize<=128*MB)
		hashBits=24;
	else 
		hashBits=25;

  for (u32 i = 1; i<hashWidth; i = i << 1)
		hashBits--;

	while(hashWidth*(1<<hashBits)+wndSize>600*MB)
	{
		hashBits--;
	}
}

void CSCSettings::SetDefaultMethod(u8 method)
{
	switch(method)
	{
	case 1:
		lzMode=1;
		hashWidth=1;
		break;
	case 2:
		lzMode=2;
		hashWidth=1;
		break;
	case 3:
		lzMode=3;
		hashWidth=2;
		break;
	case 4:
		lzMode=4;
		hashWidth=8;
		break;
	default:
		lzMode=2;
		hashWidth=1;
		break;
	}

	if (wndSize<=64*KB)
		hashBits=16;
	else if (wndSize<=128*KB)
		hashBits=17;
	else if (wndSize<=256*KB)
		hashBits=18;
	else if (wndSize<=512*KB)
		hashBits=19;
	else if (wndSize<=2*MB)
		hashBits=21;
	else if (wndSize<=8*MB)
		hashBits=22;
	else if (wndSize<=32*MB)
		hashBits=23;
	else if (wndSize<=128*MB)
		hashBits=24;
	else 
		hashBits=25;

  for (u32 i = 1; i<hashWidth; i = i << 1)
		hashBits--;

	while(hashWidth*(1<<hashBits)+wndSize>600*MB)
	{
		hashBits--;
	}


	setEXEFilter(true);
	setDLTFilter(true);
  setTXTFilter(true);
}

CCsc::CCsc()
{
	m_initialized=false;
	m_model.m_coder=&m_coder;
	m_lz.m_model=&m_model;
	typeArg1=typeArg2=typeArg3=0;
	fixedDataType=DT_NONE;
}

void CCsc::EncodeInt(u32 num,u32 bits)
{
	m_model.EncodeInt(num,bits);
}


u32 CCsc::DecodeInt(u32 bits)
{
	return m_model.DecodeInt(bits);
}


int CCsc::Init(u32 operation,CSCSettings setting)
{
	int err;

	if (m_initialized)
		return ALREADY_INITIALIZED;

	m_setting=setting;
	m_operation=operation;

	m_coder.m_io=m_setting.io;
	m_succBlockSize=m_setting.maxSuccBlockSize;
	m_rcbuffer=(u8*)malloc(m_setting.outStreamBlockSize);
	m_bcbuffer=(u8*)malloc(m_setting.outStreamBlockSize);

	if (m_rcbuffer==NULL || m_bcbuffer==NULL)
	{
		err=CANT_ALLOC_MEM;
		goto FREE_ON_ERROR;
	}


	m_coder.Init(operation,m_rcbuffer,m_bcbuffer,m_setting.outStreamBlockSize);
	err=m_model.Init();
	if (err<0)
		goto FREE_ON_ERROR;
	err=m_lz.Init(m_setting.wndSize,operation,m_setting.hashBits,m_setting.hashWidth);
	if (err<0)
		goto FREE_ON_ERROR;


	if (!m_setting.DLTFilter() && !m_setting.EXEFilter() && !m_setting.TXTFilter())
		m_useFilters=false;
	else
		m_useFilters=true;

	m_initialized=true;
	return NO_ERROR;

FREE_ON_ERROR:
	SAFEFREE(m_rcbuffer);
	SAFEFREE(m_bcbuffer);
	return err;
}


void CCsc::InternalCompress(u8 *src,u32 size,u32 type)
{
	u32 lzMode=m_setting.lzMode;

	/*printf("\n%d %d\n",size,type);*/

	if (size==0) return;

	if (type==DT_FAST) 
	{
		type=DT_NORMAL;
		lzMode=0;
	}

	if (type==DT_NORMAL)
	{
		m_model.EncodeInt(type,5);
		m_lz.EncodeNormal(src,size,lzMode);
	}
	else if (type==DT_EXE)
	{
		m_model.EncodeInt(type,5);
		m_filters.Forward_E89(src,size);
		m_lz.EncodeNormal(src,size,m_setting.lzMode);
	}
	else if (type==DT_ENGTXT)
	{
		if (m_filters.Foward_Dict(src,size))
		{
			m_model.EncodeInt(type,5);
			m_model.EncodeInt(size,MaxChunkBits);
		}
		else
			m_model.EncodeInt(DT_NORMAL,5);

		m_lz.EncodeNormal(src,size,m_setting.lzMode);
	}
	/*else if (type==DT_HARD)
	{
	m_model.EncodeBlockType(type);
	m_model.CompressHard(src,size);
	}*/
	/*else if (type==DT_AUDIO)
	{
		m_filters.Forward_Audio(src,size,typeArg1,typeArg2);
		m_model.CompressHard(src,size);
	}
	else if (type==DT_RGB)
	{
		m_model.EncodeInt(typeArg1,16);
		m_model.EncodeInt(typeArg2,6);
		m_filters.Forward_RGB(src,size,typeArg1,typeArg2);
		m_model.CompressRLE(src,size);
		//m_model.CompressValue(src,size,typeArg1,typeArg2/8);
	}*/
	else if (type==DT_BAD)
	{
		m_model.EncodeInt(type,5);
		m_model.CompressBad(src,size);
	}
	else if (type>=DT_DLT && type<DT_DLT+DLT_CHANNEL_MAX)
	{
		m_model.EncodeInt(type,5);
		u32 chnNum=DltIndex[type-DT_DLT];
		m_filters.Forward_Delta(src,size,chnNum);
		//m_lz.EncodeNormal(src,size,0);
		//if (chnNum>=0)
			m_model.CompressRLE(src,size);
		//else
		//	m_model.CompressDelta(src,size);
		//m_model.CompressHard(src,size);
	}
	else 
	{
		printf("Bad data type:%d\n",type);
		//m_model.CompressBad(src,size);
	}
}



int CCsc::Decompress(u8 *dst,u32 *size)
{
	u32 type;
	int returnValue=0;

	if (!m_initialized)
		return OPERATION_ERROR;

	type=m_model.DecodeInt(5);
	switch (type)
	{
	case DT_NORMAL:
		returnValue=m_lz.Decode(dst,size,m_succBlockSize);
		if (returnValue<0)
			return returnValue;
		break;
	case DT_EXE:
		returnValue=m_lz.Decode(dst,size,m_succBlockSize);
		if (returnValue<0)
			return returnValue;
		m_filters.Inverse_E89(dst,*size);
		break;
	case DT_ENGTXT:
		*size=m_model.DecodeInt(MaxChunkBits);
		returnValue=m_lz.Decode(dst,size,m_succBlockSize);
		if (returnValue<0)
			return returnValue;
		m_filters.Inverse_Dict(dst,*size);
		break;
	case DT_BAD:
		m_model.DecompressBad(dst,size);
		m_lz.DuplicateInsert(dst,*size);
		break;
	//case DT_HARD:
	//	m_model.DecompressHard(dst,size);
	//	m_lz.DuplicateInsert(dst,*size);
	//	break;
	/*case DT_AUDIO:
		m_model.DecompressHard(dst,size);
		m_filters.Inverse_Audio(dst,*size,typeArg1,typeArg2);
		m_lz.DuplicateInsert(dst,*size);
		break;
	case DT_RGB:
		typeArg1=m_model.DecodeInt(16);
		typeArg2=m_model.DecodeInt(6);
		m_model.DecompressHard(dst,size);
		m_filters.Inverse_RGB(dst,*size,typeArg1,typeArg2);
		m_lz.DuplicateInsert(dst,*size);
		break;*/
	case SIG_EOF:
		*size=0;
		break;
	default:
		if (type>=DT_DLT && type<DT_DLT+DLT_CHANNEL_MAX)
		{
			u32 chnNum=DltIndex[type-DT_DLT];
			//if (chnNum>=0)
				m_model.DecompressRLE(dst,size);
			//else
			//	m_model.DecompressDelta(dst,size);
			m_filters.Inverse_Delta(dst,*size,chnNum);
			m_lz.DuplicateInsert(dst,*size);
		}
		else
		{
			//printf("Error No such type :%d\n",type);
			return DECODE_ERROR;
		}
		break;
	}
	return 0;
}

void CCsc::Compress(u8 *src,u32 size)
{
	u32 lastType,thisType;
	u32 lastBegin,lastSize;
	u32 currBlockSize;
	u32 progress;

	if (!m_initialized)
		return;


	lastBegin=lastSize=0;
	lastType=DT_NORMAL;

	progress=0;



	while (progress<size)
	{
		currBlockSize=MIN(MinBlockSize,size-progress);

		if (lastSize+currBlockSize>m_succBlockSize)
		{
			InternalCompress(src+lastBegin,lastSize,lastType);
			lastBegin=progress;
			lastSize=0;
		}

		if (m_useFilters)
		{
			if (fixedDataType==DT_NONE)
				thisType=m_analyzer.analyze(src+progress,currBlockSize);
			else thisType=fixedDataType;
		}
		else
			thisType=DT_NORMAL;


		if (thisType!=DT_NORMAL)
		{
			if (thisType==DT_EXE && !m_setting.EXEFilter())
				thisType=DT_NORMAL;
			else if (thisType==DT_ENGTXT && !m_setting.TXTFilter())
				thisType=DT_NORMAL;
			else if (thisType>=DT_DLT && !m_setting.DLTFilter())
				thisType=DT_NORMAL;
		}

		if (thisType==DT_BAD)
		{
			if (lastType!=DT_BAD)
			{
				InternalCompress(src+lastBegin,lastSize,lastType);
			}

			if (m_lz.CheckDuplicate(src+progress,currBlockSize,thisType)==DT_NORMAL)
			{
				if (lastType==DT_BAD)
					InternalCompress(src+lastBegin,lastSize,lastType);

				lastBegin=progress;
				lastSize=currBlockSize;
				lastType=DT_NORMAL;
			}
			else
			{
				if (lastType==DT_BAD)
				{
					lastSize+=currBlockSize;
				}
				else
				{
					lastBegin=progress;
					lastSize=currBlockSize;
					lastType=DT_BAD;
				}
			}
		}
		else if (thisType==DT_NORMAL)
		{
			if (lastType==DT_NORMAL)
			{
				lastSize+=currBlockSize;
			}
			else
			{
				InternalCompress(src+lastBegin,lastSize,lastType);
				lastBegin=progress;
				lastSize=currBlockSize;
				lastType=DT_NORMAL;
			}
		}
		else if (thisType==DT_FAST)
		{
			if (lastType==DT_FAST)
			{
				lastSize+=currBlockSize;
			}
			else
			{
				InternalCompress(src+lastBegin,lastSize,lastType);
				lastBegin=progress;
				lastSize=currBlockSize;
				lastType=DT_FAST;
			}
		}
		else if (thisType==DT_SKIP)
		{
			if (lastType==DT_BAD || lastType>=DT_DLT)
				m_lz.DuplicateInsert(src+lastBegin+lastSize,currBlockSize);
			//Important!  because if last type is BAD or DT_DLT,this small block won't 
			//automaticlly insertted into LZ Dictionary.

			lastSize+=currBlockSize;
		}
		else if (thisType==DT_EXE)
		{
			if (lastType==DT_EXE)
			{
				lastSize+=currBlockSize;
			}
			else
			{
				InternalCompress(src+lastBegin,lastSize,lastType);
				lastBegin=progress;
				lastSize=currBlockSize;
				lastType=DT_EXE;
			}
		}
		else if (thisType==DT_ENGTXT)
		{
			if (lastType==DT_ENGTXT)
			{
				lastSize+=currBlockSize;
			}
			else
			{
				InternalCompress(src+lastBegin,lastSize,lastType);
				lastBegin=progress;
				lastSize=currBlockSize;
				lastType=DT_ENGTXT;
			}
		}
		else if (thisType>=DT_DLT && thisType<DT_DLT+DLT_CHANNEL_MAX)
		{
			if (lastType!=thisType)
			{
				InternalCompress(src+lastBegin,lastSize,lastType);
			}

			if (m_lz.CheckDuplicate(src+progress,currBlockSize,thisType)==DT_NORMAL)
			{
				if (lastType==thisType)
					InternalCompress(src+lastBegin,lastSize,lastType);

				lastBegin=progress;
				lastSize=currBlockSize;
				lastType=DT_NORMAL;
			}
			else
			{
				if (lastType==thisType)
				{
					lastSize+=currBlockSize;
				}
				else
				{
					lastBegin=progress;
					lastSize=currBlockSize;
					lastType=thisType;
				}
			}
		}
		else if (thisType==DT_RGB)
		{
			if (lastType!=thisType)
			{
				InternalCompress(src+lastBegin,lastSize,lastType);
			}

			if (m_lz.CheckDuplicate(src+progress,currBlockSize,thisType)==DT_NORMAL)
			{
				if (lastType==thisType)
					InternalCompress(src+lastBegin,lastSize,lastType);

				lastBegin=progress;
				lastSize=currBlockSize;
				lastType=DT_NORMAL;
			}
			else
			{
				if (lastType==thisType)
				{
					lastSize+=currBlockSize;
				}
				else
				{
					lastBegin=progress;
					lastSize=currBlockSize;
					lastType=thisType;
				}
			}
		}
		else if (thisType==DT_AUDIO)
		{
			if (lastType!=thisType)
			{
				InternalCompress(src+lastBegin,lastSize,lastType);
			}

			if (m_lz.CheckDuplicate(src+progress,currBlockSize,thisType)==DT_NORMAL)
			{
				if (lastType==thisType)
					InternalCompress(src+lastBegin,lastSize,lastType);

				lastBegin=progress;
				lastSize=currBlockSize;
				lastType=DT_NORMAL;
			}
			else
			{
				if (lastType==thisType)
				{
					lastSize+=currBlockSize;
				}
				else
				{
					lastBegin=progress;
					lastSize=currBlockSize;
					lastType=thisType;
				}
			}
		}
		else if (thisType==DT_HARD)
		{
			if (lastType!=DT_HARD)
			{
				InternalCompress(src+lastBegin,lastSize,lastType);
			}

			if (m_lz.CheckDuplicate(src+progress,currBlockSize,thisType)==DT_NORMAL)
			{
				if (lastType==DT_HARD)
					InternalCompress(src+lastBegin,lastSize,lastType);

				lastBegin=progress;
				lastSize=currBlockSize;
				lastType=DT_NORMAL;
			}
			else
			{
				if (lastType==DT_HARD)
				{
					lastSize+=currBlockSize;
				}
				else
				{
					lastBegin=progress;
					lastSize=currBlockSize;
					lastType=DT_HARD;
				}
			}
		}
		else
		{
			printf("FATAL ERROR!");
			exit(-1);
		}
		progress+=currBlockSize;
	}
	InternalCompress(src+lastBegin,lastSize,lastType);
}


void CCsc::Flush()
{
	if (!m_initialized)
		return;
	//m_model.EncodeInt(SIG_EOF,5);
	m_coder.Flush();
}

void CCsc::WriteEOF()
{
	if (!m_initialized)
		return;
	m_model.EncodeInt(SIG_EOF,5);
}


void CCsc::CheckFileType(u8 * /*src*/, u32 /*size*/)
{
	//fixedDataType=m_analyzer.analyzeHeader(src,size,&typeArg1,&typeArg2,&typeArg3);
	fixedDataType=DT_NONE;
	return;
}

void CCsc::Destroy()
{
	if (!m_initialized)
		return;

	m_lz.Destroy();
	SAFEFREE(m_rcbuffer);
	SAFEFREE(m_bcbuffer);
	m_initialized=false;
}


CCsc::~CCsc()
{
	Destroy();
}

i64 CCsc::GetCompressedSize()
{
	if (!m_initialized)
		return 0;
	return (m_coder.streamTotalBytes+m_coder.m_countBC+m_coder.m_countRC);
}