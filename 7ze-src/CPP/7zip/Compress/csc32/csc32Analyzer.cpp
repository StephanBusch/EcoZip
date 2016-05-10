#include <math.h>
#include <stdio.h>
#include "csc32Analyzer.h"
//#include ".h"

//FILE *fLog;
#define AYZDEBUG


Analyzer::Analyzer()
{
	u32 i;
	for(i=0;i<(MinBlockSize>>4);i++)
    logTable[i] = (u32)((double)100 * log((double)i * 16 + 8) / log((double)2));

  logTable[(MinBlockSize >> 4)] = (u32)((double)100 * log((double)(MinBlockSize)) / log((double)2));
	//fLog=fopen("r:\\dataLog.txt","w");
}


Analyzer::~Analyzer()
{
	//fclose(fLog);
}


u32 Analyzer::analyzeHeader(u8 *src,u32 size,u32 *typeArg1,u32 *typeArg2,u32 * /*typeArg3*/)
{
	if (size<128)
		return DT_NONE;

	if (src[0]=='B' && src[1]=='M')
	{
		u32 width=*(u32*)(src+0x12);
		u32 colorbits=*(u32*)(src+0x1C);
		u32 compressed=*(u32*)(src+0x1E);

#ifdef AYZDEBUG
		printf(" ===== Detect BM: width:%u colorbits:%u compressed:%u\n",width,colorbits,compressed);
#endif
		if (width<16384 && 
			(colorbits==8||
			colorbits==16||
			colorbits==24||
			colorbits==32) &&
			compressed==0 &&
			*(u16*)(src+0x1A)==1
			)
		{
			*typeArg1=width;
			*typeArg2=colorbits;
			return DT_RGB;
		}
	}

	if (src[0]==0 && src[1]==0 &&src[3]==0)
	{
		if ( (src[2]==2 || src[2]==10)
			&& (src[16]==24 || src[16]==32) )
		{
			u32 width=*(u16*)(src+0x0C);

#ifdef AYZDEBUG
			printf(" ===== Detect TGA: width:%u colorbits:%u\n",width,src[16]);
#endif
			*typeArg1=width;
			*typeArg2=src[16];
			return DT_RGB;
		}
	}


	if (src[0]=='P' && src[1]=='6' &&src[2]==0x0a)
	{
		u8 *pTmp=src+3;
		bool inComent=false;
		while(*pTmp==0 || *pTmp=='#' || inComent)
		{
			if (*pTmp==0x0a) inComent=false;
			else if (*pTmp=='#') inComent=true;
			pTmp++;
			if (pTmp-src>127)
				break;
		}

		u32 width;
		sscanf((char*)pTmp,"%d",&width);

#ifdef AYZDEBUG
		printf(" ===== Detect PPM - P6: width:%u\n",width);
#endif
			*typeArg1=width;
			*typeArg2=24;
			return DT_RGB;
	}

//	if (src[0]=='R' && src[1]=='I' && src[2]=='F' && src[3]=='F' &&
//		src[8]=='W' && src[9]=='A' && src[10]=='V' && src[11]=='E')
//	{
//#ifdef AYZDEBUG
//		printf(" ===== Detect Riff Wav \n");
//#endif
//		return DT_AUDIO;
//	}
	/*if (src[0]==80 && src[2]==10 && (src[1]==53||src[1]==54))
	{
		P5623
	}*/

/*	
	//if (*(tufx)==80  && *(tufx+1)==54 && *(tufx+2)==10 ){PPMread();Decont=1;chn=1;deltarange=1;}
	//if (*(tufx)==80  && *(tufx+1)==53 && *(tufx+2)==10 ){PPMread();Decont=1;chn=1;deltarange=1;}
	//if (Peekl(tufx+8)==1163280727)  {Decont=1;processed=1;chn=Peekw(tufx+22);deltarange=(Peekw(tufx+34)>>3);}//
	//if (Peekl(tufx)==1297239878 && Peekl(tufx+8)==1179011393 )  {Decont=1;chn=Peekb(tufx+21);deltarange=Peekb(tufx+27)>>3;} //AIFF AUDIO

	//if (*(tufx)==80  && *(tufx+1)==54  && *(tufx+2)==10 ){ CTX1=3;CTX2=6;processed=0;chn=1;deltarange=Deltac;ROLZ=0;RZCLEVEL=2;PPMread();} //*PPM

	*.wav -----   if (Peekl(tufx)==1179011410)       { processed=1;ROLZ=0;RZCLEVEL=2;chn=1;deltarange=1;} //RIFF WAVE
	if (Peekl(tufx)==1297239878        && Peekl(tufx+8)==1179011393 )            {processed=Deltac;ROLZ=0;RZCLEVEL=2;chn=Peekb(tufx+21);CTX1=chn*(Peekb(tufx+27)>>3);CTX2=CTX1*2;deltarange=Peekb(tufx+27)>>3;} //AIFF AUDIO
	if (Peekl(tufx+8)==1163280727)                                              { processed=Deltac;ROLZ=0;RZCLEVEL=2;chn=Peekw(tufx+22);CTX1=chn;CTX2=CTX1*2;deltarange=(Peekw(tufx+34)>>3);}// wave //
	//if (*(tufx)==0   && *(tufx+1)==0   && *(tufx+2)==2  && *(tufx+3)==0 ) { CTX1=3;CTX2=6;processed=Deltac;chn=1;deltarange=1;ROLZ=0;RZCLEVEL=2;} // TARGA image //
	//if (*(tufx)==0   && *(tufx+1)==0   && *(tufx+2)==10 && *(tufx+3)==0)  { CTX1=3;CTX2=6;processed=Deltac;chn=1;deltarange=1;ROLZ=0;RZCLEVEL=2;} // TARGA image RLE//
	//if (*(tufx)==80  && *(tufx+1)==53  && *(tufx+2)==10 )                { processed=Deltac;chn=1;deltarange=1;ROLZ=1;RZCLEVEL=2;PPMread();} // PGM image //
*/
	return DT_NONE;
}

u32 tmpcount=0;
u32 lastType=0;

i32 Analyzer::GetChnIdx(u8 *src,u32 size)
{
	u32 sameDist[DLT_CHANNEL_MAX]={0},succValue[DLT_CHANNEL_MAX]={0};
	u32 progress,succChar,maxSame,maxSucc,minSame,minSucc,bestChnNum;

	progress=0;
	succChar=0;
	while(progress<size-16)
	{
			sameDist[0]+=(src[progress]==src[progress+1]);
			sameDist[1]+=(src[progress]==src[progress+2]);
			sameDist[2]+=(src[progress]==src[progress+3]);
			sameDist[3]+=(src[progress]==src[progress+4]);
			sameDist[4]+=(src[progress]==src[progress+8]);
			succValue[0]+=abs((signed)src[progress]-(signed)src[progress+1]);
			succValue[1]+=abs((signed)src[progress]-(signed)src[progress+2]);
			succValue[2]+=abs((signed)src[progress]-(signed)src[progress+3]);
			succValue[3]+=abs((signed)src[progress]-(signed)src[progress+4]);
			succValue[4]+=abs((signed)src[progress]-(signed)src[progress+8]);
			progress++;
	}

	maxSame=minSame=sameDist[0];
	maxSucc=minSucc=succValue[0];
	bestChnNum=0;

	for (int i=0;i<DLT_CHANNEL_MAX;i++)
	{
		if (sameDist[i]<minSame) minSame=sameDist[i];
		if (sameDist[i]>maxSame) maxSame=sameDist[i];
		if (succValue[i]>maxSucc) maxSucc=succValue[i];
		if (succValue[i]<minSucc) 
		{
			minSucc=succValue[i];
			bestChnNum=i;
		}
	}


	if ( ((maxSucc>succValue[bestChnNum]*4) || (maxSucc>succValue[bestChnNum]+40*size)) 
		&& (sameDist[bestChnNum]>minSame*3)
		//&& (entropy>700*size || diffNum>245) 
		&& (sameDist[0]<0.3*size))
	{
		//printf("delta:%d %d %d %d %d %dr:%d \n",succValue[0],succValue[1],succValue[2],succValue[3],succValue[4],sameDist[5],bestChnNum);
		return bestChnNum;
	}
	return -1;
}
/*{	
	u32 finalRank[8]={0};
	u32 validRanks=0;
	u32 trendline[4*4];

	//split to 4 small parts,and only test these small parts
	for(u32 startPos=0;startPos<size;startPos+=size/4)
	{
		u32 validTrend[8]={0};
		u32 t00[8]={0};
		u32 t1122[8]={0};
		u32 MinChnIdx=2,MaxChnIdx=2,Min1122Idx=2,Max1122Idx=2;

		for(u32 idx=2;idx<5;idx++) //try every channel
		{
			u32 channelNum=DltIndex[idx];

			for(u32 i=0;i<4*4;i++)
				trendline[i]=0;

			for(u32 i=0;i<channelNum;i++)
			{
				i32 lastchar=0;
				i32 thischar;
				u32 state=0;
				for(u32 j=startPos+i;j<startPos+512;j+=channelNum)
				{
					state=(state*4)%16;
					thischar=buf[j];
					if (thischar>lastchar && thischar<lastchar+8)
						state=state+1;
					else if (thischar<lastchar && thischar>lastchar-8)
						state=state+2;
					else if (thischar==lastchar);
					else 
						state=state+3;

					trendline[state]++;

					lastchar=thischar;
				}
			}

			for(u32 i=0;i<16;i++)
			{
				if ( ((i/4)%4)!=3 && (i%4)!=3)
				{
					validTrend[idx]+=trendline[i];
				}
			}

			t00[idx]=trendline[0];
			t1122[idx]=trendline[1*4+1]+trendline[2*4+2];

			if (validTrend[idx]<validTrend[MinChnIdx])
				MinChnIdx=idx;
			if (validTrend[idx]>validTrend[MaxChnIdx])
				MaxChnIdx=idx;
			if (t1122[idx]<t1122[Min1122Idx])
				Min1122Idx=idx;
			if (t1122[idx]>t1122[Max1122Idx])
				Max1122Idx=idx;

		}//End for idx 2..5


		if ( (
			(validTrend[MaxChnIdx]>validTrend[MinChnIdx]*2 && validTrend[MaxChnIdx]>200)
			|| (validTrend[MaxChnIdx]+1>(validTrend[MinChnIdx]+1)*4)
			)
			&& validTrend[MinChnIdx]<30
			&& (validTrend[MaxChnIdx]>validTrend[3]*2 || validTrend[MaxChnIdx]>validTrend[2]*2)
			&& ((validTrend[2]+validTrend[3]<20) || validTrend[2]>validTrend[3]*2 || validTrend[3]>validTrend[2]*2)
			)
		{
			finalRank[MaxChnIdx]++;
			validRanks++;
		}
	} // end for 4 parts


	if (validRanks==4)
	{
		for(u32 i=0;i<8;i++)
			if (finalRank[i]>=3)
				return i;
	}

	return -1;
}
*/

u32 Analyzer::analyze(u8 *src,u32 size)
{
	u32 progress;
	u32 i;
	u32 avgFreq,freq[256]={0};
	u32 freq0x80[2]={0};
	u32 entropy,alphatbetNum,diffNum;

	tmpcount+=size;

	if (size>MinBlockSize)
		size=MinBlockSize;

	if (size<512)
		return DT_SKIP;

	progress=0;

	for(progress=0;progress<size;progress++)
		freq[src[progress]]++;

	diffNum=0;
	entropy=size*logTable[size>>4];


	for(i=0;i<256;i++)
	{
		entropy-=freq[i]*logTable[freq[i]>>4];
		diffNum+=(freq[i]>0);
		freq0x80[i>>7]+=freq[i];
	}

	avgFreq=size>>8;

	alphatbetNum=0;
	for(i='a';i<='z';i++)
	{
		alphatbetNum+=freq[i];
	}
	
	if (freq0x80[1]<(size>>3) && (freq[' ']>(size>>7)) 
		&& (freq['a']+freq['e']+freq['t']>(size>>4)) 
		&& entropy>300*size 
		&& alphatbetNum>(size>>2))
		return DT_ENGTXT;


	if (freq[0x8b]>avgFreq&&freq[0x00]>avgFreq*2&&freq[0xE8]>6)
		return DT_EXE;

	if (entropy<400*size && diffNum<200)
		return DT_NORMAL;

	i32 dltIdx=GetChnIdx(src,size);
	if (dltIdx!=-1)
		return DT_DLT+dltIdx;

	
	if (entropy>795*size)
		return DT_BAD;
	else if (entropy>780*size)
		return DT_FAST; 

	return DT_NORMAL;//DT_NORMAL;
}


