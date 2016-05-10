// Packmp3Register.cpp

#include "StdAfx.h"

#include "../Common/RegisterCodec.h"

#include "Packmp3Coder.h"

REGISTER_CODEC_E(packMP3,
    NCompress::NPackmp3::CDecoder(),
    NCompress::NPackmp3::CEncoder(),
    0x04F70301,
    "packMP3")
