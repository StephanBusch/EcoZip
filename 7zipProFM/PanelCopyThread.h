// PanelCopyThread.h

#ifndef __PANEL_COPY_THREAD_H__
#define __PANEL_COPY_THREAD_H__

// #include "CPP/Common/MyException.h"
#include "CPP/Common/MyCom.h"
#include "CPP/Common/MyString.h"

#include "ProgressDialog2.h"

#include "ExtractCallback.h"
#include "CPP/7zip/UI/FileManager/IFolder.h"
// #include "CPP/7zip/UI/FileManager/LangUtils.h"
// #include "UpdateCallback100.h"
// 
// #include "FileManager.h"
// #include "ContentsView.h"

struct CCopyToOptions
{
  bool streamMode;
  bool moveMode;
  bool testMode;
  bool includeAltStreams;
  bool replaceAltStreamChars;
  bool showErrorMessages;

  UString folder;

  UStringVector hashMethods;

  CVirtFileSystem *VirtFileSystemSpec;
  ISequentialOutStream *VirtFileSystem;

  CCopyToOptions() :
    streamMode(false),
    moveMode(false),
    testMode(false),
    includeAltStreams(true),
    replaceAltStreamChars(false),
    showErrorMessages(false),
    VirtFileSystemSpec(NULL),
    VirtFileSystem(NULL)
  {}
};

class CPanelCopyThread : public CProgressThreadVirt
{
  HRESULT ProcessVirt();
public:
  const CCopyToOptions *options;
  CMyComPtr<IFolderOperations> FolderOperations;
  CRecordVector<UInt32> Indices;
  CExtractCallbackImp *ExtractCallbackSpec;
  CMyComPtr<IFolderOperationsExtractCallback> ExtractCallback;
  
  CHashBundle Hash;
  UString FirstFilePath;

  HRESULT Result;
  

  CPanelCopyThread(): Result(E_FAIL) {}
};

#endif  // __PANEL_COPY_THREAD_H__
