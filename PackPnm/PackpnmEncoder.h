// PackpnmEncoder.h

#ifndef __PACKPNM_ENCODER_H
#define __PACKPNM_ENCODER_H

#include "PackpnmCoder.h"

namespace NCompress {
namespace NPackjpg {
namespace NPackpnm {

class CEncoder :
  public ICompressCoder,
  public ICompressSetCoderProperties,
  public ICompressWriteCoderProperties,
  public CMyUnknownImp,
  public CCoder
{
public:
  MY_UNKNOWN_IMP2(ICompressSetCoderProperties, ICompressWriteCoderProperties)
 
  STDMETHOD(Code)(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);
  STDMETHOD(SetCoderProperties)(const PROPID *propIDs, const PROPVARIANT *props, UInt32 numProps);
  STDMETHOD(WriteCoderProperties)(ISequentialOutStream *outStream);

  CEncoder();
  virtual ~CEncoder();
};

}}}

#endif
