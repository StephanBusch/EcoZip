// PanelFolderChange.cpp

#include "stdafx.h"

#include "CPP/Common/StringConvert.h"
#include "CPP/Common/Wildcard.h"

#include "CPP/Windows/FileName.h"
// #include "CPP/Windows/FileDir.h"
#include "CPP/Windows/PropVariant.h"

// #include "CPP/7zip/PropID.h"

#ifdef UNDER_CE
#include "CPP/7zip/UI/FileManager/FSFolder.h"
#else
#include "CPP/7zip/UI/FileManager/FSDrives.h"
#endif
// #include "CPP/7zip/UI/FileManager/LangUtils.h"
// #include "CPP/7zip/UI/FileManager/ListViewDialog.h"
#include "RootFolder.h"
// #include "CPP/7zip/UI/FileManager/ViewSettings.h"

#include "ContentsView.h"
#include "Messages.h"

#include "FMUtils.h"

using namespace NWindows;
using namespace NFile;
using namespace NFind;

void CContentsView::ReleaseFolder()
{
  _folder.Release();

  _folderCompare.Release();
  _folderGetItemName.Release();
  _folderRawProps.Release();
  _folderAltStreams.Release();
  _folderOperations.Release();
  
  _thereAreDeletedItems = false;
  selectedIndex = -1;
}

void CContentsView::SetNewFolder(IFolderFolder *newFolder)
{
  ReleaseFolder();
  _folder = newFolder;
  if (_folder)
  {
    _folder.QueryInterface(IID_IFolderCompare, &_folderCompare);
    _folder.QueryInterface(IID_IFolderGetItemName, &_folderGetItemName);
    _folder.QueryInterface(IID_IArchiveGetRawProps, &_folderRawProps);
    _folder.QueryInterface(IID_IFolderAltStreams, &_folderAltStreams);
    _folder.QueryInterface(IID_IFolderOperations, &_folderOperations);
  }
}

void CContentsView::SetToRootFolder()
{
  ReleaseFolder();
  _library.Free();

  CRootFolder *rootFolderSpec = new CRootFolder;
  SetNewFolder(rootFolderSpec);
  rootFolderSpec->Init();
}

HRESULT CContentsView::BindToPath(const UString &fullPath, const UString &arcFormat, bool &archiveIsOpened, bool &encrypted)
{
  UString path = fullPath;
  #ifdef _WIN32
  path.Replace(L'/', WCHAR_PATH_SEPARATOR);
  #endif

  archiveIsOpened = false;
  encrypted = false;

  CDisableTimerProcessing disableTimerProcessing(*this);
  CDisableNotify disableNotify(*this);

  for (; !_parentFolders.IsEmpty(); CloseOneLevel())
  {
    // ---------- we try to use open archive ----------

    const CFolderLink &link = _parentFolders.Back();
    const UString &virtPath = link.VirtualPath;
    if (!path.IsPrefixedBy(virtPath))
      continue;
    UString relatPath = path.Ptr(virtPath.Len());
    if (!relatPath.IsEmpty())
    {
      if (!IS_PATH_SEPAR(relatPath[0]))
        continue;
      else
        relatPath.Delete(0);
    }
    
    UString relatPath2 = relatPath;
    if (!relatPath2.IsEmpty() && !IS_PATH_SEPAR(relatPath2.Back()))
      relatPath2.Add_PathSepar();

    for (;;)
    {
      const UString foldPath = GetFolderPath(_folder);
      if (relatPath2 == foldPath)
        break;
      if (relatPath.IsPrefixedBy(foldPath))
      {
        path = relatPath.Ptr(foldPath.Len());
        break;
      }
      CMyComPtr<IFolderFolder> newFolder;
      if (_folder->BindToParentFolder(&newFolder) != S_OK)
        throw 20140918;
      if (!newFolder) // we exit from loop above if (relatPath.IsPrefixedBy(empty path for root folder)
        throw 20140918;
      SetNewFolder(newFolder);
    }
    break;
  }

  if (_parentFolders.IsEmpty())
  {
    // ---------- we open file or folder from file system ----------

    CloseOpenFolders();
    UString sysPath = path;
    
    unsigned prefixSize = NName::GetRootPrefixSize(sysPath);
    if (prefixSize == 0 || sysPath[prefixSize] == 0)
      sysPath.Empty();
    
    #if defined(_WIN32) && !defined(UNDER_CE)
    if (!sysPath.IsEmpty() && sysPath.Back() == ':' &&
      (sysPath.Len() != 2 || !NName::IsDrivePath2(sysPath)))
    {
      UString baseFile = sysPath;
      baseFile.DeleteBack();
      if (NFind::DoesFileOrDirExist(us2fs(baseFile)))
        sysPath.Empty();
    }
    #endif
    
    CFileInfo fileInfo;
    
    while (!sysPath.IsEmpty())
    {
      if (fileInfo.Find(us2fs(sysPath)))
        break;
      int pos = sysPath.ReverseFind_PathSepar();
      if (pos < 0)
        sysPath.Empty();
      else
      {
        /*
        if (reducedParts.Size() > 0 || pos < (int)sysPath.Len() - 1)
          reducedParts.Add(sysPath.Ptr(pos + 1));
        */
        #if defined(_WIN32) && !defined(UNDER_CE)
        if (pos == 2 && NName::IsDrivePath2(sysPath) && sysPath.Len() > 3)
          pos++;
        #endif

        sysPath.DeleteFrom(pos);
      }
    }
    
    SetToRootFolder();

    CMyComPtr<IFolderFolder> newFolder;
  
    if (sysPath.IsEmpty())
    {
      _folder->BindToFolder(path, &newFolder);
    }
    else if (fileInfo.IsDir())
    {
      NName::NormalizeDirPathPrefix(sysPath);
      _folder->BindToFolder(sysPath, &newFolder);
    }
    else
    {
      FString dirPrefix, fileName;
      NDir::GetFullPathAndSplit(us2fs(sysPath), dirPrefix, fileName);
      HRESULT res;
      // = OpenAsArc(fs2us(fileName), arcFormat, encrypted);
      {
        CTempFileInfo tfi;
        tfi.RelPath = fs2us(fileName);
        tfi.FolderPath = dirPrefix;
        tfi.FilePath = us2fs(sysPath);
        res = OpenAsArc(NULL, tfi, sysPath, arcFormat, encrypted);
      }
      
      if (res == S_FALSE)
        _folder->BindToFolder(fs2us(dirPrefix), &newFolder);
      else
      {
        RINOK(res);
        archiveIsOpened = true;
        _parentFolders.Back().ParentFolderPath = fs2us(dirPrefix);
        path.DeleteFrontal(sysPath.Len());
        if (!path.IsEmpty() && IS_PATH_SEPAR(path[0]))
          path.Delete(0);
      }
    }
    
    if (newFolder)
    {
      SetNewFolder(newFolder);
      // LoadFullPath();
      return S_OK;
    }
  }
  
  {
    // ---------- we open folder remPath in archive and sub archives ----------

    for (unsigned curPos = 0; curPos != path.Len();)
    {
      UString s = path.Ptr(curPos);
      int slashPos = NName::FindSepar(s);
      unsigned skipLen = s.Len();
      if (slashPos >= 0)
      {
        s.DeleteFrom(slashPos);
        skipLen = slashPos + 1;
      }

      CMyComPtr<IFolderFolder> newFolder;
      _folder->BindToFolder(s, &newFolder);
      if (newFolder)
        curPos += skipLen;
      else if (_folderAltStreams)
      {
        int pos = s.Find(L':');
        if (pos >= 0)
        {
          UString baseName = s;
          baseName.DeleteFrom(pos);
          if (_folderAltStreams->BindToAltStreams(baseName, &newFolder) == S_OK && newFolder)
            curPos += pos + 1;
        }
      }
      
      if (!newFolder)
        break;

      SetNewFolder(newFolder);
    }
  }

  return S_OK;
}

HRESULT CContentsView::BindToPathAndRefresh(const UString &path)
{
  if (path == GetFsPath())
    return S_OK;
  CDisableTimerProcessing disableTimerProcessing(*this);
  CDisableNotify disableNotify(*this);
  bool archiveIsOpened, encrypted;
  RINOK(BindToPath(path, UString(), archiveIsOpened, encrypted));
  RefreshListCtrl(UString(), -1, true, UStringVector());
  return S_OK;
}

void CContentsView::SetBookmark(unsigned index)
{
//   _appState->FastFolders.SetString(index, _currentFolderPrefix);
}

void CContentsView::OpenBookmark(unsigned index)
{
//   BindToPathAndRefresh(_appState->FastFolders.GetString(index));
}

void CContentsView::LoadFullPath()
{
  _currentFolderPrefix.Empty();
  FOR_VECTOR (i, _parentFolders)
  {
    const CFolderLink &folderLink = _parentFolders[i];
    _currentFolderPrefix += folderLink.ParentFolderPath;
        // GetFolderPath(folderLink.ParentFolder);
    _currentFolderPrefix += folderLink.RelPath;
    _currentFolderPrefix.Add_PathSepar();
  }
  if (_folder)
    _currentFolderPrefix += GetFolderPath(_folder);
}

static int GetRealIconIndex(CFSTR path, DWORD attributes)
{
  int index = -1;
  if (GetRealIconIndex(path, attributes, index) != 0)
    return index;
  return -1;
}

void CContentsView::LoadFullPathAndShow()
{
  LoadFullPath();

  GetTopLevelParent()->SendMessage(kFolderChanged, (WPARAM)this);
}

void CContentsView::FoldersHistory()
{
//   CListViewDialog listViewDialog;
//   listViewDialog.DeleteIsAllowed = true;
//   LangString(IDS_FOLDERS_HISTORY, listViewDialog.Title);
//   _appState->FolderHistory.GetList(listViewDialog.Strings);
//   if (listViewDialog.Create(GetParent()) != IDOK)
//     return;
//   UString selectString;
//   if (listViewDialog.StringsWereChanged)
//   {
//     _appState->FolderHistory.RemoveAll();
//     for (int i = listViewDialog.Strings.Size() - 1; i >= 0; i--)
//       _appState->FolderHistory.AddString(listViewDialog.Strings[i]);
//     if (listViewDialog.FocusedItemIndex >= 0)
//       selectString = listViewDialog.Strings[listViewDialog.FocusedItemIndex];
//   }
//   else
//   {
//     if (listViewDialog.FocusedItemIndex >= 0)
//       selectString = listViewDialog.Strings[listViewDialog.FocusedItemIndex];
//   }
//   if (listViewDialog.FocusedItemIndex >= 0)
//     BindToPathAndRefresh(selectString);
}

void CContentsView::OpenParentFolder()
{
  LoadFullPath(); // Maybe we don't need it ??
  
  UString parentFolderPrefix;
  UString focusedName;
  
  if (!_currentFolderPrefix.IsEmpty())
  {
    wchar_t c = _currentFolderPrefix.Back();
    if (c == WCHAR_PATH_SEPARATOR || c == ':')
    {
      focusedName = _currentFolderPrefix;
      focusedName.DeleteBack();
      /*
      if (c == ':' && !focusedName.IsEmpty() && focusedName.Back() == WCHAR_PATH_SEPARATOR)
      {
        focusedName.DeleteBack();
      }
      else
      */
      if (focusedName != L"\\\\." &&
          focusedName != L"\\\\?")
      {
        int pos = focusedName.ReverseFind_PathSepar();
        if (pos >= 0)
        {
          parentFolderPrefix = focusedName;
          parentFolderPrefix.DeleteFrom(pos + 1);
          focusedName.DeleteFrontal(pos + 1);
        }
      }
    }
  }

  CDisableTimerProcessing disableTimerProcessing(*this);
  CDisableNotify disableNotify(*this);
  
  CMyComPtr<IFolderFolder> newFolder;
  _folder->BindToParentFolder(&newFolder);

  // newFolder.Release(); // for test
  
  if (newFolder)
    SetNewFolder(newFolder);
  else
  {
    bool needSetFolder = true;
    if (!_parentFolders.IsEmpty())
    {
      {
        const CFolderLink &link = _parentFolders.Back();
        parentFolderPrefix = link.ParentFolderPath;
        focusedName = link.RelPath;
      }
      CloseOneLevel();
      needSetFolder = (!_folder);
    }
    
    if (needSetFolder)
    {
      {
        bool archiveIsOpened;
        bool encrypted;
        BindToPath(parentFolderPrefix, UString(), archiveIsOpened, encrypted);
      }
    }
  }
    
  UStringVector selectedItems;
  /*
  if (!focusedName.IsEmpty())
    selectedItems.Add(focusedName);
  */
  LoadFullPath();
  // ::SetCurrentDirectory(::_currentFolderPrefix);
  RefreshListCtrl(focusedName, -1, true, selectedItems);
  // _listView.EnsureVisible(_listView.GetFocusedItem(), false);
  SetFocus();
}

void CContentsView::CloseOneLevel()
{
  ReleaseFolder();
  _library.Free();
  {
    CFolderLink &link = _parentFolders.Back();
    if (link.ParentFolder)
      SetNewFolder(link.ParentFolder);
    _library.Attach(link.Library.Detach());
  }
  if (_parentFolders.Size() > 1)
    OpenParentArchiveFolder();
  _parentFolders.DeleteBack();
  if (_parentFolders.IsEmpty())
    _flatMode = _flatModeForDisk;
}

void CContentsView::CloseOpenFolders()
{
  if (m_pTempDir) {
    delete m_pTempDir;
    m_pTempDir = NULL;
  }
  while (!_parentFolders.IsEmpty())
    CloseOneLevel();
  _flatMode = _flatModeForDisk;
  ReleaseFolder();
  _library.Free();
}

void CContentsView::OpenRootFolder()
{
  CDisableTimerProcessing disableTimerProcessing(*this);
  CDisableNotify disableNotify(*this);
  _parentFolders.Clear();
  SetToRootFolder();
  RefreshListCtrl(UString(), -1, true, UStringVector());
  // ::SetCurrentDirectory(::_currentFolderPrefix);
  /*
  BeforeChangeFolder();
  _currentFolderPrefix.Empty();
  AfterChangeFolder();
  SetCurrentPathText();
  RefreshListCtrl(UString(), 0, UStringVector());
  _listView.EnsureVisible(_listView.GetFocusedItem(), false);
  */
}

void CContentsView::OpenDrivesFolder()
{
  CloseOpenFolders();
  #ifdef UNDER_CE
  NFsFolder::CFSFolder *folderSpec = new NFsFolder::CFSFolder;
  SetNewFolder(folderSpec);
  folderSpec->InitToRoot();
  #else
  CFSDrives *folderSpec = new CFSDrives;
  SetNewFolder(folderSpec);
  folderSpec->Init();
  #endif
  RefreshListCtrl();
}

void CContentsView::OpenFolder(int index)
{
  if (index == kParentIndex)
  {
    OpenParentFolder();
    return;
  }
  CMyComPtr<IFolderFolder> newFolder;
  _folder->BindToFolder(index, &newFolder);
  if (!newFolder)
    return;
  SetNewFolder(newFolder);
  LoadFullPath();
  RefreshListCtrl();
  this->SetItemState(this->GetFocusedItem(), LVIS_SELECTED, LVIS_SELECTED);
  this->EnsureVisible(this->GetFocusedItem(), FALSE);
}

void CContentsView::OpenAltStreams()
{
  CRecordVector<UInt32> indices;
  GetOperatedItemIndices(indices);
  Int32 realIndex = -1;
  if (indices.Size() > 1)
    return;
  if (indices.Size() == 1)
    realIndex = indices[0];

  if (_folderAltStreams)
  {
    CMyComPtr<IFolderFolder> newFolder;
    _folderAltStreams->BindToAltStreams(realIndex, &newFolder);
    if (newFolder)
    {
      CDisableTimerProcessing disableTimerProcessing(*this);
      CDisableNotify disableNotify(*this);
      SetNewFolder(newFolder);
      RefreshListCtrl(UString(), -1, true, UStringVector());
      return;
    }
    return;
  }
  
  #if defined(_WIN32) && !defined(UNDER_CE)
  UString path;
  if (realIndex >= 0)
    path = GetItemFullPath(realIndex);
  else
  {
    path = GetFsPath();
    if (!NName::IsDriveRootPath_SuperAllowed(us2fs(path)))
      if (!path.IsEmpty() && IS_PATH_SEPAR(path.Back()))
        path.DeleteBack();
  }

  path += L':';
  BindToPathAndRefresh(path);
  #endif
}
