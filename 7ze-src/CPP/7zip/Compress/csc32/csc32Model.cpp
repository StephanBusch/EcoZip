#include "csc32Model.h"
#include <math.h>

#include <stdlib.h>


Model::Model()
{

}

Model::~Model()
{
	SAFEFREE(probDltO1);	
	SAFEFREE(probLiteral);
}

int Model::Init()
{
	for (int i=0;i<(4096>>3);i++)
	{
		probToBitNum[i]=(u32)(128*log((float)(i*8+4)/4096)/log(0.5));
	}

	probLiteral=(u32*)malloc(256*257*4);
	if (probLiteral==NULL)
		return CANT_ALLOC_MEM;
	else
		return NO_ERROR;
}


void Model::Reset(void)
{
	u32 i;

	probDltO1=NULL;

	for(i=0;i<64*3;i++)
		probState[i]=2048;


	for(i=0;i<256*257;i++)
		probLiteral[i]=2048;

	for(i=0;i<64*4;i++)
		probRepDist[i]=2048;

	for(i=0;i<4*64;i++)
		probDist1[i]=2048;

	for(i=0;i<16;i++)
		probLens[i]=2048;

	probLongLen=2048;

	for(i=0;i<16;i++)
		probDist2[i]=2048;

	for(i=0;i<8;i++)
		probDist3[i]=2048;


	probRLEFlag=2048;

	for(i=0;i<16;i++)
		probRLELens[i]=2048;

	state=0;	
	ctx=256;

	if (m_coder->m_op==ENCODE)
	{

	}
}

void Model::EncodeLiteral(u32 c)//,u32 pos
{
	u32 tmpCtx;

	EncodeBit(m_coder,0,probState[state*3+0]);
	state=(state*4)&0x3F;
	tmpCtx=ctx;
	ctx=c;

	c=c|0x100;

	u32 *p=&probLiteral[tmpCtx*256];
	do
	{
		EncodeBit(m_coder,(c >> 7) & 1,p[c>>8]);
		c <<= 1;
	}
	while (c < 0x10000);

}


void Model::Encode1BMatch(void)
{
	EncodeBit(m_coder,1,probState[state*3+0]);
	EncodeBit(m_coder,0,probState[state*3+1]);
	EncodeBit(m_coder,0,probState[state*3+2]);
	state=(state*4+2)&0x3F;
}


void Model::EncodeRepDistMatch(u32 repIndex,u32 matchLen)
{
	static u32 i,j,slot,c,lastLen;

	EncodeBit(m_coder,1,probState[state*3+0]);
	EncodeBit(m_coder,0,probState[state*3+1]);
	EncodeBit(m_coder,1,probState[state*3+2]);

	i=1;
	j=(repIndex>>1)&1; EncodeBit(m_coder,j,probRepDist[state*4+i]); i+=i+j;
	j=(repIndex)&1; EncodeBit(m_coder,j,probRepDist[state*4+i]); 


	if (matchLen>129)
	{
		c=15|0x10;
		do
		{
			EncodeBit(m_coder,(c >> 3) & 1,probLens[c>>4]);
			c <<= 1;
		}
		while (c < 0x100);

		matchLen-=129;

		while(matchLen>129)
		{
			matchLen-=129;
			EncodeBit(m_coder,0,probLongLen);
		}

		EncodeBit(m_coder,1,probLongLen);
	}

	for (slot=0;slot<17;slot++)
		if (matchLenBound[slot]>matchLen) break;

	slot--;

	c=slot|0x10;

	do
	{
		EncodeBit(m_coder,(c >> 3) & 1,probLens[c>>4]);
		c <<= 1;
	}
	while (c < 0x100);

	if (matchLenExtraBit[slot]>0)
	{
		i=matchLen-matchLenBound[slot];
		j=matchLenExtraBit[slot];
		EncodeDirect(m_coder,i,j);
	}

	state=(state*4+3)&0x3F;
}



void Model::EncodeMatch(u32 matchDist, u32 matchLen)
{
	static u32 i,j,slot,c,lastLen;
	u32 realMatchLen=matchLen;

	EncodeBit(m_coder,1,probState[state*3+0]);
	EncodeBit(m_coder,1,probState[state*3+1]);

	if (matchLen>129)
	{
		c=15|0x10;
		do
		{
			EncodeBit(m_coder,(c >> 3) & 1,probLens[c>>4]);
			c <<= 1;
		}
		while (c < 0x100);

		matchLen-=129;

		while(matchLen>129)
		{
			matchLen-=129;
			EncodeBit(m_coder,0,probLongLen);
		}

		EncodeBit(m_coder,1,probLongLen);
	}

	for (slot=0;slot<17;slot++)
		if (matchLenBound[slot]>matchLen) break;

	slot--;

	c=slot|0x10;

	do
	{
		EncodeBit(m_coder,(c >> 3) & 1,probLens[c>>4]);
		c <<= 1;
	}
	while (c < 0x100);

	if (matchLenExtraBit[slot]>0)
	{
		i=matchLen-matchLenBound[slot];
		j=matchLenExtraBit[slot];
		EncodeDirect(m_coder,i,j);
	}

	if (realMatchLen==1)
	{
		for (slot=0;slot<9;slot++)
			if (MDistBound3[slot]>matchDist) break;

		slot--;

		c=slot|0x08;

		u32 *p=&probDist3[0];

		do
		{
			EncodeBit(m_coder,(c >> 2) & 1,p[c>>3]);
			c <<= 1;
		}
		while (c < 0x40);

		if (MDistExtraBit3[slot]>0)
		{
			i=matchDist-MDistBound3[slot];
			j=MDistExtraBit3[slot];
			EncodeDirect(m_coder,i,j);
		}

	}
	else if (realMatchLen==2)
	{
		for (slot=0;slot<17;slot++)
			if (MDistBound2[slot]>matchDist) break;

		slot--;

		c=slot|0x10;

		u32 *p=&probDist2[0];
		do
		{
			EncodeBit(m_coder,(c >> 3) & 1,p[c>>4]);
			c <<= 1;
		}
		while (c < 0x100);

		if (MDistExtraBit2[slot]>0)
		{
			i=matchDist-MDistBound2[slot];
			j=MDistExtraBit2[slot];
			EncodeDirect(m_coder,i,j);
		}
	}
	else
	{
		u32 lenCtx=realMatchLen>5?3:realMatchLen-3;
		for (slot=0;slot<65;slot++)
			if (MDistBound1[slot]>matchDist) break;

		slot--;

		c=slot|0x40;

		u32 *p=&probDist1[lenCtx*64];
		do
		{
			EncodeBit(m_coder,(c >> 5) & 1,p[c>>6]);
			c <<= 1;
		}
		while (c < 0x1000);

		if (MDistExtraBit1[slot]>0)
		{
			i=matchDist-MDistBound1[slot];
			j=MDistExtraBit1[slot];
			EncodeDirect(m_coder,i,j);
		}
	}
	state=(state*4+1)&0x3F;
}






void Model::EncodeInt(u32 num,u32 bits)
{
	EncodeDirect(m_coder,num,bits);
}

u32 Model::DecodeInt(u32 bits)
{
	u32 num;
	DecodeDirect(m_coder,num,bits);
	return num;
}


void Model::CompressDelta(u8 *src,u32 size)
{

	u32 i;
	u32 *p;
	u32 c;
  u32 sCtx = 0;// , sCtx2 = 0;

	if (probDltO1==NULL)
	{
		probDltO1=(u32*)malloc(256*256*4);
		for (i=0;i<256*256;i++)
			probDltO1[i]=2048;
	}


	EncodeDirect(m_coder,size,MaxChunkBits);
	for(i=0;i<size;i++)
	{
		p=&probDltO1[sCtx*256];
		c=src[i]|0x100;
		do
		{
			EncodeBit(m_coder,(c >> 7) & 1,p[c>>8]);
			c <<= 1;
		}
		while (c < 0x10000);

		sCtx=src[i];
	}
	return;
}

//void Model::DecompressDelta(u8 *dst,u32 *size)
//{
//	u32 c,i;
//	u32 *p;
//	u32 sCtx=0,sCtx2=0;
//
//	if (probDltO1==NULL)
//	{
//		probDltO1=(u32*)malloc(256*256*4);
//		for (i=0;i<256*256;i++)
//			probDltO1[i]=2048;
//	}
//
//	DecodeDirect(m_coder,*size,MaxChunkBits);
//	for(i=0;i<*size;i++)
//	{
//		p=&probDltO1[sCtx*256];
//		c=1;
//		do 
//		{ 
//			DecodeBit(m_coder,c,p[c]);
//		} while (c < 0x100);
//
//		dst[i]=c&0xFF;
//		sCtx=dst[i];
//	}
//	return;
//}


void Model::CompressHard(u8 *src,u32 size)
{

	u32 i;
	u32 *p;
	u32 c;
	u32 sCtx=8;


	EncodeDirect(m_coder,size,MaxChunkBits);
	for(i=0;i<size;i++)
	{
		p=&probLiteral[sCtx*256];
		c=src[i]|0x100;
		do
		{
			EncodeBit(m_coder,(c >> 7) & 1,p[c>>8]);
			c <<= 1;
		}
		while (c < 0x10000);

		sCtx=src[i]>>4;
	}
	return;
}

void Model::DecompressHard(u8 *dst,u32 *size)
{
	u32 c,i;
	u32 *p;
	u32 sCtx=8;

	DecodeDirect(m_coder,*size,MaxChunkBits);
	for(i=0;i<*size;i++)
	{
		p=&probLiteral[sCtx*256];
		c=1;
		do 
		{ 
			DecodeBit(m_coder,c,p[c]);
		} while (c < 0x100);

		dst[i]=c&0xFF;
		sCtx=dst[i]>>4;
	}
	return;
}


void Model::CompressBad(u8 *src,u32 size)
{
	u32 i;
	EncodeDirect(m_coder,size,MaxChunkBits);
	for(i=0;i<size;i++)
		m_coder->EncDirect16(src[i],8);
	return;
}

void Model::DecompressBad(u8 *dst,u32 *size)
{
	u32 i;
	DecodeDirect(m_coder,*size,MaxChunkBits);
	for(i=0;i<*size;i++)
		dst[i]=(u8)m_coder->DecDirect16(8);
	return;
}

void Model::CompressRLE(u8 *src, u32 size)
{
	u32 i,j,slot,c,len,m;
	u32 sCtx=0;
	u32 *p=NULL;

	EncodeDirect(m_coder,size,MaxChunkBits);

	if (probDltO1==NULL)
	{
		probDltO1=(u32*)malloc(256*256*4);
		for (i=0;i<256*256;i++)
			probDltO1[i]=2048;
	}

	for (i=0;i<size;)
	{
		if (i>0 && size-i>3 && src[i-1]==src[i] 
		&& src[i]==src[i+1]
		&& src[i]==src[i+2])
		{
			j=i;

			j+=3;
			len=3;

			while(j<size && src[j]==src[j-1] && len<130)
			{
				len++;
				j++;
			}

			if (len>10)
			{
				sCtx=src[j-1];
				len-=10;

				EncodeBit(m_coder,1,probRLEFlag);

				for (slot=0;slot<17;slot++)
					if (matchLenBound[slot]>len) break;

				slot--;

				c=slot|0x10;

				do
				{
					EncodeBit(m_coder,(c >> 3) & 1,probRLELens[c>>4]);
					c <<= 1;
				}
				while (c < 0x100);

				if (matchLenExtraBit[slot]>0)
				{
					m=len-matchLenBound[slot];
					EncodeDirect(m_coder,m,matchLenExtraBit[slot]);
				}
				i=j;
				continue;
			}
		}

		EncodeBit(m_coder,0,probRLEFlag);

		p=&probDltO1[sCtx*256];

		c=src[i]|0x100;
		do
		{
			EncodeBit(m_coder,(c >> 7) & 1,p[c>>8]);
			c <<= 1;
		}
		while (c < 0x10000);

		sCtx=src[i];
		i++;
	}
	return;
}

void Model::DecompressRLE(u8 *dst, u32 *size)
{
	u32 c,flag,slot,len,i;
	u32 sCtx=0;
	u32 *p=NULL;

	if (probDltO1==NULL)
	{
		probDltO1=(u32*)malloc(256*256*4);
		for (i=0;i<256*256;i++)
			probDltO1[i]=2048;
	}

	DecodeDirect(m_coder,*size,MaxChunkBits);
	for (i=0;i<*size;)
	{
		flag=0;
		DecodeBit(m_coder,flag,probRLEFlag);
		if (flag==0)
		{
			p=&probDltO1[sCtx*256];
			c=1;
			do 
			{ 
				DecodeBit(m_coder,c,p[c]);
			} while (c < 0x100);
			dst[i]=c&0xFF;
			sCtx=dst[i];
			i++;
		}
		else
		{
			c=1;
			do 
			{ 
				DecodeBit(m_coder,c,probRLELens[c]);
			} while (c < 0x10);
			slot=c&0xF;

			if (matchLenExtraBit[slot]>0)
			{
				DecodeDirect(m_coder,c,matchLenExtraBit[slot]);
				len=matchLenBound[slot]+c;
			}
			else
				len=matchLenBound[slot];

			len+=10;

			while(len-->0)
			{
				dst[i]=dst[i-1];
				i++;
			}

			sCtx=dst[i-1];

		}
	}
	return;
}


void Model::CompressValue(u8 * /*src*/, u32 /*size*/,u32 /*width*/,u32 /*channelNum*/)
{
	/*u32 probSig=2048;
	u32 *probTmp=(u32*)malloc(512*256*4);
	for(u32 i=0;i<512*256;i++)
	{
		probTmp[i]=2048;
	}

	for(int i=0;i<channelNum;i++)
	{
		for(int j=i;j<size;j+=channelNum)
		{
			u32 *p1,*p2;
			if (j>channelNum)
				p1=&probTmp[src[j-channelNum]*256];
			else
				p1=&probTmp[0];
			if (j>channelNum*width)
				p2=&probTmp[(src[j-channelNum*width]+256)*256];
			else
				p2=&probTmp[256*256];

			u32 c=src[j]|0x100;
			do
			{
				EncodeBit2(m_coder,(c >> 7) & 1,p1[c>>8],p2[c>>8]);
				c <<= 1;
			}
			while (c < 0x10000);
		}
	}*/


	/*rc1=*(bufx+i-CTX1)<<9;
	rc3=*(bufx+i-CTX2)<<9|256;if (CTX1==3 && i>Lung*3) rc3=bufx[i-(Lung*3)]<<9|256;
	*(bufx+i) = nread();i+=CTX1;*/
}