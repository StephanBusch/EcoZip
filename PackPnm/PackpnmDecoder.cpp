// PackpnmDecoder.cpp

#include "CPP/7zip/Compress/StdAfx.h"

#include "C/Alloc.h"

#include "CPP/7zip/Common/StreamUtils.h"

#include "PackpnmDecoder.h"
#include "ppnmtbl.h"
#include "PackjpgCoder.h"

namespace NCompress {
namespace NPackjpg {
namespace NPackpnm {

using namespace NCompress::NPackjpg;

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
  HRESULT hr;
  UInt64 inSizeProcessed = 0, outSizeProcessed = 0;
  char *imghdr = NULL;
  bool error = false;
  unsigned char ver;
  aricoder *decoder;

  reset_ex_buffers(0);

  for (;;) {
    reset_buffers();

    if (!prepare_ex_data(inStream, 1))
      break;
    Byte next = ex_data[ex_next_offset++];
    inSizeProcessed++;

    if (next == 0) {
      // parse PPN header
      int cur_ex_offset = ex_next_offset;
      imghdr = scan_header(inStream);
      if (imghdr == NULL) {
        errorlevel = 2;
        return E_FAIL;
      }
      inSizeProcessed += ex_next_offset - cur_ex_offset;

      // check packPNM version
      if (!prepare_ex_data(inStream, 1)) {
        errorlevel = 2;
        return E_FAIL;
      }
      ver = ex_data[ex_next_offset++];
      inSizeProcessed++;
      if (ver == 14) {
        endian_l = E_LITTLE; // bad (mixed endian!) (!!!)
        ver = 16; // compatibility hack for v1.4
      }
      if (ver != appversion) {
        sprintf(errormessage, "incompatible file, use v%i.%i", ver / 10, ver % 10);
        errorlevel = 2;
        return E_FAIL;
      }

      // write PNM file header
      imghdr[0] = (subtype == S_BMP) ? 'B' : (subtype == S_HDR) ? '#' : 'P';
      size_t hdrlen = (subtype != S_BMP) ? strlen(imghdr) : INT_LE(imghdr + 0x0A);
      hr = WriteStream(outStream, imghdr, hdrlen);
      if (hr == k_My_HRESULT_WritingWasCut)
        return S_OK;
      else
        RINOK(hr);
      outSizeProcessed += hdrlen;

      ex_read_limit = (int)read_int(inStream, &inSizeProcessed);
      int read_limit = ex_read_limit;

      // init arithmetic compression
      decoder = new aricoder(inStream);

      // arithmetic decode image data (select method)
      switch (imgbpp) {
      case  1:
        if (!(ppn_decode_imgdata_mono(decoder, outStream, &outSizeProcessed))) error = true;
        break;
      case  4:
      case  8:
        if (subtype == S_BMP) {
          if (!(ppn_decode_imgdata_palette(decoder, outStream, &outSizeProcessed))) error = true;
        }
        else if (!(ppn_decode_imgdata_rgba(decoder, outStream, &outSizeProcessed))) error = true;
        break;
      case 16:
      case 24:
      case 32:
      case 48:
        if (!(ppn_decode_imgdata_rgba(decoder, outStream, &outSizeProcessed))) error = true;
        break;
      default: error = true;
      }

      // finalize arithmetic decompression
      delete(decoder);
      inSizeProcessed += read_limit;

      // error flag set?
      if (error) return false;

      // write BMP junk data
      if (subtype == S_BMP) for (char bt = 0; outSizeProcessed < bmpsize;) {
        hr = WriteStream(outStream, &bt, 1);
        if (hr == k_My_HRESULT_WritingWasCut)
          return S_OK;
        else
          RINOK(hr);
        ++outSizeProcessed;
      }
    }
    else {
      hr = uncompress_replacement(inStream, outStream, &inSizeProcessed, &outSizeProcessed);
      if (hr == k_My_HRESULT_WritingWasCut)
        return S_OK;
      else
        RINOK(hr);
    }

    if (progress != NULL)
      RINOK(progress->SetRatioInfo(&inSizeProcessed, &outSizeProcessed));
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

} // namespace NPackpnm
} // namespace NPackjpg
} // namespace NCompress
