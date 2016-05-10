// Packmp3Decoder.cpp

#include "StdAfx.h"

#include "../../../C/Alloc.h"

#include "../Common/StreamUtils.h"

#include "Packmp3Coder.h"
#include "./packjpg/bitops.h"
#include "./packjpg/pmp3tbl.h"
#include "./packjpg/packmp3.h"

namespace NCompress {
  namespace NPackmp3 {

    CDecoder::CDecoder() : _outSizeDefined(false)
    {
      _inSizeProcessed = _outSizeProcessed = 0;
    }

    CDecoder::~CDecoder()
    {
    }

    STDMETHODIMP CDecoder::SetDecoderProperties2(const Byte *prop, UInt32 size)
    {
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

    STDMETHODIMP CDecoder::Code(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 * /* inSize */,
      const UInt64 *outSize, ICompressProgressInfo *progress)
    {
      reset_ex_buffers(0);
      bool remained;
      for (;;) {
        int type = check_archive_type(inStream, &remained);
        if (!remained)
          break;
        if (type == 0)
        {
          // MP3 frames
          if (!uncompress_pmp(inStream, &_inSizeProcessed))
            return E_FAIL;
          write_mp3(outStream, &_outSizeProcessed);
        }
        else if (type == 1)
        {
          RINOK(uncompress_replacement(inStream, outStream,
            &_inSizeProcessed, &_outSizeProcessed));
        }

        if (progress != NULL)
        {
          RINOK(progress->SetRatioInfo(&_inSizeProcessed, &_outSizeProcessed));
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
