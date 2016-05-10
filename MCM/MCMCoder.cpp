// MCMCoder.cpp

#define NOMINMAX

#include "CPP/7zip/Compress/StdAfx.h"

#include "C/Alloc.h"

#include "CPP/7zip/Common/CWrappers.h"
#include "CPP/7zip/Common/StreamUtils.h"

#include "MCMCoder.h"
#include "PackjpgCoder.h"
#include "X86Binary.hpp"
#include "Wav16.hpp"

CompressorFactories *CompressorFactories::instance = nullptr;

namespace NCompress {
namespace NMCM {

inline long long read_int(ISequentialInStream *inStream, UInt64 *inSizeProcessed)
{
  int sh = 0;
  long long rlt = 0;
  unsigned char b = 0;
  do {
    size_t processedSize = 1;
    if (FAILED(ReadStream(inStream, &b, &processedSize)))
      break;
    if (processedSize < 1)
      break;
    rlt = rlt + ((b & 0x7F) << sh);
    *inSizeProcessed += 1;
    sh += 7;
  } while ((b & 0x80) != 0);
  return rlt;
}

CCoder::Algorithm::Algorithm(const CompressionOptions &options, Detector::Profile profile) : profile_(profile) {
  mem_usage_ = options.mem_usage_;
  algorithm_ = Compressor::kTypeStore;
  filter_ = FilterType::kFilterTypeNone;
  if (profile == Detector::kProfileWave16) {
    algorithm_ = Compressor::kTypeWav16;
  }
  else{
    switch (options.comp_level_) {
    case kCompLevelStore:
      algorithm_ = Compressor::kTypeStore;
      break;
    case kCompLevelTurbo:
      algorithm_ = Compressor::kTypeCMTurbo;
      break;
    case kCompLevelFast:
      algorithm_ = Compressor::kTypeCMFast;
      break;
    case kCompLevelMid:
      algorithm_ = Compressor::kTypeCMMid;
      break;
    case kCompLevelHigh:
      algorithm_ = Compressor::kTypeCMHigh;
      break;
    case kCompLevelMax:
      algorithm_ = Compressor::kTypeCMMax;
      break;
    }
  }
  switch (profile) {
  case Detector::kProfileBinary:
    lzp_enabled_ = true;
    filter_ = kFilterTypeX86;
    break;
  case Detector::kProfileText:
    lzp_enabled_ = true;
    filter_ = kFilterTypeDict;
    break;
  }
  if (options.lzp_type_ == kLZPTypeEnable)lzp_enabled_ = true;
  else if (options.lzp_type_ == kLZPTypeDisable)lzp_enabled_ = false;
  if (options.filter_type_ != kFilterTypeAuto) {
    filter_ = options.filter_type_;
  }
}

CCoder::Algorithm::Algorithm(Stream *stream) {
  read(stream);
}

Compressor *CCoder::Algorithm::createCompressor() {
  switch (algorithm_) {
  case Compressor::kTypeWav16:
    return new Wav16;
  case Compressor::kTypeStore:
    return new Store;
  case Compressor::kTypeCMTurbo:
    return new CM<kCMTypeTurbo>(mem_usage_, lzp_enabled_, profile_);
  case Compressor::kTypeCMFast:
    return new CM<kCMTypeFast>(mem_usage_, lzp_enabled_, profile_);
  case Compressor::kTypeCMMid:
    return new CM<kCMTypeMid>(mem_usage_, lzp_enabled_, profile_);
  case Compressor::kTypeCMHigh:
    return new CM<kCMTypeHigh>(mem_usage_, lzp_enabled_, profile_);
  case Compressor::kTypeCMMax:
    return new CM<kCMTypeMax>(mem_usage_, lzp_enabled_, profile_);
  }
  return nullptr;
}

void CCoder::Algorithm::read(Stream *stream) {
  mem_usage_ = static_cast<uint8_t>(stream->get());
  algorithm_ = static_cast<Compressor::Type>(stream->get());
  lzp_enabled_ = stream->get() != 0 ? true : false;
  filter_ = static_cast<FilterType>(stream->get());
  profile_ = static_cast<Detector::Profile>(stream->get());
}

void CCoder::Algorithm::write(Stream *stream) {
  stream->put(mem_usage_);
  stream->put(algorithm_);
  stream->put(lzp_enabled_);
  stream->put(filter_);
  stream->put(profile_);
}

std::ostream &operator <<(std::ostream&os, CompLevel comp_level) {
  switch (comp_level) {
  case kCompLevelStore:return os << "store";
  case kCompLevelTurbo:return os << "turbo";
  case kCompLevelFast:return os << "fast";
  case kCompLevelMid:return os << "mid";
  case kCompLevelHigh:return os << "high";
  case kCompLevelMax:return os << "max";
  }
  return os << "unknown";
}

Filter *CCoder::Algorithm::createFilter(Stream *stream, Analyzer *analyzer) {
  switch (filter_) {
  case kFilterTypeDict:
    if (analyzer) {
      auto&builder = analyzer->getDictBuilder();
      Dict::CodeWordGeneratorFast generator;
      Dict::CodeWordSet code_words;
      generator.generateCodeWords(builder, &code_words);
      auto dict_filter = new Dict::Filter(stream, 0x3, 0x4, 0x6);
      dict_filter->addCodeWords(code_words.getCodeWords(), code_words.num1_, code_words.num2_, code_words.num3_);
      return dict_filter;
    }
    else{
      return new Dict::Filter(stream);
    }
  case kFilterTypeX86:
    return new X86AdvancedFilter(stream);
  }
  return nullptr;
}

CCoder::SolidBlock::SolidBlock() {
}

class BlockSizeComparator {
public:
  bool operator()(const CCoder::SolidBlock *a, const CCoder::SolidBlock *b) const {
    return a->total_size_ < b->total_size_;
  }
};

void CCoder::constructBlocks(Stream *in, Analyzer *analyzer)
{
  uint64_t total_in = 0;
  for (size_t p_idx = 0; p_idx < static_cast<size_t>(Detector::kProfileCount); ++p_idx) {
    auto profile = static_cast<Detector::Profile>(p_idx);
    uint64_t pos = 0;
    FileSegmentStream::FileSegments seg;
    seg.base_offset_ = 0;
    seg.stream_ = in;
    for (const auto &b : analyzer->getBlocks()) {
      const auto len = b.length();
      if (b.profile() == profile) {
        FileSegmentStream::SegmentRange range;
        range.offset_ = pos;
        range.length_ = len;
        seg.ranges_.push_back(range);
      }
      pos += len;
    }
    seg.calculateTotalSize();
    if (seg.total_size_ > 0) {
      auto *solid_block = new SolidBlock();
      solid_block->algorithm_ = Algorithm(options_, profile);
      solid_block->segments_.push_back(seg);
      solid_block->total_size_ = seg.total_size_;
      blocks_.blocks_.push_back(solid_block);
    }
  }
  std::sort(blocks_.blocks_.rbegin(), blocks_.blocks_.rend(), BlockSizeComparator());
}

Compressor *CCoder::createMetaDataCompressor() {
  return new CM<kCMTypeFast>(6, true, Detector::kProfileText);
}

void CCoder::writeBlocks(ISequentialOutStream *outStream, UInt64 *outSizeProcessed) {
  std::vector<uint8_t> temp;
  WriteVectorStream wvs(&temp);
  blocks_.write(&wvs);
  std::unique_ptr<Compressor> c(createMetaDataCompressor());
  ReadMemoryStream rms(&temp[0], &temp[0] + temp.size());

  std::vector<uint8_t> compressed_buffer;
  WriteVectorStream wvs_compressed(&compressed_buffer);

  c->compress(&rms, &wvs_compressed);

  if (FAILED(NCompress::NPackjpg::write_int(outStream, temp.size(), outSizeProcessed)))
    return;
  if (FAILED(NCompress::NPackjpg::write_int(outStream, compressed_buffer.size(), outSizeProcessed)))
    return;
  if (FAILED(WriteStream(outStream, compressed_buffer.data(), compressed_buffer.size())))
    return;
  *outSizeProcessed += compressed_buffer.size();
  NCompress::NPackjpg::write_int(outStream, 1234u, outSizeProcessed);
}

void CCoder::readBlocks(ISequentialInStream *inStream, UInt64 *inSizeProcessed) {
  size_t metadata_size = (size_t)read_int(inStream, inSizeProcessed);
  size_t compressed_size = (size_t)read_int(inStream, inSizeProcessed);
  std::vector<uint8_t> compressed_buffer;
  compressed_buffer.assign(compressed_size, 0);
  if (FAILED(ReadStream(inStream, compressed_buffer.data(), &compressed_size)))
    return;
  ReadMemoryStream rms_compressed(compressed_buffer.data(), compressed_buffer.data() + compressed_size);
  std::unique_ptr<Compressor> c(createMetaDataCompressor());
  std::vector<uint8_t> metadata;
  WriteVectorStream wvs(&metadata);
  c->decompress(&rms_compressed, &wvs, metadata_size);
  auto cmp = read_int(inStream, inSizeProcessed);
  check(cmp == 1234u);
  ReadMemoryStream rms(&metadata);
  blocks_.read(&rms);
}

void CCoder::Blocks::write(Stream *stream) {
  stream->leb128Encode(blocks_.size());
  for (auto *block : blocks_) {
    block->write(stream);
  }
}

void CCoder::Blocks::read(Stream *stream) {
  size_t num_blocks = (size_t)stream->leb128Decode();
  check(num_blocks < 1000000);
  blocks_.clear();
  for (size_t i = 0; i < num_blocks; ++i) {
    auto*block = new SolidBlock;
    block->read(stream);
    blocks_.push_back(block);
  }
}

void CCoder::SolidBlock::write(Stream *stream) {
  algorithm_.write(stream);
  stream->leb128Encode(segments_.size());
  for (auto&seg : segments_) {
    seg.write(stream);
  }
}

void CCoder::SolidBlock::read(Stream*stream) {
  algorithm_.read(stream);
  size_t num_segments = (size_t)stream->leb128Decode();
  check(num_segments < 10000000);
  segments_.resize(num_segments);
  for (auto&seg : segments_) {
    seg.read(stream);
    seg.calculateTotalSize();
    std::cout << Detector::profileToString(algorithm_.profile()) << " size " << seg.total_size_ << std::endl;
  }
}

} // namespace NMCM
} // namespace NCompress
