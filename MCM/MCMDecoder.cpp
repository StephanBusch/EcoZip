// MCMDecoder.cpp

#define NOMINMAX

#include "CPP/7zip/Compress/StdAfx.h"

#include "C/Alloc.h"

#include "CPP/7zip/Common/StreamUtils.h"

#include "MCMDecoder.h"

namespace NCompress {
namespace NMCM {

CDecoder::CDecoder()
{
}

static void *SzAlloc(void *p, size_t size) { p = p; return MyAlloc(size); }
static void SzFree(void *p, void *address) { p = p; MyFree(address); }
static ISzAlloc g_Alloc = { SzAlloc, SzFree };

CDecoder::~CDecoder()
{
}

STDMETHODIMP CDecoder::SetDecoderProperties2(const Byte *prop, UInt32 size)
{
  if (size < 8)
    return E_INVALIDARG;
  int offset = 0;
  options_.mem_usage_   = (size_t)*(prop + offset++);
  options_.comp_level_  = (CompLevel)*(prop + offset++);
  options_.filter_type_ = (FilterType)*(prop + offset++);
  options_.lzp_type_    = (LZPType)*(prop + offset++);
  block_size            = (*(UInt32 *)(prop + offset)) * MB;
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

STDMETHODIMP CDecoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
  const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo * progress)
{
  UInt64 inSizeProcessed = 0, outSizeProcessed = 0;

  SetOutStreamSize(outSize);

  major_version_ = (uint16_t)read_int(inStream, &inSizeProcessed);
  minor_version_ = (uint16_t)read_int(inStream, &inSizeProcessed);

  std::vector<uint8_t> plain_buffer;
  WriteVectorStream wvs_plain(&plain_buffer);

  readBlocks(inStream, &inSizeProcessed);
  size_t compressed_size = (size_t)read_int(inStream, &inSizeProcessed);
  std::vector<uint8_t> compressed_buffer;
  compressed_buffer.assign(compressed_size, 0);
  RINOK(ReadStream(inStream, compressed_buffer.data(), &compressed_size));
  inSizeProcessed += compressed_size;
  ReadMemoryStream rms_compressed(compressed_buffer.data(), compressed_buffer.data() + compressed_size);

  UInt64 inSizeDeompressed = 0;
  for (auto *block : blocks_.blocks_) {
    block->total_size_ = 0;
    for (auto&seg : block->segments_) {
      seg.stream_ = &wvs_plain;
      block->total_size_ += seg.total_size_;
    }
    size_t block_size = (size_t)rms_compressed.leb128Decode();
    FileSegmentStream segstream(&block->segments_, 0u);
    Algorithm *algo = &block->algorithm_;
    // Decompressing
    std::unique_ptr<Filter>filter(algo->createFilter(&segstream, nullptr));
    Stream *out_stream = &segstream;
    if (filter.get() != nullptr) out_stream = filter.get();
    std::unique_ptr<Compressor> comp(algo->createCompressor());
    comp->setOpt(opt_var_);
    {
      ProgressThread thr(&rms_compressed, &segstream, &inSizeDeompressed, &outSizeProcessed, progress);
      comp->decompress(&rms_compressed, out_stream, block_size);
      inSizeDeompressed += rms_compressed.tell();
    }
    if (filter.get() != nullptr) filter->flush();
  }

  RINOK(WriteStream(outStream, plain_buffer.data(), plain_buffer.size()));
  outSizeProcessed += plain_buffer.size();

  if (progress != NULL)
    RINOK(progress->SetRatioInfo(&inSizeProcessed, &outSizeProcessed));

  return S_OK;
}

#ifndef NO_READ_FROM_CODER

STDMETHODIMP CDecoder::Read(void *, UInt32 , UInt32 *)
{
  return S_OK;
}

#endif

} // namespace NMCM
} // namespace NCompress
