// Packmp3Coder.cpp

#include "CPP/7zip/Compress/StdAfx.h"

// #include "C/7zTypes.h"
// #include "CPP/Common/MyCom.h"
#include "CPP/7zip/ICoder.h"
#include "CPP/7zip/Common/StreamUtils.h"

#include "PackjpgCoder.h"

#include "C/7zTypes.h"
#include "CPP/7zip/Compress/libbsc/libbsc.h"
#include "CPP/7zip/Compress/libbsc/filters.h"
#include "CPP/7zip/Compress/libbsc/platform.h"
#include "CPP/7zip/Compress/libbsc/libbsc-prop.h"

#include <stdio.h>


namespace NCompress {
namespace NPackjpg {

unsigned char* ex_data        =  NULL;  // extra data
int            ex_data_size   =     0;  // size of extra data
int            ex_valid_size  =     0;  // size of valid extra data
int            ex_next_offset =    -1;  // offset of found mp3 file or next section
int            ex_read_limit  =     0;  // size of reading limit


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


/* ------------------- Replacement of extra data compression ----------------------- */

/* -----------------------------------------------
	LIBBSC C=f E=2 B=20
	----------------------------------------------- */

HRESULT compress_replacement(ISequentialOutStream *outStream, uint8_t *data, size_t len, UInt64 *outSizeProcessed)
{
  CLibbscProps props;

  // C=f
  props.paramSortingContexts = LIBBSC_CONTEXTS_FOLLOWING;

  // E=2
  // LIBBSC_CODER_QLFC_ADAPTIVE
  props.features |= PARAM_CODER_QLFC_ADAPTIVE;

  // B=20
  props.paramBlockSize = 20 * 1024 * 1024;

  UInt32 nReduceSize = (UInt32)len;

  UInt32 kMinBlockSize = 1 * 1024 * 1024;

  if (nReduceSize < props.paramBlockSize) {
    if (nReduceSize < kMinBlockSize)
      props.paramBlockSize = kMinBlockSize;
    else
      props.paramBlockSize = nReduceSize;
  }

  int segmentedBlock[256] = { 0 };

  if (bsc_init(props.paramFeatures()) != LIBBSC_NO_ERROR)
  {
    fprintf(stderr, "\nInternal program error, please contact the author!\n");
    return E_FAIL;
  }

#ifdef LIBBSC_OPENMP
  int numThreads = 1;
  if (paramEnableParallelProcessing)
  {
    numThreads = omp_get_max_threads();
    if (numThreads <= nBlocks) paramEnableMultiThreading = 0;
    if (numThreads >= nBlocks) numThreads = nBlocks;
  }
#endif

  int segmentationStart = 0, segmentationEnd = 0;

#ifdef LIBBSC_OPENMP
#pragma omp parallel num_threads(numThreads) if(numThreads > 1)
#endif
  {
    unsigned char * buffer = (unsigned char *)bsc_malloc(props.paramBlockSize + LIBBSC_HEADER_SIZE);
    unsigned char * buffer2 = (unsigned char *)bsc_malloc(props.paramBlockSize + LIBBSC_HEADER_SIZE);
    if (buffer == NULL || buffer2 == NULL)
      return E_OUTOFMEMORY;

    BSC_FILEOFFSET  blockOffset = 0;
    BSC_FILEOFFSET  prevBlockOffset = 0;
    BSC_FILEOFFSET  readPos = 0;
    //HRESULT res = SZ_OK;
    for (;;)
    {
      UInt32          dataSize = 0;

#ifdef LIBBSC_OPENMP
#pragma omp critical(input)
#endif
      {
        {
          UInt32 currentBlockSize = props.paramBlockSize;
          if (props.paramEnableSegmentation())
          {
            if (segmentationEnd - segmentationStart > 1) currentBlockSize = segmentedBlock[segmentationStart];
          }

          if (blockOffset < readPos) {
            UInt32 moveOffset = (UInt32)(blockOffset - prevBlockOffset);
            UInt32 remained = (UInt32)(readPos - blockOffset);
            memmove(buffer, buffer + moveOffset, remained);
            dataSize = (UInt32)min(props.paramBlockSize - remained, nReduceSize - readPos);
            memcpy(buffer + remained, data + readPos, dataSize);
            readPos += dataSize;
            dataSize += remained;
          }
          else {
            dataSize = (UInt32)min(props.paramBlockSize, nReduceSize - readPos);
            memcpy(buffer, data + readPos, dataSize);
            readPos += dataSize;
          }
          if (dataSize <= 0)
            break;
          if (props.paramEnableSegmentation()) {
            if (dataSize > currentBlockSize)
              dataSize = currentBlockSize;
          }

          if (props.paramEnableSegmentation())
          {
            bool bSegmentation = false;

            if (segmentationStart == segmentationEnd) bSegmentation = true;
            if ((segmentationEnd - segmentationStart == 1) &&
              ((int)dataSize != segmentedBlock[segmentationStart]))
              bSegmentation = true;

            if (bSegmentation)
            {
              segmentationStart = 0; segmentationEnd = bsc_detect_segments(buffer, dataSize, segmentedBlock, 256, props.paramFeatures());
              if (segmentationEnd <= LIBBSC_NO_ERROR)
              {
                switch (segmentationEnd)
                {
                case LIBBSC_NOT_ENOUGH_MEMORY:
                  return E_OUTOFMEMORY;
                default:
                  return E_FAIL;
                }
              }
            }

            int newDataSize = segmentedBlock[segmentationStart++];
            if ((int)dataSize != newDataSize)
            {
              //                   BSC_FILEOFFSET pos = blockOffset + newDataSize;
              //                   BSC_FSEEK(fInput, pos, SEEK_SET);
              dataSize = newDataSize;
            }
          }
        }
      }

      if (dataSize == 0) break;

      memcpy(buffer2, buffer, dataSize);

      signed char recordSize = 1;
      if (props.paramEnableReordering())
      {
        recordSize = (signed char)bsc_detect_recordsize(buffer2, dataSize, props.paramFeatures());
        if ((int)recordSize < LIBBSC_NO_ERROR)
        {
          switch (recordSize)
          {
          case LIBBSC_NOT_ENOUGH_MEMORY:
            return E_OUTOFMEMORY;
          default:
            return E_FAIL;
          }
        }
        if (recordSize > 1)
        {
          int result = bsc_reorder_forward(buffer2, dataSize, recordSize, props.paramFeatures());
          if (result != LIBBSC_NO_ERROR)
          {
            switch (result)
            {
            case LIBBSC_NOT_ENOUGH_MEMORY:
              return E_OUTOFMEMORY;
            default:
              return E_FAIL;
            }
          }
        }
      }

      signed char sortingContexts = props.paramSortingContexts;
      if ((int)props.paramSortingContexts == LIBBSC_CONTEXTS_AUTODETECT)
      {
        sortingContexts = (signed char)bsc_detect_contextsorder(buffer2, dataSize, props.paramFeatures());
        if (sortingContexts < LIBBSC_NO_ERROR)
        {
          switch (sortingContexts)
          {
          case LIBBSC_NOT_ENOUGH_MEMORY:
            return E_OUTOFMEMORY;
          default:
            return E_FAIL;
          }
        }
      }
      if (sortingContexts == LIBBSC_CONTEXTS_PRECEDING)
      {
        int result = bsc_reverse_block(buffer2, dataSize, props.paramFeatures());
        if (result != LIBBSC_NO_ERROR)
          return E_FAIL;
      }

      int blockSize = bsc_compress(buffer2, buffer2, dataSize,
        props.paramLZPHashSize, props.paramLZPMinLen, props.paramBlockSorter,
        props.paramCoder(), props.paramFeatures());
      if (blockSize == LIBBSC_NOT_COMPRESSIBLE)
      {
#ifdef LIBBSC_OPENMP
#pragma omp critical(input)
#endif
        {
          sortingContexts = LIBBSC_CONTEXTS_FOLLOWING; recordSize = 1;

//           BSC_FILEOFFSET pos = blockOffset;
//           BSC_FSEEK(fInput, blockOffset, SEEK_SET);
//           RINOK(read(inStream, buffer, dataSize, &dataSize));
//           _inSizeProcessed = blockOffset + dataSize;
//           BSC_FSEEK(fInput, pos, SEEK_SET);
//           memcpy(buffer2, buffer, dataSize);
        }

        blockSize = bsc_store(buffer, buffer2, dataSize, props.paramFeatures());
      }
      if (blockSize < LIBBSC_NO_ERROR)
      {
#ifdef LIBBSC_OPENMP
#pragma omp critical(print)
#endif
        {
          switch (blockSize)
          {
          case LIBBSC_NOT_ENOUGH_MEMORY: fprintf(stderr, "\nNot enough memory! Please check README file for more information.\n"); break;
          case LIBBSC_NOT_SUPPORTED: fprintf(stderr, "\nSpecified compression method is not supported on this platform!\n"); break;
          case LIBBSC_GPU_ERROR: fprintf(stderr, "\nGeneral GPU failure! Please check README file for more information.\n"); break;
          case LIBBSC_GPU_NOT_SUPPORTED: fprintf(stderr, "\nYour GPU is not supported! Please check README file for more information.\n"); break;
          case LIBBSC_GPU_NOT_ENOUGH_MEMORY: fprintf(stderr, "\nNot enough GPU memory! Please check README file for more information.\n"); break;

          default: fprintf(stderr, "\nInternal program error, please contact the author!\n");
          }
          return E_FAIL;
        }
      }

#ifdef LIBBSC_OPENMP
#pragma omp critical(output)
#endif
      {
        Byte next = 1;
        RINOK(WriteStream(outStream, &next, 1));
        *outSizeProcessed += 1;

//         BSC_BLOCK_HEADER header = { blockOffset, recordSize, sortingContexts };
//         RINOK(outStream->Write(&header, sizeof(BSC_BLOCK_HEADER), &processedSize));

        write_int(outStream, blockOffset, outSizeProcessed);
        write_int(outStream, recordSize, outSizeProcessed);
        write_int(outStream, sortingContexts, outSizeProcessed);

        RINOK(WriteStream(outStream, buffer2, blockSize));
        *outSizeProcessed += blockSize;

        prevBlockOffset = blockOffset;
        blockOffset = blockOffset + dataSize;
      }
    }

    bsc_free(buffer2);
    bsc_free(buffer);
  }

  {
    Byte next = 0;
    RINOK(WriteStream(outStream, &next, 1));
    *outSizeProcessed += 1;
  }
  return S_OK;
}


/* -----------------------------------------------
	LIBBSC C=f E=2 B=20
	----------------------------------------------- */

HRESULT uncompress_replacement(ISequentialInStream * inStream, ISequentialOutStream *outStream, UInt64 *inSizeProcessed, UInt64 *outSizeProcessed)
{
  UInt32 _inSize;
  CLibbscProps props;

  // C=f
  props.paramSortingContexts = LIBBSC_CONTEXTS_FOLLOWING;

  // E=2
  // LIBBSC_CODER_QLFC_ADAPTIVE
  props.features |= PARAM_CODER_QLFC_ADAPTIVE;

  // B=20
  props.paramBlockSize = 20 * 1024 * 1024;

  if (bsc_init(props.paramFeatures()) != LIBBSC_NO_ERROR)
  {
    //fprintf(stderr, "\nInternal program error, please contact the author!\n");
    return E_FAIL;
  }

#ifdef LIBBSC_OPENMP

  int numThreads = 1;
  if (paramEnableParallelProcessing)
  {
    numThreads = omp_get_max_threads();
    if (numThreads <= nBlocks) paramEnableMultiThreading = 0;
    if (numThreads >= nBlocks) numThreads = nBlocks;
  }

#pragma omp parallel num_threads(numThreads) if(numThreads > 1)
#endif
  {
    int bufferSize = -1; unsigned char * buffer = NULL;

    for (;;)
    {
      BSC_FILEOFFSET  blockOffset = 0;

      signed char     sortingContexts = 0;
      signed char     recordSize = 0;
      int             blockSize = 0;
      int             dataSize = 0;

#ifdef LIBBSC_OPENMP
#pragma omp critical(input)
#endif
      {
        Byte next = 0;
        if (!prepare_ex_data(inStream, 1))
          break;
        _inSize = 1;
        next = ex_data[ex_next_offset++];
        *inSizeProcessed += 1;
        if (next == 0)
          break;

        BSC_BLOCK_HEADER header = { 0, 0, 0 };
        header.blockOffset = read_int(inStream, inSizeProcessed);
        header.recordSize = (char)read_int(inStream, inSizeProcessed);
        header.sortingContexts = (char)read_int(inStream, inSizeProcessed);

        recordSize = header.recordSize;
        if (recordSize < 1)
        {
          fprintf(stderr, "\nThis is not bsc archive or invalid compression method!\n");
          return E_FAIL;
        }

        sortingContexts = header.sortingContexts;
        if ((sortingContexts != LIBBSC_CONTEXTS_FOLLOWING) && (sortingContexts != LIBBSC_CONTEXTS_PRECEDING))
        {
          fprintf(stderr, "\nThis is not bsc archive or invalid compression method!\n");
          return E_FAIL;
        }

        blockOffset = (BSC_FILEOFFSET)header.blockOffset;

        unsigned char *bscBlockHeader;

        _inSize = LIBBSC_HEADER_SIZE;
        if (!prepare_ex_data(inStream, _inSize))
          break;
        bscBlockHeader = ex_data + ex_next_offset;
        ex_next_offset += _inSize;
        *inSizeProcessed += _inSize;

        if (bsc_block_info(bscBlockHeader, LIBBSC_HEADER_SIZE, &blockSize, &dataSize, props.paramFeatures()) != LIBBSC_NO_ERROR)
        {
          fprintf(stderr, "\nThis is not bsc archive or invalid compression method!\n");
          return E_FAIL;
        }

        if ((blockSize > bufferSize) || (dataSize > bufferSize))
        {
          if (blockSize > bufferSize) bufferSize = blockSize;
          if (dataSize > bufferSize) bufferSize = dataSize;

          if (buffer != NULL) bsc_free(buffer); buffer = (unsigned char *)bsc_malloc(bufferSize);
        }

        if (buffer == NULL)
        {
          fprintf(stderr, "\nNot enough memory! Please check README file for more information.\n");
          return E_FAIL;
        }

        memcpy(buffer, bscBlockHeader, LIBBSC_HEADER_SIZE);

        _inSize = blockSize - LIBBSC_HEADER_SIZE;
        if (!prepare_ex_data(inStream, _inSize))
          break;
        memcpy(buffer + LIBBSC_HEADER_SIZE, ex_data + ex_next_offset, _inSize);
        ex_next_offset += _inSize;
        *inSizeProcessed += _inSize;
      }


      if (dataSize == 0) break;

      int result = bsc_decompress(buffer, blockSize, buffer, dataSize, props.paramFeatures());
      if (result < LIBBSC_NO_ERROR)
      {
#ifdef LIBBSC_OPENMP
#pragma omp critical(print)
#endif
        {
          switch (result)
          {
          case LIBBSC_DATA_CORRUPT: fprintf(stderr, "\nThe compressed data is corrupted!\n"); break;
          case LIBBSC_NOT_ENOUGH_MEMORY: fprintf(stderr, "\nNot enough memory! Please check README file for more information.\n"); break;
          case LIBBSC_GPU_ERROR: fprintf(stderr, "\nGeneral GPU failure! Please check README file for more information.\n"); break;
          case LIBBSC_GPU_NOT_SUPPORTED: fprintf(stderr, "\nYour GPU is not supported! Please check README file for more information.\n"); break;
          case LIBBSC_GPU_NOT_ENOUGH_MEMORY: fprintf(stderr, "\nNot enough GPU memory! Please check README file for more information.\n"); break;

          default: fprintf(stderr, "\nInternal program error, please contact the author!\n");
          }
          return E_FAIL;
        }
      }

      if (sortingContexts == LIBBSC_CONTEXTS_PRECEDING)
      {
        result = bsc_reverse_block(buffer, dataSize, props.paramFeatures());
        if (result != LIBBSC_NO_ERROR)
        {
#ifdef LIBBSC_OPENMP
#pragma omp critical(print)
#endif
          {
            fprintf(stderr, "\nInternal program error, please contact the author!\n");
            return E_FAIL;
          }
        }
      }

      if (recordSize > 1)
      {
        result = bsc_reorder_reverse(buffer, dataSize, recordSize, props.paramFeatures());
        if (result != LIBBSC_NO_ERROR)
        {
#ifdef LIBBSC_OPENMP
#pragma omp critical(print)
#endif
          {
            switch (result)
            {
            case LIBBSC_NOT_ENOUGH_MEMORY: fprintf(stderr, "\nNot enough memory! Please check README file for more information.\n"); break;
            default: fprintf(stderr, "\nInternal program error, please contact the author!\n");
            }
            return E_FAIL;
          }
        }
      }

#ifdef LIBBSC_OPENMP
#pragma omp critical(output)
#endif
      {
// 				if (BSC_FSEEK(fOutput, blockOffset, SEEK_SET))
// 				{
// 					fprintf(stderr, "\nIO error on file: %s!\n", argv[3]);
//          return E_FAIL;
// 				}

        UInt32 processedSize = 0;
        RINOK(WriteStream(outStream, buffer, dataSize));
        *outSizeProcessed += dataSize;
      }
    }
  }
  return S_OK;
}


}}

