#ifndef _FILTERS_H
#define _FILTERS_H

#include "csc32Common.h"
#define MAX_WORDTREE_NODE_NUM 300 //Enough now!

class Filters
{
public:
	void Forward_E89( u8* src, u32 size);
	void Inverse_E89( u8* src, u32 size);
	~Filters();
	Filters();

	u32 Foward_Dict(u8 *src,u32 size);
	void Inverse_Dict(u8 *src,u32 size);

	void Forward_Delta(u8 *src,u32 size,u32 chnNum);
	void Inverse_Delta(u8 *src,u32 size,u32 chnNum);
	void Forward_RGB(u8 *src,u32 size,u32 width,u32 colorBits);
	void Inverse_RGB(u8 *src,u32 size,u32 width,u32 colorBits);
	void Forward_Audio(u8 *src,u32 size,u32 wavChannels,u32 sampleBits);
	void Inverse_Audio(u8 *src,u32 size,u32 wavChannels,u32 sampleBits);
	//void Forward_Audio4(u8 *src,u32 size);
	//void Inverse_Audio4(u8 *src,u32 size);


private:
	typedef struct
	{
		u32 next[26];
		u8 symbol;
	} CTreeNode;
	CTreeNode wordTree[MAX_WORDTREE_NODE_NUM];
	u32 nodeMum;
	u8 maxSymbol;
	//Used for DICT transformer. Words are stored in trees.

	u32 wordIndex[256];
	//Used for DICT untransformer.choose words by symbols.
	void MakeWordTree();  //Init the DICT transformer

	//Swap buffer for all filters. 
	u8 *m_fltSwapBuf;
	u32 m_fltSwapSize;

	/*
	Below is Shelwien's E89 filter
	*/
	void E89init(void);
	i32 E89cache_byte(i32 c);
	u32 E89xswap(u32 x);
	u32 E89yswap(u32 x);
	i32 E89forward(i32 c);
	i32 E89inverse(i32 c);
	i32 E89flush(void);

	u32 x0,x1;
	u32 i,k;
	u8 cs; // cache size, F8 - 5 bytes
};


#endif