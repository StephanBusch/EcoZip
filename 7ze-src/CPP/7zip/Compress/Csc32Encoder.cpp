// Csc32Encoder.cpp

#include "StdAfx.h"

#include "../../../C/Alloc.h"

#include "../Common/CWrappers.h"
#include "../Common/StreamUtils.h"

#include "csc32Coder.h"
#include "csc32/csc32Common.h"
#include "csc32/csc32Crc32.h"

#include <stdio.h>

namespace NCompress {

  namespace NCsc32 {

    static UInt32 kMinBlockSize = 64 * KB;

    CEncoder::CEncoder()
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

    CEncoder::~CEncoder()
    {
    }

    STDMETHODIMP CEncoder::SetCoderProperties(const PROPID *propIDs,
      const PROPVARIANT *coderProps, UInt32 numProps)
    {
      u32 nDictSize = 0;
      for (UInt32 i = 0; i < numProps; i++)
      {
        const PROPVARIANT &prop = coderProps[i];
        PROPID propID = propIDs[i];
        switch (propID)
        {
        case NCoderPropID::kCsc32M:
          if (prop.vt != VT_UI4) return E_INVALIDARG;
          if ((prop.ulVal < 1) || (prop.ulVal > 4))
            return E_INVALIDARG;
          currSetting.SetDefaultMethod((u8)prop.ulVal);
          break;
        case NCoderPropID::kCsc32D:
          if (prop.vt != VT_UI4) return E_INVALIDARG;
          if ((prop.ulVal < 1) || (prop.ulVal > 512))
            return E_INVALIDARG;
          nDictSize = prop.ulVal * MB;
          break;
        case NCoderPropID::kCsc32DK:
          if (prop.vt != VT_UI4) return E_INVALIDARG;
          if ((prop.ulVal < 1) || (prop.ulVal > 512))
            return E_INVALIDARG;
          nDictSize = prop.ulVal * KB;
          break;
        case NCoderPropID::kCsc32FO:
          currSetting.setEXEFilter(false);
          currSetting.setDLTFilter(false);
          currSetting.setTXTFilter(false);
          break;
        case NCoderPropID::kReduceSize:
          if (prop.ulVal < gDictSize) {
            if (prop.ulVal < kMinBlockSize)
              nDictSize = kMinBlockSize;
            else
              nDictSize = prop.ulVal;
          }
          break;
        default:
          break;
        }
      }
      if (nDictSize != 0)
        gDictSize = currSetting.wndSize = nDictSize;
      return S_OK;
    }

    STDMETHODIMP CEncoder::WriteCoderProperties(ISequentialOutStream *outStream)
    {
      return WriteStream(outStream, &currSetting, CSC32_SETTING_LENGTH);
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

    STDMETHODIMP CEncoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 * /* inSize */, const UInt64 * /* outSize */, ICompressProgressInfo *progress)
    {
      UInt32 processedSize;
      UInt64 _inSizeProcessed = 0, _outSizeProcessed = 0;
      u8 *readBuffer = (u8*)malloc(DefaultInBufferSize);
      if (readBuffer == NULL)
        return E_FAIL;

      FileDirectIO currIO;//the callback which is called by compressor
      CCsc compressor;        // Core

      outStream->Write(&currSetting.outStreamBlockSize, sizeof(u32), &processedSize);
      outStream->Write(&currSetting.wndSize, sizeof(u32), &processedSize);

      currSetting.Refresh();
      currIO.SetStream(outStream);
      currSetting.io = &currIO;
      compressor.Init(ENCODE, currSetting);

      u32 curCrc32 = 0;

      for (;;)
      {
        RINOK(read(inStream, readBuffer, currSetting.InBufferSize, &processedSize));
        if (processedSize == 0)
          break;
        if (_inSizeProcessed == 0)
          compressor.CheckFileType(readBuffer, processedSize);

        _inSizeProcessed += processedSize;
        curCrc32 = crc32(curCrc32, readBuffer, processedSize);
        compressor.Compress(readBuffer, processedSize);
        _outSizeProcessed = compressor.GetCompressedSize();
        //u64 ratio = _outSizeProcessed * 100 / _inSizeProcessed;

        if (progress != NULL)
          RINOK(progress->SetRatioInfo(&_inSizeProcessed, &_outSizeProcessed));
      }
      compressor.WriteEOF();
      compressor.EncodeInt(curCrc32, 32);
      compressor.Flush();
      compressor.Destroy();

      return S_OK;
    }
  }
}
