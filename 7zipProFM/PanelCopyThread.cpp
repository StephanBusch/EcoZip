// PanelCopyThread.cpp

#include "StdAfx.h"

#include "HashGUI.h"
#include "PanelCopyThread.h"

HRESULT CPanelCopyThread::ProcessVirt()
{
  /*
  CMyComPtr<IFolderSetReplaceAltStreamCharsMode> iReplace;
  FolderOperations.QueryInterface(IID_IFolderSetReplaceAltStreamCharsMode, &iReplace);
  if (iReplace)
  {
    RINOK(iReplace->SetReplaceAltStreamCharsMode(ReplaceAltStreamChars ? 1 : 0));
  }
  */

  if (options->testMode)
  {
    CMyComPtr<IArchiveFolder> archiveFolder;
    FolderOperations.QueryInterface(IID_IArchiveFolder, &archiveFolder);
    if (!archiveFolder)
      return E_NOTIMPL;
    CMyComPtr<IFolderArchiveExtractCallback> extractCallback2;
    RINOK(ExtractCallback.QueryInterface(IID_IFolderArchiveExtractCallback, &extractCallback2));
    NExtract::NPathMode::EEnum pathMode =
        NExtract::NPathMode::kCurPaths;
        // NExtract::NPathMode::kFullPathnames;
    Result = archiveFolder->Extract(&Indices.Front(), Indices.Size(),
        BoolToInt(options->includeAltStreams),
        BoolToInt(options->replaceAltStreamChars),
        pathMode, NExtract::NOverwriteMode::kAsk,
        options->folder, BoolToInt(true), extractCallback2);
  }
  else
    Result = FolderOperations->CopyTo(
      BoolToInt(options->moveMode),
      &Indices.Front(), Indices.Size(),
      BoolToInt(options->includeAltStreams),
      BoolToInt(options->replaceAltStreamChars),
      options->folder, ExtractCallback);

  if (Result == S_OK && !ExtractCallbackSpec->ThereAreMessageErrors &&
      (!options->hashMethods.IsEmpty() || options->testMode))
  {
    CProgressMessageBoxPair &pair = GetMessagePair(false); // GetMessagePair(ExtractCallbackSpec->Hash.NumErrors != 0);
    AddHashBundleRes(pair.Message, Hash, FirstFilePath);
  }

  return Result;
}

