// LibbscDecoder.cpp

#include "StdAfx.h"

#include "../../../C/Alloc.h"

#include "../Common/StreamUtils.h"

#include "LibbscCoder.h"

#include <stdio.h>

namespace NCompress {
  namespace NLibbsc {

    CDecoder::CDecoder() : _outSizeDefined(false)
    {
    }

    CDecoder::~CDecoder()
    {
    }

    STDMETHODIMP CDecoder::SetDecoderProperties2(const Byte *prop, UInt32 size)
    {
      const UInt32 kPropSize = 5;
      if (size < kPropSize)
        return E_INVALIDARG;
      Byte props[kPropSize];
      _props.features = props[0];
      _props.paramBlockSize = props[1] * (1024 * 1024);
      _props.features |= (((props[2] >> 5) & 0x7) == LIBBSC_CODER_QLFC_STATIC) ? 0 : PARAM_CODER_QLFC_ADAPTIVE;
      _props.paramBlockSorter = (props[2] & 0x1f);
      _props.paramLZPMinLen = props[3];
      _props.paramLZPHashSize = props[4];
      return S_OK;
    }

    STDMETHODIMP CDecoder::GetInStreamProcessedSize(UInt64 *value) { *value = _inSizeProcessed; return S_OK; }
    STDMETHODIMP CDecoder::SetInStream(ISequentialInStream *inStream) { _inStream = inStream; return S_OK; }
    STDMETHODIMP CDecoder::ReleaseInStream() { _inStream.Release(); return S_OK; }

    STDMETHODIMP CDecoder::SetOutStreamSize(const UInt64 *outSize)
    {
      _outSizeDefined = (outSize != NULL);
      if (_outSizeDefined)
        _outSize = *outSize;

      _inSizeProcessed = _outSizeProcessed = 0;
      return S_OK;
    }

    static HRESULT read(ISequentialInStream *inStream,
      void *data, UInt32 size, UInt32 *processedSize)
    {
      if (processedSize != NULL)
        *processedSize = 0;
      UInt32 inSize;
      while (size > 0) {
        RINOK(inStream->Read(data, size, &inSize));
        if (inSize == 0)
          return E_FAIL;
        data = (unsigned char *)data + inSize;
        size -= inSize;
        if (processedSize != NULL)
          *processedSize += inSize;
      }
      return S_OK;
    }

    static HRESULT write(ISequentialOutStream *outStream,
      void *data, UInt32 size, UInt32 *processedSize)
    {
      if (processedSize != NULL)
        *processedSize = 0;
      UInt32 outSize;
      while (size > 0) {
        RINOK(outStream->Write(data, size, &outSize));
        if (outSize == 0)
          return E_FAIL;
        data = (unsigned char *)data + outSize;
        size -= outSize;
        if (processedSize != NULL)
          *processedSize += outSize;
      }
      return S_OK;
    }

    STDMETHODIMP CDecoder::Code(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 * /* inSize */,
      const UInt64 *outSize, ICompressProgressInfo *progress)
    {
      SetOutStreamSize(outSize);

      if (bsc_init(_props.paramFeatures()) != LIBBSC_NO_ERROR)
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
            RINOK(inStream->Read(&next, 1, &_inSize));
            _inSizeProcessed += _inSize;
            if (next == 0)
              break;

            BSC_BLOCK_HEADER header = { 0, 0, 0 };
            RINOK(read(inStream, &header, sizeof(BSC_BLOCK_HEADER), &_inSize));
            if (_inSize == 0) break;
            _inSizeProcessed += _inSize;

            recordSize = header.recordSize;
            if (recordSize < 1)
            {
              fprintf(stderr, "\nThis is not bsc archive or invalid compression method!\n");
              exit(2);
            }

            sortingContexts = header.sortingContexts;
            if ((sortingContexts != LIBBSC_CONTEXTS_FOLLOWING) && (sortingContexts != LIBBSC_CONTEXTS_PRECEDING))
            {
              fprintf(stderr, "\nThis is not bsc archive or invalid compression method!\n");
              exit(2);
            }

            blockOffset = (BSC_FILEOFFSET)header.blockOffset;

            unsigned char bscBlockHeader[LIBBSC_HEADER_SIZE];

            RINOK(read(inStream, bscBlockHeader, LIBBSC_HEADER_SIZE, &_inSize));
            _inSizeProcessed += _inSize;

            if (bsc_block_info(bscBlockHeader, LIBBSC_HEADER_SIZE, &blockSize, &dataSize, _props.paramFeatures()) != LIBBSC_NO_ERROR)
            {
              fprintf(stderr, "\nThis is not bsc archive or invalid compression method!\n");
              exit(2);
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
              exit(2);
            }

            memcpy(buffer, bscBlockHeader, LIBBSC_HEADER_SIZE);

            RINOK(read(inStream, buffer + LIBBSC_HEADER_SIZE, blockSize - LIBBSC_HEADER_SIZE, &_inSize));
            _inSizeProcessed += _inSize;
          }


          if (dataSize == 0) break;

          int result = bsc_decompress(buffer, blockSize, buffer, dataSize, _props.paramFeatures());
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
              exit(2);
            }
          }

          if (sortingContexts == LIBBSC_CONTEXTS_PRECEDING)
          {
            result = bsc_reverse_block(buffer, dataSize, _props.paramFeatures());
            if (result != LIBBSC_NO_ERROR)
            {
#ifdef LIBBSC_OPENMP
#pragma omp critical(print)
#endif
              {
                fprintf(stderr, "\nInternal program error, please contact the author!\n");
                exit(2);
              }
            }
          }

          if (recordSize > 1)
          {
            result = bsc_reorder_reverse(buffer, dataSize, recordSize, _props.paramFeatures());
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
                exit(2);
              }
            }
          }

#ifdef LIBBSC_OPENMP
#pragma omp critical(output)
#endif
          {
// 						if (BSC_FSEEK(fOutput, blockOffset, SEEK_SET))
// 						{
// 							fprintf(stderr, "\nIO error on file: %s!\n", argv[3]);
// 							exit(1);
// 						}

            UInt32 processedSize = 0;
            RINOK(write(outStream, buffer, dataSize, &processedSize));
            _outSizeProcessed += processedSize;
          }

          if (progress != NULL)
          {
            RINOK(progress->SetRatioInfo(&_inSizeProcessed, &_outSizeProcessed));
          }
        }
      }
      return S_OK;
    }

#ifndef NO_READ_FROM_CODER

    STDMETHODIMP CDecoder::Read(void *, UInt32 , UInt32 *processedSize)
    {
      if (processedSize)
        *processedSize = 0;
      return S_OK;
    }

#endif

  }
}
