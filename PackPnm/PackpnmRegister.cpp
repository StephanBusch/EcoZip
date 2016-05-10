// PackpnmRegister.cpp

#include "CPP/7zip/Compress/StdAfx.h"

#include "CPP/7zip/Common/RegisterCodec.h"

#include "PackpnmEncoder.h"
#include "PackpnmDecoder.h"

REGISTER_CODEC_E(packPNM,
    NCompress::NPackjpg::NPackpnm::CDecoder(),
    NCompress::NPackjpg::NPackpnm::CEncoder(),
    0x04F70401,
    "packPNM")
