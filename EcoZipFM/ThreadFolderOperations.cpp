// ThreadFolderOperations.cpp

#include "StdAfx.h"

#include "EcoZipFM.h"
#include "Messages.h"
#include "FMUtils.h"
#include "ThreadFolderOperations.h"


using namespace NWindows;

#ifndef _UNICODE
extern bool g_IsNT;
#endif

HRESULT CThreadFolderOperations::ProcessVirt()
{
  NCOM::CComInitializer comInitializer;
  switch(OpType)
  {
    case FOLDER_TYPE_CREATE_FOLDER:
      Result = FolderOperations->CreateFolder(Name, UpdateCallback);
      break;
    case FOLDER_TYPE_DELETE:
      Result = FolderOperations->Delete(&Indices.Front(), Indices.Size(), UpdateCallback);
      break;
    case FOLDER_TYPE_RENAME:
      Result = FolderOperations->Rename(Index, Name, UpdateCallback);
      break;
    default:
      Result = E_FAIL;
  }
  return Result;
}


HRESULT CThreadFolderOperations::DoOperation(CWnd *panel, CObjectVector<CFolderLink> *parentFolders, const UString &progressTitle, const UString &titleError)
{
  UpdateCallbackSpec = new CUpdateCallback100Imp;
  UpdateCallback = UpdateCallbackSpec;
  UpdateCallbackSpec->ProgressDialog = &ProgressDialog;

  ProgressDialog.WaitMode = true;
  ProgressDialog.Sync.FinalMessage.ErrorMessage.Title = titleError;
  Result = S_OK;

  UpdateCallbackSpec->Init();

  if (parentFolders != NULL && parentFolders->Size() > 0)
  {
    const CFolderLink &fl = parentFolders->Back();
    UpdateCallbackSpec->PasswordIsDefined = fl.UsePassword;
    UpdateCallbackSpec->Password = fl.Password;
  }


  ProgressDialog.MainWindow = *panel->GetTopLevelParent(); // panel.GetParent()
  ProgressDialog.MainTitle = L"EcoZip"; // LangString(IDS_APP_TITLE);
  ProgressDialog.MainAddTitle = progressTitle + L' ';

  RINOK(Create(progressTitle, ProgressDialog.MainWindow));
  return Result;
}

