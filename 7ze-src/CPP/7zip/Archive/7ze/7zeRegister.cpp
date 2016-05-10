// 7zRegister.cpp

#include "StdAfx.h"

#include "../../Common/RegisterArc.h"

#include "7zeHandler.h"

namespace NArchive {
namespace N7ze {

static Byte k_Signature_Dec[kSignatureSize] = {'7' + 1, 'z', 'e', 0xAF, 0x27, 0x1C};

REGISTER_ARC_IO_DECREMENT_SIG(
  "7ze", "7ze", NULL, 7,
  k_Signature_Dec,
  0,
  NArcInfoFlags::kFindSignature,
  NULL);

}}
