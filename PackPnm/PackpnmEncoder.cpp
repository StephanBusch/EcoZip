// PackpnmEncoder.cpp

#include "CPP/7zip/Compress/StdAfx.h"

#include "C/Alloc.h"

#include "CPP/7zip/Common/CWrappers.h"
#include "CPP/7zip/Common/StreamUtils.h"

#include "PackpnmEncoder.h"
#include "ppnmtbl.h"
#include "PackjpgCoder.h"

namespace NCompress {
namespace NPackjpg {
namespace NPackpnm {

using namespace NCompress::NPackjpg;

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
  UInt64 inSizeProcessed = 0, outSizeProcessed = 0;
  bool error = false;
  aricoder *encoder;
  abytewriter *data_writer;

  reset_ex_buffers(0);

  for (;;) {
    abytewriter gaps(0);
    char *imghdr = NULL;

    reset_buffers();

    // parse PNM file header
    while (prepare_ex_data(inStream, 1)) {
      int cur_ex_offset = ex_next_offset;
      imghdr = scan_header(inStream);
      if (imghdr != NULL) {
        inSizeProcessed += ex_next_offset - cur_ex_offset;
        break;
      }
      gaps.write(ex_data[ex_next_offset++]);
      inSizeProcessed++;
    }
    if (gaps.getpos() > 0) {
      Byte next = 1;
      RINOK(WriteStream(outStream, &next, 1));
      outSizeProcessed++;

      compress_replacement(outStream, gaps.getptr(), gaps.getpos(), &outSizeProcessed);
      free(gaps.getptr());
    }

    if (imghdr == NULL)
      break;

    Byte next = 0;
    RINOK(WriteStream(outStream, &next, 1));
    outSizeProcessed++;

    // write PPN file header

    data_writer = new abytewriter(0);

    imghdr[0] = 'S';
    //   data_writer->write( imghdr, 1, ( subtype != S_BMP ) ? strlen( imghdr ) : INT_LE( imghdr + 0x0A ) );
    //   data_writer->write( ( void* ) &appversion, 1, 1 );

    // init arithmetic compression
    encoder = new aricoder(data_writer);

    // arithmetic encode image data (select method)
    switch (imgbpp) {
    case  1:
      if (!(ppn_encode_imgdata_mono(encoder, inStream, &inSizeProcessed))) error = true;
      break;
    case  4:
    case  8:
      if (subtype == S_BMP) {
        if (!(ppn_encode_imgdata_palette(encoder, inStream, &inSizeProcessed))) error = true;
      }
      else if (!(ppn_encode_imgdata_rgba(encoder, inStream, &inSizeProcessed))) error = true;
      break;
    case 16:
    case 24:
    case 32:
    case 48:
      if (!(ppn_encode_imgdata_rgba(encoder, inStream, &inSizeProcessed))) error = true;
      break;
    default: sprintf(errormessage, "%ibpp is not supported", imgbpp); error = true; break;
    }

    // finalize arithmetic compression
    delete(encoder);

    unsigned char * data = data_writer->getptr();
    int data_size = data_writer->getpos();
    delete (data_writer);

    // Write data to stream.

    // PMP magic number & version byte
    size_t hdrlen = (subtype != S_BMP) ? strlen(imghdr) : INT_LE(imghdr + 0x0A);
    if (FAILED(WriteStream(outStream, (unsigned char *)imghdr, hdrlen)) ||
      FAILED(WriteStream(outStream, (unsigned char *)&appversion, 1)))
    {
      free(data);
      return E_FAIL;
    }
    outSizeProcessed += hdrlen + 1;

    write_int(outStream, data_size, &outSizeProcessed);

    if (FAILED(WriteStream(outStream, data, data_size))) {
      free(data);
      return E_FAIL;
    }
    outSizeProcessed += data_size;

    free(data);

    if (progress != NULL)
      RINOK(progress->SetRatioInfo(&inSizeProcessed, &outSizeProcessed));
  }

  return S_OK;
}

/* ----------------------- End of main functions -------------------------- */

} // namespace NPackpnm
} // namespace NPackjpg
} // namespace NCompress
