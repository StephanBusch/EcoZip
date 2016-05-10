#ifndef _LZ_H
#define _LZ_H
#include "csc32Common.h"
#include "csc32Model.h"


class LZ
{
public:
	Model *m_model;
	int Init(u32 WindowSize,u32 operation,u32 hashBits,u32 hashWidth);
	int Init(u32 WindowSize,u32 operation);
	void EncodeNormal(u8 *src,u32 size,u32 lzMode);
	u32 CheckDuplicate(u8 *src,u32 size,u32 type);
	void DuplicateInsert(u8 *src,u32 size);
	void Reset(void);
	int Decode(u8 *dst,u32 *size,u32 sizeLimit);
	void Destroy(void);
	u32 m_WndCurrPos;


private:
	u32 m_operation;

	u32 m_Hash6Bits;
	u32 m_MaxCycles;

	u32 m_HashNum;
	u32 m_lazyParsing;

	u8  *m_Wnd;
	u32 *m_HT2;
	u32 *m_HT3;
	u32 *m_HT6;
	u32 *m_HT6Raw;
	u32 m_RepDist[4];
	u32 m_RepMatchLen;
	u32 m_LitCount,m_PassedCount;

	struct 
	{
		u32 total;
		struct 
		{
			PackType type;
			u32 len;
			u32 dist;
			u32 price;
		} candidate[32];
	} parsing[2];
	u32 m_ParsingPos;

	u32 m_WndSize;
	//u32 m_WndCurrPos;

	int LZMinBlock(u32 size);
	int LZMinBlockFast(u32 size);
	u32 LZMinBlockSkip(u32 size,u32 type);
	void InsertHash(u32 currHash3,u32 currHash6,u32 slideLen);
	void InsertHashFast(u32 currHash6,u32 slideLen);

	u32 currBlockEndPos;

// New LZ77 Algorithm===============================
	int LZMinBlockNew(u32 size,u32 TryLazy,u32 LazyStep,u32 GoodLen);
	void LZBackward(const u32 initWndPos,u32 start,u32 end);


	struct ParserAtom
	{
		u8 finalChoice;
		u32 finalLen;
		u32 fstate;
		u32 price;

		struct 
		{
			u32 startPos;
			u16 choiceNum;
		} choices;

		u32 repDist[4];

		struct
		{
			u16 fromPos;
			u8 choice;
			u32 fromLen;
		} backChoice;
	};

	struct MatchAtom
	{
		u32 pos;
		u16 len;
	};

	ParserAtom *parser;
	MatchAtom  *matchList;
	u32 matchListSize;

	u32 cHash2;
	u32 cHash3;
	u32 cHash6;
	void NewInsert1();
	void NewInsertN(u32 len);
	u32 FindMatch(u32 idx,u32 minMatchLen);
// New LZ77 Algorithm====================================
};




#endif
