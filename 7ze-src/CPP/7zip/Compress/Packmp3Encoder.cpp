// Packmp3Encoder.cpp

#include "StdAfx.h"

#include "../../../C/Alloc.h"

#include "../Common/CWrappers.h"
#include "../Common/StreamUtils.h"

#include "Packmp3Coder.h"
#include "./packjpg/bitops.h"
#include "./packjpg/pmp3tbl.h"
#include "./packjpg/packmp3.h"

namespace NCompress {

  namespace NPackmp3 {

    static void Encode2();

    CEncoder::CEncoder()
    {
    }

    CEncoder::~CEncoder()
    {
    }

    STDMETHODIMP CEncoder::SetCoderProperties(const PROPID *propIDs,
      const PROPVARIANT *coderProps, UInt32 numProps)
    {
      return S_OK;
    }

    STDMETHODIMP CEncoder::WriteCoderProperties(ISequentialOutStream *outStream)
    {
      return S_OK;
    }

    STDMETHODIMP CEncoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 * /* inSize */, const UInt64 * /*outSize*/, ICompressProgressInfo *progress)
    {
      int processedSize;
      UInt64 inSizeProcessed = 0, outSizeProcessed = 0;
      reset_ex_buffers(-1);
      for (;;) {
        HRESULT result = seek_mp3(inStream, &processedSize);
        if (FAILED(result))
          return result;
        if (result == S_OK)
        {
          if (processedSize != 0)
            compress_replacement(outStream, 0, &inSizeProcessed, &outSizeProcessed);
          result = read_mp3(inStream, &inSizeProcessed);
          if (FAILED(result))
            continue;
          if (result == S_OK) {
            analyze_frames();
            compress_mp3(outStream, &outSizeProcessed);
          }
          else {
            compress_replacement(outStream, 1, &inSizeProcessed, &outSizeProcessed);
          }
        }
        else
        {
          if (processedSize == 0)
            break;
          compress_replacement(outStream, 0, &inSizeProcessed, &outSizeProcessed);
        }

        if (progress != NULL)
        {
          RINOK(progress->SetRatioInfo(&inSizeProcessed, &outSizeProcessed));
        }
      }
      return S_OK;
    }
  }
}
