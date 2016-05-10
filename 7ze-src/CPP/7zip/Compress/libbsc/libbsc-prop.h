#ifndef _LIBBSC_LIBBSC_PROP_H
#define _LIBBSC_LIBBSC_PROP_H

#define PARAM_CODER_QLFC_ADAPTIVE          0x00000100

#define PARAM_FEATURE_FASTMODE             0x00000001
#define PARAM_FEATURE_MULTITHREADING       0x00000002
#define PARAM_FEATURE_LARGEPAGES           0x00000004
#define PARAM_FEATURE_CUDA                 0x00000008
#define PARAM_FEATURE_PARALLEL_PROCESSING  0x00000010
#define PARAM_FEATURE_SEGMENTATION         0x00000020
#define PARAM_FEATURE_REORDERING           0x00000040
#define PARAM_FEATURE_LZP                  0x00000080

#pragma pack(push, 1)

#define LIBBSC_CONTEXTS_AUTODETECT   3
#define BSC_BLOCK_SIZE  (25 * 1024 * 1024)

typedef struct BSC_BLOCK_HEADER
{
	long long       blockOffset;
	signed char     recordSize;
	signed char     sortingContexts;
} BSC_BLOCK_HEADER;

typedef struct _CLibbscProps
{
  UInt32 features;
  UInt32 paramBlockSize;
  Byte   paramBlockSorter;
  Byte   paramSortingContexts;
  Byte   paramLZPHashSize;
  Byte   paramLZPMinLen;

  _CLibbscProps() {
    features = 0;
    paramBlockSize = BSC_BLOCK_SIZE;
    paramBlockSorter = LIBBSC_BLOCKSORTER_BWT;
    //paramCoder = LIBBSC_CODER_QLFC_STATIC;
    paramSortingContexts = LIBBSC_CONTEXTS_FOLLOWING;

    features |= PARAM_FEATURE_FASTMODE;
    features |= PARAM_FEATURE_MULTITHREADING;
    //paramEnableLargePages = 0;
    //paramEnableCUDA = 0;
    features |= PARAM_FEATURE_PARALLEL_PROCESSING;
    //paramEnableSegmentation = 0;
    //paramEnableReordering = 0;
#if 0
    //paramEnableLZP = 0;
    paramLZPHashSize = 0;
    paramLZPMinLen = 0;
#else
    features |= PARAM_FEATURE_LZP;
    paramLZPHashSize = LIBBSC_DEFAULT_LZPHASHSIZE;
    paramLZPMinLen = LIBBSC_DEFAULT_LZPMINLEN;
#endif
  }

  UInt32 paramCoder() {
    return ((features & PARAM_CODER_QLFC_ADAPTIVE) == 0) ?
    LIBBSC_CODER_QLFC_STATIC : LIBBSC_CODER_QLFC_ADAPTIVE;
  }
  bool paramEnableFastMode() { return (features & PARAM_FEATURE_FASTMODE) != 0; }
  bool paramEnableMultiThreading() { return (features & PARAM_FEATURE_MULTITHREADING) != 0; }
  bool paramEnableLargePages() { return (features & PARAM_FEATURE_LARGEPAGES) != 0; }
  bool paramEnableCUDA() { return (features & PARAM_FEATURE_CUDA) != 0; }
  bool paramEnableParallelProcessing() { return (features & PARAM_FEATURE_PARALLEL_PROCESSING) != 0; }
  bool paramEnableSegmentation() { return (features & PARAM_FEATURE_SEGMENTATION) != 0; }
  bool paramEnableReordering() { return (features & PARAM_FEATURE_REORDERING) != 0; }
  bool paramEnableLZP() { return (features & PARAM_FEATURE_LZP) != 0; }

  int paramFeatures()
  {
    int features =
      (paramEnableFastMode() ? LIBBSC_FEATURE_FASTMODE : LIBBSC_FEATURE_NONE) |
      (paramEnableMultiThreading() ? LIBBSC_FEATURE_MULTITHREADING : LIBBSC_FEATURE_NONE) |
      (paramEnableLargePages() ? LIBBSC_FEATURE_LARGEPAGES : LIBBSC_FEATURE_NONE) |
      (paramEnableCUDA() ? LIBBSC_FEATURE_CUDA : LIBBSC_FEATURE_NONE)
      ;

    return features;
  }
} CLibbscProps;

#pragma pack(pop)


#if defined(__GNUC__) && (defined(_GLIBCXX_USE_LFS) || defined(__MINGW32__))
#define BSC_FILEOFFSET off64_t
#elif defined(_MSC_VER) && _MSC_VER >= 1400
#define BSC_FILEOFFSET __int64
#else
#define BSC_FILEOFFSET long
#endif

#endif

/*-------------------------------------------------*/
/* End                               libbsc-prop.h */
/*-------------------------------------------------*/
