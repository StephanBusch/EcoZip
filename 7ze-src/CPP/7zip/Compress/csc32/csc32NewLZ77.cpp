#include "csc32Model.h"
#include "csc32LZ.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "csc32Coder.h"
#include "csc32LZCommon.h"



inline void LZ::NewInsert1()
{
	u32	*HT6=&m_HT6[cHash6*m_HashNum];
	u32 i;

	m_HT2[cHash2]=CURPOS;
	m_HT3[cHash3]=CURPOS;

	for(i=m_HashNum-1;i>0;i--)
		HT6[i]=HT6[i-1];

	HT6[0]=CURPOS;
	m_WndCurrPos++;
}

inline void LZ::NewInsertN(u32 len)
{
	u32	*HT6=&m_HT6[cHash6*m_HashNum];

	u32 lastCurrHash6;
	u32 i;

	m_HT2[cHash2]=CURPOS;
	m_HT3[cHash3]=CURPOS;

	for(i=m_HashNum-1;i>0;i--)
		HT6[i]=HT6[i-1];

	HT6[0]=CURPOS;
	len--;
	m_WndCurrPos++;

	lastCurrHash6=cHash6;

	while(len>0)
	{
		cHash6=HASH6(m_Wnd[m_WndCurrPos]);
		cHash3=HASH3(m_Wnd[m_WndCurrPos]);
		HT6=&m_HT6[cHash6*m_HashNum];

		m_HT2[*(u16*)(m_Wnd+m_WndCurrPos)]=CURPOS;
		m_HT3[cHash3]=CURPOS;

		if (lastCurrHash6!=cHash6)
		{
			for(i=m_HashNum-1;i>0;i--)
				HT6[i]=HT6[i-1];
		}
		HT6[0]=CURPOS;

		if (len>256)
		{
			len-=4;
			m_WndCurrPos+=4;
		}
		len--;
		m_WndCurrPos++;	
		lastCurrHash6=cHash6;
	}
}


u32 LZ::FindMatch(u32 idx,u32 minMatchLen)
{
	u32 cmpPos1,cmpPos2;
	u32 remainLen,cmpLen,matchDist;
	u32 i,mostRecPos=0,lastDist;
	u32 maxValidDist=0;
	u32 *HT6,*HT3,*HT2;
	u32 longestMatch=0;

	parser[idx].choices.startPos=matchListSize;
	parser[idx].choices.choiceNum=0;

	cHash6=HASH6(m_Wnd[m_WndCurrPos]);
	cHash3=HASH3(m_Wnd[m_WndCurrPos]);
	cHash2=HASH2(m_Wnd[m_WndCurrPos]);


	//===================find repeat distance match=============================
	for(i=0;i<4;i++)
	{
		cmpPos1=m_WndCurrPos>parser[idx].repDist[i]?
			m_WndCurrPos-parser[idx].repDist[i]:m_WndCurrPos+m_WndSize-parser[idx].repDist[i];

		if (i==0) mostRecPos=cmpPos1;
		cmpLen=0;

		if ((cmpPos1<m_WndCurrPos || cmpPos1>currBlockEndPos) && (cmpPos1<m_WndSize))
		{
			cmpPos2=m_WndCurrPos;
			remainLen=MIN(currBlockEndPos-m_WndCurrPos, m_WndSize-cmpPos1);

			if (remainLen<MIN_LEN_REP || remainLen<minMatchLen)
				continue;

			if ((m_Wnd[cmpPos1+minMatchLen]!=m_Wnd[cmpPos2+minMatchLen])||
				(*(u16*)&m_Wnd[cmpPos1]!=*(u16*)&m_Wnd[cmpPos2])
				)
				continue;

			cmpPos1+=2;
			cmpPos2+=2;
			cmpLen=2;

			if (remainLen>3)
				while ((cmpLen<remainLen-3)&&(*(u32*)(m_Wnd+cmpPos1)==*(u32*)(m_Wnd+cmpPos2)))
				{cmpPos1+=4;cmpPos2+=4;cmpLen+=4;}

				while((cmpLen<remainLen)&&(m_Wnd[cmpPos1++]==m_Wnd[cmpPos2++])) cmpLen++;

				if (cmpLen>minMatchLen)
				{
					longestMatch=minMatchLen=
						matchList[matchListSize].len=(u16)cmpLen;
					matchList[matchListSize].pos=i;
					matchListSize++;
					parser[idx].choices.choiceNum++;
					//repMatchNum++;

					if (parser[idx].repDist[i]>maxValidDist)
						maxValidDist=parser[idx].repDist[i];
				}
		}
	}
	//===================find repeat distance match=============================


	//===================find repeat byte match=============================
	if ((mostRecPos<m_WndCurrPos || mostRecPos>currBlockEndPos)
		&& (mostRecPos<m_WndSize) && (m_Wnd[mostRecPos]==m_Wnd[m_WndCurrPos]) )
	{
		matchList[matchListSize].len=1;
		matchList[matchListSize].pos=4;
		matchListSize++;
		parser[idx].choices.choiceNum++;
	}
	//===================find repeat byte match=============================

	//===================find hash 6 match=============================
	HT6=&m_HT6[cHash6*m_HashNum];
	lastDist=1;
	for(i=0;i<m_HashNum;i++)
	{
		cmpPos1=HT6[i]&0x1FFFFFFF;
		if ((cmpPos1<m_WndCurrPos || cmpPos1>currBlockEndPos)  
			&& (cmpPos1<m_WndSize) 
			&& ((HT6[i]>>29)==(((u32)m_Wnd[m_WndCurrPos]&0x0E)>>1))) //cache
		{
			matchDist=m_WndCurrPos>cmpPos1?
				m_WndCurrPos-cmpPos1:m_WndCurrPos+m_WndSize-cmpPos1;

			if (matchDist<lastDist) break;
			lastDist=matchDist;

			cmpPos2=m_WndCurrPos;
			remainLen=MIN(currBlockEndPos-m_WndCurrPos, m_WndSize-cmpPos1);

			if (remainLen<MIN_LEN_HT6 || remainLen<minMatchLen)
				continue;

			if ((m_Wnd[cmpPos1+minMatchLen]!=m_Wnd[cmpPos2+minMatchLen])||
				(*(u32*)&m_Wnd[cmpPos1]!=*(u32*)&m_Wnd[cmpPos2]))
				continue;

			cmpPos1+=4;
			cmpPos2+=4;
			cmpLen=4;

			if (remainLen>3)
				while ((cmpLen<remainLen-3)&&(*(u32*)(m_Wnd+cmpPos1)==*(u32*)(m_Wnd+cmpPos2)))
				{cmpPos1+=4;cmpPos2+=4;cmpLen+=4;}
				while((cmpLen<remainLen)&&(m_Wnd[cmpPos1++]==m_Wnd[cmpPos2++])) cmpLen++;

				if	(cmpLen>minMatchLen)
				{ 
					if (cmpLen==MIN_LEN_HT2 && matchDist>=63)
						continue;
					if (cmpLen==MIN_LEN_HT3 && matchDist>=2047)
						continue;
					//if (cmpLen==4 && matchDist>=256*KB)
					//	continue;

					longestMatch=minMatchLen=
						matchList[matchListSize].len=(u16)cmpLen;
					matchList[matchListSize].pos=matchDist+4;
					matchListSize++;
					parser[idx].choices.choiceNum++;
				}
		}
	}

	if (minMatchLen>MIN_LEN_HT3) 
		return minMatchLen;

	//===================find hash 3 match=============================
	HT3=&m_HT3[cHash3];

	for(i=0;i<1;i++)
	{
		cmpPos1=HT3[i]&0x1FFFFFFF;

		if ((cmpPos1<m_WndCurrPos || cmpPos1>currBlockEndPos) 
			&& (cmpPos1<m_WndSize)
			&& ((HT3[i]>>29)==(((u32)m_Wnd[m_WndCurrPos]&0x0E)>>1)))
		{
			cmpPos2=m_WndCurrPos;
			remainLen=MIN(currBlockEndPos-m_WndCurrPos, m_WndSize-cmpPos1);
			matchDist=m_WndCurrPos>cmpPos1?
				m_WndCurrPos-cmpPos1:m_WndCurrPos+m_WndSize-cmpPos1;

			if (remainLen<MIN_LEN_HT3 || remainLen<minMatchLen)
				continue;

			if ((m_Wnd[cmpPos1+minMatchLen]!=m_Wnd[cmpPos2+minMatchLen])||
				(*(u16*)&m_Wnd[cmpPos1]!=*(u16*)&m_Wnd[cmpPos2]))
				continue;

			cmpPos1+=2;
			cmpPos2+=2;
			cmpLen=2;

			if (remainLen>3)
				while ((cmpLen<remainLen-3)&&(*(u32*)(m_Wnd+cmpPos1)==*(u32*)(m_Wnd+cmpPos2)))
				{cmpPos1+=4;cmpPos2+=4;cmpLen+=4;}
				while((cmpLen<remainLen)&&(m_Wnd[cmpPos1++]==m_Wnd[cmpPos2++])) cmpLen++;


				if (cmpLen>minMatchLen)
				{
					if (cmpLen==MIN_LEN_HT2 && matchDist>=63)
						continue;
					if (cmpLen==MIN_LEN_HT3 && matchDist>=2047)
						continue;
					//if (cmpLen==4 && matchDist>=256*KB)
					//	continue;

					longestMatch=minMatchLen=
						matchList[matchListSize].len=(u16)cmpLen;
					matchList[matchListSize].pos=matchDist+4;
					matchListSize++;
					parser[idx].choices.choiceNum++;
				}
		}
	}
	//===================find hash 3 match=============================

	if (minMatchLen>MIN_LEN_HT2) 
		return minMatchLen;

	//===================find hash 2 match=============================
	HT2=&m_HT2[cHash2];

	for(i=0;i<1;i++)
	{
		cmpPos1=HT2[i]&0x1FFFFFFF;

		if ((cmpPos1<m_WndCurrPos || cmpPos1>currBlockEndPos) 
			&& (cmpPos1<m_WndSize)
			&& ((HT2[i]>>29)==(((u32)m_Wnd[m_WndCurrPos]&0x0E)>>1)))
		{
			cmpPos2=m_WndCurrPos;
			remainLen=MIN(currBlockEndPos-m_WndCurrPos, m_WndSize-cmpPos1);
			matchDist=m_WndCurrPos>cmpPos1?
				m_WndCurrPos-cmpPos1:m_WndCurrPos+m_WndSize-cmpPos1;


			if (remainLen<MIN_LEN_HT2 || remainLen<minMatchLen)
				continue;

			if ((m_Wnd[cmpPos1+minMatchLen]!=m_Wnd[cmpPos2+minMatchLen])||
				(*(u16*)&m_Wnd[cmpPos1]!=*(u16*)&m_Wnd[cmpPos2]))
				continue;


			cmpPos1+=2;
			cmpPos2+=2;
			cmpLen=2;

			if (remainLen>3)
				while ((cmpLen<remainLen-3)&&(*(u32*)(m_Wnd+cmpPos1)==*(u32*)(m_Wnd+cmpPos2)))
				{cmpPos1+=4;cmpPos2+=4;cmpLen+=4;}
				while((cmpLen<remainLen)&&(m_Wnd[cmpPos1++]==m_Wnd[cmpPos2++])) cmpLen++;


				if (cmpLen>minMatchLen)
				{
					if (cmpLen==MIN_LEN_HT2 && matchDist>=63)
						continue;
					if (cmpLen==MIN_LEN_HT3 && matchDist>=2047)
						continue;
					//if (cmpLen==4 && matchDist>=256*KB)
					//	continue;

					longestMatch=minMatchLen=
						matchList[matchListSize].len=(u16)cmpLen;
					matchList[matchListSize].pos=matchDist+4;
					matchListSize++;
					parser[idx].choices.choiceNum++;
				}
		}
	}
	//===================find hash 2 match=============================
	return longestMatch;
}

//u32 literalNum=0;

int LZ::LZMinBlockNew(u32 size,u32 TryLazy,u32 LazyStep,u32 GoodLen)
{
	u32 maxReach=0;
	const u32 initWndPos=m_WndCurrPos;
	u32 fCtx;
	u32 lastBackPos=0;
	u32 lazyNum=0,lazyEndPos=0;

	currBlockEndPos=m_WndCurrPos+size;
	matchListSize=0;

	for(u32 i=0;i<size+1;i++)
		parser[i].price=0x0FFFFFFF;

	parser[0].price=0;
	parser[0].fstate=m_model->state;
	parser[0].repDist[0]=m_RepDist[0];parser[0].repDist[1]=m_RepDist[1];
	parser[0].repDist[2]=m_RepDist[2];parser[0].repDist[3]=m_RepDist[3];
	fCtx=m_model->GetLiteralCtx();

	for(u32 i=0;i<size;)
	{
		u32 curOpPrice;
		u32 newReach,longestMatch;

		longestMatch=FindMatch(i,(lazyNum>0)?(maxReach-i-1):1);
		
		newReach=i+MAX(1,longestMatch);
		//if (m_WndCurrPos>540)
		//	printf("asdf");
		curOpPrice=m_model->GetLiteralPrice(parser[i].fstate,fCtx,m_Wnd[m_WndCurrPos]);
		if (parser[i+1].price>parser[i].price+curOpPrice)
		{
			parser[i+1].backChoice.choice=0;
			parser[i+1].backChoice.fromPos=(u16)i;
			parser[i+1].price=parser[i].price+curOpPrice;
			parser[i+1].fstate=(parser[i].fstate*4)&0x3F;
			parser[i+1].repDist[0]=parser[i].repDist[0];
			parser[i+1].repDist[1]=parser[i].repDist[1];
			parser[i+1].repDist[2]=parser[i].repDist[2];
			parser[i+1].repDist[3]=parser[i].repDist[3];

		}

		for(int j=0;j<parser[i].choices.choiceNum;j++)
		{
			u32 tmpChoiceIdx=j+parser[i].choices.startPos;
			u32 tmpCurState;
			u32 tmpCurLen;

			tmpCurLen=matchList[tmpChoiceIdx].len;
			if (matchList[tmpChoiceIdx].pos<4)
			{
				curOpPrice=m_model->GetRepDistMatchPrice(parser[i].fstate,
					matchList[tmpChoiceIdx].pos,
					tmpCurLen);
				tmpCurState=3;
			}
			else if (matchList[tmpChoiceIdx].pos==4)
			{
				curOpPrice=m_model->Get1BMatchPrice(parser[i].fstate);
				tmpCurState=2;
			}
			else
			{
				curOpPrice=m_model->GetMatchPrice(parser[i].fstate,
					matchList[tmpChoiceIdx].pos-4,
					tmpCurLen);
				tmpCurState=1;
			}

			{
				int k=tmpCurLen;

				if (parser[i+k].price>parser[i].price+curOpPrice)
				{

					parser[i+k].backChoice.choice=(u8)(j+1);
					parser[i+k].backChoice.fromPos=(u16)i;
					parser[i+k].backChoice.fromLen=k;
					parser[i+k].price=parser[i].price+curOpPrice;
					parser[i+k].fstate=(parser[i].fstate*4+tmpCurState)&0x3F;
					if ((tmpCurState==2)||(tmpCurState==3 && matchList[tmpChoiceIdx].pos==0))
					{
						parser[i+k].repDist[0]=parser[i].repDist[0];
						parser[i+k].repDist[1]=parser[i].repDist[1];
						parser[i+k].repDist[2]=parser[i].repDist[2];
						parser[i+k].repDist[3]=parser[i].repDist[3];
					}
					else if (tmpCurState==1)
					{
						parser[i+k].repDist[0]=matchList[tmpChoiceIdx].pos-4;
						parser[i+k].repDist[1]=parser[i].repDist[0];
						parser[i+k].repDist[2]=parser[i].repDist[1];
						parser[i+k].repDist[3]=parser[i].repDist[2];
					}
					else
					{
						if (matchList[tmpChoiceIdx].pos==1)
						{
							parser[i+k].repDist[0]=parser[i].repDist[1];
							parser[i+k].repDist[1]=parser[i].repDist[0];
							parser[i+k].repDist[2]=parser[i].repDist[2];
							parser[i+k].repDist[3]=parser[i].repDist[3];
						}
						else if (matchList[tmpChoiceIdx].pos==2)
						{
							parser[i+k].repDist[0]=parser[i].repDist[2];
							parser[i+k].repDist[1]=parser[i].repDist[0];
							parser[i+k].repDist[2]=parser[i].repDist[1];
							parser[i+k].repDist[3]=parser[i].repDist[3];
						}
						else if (matchList[tmpChoiceIdx].pos==3)
						{
							parser[i+k].repDist[0]=parser[i].repDist[3];
							parser[i+k].repDist[1]=parser[i].repDist[0];
							parser[i+k].repDist[2]=parser[i].repDist[1];
							parser[i+k].repDist[3]=parser[i].repDist[2];
						}
					}	
				}
			}
		}

		if (longestMatch>TryLazy && lazyNum==0)
		{
			lazyNum=LazyStep;
			lazyEndPos=i+longestMatch;
		}

		if (lazyNum>0)
		{
			lazyNum--;
			if (lazyNum==0)
			{
				NewInsertN(lazyEndPos-i);
				i=lazyEndPos;
				fCtx=m_Wnd[m_WndCurrPos-1];
			}
			else
			{
				fCtx=m_Wnd[m_WndCurrPos];
				NewInsert1();
				i++;
			}
		}
		else
		{
			fCtx=m_Wnd[m_WndCurrPos];
			NewInsert1();
			i++;
		}

		if (newReach>maxReach)
			maxReach=newReach;

		if (maxReach-lastBackPos>1024 || longestMatch>GoodLen)
		{
			if (maxReach>i)
				NewInsertN(maxReach-i);
			LZBackward(initWndPos,lastBackPos,maxReach);
			lastBackPos=i=maxReach;
			fCtx=m_Wnd[m_WndCurrPos-1];

			lazyNum=0;
			parser[maxReach].price=0;
			parser[maxReach].fstate=m_model->state;
			parser[maxReach].repDist[0]=m_RepDist[0];parser[maxReach].repDist[1]=m_RepDist[1];
			parser[maxReach].repDist[2]=m_RepDist[2];parser[maxReach].repDist[3]=m_RepDist[3];
		}
	}
	
	LZBackward(initWndPos,lastBackPos,size);
	

	/*float a=(float)m_model->GetMatchPrice(m_model->state,13,2)/128;
	printf("match price: 2 37 %f\n",a);
	a=(float)m_model->GetMatchPrice(m_model->state,33,3)/128;
	printf("match price: 3 1023 %f\n",a);
	a=(float)m_model->GetMatchPrice(m_model->state,165,4)/128;
	printf("match price: 4 64kB %f\n",a);
	a=(float)m_model->GetMatchPrice(m_model->state,235,5)/128;
	printf("match price: 5 1MB %f\n",a);
	a=(float)m_model->GetLiteralPrice(m_model->state,0,'t')/128;
	printf("litral price: t %f\n",a);
	a=(float)m_model->GetRepDistMatchPrice(m_model->state,2,4)/128;
	printf("match rep price: 2 4 %f\n",a);
	printf("=============================\n");*/
	//printf("%d\n",literalNum);
	return NO_ERROR;
}



void LZ::LZBackward(const u32 initWndPos,u32 start,u32 end)
{
  for (u32 i = end; i>start;)
	{
		parser[parser[i].backChoice.fromPos].finalChoice=parser[i].backChoice.choice;
		parser[parser[i].backChoice.fromPos].finalLen=parser[i].backChoice.fromLen;
		i=parser[i].backChoice.fromPos;
	}

  for (u32 i = start; i<end;)
	{
		if (parser[i].finalChoice==0)
		{
			m_model->EncodeLiteral(m_Wnd[initWndPos+i]);
			//literalNum++;
			i++;
		}
		else
		{
			u32 tmpIdx=parser[i].finalChoice-1+parser[i].choices.startPos;
			if (matchList[tmpIdx].pos<4)
			{
				m_model->EncodeRepDistMatch(matchList[tmpIdx].pos,parser[i].finalLen-1);

				u32 tmpDist=m_RepDist[matchList[tmpIdx].pos];
				for(int j=matchList[tmpIdx].pos;j>0;j--)
					m_RepDist[j]=m_RepDist[j-1];
				m_RepDist[0]=tmpDist;

				i+=parser[i].finalLen;
				m_model->SetLiteralCtx(m_Wnd[initWndPos+i-1]);
			}
			else if (matchList[tmpIdx].pos==4)
			{
				m_model->Encode1BMatch();
				m_model->SetLiteralCtx(m_Wnd[initWndPos+i]);
				i++;
			}
			else
			{
				m_model->EncodeMatch(matchList[tmpIdx].pos-4,parser[i].finalLen-1);

				m_RepDist[3]=m_RepDist[2];
				m_RepDist[2]=m_RepDist[1];
				m_RepDist[1]=m_RepDist[0];
				m_RepDist[0]=matchList[tmpIdx].pos-4;

				i+=parser[i].finalLen;
				m_model->SetLiteralCtx(m_Wnd[initWndPos+i-1]);
			}

		}	
	}
}