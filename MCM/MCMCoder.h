// MCMCoder.h

#ifndef __MCM_CODER_H
#define __MCM_CODER_H

#include "C/7zTypes.h"
#include "CPP/Common/MyCom.h"
#include "CPP/7zip/ICoder.h"

#include "CM.hpp"
#include "Compressor.hpp"
#include "File.hpp"
#include "Stream.hpp"

namespace NCompress {
namespace NMCM {

enum FilterType {
  kFilterTypeNone,
  kFilterTypeDict,
  kFilterTypeX86,
  kFilterTypeAuto,
  kFilterTypeCount,
};
enum CompLevel {
  kCompLevelStore,
  kCompLevelTurbo,
  kCompLevelFast,
  kCompLevelMid,
  kCompLevelHigh,
  kCompLevelMax,
};
enum LZPType {
  kLZPTypeAuto,
  kLZPTypeEnable,
  kLZPTypeDisable,
};
class CompressionOptions {
public:
  static const size_t kDefaultMemUsage = 6;
  static const CompLevel kDefaultLevel = kCompLevelMid;
  static const FilterType kDefaultFilter = kFilterTypeAuto;
  static const LZPType kDefaultLZPType = kLZPTypeAuto;
  CompressionOptions() :mem_usage_(kDefaultMemUsage), comp_level_(kDefaultLevel), filter_type_(kDefaultFilter), lzp_type_(kDefaultLZPType){
  }
public:
  size_t mem_usage_;
  CompLevel comp_level_;
  FilterType filter_type_;
  LZPType lzp_type_;
};

class ProgressThread {
public:
  ProgressThread(Stream *in_stream, Stream *out_stream,
    uint64_t *in_size, uint64_t *out_size, ICompressProgressInfo *progress,
    uintptr_t interval = 250)
    : done_(false)
    , in_stream_(in_stream), out_stream_(out_stream)
    , in_size_(in_size), out_size_(out_size), progress_(progress)
    , interval_(interval)
    , in_out_(in_stream->tell()), sub_out_(out_stream->tell())
  {
    thread_ = new std::thread(Callback, this);
  }
  virtual ~ProgressThread() {
    done();
    delete thread_;
  }
  void done() {
    if (!done_) {
        {
          ScopedLock mu(mutex_);
          done_ = true;
          cond_.notify_one();
        }
      thread_->join();
    }
  }
  void print() {
    if (progress_ != NULL) {
      auto in_c = *in_size_ + in_stream_->tell() - in_out_;
      auto out_c = *out_size_ + out_stream_->tell() - sub_out_;
      progress_->SetRatioInfo(&in_c, &out_c);
    }
  }
private:
  bool done_;
  const uintptr_t interval_;
  Stream *const in_stream_;
  Stream *const out_stream_;
  uint64_t *in_size_;
  uint64_t *out_size_;
  ICompressProgressInfo *progress_;
  const uint64_t in_out_;
  const uint64_t sub_out_;
  std::thread *thread_;
  std::mutex mutex_;
  std::condition_variable cond_;
  void run() {
    std::unique_lock<std::mutex>lock(mutex_);
    while (!done_) {
      cond_.wait_for(lock, std::chrono::milliseconds(interval_));
      print();
    }
  }
  static void Callback(ProgressThread*thr) {
    thr->run();
  }
};

class CCoder {
public:
  static const UInt64 kDefaultBlockSize = 0;
  enum Mode{
    kModeUnknown,
    kModeTest,
    kModeOpt,
    kModeMemTest,
    kModeSingleTest,
    kModeAdd,
    kModeExtract,
    kModeExtractAll,
    kModeCompress,
    kModeDecompress,
  };
  bool opt_mode;
  CompressionOptions options_;
  Compressor *compressor;
  uint32_t threads;
  uint64_t block_size;
  CCoder()
    : opt_mode(false)
    , compressor(nullptr)
    , threads(1)
    , block_size(kDefaultBlockSize)
    , major_version_(kCurMajorVersion)
    , minor_version_(kCurMinorVersion)
    , opt_var_(0)
  {
  }

  static const size_t kCurMajorVersion = 0;
  static const size_t kCurMinorVersion = 83;
  uint16_t major_version_;
  uint16_t minor_version_;

public:
  class Algorithm {
  public:
    Algorithm() {}
    Algorithm(const CompressionOptions &options, Detector::Profile profile);
    Algorithm(Stream *stream);
    Compressor *createCompressor();
    void read(Stream *stream);
    void write(Stream *stream);
    Filter *createFilter(Stream *stream, Analyzer *analyzer);
    Detector::Profile profile() const {
      return profile_;
    }
  private:
    uint8_t mem_usage_;
    Compressor::Type algorithm_;
    bool lzp_enabled_;
    FilterType filter_;
    Detector::Profile profile_;
  };
  class SolidBlock {
  public:
    Algorithm algorithm_;
    std::vector<FileSegmentStream::FileSegments> segments_;
    uint64_t total_size_;
    SolidBlock();
    void write(Stream *stream);
    void read(Stream *stream);
  };
  class Blocks {
  public:
    std::vector<SolidBlock*> blocks_;
    void write(Stream *stream);
    void read(Stream *stream);
  };

  void constructBlocks(Stream *stream, Analyzer *analyzer);
  bool setOpt(size_t var) {
    opt_var_ = var;
    return true;
  }
  void writeBlocks(ISequentialOutStream *outStream, UInt64 *outSizeProcessed);
  void readBlocks(ISequentialInStream *inStream, UInt64 *inSizeProcessed);
protected:
  size_t opt_var_;
  Blocks blocks_;
  void init();
  Compressor*createMetaDataCompressor();
};

inline long long read_int(ISequentialInStream *inStream, UInt64 *inSizeProcessed);

}}

#endif
