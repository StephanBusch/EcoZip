// ThreadFolderOperations.h

#ifndef __THREAD_FOLDER_OPERATIONS_H__
#define __THREAD_FOLDER_OPERATIONS_H__

#include "CPP/Windows/COM.h"
#include "CPP/Windows/DLL.h"
#include "CPP/Windows/FileDir.h"
#include "CPP/Windows/FileFind.h"

#include "ProgressDialog2.h"
#include "UpdateCallback100.h"

enum EFolderOpType
{
  FOLDER_TYPE_CREATE_FOLDER = 0,
  FOLDER_TYPE_DELETE = 1,
  FOLDER_TYPE_RENAME = 2
};

struct CTempFileInfo
{
  UInt32 FileIndex;  // index of file in folder
  UString RelPath;   // Relative path of file from Folder
  FString FolderPath;
  FString FilePath;
  NWindows::NFile::NFind::CFileInfo FileInfo;
  bool NeedDelete;

  CTempFileInfo() : FileIndex((UInt32)(Int32)-1), NeedDelete(false) {}
  void DeleteDirAndFile() const
  {
    if (NeedDelete)
    {
      NWindows::NFile::NDir::DeleteFileAlways(FilePath);
      NWindows::NFile::NDir::RemoveDir(FolderPath);
    }
  }
  bool WasChanged(const NWindows::NFile::NFind::CFileInfo &newFileInfo) const
  {
    return newFileInfo.Size != FileInfo.Size ||
      CompareFileTime(&newFileInfo.MTime, &FileInfo.MTime) != 0;
  }
};

struct CFolderLink : public CTempFileInfo
{
  NWindows::NDLL::CLibrary Library;
  CMyComPtr<IFolderFolder> ParentFolder; // can be NULL, if parent is FS folder (in _parentFolders[0])
  UString ParentFolderPath; // including tail slash (doesn't include paths parts of parent in next level)
  bool UsePassword;
  UString Password;
  bool IsVirtual;

  UString VirtualPath; // without tail slash
  CFolderLink() : UsePassword(false), IsVirtual(false) {}

  bool WasChanged(const NWindows::NFile::NFind::CFileInfo &newFileInfo) const
  {
    return IsVirtual || CTempFileInfo::WasChanged(newFileInfo);
  }

};

class CThreadFolderOperations : public CProgressThreadVirt
{
  HRESULT ProcessVirt();
public:
  EFolderOpType OpType;
  UString Name;
  UInt32 Index;
  CRecordVector<UInt32> Indices;

  CMyComPtr<IFolderOperations> FolderOperations;
  CMyComPtr<IProgress> UpdateCallback;
  CUpdateCallback100Imp *UpdateCallbackSpec;
  
  HRESULT Result;

  CThreadFolderOperations(EFolderOpType opType): OpType(opType), Result(E_FAIL) {}
  HRESULT DoOperation(CWnd *panel, CObjectVector<CFolderLink> *parentFolders, const UString &progressTitle, const UString &titleError);
};

#endif  // __THREAD_FOLDER_OPERATIONS_H__
