// Packmp3Coder.cpp

#include "CPP/7zip/Compress/StdAfx.h"

// #include "C/7zTypes.h"
// #include "CPP/Common/MyCom.h"
#include "CPP/7zip/ICoder.h"
#include "CPP/7zip/Common/StreamUtils.h"

#include "PackjpgCoder.h"

namespace NCompress {
namespace NPackjpg {

unsigned char* ex_data        =  NULL;  // extra data
int            ex_data_size   =     0;  // size of extra data
int            ex_valid_size  =     0;  // size of valid extra data
int            ex_next_offset =    -1;  // offset of found mp3 file or next section
int            ex_read_limit  =     0;  // size of reading limit
unsigned char* ex_backup      =  NULL;  // data backup for corrupted file
int            ex_back_size   =     0;  // size of data backup


#define FIRST_FRAME_AREA  1 * 1024 * 1024
#define INITIAL_EX_SIZE   FIRST_FRAME_AREA

void reset_ex_buffers(int default_next_offset)
{
  ex_valid_size = 0;
  ex_next_offset = default_next_offset;
  if (ex_data == NULL) {
    ex_data_size = FIRST_FRAME_AREA;
    ex_data = (unsigned char*)calloc(ex_data_size + 1, sizeof(char));
    if ((ex_data == NULL)) {
//       sprintf(errormessage, MEM_ERRMSG);
//       errorlevel = 2;
      return;
    }
  }
}


inline long long read_int(ISequentialInStream *inStream, UInt64 *inSizeProcessed)
{
  int sh = 0;
  long long rlt = 0;
  unsigned char b = 0;
  do {
    if (!prepare_ex_data(inStream, 1))
      break;
    b = ex_data[ex_next_offset++];
    rlt = rlt + ((b & 0x7F) << sh);
    *inSizeProcessed += 1;
    sh += 7;
  } while ((b & 0x80) != 0);
  return rlt;
}


inline HRESULT write_int(ISequentialOutStream *outStream, long long data_size, UInt64 *outSizeProcessed)
{
  do {
    unsigned char b = data_size & 0x7F;
    data_size >>= 7;
    if (data_size > 0)
      b |= 0x80;
    RINOK(WriteStream(outStream, &b, 1));
    *outSizeProcessed += 1;
  } while (data_size != 0);
  return S_OK;
}


bool prepare_ex_data(ISequentialInStream *inStream, int needed)
{
  size_t processedSize;
  if (ex_valid_size - ex_next_offset < needed) {
    if (ex_data_size - ex_next_offset < needed) {
      memmove(ex_data, ex_data + ex_next_offset, ex_valid_size - ex_next_offset);
      ex_valid_size -= ex_next_offset;
      ex_next_offset = 0;
    }
//     if (FAILED(str->Read(ex_data + ex_valid_size, needed - (ex_valid_size - ex_next_offset), &processedSize)))
    processedSize = ex_data_size - ex_valid_size;
    if (FAILED(ReadStream(inStream, ex_data + ex_valid_size, &processedSize)))
      return false;
    ex_valid_size += (int)processedSize;
    if (ex_valid_size - ex_next_offset < needed)
      return false;
  }
  return true;
}

}}

