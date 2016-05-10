// PluginsPage.cpp : implementation file
//

#include "stdafx.h"

#include "CPP/Common/IntToString.h"
#include "CPP/Common/StringConvert.h"

#include "CPP/Windows/Defs.h"
#include "CPP/Windows/DLL.h"
#include "CPP/Windows/PropVariant.h"
#include "CPP/Windows/ErrorMsg.h"

#include "CPP/7zip/Common/CreateCoder.h"
#include "CPP/7zip/UI/FileManager/HelpUtils.h"
#include "CPP/7zip/UI/FileManager/IFolder.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "LoadCodecs.h"
#include "PanelCopyThread.h"
#include "FMUtils.h"

#include "7zipProFM.h"
#include "PluginsPage.h"
#include "PluginInfoDlg.h"


static const UInt32 kLangIDs[] =
{
  IDT_BASE_FOLDER
};

static LPCWSTR kSystemTopic = L"FME/options.htm#system";

// CPluginsPage dialog

IMPLEMENT_DYNAMIC(CPluginsPage, CPropertyPage)

CPluginsPage::CPluginsPage()
  : CPropertyPage(CPluginsPage::IDD)
{

}

CPluginsPage::~CPluginsPage()
{
}

void CPluginsPage::DoDataExchange(CDataExchange* pDX)
{
  CPropertyPage::DoDataExchange(pDX);
  DDX_Control(pDX, IDC_LST_PLUGINS, m_lstPlugins);
}


BEGIN_MESSAGE_MAP(CPluginsPage, CPropertyPage)
  ON_BN_CLICKED(IDC_ADD, &CPluginsPage::OnBnClickedAdd)
END_MESSAGE_MAP()


// CPluginsPage message handlers


extern FString GetBaseFolderPrefixFromRegistry();

HRESULT ReadNumberOfStreams(ICompressCodecsInfo *codecsInfo, UInt32 index, PROPID propID, UInt32 &res)
{
  NWindows::NCOM::CPropVariant prop;
  RINOK(codecsInfo->GetProperty(index, propID, &prop));
  if (prop.vt == VT_EMPTY)
    res = 1;
  else if (prop.vt == VT_UI4)
    res = prop.ulVal;
  else
    return E_INVALIDARG;
  return S_OK;
}

HRESULT ReadIsAssignedProp(ICompressCodecsInfo *codecsInfo, UInt32 index, PROPID propID, bool &res)
{
  NWindows::NCOM::CPropVariant prop;
  RINOK(codecsInfo->GetProperty(index, propID, &prop));
  if (prop.vt == VT_EMPTY)
    res = true;
  else if (prop.vt == VT_BOOL)
    res = VARIANT_BOOLToBool(prop.boolVal);
  else
    return E_INVALIDARG;
  return S_OK;
}

BOOL CPluginsPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

  LangSetDlgItems(*this, kLangIDs, ARRAY_SIZE(kLangIDs));

  const FString baseFolder = GetBaseFolderPrefixFromRegistry();

  SetDlgItemText(IDT_BASE_FOLDER_VAL, baseFolder);

  // set style for tree view
  UINT uTreeStyle = TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | /*TVS_CHECKBOXES | */TVS_FULLROWSELECT;
  m_lstPlugins.GetTreeCtrl().ModifyStyle(0, uTreeStyle);

  /*
   * Set background image (works with owner-drawn tree only)
   */

  LVBKIMAGE bk;
  bk.xOffsetPercent = bk.yOffsetPercent = 70;
  bk.hbm = LoadBitmap(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_LOGO));
  m_lstPlugins.GetTreeCtrl().SetBkImage(&bk);

  /*
   * Create image list for tree & load icons
   */

  m_imgList.Create(16, 16, ILC_MASK | ILC_COLOR32, 0, 0);

  // assign image list to tree control
  m_lstPlugins.GetTreeCtrl().SetImageList(&m_imgList, TVSIL_NORMAL);

  /*
   *  Insert columns to tree control
   */

  m_lstPlugins.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 120);
  m_lstPlugins.InsertColumn(1, _T("ID"), LVCFMT_CENTER, 60);
  m_lstPlugins.InsertColumn(2, _T("Decoder"), LVCFMT_CENTER, 70);
  m_lstPlugins.InsertColumn(3, _T("Encoder"), LVCFMT_CENTER, 70);
  m_lstPlugins.InsertColumn(4, _T("InStreams"), LVCFMT_CENTER, 70);
  m_lstPlugins.InsertColumn(5, _T("OutStreams"), LVCFMT_CENTER, 75);
  m_lstPlugins.InsertColumn(6, _T("Description"), LVCFMT_CENTER, 80);
  m_lstPlugins.InsertColumn(7, _T("DigestSize"), LVCFMT_CENTER, 70);

  CCodecs *codecs = new CCodecs;
  if (FAILED(codecs->Load()))
    return FALSE;

  CCustomTreeChildCtrl &ctrl = m_lstPlugins.GetTreeCtrl();

  FOR_VECTOR(j, codecs->Libs) {
    const CCodecLib &lib = codecs->Libs[j];
    FString path = lib.Path;
    if (path.Mid(0, baseFolder.Len()).Compare(baseFolder) == 0)
      path = path.Mid(baseFolder.Len(), path.Len() - baseFolder.Len());
    HTREEITEM hPlugin = ctrl.InsertItem(path);

    UInt32 num;
    if (FAILED(codecs->GetNumMethods(&num)))
      return FALSE;
    if (num > 0) {
      HTREEITEM hCodecs = ctrl.InsertItem(_T("Codecs"), hPlugin);
      for (UInt32 i = 0; i < num; i++) {
        CDllCodecInfo &codecInfo = codecs->Codecs[i];
        if (codecInfo.LibIndex != j)
          continue;
        CCodecInfoEx info;
        if (FAILED(codecs->GetCodec_Id(i, info.Id)))
          continue;
        info.Name = codecs->GetCodec_Name(i);

        ReadNumberOfStreams(codecs, i, NMethodPropID::kPackStreams, info.NumStreams);
        ReadNumberOfStreams(codecs, i, NMethodPropID::kUnpackStreams, info.NumStreams);
        ReadIsAssignedProp(codecs, i, NMethodPropID::kEncoderIsAssigned, info.EncoderIsAssigned);
        ReadIsAssignedProp(codecs, i, NMethodPropID::kDecoderIsAssigned, info.DecoderIsAssigned);

        HTREEITEM hCodec = ctrl.InsertItem(GetSystemString(info.Name), hCodecs);
        CHAR szBuff[32];
        ConvertUInt64ToHex(info.Id, szBuff);
        m_lstPlugins.SetItemText(hCodec, 1, GetSystemString(szBuff));
        if (info.DecoderIsAssigned)
          m_lstPlugins.SetItemText(hCodec, 2, _T("Assigned"));
        if (info.EncoderIsAssigned)
          m_lstPlugins.SetItemText(hCodec, 3, _T("Assigned"));
        ConvertUInt32ToString(info.NumStreams, szBuff);
        m_lstPlugins.SetItemText(hCodec, 4, GetSystemString(szBuff));
        ConvertUInt32ToString(info.NumStreams, szBuff);
        m_lstPlugins.SetItemText(hCodec, 5, GetSystemString(szBuff));
      }

      ctrl.Expand(hCodecs, TVE_EXPAND);
    }

    num = codecs->GetNumHashers();
    if (num > 0) {
      HTREEITEM hHashers = ctrl.InsertItem(_T("Hashers"), hPlugin);
      for (UInt32 i = 0; i < num; i++)
      {
        CDllHasherInfo &hasherInfo = codecs->Hashers[i];
        if (hasherInfo.LibIndex != j)
          continue;
        CHasherInfoEx info;
        if (FAILED(info.Id = codecs->GetHasherId(i)))
          return FALSE;
        info.Name = codecs->GetHasherName(i);
        UInt32 DigestSize = codecs->GetHasherDigestSize(i);

        HTREEITEM hHasher = ctrl.InsertItem(GetSystemString(info.Name), hHashers);
        CHAR szBuff[32];
        ConvertUInt64ToHex(info.Id, szBuff);
        m_lstPlugins.SetItemText(hHasher, 1, GetSystemString(szBuff));
        ConvertUInt32ToString(DigestSize, szBuff);
        m_lstPlugins.SetItemText(hHasher, 7, GetSystemString(szBuff));
      }

      ctrl.Expand(hHashers, TVE_EXPAND);
    }

    ctrl.Expand(hPlugin, TVE_EXPAND);
  }

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPluginsPage::OnApply()
{
	return CPropertyPage::OnApply();
}


void CPluginsPage::OnBnClickedAdd()
{
  TCHAR strFilter[] = _T("Codec DLLs (*.dll)|*.dll|All Files (*.*)|*.*||");
  CFileDialog dlgOpen(TRUE, _T("dll"), NULL,
    OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT, strFilter);
  FString str;
  int nMaxFiles = 256;
  int nBufferSize = nMaxFiles * MAX_PATH + 1;
  //dlgOpen.GetOFN().lpstrFile = str.GetBuffer(nBufferSize);
  if (dlgOpen.DoModal() != IDOK)
    return;
  CPluginInfoDlg dlg;
  CString curBaseFolder = dlgOpen.GetFolderPath() + _T("\\");
  const FString baseFolder = GetBaseFolderPrefixFromRegistry();
  if (curBaseFolder.Compare(baseFolder) == 0)
    return;
  dlg.m_strBaseFolder = curBaseFolder;

  POSITION pos = dlgOpen.GetStartPosition();
  while (pos) {
    CString strName = dlgOpen.GetNextPathName(pos);
    if (strName.Mid(0, curBaseFolder.GetLength()).Compare(curBaseFolder) == 0) {
      strName = strName.Mid(curBaseFolder.GetLength(),
        strName.GetLength() - curBaseFolder.GetLength());
    }
    dlg.m_codecDlls.Add(FString(strName));
  }
  if (dlg.DoModal() != IDOK)
    return;

  // Copy DLLs

  CCopyToOptions options;
  options.folder = baseFolder;
  options.moveMode = false;
  options.includeAltStreams = true;
  options.replaceAltStreamChars = false;
  options.showErrorMessages = true;

  UInt32 numItems;
  CMyComPtr<IFolderFolder> folder;
  folder = GetFolder(dlg.m_strBaseFolder);
  if (!folder)
    return;
  if (FAILED(folder->LoadItems()))
    return;
  if (folder->GetNumberOfItems(&numItems) != S_OK || numItems <= 0)
    return;

  CMyComPtr<IFolderGetItemName> folderGetItemName;
  folder.QueryInterface(IID_IFolderGetItemName, &folderGetItemName);

  UString itemName;
  CRecordVector<UInt32> indices;
  for (UInt32 i = 0; i < numItems; i++) {
    const wchar_t *name = NULL;
    unsigned nameLen = 0;
    if (folderGetItemName)
      folderGetItemName->GetItemName(i, &name, &nameLen);
    if (name == NULL)
    {
      NWindows::NCOM::CPropVariant prop;
      if (folder->GetProperty(i, kpidName, &prop) != S_OK)
        break;// throw 2723400;
      if (prop.vt != VT_BSTR)
        break;// throw 2723401;
      itemName.Empty();
      itemName += prop.bstrVal;

      name = itemName;
      nameLen = itemName.Len();
    }

    int nIndex = dlg.m_codecDlls.FindInSorted(GetSystemString(name));
    if (nIndex < 0)
      continue;
    indices.Add(i);
  }

  bool showProgress = true;
  CWaitCursor *waitCursor = NULL;
  if (!showProgress)
    waitCursor = new CWaitCursor;
  CMyComPtr<IFolderOperations> folderOperations;
  if (folder.QueryInterface(IID_IFolderOperations, &folderOperations) != S_OK)
  {
    UString errorMessage = LangString(IDS_OPERATION_IS_NOT_SUPPORTED);
    MessageBox(errorMessage);
    if (waitCursor != NULL)
      delete waitCursor;
    return;
  }

    HRESULT res;
    {
      CPanelCopyThread extracter;

      extracter.ExtractCallbackSpec = new CExtractCallbackImp;
      extracter.ExtractCallback = extracter.ExtractCallbackSpec;

      extracter.options = &options;
      extracter.ExtractCallbackSpec->ProgressDialog = &extracter.ProgressDialog;
      extracter.ProgressDialog.CompressingMode = false;

      extracter.ExtractCallbackSpec->StreamMode = options.streamMode;

#ifdef EXTERNAL_CODECS
      CExternalCodecs __externalCodecs;
#else
      CMyComPtr<IUnknown> compressCodecsInfo;
#endif

      if (indices.Size() == 1)
        extracter.FirstFilePath = GetItemRelPath(folder, indices[0]);

      if (options.VirtFileSystem)
      {
        extracter.ExtractCallbackSpec->VirtFileSystem = options.VirtFileSystem;
        extracter.ExtractCallbackSpec->VirtFileSystemSpec = options.VirtFileSystemSpec;
      }
      extracter.ExtractCallbackSpec->ProcessAltStreams = options.includeAltStreams;

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

      UString progressWindowTitle = L"7-Zip"; // LangString(IDS_APP_TITLE);

      extracter.ProgressDialog.MainWindow = *GetTopLevelParent();
      extracter.ProgressDialog.MainTitle = progressWindowTitle;
      extracter.ProgressDialog.MainAddTitle = title + L' ';

      extracter.ExtractCallbackSpec->OverwriteMode =
        showProgress ? NExtract::NOverwriteMode::kAsk : NExtract::NOverwriteMode::kSkip;
      extracter.ExtractCallbackSpec->Init();
      extracter.Indices = indices;
      extracter.FolderOperations = folderOperations;
      extracter.ExtractCallbackSpec->PasswordIsDefined = false;

      if (FAILED(extracter.Create(title, *GetTopLevelParent(), showProgress))) {
        if (waitCursor != NULL)
          delete waitCursor;
        return;
      }

      res = extracter.Result;

      if (res == S_OK && extracter.ExtractCallbackSpec->IsOK()) {
        UString errorMessage = _T("Codecs are added successfully.");
        MessageBox(errorMessage);
      }
      else if (!extracter.ProgressDialog.Sync.Messages.IsEmpty())
        MessageBox(extracter.ProgressDialog.Sync.Messages[0]);
    }

  if (waitCursor != NULL)
    delete waitCursor;
}
