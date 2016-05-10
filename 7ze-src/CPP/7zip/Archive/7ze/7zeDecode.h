// 7zDecode.h

#ifndef __7ZE_DECODE_H
#define __7ZE_DECODE_H

#include "../Common/CoderMixer2.h"

#include "7zeIn.h"

namespace NArchive {
namespace N7ze {

struct CBindInfoEx: public NCoderMixer2::CBindInfo
{
  CRecordVector<CMethodId> CoderMethodIDs;

  void Clear()
  {
    CBindInfo::Clear();
    CoderMethodIDs.Clear();
  }
};

class CDecoder
{
  bool _bindInfoPrev_Defined;
  CBindInfoEx _bindInfoPrev;
  
  bool _useMixerMT;
  
  #ifdef USE_MIXER_ST
    NCoderMixer2::CMixerST *_mixerST;
  #endif
  
  #ifdef USE_MIXER_MT
    NCoderMixer2::CMixerMT *_mixerMT;
  #endif
  
  NCoderMixer2::CMixer *_mixer;
  CMyComPtr<IUnknown> _mixerRef;

public:

  CDecoder(bool useMixerMT);
  
  HRESULT Decode(
      DECL_EXTERNAL_CODECS_LOC_VARS
      IInStream *inStream,
      UInt64 startPos,
      const CFolders &folders, unsigned folderIndex,
      const UInt64 *unpackSize // if (!unpackSize), then full folder is required
                               // if (unpackSize), then only *unpackSize bytes from folder are required

      , ISequentialOutStream *outStream
      , ICompressProgressInfo *compressProgress
      , ISequentialInStream **inStreamMainRes
      
      _7ZE_DECODER_CRYPRO_VARS_DECL
      
      #if !defined(_7ZIP_ST) && !defined(_SFX)
      , bool mtMode, UInt32 numThreads
      #endif
      );
};

}}

#endif
