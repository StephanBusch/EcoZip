#ifndef _CODER_H
#define _CODER_H

#include "csc32Common.h"
#include <stdio.h>
#include "csc32IOCallBack.h"


class Coder
{
public:
	IOCallBack *m_io;

	//Initialize the coder, buffer1\2\size should not be NULL if x==ENCODE 
	void Init(u32 x,u8* buffer1,u8* buffer2,u32 size); 
	//flush the coder
	void Flush();	

	//Get coded length
	u32 GetCodedLength(void) {return m_countBC+m_countRC;}

	void EncDirect16(u32 val,u32 len);

	u32 DecDirect16(u32 len)
	{
		static u32 result;

		while(m_bitNum<len)
		{
			m_bitValue=(m_bitValue<<8)|*m_pBC;
			m_pBC++;
			m_countBC++;
			if (m_countBC==m_BCBufferSize)
			{
				streamTotalBytes+=m_countBC;
				m_io->ReadBCProc(m_ioBCBuffer,m_BCBufferSize);
				m_countBC=0;
				m_pBC=m_ioBCBuffer;
			}
			m_bitNum+=8;
		}
		result=(m_bitValue>>(m_bitNum-len))&((1<<len)-1);
		m_bitNum-=len;
		return result;
	}

	void RC_ShiftLow(void);

	//the main buffer for encoding/decoding range/bit coder
	u8* m_ioRCBuffer,*m_ioBCBuffer;		

	//indicates the full size of buffer range/bit coder
	u32 m_RCBufferSize,m_BCBufferSize;				

	//identify it's a encoder/decoder

	u32 m_op;		
	// for bitwise range coder
	u32 m_x1,m_x2,m_mid,m_x;

	u64 m_low,m_cacheSize;
	u32 m_range,m_code;
	u8 m_cache;

	// for bit coder
	u32 m_bitNum,m_bitValue;	

	//the i/o pointer of range coder and bit coder
	u8 *m_pRC,*m_pBC;	
	//byte counts of output bytes by range coder and bit coder
	u32 m_countBC,m_countRC;
	i64 streamTotalBytes;
};


#define EncodeBit(coder,v,p) \
__pragma(warning(push)) \
__pragma(warning(disable:4127)) \
do\
{\
	u32 newBound=(coder->m_range>>12)*p;\
	if (v)\
{\
	coder->m_range = newBound;\
	p += (0xFFF - p) >> 5;\
}\
	else\
{\
	coder->m_low+=newBound;\
	coder->m_range-=newBound;\
	p -= p >> 5;\
}\
	if (coder->m_range < (1<<24))\
{\
	coder->m_range <<= 8;\
	coder->RC_ShiftLow();\
}\
}while(0)\
__pragma(warning(pop))

/*
#define EncodeBit2(coder,v,p1,p2) do\
{\
	u32 newBound=(coder->m_range>>13)*(p1+p2);\
	if (v)\
{\
	coder->m_range = newBound;\
	p1 += (0xFFF - p1) >> 5;\
	p2 += (0xFFF - p2) >> 5;\
}\
	else\
{\
	coder->m_low+=newBound;\
	coder->m_range-=newBound;\
	p1 -= p1 >> 5;\
	p2 -= p2 >> 5;\
}\
	if (coder->m_range < (1<<24))\
{\
	coder->m_range <<= 8;\
	coder->RC_ShiftLow();\
}\
}while(0)
*/

#define DecodeBit(coder,v,p) \
__pragma(warning(push)) \
__pragma(warning(disable:4127)) \
do\
{\
	if (coder->m_range<(1<<24))\
{\
	coder->m_range<<=8;\
	coder->m_code=(coder->m_code<<8)+*coder->m_pRC++;\
	coder->m_countRC++;\
	if (coder->m_countRC==coder->m_RCBufferSize)\
{\
	coder->streamTotalBytes+=coder->m_countRC;\
	coder->m_io->ReadRCProc(coder->m_ioRCBuffer,coder->m_RCBufferSize);\
	coder->m_countRC=0;\
	coder->m_pRC=coder->m_ioRCBuffer;\
}\
}\
	u32 bound=(coder->m_range>>12)*p;\
	if (coder->m_code<bound)\
{\
	coder->m_range = bound;\
	p+=(0xFFF-p) >> 5;\
	v=v+v+1;\
}\
	else\
{\
	coder->m_range-=bound;\
	coder->m_code-=bound;\
	p -= p >> 5;\
	v=v+v;\
}\
}while(0)\
__pragma(warning(pop))


#define EncodeDirect(x,v,l) \
__pragma(warning(push)) \
__pragma(warning(disable:4127)) \
do\
{if (l<=16)\
	x->EncDirect16(v,l);\
else\
{\
	x->EncDirect16(v>>16,l-16);\
	x->EncDirect16(v&0xFFFF,16);\
}\
}while(0)\
__pragma(warning(pop))

#define DecodeDirect(x,v,l) \
__pragma(warning(push)) \
__pragma(warning(disable:4127)) \
do\
{if (l<=16)\
	v=x->DecDirect16(l);\
else\
{\
	v=x->DecDirect16(l-16)<<16;\
	v=v|x->DecDirect16(16);\
}\
}while(0)\
__pragma(warning(pop))



#endif
