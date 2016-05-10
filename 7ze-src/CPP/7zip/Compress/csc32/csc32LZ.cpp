#include "csc32Model.h"
#include "csc32LZ.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "csc32Coder.h"
#include "csc32LZCommon.h"



int LZ::Init(u32 WindowSize,u32 operation)
{
	return Init(WindowSize,operation,22,1);
}


int LZ::Init(u32 WindowSize,u32 operation,u32 hashBits,u32 hashWidth)
{
	m_operation=operation;
	m_WndSize=WindowSize;

	if(m_WndSize<32*KB)
		m_WndSize=32*KB;
	if(m_WndSize>MaxDictSize)
		m_WndSize=MaxDictSize;


	m_Wnd=(u8*)malloc(WindowSize+8);
	if (m_Wnd==NULL)
		goto FREE_ON_ERROR;

	if (operation==ENCODE)
	{
			m_HT2=(u32*)malloc(sizeof(u32)*64*KB);
			if (m_HT2==NULL)
				goto FREE_ON_ERROR;
			m_HT3=(u32*)malloc(sizeof(u32)*64*KB);
			if (m_HT3==NULL)
				goto FREE_ON_ERROR;

		m_Hash6Bits=hashBits;
		m_HashNum=hashWidth;

		m_HT6Raw=(u32*)malloc(sizeof(u32)*m_HashNum*((u32)(1<<m_Hash6Bits))+256);
		if (m_HT6Raw==NULL)
			goto FREE_ON_ERROR;
		m_HT6=(u32*)(m_HT6Raw+(64-(u32)(m_HT6Raw)%64));

		parser=(ParserAtom*)malloc(sizeof(ParserAtom)*(MinBlockSize+1));
		matchList=(MatchAtom*)malloc(sizeof(MatchAtom)*(2+(MinBlockSize<<3)));
		if (parser==NULL || matchList==NULL)
			goto FREE_ON_ERROR;
	}
	Reset();
	//memset(m_Wnd,0,m_WndSize);
	return NO_ERROR;

FREE_ON_ERROR:
	SAFEFREE(m_HT2);
	SAFEFREE(m_HT3);
	SAFEFREE(m_HT6Raw);
	SAFEFREE(parser);
	SAFEFREE(matchList);
	SAFEFREE(m_Wnd);
	return CANT_ALLOC_MEM;
	
}


void LZ::Reset(void)
{
	m_WndCurrPos=0;

	m_RepDist[0]=
		m_RepDist[1]=
		m_RepDist[2]=
		m_RepDist[3]=0;
	m_RepMatchLen=0;

	parsing[0].total=
		parsing[1].total=1;
	parsing[0].candidate[0].type=
		parsing[1].candidate[0].type=P_LITERAL;
	parsing[0].candidate[0].price=
		parsing[1].candidate[0].price=0;
	parsing[0].candidate[0].len=
		parsing[1].candidate[0].len=1;

	m_PassedCount=1024;
	m_LitCount=400;
	m_model->Reset();
}


void LZ::Destroy(void)
{
	if (m_operation==ENCODE)
	{
		SAFEFREE(m_HT2);
		SAFEFREE(m_HT3);
		SAFEFREE(m_HT6Raw);
		SAFEFREE(parser);
		SAFEFREE(matchList);
	}
	SAFEFREE(m_Wnd);
}




int LZ::Decode(u8 *dst,u32 *size,u32 sizeLimit)
{
// 	u32 groupNum=0;
// 	u32 progress=0;
	u32 i=0,j,k,v;
	u32 a,b,cpyPos;
	u32 dstInWndPos;
	u8 *wndDst,*wndCpy;
	u32 lastCopySize;
	bool notEnd;
// 	PackType x;

	notEnd=true;
	dstInWndPos=m_WndCurrPos;
	lastCopySize=0;

	while(notEnd && i<=sizeLimit)
	{
		v=0;
		DecodeBit(m_model->m_coder,v,m_model->probState[m_model->state*3+0]);
		if (v==0) 
		{
			m_Wnd[m_WndCurrPos++]=(u8)m_model->DecodeLiteral();
			i++;
		}
		else
		{
			v=0;
			DecodeBit(m_model->m_coder,v,m_model->probState[m_model->state*3+1]);
			if (v==1) 
			{
				m_model->DecodeMatch(a,b);
				if (b==2&&a==2047) 
				{
					//printf("\n%u %u\n",m_WndCurrPos,i);
					notEnd=false;
					break;
				}
				b+=1;
				m_RepDist[3]=m_RepDist[2];
				m_RepDist[2]=m_RepDist[1];
				m_RepDist[1]=m_RepDist[0];
				m_RepDist[0]=a;
				cpyPos=m_WndCurrPos>=a?m_WndCurrPos-a:m_WndCurrPos+m_WndSize-a;
				if (cpyPos>m_WndSize || cpyPos+b>m_WndSize || b+i>sizeLimit)
				{
					return DECODE_ERROR;
				}
				wndDst=m_Wnd+m_WndCurrPos;
				wndCpy=m_Wnd+cpyPos;
				i+=b;
				m_WndCurrPos+=b;
				while(b--) *wndDst++=*wndCpy++;
				m_model->SetLiteralCtx(m_Wnd[m_WndCurrPos-1]);
			}
			else
			{
				v=0;
				DecodeBit(m_model->m_coder,v,m_model->probState[m_model->state*3+2]);
				if (v==0) 
				{
					m_model->Decode1BMatch();
					cpyPos=m_WndCurrPos>m_RepDist[0]?
						m_WndCurrPos-m_RepDist[0]:m_WndCurrPos+m_WndSize-m_RepDist[0];
					m_Wnd[m_WndCurrPos++]=m_Wnd[cpyPos];
					i++;
					m_model->SetLiteralCtx(m_Wnd[m_WndCurrPos-1]);

				}
				else 
				{
					m_model->DecodeRepDistMatch(a,b);
					b+=1;
					if (b+i>sizeLimit) 
					{
						return DECODE_ERROR;
					}
					k=m_RepDist[a];
					for(j=a;j>0;j--) 
						m_RepDist[j]=m_RepDist[j-1];
					m_RepDist[0]=k;
					cpyPos=m_WndCurrPos>k?m_WndCurrPos-k:m_WndCurrPos+m_WndSize-k;
					if (cpyPos+b>m_WndSize)
					{
						return DECODE_ERROR;
					}
					wndDst=m_Wnd+m_WndCurrPos;
					wndCpy=m_Wnd+cpyPos;
					i+=b;
					m_WndCurrPos+=b;
					while(b--) *wndDst++=*wndCpy++;
					m_model->SetLiteralCtx(m_Wnd[m_WndCurrPos-1]);
				}
			}
		}


		if (m_WndCurrPos>=m_WndSize) 
		{
			m_WndCurrPos=0;
			memcpy(dst+lastCopySize,m_Wnd+dstInWndPos,i-lastCopySize);
			dstInWndPos=0;
			lastCopySize=i;
		}
	}
	*size=i;
	memcpy(dst+lastCopySize,m_Wnd+dstInWndPos,i-lastCopySize);
	return NO_ERROR;
}

void LZ::EncodeNormal(u8 *src,u32 size,u32 lzMode)
{
	u32 progress=0;
	u32 currBlockSize;

	while (progress<size)
	{
		currBlockSize=MIN(m_WndSize-m_WndCurrPos,size-progress);
		currBlockSize=MIN(currBlockSize,MinBlockSize);

		memcpy(m_Wnd+m_WndCurrPos,src+progress,currBlockSize);
		if (lzMode==1)
			LZMinBlock(currBlockSize);
		else if (lzMode==2)
			LZMinBlockNew(currBlockSize,2,3,100000);
		else if (lzMode==3)
			LZMinBlockNew(currBlockSize,10,6,100000);
		else if (lzMode==0)
			LZMinBlockFast(currBlockSize);
		else if (lzMode==4)
			LZMinBlockNew(currBlockSize,24,16,100000);
			

		if (m_WndCurrPos>=m_WndSize) m_WndCurrPos=0;
		progress+=currBlockSize;
	}
	m_model->EncodeMatch(2047,2);

	return;
}


u32 LZ::CheckDuplicate(u8 *src,u32 size,u32 type)
{
	u32 lastWndPos=m_WndCurrPos;
	u32 progress=0;
	u32 currBlockSize;

	while (progress<size)
	{
		currBlockSize=MIN(m_WndSize-m_WndCurrPos,size-progress);
		currBlockSize=MIN(currBlockSize,MinBlockSize);

		memcpy(m_Wnd+m_WndCurrPos,src+progress,currBlockSize);
		if (LZMinBlockSkip(currBlockSize,type)==DT_NORMAL)
		{
			m_WndCurrPos=lastWndPos;
			return DT_NORMAL;
		}

		m_WndCurrPos+=currBlockSize;
		if (m_WndCurrPos>=m_WndSize) m_WndCurrPos=0;
		progress+=currBlockSize;
	}

	return DT_SKIP;
}

void LZ::DuplicateInsert(u8 *src,u32 size)
{
// 	u32 lastWndPos=m_WndCurrPos;
	u32 progress=0;
	u32 currBlockSize;

	while (progress<size)
	{
		currBlockSize=MIN(m_WndSize-m_WndCurrPos,size-progress);
		currBlockSize=MIN(currBlockSize,MinBlockSize);

		memcpy(m_Wnd+m_WndCurrPos,src+progress,currBlockSize);

		m_WndCurrPos+=currBlockSize;
		if (m_WndCurrPos>=m_WndSize) m_WndCurrPos=0;
		progress+=currBlockSize;
	}
	return;
}

inline void LZ::InsertHash(u32 currHash3, u32 currHash6, u32 len)
{
	u32	*HT6=&m_HT6[currHash6*m_HashNum];
	u32	*HT3=&m_HT3[currHash3];

	u32 lastCurrHash6;
	u32 i;

	
	HT3[0]=CURPOS;
	m_HT2[*(u16*)(m_Wnd+m_WndCurrPos)]=CURPOS;

	for(i=m_HashNum-1;i>0;i--)
		HT6[i]=HT6[i-1];

	HT6[0]=CURPOS;
	len--;
	m_WndCurrPos++;

	lastCurrHash6=currHash6;

	while(len>0)
	{
		currHash6=HASH6(m_Wnd[m_WndCurrPos]);
		currHash3=HASH3(m_Wnd[m_WndCurrPos]);
		HT6=&m_HT6[currHash6*m_HashNum];
		HT3=&m_HT3[currHash3];

		m_HT2[*(u16*)(m_Wnd+m_WndCurrPos)]=CURPOS;

		if (lastCurrHash6!=currHash6)
		{
			for(i=m_HashNum-1;i>0;i--)
				HT6[i]=HT6[i-1];
		}
		HT6[0]=CURPOS;
		HT3[0]=CURPOS;

		if (len>256)
		{
			len-=4;
			m_WndCurrPos+=4;
		}
		len--;
		m_WndCurrPos++;	
		lastCurrHash6=currHash6;
	}
}


inline void LZ::InsertHashFast(u32 currHash6, u32 len)
{
	u32	*HT6=&m_HT6[currHash6*m_HashNum];

	HT6[0]=CURPOS;
	len--;
	m_WndCurrPos++;

	while(len>0)
	{
		currHash6=HASH6(m_Wnd[m_WndCurrPos]);
		HT6=&m_HT6[currHash6*m_HashNum];
		HT6[0]=CURPOS;
		if (len>256)
		{
			len-=4;
			m_WndCurrPos+=4;
		}
		len--;
		m_WndCurrPos++;	
	}
}


u32 LZ::LZMinBlockSkip(u32 size,u32 type)
{

  u32 currHash6;// , currHash3;
	u32 i,j,cmpPos1,cmpPos2,cmpLen,remainLen,remainLen2;
	u32 matchDist;
	u32 minMatchLen;
	u32 *HT6;


	const u32 currBlockEndPos=m_WndCurrPos+size;
	const u32 currBlockStartPos=m_WndCurrPos;

	minMatchLen=70;
	remainLen=size;

	for(i=0;i<4;i++)
	{
		cmpPos1=m_WndCurrPos>m_RepDist[i]?m_WndCurrPos-m_RepDist[i]:m_WndCurrPos+m_WndSize-m_RepDist[i];
		if ((cmpPos1<m_WndCurrPos || cmpPos1>currBlockEndPos) && (cmpPos1<m_WndSize) )
		{
			cmpPos2=m_WndCurrPos;
			remainLen2=MIN(remainLen,m_WndSize-cmpPos1);
			remainLen2=MIN(remainLen2,72);

			if (*(u16*)&m_Wnd[cmpPos1]!=*(u16*)&m_Wnd[cmpPos2]) continue;
			if ((remainLen2<minMatchLen)||
				(m_Wnd[cmpPos1+minMatchLen+1]!=m_Wnd[cmpPos2+minMatchLen+1])||
				(m_Wnd[cmpPos1+(minMatchLen>>1)]!=m_Wnd[cmpPos2+(minMatchLen>>1)])
				)
				continue;

			cmpPos1+=2;
			cmpPos2+=2;
			cmpLen=2;

			if (remainLen2>3)
				while ((cmpLen<remainLen2-3)&&(*(u32*)(m_Wnd+cmpPos1)==*(u32*)(m_Wnd+cmpPos2)))
				{cmpPos1+=4;cmpPos2+=4;cmpLen+=4;}
				while((cmpLen<remainLen2)&&(m_Wnd[cmpPos1++]==m_Wnd[cmpPos2++])) cmpLen++;
				if (cmpLen>minMatchLen)
				{
					return DT_NORMAL;
				}
		}
	}

	for(i=0;i<MIN(512,size);i++)
	{
		currHash6=HASH6(m_Wnd[m_WndCurrPos+i]);
		remainLen=size-i;


		cmpPos1=m_HT6[currHash6*m_HashNum]&0x1FFFFFFF;

		if ((cmpPos1<m_WndCurrPos || cmpPos1>currBlockEndPos) 
			&& (cmpPos1<m_WndSize) 
			&& ((m_HT6[currHash6*m_HashNum]>>29)==(((u32)m_Wnd[m_WndCurrPos+i]&0x0E)>>1)))
		{
			matchDist=m_WndCurrPos+i>cmpPos1?
				m_WndCurrPos+i-cmpPos1:m_WndCurrPos+i+m_WndSize-cmpPos1;
			cmpPos2=m_WndCurrPos+i;
			remainLen2=MIN(remainLen,m_WndSize-cmpPos1);
			remainLen2=MIN(remainLen2,72);

			if (*(u32*)&m_Wnd[cmpPos1]==*(u32*)&m_Wnd[cmpPos2])
			{
				if ((remainLen2<minMatchLen)||
					(m_Wnd[cmpPos1+minMatchLen+1]!=m_Wnd[cmpPos2+minMatchLen+1])||
					(m_Wnd[cmpPos1+(minMatchLen>>1)]!=m_Wnd[cmpPos2+(minMatchLen>>1)])
					)
					continue;
				cmpPos1+=4;
				cmpPos2+=4;
				cmpLen=4;

				if (remainLen2>3)
					while ((cmpLen<remainLen2-3)&&(*(u32*)(m_Wnd+cmpPos1)==*(u32*)(m_Wnd+cmpPos2)))
					{cmpPos1+=4;cmpPos2+=4;cmpLen+=4;}
					while((cmpLen<remainLen2)&&(m_Wnd[cmpPos1++]==m_Wnd[cmpPos2++])) cmpLen++;

					if (cmpLen>=minMatchLen)
					{
						return DT_NORMAL;
					}
			}
		}
	}

	if (type==DT_BAD)
	{
		for (j=0;j<size;j+=5)
		{
			currHash6=HASH6(m_Wnd[currBlockStartPos+j]);
			HT6=&m_HT6[currHash6*m_HashNum];
			for(i=m_HashNum-1;i>0;i--)
				HT6[i]=HT6[i-1];
			HT6[0]=(currBlockStartPos+j)|((m_Wnd[(currBlockStartPos+j)]&0x0E)<<28);
		}
	}
	else
	{
		for (j=0;j<size;j+=2)
		{
			//currHash3=HASH3(m_Wnd[currBlockStartPos+j]);
			currHash6=HASH6(m_Wnd[currBlockStartPos+j]);
			HT6=&m_HT6[currHash6*m_HashNum];
			//for(i=m_HashNum-1;i>0;i--)
			//	HT6[i]=HT6[i-1];
			HT6[0]=(currBlockStartPos+j)|((m_Wnd[(currBlockStartPos+j)]&0x0E)<<28);
			//m_HT3[currHash3]=(currBlockStartPos+j)|((m_Wnd[(currBlockStartPos+j)]&0x0E)<<28);
		}
	}

	return DT_SKIP;
}



int LZ::LZMinBlock(u32 size)
{
	u32 currHash6,currHash3,currHash2;
	u32 progress=0;

	u32 matchDist,lastHashFDist;
	u32 minMatchLen;
	u32 *HT6,*HT3,*HT2;

	u32 bestCandidate0=0,bestCandidate1=0;
	bool outPrevMatch;
	u8 outPrevChar=0;
	u32 lastDist;
	u32 distExtra;

	u32 i,j,cmpPos1,cmpPos2,cmpLen,remainLen,remainLen2;
	bool gotMatch;

	currBlockEndPos=m_WndCurrPos+size;

	m_ParsingPos=0;

	while(progress<size)
	{
		currHash6=HASH6(m_Wnd[m_WndCurrPos]);
		currHash3=HASH3(m_Wnd[m_WndCurrPos]);
		currHash2=HASH2(m_Wnd[m_WndCurrPos]);

		parsing[m_ParsingPos].total=1;
		parsing[m_ParsingPos].candidate[0].price=0;

		lastHashFDist=0xFFFFFFFF;
		gotMatch=false;

		if (m_ParsingPos==0)
		{
			minMatchLen=1;
			remainLen=size-progress;
		}
		else
		{
			minMatchLen=parsing[0].candidate[bestCandidate0].len;
			remainLen=size-progress;
			if (remainLen>0)
				remainLen--;

			if (parsing[0].candidate[bestCandidate0].type!=P_MATCH&&minMatchLen>5)
				minMatchLen-=1;
		}

		if (minMatchLen<1)
			minMatchLen=1;

		if (++m_PassedCount==2048) 
		{
			m_PassedCount>>=1;
			m_LitCount>>=1;
		}

		if (m_LitCount>0.9*m_PassedCount) goto HASH6SEARCH;

		if (remainLen>=MIN_LEN_HT2)
		for(i=0;i<4;i++)
		{
			cmpPos1=m_WndCurrPos>m_RepDist[i]?m_WndCurrPos-m_RepDist[i]:m_WndCurrPos+m_WndSize-m_RepDist[i];
			if ((cmpPos1<m_WndCurrPos || cmpPos1>currBlockEndPos) && (cmpPos1<m_WndSize))
			{
				cmpPos2=m_WndCurrPos;
				remainLen2=MIN(remainLen,m_WndSize-cmpPos1);

				if (*(u16*)&m_Wnd[cmpPos1]!=*(u16*)&m_Wnd[cmpPos2]) continue;
				if ((remainLen2<=minMatchLen)||
					(m_Wnd[cmpPos1+minMatchLen]!=m_Wnd[cmpPos2+minMatchLen])||
					(m_Wnd[cmpPos1+(minMatchLen>>1)]!=m_Wnd[cmpPos2+(minMatchLen>>1)])
					)
					continue;

				cmpPos1+=2;
				cmpPos2+=2;
				cmpLen=2;

				if (remainLen2>3)
					while ((cmpLen<remainLen2-3)&&(*(u32*)(m_Wnd+cmpPos1)==*(u32*)(m_Wnd+cmpPos2)))
					{cmpPos1+=4;cmpPos2+=4;cmpLen+=4;}
					while((cmpLen<remainLen2)&&(m_Wnd[cmpPos1++]==m_Wnd[cmpPos2++])) cmpLen++;
					if (cmpLen>minMatchLen)
					{
						minMatchLen=cmpLen;
						parsing[m_ParsingPos].candidate[parsing[m_ParsingPos].total].type=P_REPDIST_MATCH;
						parsing[m_ParsingPos].candidate[parsing[m_ParsingPos].total].len=cmpLen;
						parsing[m_ParsingPos].candidate[parsing[m_ParsingPos].total].price=cmpLen*8+3;
						parsing[m_ParsingPos].candidate[parsing[m_ParsingPos].total].dist=i;
						parsing[m_ParsingPos].total++;
						if (minMatchLen>GOOD_LEN_REP)
						{
							gotMatch=true;
							break;
						}
						if (m_RepDist[i]<lastHashFDist)
							lastHashFDist=m_RepDist[i];
					}
			}
		}

		if (gotMatch) goto DETERMINE;

		cmpPos1=m_WndCurrPos>m_RepDist[0]?m_WndCurrPos-m_RepDist[0]:m_WndCurrPos+m_WndSize-m_RepDist[0];

		if ((minMatchLen<3) && (cmpPos1<m_WndCurrPos || cmpPos1>currBlockEndPos)
			&& (cmpPos1<m_WndSize) && (m_Wnd[cmpPos1]==m_Wnd[m_WndCurrPos]) )
		{
			parsing[m_ParsingPos].candidate[parsing[m_ParsingPos].total].type=P_1BYTE_MATCH;
			parsing[m_ParsingPos].candidate[parsing[m_ParsingPos].total].len=1;
			parsing[m_ParsingPos].candidate[parsing[m_ParsingPos].total].price=48;
			parsing[m_ParsingPos].total++;
		}


HASH6SEARCH:

		HT6=&m_HT6[currHash6*m_HashNum];
		lastDist=1;
		distExtra=0;
		if (remainLen>=MIN_LEN_HT6)
		for(i=0;i<m_HashNum;i++)
		{
			cmpPos1=HT6[i]&0x1FFFFFFF;
			if ((cmpPos1<m_WndCurrPos || cmpPos1>currBlockEndPos) &&
        (cmpPos1<m_WndSize) &&
        ((HT6[i]>>29)==(((u32)m_Wnd[m_WndCurrPos]&0x0E)>>1)))
			{
				matchDist=m_WndCurrPos>cmpPos1?m_WndCurrPos-cmpPos1:m_WndCurrPos+m_WndSize-cmpPos1;
				if (matchDist<lastDist) break;
				lastDist=matchDist;

				cmpPos2=m_WndCurrPos;
				remainLen2=MIN(remainLen,m_WndSize-cmpPos1);

				if (*(u32*)&m_Wnd[cmpPos1]!=*(u32*)&m_Wnd[cmpPos2]) continue;
				if ((remainLen2<=minMatchLen)||
					(m_Wnd[cmpPos1+minMatchLen]!=m_Wnd[cmpPos2+minMatchLen])||
					(m_Wnd[cmpPos1+(minMatchLen>>1)]!=m_Wnd[cmpPos2+(minMatchLen>>1)])
					)
					continue;

				cmpPos1+=4;
				cmpPos2+=4;
				cmpLen=4;

				if (remainLen2>3)
					while ((cmpLen<remainLen2-3)&&(*(u32*)(m_Wnd+cmpPos1)==*(u32*)(m_Wnd+cmpPos2)))
					{cmpPos1+=4;cmpPos2+=4;cmpLen+=4;}
					while((cmpLen<remainLen2)&&(m_Wnd[cmpPos1++]==m_Wnd[cmpPos2++])) cmpLen++;

					if	( (cmpLen>minMatchLen) &&
						(
						   ((cmpLen==MIN_LEN_HT6)&&(matchDist<(1<<19)))
						|| ((cmpLen==MIN_LEN_HT6+1)&&(matchDist<(1<<22)))
						|| (cmpLen>MIN_LEN_HT6+1)
						)
						)
					{ 
						u32 countExtra=(matchDist>>7);
						distExtra=0;
						while(countExtra>0)
						{
							countExtra>>=2;
							distExtra+=3;
						}
						parsing[m_ParsingPos].candidate[parsing[m_ParsingPos].total].type=P_MATCH;
						parsing[m_ParsingPos].candidate[parsing[m_ParsingPos].total].len=cmpLen;
						parsing[m_ParsingPos].candidate[parsing[m_ParsingPos].total].dist=matchDist;
						parsing[m_ParsingPos].candidate[parsing[m_ParsingPos].total].price=cmpLen*8-14-distExtra;
						parsing[m_ParsingPos].total++;
						minMatchLen=cmpLen;
						if (cmpLen>GOOD_LEN_HT6)
						{
							gotMatch=true;
							break;
						}
						if (matchDist<lastHashFDist)
							lastHashFDist=matchDist;
					}
			}
		}

		if (parsing[0].total>1) goto DETERMINE;
		if (m_LitCount>0.8*m_PassedCount) goto DETERMINE;

		minMatchLen=1;
		HT3=&m_HT3[currHash3];
		//lastDist=1;

		if (remainLen>=MIN_LEN_HT3)
		for(i=0;i<1;i++)
		{
			cmpPos1=HT3[i]&0x1FFFFFFF;
			if ((cmpPos1<m_WndCurrPos || cmpPos1>currBlockEndPos) &&
        (cmpPos1<m_WndSize) &&
        ((HT3[i]>>29)==(((u32)m_Wnd[m_WndCurrPos]&0x0E)>>1)))
			{
				cmpPos2=m_WndCurrPos;
				remainLen2=MIN(remainLen,m_WndSize-cmpPos1);
				matchDist=m_WndCurrPos>cmpPos1?m_WndCurrPos-cmpPos1:m_WndCurrPos+m_WndSize-cmpPos1;
				//if (matchDist<lastDist) break;
				if (matchDist>=lastHashFDist)
					break;
				lastDist=matchDist;

				if (*(u16*)&m_Wnd[cmpPos1]!=*(u16*)&m_Wnd[cmpPos2]) continue;
				if ((remainLen2<=minMatchLen)||
					(m_Wnd[cmpPos1+minMatchLen]!=m_Wnd[cmpPos2+minMatchLen])||
					(m_Wnd[cmpPos1+(minMatchLen>>1)]!=m_Wnd[cmpPos2+(minMatchLen>>1)])
					)
					continue;

				cmpPos1+=2;
				cmpPos2+=2;
				cmpLen=2;

				if (remainLen2>3)
					while ((cmpLen<remainLen2-3)&&(*(u32*)(m_Wnd+cmpPos1)==*(u32*)(m_Wnd+cmpPos2)))
					{cmpPos1+=4;cmpPos2+=4;cmpLen+=4;}
					while((cmpLen<remainLen2)&&(m_Wnd[cmpPos1++]==m_Wnd[cmpPos2++])) cmpLen++;


					if ( (cmpLen>minMatchLen) && 
						(
						   ((cmpLen==MIN_LEN_HT2+1)&&(matchDist<(1<<9)))
						|| ((cmpLen==MIN_LEN_HT2+2)&&(matchDist<(1<<12)))
						|| ((cmpLen==MIN_LEN_HT2+3)&&(matchDist<(1<<16)))
						//|| ((cmpLen==MIN_LEN_HT6)&&(matchDist<(1<<18))

						)
						)
					{
						parsing[m_ParsingPos].candidate[parsing[m_ParsingPos].total].type=P_MATCH;
						parsing[m_ParsingPos].candidate[parsing[m_ParsingPos].total].len=cmpLen;
						parsing[m_ParsingPos].candidate[parsing[m_ParsingPos].total].dist=matchDist;
						parsing[m_ParsingPos].candidate[parsing[m_ParsingPos].total].price=cmpLen*8-13+4*(matchDist<64);
						parsing[m_ParsingPos].total++;
						minMatchLen=cmpLen;
						lastHashFDist=matchDist;
					}
			}
		}

		//if (parsing[0].total>1) goto DETERMINE;
		if (m_LitCount>0.8*m_PassedCount) goto DETERMINE;


		minMatchLen=1;
		HT2=&m_HT2[currHash2];
		//lastDist=1;
		if (remainLen>=MIN_LEN_HT2)
		for(i=0;i<1;i++)
		{
			cmpPos1=HT2[i]&0x1FFFFFFF;
			if ((cmpPos1<m_WndCurrPos || cmpPos1>currBlockEndPos) &&
        (cmpPos1<m_WndSize) &&
        ((HT2[i]>>29)==(((u32)m_Wnd[m_WndCurrPos]&0x0E)>>1)))
			{
				cmpPos2=m_WndCurrPos;
				remainLen2=MIN(remainLen,m_WndSize-cmpPos1);
				matchDist=m_WndCurrPos>cmpPos1?m_WndCurrPos-cmpPos1:m_WndCurrPos+m_WndSize-cmpPos1;
				//if (matchDist<lastDist) break;
				//lastDist=matchDist;
				if (matchDist>=lastHashFDist)
					break;

				if (*(u16*)&m_Wnd[cmpPos1]!=*(u16*)&m_Wnd[cmpPos2]) continue;
				if ((remainLen2<=minMatchLen)||
					(m_Wnd[cmpPos1+minMatchLen]!=m_Wnd[cmpPos2+minMatchLen])||
					(m_Wnd[cmpPos1+(minMatchLen>>1)]!=m_Wnd[cmpPos2+(minMatchLen>>1)])
					)
					continue;

				cmpPos1+=2;
				cmpPos2+=2;
				cmpLen=2;

				if (remainLen2>3)
					while ((cmpLen<remainLen2-3)&&(*(u32*)(m_Wnd+cmpPos1)==*(u32*)(m_Wnd+cmpPos2)))
					{cmpPos1+=4;cmpPos2+=4;cmpLen+=4;}
					while((cmpLen<remainLen2)&&(m_Wnd[cmpPos1++]==m_Wnd[cmpPos2++])) cmpLen++;


					if ( (cmpLen>minMatchLen) &&
						((
						(cmpLen==MIN_LEN_HT2)&&(matchDist<(1<<6)))
						)
						)
					{
						parsing[m_ParsingPos].candidate[parsing[m_ParsingPos].total].type=P_MATCH;
						parsing[m_ParsingPos].candidate[parsing[m_ParsingPos].total].len=cmpLen;
						parsing[m_ParsingPos].candidate[parsing[m_ParsingPos].total].dist=matchDist;
						parsing[m_ParsingPos].candidate[parsing[m_ParsingPos].total].price=cmpLen*8-10;
						parsing[m_ParsingPos].total++;
						minMatchLen=cmpLen;
						lastHashFDist=matchDist;
					}
			}
		}




DETERMINE:

		if (parsing[0].total==1) m_LitCount++;

		outPrevMatch=false;


		if (m_ParsingPos==0)
		{
			bestCandidate0=0;
			for (i=1;i<parsing[0].total;i++)
			{
				if (parsing[0].candidate[i].price>parsing[0].candidate[bestCandidate0].price)
					bestCandidate0=i;
			}


			if (parsing[0].candidate[bestCandidate0].len+progress>=size)
			{
				if (parsing[0].candidate[bestCandidate0].len+progress>size)
				{
					parsing[0].candidate[bestCandidate0].len=size-progress;
				}
				goto OUTCANDIDATE;
			}

			if (parsing[0].candidate[bestCandidate0].price>230)
			{
				goto OUTCANDIDATE;
			}

			if (bestCandidate0>0)
			{
				outPrevChar=m_Wnd[m_WndCurrPos];
				InsertHash(currHash3,currHash6,1);
				m_ParsingPos=1;
				continue;
			}
			else
				goto OUTCANDIDATE;
		}
		else
		{
			bestCandidate1=0;

			for (i=1;i<parsing[1].total;i++)
			{
				if (parsing[1].candidate[i].price>parsing[1].candidate[bestCandidate1].price)
					bestCandidate1=i;
			}

			if (parsing[1].candidate[bestCandidate1].len+progress+1>=size)
			{
				m_model->EncodeLiteral(outPrevChar);//,m_WndCurrPos-1
				progress++;

				if (parsing[1].candidate[bestCandidate1].len+progress>size)
				{
					parsing[1].candidate[bestCandidate1].len=size-progress;
				}
				parsing[0]=parsing[1];
				bestCandidate0=bestCandidate1;
				m_ParsingPos=0;
				goto OUTCANDIDATE;
			}

			if (parsing[1].candidate[bestCandidate1].price>230)
			{
				m_model->EncodeLiteral(outPrevChar);//,m_WndCurrPos-1
				progress++;
				parsing[0]=parsing[1];
				bestCandidate0=bestCandidate1;
				m_ParsingPos=0;
				goto OUTCANDIDATE;
			}

			if (parsing[1].candidate[bestCandidate1].price>parsing[0].candidate[bestCandidate0].price+8)
			{
				m_model->EncodeLiteral(outPrevChar);//,m_WndCurrPos-1
				progress++;
				parsing[0]=parsing[1];
				bestCandidate0=bestCandidate1;

				outPrevChar=m_Wnd[m_WndCurrPos];
				InsertHash(currHash3,currHash6,1);
				continue;
			}
			else
			{
				m_ParsingPos=0;
				outPrevMatch=true;
				goto OUTCANDIDATE;
			}
		}


OUTCANDIDATE:
		switch (parsing[0].candidate[bestCandidate0].type)
		{
		case P_LITERAL:
			m_model->EncodeLiteral(m_Wnd[m_WndCurrPos]);//,m_WndCurrPos
			InsertHash(currHash3,currHash6,1);
			progress++;
			break;
		case P_MATCH:
			m_model->EncodeMatch(parsing[0].candidate[bestCandidate0].dist,parsing[0].candidate[bestCandidate0].len-1);
			m_RepDist[3]=m_RepDist[2];
			m_RepDist[2]=m_RepDist[1];
			m_RepDist[1]=m_RepDist[0];
			m_RepDist[0]=parsing[0].candidate[bestCandidate0].dist;
			if (outPrevMatch)
				InsertHash(currHash3,currHash6,parsing[0].candidate[bestCandidate0].len-1);
			else
				InsertHash(currHash3,currHash6,parsing[0].candidate[bestCandidate0].len);
			progress+=parsing[0].candidate[bestCandidate0].len;
			m_model->SetLiteralCtx(m_Wnd[m_WndCurrPos-1]);
			break;
		case P_REPDIST_MATCH:
			matchDist=m_RepDist[parsing[0].candidate[bestCandidate0].dist];
			for(j=parsing[0].candidate[bestCandidate0].dist;j>0;j--)
				m_RepDist[j]=m_RepDist[j-1];
			m_RepDist[0]=matchDist;
			m_model->EncodeRepDistMatch(parsing[0].candidate[bestCandidate0].dist,parsing[0].candidate[bestCandidate0].len-1);
			if (outPrevMatch)
				InsertHash(currHash3,currHash6,parsing[0].candidate[bestCandidate0].len-1);
			else
				InsertHash(currHash3,currHash6,parsing[0].candidate[bestCandidate0].len);
			progress+=parsing[0].candidate[bestCandidate0].len;
			m_model->SetLiteralCtx(m_Wnd[m_WndCurrPos-1]);
			break;
		case P_1BYTE_MATCH:
			m_model->Encode1BMatch();
			if (outPrevMatch) 
				;
			else
				InsertHash(currHash3,currHash6,1);
			progress++;
			m_model->SetLiteralCtx(m_Wnd[m_WndCurrPos-1]);
			break;
		}


	}

	return NO_ERROR;

}



int LZ::LZMinBlockFast(u32 size)
{
	u32 currHash6=0;
	u32 progress=0;

	u32 matchDist=0;
	u32 minMatchLen=1;
	u32 *HT6;

	u32 bestCandidate0;
	u32 lastDist=0;
	u32 distExtra=0;

	u32 i,j,cmpPos1,cmpPos2,cmpLen,remainLen,remainLen2;
	bool gotMatch;

	const u32 currBlockEndPos=m_WndCurrPos+size;

	m_ParsingPos=0;

	while(progress<size)
	{

		currHash6=HASH6(m_Wnd[m_WndCurrPos]);

		parsing[m_ParsingPos].total=1;
		parsing[m_ParsingPos].candidate[0].price=5;

		gotMatch=false;

		minMatchLen=1;
		remainLen=size-progress;

		if (remainLen>=MIN_LEN_HT2)
		for(i=0;i<4;i++)
		{
			cmpPos1=m_WndCurrPos>m_RepDist[i]?m_WndCurrPos-m_RepDist[i]:m_WndCurrPos+m_WndSize-m_RepDist[i];
			if ((cmpPos1<m_WndCurrPos || cmpPos1>currBlockEndPos) && (cmpPos1<m_WndSize))
			{
				cmpPos2=m_WndCurrPos;
				remainLen2=MIN(remainLen,m_WndSize-cmpPos1);
				if (*(u16*)&m_Wnd[cmpPos1]!=*(u16*)&m_Wnd[cmpPos2]) continue;
				if ((remainLen2<minMatchLen)||
					(m_Wnd[cmpPos1+minMatchLen]!=m_Wnd[cmpPos2+minMatchLen])||
					(m_Wnd[cmpPos1+(minMatchLen>>1)]!=m_Wnd[cmpPos2+(minMatchLen>>1)])
					)
					continue;

				cmpPos1+=2;
				cmpPos2+=2;
				cmpLen=2;

				if (remainLen2>3)
					while ((cmpLen<remainLen2-3)&&(*(u32*)(m_Wnd+cmpPos1)==*(u32*)(m_Wnd+cmpPos2)))
					{
						cmpPos1+=4;
						cmpPos2+=4;
						cmpLen+=4;
					}

					while((cmpLen<remainLen2)&&(m_Wnd[cmpPos1++]==m_Wnd[cmpPos2++])) 
						cmpLen++;

					if (cmpLen>minMatchLen)
					{
						minMatchLen=cmpLen;
						parsing[m_ParsingPos].candidate[parsing[m_ParsingPos].total].type=P_REPDIST_MATCH;
						parsing[m_ParsingPos].candidate[parsing[m_ParsingPos].total].len=cmpLen;
						parsing[m_ParsingPos].candidate[parsing[m_ParsingPos].total].price=cmpLen*8+3;
						parsing[m_ParsingPos].candidate[parsing[m_ParsingPos].total].dist=i;
						parsing[m_ParsingPos].total++;
						if (minMatchLen>GOOD_LEN_REP)
						{
							gotMatch=true;
							break;
						}
					}
			}
		}

		if (gotMatch) goto DETERMINE;

		cmpPos1=m_WndCurrPos>m_RepDist[0]?m_WndCurrPos-m_RepDist[0]:m_WndCurrPos+m_WndSize-m_RepDist[0];

		if ((minMatchLen<3) && (cmpPos1<m_WndCurrPos || cmpPos1>currBlockEndPos)
			&& (cmpPos1<m_WndSize) && (m_Wnd[cmpPos1]==m_Wnd[m_WndCurrPos]) )
		{
			parsing[m_ParsingPos].candidate[parsing[m_ParsingPos].total].type=P_1BYTE_MATCH;
			parsing[m_ParsingPos].candidate[parsing[m_ParsingPos].total].len=1;
			parsing[m_ParsingPos].candidate[parsing[m_ParsingPos].total].price=48;
			parsing[m_ParsingPos].total++;
		}


		HT6=&m_HT6[currHash6*m_HashNum];
		lastDist=1;
		distExtra=0;
		if (remainLen>=MIN_LEN_HT6)
		for(i=0;i<1;i++)
		{
			cmpPos1=HT6[i]&0x1FFFFFFF;
			if ((cmpPos1<m_WndCurrPos || cmpPos1>currBlockEndPos) &&
        (cmpPos1<m_WndSize) &&
        ((HT6[i]>>29)==(((u32)m_Wnd[m_WndCurrPos]&0x0E)>>1)))
			{ 
				matchDist=m_WndCurrPos>cmpPos1?m_WndCurrPos-cmpPos1:m_WndCurrPos+m_WndSize-cmpPos1;
				if (matchDist<lastDist) break;
				lastDist=matchDist;

				cmpPos2=m_WndCurrPos;
				remainLen2=MIN(remainLen,m_WndSize-cmpPos1);

				if (*(u32*)&m_Wnd[cmpPos1]!=*(u32*)&m_Wnd[cmpPos2]) continue;
				if ((remainLen2<minMatchLen)||
					(m_Wnd[cmpPos1+minMatchLen]!=m_Wnd[cmpPos2+minMatchLen])||
					(m_Wnd[cmpPos1+(minMatchLen>>1)]!=m_Wnd[cmpPos2+(minMatchLen>>1)])
					)
					continue;

				cmpPos1+=4;
				cmpPos2+=4;
				cmpLen=4;

				if (remainLen2>3)
					while ((cmpLen<remainLen2-3)&&(*(u32*)(m_Wnd+cmpPos1)==*(u32*)(m_Wnd+cmpPos2)))
					{cmpPos1+=4;cmpPos2+=4;cmpLen+=4;}
					while((cmpLen<remainLen2)&&(m_Wnd[cmpPos1++]==m_Wnd[cmpPos2++])) cmpLen++;

					if	( (cmpLen>minMatchLen) &&
						(
						((cmpLen>=MIN_LEN_HT6)&&(matchDist<(1<<18)))
						|| (cmpLen>=MIN_LEN_HT6+1)
						))
					{ 
						u32 countExtra=(matchDist>>7);
						distExtra=0;
						while(countExtra>0)
						{
							countExtra>>=2;
							distExtra+=3;
						}
						parsing[m_ParsingPos].candidate[parsing[m_ParsingPos].total].type=P_MATCH;
						parsing[m_ParsingPos].candidate[parsing[m_ParsingPos].total].len=cmpLen;
						parsing[m_ParsingPos].candidate[parsing[m_ParsingPos].total].dist=matchDist;
						parsing[m_ParsingPos].candidate[parsing[m_ParsingPos].total].price=cmpLen*8-10-distExtra;
						parsing[m_ParsingPos].total++;
						minMatchLen=cmpLen;
						if (cmpLen>GOOD_LEN_HT6)
						{
							gotMatch=true;
							break;
						}
					}
			}
		}


DETERMINE:



		bestCandidate0=0;
		for (i=1;i<parsing[0].total;i++)
		{
			if (parsing[0].candidate[i].price>parsing[0].candidate[bestCandidate0].price)
				bestCandidate0=i;
		}


		switch (parsing[0].candidate[bestCandidate0].type)
		{
		case P_LITERAL:
			m_model->EncodeLiteral(m_Wnd[m_WndCurrPos]);//,m_WndCurrPos
			InsertHashFast(currHash6,1);
			progress++;
			break;
		case P_MATCH:
			m_model->EncodeMatch(parsing[0].candidate[bestCandidate0].dist,parsing[0].candidate[bestCandidate0].len-1);
			m_RepDist[3]=m_RepDist[2];
			m_RepDist[2]=m_RepDist[1];
			m_RepDist[1]=m_RepDist[0];
			m_RepDist[0]=parsing[0].candidate[bestCandidate0].dist;
			InsertHashFast(currHash6,parsing[0].candidate[bestCandidate0].len);
			progress+=parsing[0].candidate[bestCandidate0].len;
			m_model->SetLiteralCtx(m_Wnd[m_WndCurrPos-1]);
			break;
		case P_REPDIST_MATCH:
			matchDist=m_RepDist[parsing[0].candidate[bestCandidate0].dist];
			for(j=parsing[0].candidate[bestCandidate0].dist;j>0;j--)
				m_RepDist[j]=m_RepDist[j-1];
			m_RepDist[0]=matchDist;
			m_model->EncodeRepDistMatch(parsing[0].candidate[bestCandidate0].dist,parsing[0].candidate[bestCandidate0].len-1);
			InsertHashFast(currHash6,parsing[0].candidate[bestCandidate0].len);
			progress+=parsing[0].candidate[bestCandidate0].len;
			m_model->SetLiteralCtx(m_Wnd[m_WndCurrPos-1]);
			break;
		case P_1BYTE_MATCH:
			m_model->Encode1BMatch();
			InsertHashFast(currHash6,1);
			progress++;
			m_model->SetLiteralCtx(m_Wnd[m_WndCurrPos-1]);
			break;
		}


	}
	if (progress>size)
		printf("\nff %u\n",m_WndCurrPos);

	return NO_ERROR;

}
