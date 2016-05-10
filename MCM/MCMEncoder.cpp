// MCMEncoder.cpp

#define NOMINMAX

#include "CPP/7zip/Compress/StdAfx.h"

#include "C/Alloc.h"

#include "CPP/7zip/Common/CWrappers.h"
#include "CPP/7zip/Common/StreamUtils.h"

#include "MCMEncoder.h"
#include "PackjpgCoder.h"

namespace NCompress {
namespace NMCM {

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
  uint64_t nReduceSize = 0;
  for (UInt32 i = 0; i < numProps; i++)
  {
    const PROPVARIANT &prop = coderProps[i];
    PROPID propID = propIDs[i];
    if (propID > NCoderPropID::kReduceSize)
      continue;
    if (prop.vt != VT_UI4 && prop.vt != VT_UI8)
      return E_INVALIDARG;
    UInt32 v = (UInt32)prop.ulVal;
    switch (propID)
    {
    case NCoderPropID::kDictionarySize:
      if (prop.vt != VT_UI4)
        return E_INVALIDARG;
      block_size = v;
      break;
    case NCoderPropID::kUsedMemorySize:
    {
      size_t i = 0;
      v = (v + 2 * MB - 1) / 2 / MB;
      while (v > 0) {
        v >>= 1;
        i++;
      }
      options_.mem_usage_ = i;
    }
      break;
    case NCoderPropID::kLevel:
      v = (v + 1) / 2;
      options_.comp_level_ = (CompLevel)v;
      break;
    case NCoderPropID::kNumThreads:
      threads = v;
      break;
    case NCoderPropID::kReduceSize:
      nReduceSize = prop.uhVal.QuadPart;
      break;
    default: return E_INVALIDARG;
    }
  }
  if (nReduceSize > 0 && nReduceSize < block_size)
    block_size = nReduceSize;
  return S_OK;
}

STDMETHODIMP CEncoder::WriteCoderProperties(ISequentialOutStream *outStream)
{
  Byte bt;
  bt = (Byte)options_.mem_usage_;   RINOK(WriteStream(outStream, &bt, 1));
  bt = (Byte)options_.comp_level_;  RINOK(WriteStream(outStream, &bt, 1));
  bt = (Byte)options_.filter_type_; RINOK(WriteStream(outStream, &bt, 1));
  bt = (Byte)options_.lzp_type_;    RINOK(WriteStream(outStream, &bt, 1));
  UInt32 bs = (UInt32)((block_size + MB - 1) / MB); RINOK(WriteStream(outStream, &bs, 4));
  return S_OK;
}

STDMETHODIMP CEncoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
  const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress)
{
  UInt64 inSizeProcessed = 0, outSizeProcessed = 0;

  std::vector<uint8_t> plain_buffer;
  WriteVectorStream wvs_plain(&plain_buffer);
  {
    uint8_t buff[512];
    size_t processedSize = sizeof(buff);
    while (processedSize > 0) {
      processedSize = sizeof(buff);
      RINOK(ReadStream(inStream, buff, &processedSize));
      wvs_plain.write(buff, processedSize);
      inSizeProcessed += processedSize;
    }
  }
  ReadMemoryStream rms(&plain_buffer[0], &plain_buffer[0] + plain_buffer.size());
  Stream *in = &rms;

  RINOK(write_int(outStream, major_version_, &outSizeProcessed));
  RINOK(write_int(outStream, minor_version_, &outSizeProcessed));

  Analyzer analyzer;

  // Analyzing
  analyzer.analyze(in);

  constructBlocks(in, &analyzer);
  writeBlocks(outStream, &outSizeProcessed);

  UInt64 inSizeCompressed = 0;
  std::vector<uint8_t> compressed_buffer;
  WriteVectorStream wvs_compressed(&compressed_buffer);
  for (auto *block : blocks_.blocks_) {
    std::vector<uint8_t> temp_buffer;
    WriteVectorStream wvs_temp(&temp_buffer);

    FileSegmentStream segstream(&block->segments_, 0u);
    Algorithm *algo = &block->algorithm_;

    // Compressing
    std::unique_ptr<Filter> filter(algo->createFilter(&segstream, &analyzer));
    Stream *in_stream = &segstream;
    if (filter.get() != nullptr) in_stream = filter.get();
    auto in_start = in_stream->tell();
    std::unique_ptr<Compressor> comp(algo->createCompressor());
    comp->setOpt(opt_var_);
    {
      ProgressThread thr(&segstream, &wvs_temp, &inSizeCompressed, &outSizeProcessed, progress);
      comp->compress(in_stream, &wvs_temp);
      inSizeCompressed += in_stream->tell();
    }

    const auto filter_size = in_stream->tell() - in_start;
    wvs_compressed.leb128Encode(filter_size);
    wvs_compressed.write(temp_buffer.data(), temp_buffer.size());
  }

  RINOK(write_int(outStream, compressed_buffer.size(), &outSizeProcessed));
  RINOK(WriteStream(outStream, compressed_buffer.data(), compressed_buffer.size()));
  outSizeProcessed += compressed_buffer.size();

  if (progress != NULL)
    RINOK(progress->SetRatioInfo(&inSizeProcessed, &outSizeProcessed));

  return S_OK;
}

} // namespace NMCM
} // namespace NCompress
