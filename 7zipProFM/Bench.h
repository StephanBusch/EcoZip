// Bench.h

#ifndef __7ZIP_BENCH_H
#define __7ZIP_BENCH_H

#include "CPP/7zip/Common/CreateCoder.h"
#include "CPP/7zip/UI/Common/Property.h"

struct CBenchInfo
{
  UInt64 GlobalTime;
  UInt64 GlobalFreq;
  UInt64 UserTime;
  UInt64 UserFreq;
  UInt64 UnpackSize;
  UInt64 PackSize;
  UInt64 NumIterations;
  
  CBenchInfo(): NumIterations(0) {}
  UInt64 GetUsage() const;
  UInt64 GetRatingPerUsage(UInt64 rating) const;
  UInt64 GetSpeed(UInt64 numCommands) const;
};

struct IBenchCallback
{
  virtual HRESULT SetFreq(bool showFreq, UInt64 cpuFreq) = 0;
  virtual HRESULT SetEncodeResult(const CBenchInfo &info, bool final) = 0;
  virtual HRESULT SetDecodeResult(const CBenchInfo &info, bool final) = 0;
};

UInt64 GetCompressRating(UInt32 dictSize, UInt64 elapsedTime, UInt64 freq, UInt64 size);
UInt64 GetDecompressRating(UInt64 elapsedTime, UInt64 freq, UInt64 outSize, UInt64 inSize, UInt64 numIterations);

const int kBenchMinDicLogSize = 18;

UInt64 GetBenchMemoryUsage(UInt32 numThreads, UInt32 dictionary);

struct IBenchPrintCallback
{
  virtual void Print(const char *s) = 0;
  virtual void NewLine() = 0;
  virtual HRESULT CheckBreak() = 0;
};

HRESULT Bench(
    DECL_EXTERNAL_CODECS_LOC_VARS
    IBenchPrintCallback *printCallback,
    IBenchCallback *benchCallback,
    const CObjectVector<CProperty> &props,
    UInt32 numIterations,
    bool multiDict
    );


extern const UInt64 kComplexInCommands;
extern const UInt64 kComplexInSeconds;

void SetComplexCommands(UInt32 complexInSeconds, UInt64 cpuFreq, UInt64 &complexInCommands);

extern const unsigned kNumHashDictBits;
extern const UInt32 kFilterUnpackSize;

extern const unsigned kOldLzmaDictBits;

extern const UInt32 kAdditionalSize;
extern const UInt32 kCompressedAdditionalSize;
extern const UInt32 kMaxLzmaPropSize;

UInt64 GetTimeCount();
UInt64 GetFreq();

struct CBenchProps
{
  bool LzmaRatingMode;
  
  UInt32 EncComplex;
  UInt32 DecComplexCompr;
  UInt32 DecComplexUnc;

  CBenchProps(): LzmaRatingMode(false) {}
  void SetLzmaCompexity();

  UInt64 GeComprCommands(UInt64 unpackSize)
  {
    return unpackSize * EncComplex;
  }

  UInt64 GeDecomprCommands(UInt64 packSize, UInt64 unpackSize)
  {
    return (packSize * DecComplexCompr + unpackSize * DecComplexUnc);
  }

  UInt64 GetCompressRating(UInt32 dictSize, UInt64 elapsedTime, UInt64 freq, UInt64 size);
  UInt64 GetDecompressRating(UInt64 elapsedTime, UInt64 freq, UInt64 outSize, UInt64 inSize, UInt64 numIterations);
};

HRESULT MethodBench(
  DECL_EXTERNAL_CODECS_LOC_VARS
  UInt64 complexInCommands,
  bool oldLzmaBenchMode,
  UInt32 numThreads,
  const COneMethodInfo &method2,
  UInt32 uncompressedDataSize,
  unsigned generateDictBits,
  IBenchPrintCallback *printCallback,
  IBenchCallback *callback,
  CBenchProps *benchProps);

extern UInt32 g_BenchCpuFreqTemp;
extern const UInt32 kNumFreqCommands;

UInt32 CountCpuFreq(UInt32 sum, UInt32 num, UInt32 val);

struct CBenchMethod
{
  unsigned Weight;
  unsigned DictBits;
  UInt32 EncComplex;
  UInt32 DecComplexCompr;
  UInt32 DecComplexUnc;
  const char *Name;
};

extern const CBenchMethod g_Bench[];

struct CBenchHash
{
  unsigned Weight;
  UInt32 Complex;
  UInt32 CheckSum;
  const char *Name;
};

extern const CBenchHash g_Hash[];

struct CTotalBenchRes
{
  // UInt64 NumIterations1; // for Usage
  UInt64 NumIterations2; // for Rating / RPU

  UInt64 Rating;
  UInt64 Usage;
  UInt64 RPU;
  
  void Init() { /* NumIterations1 = 0; */ NumIterations2 = 0; Rating = 0; Usage = 0; RPU = 0; }

  void SetSum(const CTotalBenchRes &r1, const CTotalBenchRes &r2)
  {
    Rating = (r1.Rating + r2.Rating);
    Usage = (r1.Usage + r2.Usage);
    RPU = (r1.RPU + r2.RPU);
    // NumIterations1 = (r1.NumIterations1 + r2.NumIterations1);
    NumIterations2 = (r1.NumIterations2 + r2.NumIterations2);
  }
};

HRESULT FreqBench(
  UInt64 complexInCommands,
  UInt32 numThreads,
  IBenchPrintCallback *_file,
  bool showFreq,
  UInt64 &cpuFreq,
  UInt32 &res);

HRESULT CrcBench(
  DECL_EXTERNAL_CODECS_LOC_VARS
  UInt64 complexInCommands,
  UInt32 numThreads, UInt32 bufferSize,
  UInt64 &speed,
  UInt32 complexity,
  const UInt32 *checkSum,
  const COneMethodInfo &method,
  IBenchPrintCallback *_file,
  CTotalBenchRes *encodeRes,
  bool showFreq, UInt64 cpuFreq);

#endif
