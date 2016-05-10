// LibbscEncoder.cpp

#include "StdAfx.h"

#include "../../../C/Alloc.h"

#include "../Common/CWrappers.h"
#include "../Common/StreamUtils.h"

#include "LibbscCoder.h"

#include <stdio.h>

namespace NCompress {

  namespace NLibbsc {

    static UInt32 kMinBlockSize = 1 * 1024 * 1024;

    CEncoder::CEncoder()
    {
    }

    CEncoder::~CEncoder()
    {
    }

    STDMETHODIMP CEncoder::SetCoderProperties(const PROPID *propIDs,
      const PROPVARIANT *coderProps, UInt32 numProps)
    {
      UInt32 nReduceSize = 0;
      for (UInt32 i = 0; i < numProps; i++)
      {
        const PROPVARIANT &prop = coderProps[i];
        PROPID propID = propIDs[i];
        switch (propID)
        {
        case NCoderPropID::kLibbscBlockSize:
          if (prop.vt != VT_UI4) return E_INVALIDARG;
          if ((prop.ulVal < 1) || (prop.ulVal > 1024))
            return E_INVALIDARG;
#if _WIN64
          _props.paramBlockSize = prop.ulVal * 1024 * 1024;
#else
          _props.paramBlockSize = (prop.ulVal > 32 ? 32 : prop.ulVal) * 1024 * 1024;
#endif
          break;
        case NCoderPropID::kLibbscBlockSorter:
          if (prop.vt != VT_UI4) return E_INVALIDARG;
          switch (prop.ulVal)
          {
          case 0: _props.paramBlockSorter = LIBBSC_BLOCKSORTER_BWT; break;

#ifdef LIBBSC_SORT_TRANSFORM_SUPPORT
          case 3: _props.paramBlockSorter = LIBBSC_BLOCKSORTER_ST3; break;
          case 4: _props.paramBlockSorter = LIBBSC_BLOCKSORTER_ST4; break;
          case 5: _props.paramBlockSorter = LIBBSC_BLOCKSORTER_ST5; break;
          case 6: _props.paramBlockSorter = LIBBSC_BLOCKSORTER_ST6; break;
          case 7: _props.paramBlockSorter = LIBBSC_BLOCKSORTER_ST7; _props.features |= PARAM_FEATURE_CUDA; break;
          case 8: _props.paramBlockSorter = LIBBSC_BLOCKSORTER_ST8; _props.features |= PARAM_FEATURE_CUDA; break;
#endif
          default: return E_INVALIDARG;
          }
          break;
        case NCoderPropID::kLibbscCoder:
          if (prop.vt != VT_UI4) return E_INVALIDARG;
          if (prop.ulVal != LIBBSC_CODER_QLFC_STATIC &&
            prop.ulVal != LIBBSC_CODER_QLFC_ADAPTIVE)
            return E_INVALIDARG;
          _props.features |= (prop.ulVal == LIBBSC_CODER_QLFC_STATIC) ? 0 : PARAM_CODER_QLFC_ADAPTIVE;
          break;
        case NCoderPropID::kLibbscSortingContexts:
          if (prop.vt != VT_BSTR) return E_INVALIDARG;
          switch (prop.bstrVal[0]) {
          case 'f':
          case 'F':
            _props.paramSortingContexts = LIBBSC_CONTEXTS_FOLLOWING; break;
          case 'p':
          case 'P':
            _props.paramSortingContexts = LIBBSC_CONTEXTS_PRECEDING; break;
          case 'a':
          case 'A':
            _props.paramSortingContexts = LIBBSC_CONTEXTS_AUTODETECT; break;
          default:
            return E_INVALIDARG;
          }
          break;
        case NCoderPropID::kLibbscEnableParallelProcessing:
          if (prop.vt != VT_BOOL) return E_INVALIDARG;
          if (prop.boolVal)
            _props.features |= PARAM_FEATURE_PARALLEL_PROCESSING | PARAM_FEATURE_MULTITHREADING;
          else
            _props.features &= !(PARAM_FEATURE_PARALLEL_PROCESSING | PARAM_FEATURE_MULTITHREADING);
          break;
//         case NCoderPropID::kLibbscEnableMultiThreading:
//           break;
        case NCoderPropID::kLibbscEnableFastMode:
          if (prop.vt != VT_BOOL) return E_INVALIDARG;
          if (prop.boolVal)
            _props.features |= PARAM_FEATURE_FASTMODE;
          else
            _props.features &= !PARAM_FEATURE_FASTMODE;
          break;
        case NCoderPropID::kLibbscEnableLargePages:
          if (prop.vt != VT_BOOL) return E_INVALIDARG;
          if (prop.boolVal)
            _props.features |= PARAM_FEATURE_LARGEPAGES;
          else
            _props.features &= !PARAM_FEATURE_LARGEPAGES;
          break;
        case NCoderPropID::kLibbscEnableCUDA:
          if (prop.vt != VT_BOOL) return E_INVALIDARG;
          if (prop.boolVal)
            _props.features |= PARAM_FEATURE_CUDA;
          else
            _props.features &= !PARAM_FEATURE_CUDA;
          break;
        case NCoderPropID::kLibbscEnableSegmentation:
          if (prop.vt != VT_BOOL) return E_INVALIDARG;
          if (prop.boolVal)
            _props.features |= PARAM_FEATURE_SEGMENTATION;
          else
            _props.features &= !PARAM_FEATURE_SEGMENTATION;
          break;
        case NCoderPropID::kLibbscEnableReordering:
          if (prop.vt != VT_BOOL) return E_INVALIDARG;
          if (prop.boolVal)
            _props.features |= PARAM_FEATURE_REORDERING;
          else
            _props.features &= !PARAM_FEATURE_REORDERING;
          break;
        case NCoderPropID::kLibbscEnableLZP:
          if (prop.vt != VT_BOOL) return E_INVALIDARG;
          if (prop.boolVal)
            _props.features |= PARAM_FEATURE_LZP;
          else
            _props.features &= !PARAM_FEATURE_LZP;
          break;
        case NCoderPropID::kLibbscLZPHashSize:
          if (prop.vt != VT_UI4) return E_INVALIDARG;
          if ((prop.ulVal < 10) || (prop.ulVal > 28))
            return E_INVALIDARG;
          _props.paramLZPHashSize = (Byte)prop.ulVal;
          break;
        case NCoderPropID::kLibbscLZPMinLen:
          if (prop.vt != VT_UI4) return E_INVALIDARG;
          if ((prop.ulVal < 4) || (prop.ulVal > 255))
            return E_INVALIDARG;
          _props.paramLZPMinLen = (Byte)prop.ulVal;
          break;
        case NCoderPropID::kReduceSize:
          nReduceSize = prop.ulVal; break;
        default:
          break;
        }
      }
      if (nReduceSize < _props.paramBlockSize) {
        if (nReduceSize < kMinBlockSize)
          _props.paramBlockSize = kMinBlockSize;
        else
          _props.paramBlockSize = nReduceSize;
      }
      return S_OK;
    }

    STDMETHODIMP CEncoder::WriteCoderProperties(ISequentialOutStream *outStream)
    {
      const UInt32 kPropSize = 5;
      Byte props[kPropSize];
      props[0] = (Byte)_props.features;
      props[1] = (Byte)((_props.paramBlockSize + 1024 * 1024 - 1) / (1024 * 1024));
      props[2] = (Byte)((_props.paramCoder() << 5) + (_props.paramBlockSorter & 0x1f));
      props[3] = (Byte)_props.paramLZPMinLen;
      props[4] = (Byte)_props.paramLZPHashSize;
      return WriteStream(outStream, props, kPropSize);
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
          return S_OK;
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

    STDMETHODIMP CEncoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 * /* inSize */, const UInt64 * /*outSize*/, ICompressProgressInfo *progress)
    {
      int segmentedBlock[256] = { 0 };

      if (bsc_init(_props.paramFeatures()) != LIBBSC_NO_ERROR)
      {
        fprintf(stderr, "\nInternal program error, please contact the author!\n");
        return E_FAIL;
      }

      UInt32 processedSize = 0;
      UInt64 _inSizeProcessed = 0, _outSizeProcessed = 0;

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
        unsigned char * buffer = (unsigned char *)bsc_malloc(_props.paramBlockSize + LIBBSC_HEADER_SIZE);
        unsigned char * buffer2 = (unsigned char *)bsc_malloc(_props.paramBlockSize + LIBBSC_HEADER_SIZE);
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
              UInt32 currentBlockSize = _props.paramBlockSize;
              if (_props.paramEnableSegmentation())
              {
                if (segmentationEnd - segmentationStart > 1) currentBlockSize = segmentedBlock[segmentationStart];
              }

              if (blockOffset < readPos) {
                UInt32 moveOffset = (UInt32)(blockOffset - prevBlockOffset);
                UInt32 remained = (UInt32)(readPos - blockOffset);
                memmove(buffer, buffer + moveOffset, remained);
                RINOK(read(inStream, buffer + remained, _props.paramBlockSize - remained, &dataSize));
                readPos += dataSize;
                dataSize += remained;
              }
              else {
                RINOK(read(inStream, buffer, _props.paramBlockSize, &dataSize));
                readPos += dataSize;
              }
              if (dataSize <= 0)
                break;
              if (_props.paramEnableSegmentation()) {
                if (dataSize > currentBlockSize)
                  dataSize = currentBlockSize;
              }

              if (_props.paramEnableSegmentation())
              {
                bool bSegmentation = false;

                if (segmentationStart == segmentationEnd) bSegmentation = true;
                if ((segmentationEnd - segmentationStart == 1) &&
                  ((int)dataSize != segmentedBlock[segmentationStart]))
                  bSegmentation = true;

                if (bSegmentation)
                {
                  segmentationStart = 0; segmentationEnd = bsc_detect_segments(buffer, dataSize, segmentedBlock, 256, _props.paramFeatures());
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

          _inSizeProcessed += dataSize;
//           _inSizeProcessed = blockOffset + dataSize;

          signed char recordSize = 1;
          if (_props.paramEnableReordering())
          {
            recordSize = (signed char)bsc_detect_recordsize(buffer2, dataSize, _props.paramFeatures());
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
              int result = bsc_reorder_forward(buffer2, dataSize, recordSize, _props.paramFeatures());
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

          signed char sortingContexts = _props.paramSortingContexts;
          if ((int)_props.paramSortingContexts == LIBBSC_CONTEXTS_AUTODETECT)
          {
            sortingContexts = (signed char)bsc_detect_contextsorder(buffer2, dataSize, _props.paramFeatures());
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
            int result = bsc_reverse_block(buffer2, dataSize, _props.paramFeatures());
            if (result != LIBBSC_NO_ERROR)
              return E_FAIL;
          }

          if (progress != NULL)
          {
            RINOK(progress->SetRatioInfo(&_inSizeProcessed, &_outSizeProcessed));
          }
          int blockSize = bsc_compress(buffer2, buffer2, dataSize,
            _props.paramLZPHashSize, _props.paramLZPMinLen, _props.paramBlockSorter,
            _props.paramCoder(), _props.paramFeatures());
          if (blockSize == LIBBSC_NOT_COMPRESSIBLE)
          {
#ifdef LIBBSC_OPENMP
#pragma omp critical(input)
#endif
            {
              sortingContexts = LIBBSC_CONTEXTS_FOLLOWING; recordSize = 1;

//               BSC_FILEOFFSET pos = blockOffset;
//               BSC_FSEEK(fInput, blockOffset, SEEK_SET);
//               RINOK(read(inStream, buffer, dataSize, &dataSize));
//               _inSizeProcessed = blockOffset + dataSize;
//               BSC_FSEEK(fInput, pos, SEEK_SET);
//               memcpy(buffer2, buffer, dataSize);
            }

            blockSize = bsc_store(buffer, buffer2, dataSize, _props.paramFeatures());
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
              exit(2);
            }
          }

#ifdef LIBBSC_OPENMP
#pragma omp critical(output)
#endif
          {
            Byte next = 1;
    			  RINOK(write(outStream, &next, 1, &processedSize));
            _outSizeProcessed += processedSize;

            BSC_BLOCK_HEADER header = { blockOffset, recordSize, sortingContexts };

			      RINOK(write(outStream, &header, sizeof(BSC_BLOCK_HEADER), &processedSize));
            _outSizeProcessed += processedSize;

            RINOK(write(outStream, buffer2, blockSize, &processedSize));
            _outSizeProcessed += processedSize;

            prevBlockOffset = blockOffset;
            blockOffset = blockOffset + dataSize;
          }

          if (progress != NULL)
          {
            RINOK(progress->SetRatioInfo(&_inSizeProcessed, &_outSizeProcessed));
          }
        }

		    bsc_free(buffer2);
		    bsc_free(buffer);
      }

      {
        Byte next = 0;
		    RINOK(write(outStream, &next, 1, &processedSize));
        if (processedSize != 1)
        {
          fprintf(stderr, "\nIO error on file (LIBBSC)!\n");
          exit(1);
        }
      }
      return S_OK;
    }
  }
}
