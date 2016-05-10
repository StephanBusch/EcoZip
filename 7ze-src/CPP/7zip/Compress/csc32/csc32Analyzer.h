#ifndef _ANALYZER_H
#define _ANALYZER_H

#include "csc32Common.h"


class Analyzer
{
public:
	Analyzer();
	~Analyzer();
	u32 analyze(u8* src,u32 size);
	u32 analyzeHeader(u8 *src,u32 size,u32 *typeArg1,u32 *typeArg2,u32 *typeArg3);

private:
	u32 logTable[(MinBlockSize>>4)+1];
	i32 GetChnIdx(u8 *src,u32 size);
};


#endif

