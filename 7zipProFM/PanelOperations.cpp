// PanelOperations.cpp

#include "StdAfx.h"

#include "CPP/Common/DynamicBuffer.h"
#include "CPP/Common/StringConvert.h"
#include "CPP/Common/Wildcard.h"

#include "CPP/Windows/COM.h"
#include "CPP/Windows/FileName.h"
#include "CPP/Windows/PropVariant.h"

#include "ComboDialog.h"

#include "CPP/7zip/UI/FileManager/FSFolder.h"
#include "CPP/7zip/UI/FileManager/FormatUtils.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"

#include "7zipProFM.h"
#include "ContentsView.h"
#include "Messages.h"
#include "FMUtils.h"


using namespace NWindows;
using namespace NFile;
using namespace NName;

#ifndef _UNICODE
extern bool g_IsNT;
#endif


#ifndef _UNICODE
typedef int (WINAPI * SHFileOperationWP)(LPSHFILEOPSTRUCTW lpFileOp);
#endif

/*
void CContentsView::MessageBoxErrorForUpdate(HRESULT errorCode, UINT resourceID)
{
  if (errorCode == E_NOINTERFACE)
    MessageBoxErrorLang(IDS_OPERATION_IS_NOT_SUPPORTED);
  else
    MessageBoxError(errorCode, LangString(resourceID));
}
*/

void CContentsView::DeleteItems(bool NON_CE_VAR(toRecycleBin))
{
  CDisableTimerProcessing disableTimerProcessing(*this);
  CRecordVector<UInt32> indices;
  GetOperatedItemIndices(indices);
  if (indices.IsEmpty())
    return;
  CSelectedState state;
  SaveSelectedState(state);

  #ifndef UNDER_CE
  // WM6 / SHFileOperationW doesn't ask user! So we use internal delete
  if (IsFSFolder() && toRecycleBin)
  {
    bool useInternalDelete = false;
    #ifndef _UNICODE
    if (!g_IsNT)
    {
      CDynamicBuffer<CHAR> buffer;
      FOR_VECTOR (i, indices)
      {
        const AString path = GetSystemString(GetItemFullPath(indices[i]));
        buffer.AddData(path, path.Len() + 1);
      }
      *buffer.GetCurPtrAndGrow(1) = 0;
      SHFILEOPSTRUCTA fo;
      fo.hwnd = GetParent();
      fo.wFunc = FO_DELETE;
      fo.pFrom = (const CHAR *)buffer;
      fo.pTo = 0;
      fo.fFlags = 0;
      if (toRecycleBin)
        fo.fFlags |= FOF_ALLOWUNDO;
      // fo.fFlags |= FOF_NOCONFIRMATION;
      // fo.fFlags |= FOF_NOERRORUI;
      // fo.fFlags |= FOF_SILENT;
      // fo.fFlags |= FOF_WANTNUKEWARNING;
      fo.fAnyOperationsAborted = FALSE;
      fo.hNameMappings = 0;
      fo.lpszProgressTitle = 0;
      /* int res = */ ::SHFileOperationA(&fo);
    }
    else
    #endif
    {
      CDynamicBuffer<WCHAR> buffer;
      unsigned maxLen = 0;
      const UString prefix = GetFsPath();
      FOR_VECTOR (i, indices)
      {
        // L"\\\\?\\") doesn't work here.
        const UString path = prefix + GetItemRelPath2(indices[i]);
        if (path.Len() > maxLen)
          maxLen = path.Len();
        buffer.AddData(path, path.Len() + 1);
      }
      *buffer.GetCurPtrAndGrow(1) = 0;
      if (maxLen >= MAX_PATH)
      {
        if (toRecycleBin)
        {
          MessageBoxErrorLang(IDS_ERROR_LONG_PATH_TO_RECYCLE);
          return;
        }
        useInternalDelete = true;
      }
      else
      {
        SHFILEOPSTRUCTW fo;
        fo.hwnd = *GetTopLevelParent();
        fo.wFunc = FO_DELETE;
        fo.pFrom = (const WCHAR *)buffer;
        fo.pTo = 0;
        fo.fFlags = 0;
        if (toRecycleBin)
          fo.fFlags |= FOF_ALLOWUNDO;
        fo.fAnyOperationsAborted = FALSE;
        fo.hNameMappings = 0;
        fo.lpszProgressTitle = 0;
        // int res;
        #ifdef _UNICODE
        /* res = */ ::SHFileOperationW(&fo);
        #else
        SHFileOperationWP shFileOperationW = (SHFileOperationWP)
          ::GetProcAddress(::GetModuleHandleW(L"shell32.dll"), "SHFileOperationW");
        if (shFileOperationW == 0)
          return;
        /* res = */ shFileOperationW(&fo);
        #endif
      }
    }
    /*
    if (fo.fAnyOperationsAborted)
      MessageBoxError(result, LangString(IDS_ERROR_DELETING, 0x03020217));
    */
    if (!useInternalDelete)
    {
      RefreshListCtrl(state);
      return;
    }
  }
  #endif
 
  // DeleteItemsInternal

  if (!CheckBeforeUpdate(IDS_ERROR_DELETING))
    return;

  UInt32 titleID, messageID;
  UString messageParam;
  if (indices.Size() == 1)
  {
    int index = indices[0];
    messageParam = GetItemRelPath2(index);
    if (IsItem_Folder(index))
    {
      titleID = IDS_CONFIRM_FOLDER_DELETE;
      messageID = IDS_WANT_TO_DELETE_FOLDER;
    }
    else
    {
      titleID = IDS_CONFIRM_FILE_DELETE;
      messageID = IDS_WANT_TO_DELETE_FILE;
    }
  }
  else
  {
    titleID = IDS_CONFIRM_ITEMS_DELETE;
    messageID = IDS_WANT_TO_DELETE_ITEMS;
    messageParam = NumberToString(indices.Size());
  }
  if (::MessageBoxW(*GetTopLevelParent(), MyFormatNew(messageID, messageParam), LangString(titleID), MB_OKCANCEL | MB_ICONQUESTION) != IDOK)
    return;

  CDisableNotify disableNotify(*this);
  {
    CThreadFolderOperations op(FOLDER_TYPE_DELETE);
    op.FolderOperations = _folderOperations;
    op.Indices = indices;
    op.DoOperation(this, &_parentFolders,
        LangString(IDS_DELETING),
        LangString(IDS_ERROR_DELETING));
  }
  RefreshTitle();
  RefreshListCtrl(state);
}


void CContentsView::OnLvnBeginlabeledit(NMHDR *pNMHDR, LRESULT *pResult)
{
  NMLVDISPINFO *lpnmh = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
  int realIndex = GetRealIndex(lpnmh->item);
  if (realIndex == kParentIndex)
    return;
  if (IsThereReadOnlyFolder())
    return;
  *pResult = 0;
}

bool CContentsView::CorrectFsPath(const UString &path2, UString &result)
{
  return ::CorrectFsPath(GetFsPath(), path2, result);
}


void CContentsView::OnLvnEndlabeledit(NMHDR *pNMHDR, LRESULT *pResult)
{
  NMLVDISPINFO *lpnmh = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
  if (lpnmh->item.pszText == NULL)
    return;
  CDisableTimerProcessing disableTimerProcessing2(*this);

  if (!CheckBeforeUpdate(IDS_ERROR_RENAMING))
    return;

  UString newName = lpnmh->item.pszText;
  if (!IsCorrectFsName(newName))
  {
    MessageBoxError(E_INVALIDARG);
    return;
  }

  if (IsFSFolder())
  {
    UString correctName;
    if (!CorrectFsPath(newName, correctName))
    {
      MessageBoxError(E_INVALIDARG);
      return;
    }
    newName = correctName;
  }

  SaveSelectedState(_selectedState);

  int realIndex = GetRealIndex(lpnmh->item);
  if (realIndex == kParentIndex)
    return;
  const UString prefix = GetItemPrefix(realIndex);


  CDisableNotify disableNotify(*this);
  {
    CThreadFolderOperations op(FOLDER_TYPE_RENAME);
    op.FolderOperations = _folderOperations;
    op.Index = realIndex;
    op.Name = newName;
    /* HRESULTres = */ op.DoOperation(this, &_parentFolders,
        LangString(IDS_RENAMING),
        LangString(IDS_ERROR_RENAMING));
    // fixed in 9.26: we refresh list even after errors
    // (it's more safe, since error can be at different stages, so list can be incorrect).
    /*
    if (res != S_OK)
      return FALSE;
    */
  }

  // Can't use RefreshListCtrl here.
  // RefreshListCtrlSaveFocused();
  _selectedState.FocusedName = prefix + newName;
  _selectedState.SelectFocused = true;

  // We need clear all items to disable GetText before Reload:
  // number of items can change.
  // _listView.DeleteAllItems();
  // But seems it can still call GetText (maybe for current item)
  // so we can't delete items.

  _dontShowMode = true;

  PostMessage(kReLoadMessage);

  *pResult = 0;
}


bool Dlg_CreateFolder(HWND wnd, UString &destName)
{
  destName.Empty();
  CComboDialog dlg;
  LangString(IDS_CREATE_FOLDER, dlg.Title);
  LangString(IDS_CREATE_FOLDER_NAME, dlg.Static);
  LangString(IDS_CREATE_FOLDER_DEFAULT_NAME, dlg.Value);
  if (dlg.DoModal() != IDOK)
    return false;
  destName = dlg.Value;
  return true;
}

void CContentsView::CreateFolder()
{
  if (!CheckBeforeUpdate(IDS_CREATE_FOLDER_ERROR))
    return;

  CDisableTimerProcessing disableTimerProcessing2(*this);
  CSelectedState state;
  SaveSelectedState(state);

  UString newName;
  if (!Dlg_CreateFolder(*GetTopLevelParent(), newName))
    return;
  
  if (!IsCorrectFsName(newName))
  {
    MessageBoxError(E_INVALIDARG);
    return;
  }

  if (IsFSFolder())
  {
    UString correctName;
    if (!CorrectFsPath(newName, correctName))
    {
      MessageBoxError(E_INVALIDARG);
      return;
    }
    newName = correctName;
  }
  
  HRESULT res;
  CDisableNotify disableNotify(*this);
  {
    CThreadFolderOperations op(FOLDER_TYPE_CREATE_FOLDER);
    op.FolderOperations = _folderOperations;
    op.Name = newName;
    res = op.DoOperation(this, &_parentFolders,
        LangString(IDS_CREATE_FOLDER),
        LangString(IDS_CREATE_FOLDER_ERROR));
    /*
    // fixed for 9.26: we must refresh always
    if (res != S_OK)
      return;
    */
  }
  if (res == S_OK)
  {
    int pos = newName.Find(WCHAR_PATH_SEPARATOR);
    if (pos >= 0)
      newName.DeleteFrom(pos);
    if (!_mySelectMode)
      state.SelectedNames.Clear();
    state.FocusedName = newName;
    state.SelectFocused = true;
  }
  RefreshTitle();
  RefreshListCtrl(state);
}

void CContentsView::CreateFile()
{
  if (!CheckBeforeUpdate(IDS_CREATE_FILE_ERROR))
    return;

  CDisableTimerProcessing disableTimerProcessing2(*this);
  CSelectedState state;
  SaveSelectedState(state);
  CComboDialog dlg;
  LangString(IDS_CREATE_FILE, dlg.Title);
  LangString(IDS_CREATE_FILE_NAME, dlg.Static);
  LangString(IDS_CREATE_FILE_DEFAULT_NAME, dlg.Value);

  if (dlg.DoModal() != IDOK)
    return;

  CDisableNotify disableNotify(*this);
  
  UString newName = dlg.Value;

  if (IsFSFolder())
  {
    UString correctName;
    if (!CorrectFsPath(newName, correctName))
    {
      MessageBoxError(E_INVALIDARG);
      return;
    }
    newName = correctName;
  }

  HRESULT result = _folderOperations->CreateFile(newName, 0);
  if (result != S_OK)
  {
    MessageBoxError(result, LangString(IDS_CREATE_FILE_ERROR));
    // MessageBoxErrorForUpdate(result, IDS_CREATE_FILE_ERROR);
    return;
  }
  int pos = newName.Find(WCHAR_PATH_SEPARATOR);
  if (pos >= 0)
    newName.DeleteFrom(pos);
  if (!_mySelectMode)
    state.SelectedNames.Clear();
  state.FocusedName = newName;
  state.SelectFocused = true;
  RefreshListCtrl(state);
}

void CContentsView::RenameFile()
{
  if (!CheckBeforeUpdate(IDS_ERROR_RENAMING))
    return;
  int index = this->GetFocusedItem();
  if (index >= 0)
    this->EditLabel(index);
}

void CContentsView::ChangeComment()
{
  if (!CheckBeforeUpdate(IDS_COMMENT))
    return;
  CDisableTimerProcessing disableTimerProcessing2(*this);
  int index = this->GetFocusedItem();
  if (index < 0)
    return;
  int realIndex = GetRealItemIndex(index);
  if (realIndex == kParentIndex)
    return;
  CSelectedState state;
  SaveSelectedState(state);
  UString comment;
  {
    NCOM::CPropVariant propVariant;
    if (_folder->GetProperty(realIndex, kpidComment, &propVariant) != S_OK)
      return;
    if (propVariant.vt == VT_BSTR)
      comment = propVariant.bstrVal;
    else if (propVariant.vt != VT_EMPTY)
      return;
  }
  UString name = GetItemRelPath2(realIndex);
  CComboDialog dlg;
  dlg.Title = name;
  dlg.Title += L" : ";
  AddLangString(dlg.Title, IDS_COMMENT);
  dlg.Value = comment;
  LangString(IDS_COMMENT2, dlg.Static);
  if (dlg.DoModal() != IDOK)
    return;
  NCOM::CPropVariant propVariant = dlg.Value.Ptr();

  CDisableNotify disableNotify(*this);
  HRESULT result = _folderOperations->SetProperty(realIndex, kpidComment, &propVariant, NULL);
  if (result != S_OK)
  {
    if (result == E_NOINTERFACE)
      MessageBoxErrorLang(IDS_OPERATION_IS_NOT_SUPPORTED);
    else
      MessageBoxError(result, L"Set Comment Error");
  }
  RefreshListCtrl(state);
}
