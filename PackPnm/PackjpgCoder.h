// PackjpgCoder.h

#ifndef __PACKJPG_CODER_H
#define __PACKJPG_CODER_H

#include <stdint.h>

// #include "C/7zTypes.h"
// #include "CPP/Common/MyCom.h"
#include "CPP/7zip/ICoder.h"

namespace NCompress {
namespace NPackjpg {

extern unsigned char *ex_data;
extern int            ex_data_size;
extern int            ex_valid_size;
extern int            ex_next_offset;
extern int            ex_read_limit;

extern void reset_ex_buffers(int default_next_offset);
extern inline long long read_int(ISequentialInStream *inStream, UInt64 *inSizeProcessed);
extern inline HRESULT write_int(ISequentialOutStream *outStream, long long data_size, UInt64 *outSizeProcessed);
extern bool prepare_ex_data(ISequentialInStream *inStream, int needed);


/* -----------------------------------------------
	LIBBSC C=f E=2 B=20
	----------------------------------------------- */

HRESULT compress_replacement(ISequentialOutStream *outStream, uint8_t *data, size_t len, UInt64 *outSizeProcessed);
HRESULT uncompress_replacement(ISequentialInStream * inStream, ISequentialOutStream *outStream, UInt64 *inSizeProcessed, UInt64 *outSizeProcessed);


}}

#endif
