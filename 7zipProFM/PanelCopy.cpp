/// PanelCopy.cpp

#include "StdAfx.h"

#include "CPP/Common/MyException.h"

#include "HashGUI.h"

#include "ProgressDialog2.h"

#include "ExtractCallback.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "UpdateCallback100.h"

#include "7zipProFM.h"
#include "ContentsView.h"

using namespace NWindows;

/*
#ifdef EXTERNAL_CODECS

static void ThrowException_if_Error(HRESULT res)
{
  if (res != S_OK)
    throw CSystemException(res);
}

#endif
*/


HRESULT CContentsView::CopyTo(CCopyToOptions &options,
  const CRecordVector<UInt32> &indices,
    UStringVector *messages,
    bool &usePassword, UString &password,
    bool showProgress)
{
  CWaitCursor *waitCursor = NULL;
  if (!showProgress)
    waitCursor = new CWaitCursor;
  if (!_folderOperations)
  {
    UString errorMessage = LangString(IDS_OPERATION_IS_NOT_SUPPORTED);
    if (options.showErrorMessages)
      MessageBox(errorMessage);
    else if (messages != 0)
      messages->Add(errorMessage);
    if (waitCursor != NULL)
      delete waitCursor;
    return E_FAIL;
  }

  HRESULT res = S_OK;

  {
  /*
  #ifdef EXTERNAL_CODECS
  CExternalCodecs g_ExternalCodecs;
  #endif
  */
  /* extracter.Hash uses g_ExternalCodecs
     extracter must be declared after g_ExternalCodecs for correct destructor order !!! */

  CPanelCopyThread extracter;

  extracter.ExtractCallbackSpec = new CExtractCallbackImp;
  extracter.ExtractCallback = extracter.ExtractCallbackSpec;

  extracter.options = &options;
  extracter.ExtractCallbackSpec->ProgressDialog = &extracter.ProgressDialog;
  extracter.ProgressDialog.CompressingMode = false;

  extracter.ExtractCallbackSpec->StreamMode = options.streamMode;


  if (indices.Size() == 1)
    extracter.FirstFilePath = GetItemRelPath(indices[0]);

  if (options.VirtFileSystem)
  {
    extracter.ExtractCallbackSpec->VirtFileSystem = options.VirtFileSystem;
    extracter.ExtractCallbackSpec->VirtFileSystemSpec = options.VirtFileSystemSpec;
  }
  extracter.ExtractCallbackSpec->ProcessAltStreams = options.includeAltStreams;

  if (!options.hashMethods.IsEmpty())
  {
    /* this code is used when we call CRC calculation for files in side archive
       But new code uses global codecs so we don't need to call LoadGlobalCodecs again */

    /*
    #ifdef EXTERNAL_CODECS
    ThrowException_if_Error(LoadGlobalCodecs());
    #endif
    */

    extracter.Hash.SetMethods(EXTERNAL_CODECS_VARS_G options.hashMethods);
    extracter.ExtractCallbackSpec->SetHashMethods(&extracter.Hash);
  }
  else if (options.testMode)
  {
    extracter.ExtractCallbackSpec->SetHashCalc(&extracter.Hash);
  }

  extracter.Hash.Init();

  UString title;
  {
    UInt32 titleID = IDS_COPYING;
    if (options.moveMode)
      titleID = IDS_MOVING;
    else if (!options.hashMethods.IsEmpty() && options.streamMode)
    {
      titleID = IDS_CHECKSUM_CALCULATING;
      if (options.hashMethods.Size() == 1)
      {
        const UString &s = options.hashMethods[0];
        if (s != L"*")
          title = s;
      }
    }
    else if (options.testMode)
      titleID = IDS_PROGRESS_TESTING;

    if (title.IsEmpty())
      title = LangString(titleID);
  }

  UString progressWindowTitle = L"7-ZipPro"; // LangString(IDS_APP_TITLE);

  extracter.ProgressDialog.MainWindow = *GetTopLevelParent();
  extracter.ProgressDialog.MainTitle = progressWindowTitle;
  extracter.ProgressDialog.MainAddTitle = title + L' ';

  extracter.ExtractCallbackSpec->OverwriteMode = NExtract::NOverwriteMode::kAsk;
  extracter.ExtractCallbackSpec->Init();
  extracter.Indices = indices;
  extracter.FolderOperations = _folderOperations;

  extracter.ExtractCallbackSpec->PasswordIsDefined = usePassword;
  extracter.ExtractCallbackSpec->Password = password;
  
  RINOK(extracter.Create(title, *GetTopLevelParent()));
  
  if (messages != 0)
    *messages = extracter.ProgressDialog.Sync.Messages;
  res = extracter.Result;

  if (res == S_OK && extracter.ExtractCallbackSpec->IsOK())
  {
    usePassword = extracter.ExtractCallbackSpec->PasswordIsDefined;
    password = extracter.ExtractCallbackSpec->Password;
  }
  }
  
  RefreshTitle();

  if (waitCursor != NULL)
    delete waitCursor;

  return res;
}


struct CThreadUpdate
{
  CMyComPtr<IFolderOperations> FolderOperations;
  UString FolderPrefix;
  UStringVector FileNames;
  CRecordVector<const wchar_t *> FileNamePointers;
  CProgressDialog ProgressDialog;
  CMyComPtr<IFolderArchiveUpdateCallback> UpdateCallback;
  CUpdateCallback100Imp *UpdateCallbackSpec;
  HRESULT Result;
  bool MoveMode;
  
  void Process()
  {
    try
    {
      CProgressCloser closer(ProgressDialog);
      Result = FolderOperations->CopyFrom(
        MoveMode,
        FolderPrefix,
        &FileNamePointers.Front(),
        FileNamePointers.Size(),
        UpdateCallback);
    }
    catch(...) { Result = E_FAIL; }
  }
  static UINT WINAPI MyThreadFunction(void *param)
  {
    ((CThreadUpdate *)param)->Process();
    return 0;
  }
};

HRESULT CContentsView::CopyFrom(bool moveMode, const UString &folderPrefix, const UStringVector &filePaths,
    bool showErrorMessages, UStringVector *messages)
{
  HRESULT res;
  if (!_folderOperations)
    res = E_NOINTERFACE;
  else
  {
  CThreadUpdate updater;
  updater.MoveMode = moveMode;
  updater.UpdateCallbackSpec = new CUpdateCallback100Imp;
  updater.UpdateCallback = updater.UpdateCallbackSpec;
  updater.UpdateCallbackSpec->Init();

  updater.UpdateCallbackSpec->ProgressDialog = &updater.ProgressDialog;

  UString title = LangString(IDS_COPYING);
  UString progressWindowTitle = L"7-Zip"; // LangString(IDS_APP_TITLE);

  updater.ProgressDialog.MainWindow = *GetTopLevelParent();
  updater.ProgressDialog.MainTitle = progressWindowTitle;
  updater.ProgressDialog.MainAddTitle = title + L' ';
  
  {
    if (!_parentFolders.IsEmpty())
    {
      const CFolderLink &fl = _parentFolders.Back();
      updater.UpdateCallbackSpec->PasswordIsDefined = fl.UsePassword;
      updater.UpdateCallbackSpec->Password = fl.Password;
    }
  }

  updater.FolderOperations = _folderOperations;
  updater.FolderPrefix = folderPrefix;
  updater.FileNames.ClearAndReserve(filePaths.Size());
  unsigned i;
  for (i = 0; i < filePaths.Size(); i++)
    updater.FileNames.AddInReserved(filePaths[i]);
  updater.FileNamePointers.ClearAndReserve(updater.FileNames.Size());
  for (i = 0; i < updater.FileNames.Size(); i++)
    updater.FileNamePointers.AddInReserved(updater.FileNames[i]);

  unsigned threadID;
  HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, &CThreadUpdate::MyThreadFunction, &updater, 0, &threadID);
  if (hThread == NULL)
    return E_FAIL;
  updater.ProgressDialog.Create(title, hThread, *GetTopLevelParent());
  
  if (messages != 0)
    *messages = updater.ProgressDialog.Sync.Messages;

  res = updater.Result;
  }

  if (res == E_NOINTERFACE)
  {
    UString errorMessage = LangString(IDS_OPERATION_IS_NOT_SUPPORTED);
    if (showErrorMessages)
      MessageBox(errorMessage);
    else if (messages != 0)
      messages->Add(errorMessage);
    return E_ABORT;
  }

  RefreshTitle();
  return res;
}

void CContentsView::CopyFromNoAsk(const UStringVector &filePaths)
{
  CDisableTimerProcessing disableTimerProcessing(*this);

  CSelectedState srcSelState;
  SaveSelectedState(srcSelState);

  HRESULT result = CopyFrom(false, L"", filePaths, true, 0);

  CDisableNotify disableNotify(*this);

  if (result != S_OK)
  {
    disableNotify.Restore();
    // For Password:
    this->SetFocus();
    if (result != E_ABORT)
      MessageBoxError(result);
    return;
  }

  RefreshListCtrl(srcSelState);

  disableNotify.Restore();
  this->SetFocus();
}

void CContentsView::CopyFromAsk(const UStringVector &filePaths)
{
  UString title = LangString(IDS_CONFIRM_FILE_COPY);
  UString message = LangString(IDS_WANT_TO_COPY_FILES);
  message += L"\n\'";
  message += _currentFolderPrefix;
  message += L"\' ?";
  int res = ::MessageBoxW(*(this), message, title, MB_YESNOCANCEL | MB_ICONQUESTION);
  if (res != IDYES)
    return;

  CopyFromNoAsk(filePaths);
}
