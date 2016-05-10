#ifndef _MODEL_H
#define _MODEL_H
#include "csc32Coder.h"
#include "csc32Common.h"


/*  Current specific 
The compression stream is made up by myriads of small data packs.
Each pack:

0-- 0 Rawbyte					Literal byte
1-- 1 1  MatchDist MatchLen		Ordinary match
disabled-(2-- 1 0 0 0 			Last match pair)
disabled-(3-- 1 0 0 1			1-Byte match with last repeat MatchDist)
3-- 1 0 0    					1-Byte match with last repeat MatchDist
4-- 1 0 1 0 0 MatchLen			A match with last repeat MatchDist(0)
5-- 1 0 1 0 1 MatchLen			A match with repeat MatchDist[1]
6-- 1 0 1 1 0 MatchLen			A match with repeat MatchDist[2]
7-- 1 0 1 1 1 MatchLen			A match with repeat MatchDist[3]

MatchDist
64 buckets with different num of extra direct bits.
probDists[64] is the statistical model.

MatchLen
16 buckets with different num of extra direct bits.
probLens[16] is the statistical model.

Rawbyte
Order-1 coder with only 3-MSBs as context
probLiteral[8][256] is the model

About state type:
pack 0			--- current type 0
pack 1			--- current type 1
pack 2,3		--- current type 2
pack 4,5,6,7	---	current type 3

The state:
stores last 4 state type.
probState is the model.
*/
enum PackType
{
	P_LITERAL,
	P_MATCH,
	P_REPDIST_MATCH,
	P_1BYTE_MATCH,
	P_REP_MATCH,
	P_INVALID
};


static u32 MDistBound1[65]=
{
	1,			2,			3,			4,
	6,			8,			10,			12,
	16,			20,			24,			28,
	36,			44,			52,			60,
	76,			92,			108,		124,
	156,		188,		252,		316,
	444,		572,		828,		1084,
	1596,		2108,		3132,		4156,
	6204,		8252,		12348,		16444,
	24636,		32828,		49212,		65596,
	98364,		131132,		196668,		262204,
	393276,		524348,		786492,		1048636,
	1572924,	2097212,	3145788,	4194364,
	6291516,	8388668,	12582972,	16777276,
	25165884,	33554492,	50331708,	67108924,
	100663356,	134217788,	201326652,	268435516,
	unsigned(-1),
};

static u32 MDistExtraBit1[64]=
{
	0,	0,	0,	1,
	1,	1,	1,	2,
	2,	2,	2,	3,
	3,	3,	3,	4,
	4,	4,	4,	5,
	5,	6,	6,	7,
	7,	8,	8,	9,
	9,	10,	10,	11,
	11,	12,	12,	13,
	13,	14,	14,	15,
	15,	16,	16,	17,
	17,	18,	18,	19,
	19,	20,	20,	21,
	21,	22,	22,	23,
	23,	24,	24,	25,
	25,	26,	26,	28,
};


static u32 matchLenBound[17]=
{
	1,		2,		3,		4,
	5,		6,		8,		10,
	14,		18,		26,		34,
	50,		66,		98,	    130,
	unsigned(-1),
};

static u32 matchLenExtraBit[16]=
{

	0,	0,	0,	0,
	0,	1,	1,	2,
	2,	3,	3,	4,
	4,	5,	5,	6,
};

static u32 MDistBound2[17]=
{
	1,		2,		4,		8,
	16,		32,		48,		64,
	96,		128,	192,	256,
	384,	512,	768,	1024,
	unsigned(-1),
};


static u32 MDistExtraBit2[16]=
{
	0,	1,	2,	3,
	4,	4,	4,	5,
	5,	6,	6,	7,
	7,	8,	8,	10,
};



static u32 MDistBound3[9]=
{
	1,		2,		3,		4,
	6,		8,		16,		32,
	unsigned(-1),
};


static u32 MDistExtraBit3[8]=
{
	0,	0,	0,	1,
	1,	3,	4,	5,
};






class Model
{
public:
	Coder *m_coder;

	void Reset(void);
	Model(void);
	~Model();
	int Init();
	void EncodeLiteral(u32 c);
	u32 DecodeLiteral()
	{
		static u32 i,tmpCtx,result;
		static u32 *p;

		tmpCtx=ctx;
		i=1;
		p=&probLiteral[tmpCtx*256];
		do 
		{ 
			DecodeBit(m_coder,i,p[i]);
		} while (i < 0x100);

		result=i&0xFF;
		ctx=result;
		state=(state*4+0)&0x3F;
		return result;
	}

	void SetLiteralCtx(u32 c) {ctx=c;}
	u32 GetLiteralCtx() {return ctx;}


	void EncodeMatch(u32 matchDist,u32 matchLen);
	void DecodeMatch(u32 &matchDist,u32 &matchLen)
	{
		static u32 v,i,slot,tmpCtx;
		static u32 *p;

		matchLen=0;
		i=1;
		do 
		{ 
			DecodeBit(m_coder,i,probLens[i]);
		} while (i < 0x10);
		slot=i&0xF;

		if (slot==15)
		{
			i=1;
			do 
			{ 
				matchLen+=129;
				DecodeBit(m_coder,i,probLongLen);
			} while ((i&0x1)==0);

			i=1;
			do 
			{ 
				DecodeBit(m_coder,i,probLens[i]);
			} while (i < 0x10);
			slot=i&0xF;
		}

		if (matchLenExtraBit[slot]>0)
		{
			DecodeDirect(m_coder,i,matchLenExtraBit[slot]);
			matchLen+=matchLenBound[slot]+i;
		}
		else
			matchLen+=matchLenBound[slot];


		if (matchLen==1)
		{
			i=1;
			p=&probDist3[0];
			do 
			{ 
				DecodeBit(m_coder,i,p[i]);
			} while (i < 0x08);
			slot=i&0x7;

			if (MDistExtraBit3[slot]>0)
			{
				DecodeDirect(m_coder,i,MDistExtraBit3[slot]);
				matchDist=MDistBound3[slot]+i;
			}
			else
				matchDist=MDistBound3[slot];
		}
		else if (matchLen==2)
		{
			i=1;
			p=&probDist2[0];
			do 
			{ 
				DecodeBit(m_coder,i,p[i]);
			} while (i < 0x10);
			slot=i&0xF;

			if (MDistExtraBit2[slot]>0)
			{
				DecodeDirect(m_coder,i,MDistExtraBit2[slot]);
				matchDist=MDistBound2[slot]+i;
			}
			else
				matchDist=MDistBound2[slot];
		}
		else
		{
			u32 lenCtx=(matchLen)>5?3:(matchLen)-3;
			p=&probDist1[lenCtx*64];
			i=1;
			do 
			{ 
				DecodeBit(m_coder,i,p[i]);
			} while (i < 0x40);
			slot=i&0x3F;

			if (MDistExtraBit1[slot]>0)
			{
				DecodeDirect(m_coder,i,MDistExtraBit1[slot]);
				matchDist=MDistBound1[slot]+i;
			}
			else
				matchDist=MDistBound1[slot];
		}


		state=(state*4+1)&0x3F;
		return;
	}


	void EncodeRepDistMatch(u32 repIndex,u32 matchLen);
	void DecodeRepDistMatch(u32 &repIndex,u32 &matchLen)
	{
		static u32 v,i,slot,tmpCtx;
		static u32 *p;

		matchLen=0;
		i=1;
		do 
		{ 
			DecodeBit(m_coder,i,probRepDist[state*4+i]);
		} while (i < 0x4);
		repIndex=i&0x3;

		i=1;
		do 
		{ 
			DecodeBit(m_coder,i,probLens[i]);
		} while (i < 0x10);
		slot=i&0xF;

		if (slot==15)
		{
			i=1;
			do 
			{ 
				matchLen+=129;
				DecodeBit(m_coder,i,probLongLen);
			} while ((i&0x1)==0);

			i=1;
			do 
			{ 
				DecodeBit(m_coder,i,probLens[i]);
			} while (i < 0x10);
			slot=i&0xF;
		}

		if (matchLenExtraBit[slot]>0)
		{
			DecodeDirect(m_coder,i,matchLenExtraBit[slot]);
			matchLen+=matchLenBound[slot]+i;
		}
		else
			matchLen+=matchLenBound[slot];


		state=(state*4+3)&0x3F;
	}


	void Encode1BMatch(void);
	void Decode1BMatch(void)
	{
		state=(state*4+2)&0x3F;
		ctx=16;
	}

	void CompressDelta(u8 *src,u32 size);
	//void DecompressDelta(u8 *dst,u32 *size);

	void CompressHard(u8 *src,u32 size);
	void DecompressHard(u8 *dst,u32 *size);

	void CompressBad(u8 *src,u32 size);
	void DecompressBad(u8 *dst,u32 *size);

	void CompressRLE(u8 *src,u32 size);
	void DecompressRLE(u8 *dst,u32 *size);

	void CompressValue(u8 *src, u32 size,u32 width,u32 channelNum);

	void EncodeInt(u32 num,u32 bits);
	u32 DecodeInt(u32 bits);


	//void DecodePack(PackType *type,u32 *a,u32 *b);
	u32 probState[4*4*4*3];//Original [64][3]
	u32 state;

	//Fake Encode --- to get price
	u32 GetLiteralPrice(u32 fstate,u32 fctx,u32 c);
	u32 Get1BMatchPrice(u32 fstate);
	u32 GetRepDistMatchPrice(u32 fstate,u32 repIndex,u32 matchLen);
	u32 GetMatchPrice(u32 fstate,u32 matchDist,u32 matchLen);
	//Fake Encode --- to get price
private:
	u32 probRLEFlag;
	u32 probRLELens[16];
	u32 *probLiteral;//Original [17][256]

	u32 *probDltO1;

	u32 probRepDist[64*4];
	u32 probDist1[4*64];
	u32 probDist2[16];
	u32 probDist3[8];

	u32 probLens[16];
	u32 probLongLen;
	u32 probToBitNum[4096>>3];

	u32 ctx;
};


#endif
