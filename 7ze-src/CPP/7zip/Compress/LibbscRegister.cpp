// LibbscRegister.cpp

#include "StdAfx.h"

#include "../Common/RegisterCodec.h"

#include "LibbscCoder.h"

REGISTER_CODEC_E(LIBBSC,
    NCompress::NLibbsc::CDecoder(),
    NCompress::NLibbsc::CEncoder(),
    0x04F70101,
    "LIBBSC")
