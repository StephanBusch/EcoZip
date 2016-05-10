#include "csc32Coder.h"
#include <stdio.h>


void Coder::Init(u32 x,u8* buffer1,u8* buffer2,u32 size)
{
	m_op=x;

	m_x1=m_x=0;
	m_x2=0xFFFFFFFF;

	m_low = 0;
	m_range = 0xFFFFFFFF;
	m_cacheSize = 1;
	m_cache = 0;
	m_code=0;

	m_countRC=m_countBC=0;
	m_bitNum=m_bitValue=0;

	streamTotalBytes=0;


	if (m_op==DECODE)
	{
		m_pRC=m_ioRCBuffer=buffer1;
		m_pBC=m_ioBCBuffer=buffer2;

		m_io->ReadRCProc(m_ioRCBuffer, m_RCBufferSize);
		m_io->ReadBCProc(m_ioBCBuffer, m_BCBufferSize);

		m_code=((u32)m_pRC[1] << 24) | ((u32)m_pRC[2] << 16) | ((u32)m_pRC[3] << 8) | ((u32)m_pRC[4]);
		m_pRC+=5;
		m_countRC+=5;
	}
	else
	{
		m_pRC=m_ioRCBuffer=buffer1;
		m_pBC=m_ioBCBuffer=buffer2;
		m_RCBufferSize=m_BCBufferSize=size;
	}

}


void Coder::Flush()
{
	for (int i=0;i<5;i++) // One more byte for EOF
	{
		RC_ShiftLow();
	}
	m_pRC++;
	m_countRC++;

	*m_pBC=(m_bitValue<<(8-m_bitNum))&0xFF;
	m_pBC++;
	m_countBC++;
	if (m_countBC==m_BCBufferSize)
	{
		streamTotalBytes+=m_countRC;
		m_io->WriteBCProc(m_ioBCBuffer,m_BCBufferSize);
		m_countBC=0;
		m_pBC=m_ioBCBuffer;
	}

	//one more byte for bitcoder is to prevent overflow while reading
	*m_pBC=0;
	m_pBC++;
	m_countBC++;
	if (m_countBC==m_BCBufferSize)
	{
		streamTotalBytes+=m_countBC;
		m_io->WriteBCProc(m_ioBCBuffer,m_BCBufferSize);
		m_countBC=0;
		m_pBC=m_ioBCBuffer;
	}
	m_bitValue=0;
	m_bitNum=0;

	m_io->WriteRCProc(m_ioRCBuffer,m_countRC);
	m_io->WriteBCProc(m_ioBCBuffer,m_countBC);
}




void Coder::EncDirect16(u32 val,u32 len)
{
	m_bitValue=(m_bitValue<<len)|val;
	m_bitNum+=len;
	while(m_bitNum>=8)
	{
		*m_pBC=(m_bitValue>>(m_bitNum-8))&0xFF;
		m_pBC++;
		m_countBC++;
		if (m_countBC==m_BCBufferSize)
		{
			streamTotalBytes+=m_countBC;
			m_io->WriteBCProc(m_ioBCBuffer,m_BCBufferSize);
			m_countBC=0;
			m_pBC=m_ioBCBuffer;
		}
		m_bitNum-=8;
	}
}


void Coder::RC_ShiftLow(void)
{
	static u8 temp;
	if ((u32)m_low < (u32)0xFF000000 || (i32)(m_low >> 32) != 0)
	{
		temp = m_cache;
		do
		{
			*m_pRC++=(u8)(temp+(u8)(m_low >> 32));
			m_countRC++;
			if (m_countRC==m_RCBufferSize)
			{
				streamTotalBytes+=m_countRC;
				m_io->WriteRCProc(m_ioRCBuffer,m_RCBufferSize);
				m_countRC=0;
				m_pRC=m_ioRCBuffer;
			}
			temp = 0xFF;
		}
		while (--m_cacheSize != 0);
		m_cache = (u8)((u32)m_low >> 24);
	}
	m_cacheSize++;
	m_low = (u32)m_low << 8;
}