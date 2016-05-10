// Packmp3Coder.h

#ifndef __PACKMP3_CODER_H
#define __PACKMP3_CODER_H

#include "../../../C/7zTypes.h"
// #include "libbsc/libbsc.h"
// #include "libbsc/filters.h"
// #include "libbsc/platform.h"

#include "../../Common/MyCom.h"

#include "../ICoder.h"

// #define PARAM_CODER_QLFC_ADAPTIVE          0x00000100
// 
// #define PARAM_FEATURE_FASTMODE             0x00000001
// #define PARAM_FEATURE_MULTITHREADING       0x00000002
// #define PARAM_FEATURE_LARGEPAGES           0x00000004
// #define PARAM_FEATURE_CUDA                 0x00000008
// #define PARAM_FEATURE_PARALLEL_PROCESSING  0x00000010
// #define PARAM_FEATURE_SEGMENTATION         0x00000020
// #define PARAM_FEATURE_REORDERING           0x00000040
// #define PARAM_FEATURE_LZP                  0x00000080

#pragma pack(push, 1)

// typedef struct BSC_BLOCK_HEADER
// {
// 	long long       blockOffset;
// 	signed char     recordSize;
// 	signed char     sortingContexts;
// } BSC_BLOCK_HEADER;

// typedef struct _CLibbscProps
// {
//   UInt32 features;
//   UInt32 paramBlockSize;
//   Byte   paramBlockSorter;
//   Byte   paramSortingContexts;
//   Byte   paramLZPHashSize;
//   Byte   paramLZPMinLen;
// 
//   _CLibbscProps() {
//     features = 0;
//     paramBlockSize = BSC_BLOCK_SIZE;
//     paramBlockSorter = LIBBSC_BLOCKSORTER_BWT;
//     //paramCoder = LIBBSC_CODER_QLFC_STATIC;
//     paramSortingContexts = LIBBSC_CONTEXTS_FOLLOWING;
// 
//     features |= PARAM_FEATURE_FASTMODE;
//     features |= PARAM_FEATURE_MULTITHREADING;
//     //paramEnableLargePages = 0;
//     //paramEnableCUDA = 0;
//     features |= PARAM_FEATURE_PARALLEL_PROCESSING;
//     //paramEnableSegmentation = 0;
//     //paramEnableReordering = 0;
// #if 0
//     //paramEnableLZP = 0;
//     paramLZPHashSize = 0;
//     paramLZPMinLen = 0;
// #else
//     features |= PARAM_FEATURE_LZP;
//     paramLZPHashSize = LIBBSC_DEFAULT_LZPHASHSIZE;
//     paramLZPMinLen = LIBBSC_DEFAULT_LZPMINLEN;
// #endif
//   }
// 
//   UInt32 paramCoder() {
//     return ((features & PARAM_CODER_QLFC_ADAPTIVE) == 0) ?
//     LIBBSC_CODER_QLFC_STATIC : LIBBSC_CODER_QLFC_ADAPTIVE;
//   }
//   bool paramEnableFastMode() { return (features & PARAM_FEATURE_FASTMODE) != 0; }
//   bool paramEnableMultiThreading() { return (features & PARAM_FEATURE_MULTITHREADING) != 0; }
//   bool paramEnableLargePages() { return (features & PARAM_FEATURE_LARGEPAGES) != 0; }
//   bool paramEnableCUDA() { return (features & PARAM_FEATURE_CUDA) != 0; }
//   bool paramEnableParallelProcessing() { return (features & PARAM_FEATURE_PARALLEL_PROCESSING) != 0; }
//   bool paramEnableSegmentation() { return (features & PARAM_FEATURE_SEGMENTATION) != 0; }
//   bool paramEnableReordering() { return (features & PARAM_FEATURE_REORDERING) != 0; }
//   bool paramEnableLZP() { return (features & PARAM_FEATURE_LZP) != 0; }
// 
//   int paramFeatures()
//   {
//     int features =
//       (paramEnableFastMode() ? LIBBSC_FEATURE_FASTMODE : LIBBSC_FEATURE_NONE) |
//       (paramEnableMultiThreading() ? LIBBSC_FEATURE_MULTITHREADING : LIBBSC_FEATURE_NONE) |
//       (paramEnableLargePages() ? LIBBSC_FEATURE_LARGEPAGES : LIBBSC_FEATURE_NONE) |
//       (paramEnableCUDA() ? LIBBSC_FEATURE_CUDA : LIBBSC_FEATURE_NONE)
//       ;
// 
//     return features;
//   }
// } CLibbscProps;

#pragma pack(pop)

namespace NCompress {
namespace NPackmp3 {

class CEncoder:
  public ICompressCoder,
  public ICompressSetCoderProperties,
  public ICompressWriteCoderProperties,
  public CMyUnknownImp
{
//   CLibbscProps props;
public:
  MY_UNKNOWN_IMP2(ICompressSetCoderProperties, ICompressWriteCoderProperties)
 
  STDMETHOD(Code)(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);
  STDMETHOD(SetCoderProperties)(const PROPID *propIDs, const PROPVARIANT *props, UInt32 numProps);
  STDMETHOD(WriteCoderProperties)(ISequentialOutStream *outStream);

  CEncoder();
  virtual ~CEncoder();
};

class CDecoder:
  public ICompressCoder,
  public ICompressSetDecoderProperties2,
  public ICompressGetInStreamProcessedSize,
  #ifndef NO_READ_FROM_CODER
  public ICompressSetInStream,
  public ICompressSetOutStreamSize,
  public ISequentialInStream,
  #endif
  public CMyUnknownImp
{
  CMyComPtr<ISequentialInStream> _inStream;
//   CLibbscProps props;
  UInt32 _inSize;
  bool _outSizeDefined;
  UInt64 _outSize;
  UInt64 _inSizeProcessed;
  UInt64 _outSizeProcessed;
public:

  #ifndef NO_READ_FROM_CODER
  MY_UNKNOWN_IMP5(
      ICompressSetDecoderProperties2,
      ICompressGetInStreamProcessedSize,
      ICompressSetInStream,
      ICompressSetOutStreamSize,
      ISequentialInStream)
  #else
  MY_UNKNOWN_IMP2(
      ICompressSetDecoderProperties2,
      ICompressGetInStreamProcessedSize)
  #endif

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *_inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);

  STDMETHOD(SetDecoderProperties2)(const Byte *data, UInt32 size);

  STDMETHOD(GetInStreamProcessedSize)(UInt64 *value);

  STDMETHOD(SetInStream)(ISequentialInStream *inStream);
  STDMETHOD(ReleaseInStream)();
  STDMETHOD(SetOutStreamSize)(const UInt64 *outSize);

  #ifndef NO_READ_FROM_CODER
  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);
  #endif

  CDecoder();
  virtual ~CDecoder();

};

}}

#endif
