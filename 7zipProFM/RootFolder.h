// RootFolder.h

#ifndef __ROOT_FOLDER_H
#define __ROOT_FOLDER_H

#include "CPP/Common/MyCom.h"
#include "CPP/Common/MyString.h"

#include "CPP/7zip/UI/FileManager/IFolder.h"

enum
{
  ROOT_INDEX_COMPUTER = 0
#ifndef UNDER_CE
  , ROOT_INDEX_DESKTOP
  , ROOT_INDEX_DOCUMENTS
  , ROOT_INDEX_NETWORK
  , ROOT_INDEX_VOLUMES
#endif
  , ROOT_INDEX_MAX
};

const unsigned kNumRootFolderItems_Max = ROOT_INDEX_MAX;

class CRootFolder:
  public IFolderFolder,
  public IFolderGetSystemIconIndex,
  public CMyUnknownImp
{
  UString _names[kNumRootFolderItems_Max];
  int _iconIndices[kNumRootFolderItems_Max];

public:
  MY_UNKNOWN_IMP1(IFolderGetSystemIconIndex)
  INTERFACE_FolderFolder(;)
  STDMETHOD(GetSystemIconIndex)(UInt32 index, Int32 *iconIndex);
  void Init();
};

extern UString RootFolder_GetName_Computer(int &iconIndex);
extern UString RootFolder_GetName_Network(int &iconIndex);
extern UString RootFolder_GetName_Desktop(int &iconIndex);
extern UString RootFolder_GetName_Documents(int &iconIndex);

#endif
