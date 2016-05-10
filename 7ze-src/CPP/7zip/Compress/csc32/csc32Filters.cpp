#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include "csc32Filters.h"
#include "csc32Common.h"
#include <stdio.h>

const u32 wordNum=123;

u8 wordList[wordNum][8]=
{
	"",
	"ac","ad","ai","al","am",
	"an","ar","as","at","ea",
	"ec","ed","ee","el","en",
	"er","es","et","id","ie",
	"ig","il","in","io","is",
	"it","of","ol","on","oo",
	"or","os","ou","ow","ul",
	"un","ur","us","ba","be",
	"ca","ce","co","ch","de",
	"di","ge","gh","ha","he",
	"hi","ho","ra","re","ri",
	"ro","rs","la","le","li",
	"lo","ld","ll","ly","se",
	"si","so","sh","ss","st",
	"ma","me","mi","ne","nc",
	"nd","ng","nt","pa","pe",
	"ta","te","ti","to","th",
	"tr","wa","ve",
	"all","and","but","dow",
	"for","had","hav","her",
	"him","his","man","mor",
	"not","now","one","out",
	"she","the","was","wer",
	"whi","whe","wit","you",
	"any","are",
	"that","said","with","have",
	"this","from","were","tion",
};



void Filters::MakeWordTree()
{
	u32 i,j;
	u32 treePos;
	u8 symbolIndex=0x82;

	nodeMum=1;

	memset(wordTree,0,sizeof(wordTree));

	for (i=1;i<wordNum;i++)
	{	
		treePos=0;
		for(j=0;wordList[i][j]!=0;j++)
		{
			u32 idx=wordList[i][j]-'a';
			if (wordTree[treePos].next[idx])
				treePos=wordTree[treePos].next[idx];
			else
			{
				wordTree[treePos].next[idx]=nodeMum;
				treePos=nodeMum;
				nodeMum++;
			}
		}
		wordIndex[symbolIndex]=i;
		wordTree[treePos].symbol=symbolIndex++;
	}

	maxSymbol=symbolIndex;

}


Filters::Filters()
{
	m_fltSwapSize=0;
	MakeWordTree();
}



Filters::~Filters()
{
	if (m_fltSwapSize>0)
	{
		SAFEFREE(m_fltSwapBuf);
	}
	m_fltSwapBuf=0;
}


void Filters::Forward_Delta(u8 *src,u32 size,u32 chnNum)
{
	u32 dstPos,i,j,prevByte;
	u32 lastDelta;

	if (size<512)
		return;

	if (m_fltSwapSize<size)
	{
		if (m_fltSwapSize>0)
		{
			SAFEFREE(m_fltSwapBuf);
		}
		m_fltSwapBuf=(u8*)malloc(size);
		m_fltSwapSize=size;
	}

	memcpy(m_fltSwapBuf,src,size);

	dstPos=0;
	prevByte=0;
	lastDelta=0;
	for (i=0;i<chnNum;i++)
		for(j=i;j<size;j+=chnNum)
		{
			src[dstPos++]=(u8)(m_fltSwapBuf[j]-prevByte);
			prevByte=m_fltSwapBuf[j];
			/*src[dstPos++]=m_fltSwapBuf[j]-(prevByte+lastDelta);
			lastDelta=m_fltSwapBuf[j]-prevByte;
			prevByte=m_fltSwapBuf[j];*/
			//src[dstPos++]=m_fltSwapBuf[j];
		}

}


void Filters::Forward_RGB(u8 *src,u32 size,u32 width,u32 colorBits)
{
	if (size<512)
		return;

	u32 totalTest=0;

  u32 channelNum = colorBits / 8;

	if (m_fltSwapSize<size+(width+1)*channelNum) //no need boundry check
	{
		if (m_fltSwapSize>0)
		{
			SAFEFREE(m_fltSwapBuf);
		}
		m_fltSwapBuf=(u8*)malloc(size+(width+1)*channelNum);
		m_fltSwapSize=size;
	}

	memset(m_fltSwapBuf,0,(width+1)*channelNum);
	memcpy(m_fltSwapBuf+(width+1)*channelNum,src,size);
	u8 *newSrc=m_fltSwapBuf+(width+1)*channelNum;
	u32 dstPos=0;


  for (u32 i = 0; i<size - channelNum; i += channelNum)
	{
		u8 G=newSrc[i+1];
		newSrc[i]-=G/4;
		newSrc[i+2]-=G*3/4;
	}

	for(u32 i=0;i<channelNum;i++)
	{
		u32 vLeft,vUpper,vUpperLeft;
		int vPredict;
// 		u32 pa,pb,pc;
		for(u32 j=i;j<size;j+=channelNum)
		{
			vLeft=newSrc[j-channelNum];
			vUpper=newSrc[j-channelNum*width];
			vUpperLeft=newSrc[j-channelNum*(width+1)];
			/*vPredict=(vLeft*3+vUpper*3+vUpperLeft*2)/8;
			pa=abs((int)vUpper-vUpperLeft);
			pb=abs((int)vLeft-vUpperLeft);
			if (pa<pb)
			vPredict=vLeft;
			else
			vPredict=vUpper;*/
			vPredict=((int)vLeft+vUpper-vUpperLeft);
			if (vPredict>255)
				vPredict=255;
			if (vPredict<0)
				vPredict=0;
			/*vPredict=((int)vLeft+vUpper-vUpperLeft);
			pa=abs(vPredict-vLeft);
			pb=abs(vPredict-vUpper);
			pc=abs(vPredict-vUpperLeft);
			if (pa<=pb && pa<=pc)
			vPredict=vLeft;
			else
			if (pb<=pc)
			vPredict=vUpper;
			else
			vPredict=vUpperLeft;*/

			src[dstPos++]=(u8)(vPredict-newSrc[j]);

			totalTest+=abs(newSrc[j]-vPredict);
		}
	}

	printf("size:%d --- %f\n",size,(float)totalTest/size);


}

void Filters::Inverse_RGB(u8 *src,u32 size,u32 /*width*/,u32 /*colorBits*/)
{
	if (size<512)
		return;

	if (m_fltSwapSize<size)
	{
		if (m_fltSwapSize>0)
		{
			SAFEFREE(m_fltSwapBuf);
		}
		m_fltSwapBuf=(u8*)malloc(size);
		m_fltSwapSize=size;
	}

	memcpy(m_fltSwapBuf,src,size);

// 	int channelNum=colorBits/8;

}



void Filters::Forward_Audio(u8 * /*src*/,u32 /*size*/,u32 /*width*/,u32 /*colorBits*/)
{
}

void Filters::Inverse_Audio(u8 * /*src*/, u32 /*size*/, u32 /*width*/, u32 /*colorBits*/)
{
}


u32 Filters::Foward_Dict(u8 *src,u32 size)
{
	if (size<16384) 
		return 0;

	if (m_fltSwapSize<size)
	{
		if (m_fltSwapSize>0)
		{
			SAFEFREE(m_fltSwapBuf);
		}
		m_fltSwapBuf=(u8*)malloc(size);
		m_fltSwapSize=size;
	}

	u8 *dst=m_fltSwapBuf;
	u32 i,j,treePos=0;
	u32 lastSymbol=0; 
	u32 dstSize=0;
	u32 idx;


	for(i=0;i<size-5;)
	{
		if (dstSize>m_fltSwapSize-16)
		{
			return 0;
		}
		if (src[i]>='a'&& src[i]<='z')
		{

			u32 matchSymbol=0,longestWord=0;
			treePos=0;
			for(j=0;;)
			{
				idx=src[i+j]-'a';
				if (idx<0 || idx>25)
					break;
				if (wordTree[treePos].next[idx]==0)
					break;

				treePos=wordTree[treePos].next[idx];
				j++;
				if (wordTree[treePos].symbol)  
				{
					matchSymbol=wordTree[treePos].symbol;
					longestWord=j;
				}
			}

			if (matchSymbol)
			{
				dst[dstSize++]=(u8)matchSymbol;
				i+=longestWord;
				continue;
			}
			lastSymbol=0;
			dst[dstSize++]=src[i];
			i++;
		}
		else
		{
			if (src[i]>=0x82)// && src[i]<maxSymbol)
			{
				dst[dstSize++]=254;
				dst[dstSize++]=src[i];
			}
			else
				dst[dstSize++]=src[i];

			lastSymbol=0;
			treePos=0;
			i++;
		}

	}

	for (;i<size;i++)
	{
		if (src[i]>=0x82)// && src[i]<maxSymbol)
		{
			dst[dstSize++]=254;
			dst[dstSize++]=src[i];
		}
		else
			dst[dstSize++]=src[i];
	}

	if (dstSize>size*0.82)
		return 0;

	memset(dst+dstSize,0x20,size-dstSize);
	memcpy(src,dst,size);
	//FILE *f=fopen("r:\\mid.txt","a+b");
	//fwrite(src,1,dstSize,f);
	//fclose(f);
	return 1;

}

void Filters::Inverse_Dict(u8 *src,u32 size)
{

	if (m_fltSwapSize<size)
	{
		if (m_fltSwapSize>0)
		{
			SAFEFREE(m_fltSwapBuf);
		}
		m_fltSwapBuf=(u8*)malloc(size);
		m_fltSwapSize=size;
	}

	u8 *dst=m_fltSwapBuf;
	u32 i=0,j;
	u32 dstPos=0,idx;

	while(dstPos<size)
	{
		if (src[i]>=0x82 && src[i]<maxSymbol)	
		{
			idx=wordIndex[src[i]];
			for(j=0;wordList[idx][j];j++)
				dst[dstPos++]=wordList[idx][j];
		}
		else if (src[i]==254 && (i+1<size && src[i+1]>=0x82))// && src[i+1]<maxSymbol))
		{
			i++;
			dst[dstPos++]=src[i];
		}
		else 
			dst[dstPos++]=src[i];

		i++;
	}

	memcpy(src,dst,size);
}


void Filters::Inverse_Delta(u8 *src,u32 size,u32 chnNum)
{
	u32 dstPos,i,j,prevByte;
	u32 lastDelta;

	if (size<512) 
		return;

	if (m_fltSwapSize<size)
	{
		if (m_fltSwapSize>0)
		{
			SAFEFREE(m_fltSwapBuf);
		}
		m_fltSwapBuf=(u8*)malloc(size);
		m_fltSwapSize=size;
	}

	memcpy(m_fltSwapBuf,src,size);

	dstPos=0;
	prevByte=0;
	lastDelta=0;
	for (i=0;i<chnNum;i++)
		for(j=i;j<size;j+=chnNum)
		{
			src[j]=(u8)(m_fltSwapBuf[dstPos++]+prevByte);
			prevByte=src[j];
			/*src[j]=m_fltSwapBuf[dstPos++]+prevByte+lastDelta;
			lastDelta=src[j]-prevByte;
			prevByte=src[j];*/
		}
}


//void Filters::Forward_Audio4(u8 *src,u32 size)
//{
//
//	u32 dstPos,i,j,prevByte;
//	u32 chnNum=4;
//
//	if (m_fltSwapSize<size)
//	{
//		if (m_fltSwapSize>0)
//		{
//			SAFEFREE(m_fltSwapBuf);
//		}
//		m_fltSwapBuf=(u8*)malloc(size);
//		m_fltSwapSize=size;
//	}
//
//	memcpy(m_fltSwapBuf,src,size);
//
//	dstPos=0;
//	prevByte=0;
//	for (i=0;i<chnNum;i++)
//		for(j=i;j<size;j+=chnNum)
//		{
//			src[dstPos++]=m_fltSwapBuf[j]-prevByte;
//			prevByte=m_fltSwapBuf[j];
//		}
//		/*u8 *SrcData;
//		u8 *DestData;
//		int Channels=4;
//
//		if (m_fltSwapSize<size)
//		{
//		if (m_fltSwapSize>0)
//		{
//		SAFEFREE(m_fltSwapBuf);
//		}
//		m_fltSwapBuf=(u8*)malloc(size);
//		m_fltSwapSize=size;
//		}
//
//		memcpy(m_fltSwapBuf,src,size);
//
//		SrcData=m_fltSwapBuf;
//		DestData=src;
//
//		for (int CurChannel=0;CurChannel<Channels;CurChannel++)
//		{
//		unsigned int PrevByte=0,PrevDelta=0,Dif[7];
//		int D1=0,D2=0,D3;
//		int K1=0,K2=0,K3=0;
//		memset(Dif,0,sizeof(Dif));
//
//		for (int I=CurChannel,ByteCount=0;I<size;I+=Channels,ByteCount++)
//		{
//		D3=D2;
//		D2=PrevDelta-D1;
//		D1=PrevDelta;
//
//		unsigned int Predicted=8*PrevByte+K1*D1+K2*D2+K3*D3;
//		Predicted=(Predicted>>3) & 0xff;
//
//
//		unsigned int CurByte=SrcData[I];
//
//		PrevDelta=(signed char)(CurByte-PrevByte);
//		*DestData++=Predicted-CurByte;
//		PrevByte=CurByte;
//
//		int D=((signed char)(Predicted-CurByte))<<3;
//
//		Dif[0]+=abs(D);
//		Dif[1]+=abs(D-D1);
//		Dif[2]+=abs(D+D1);
//		Dif[3]+=abs(D-D2);
//		Dif[4]+=abs(D+D2);
//		Dif[5]+=abs(D-D3);
//		Dif[6]+=abs(D+D3);
//
//		if ((ByteCount & 0x1f)==0)
//		{
//		unsigned int MinDif=Dif[0],NumMinDif=0;
//		Dif[0]=0;
//		for (int J=1;J<sizeof(Dif)/sizeof(Dif[0]);J++)
//		{
//		if (Dif[J]<MinDif)
//		{
//		MinDif=Dif[J];
//		NumMinDif=J;
//		}
//		Dif[J]=0;
//		}
//		switch(NumMinDif)
//		{
//		case 1: if (K1>=-16) K1--; break;
//		case 2: if (K1 < 16) K1++; break;
//		case 3: if (K2>=-16) K2--; break;
//		case 4: if (K2 < 16) K2++; break;
//		case 5: if (K3>=-16) K3--; break;
//		case 6: if (K3 < 16) K3++; break;
//		}
//		}
//		}
//		}*/
//}



void Filters::E89init( void ) 
{
	cs = 0xFF;
	x0 = x1 = 0;
	i  = 0;
	k  = 5;
}

i32 Filters::E89cache_byte( i32 c ) 
{
	i32 d = cs&0x80 ? -1 : (u8)(x1);
	x1>>=8;
	x1|=(x0<<24);
	x0>>=8;
	x0|=(c <<24);
	cs<<=1; i++;
	return d;
}

u32 Filters::E89xswap( u32 x ) 
{
	x<<=7;
	return (x>>24)|((u8)(x>>16)<<8)|((u8)(x>>8)<<16)|((u8)(x)<<(24-7));
}

u32 Filters::E89yswap( u32 x ) 
{
	x = ((u8)(x>>24)<<7)|((u8)(x>>16)<<8)|((u8)(x>>8)<<16)|(x<<24);
	return x>>7;
}


i32 Filters::E89forward( i32 c ) 
{
	u32 x;
	if( i>=k ) {
		if( (x1&0xFE000000)==0xE8000000 ) {
			k = i+4;
			x= x0 - 0xFF000000;
			if( x<0x02000000 ) {
				x = (x+i) & 0x01FFFFFF;
				x = E89xswap(x);
				x0 = x + 0xFF000000;
			}
		}
	} 
	return E89cache_byte(c);
}

i32 Filters::E89inverse( i32 c ) 
{
	u32 x;
	if( i>=k ) {
		if( (x1&0xFE000000)==0xE8000000 ) {
			k = i+4;
			x = x0 - 0xFF000000;
			if( x<0x02000000 ) {
				x = E89yswap(x);
				x = (x-i) & 0x01FFFFFF;
				x0 = x + 0xFF000000;
			}
		}
	}
	return E89cache_byte(c);
}

i32 Filters::E89flush() 
{
	i32 d;
	if( cs!=0xFF ) {
		while( cs&0x80 ) E89cache_byte(0),++cs;
		d = E89cache_byte(0); ++cs;
		return d;
	} else {
		E89init();
		return -1;
	}
}


void Filters::Forward_E89( u8* src, u32 size ) 
{
  u32 i, j;
  i32 c;
	E89init();
	for( i=0,j=0; i<size; i++ ) {
		c = E89forward( src[i] );
		if( c>=0 ) src[j++]=(u8)c;
	}
	while( (c=E89flush())>=0 ) src[j++]=(u8)c;
}

void Filters::Inverse_E89( u8* src, u32 size ) 
{

  u32 i, j;
  i32 c;

	E89init();
	for( i=0,j=0; i<size; i++ ) {
		c = E89inverse( src[i] );
		if( c>=0 ) src[j++]=(u8)c;
	}
	while( (c=E89flush())>=0 ) src[j++]=(u8)c;
}
