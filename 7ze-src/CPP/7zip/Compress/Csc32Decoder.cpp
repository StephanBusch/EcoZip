// Csc32Decoder.cpp

#include "StdAfx.h"

#include "../../../C/Alloc.h"

#include "../Common/StreamUtils.h"

#include "Csc32Coder.h"

#include <stdio.h>

namespace NCompress {
  namespace NCsc32 {

    CDecoder::CDecoder()
    {
      int blockSize = 256 * KB;
      u32 dictSize = 32 * MB;
      u8 level = 2;
      bool useFilters = true;

      currSetting.outStreamBlockSize = blockSize;
#if 1
      gDictSize = currSetting.wndSize = dictSize;
      currSetting.SetDefaultMethod(level);
      currSetting.setEXEFilter(useFilters == true);
      currSetting.setDLTFilter(useFilters == true);
      currSetting.setTXTFilter(useFilters == true);
#else
      int hashBits = 23;
      int hashWidth = 16;
      int lzMode = 0;
      bool EXEFilter = false;
      bool DLTFilter = false;
      bool TXTFilter = false;
      currSetting.hashBits = hashBits;
      currSetting.hashWidth = hashWidth;
      currSetting.lzMode = lzMode;
      currSetting.setEXEFilter(EXEFilter);
      currSetting.setDLTFilter(DLTFilter);
      currSetting.setTXTFilter(TXTFilter);
#endif
    }

    static void *SzAlloc(void *p, size_t size) { p = p; return MyAlloc(size); }
    static void SzFree(void *p, void *address) { p = p; MyFree(address); }
    static ISzAlloc g_Alloc = { SzAlloc, SzFree };

    CDecoder::~CDecoder()
    {
    }

    STDMETHODIMP CDecoder::SetDecoderProperties2(const Byte *prop, UInt32 size)
    {
      if (size < CSC32_SETTING_LENGTH)
        return E_INVALIDARG;
      memcpy(&currSetting, prop, CSC32_SETTING_LENGTH);
      gDictSize = currSetting.wndSize;
      currSetting.SetDefaultMethod(currSetting.lzMode);
      currSetting.Refresh();
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

    STDMETHODIMP CDecoder::Code(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 * /* inSize */,
      const UInt64 *outSize, ICompressProgressInfo * /*progress*/)
    {
      SetOutStreamSize(outSize);

      //UInt64 _inSizeProcessed = 0, _outSizeProcessed = 0;
      UInt32 processedSize;
      u8 *writeBuffer;

      writeBuffer = (u8*)malloc(DefaultInBufferSize);
      if (writeBuffer == NULL) return CANT_ALLOC_MEM;

      FileDirectIO currIO;//the callback which is called by compressor
      CCsc compressor;        // Core

      read(inStream, &currSetting.outStreamBlockSize, sizeof(u32), &processedSize);
      read(inStream, &currSetting.wndSize, sizeof(u32), &processedSize);

      currSetting.Refresh();
      currIO.SetStream(inStream);
      currSetting.io = &currIO;
      compressor.Init(DECODE, currSetting);

      for (;;) {
        int returnValue = compressor.Decompress(writeBuffer, &processedSize);
        if (returnValue<0)
        {
          SAFEFREE(writeBuffer);
          return returnValue;
        }
        if (processedSize == 0)
          break;
        outStream->Write(writeBuffer, processedSize, &processedSize);
      }

      compressor.Destroy();
      SAFEFREE(writeBuffer);

      return S_OK;
    }

#ifndef NO_READ_FROM_CODER

    STDMETHODIMP CDecoder::Read(void *, UInt32 , UInt32 *)
    {
      return S_OK;
    }

#endif

  }
}
