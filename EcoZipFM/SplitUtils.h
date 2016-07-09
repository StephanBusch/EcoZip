// SplitUtils.h

#ifndef __SPLIT_UTILS_H
#define __SPLIT_UTILS_H

#include "CPP/Common/MyTypes.h"
#include "CPP/Common/MyString.h"

bool ParseVolumeSizes(const UString &s, CRecordVector<UInt64> &values);
void AddVolumeItems(CComboBox &volumeCombo);
UInt64 GetNumberOfVolumes(UInt64 size, const CRecordVector<UInt64> &volSizes);

#endif
