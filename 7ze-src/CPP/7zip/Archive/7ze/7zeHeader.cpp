// 7zHeader.cpp

#include "StdAfx.h"

#include "7zeHeader.h"

namespace NArchive {
namespace N7ze {

Byte kSignature[kSignatureSize] = {'7', 'z', 'e', 0xAF, 0x27, 0x1C};
#ifdef _7ZE_VOL
Byte kFinishSignature[kSignatureSize] = {'7', 'z', 'e', 0xAF, 0x27, 0x1C + 1};
#endif

// We can change signature. So file doesn't contain correct signature.
// struct SignatureInitializer { SignatureInitializer() { kSignature[0]--; } };
// static SignatureInitializer g_SignatureInitializer;

}}
