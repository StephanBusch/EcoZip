// ExtractDialog.cpp : implementation file
//

#include "stdafx.h"

#include "CPP/Common/StringConvert.h"
#include "CPP/Common/Wildcard.h"

#include "CPP/Windows/FileName.h"
#include "CPP/Windows/Shell.h"

#ifndef NO_REGISTRY
#include "CPP/7zip/UI/FileManager/HelpUtils.h"
#endif

#include "CPP/7zip/UI/FileManager/LangUtils.h"

#include "ExtractDialog.h"
#include "FMUtils.h"

#include <fstream>

static const UInt32 kPathMode_IDs[] =
{
  IDS_EXTRACT_PATHS_FULL,
  IDS_EXTRACT_PATHS_NO,
  IDS_EXTRACT_PATHS_ABS
};

static const UInt32 kOverwriteMode_IDs[] =
{
  IDS_EXTRACT_OVERWRITE_ASK,
  IDS_EXTRACT_OVERWRITE_WITHOUT_PROMPT,
  IDS_EXTRACT_OVERWRITE_SKIP_EXISTING,
  IDS_EXTRACT_OVERWRITE_RENAME,
  IDS_EXTRACT_OVERWRITE_RENAME_EXISTING
};

#ifndef _SFX

static const
  // NExtract::NPathMode::EEnum
  int
  kPathModeButtonsVals[] =
{
  NExtract::NPathMode::kFullPaths,
  NExtract::NPathMode::kNoPaths,
  NExtract::NPathMode::kAbsPaths
};

static const
  int
  // NExtract::NOverwriteMode::EEnum
  kOverwriteButtonsVals[] =
{
  NExtract::NOverwriteMode::kAsk,
  NExtract::NOverwriteMode::kOverwrite,
  NExtract::NOverwriteMode::kSkip,
  NExtract::NOverwriteMode::kRename,
  NExtract::NOverwriteMode::kRenameExisting
};

#endif

#ifdef LANG

static const UInt32 kLangIDs[] =
{
  IDT_EXTRACT_EXTRACT_TO,
  IDT_EXTRACT_PATH_MODE,
  IDT_EXTRACT_OVERWRITE_MODE,
  // IDX_EXTRACT_ALT_STREAMS,
  IDX_EXTRACT_NT_SECUR,
  IDX_EXTRACT_ELIM_DUP,
  IDG_PASSWORD,
  IDX_PASSWORD_SHOW
};
#endif

#ifndef NO_REGISTRY
static const unsigned kHistorySize = 16;
#endif

// CExtractDialog dialog

IMPLEMENT_DYNAMIC(CExtractDialog, CDialogEx)

CExtractDialog::CExtractDialog(CWnd* pParent /*=NULL*/)
	: CDialogEx(CExtractDialog::IDD, pParent)
    , PathMode_Force(false)
    , OverwriteMode_Force(false)
    , m_strPassword(_T(""))
{
  ElimDup.Val = true;
}

CExtractDialog::~CExtractDialog()
{
}

void CExtractDialog::DoDataExchange(CDataExchange* pDX)
{
  CDialogEx::DoDataExchange(pDX);
  DDX_Control(pDX, IDC_EXTRACT_PATH, m_cmbPath);
  #ifndef _SFX
  DDX_Control(pDX, IDE_EXTRACT_NAME, m_edtPathName);
  DDX_Control(pDX, IDC_EXTRACT_PATH_MODE, m_cmbPathMode);
  DDX_Control(pDX, IDC_EXTRACT_OVERWRITE_MODE, m_cmbOverwriteMode);
  DDX_Control(pDX, IDE_EXTRACT_PASSWORD, m_edtPasswordControl);
  DDX_CBString(pDX, IDE_EXTRACT_PASSWORD, m_strPassword);
  #endif
}


void CExtractDialog::CheckButton_TwoBools(UINT id, const CBoolPair &b1, const CBoolPair &b2)
{
  CheckDlgButton(id, GetBoolsVal(b1, b2) ? BST_CHECKED : BST_UNCHECKED);
}

void CExtractDialog::GetButton_Bools(UINT id, CBoolPair &b1, CBoolPair &b2)
{
  bool val = (IsDlgButtonChecked(id) == BST_CHECKED);
  bool oldVal = GetBoolsVal(b1, b2);
  if (val != oldVal)
    b1.Def = b2.Def = true;
  b1.Val = b2.Val = val;
}

BEGIN_MESSAGE_MAP(CExtractDialog, CDialogEx)
  ON_BN_CLICKED(IDC_EXTRACT_SET_PATH, &CExtractDialog::OnBnClickedExtractSetPath)
  ON_BN_CLICKED(IDX_EXTRACT_NAME_ENABLE, &CExtractDialog::OnBnClickedExtractNameEnable)
  ON_BN_CLICKED(IDX_PASSWORD_SHOW, &CExtractDialog::OnBnClickedPasswordShow)
END_MESSAGE_MAP()


// CExtractDialog message handlers


BOOL CExtractDialog::OnInitDialog()
{
  Password = GetUnicodeString(m_strPassword);

	CDialogEx::OnInitDialog();

	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

  #ifdef LANG
  {
    UString s;
    LangString_OnlyFromLangFile(IDD_EXTRACT, s);
    if (s.IsEmpty()) {
      CString str;
      GetWindowText(str);
      s = str;
    }
    if (!ArcPath.IsEmpty())
    {
      s.AddAscii(" : ");
      s += ArcPath;
    }
    ::SetWindowTextW(m_hWnd, s);
    // LangSetWindowText(*this, IDD_EXTRACT);
    LangSetDlgItems(*this, kLangIDs, ARRAY_SIZE(kLangIDs));
  }
  #endif
  
  #ifdef NO_REGISTRY
  
  PathMode = NExtract::NPathMode::kFullPaths;
  OverwriteMode = NExtract::NOverwriteMode::kAsk;
  
  #else
  
  _info.Load();

  if (_info.PathMode == NExtract::NPathMode::kCurPaths)
    _info.PathMode = NExtract::NPathMode::kFullPaths;

  if (!PathMode_Force && _info.PathMode_Force)
    PathMode = _info.PathMode;
  if (!OverwriteMode_Force && _info.OverwriteMode_Force)
    OverwriteMode = _info.OverwriteMode;

  // CheckButton_TwoBools(IDX_EXTRACT_ALT_STREAMS, AltStreams, _info.AltStreams);
  CheckButton_TwoBools(IDX_EXTRACT_NT_SECUR,    NtSecurity, _info.NtSecurity);
  CheckButton_TwoBools(IDX_EXTRACT_ELIM_DUP,    ElimDup,    _info.ElimDup);
  
  CheckDlgButton(IDX_PASSWORD_SHOW, _info.ShowPassword.Val ? BST_CHECKED : BST_UNCHECKED);
  UpdatePasswordControl();

  #endif

  UString pathPrefix = DirPath;

  #ifndef _SFX
  
  if (_info.SplitDest.Val)
  {
    CheckDlgButton(IDX_EXTRACT_NAME_ENABLE, BST_CHECKED);
    UString pathName;
    SplitPathToParts_Smart(DirPath, pathPrefix, pathName);
    if (pathPrefix.IsEmpty())
      pathPrefix = pathName;
    else
      m_edtPathName.SetWindowText(GetSystemString(pathName));
  }
  else
    GetDlgItem(IDE_EXTRACT_NAME)->ShowWindow(SW_HIDE);

  #endif

  ::SetWindowTextW(m_cmbPath, pathPrefix);

  #ifndef NO_REGISTRY
  for (unsigned i = 0; i < _info.Paths.Size() && i < kHistorySize; i++)
    m_cmbPath.AddString(GetSystemString(_info.Paths[i]));
  #endif

  /*
  if (_info.Paths.Size() > 0)
    _path.SetCurSel(0);
  else
    _path.SetCurSel(-1);
  */

  #ifndef _SFX

  AddComboItems(m_cmbPathMode, kPathMode_IDs, ARRAY_SIZE(kPathMode_IDs), kPathModeButtonsVals, PathMode);
  AddComboItems(m_cmbOverwriteMode, kOverwriteMode_IDs, ARRAY_SIZE(kOverwriteMode_IDs), kOverwriteButtonsVals, OverwriteMode);

  #endif

  // CWindow filesWindow = GetItem(IDC_EXTRACT_RADIO_FILES);
  // filesWindow.Enable(_enableFilesButton);

  return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

#ifndef _SFX
void CExtractDialog::UpdatePasswordControl()
{
  m_edtPasswordControl.SetPasswordChar(IsShowPasswordChecked() ? 0 : TEXT('*'));
  m_edtPasswordControl.RedrawWindow();
}
#endif


void CExtractDialog::OnBnClickedExtractSetPath()
{
  CString str;
  UString path;
  m_cmbPath.GetWindowText(str);
  path = GetUnicodeString(str);
  UString title = LangString(IDS_EXTRACT_SET_FOLDER);
  UString resultPath;

  #ifndef UNDER_CE
  if (!NWindows::NShell::BrowseForFolder(m_hWnd, title, path, resultPath))
    return;
  #endif

  #ifndef NO_REGISTRY
  m_cmbPath.SetCurSel(-1);
  #endif
  ::SetWindowTextW(m_cmbPath, resultPath);
}


void CExtractDialog::OnBnClickedExtractNameEnable()
{
  GetDlgItem(IDE_EXTRACT_NAME)->ShowWindow(
    (IsDlgButtonChecked(IDX_EXTRACT_NAME_ENABLE) == BST_CHECKED) ?
    SW_SHOW : SW_HIDE);
}


void CExtractDialog::OnBnClickedPasswordShow()
{
  UpdatePasswordControl();
}

void CExtractDialog::OnOK()
{
  UpdateData();

  #ifndef _SFX
  int pathMode2 = kPathModeButtonsVals[m_cmbPathMode.GetCurSel()];
  if (PathMode != NExtract::NPathMode::kCurPaths ||
      pathMode2 != NExtract::NPathMode::kFullPaths)
    PathMode = (NExtract::NPathMode::EEnum)pathMode2;

  OverwriteMode = (NExtract::NOverwriteMode::EEnum)kOverwriteButtonsVals[m_cmbOverwriteMode.GetCurSel()];

  // _filesMode = (NExtractionDialog::NFilesMode::EEnum)GetFilesMode();

  Password = GetUnicodeString(m_strPassword);

  #endif

  #ifndef NO_REGISTRY

  // GetButton_Bools(IDX_EXTRACT_ALT_STREAMS, AltStreams, _info.AltStreams);
  GetButton_Bools(IDX_EXTRACT_NT_SECUR,    NtSecurity, _info.NtSecurity);
  GetButton_Bools(IDX_EXTRACT_ELIM_DUP,    ElimDup,    _info.ElimDup);

  bool showPassword = IsShowPasswordChecked();
  if (showPassword != _info.ShowPassword.Val)
  {
    _info.ShowPassword.Def = true;
    _info.ShowPassword.Val = showPassword;
  }

  if (_info.PathMode != pathMode2)
  {
    _info.PathMode_Force = true;
    _info.PathMode = (NExtract::NPathMode::EEnum)pathMode2;
    /*
    // we allow kAbsPaths in registry.
    if (_info.PathMode == NExtract::NPathMode::kAbsPaths)
      _info.PathMode = NExtract::NPathMode::kFullPaths;
    */
  }

  if (!OverwriteMode_Force && _info.OverwriteMode != OverwriteMode)
    _info.OverwriteMode_Force = true;
  _info.OverwriteMode = OverwriteMode;


  #else
  
  ElimDup.Val = IsButtonCheckedBool(IDX_EXTRACT_ELIM_DUP);

  #endif
  
  UString s;
  
  #ifdef NO_REGISTRY
  
  _path.GetText(s);
  
  #else

  int currentItem = m_cmbPath.GetCurSel();
  if (currentItem == CB_ERR)
  {
    CString str;
    m_cmbPath.GetWindowText(str);
    s = GetUnicodeString(str);
    if (m_cmbPath.GetCount() >= kHistorySize)
      currentItem = m_cmbPath.GetCount() - 1;
  }
  else {
    CString str;
    m_cmbPath.GetLBText(currentItem, str);
    s = GetUnicodeString(str);
  }
  
  #endif

  s.Trim();
  NWindows::NFile::NName::NormalizeDirPathPrefix(s);
  
  #ifndef _SFX
  
  bool splitDest = (IsDlgButtonChecked(IDX_EXTRACT_NAME_ENABLE) == BST_CHECKED);
  if (splitDest)
  {
    CString pathName;
    m_edtPathName.GetWindowText(pathName);
    pathName.Trim();
    s += GetUnicodeString(pathName);
    NWindows::NFile::NName::NormalizeDirPathPrefix(s);
  }
  if (splitDest != _info.SplitDest.Val)
  {
    _info.SplitDest.Def = true;
    _info.SplitDest.Val = splitDest;
  }

  #endif

  DirPath = s;
  
  #ifndef NO_REGISTRY
  _info.Paths.Clear();
  #ifndef _SFX
  AddUniqueString(_info.Paths, s);
  #endif
  for (int i = 0; i < m_cmbPath.GetCount(); i++)
    if (i != currentItem)
    {
      CString sTemp;
      m_cmbPath.GetLBText(i, sTemp);
      sTemp.Trim();
      AddUniqueString(_info.Paths, GetUnicodeString(sTemp));
    }
  _info.Save();
  #endif
  
  CDialogEx::OnOK();
}

#ifndef NO_REGISTRY
static LPCWSTR kHelpTopic = L"fme/plugins/EcoZip/extract.htm";
void CExtractDialog::OnHelp()
{
  ShowHelpWindow(NULL, kHelpTopic);
  CDialogEx::OnHelp();
}
#endif
