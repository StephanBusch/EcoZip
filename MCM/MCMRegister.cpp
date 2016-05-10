// MCMRegister.cpp

#define NOMINMAX

#include "CPP/7zip/Compress/StdAfx.h"

#include "CPP/7zip/Common/RegisterCodec.h"

#include "MCMCoder.h"
#include "MCMDecoder.h"
#include "MCMEncoder.h"

REGISTER_CODEC_E(MCM,
    NCompress::NMCM::CDecoder(),
    NCompress::NMCM::CEncoder(),
    0x04F70501,
    "MCM")
