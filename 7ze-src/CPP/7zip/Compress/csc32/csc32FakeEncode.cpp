#include "csc32Model.h"
#include <math.h>

#include <stdlib.h>



#define FEncodeBit(price,v,p) \
__pragma(warning(push)) \
__pragma(warning(disable:4127)) \
do\
{\
	if (v)\
		price+=probToBitNum[p>>3];\
	else\
		price+=probToBitNum[(4096-p)>>3];\
}while(0) \
__pragma(warning(pop))


u32 Model::GetLiteralPrice(u32 fstate,u32 fctx,u32 c)
{
	u32 result=0;

	FEncodeBit(result,0,probState[fstate*3+0]);

	c=c|0x100;
	u32 *p=&probLiteral[(fctx)*256];
	do
	{
		FEncodeBit(result,(c >> 7) & 1,p[c>>8]);
		c <<= 1;
	}
	while (c < 0x10000);

	return result;
}

u32 Model::Get1BMatchPrice(u32 fstate)
{
	u32 result=0;
	FEncodeBit(result,1,probState[fstate*3+0]);
	FEncodeBit(result,0,probState[fstate*3+1]);
	FEncodeBit(result,0,probState[fstate*3+2]);
	return result;
}


u32 Model::GetRepDistMatchPrice(u32 fstate,u32 repIndex,u32 matchLen)
{
	u32 result=0;
	matchLen--;

	static u32 i,j,slot,c;

	FEncodeBit(result,1,probState[fstate*3+0]);
	FEncodeBit(result,0,probState[fstate*3+1]);
	FEncodeBit(result,1,probState[fstate*3+2]);

	i=1;
	j=(repIndex>>1)&1; FEncodeBit(result,j,probRepDist[fstate*4+i]); i+=i+j;
	j=(repIndex)&1; FEncodeBit(result,j,probRepDist[fstate*4+i]); 


	if (matchLen>129)
	{
		c=15|0x10;
		do
		{
			FEncodeBit(result,(c >> 3) & 1,probLens[c>>4]);
			c <<= 1;
		}
		while (c < 0x100);

		matchLen-=129;

		while(matchLen>129)
		{
			matchLen-=129;
			FEncodeBit(result,0,probLongLen);
		}

		FEncodeBit(result,1,probLongLen);
	}

	for (slot=0;slot<17;slot++)
		if (matchLenBound[slot]>matchLen) break;

	slot--;

	c=slot|0x10;

	do
	{
		FEncodeBit(result,(c >> 3) & 1,probLens[c>>4]);
		c <<= 1;
	}
	while (c < 0x100);

	result+=matchLenExtraBit[slot]*128;

	return result;
}


u32 Model::GetMatchPrice(u32 fstate,u32 matchDist,u32 matchLen)
{
	matchLen--;//no -1 in LZ part when call this

	u32 result=0;
	static u32 i,j,slot,c;
	u32 realMatchLen=matchLen;

	FEncodeBit(result,1,probState[fstate*3+0]);
	FEncodeBit(result,1,probState[fstate*3+1]);

	if (matchLen>129)
	{
		c=15|0x10;
		do
		{
			FEncodeBit(result,(c >> 3) & 1,probLens[c>>4]);
			c <<= 1;
		}
		while (c < 0x100);

		matchLen-=129;

		while(matchLen>129)
		{
			matchLen-=129;
			FEncodeBit(result,0,probLongLen);
		}

		FEncodeBit(result,1,probLongLen);
	}

	for (slot=0;slot<17;slot++)
		if (matchLenBound[slot]>matchLen) break;

	slot--;

	c=slot|0x10;

	do
	{
		FEncodeBit(result,(c >> 3) & 1,probLens[c>>4]);
		c <<= 1;
	}
	while (c < 0x100);

	result+=matchLenExtraBit[slot]*128;

	if (realMatchLen==1)
	{
		for (slot=0;slot<9;slot++)
			if (MDistBound3[slot]>matchDist) break;

		slot--;

		c=slot|0x08;

		u32 *p=&probDist3[0];

		do
		{
			FEncodeBit(result,(c >> 2) & 1,p[c>>3]);
			c <<= 1;
		}
		while (c < 0x40);

		result+=MDistExtraBit3[slot]*128;

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
			FEncodeBit(result,(c >> 3) & 1,p[c>>4]);
			c <<= 1;
		}
		while (c < 0x100);

		result+=MDistExtraBit2[slot]*128;
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
			FEncodeBit(result,(c >> 5) & 1,p[c>>6]);
			c <<= 1;
		}
		while (c < 0x1000);

		result+=MDistExtraBit1[slot]*128;
	}
	
	return result;
}
