// Csc32Register.cpp

#include "StdAfx.h"

#include "../Common/RegisterCodec.h"

#include "Csc32Coder.h"

REGISTER_CODEC_E(CSC32,
    NCompress::NCsc32::CDecoder(),
    NCompress::NCsc32::CEncoder(),
    0x04F70201,
    "CSC32")
