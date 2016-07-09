// CompressDialog.cpp : implementation file
//

#include "stdafx.h"

#include "C/CpuArch.h"

#include "CPP/Common/IntToString.h"
#include "CPP/Common/StringConvert.h"

#include "CPP/Windows/FileDir.h"
#include "CPP/Windows/FileName.h"
#include "CPP/Windows/System.h"

#include "CPP/7zip/UI/FileManager/FormatUtils.h"
#include "CPP/7zip/UI/FileManager/HelpUtils.h"
#include "SplitUtils.h"
#ifdef LANG
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#endif

#include "CPP/7zip/UI/Explorer/MyMessages.h"

#include "CPP/7zip/Compress/LibbscCoder.h"

#include "CompressDialog.h"
#include "FMUtils.h"

#include <fstream>


#ifdef LANG
static const UInt32 kLangIDs[] =
{
  IDT_COMPRESS_ARCHIVE,
  IDT_COMPRESS_UPDATE_MODE,
  IDT_COMPRESS_FORMAT,
  IDT_COMPRESS_LEVEL,
  IDT_COMPRESS_METHOD,
  IDT_COMPRESS_DICTIONARY,
  IDT_COMPRESS_ORDER,
  IDT_COMPRESS_SOLID,
  IDT_COMPRESS_THREADS,
  IDT_COMPRESS_PARAMETERS,
  
  IDG_COMPRESS_OPTIONS,
  IDX_COMPRESS_SFX,
  IDX_COMPRESS_SHARED,
  IDX_LOAD_CONFIG,
  IDX_COMPRESS_DEL,

  IDT_COMPRESS_MEMORY,
  IDT_COMPRESS_MEMORY_DE,

  IDX_COMPRESS_NT_SYM_LINKS,
  IDX_COMPRESS_NT_HARD_LINKS,
  IDX_COMPRESS_NT_ALT_STREAMS,
  IDX_COMPRESS_NT_SECUR,

  IDG_COMPRESS_ENCRYPTION,
  IDT_COMPRESS_ENCRYPTION_METHOD,
  IDX_COMPRESS_ENCRYPT_FILE_NAMES,

  IDT_PASSWORD_ENTER,
  IDT_PASSWORD_REENTER,
  IDX_PASSWORD_SHOW,

  IDT_SPLIT_TO_VOLUMES,
  IDT_COMPRESS_PATH_MODE
};
#endif

using namespace NWindows;
using namespace NFile;
using namespace NName;
using namespace NDir;

static const unsigned kHistorySize = 20;

static LPCWSTR kExeExt = L".exe";
static LPCWSTR k7zeFormat = L"7ze";

static const char kInifileName[] = "7z.groups.ini";
static bool bConfigLoadable = false;

static const UInt32 g_Levels[] =
{
  IDS_METHOD_STORE,
  IDS_METHOD_FASTEST,
  0,
  IDS_METHOD_FAST,
  0,
  IDS_METHOD_NORMAL,
  0,
  IDS_METHOD_MAXIMUM,
  0,
  IDS_METHOD_ULTRA
};

enum EMethodID
{
  kCopy,
  kLZMA,
  kLZMA2,
  kPPMd,
  kBZip2,
  kDeflate,
  kDeflate64,
  kPPMdZip,
  kLIBBSC,
  kCSC32
};

static const LPCWSTR kMethodsNames[] =
{
  L"Copy",
  L"LZMA",
  L"LZMA2",
  L"PPMd",
  L"BZip2",
  L"Deflate",
  L"Deflate64",
  L"PPMd",
  L"LIBBSC",
  L"CSC32",
};

static const EMethodID g_7zMethods[] =
{
  kLZMA2,
  kLZMA,
  kPPMd,
  kBZip2
};

static const EMethodID g_7zeMethods[] =
{
  kLZMA2,
  kLZMA,
  kPPMd,
  kBZip2,
  kLIBBSC,
  kCSC32
};

static const EMethodID g_7zeSfxMethods[] =
{
  kCopy,
  kLZMA,
  kLZMA2,
  kPPMd,
  kLIBBSC,
  kCSC32
};

static const EMethodID g_ZipMethods[] =
{
  kDeflate,
  kDeflate64,
  kBZip2,
  kLZMA,
  kPPMdZip
};

static const EMethodID g_GZipMethods[] =
{
  kDeflate
};

static const EMethodID g_BZip2Methods[] =
{
  kBZip2
};

static const EMethodID g_XzMethods[] =
{
  kLZMA2
};

static const EMethodID g_SwfcMethods[] =
{
  kDeflate
  // kLZMA
};

struct CFormatInfo
{
  LPCWSTR Name;
  UInt32 LevelsMask;
  const EMethodID *MathodIDs;
  unsigned NumMethods;
  bool Filter;
  bool Solid;
  bool MultiThread;
  bool SFX;

  bool Encrypt;
  bool EncryptFileNames;
};

#define METHODS_PAIR(x) x, ARRAY_SIZE(x)

static const CFormatInfo g_Formats[] =
{
  {
    L"",
    (1 << 0) | (1 << 1) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 9),
    0, 0,
    false, false, false, false, false, false
  },
  {
    L"7z",
    (1 << 0) | (1 << 1) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 9),
    METHODS_PAIR(g_7zMethods),
    true, true, true, true, true, true
  },
  {
    k7zeFormat,
    (1 << 0) | (1 << 1) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 9),
    METHODS_PAIR(g_7zeMethods),
    true, true, true, false, true, true
  },
  {
    L"Zip",
    (1 << 0) | (1 << 1) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 9),
    METHODS_PAIR(g_ZipMethods),
    false, false, true, false, true, false
  },
  {
    L"GZip",
    (1 << 1) | (1 << 5) | (1 << 7) | (1 << 9),
    METHODS_PAIR(g_GZipMethods),
    false, false, false, false, false, false
  },
  {
    L"BZip2",
    (1 << 1) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 9),
    METHODS_PAIR(g_BZip2Methods),
    false, false, true, false, false, false
  },
  {
    L"xz",
    (1 << 1) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 9),
    METHODS_PAIR(g_XzMethods),
    false, false, true, false, false, false
  },
  {
    L"Swfc",
    (1 << 1) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 9),
    METHODS_PAIR(g_SwfcMethods),
    false, false, true, false, false, false
  },
  {
    L"Tar",
    (1 << 0),
    0, 0,
    false, false, false, false, false, false
  },
  {
    L"wim",
    (1 << 0),
    0, 0,
    false, false, false, false, false, false
  }
};

static bool IsMethodSupportedBySfx(int methodID)
{
  for (int i = 0; i < ARRAY_SIZE(g_7zeSfxMethods); i++)
    if (methodID == g_7zeSfxMethods[i])
      return true;
  return false;
}

static bool GetMaxRamSizeForProgram(UInt64 &physSize)
{
  physSize = (UInt64)(sizeof(size_t)) << 29;
  bool ramSize_Defined = NSystem::GetRamSize(physSize);
  const UInt64 kMinSysSize = (1 << 24);
  if (physSize <= kMinSysSize)
    physSize = 0;
  else
    physSize -= kMinSysSize;
  const UInt64 kMinUseSize = (1 << 24);
  if (physSize < kMinUseSize)
    physSize = kMinUseSize;
  return ramSize_Defined;
}


static const
  // NCompressDialog::NUpdateMode::EEnum
  int
  k_UpdateMode_Vals[] =
{
  NCompressDialog::NUpdateMode::kAdd,
  NCompressDialog::NUpdateMode::kUpdate,
  NCompressDialog::NUpdateMode::kFresh,
  NCompressDialog::NUpdateMode::kSync
};
  
static const UInt32 k_UpdateMode_IDs[] =
{
  IDS_COMPRESS_UPDATE_MODE_ADD,
  IDS_COMPRESS_UPDATE_MODE_UPDATE,
  IDS_COMPRESS_UPDATE_MODE_FRESH,
  IDS_COMPRESS_UPDATE_MODE_SYNC
};

static const
  // NWildcard::ECensorPathMode
  int
  k_PathMode_Vals[] =
{
  NWildcard::k_RelatPath,
  NWildcard::k_FullPath,
  NWildcard::k_AbsPath,
};

static const UInt32 k_PathMode_IDs[] =
{
  IDS_PATH_MODE_RELAT,
  IDS_EXTRACT_PATHS_FULL,
  IDS_EXTRACT_PATHS_ABS
};

// CCompressDialog dialog

IMPLEMENT_DYNAMIC(CCompressDialog, CDialogEx)

CCompressDialog::CCompressDialog(CWnd* pParent /*=NULL*/)
	: CDialogEx(CCompressDialog::IDD, pParent), CurrentDirWasChanged(false)
{
}

CCompressDialog::~CCompressDialog()
{
}

void CCompressDialog::DoDataExchange(CDataExchange* pDX)
{
  CDialogEx::DoDataExchange(pDX);
  DDX_Control(pDX, IDC_COMPRESS_ARCHIVE, m_cmbArchivePath);
  DDX_Control(pDX, IDC_COMPRESS_FORMAT, m_cmbFormat);
  DDX_Control(pDX, IDC_COMPRESS_LEVEL, m_cmbLevel);
  DDX_Control(pDX, IDC_COMPRESS_METHOD, m_cmbMethod);
  DDX_Control(pDX, IDC_COMPRESS_DICTIONARY, m_cmbDictionary);
  DDX_Control(pDX, IDC_COMPRESS_ORDER, m_cmbOrder);
  DDX_Control(pDX, IDC_COMPRESS_SOLID, m_cmbSolid);
  DDX_Control(pDX, IDC_COMPRESS_THREADS, m_cmbNumThreads);
  DDX_Control(pDX, IDC_COMPRESS_UPDATE_MODE, m_cmbUpdateMode);
  DDX_Control(pDX, IDC_COMPRESS_PATH_MODE, m_cmbPathMode);
  DDX_Control(pDX, IDC_COMPRESS_VOLUME, m_cmbVolume);
  DDX_Control(pDX, IDE_COMPRESS_PARAMETERS, m_edtParams);
  DDX_Control(pDX, IDE_COMPRESS_PASSWORD1, m_edtPassword1);
  DDX_Control(pDX, IDE_COMPRESS_PASSWORD2, m_edtPassword2);
  DDX_Control(pDX, IDC_COMPRESS_ENCRYPTION_METHOD, m_cmbEncryptionMethod);
}


void CCompressDialog::CheckButton_TwoBools(UINT id, const CBoolPair &b1, const CBoolPair &b2)
{
  CheckDlgButton(id, GetBoolsVal(b1, b2) ? BST_CHECKED : BST_UNCHECKED);
}

void CCompressDialog::GetButton_Bools(UINT id, CBoolPair &b1, CBoolPair &b2)
{
  bool val = (IsDlgButtonChecked(id) == BST_CHECKED);
  bool oldVal = GetBoolsVal(b1, b2);
  if (val != oldVal)
    b1.Def = b2.Def = true;
  b1.Val = b2.Val = val;
}

BEGIN_MESSAGE_MAP(CCompressDialog, CDialogEx)
  ON_CBN_SELCHANGE(IDC_COMPRESS_ARCHIVE, &CCompressDialog::OnCbnSelchangeCompressArchive)
  ON_CBN_SELCHANGE(IDC_COMPRESS_FORMAT, &CCompressDialog::OnCbnSelchangeCompressFormat)
  ON_CBN_SELCHANGE(IDC_COMPRESS_LEVEL, &CCompressDialog::OnCbnSelchangeCompressLevel)
  ON_CBN_SELCHANGE(IDC_COMPRESS_METHOD, &CCompressDialog::OnCbnSelchangeCompressMethod)
  ON_CBN_SELCHANGE(IDC_COMPRESS_DICTIONARY, &CCompressDialog::OnCbnSelchangeCompressDictionary)
  ON_CBN_SELCHANGE(IDC_COMPRESS_SOLID, &CCompressDialog::OnCbnSelchangeCompressSolid)
  ON_CBN_SELCHANGE(IDC_COMPRESS_THREADS, &CCompressDialog::OnCbnSelchangeCompressThreads)
  ON_BN_CLICKED(IDC_COMPRESS_SET_ARCHIVE, &CCompressDialog::OnBnClickedCompressSetArchive)
  ON_BN_CLICKED(IDX_COMPRESS_SFX, &CCompressDialog::OnBnClickedCompressSfx)
  ON_BN_CLICKED(IDX_PASSWORD_SHOW, &CCompressDialog::OnBnClickedPasswordShow)
  ON_BN_CLICKED(IDX_LOAD_CONFIG, &CCompressDialog::OnBnClickedLoadConfig)
END_MESSAGE_MAP()


// CCompressDialog message handlers


BOOL CCompressDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

#ifdef _DEBUG
  AString strIniPath;
  char result[MAX_PATH];
  GetCurrentDirectoryA(MAX_PATH, result);
  strIniPath = result;
  strIniPath.Add_PathSepar();
  strIniPath += kInifileName;
#else
  AString strIniPath = GetAnsiString(NWindows::NDLL::GetModuleDirPrefix()) + kInifileName;
#endif
  std::ifstream f(strIniPath);
  bConfigLoadable = f.good();
  f.close();

  #ifdef LANG
  LangSetWindowText(*this, IDD_COMPRESS);
  LangSetDlgItems(*this, kLangIDs, ARRAY_SIZE(kLangIDs));
  #endif

  AddVolumeItems(m_cmbVolume);

  m_RegistryInfo.Load();
  CheckDlgButton(IDX_PASSWORD_SHOW, m_RegistryInfo.ShowPassword ? BST_CHECKED : BST_UNCHECKED);
  CheckDlgButton(IDX_COMPRESS_ENCRYPT_FILE_NAMES, m_RegistryInfo.EncryptHeaders ? BST_CHECKED : BST_UNCHECKED);
  
  CheckButton_TwoBools(IDX_COMPRESS_NT_SYM_LINKS,   Info.SymLinks,   m_RegistryInfo.SymLinks);
  CheckButton_TwoBools(IDX_COMPRESS_NT_HARD_LINKS,  Info.HardLinks,  m_RegistryInfo.HardLinks);
  CheckButton_TwoBools(IDX_COMPRESS_NT_ALT_STREAMS, Info.AltStreams, m_RegistryInfo.AltStreams);
  CheckButton_TwoBools(IDX_COMPRESS_NT_SECUR,       Info.NtSecurity, m_RegistryInfo.NtSecurity);

  UpdatePasswordControl();

  {
    bool needSetMain = (Info.FormatIndex < 0);
    FOR_VECTOR(i, ArcIndices)
    {
      unsigned arcIndex = ArcIndices[i];
      const CArcInfoEx &ai = (*ArcFormats)[arcIndex];
      int index = (int)m_cmbFormat.AddString(GetSystemString(ai.Name));
      m_cmbFormat.SetItemData(index, arcIndex);
      if (!needSetMain)
      {
        if (Info.FormatIndex == (int)arcIndex)
          m_cmbFormat.SetCurSel(index);
        continue;
      }
      if (i == 0 || ai.Name.IsEqualTo_NoCase(m_RegistryInfo.ArcType))
      {
        m_cmbFormat.SetCurSel(index);
        Info.FormatIndex = arcIndex;
      }
    }
  }

  {
    UString fileName;
    SetArcPathFields(Info.ArcPath, fileName, true);
    StartDirPrefix = DirPrefix;
    SetArchiveName(fileName);
  }
  SetLevel();
  SetParams();
  
  for (unsigned i = 0; i < m_RegistryInfo.ArcPaths.Size() && i < kHistorySize; i++)
    m_cmbArchivePath.AddString(GetSystemString(m_RegistryInfo.ArcPaths[i]));

  AddComboItems(m_cmbUpdateMode, k_UpdateMode_IDs, ARRAY_SIZE(k_UpdateMode_IDs),
      k_UpdateMode_Vals, Info.UpdateMode);

  AddComboItems(m_cmbPathMode, k_PathMode_IDs, ARRAY_SIZE(k_PathMode_IDs),
      k_PathMode_Vals, Info.PathMode);

  SetSolidBlockSize();
  SetNumThreads();

  TCHAR s[40] = { TEXT('/'), TEXT(' '), 0 };
  ConvertUInt32ToString(NSystem::GetNumberOfProcessors(), s + 2);
  SetDlgItemText(IDT_COMPRESS_HARDWARE_THREADS, s);

  CheckDlgButton(IDX_COMPRESS_SFX, Info.SFXMode ? BST_CHECKED : BST_UNCHECKED);
  CheckDlgButton(IDX_COMPRESS_SHARED, Info.OpenShareForWrite ? BST_CHECKED : BST_UNCHECKED);
  CheckDlgButton(IDX_LOAD_CONFIG, Info.LoadConfig ? BST_CHECKED : BST_UNCHECKED);
  CheckDlgButton(IDX_COMPRESS_DEL, Info.DeleteAfterCompressing ? BST_CHECKED : BST_UNCHECKED);
  
  CheckControlsEnable();

  // OnButtonSFX();

  SetEncryptionMethod();
  SetMemoryUsage();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


bool CCompressDialog::IsShowPasswordChecked() const
{
  return (IsDlgButtonChecked(IDX_PASSWORD_SHOW) == BST_CHECKED);
}


void CCompressDialog::UpdatePasswordControl()
{
  bool showPassword = IsShowPasswordChecked();
  TCHAR c = showPassword ? (TCHAR)0: TEXT('*');
  m_edtPassword1.SetPasswordChar(c);
  m_edtPassword2.SetPasswordChar(c);
  m_edtPassword1.RedrawWindow();
  m_edtPassword2.RedrawWindow();

  ShowItem_Bool(IDT_PASSWORD_REENTER, !showPassword);
  m_edtPassword2.ShowWindow(showPassword ? SW_HIDE : SW_SHOW);
}


void CCompressDialog::OnBnClickedCompressSfx()
{
  SetMethod(GetMethodID());
  OnButtonSFX();
  SetMemoryUsage();
}


void CCompressDialog::OnBnClickedPasswordShow()
{
  UpdatePasswordControl();
}


void CCompressDialog::OnBnClickedLoadConfig()
{
  CheckCompressControlsEnable();
}

void CCompressDialog::CheckSFXControlsEnable()
{
  const CFormatInfo &fi = g_Formats[GetStaticFormatIndex()];
  bool enable = fi.SFX;
  if (enable)
  {
    int methodID = GetMethodID();
    enable = (methodID == -1 || IsMethodSupportedBySfx(methodID));
  }
  if (!enable)
    CheckDlgButton(IDX_COMPRESS_SFX, BST_UNCHECKED);
  EnableItem(IDX_COMPRESS_SFX, enable);
}

void CCompressDialog::CheckCFGControlsEnable()
{
  bool enable = true;
  if (!bConfigLoadable)
    enable = false;
  const CFormatInfo &fi = g_Formats[GetStaticFormatIndex()];
  if (!(fi.Name == k7zeFormat))
    enable = false;
//   if (!enable)
//     CheckButton(IDX_LOAD_CONFIG, false);
  EnableItem(IDX_LOAD_CONFIG, enable);
  CheckCompressControlsEnable();
}

/*
void CCompressDialog::CheckVolumeEnable()
{
  bool isSFX = IsSFX();
  m_Volume.Enable(!isSFX);
  if (isSFX)
    m_Volume.SetText(TEXT(""));
}
*/

void CCompressDialog::CheckControlsEnable()
{
  const CFormatInfo &fi = g_Formats[GetStaticFormatIndex()];
  Info.SolidIsSpecified = fi.Solid;
  bool multiThreadEnable = fi.MultiThread;
  Info.MultiThreadIsAllowed = multiThreadEnable;
  Info.EncryptHeadersIsAllowed = fi.EncryptFileNames;
  
  EnableItem(IDC_COMPRESS_SOLID, fi.Solid);
  EnableItem(IDC_COMPRESS_THREADS, multiThreadEnable);

  CheckSFXControlsEnable();

  {
    const CArcInfoEx &ai = (*ArcFormats)[GetFormatIndex()];
    
    ShowItem_Bool(IDX_COMPRESS_NT_SYM_LINKS, ai.Flags_SymLinks());
    ShowItem_Bool(IDX_COMPRESS_NT_HARD_LINKS, ai.Flags_HardLinks());
    ShowItem_Bool(IDX_COMPRESS_NT_ALT_STREAMS, ai.Flags_AltStreams());
    ShowItem_Bool(IDX_COMPRESS_NT_SECUR, ai.Flags_NtSecure());

    ShowItem_Bool(IDG_COMPRESS_NTFS,
           ai.Flags_SymLinks()
        || ai.Flags_HardLinks()
        || ai.Flags_AltStreams()
        || ai.Flags_NtSecure());
  }
  // CheckVolumeEnable();
  CheckCFGControlsEnable();

  EnableItem(IDG_COMPRESS_ENCRYPTION, fi.Encrypt);

  EnableItem(IDT_PASSWORD_ENTER, fi.Encrypt);
  EnableItem(IDT_PASSWORD_REENTER, fi.Encrypt);
  EnableItem(IDE_COMPRESS_PASSWORD1, fi.Encrypt);
  EnableItem(IDE_COMPRESS_PASSWORD2, fi.Encrypt);
  EnableItem(IDX_PASSWORD_SHOW, fi.Encrypt);

  EnableItem(IDT_COMPRESS_ENCRYPTION_METHOD, fi.Encrypt);
  EnableItem(IDC_COMPRESS_ENCRYPTION_METHOD, fi.Encrypt);
  EnableItem(IDX_COMPRESS_ENCRYPT_FILE_NAMES, fi.EncryptFileNames);

  ShowItem_Bool(IDX_COMPRESS_ENCRYPT_FILE_NAMES, fi.EncryptFileNames);
}

bool CCompressDialog::IsSFX()
{
  CWnd *sfxButton = GetDlgItem(IDX_COMPRESS_SFX);
  return sfxButton->IsWindowEnabled() && (IsDlgButtonChecked(IDX_COMPRESS_SFX) == BST_CHECKED);
}

static int GetExtDotPos(const UString &s)
{
  int dotPos = s.ReverseFind_Dot();
  if (dotPos > s.ReverseFind_PathSepar() + 1)
    return dotPos;
  return -1;
}

void CCompressDialog::OnButtonSFX()
{
  CString str;
  UString fileName;
  m_cmbArchivePath.GetWindowText(str);
  fileName = GetUnicodeString(str);
  int dotPos = GetExtDotPos(fileName);
  if (IsSFX())
  {
    if (dotPos >= 0)
      fileName.DeleteFrom(dotPos);
    fileName += kExeExt;
    m_cmbArchivePath.SetWindowText(GetSystemString(fileName));
  }
  else
  {
    if (dotPos >= 0)
    {
      UString ext = fileName.Ptr(dotPos);
      if (ext.IsEqualTo_NoCase(kExeExt))
      {
        fileName.DeleteFrom(dotPos);
        m_cmbArchivePath.SetWindowText(GetSystemString(fileName));
      }
    }
    SetArchiveName2(false); // it's for OnInit
  }

  // CheckVolumeEnable();
}

bool CCompressDialog::IsLoadConfig()
{
  CWnd *cfgButton = GetDlgItem(IDX_LOAD_CONFIG);
  return cfgButton->IsWindowEnabled() && (IsDlgButtonChecked(IDX_LOAD_CONFIG) == BST_CHECKED);
}

void CCompressDialog::CheckCompressControlsEnable()
{
  const CFormatInfo &fi = g_Formats[GetStaticFormatIndex()];
  bool bLoadConfig = IsLoadConfig();
  EnableItem(IDC_COMPRESS_SOLID, fi.Solid && !bLoadConfig);
  EnableItem(IDC_COMPRESS_THREADS, fi.MultiThread && !bLoadConfig);
  EnableItem(IDC_COMPRESS_METHOD, !bLoadConfig);
  EnableItem(IDC_COMPRESS_DICTIONARY, !bLoadConfig);
  EnableItem(IDC_COMPRESS_ORDER, !bLoadConfig);
}

bool CCompressDialog::GetFinalPath_Smart(UString &resPath)
{
  UString name;
  CString str;
  m_cmbArchivePath.GetWindowText(str);
  name = str;
  name.Trim();
  UString tempPath = name;
  if (!IsAbsolutePath(name))
  {
    UString newDirPrefix = DirPrefix;
    if (newDirPrefix.IsEmpty())
      newDirPrefix = StartDirPrefix;
    FString resultF;
    if (!MyGetFullPathName(us2fs(newDirPrefix + name), resultF))
      return false;
    tempPath = fs2us(resultF);
  }
  if (!SetArcPathFields(tempPath, name, false))
    return false;
  FString resultF;
  if (!MyGetFullPathName(us2fs(DirPrefix + name), resultF))
    return false;
  resPath = fs2us(resultF);
  return true;
}

bool CCompressDialog::SetArcPathFields(const UString &path, UString &name, bool always)
{
  FString resDirPrefix;
  FString resFileName;
  bool res = GetFullPathAndSplit(us2fs(path), resDirPrefix, resFileName);
  if (res)
  {
    DirPrefix = fs2us(resDirPrefix);
    name = fs2us(resFileName);
  }
  else
  {
    if (!always)
      return false;
    DirPrefix.Empty();
    name = path;
  }
  ::SetDlgItemTextW(*this, IDT_COMPRESS_ARCHIVE_FOLDER, DirPrefix);
  ::SetDlgItemTextW(*this, IDC_COMPRESS_ARCHIVE, name);
  return res;
}

static const wchar_t *k_IncorrectPathMessage = L"Incorrect archive path";


void CCompressDialog::OnBnClickedCompressSetArchive()
{
  UString path;
  if (!GetFinalPath_Smart(path))
  {
    ShowErrorMessage(*this, k_IncorrectPathMessage);
    return;
  }

  UString title = LangString(IDS_COMPRESS_SET_ARCHIVE_BROWSE);
  UString filterDescription = LangString(IDS_OPEN_TYPE_ALL_FILES);
  filterDescription += L" (*.*)";
  UString resPath;
  CurrentDirWasChanged = true;

  CString strPath = GetSystemString(path);
  CString strTitle = GetSystemString(title);
  CString strFilterDescription = GetSystemString(filterDescription);
  CString strResult;
  CFileDialog dlgFile(TRUE, NULL, strPath,
    OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, strFilterDescription, this);
  OPENFILENAME &ofn = dlgFile.GetOFN();
  ofn.lpstrTitle = strTitle;
  if (dlgFile.DoModal() == IDCANCEL)
    return;
  strResult = dlgFile.GetPathName();

  resPath = GetUnicodeString(strResult);

  UString dummyName;
  SetArcPathFields(resPath, dummyName, true);
}

void CCompressDialog::OnOK()
{
  CString str;
  m_edtPassword1.GetWindowText(str);
  Info.Password = GetUnicodeString(str);
  if (IsZipFormat())
  {
    if (!IsAsciiString(Info.Password))
    {
      ShowErrorMessageHwndRes(*this, IDS_PASSWORD_USE_ASCII);
      return;
    }
    UString method = GetEncryptionMethodSpec();
    if (method.IsPrefixedBy_Ascii_NoCase("aes"))
    {
      if (Info.Password.Len() > 99)
      {
        ShowErrorMessageHwndRes(*this, IDS_PASSWORD_TOO_LONG);
        return;
      }
    }
  }
  if (!IsShowPasswordChecked())
  {
    UString password2;
    m_edtPassword2.GetWindowText(str);
    password2 = GetUnicodeString(str);
    if (password2 != Info.Password)
    {
      ShowErrorMessageHwndRes(*this, IDS_PASSWORD_NOT_MATCH);
      return;
    }
  }

  SaveOptionsInMem();
  {
    UString s;
    if (!GetFinalPath_Smart(s))
    {
      ShowErrorMessage(*this, k_IncorrectPathMessage);
      return;
    }
    
    m_RegistryInfo.ArcPaths.Clear();
    AddUniqueString(m_RegistryInfo.ArcPaths, s);
    Info.ArcPath = s;
  }
  
  Info.UpdateMode = (NCompressDialog::NUpdateMode::EEnum)k_UpdateMode_Vals[m_cmbUpdateMode.GetCurSel()];;
  Info.PathMode = (NWildcard::ECensorPathMode)k_PathMode_Vals[m_cmbPathMode.GetCurSel()];

  Info.Level = GetLevelSpec();
  Info.Dictionary = GetDictionarySpec();
  Info.Order = GetOrderSpec();
  Info.OrderMode = GetOrderMode();
  Info.NumThreads = GetNumThreadsSpec();

  UInt32 solidLogSize = GetBlockSizeSpec();
  Info.SolidBlockSize = 0;
  if (solidLogSize > 0 && solidLogSize != (UInt32)(Int32)-1)
    Info.SolidBlockSize = (solidLogSize >= 64) ? (UInt64)(Int64)-1 : ((UInt64)1 << solidLogSize);

  Info.Method = GetMethodSpec();
  Info.EncryptionMethod = GetEncryptionMethodSpec();
  Info.FormatIndex = GetFormatIndex();
  Info.SFXMode = IsSFX();
  Info.OpenShareForWrite = (IsDlgButtonChecked(IDX_COMPRESS_SHARED) == BST_CHECKED);
  Info.DeleteAfterCompressing = (IsDlgButtonChecked(IDX_COMPRESS_DEL) == BST_CHECKED);
  Info.LoadConfig = IsLoadConfig();

  m_RegistryInfo.EncryptHeaders =
    Info.EncryptHeaders = (IsDlgButtonChecked(IDX_COMPRESS_ENCRYPT_FILE_NAMES) == BST_CHECKED);


  GetButton_Bools(IDX_COMPRESS_NT_SYM_LINKS,   Info.SymLinks,   m_RegistryInfo.SymLinks);
  GetButton_Bools(IDX_COMPRESS_NT_HARD_LINKS,  Info.HardLinks,  m_RegistryInfo.HardLinks);
  GetButton_Bools(IDX_COMPRESS_NT_ALT_STREAMS, Info.AltStreams, m_RegistryInfo.AltStreams);
  GetButton_Bools(IDX_COMPRESS_NT_SECUR,       Info.NtSecurity, m_RegistryInfo.NtSecurity);

  {
    const CArcInfoEx &ai = (*ArcFormats)[GetFormatIndex()];
    if (!ai.Flags_SymLinks()) Info.SymLinks.Val = false;
    if (!ai.Flags_HardLinks()) Info.HardLinks.Val = false;
    if (!ai.Flags_AltStreams()) Info.AltStreams.Val = false;
    if (!ai.Flags_NtSecure()) Info.NtSecurity.Val = false;
  }

  m_edtParams.GetWindowText(str);
  Info.Options = GetUnicodeString(str);
  
  UString volumeString;
  m_cmbVolume.GetWindowText(str);
  volumeString = GetUnicodeString(str);
  volumeString.Trim();
  Info.VolumeSizes.Clear();
  
  if (!volumeString.IsEmpty())
  {
    if (!ParseVolumeSizes(volumeString, Info.VolumeSizes))
    {
      ShowErrorMessageHwndRes(*this, IDS_INCORRECT_VOLUME_SIZE);
      return;
    }
    if (!Info.VolumeSizes.IsEmpty())
    {
      const UInt64 volumeSize = Info.VolumeSizes.Back();
      if (volumeSize < (100 << 10))
      {
        wchar_t s[32];
        ConvertUInt64ToString(volumeSize, s);
        if (::MessageBoxW(*this, MyFormatNew(IDS_SPLIT_CONFIRM, s),
            L"EcoZip", MB_YESNOCANCEL | MB_ICONQUESTION) != IDYES)
          return;
      }
    }
  }

  for (int i = 0; i < m_cmbArchivePath.GetCount(); i++)
  {
    CString sTemp;
    m_cmbArchivePath.GetLBText(i, sTemp);
    sTemp.Trim();
    AddUniqueString(m_RegistryInfo.ArcPaths, GetUnicodeString(sTemp));
  }
  
  if (m_RegistryInfo.ArcPaths.Size() > kHistorySize)
    m_RegistryInfo.ArcPaths.DeleteBack();
  
  if (Info.FormatIndex >= 0)
    m_RegistryInfo.ArcType = (*ArcFormats)[Info.FormatIndex].Name;
  m_RegistryInfo.ShowPassword = IsShowPasswordChecked();

  m_RegistryInfo.Save();
  
  CDialogEx::OnOK();
}

static LPCWSTR kHelpTopic = L"fme/plugins/EcoZip/add.htm";

void CCompressDialog::OnHelp()
{
  ShowHelpWindow(NULL, kHelpTopic);
  CDialogEx::OnHelp();
}


void CCompressDialog::OnCbnSelchangeCompressArchive()
{
  // we can 't change m_ArchivePath in that handler !
  DirPrefix.Empty();
  SetDlgItemText(IDT_COMPRESS_ARCHIVE_FOLDER, _T(""));

  /*
  UString path;
  m_ArchivePath.GetText(path);
  m_ArchivePath.SetText(L"");
  if (IsAbsolutePath(path))
  {
    UString fileName;
    SetArcPathFields(path, fileName);
    SetArchiveName(fileName);
  }
  */
}


void CCompressDialog::OnCbnSelchangeCompressFormat()
{
  bool isSFX = IsSFX();
  SaveOptionsInMem();
  SetLevel();
  SetSolidBlockSize();
  SetNumThreads();
  SetParams();
  CheckControlsEnable();
  SetArchiveName2(isSFX);
  SetEncryptionMethod();
  SetMemoryUsage();
}


void CCompressDialog::OnCbnSelchangeCompressLevel()
{
  const CArcInfoEx &ai = (*ArcFormats)[GetFormatIndex()];
  int index = FindRegistryFormatAlways(ai.Name);
  NCompression::CFormatOptions &fo = m_RegistryInfo.Formats[index];
  fo.ResetForLevelChange();
  SetMethod();
  SetSolidBlockSize();
  SetNumThreads();
  CheckSFXNameChange();
  SetMemoryUsage();
}


void CCompressDialog::OnCbnSelchangeCompressMethod()
{
  SetDictionary();
  SetOrder();
  SetSolidBlockSize();
  SetNumThreads();
  CheckSFXNameChange();
  SetMemoryUsage();
}


void CCompressDialog::OnCbnSelchangeCompressDictionary()
{
  SetSolidBlockSize();
  SetMemoryUsage();
}


void CCompressDialog::OnCbnSelchangeCompressSolid()
{
  SetSolidBlockSize();
  SetMemoryUsage();
}


void CCompressDialog::OnCbnSelchangeCompressThreads()
{
  SetMemoryUsage();
}

void CCompressDialog::CheckSFXNameChange()
{
  bool isSFX = IsSFX();
  CheckSFXControlsEnable();
  if (isSFX != IsSFX())
    SetArchiveName2(isSFX);
}

void CCompressDialog::SetArchiveName2(bool prevWasSFX)
{
  CString str;
  UString fileName;
  m_cmbArchivePath.GetWindowText(str);
  fileName = GetUnicodeString(str);
  const CArcInfoEx &prevArchiverInfo = (*ArcFormats)[m_PrevFormat];
  if (prevArchiverInfo.Flags_KeepName() || Info.KeepName)
  {
    UString prevExtension;
    if (prevWasSFX)
      prevExtension = kExeExt;
    else
      prevExtension = UString(L'.') + prevArchiverInfo.GetMainExt();
    const unsigned prevExtensionLen = prevExtension.Len();
    if (fileName.Len() >= prevExtensionLen)
      if (StringsAreEqualNoCase(fileName.RightPtr(prevExtensionLen), prevExtension))
        fileName.DeleteFrom(fileName.Len() - prevExtensionLen);
  }
  SetArchiveName(fileName);
}

// if type.KeepName then use OriginalFileName
// else if !KeepName remove extension
// add new extension

void CCompressDialog::SetArchiveName(const UString &name)
{
  UString fileName = name;
  Info.FormatIndex = GetFormatIndex();
  const CArcInfoEx &ai = (*ArcFormats)[Info.FormatIndex];
  m_PrevFormat = Info.FormatIndex;
  if (ai.Flags_KeepName())
  {
    fileName = OriginalFileName;
  }
  else
  {
    if (!Info.KeepName)
    {
      int dotPos = GetExtDotPos(fileName);
      if (dotPos >= 0)
        fileName.DeleteFrom(dotPos);
    }
  }

  if (IsSFX())
    fileName += kExeExt;
  else
  {
    fileName += L'.';
    fileName += ai.GetMainExt();
  }
  ::SetDlgItemTextW(*this, IDC_COMPRESS_ARCHIVE, fileName);
}

int CCompressDialog::FindRegistryFormat(const UString &name)
{
  FOR_VECTOR (i, m_RegistryInfo.Formats)
  {
    const NCompression::CFormatOptions &fo = m_RegistryInfo.Formats[i];
    if (name.IsEqualTo_NoCase(GetUnicodeString(fo.FormatID)))
      return i;
  }
  return -1;
}

int CCompressDialog::FindRegistryFormatAlways(const UString &name)
{
  int index = FindRegistryFormat(name);
  if (index < 0)
  {
    NCompression::CFormatOptions fo;
    fo.FormatID = GetSystemString(name);
    index = m_RegistryInfo.Formats.Add(fo);
  }
  return index;
}

int CCompressDialog::GetStaticFormatIndex()
{
  const CArcInfoEx &ai = (*ArcFormats)[GetFormatIndex()];
  for (unsigned i = 0; i < ARRAY_SIZE(g_Formats); i++)
    if (ai.Name.IsEqualTo_NoCase(g_Formats[i].Name))
      return i;
  return 0; // -1;
}

void CCompressDialog::SetNearestSelectComboBox(CComboBox &comboBox, UInt32 value)
{
  for (int i = comboBox.GetCount() - 1; i >= 0; i--)
    if ((UInt32)comboBox.GetItemData(i) <= value)
    {
      comboBox.SetCurSel(i);
      return;
    }
  if (comboBox.GetCount() > 0)
    comboBox.SetCurSel(0);
}

void CCompressDialog::SetLevel()
{
  m_cmbLevel.ResetContent();
  const CFormatInfo &fi = g_Formats[GetStaticFormatIndex()];
  const CArcInfoEx &ai = (*ArcFormats)[GetFormatIndex()];
  int index = FindRegistryFormat(ai.Name);
  UInt32 level = 5;
  {
    int index = FindRegistryFormat(ai.Name);
    if (index >= 0)
    {
      const NCompression::CFormatOptions &fo = m_RegistryInfo.Formats[index];
      if (fo.Level <= 9)
        level = fo.Level;
      else
        level = 9;
    }
  }
  
  for (unsigned i = 0; i <= 9; i++)
  {
    if ((fi.LevelsMask & (1 << i)) != 0)
    {
      UInt32 langID = g_Levels[i];
      int index = (int)m_cmbLevel.AddString(GetSystemString(LangString(langID)));
      m_cmbLevel.SetItemData(index, i);
    }
  }
  SetNearestSelectComboBox(m_cmbLevel, level);
  SetMethod();
}

void CCompressDialog::SetMethod(int keepMethodId)
{
  m_cmbMethod.ResetContent();
  UInt32 level = GetLevel();
  if (level == 0)
  {
    SetDictionary();
    SetOrder();
    return;
  }
  const CFormatInfo &fi = g_Formats[GetStaticFormatIndex()];
  const CArcInfoEx &ai = (*ArcFormats)[GetFormatIndex()];
  int index = FindRegistryFormat(ai.Name);
  UString defaultMethod;
  if (index >= 0)
  {
    const NCompression::CFormatOptions &fo = m_RegistryInfo.Formats[index];
    defaultMethod = fo.Method;
  }
  bool isSfx = IsSFX();
  bool weUseSameMethod = false;
  
  for (unsigned m = 0; m < fi.NumMethods; m++)
  {
    EMethodID methodID = fi.MathodIDs[m];
    if (isSfx)
      if (!IsMethodSupportedBySfx(methodID))
        continue;
    const LPCWSTR method = kMethodsNames[methodID];
    int itemIndex = (int)m_cmbMethod.AddString(GetSystemString(method));
    m_cmbMethod.SetItemData(itemIndex, methodID);
    if (keepMethodId == methodID)
    {
      m_cmbMethod.SetCurSel(itemIndex);
      weUseSameMethod = true;
      continue;
    }
    if ((defaultMethod.IsEqualTo_NoCase(method) || m == 0) && !weUseSameMethod)
      m_cmbMethod.SetCurSel(itemIndex);
  }
  
  if (!weUseSameMethod)
  {
    SetDictionary();
    SetOrder();
  }
}

bool CCompressDialog::IsZipFormat()
{
  const CArcInfoEx &ai = (*ArcFormats)[GetFormatIndex()];
  return ai.Name.IsEqualTo_Ascii_NoCase("zip");
}

void CCompressDialog::SetEncryptionMethod()
{
  m_cmbEncryptionMethod.ResetContent();
  const CArcInfoEx &ai = (*ArcFormats)[GetFormatIndex()];
  if (ai.Name.IsEqualTo_Ascii_NoCase("7z") || ai.Name.IsEqualTo_Ascii_NoCase("7ze"))
  {
    m_cmbEncryptionMethod.AddString(TEXT("AES-256"));
    m_cmbEncryptionMethod.SetCurSel(0);
  }
  else if (ai.Name.IsEqualTo_Ascii_NoCase("zip"))
  {
    int index = FindRegistryFormat(ai.Name);
    UString encryptionMethod;
    if (index >= 0)
    {
      const NCompression::CFormatOptions &fo = m_RegistryInfo.Formats[index];
      encryptionMethod = fo.EncryptionMethod;
    }
    m_cmbEncryptionMethod.AddString(TEXT("ZipCrypto"));
    m_cmbEncryptionMethod.AddString(TEXT("AES-256"));
    m_cmbEncryptionMethod.SetCurSel(encryptionMethod.IsPrefixedBy_Ascii_NoCase("aes") ? 1 : 0);
  }
}

int CCompressDialog::GetMethodID()
{
  if (m_cmbMethod.GetCount() <= 0)
    return -1;
  return (int)(UInt32)m_cmbMethod.GetItemData(m_cmbMethod.GetCurSel());
}

UString CCompressDialog::GetMethodSpec()
{
  if (m_cmbMethod.GetCount() <= 1)
    return UString();
  return kMethodsNames[GetMethodID()];
}

UString CCompressDialog::GetEncryptionMethodSpec()
{
  if (m_cmbEncryptionMethod.GetCount() <= 1)
    return UString();
  if (m_cmbEncryptionMethod.GetCurSel() <= 0)
    return UString();
  UString result;
  CString str;
  m_cmbEncryptionMethod.GetWindowText(str);
  result = str;
  result.RemoveChar(L'-');
  return result;
}

void CCompressDialog::AddDictionarySize(UInt32 size)
{
  Byte c = 0;
  unsigned moveBits = 0;
  if ((size & 0xFFFFF) == 0)    { moveBits = 20; c = 'M'; }
  else if ((size & 0x3FF) == 0) { moveBits = 10; c = 'K'; }
  TCHAR s[40];
  ConvertUInt32ToString(size >> moveBits, s);
  unsigned pos = MyStringLen(s);
  s[pos++] = ' ';
  if (moveBits != 0)
    s[pos++] = c;
  s[pos++] = 'B';
  s[pos++] = 0;
  int index = (int)m_cmbDictionary.AddString(s);
  m_cmbDictionary.SetItemData(index, size);
}

void CCompressDialog::SetDictionary()
{
  m_cmbDictionary.ResetContent();
  const CArcInfoEx &ai = (*ArcFormats)[GetFormatIndex()];
  int index = FindRegistryFormat(ai.Name);
  UInt32 defaultDict = (UInt32)(Int32)-1;
  
  if (index >= 0)
  {
    const NCompression::CFormatOptions &fo = m_RegistryInfo.Formats[index];
    if (fo.Method.IsEqualTo_NoCase(GetMethodSpec()))
      defaultDict = fo.Dictionary;
  }
  
  int methodID = GetMethodID();
  UInt32 level = GetLevel2();
  if (methodID < 0)
    return;
  UInt64 maxRamSize;
  bool maxRamSize_Defined = GetMaxRamSizeForProgram(maxRamSize);
  
  switch (methodID)
  {
    case kLZMA:
    case kLZMA2:
    {
      static const UInt32 kMinDicSize = (1 << 16);
      if (defaultDict == (UInt32)(Int32)-1)
      {
             if (level >= 9) defaultDict = (1 << 26);
        else if (level >= 7) defaultDict = (1 << 25);
        else if (level >= 5) defaultDict = (1 << 24);
        else if (level >= 3) defaultDict = (1 << 20);
        else                 defaultDict = (kMinDicSize);
      }
      
      AddDictionarySize(kMinDicSize);
      m_cmbDictionary.SetCurSel(0);
      
      for (unsigned i = 20; i <= 31; i++)
        for (unsigned j = 0; j < 2; j++)
        {
          if (i == 20 && j > 0)
            continue;
          UInt32 dict = ((UInt32)(2 + j) << (i - 1));
          
          if (dict >
            #ifdef MY_CPU_64BIT
              (3 << 29)
            #else
              (1 << 26)
            #endif
            )
            continue;
          
          AddDictionarySize(dict);
          UInt64 decomprSize;
          UInt64 requiredComprSize = GetMemoryUsage(dict, decomprSize);
          if (dict <= defaultDict && (!maxRamSize_Defined || requiredComprSize <= maxRamSize))
            m_cmbDictionary.SetCurSel(m_cmbDictionary.GetCount() - 1);
        }

      // SetNearestSelectComboBox(m_Dictionary, defaultDict);
      break;
    }
    
    case kPPMd:
    {
      if (defaultDict == (UInt32)(Int32)-1)
      {
             if (level >= 9) defaultDict = (192 << 20);
        else if (level >= 7) defaultDict = ( 64 << 20);
        else if (level >= 5) defaultDict = ( 16 << 20);
        else                 defaultDict = (  4 << 20);
      }

      for (unsigned i = 20; i < 31; i++)
        for (unsigned j = 0; j < 2; j++)
        {
          if (i == 20 && j > 0)
            continue;
          UInt32 dict = ((UInt32)(2 + j) << (i - 1));
          if (dict >
            #ifdef MY_CPU_64BIT
              (1 << 30)
            #else
              (1 << 29)
            #endif
            )
            continue;
          AddDictionarySize(dict);
          UInt64 decomprSize;
          UInt64 requiredComprSize = GetMemoryUsage(dict, decomprSize);
          if ((dict <= defaultDict && (!maxRamSize_Defined || requiredComprSize <= maxRamSize))
              || m_cmbDictionary.GetCount() == 1)
            m_cmbDictionary.SetCurSel(m_cmbDictionary.GetCount() - 1);
        }
      
      // SetNearestSelectComboBox(m_Dictionary, defaultDict);
      break;
    }

    case kDeflate:
    {
      AddDictionarySize(32 << 10);
      m_cmbDictionary.SetCurSel(0);
      break;
    }
    
    case kDeflate64:
    {
      AddDictionarySize(64 << 10);
      m_cmbDictionary.SetCurSel(0);
      break;
    }
    
    case kBZip2:
    {
      if (defaultDict == (UInt32)(Int32)-1)
      {
             if (level >= 5) defaultDict = (900 << 10);
        else if (level >= 3) defaultDict = (500 << 10);
        else                 defaultDict = (100 << 10);
      }
      
      for (unsigned i = 1; i <= 9; i++)
      {
        UInt32 dict = ((UInt32)i * 100) << 10;
        AddDictionarySize(dict);
        if (dict <= defaultDict || m_cmbDictionary.GetCount() == 0)
          m_cmbDictionary.SetCurSel(m_cmbDictionary.GetCount() - 1);
      }
      
      break;
    }
    
    case kPPMdZip:
    {
      if (defaultDict == (UInt32)(Int32)-1)
        defaultDict = (1 << (19 + (level > 8 ? 8 : level)));
      
      for (unsigned i = 20; i <= 28; i++)
      {
        UInt32 dict = (1 << i);
        AddDictionarySize(dict);
        UInt64 decomprSize;
        UInt64 requiredComprSize = GetMemoryUsage(dict, decomprSize);
        if ((dict <= defaultDict && (!maxRamSize_Defined || requiredComprSize <= maxRamSize))
            || m_cmbDictionary.GetCount() == 1)
          m_cmbDictionary.SetCurSel(m_cmbDictionary.GetCount() - 1);
      }
      SetNearestSelectComboBox(m_cmbDictionary, defaultDict);
      break;
    }
    case kLIBBSC:
    {
      static const UInt32 kMinDicSize = (1 << 20);
      if (defaultDict == (UInt32)(Int32)-1)
        defaultDict = (BSC_BLOCK_SIZE);
      int i;
      AddDictionarySize(kMinDicSize);
      m_cmbDictionary.SetCurSel(0);
      for (i = 21; i <= 30; i++)
        for (int j = 0; j < 2; j++)
        {
          if (i == 20 && j > 0)
            continue;
          UInt32 dict = (1 << i) + (j << (i - 1));
          if (dict >
          #ifdef MY_CPU_64BIT
            (1 << 30)
          #else
            (1 << 26)
          #endif
            )
            continue;
          AddDictionarySize(dict);
          UInt64 decomprSize;
          UInt64 requiredComprSize = GetMemoryUsage(dict, decomprSize);
          if (dict <= defaultDict && (!maxRamSize_Defined || requiredComprSize <= maxRamSize))
            m_cmbDictionary.SetCurSel(m_cmbDictionary.GetCount() - 1);
        }

      // SetNearestSelectComboBox(m_Dictionary, defaultDict);
      break;
    }
    case kCSC32:
    {
      static const UInt32 kMinDicSize = (1 << 20);
      if (defaultDict == (UInt32)(Int32)-1)
        defaultDict = (BSC_BLOCK_SIZE);
      int i;
      AddDictionarySize(kMinDicSize);
      m_cmbDictionary.SetCurSel(0);
      for (i = 21; i <= 29; i++)
        for (int j = 0; j < 2; j++)
        {
          if (i == 20 && j > 0)
            continue;
          UInt32 dict = (1 << i) + (j << (i - 1));
          if (dict >
          #ifdef MY_CPU_64BIT
            (1 << 29)
          #else
            (1 << 26)
          #endif
            )
            continue;
          AddDictionarySize(dict);
          UInt64 decomprSize;
          UInt64 requiredComprSize = GetMemoryUsage(dict, decomprSize);
          if (dict <= defaultDict && (!maxRamSize_Defined || requiredComprSize <= maxRamSize))
            m_cmbDictionary.SetCurSel(m_cmbDictionary.GetCount() - 1);
        }

      // SetNearestSelectComboBox(m_Dictionary, defaultDict);
      break;
    }
  }
}

UInt32 CCompressDialog::GetComboValue(CComboBox &c, int defMax)
{
  if (c.GetCount() <= defMax)
    return (UInt32)(Int32)-1;
  return (UInt32)c.GetItemData(c.GetCurSel());
}

UInt32 CCompressDialog::GetLevel2()
{
  UInt32 level = GetLevel();
  if (level == (UInt32)(Int32)-1)
    level = 5;
  return level;
}

int CCompressDialog::AddOrder(UInt32 size)
{
  TCHAR s[40];
  ConvertUInt32ToString(size, s);
  int index = (int)m_cmbOrder.AddString(s);
  m_cmbOrder.SetItemData(index, size);
  return index;
}

void CCompressDialog::SetOrder()
{
  m_cmbOrder.ResetContent();
  const CArcInfoEx &ai = (*ArcFormats)[GetFormatIndex()];
  int index = FindRegistryFormat(ai.Name);
  UInt32 defaultOrder = (UInt32)(Int32)-1;
  
  if (index >= 0)
  {
    const NCompression::CFormatOptions &fo = m_RegistryInfo.Formats[index];
    if (fo.Method.IsEqualTo_NoCase(GetMethodSpec()))
      defaultOrder = fo.Order;
  }

  int methodID = GetMethodID();
  UInt32 level = GetLevel2();
  if (methodID < 0)
    return;
  
  switch (methodID)
  {
    case kLZMA:
    case kLZMA2:
    {
      if (defaultOrder == (UInt32)(Int32)-1)
        defaultOrder = (level >= 7) ? 64 : 32;
      for (unsigned i = 3; i <= 8; i++)
        for (unsigned j = 0; j < 2; j++)
        {
          UInt32 order = ((UInt32)(2 + j) << (i - 1));
          if (order <= 256)
            AddOrder(order);
        }
      AddOrder(273);
      SetNearestSelectComboBox(m_cmbOrder, defaultOrder);
      break;
    }
    
    case kPPMd:
    {
      if (defaultOrder == (UInt32)(Int32)-1)
      {
             if (level >= 9) defaultOrder = 32;
        else if (level >= 7) defaultOrder = 16;
        else if (level >= 5) defaultOrder = 6;
        else                 defaultOrder = 4;
      }
      
      AddOrder(2);
      AddOrder(3);
      
      for (unsigned i = 2; i < 8; i++)
        for (unsigned j = 0; j < 4; j++)
        {
          UInt32 order = (4 + j) << (i - 2);
          if (order < 32)
            AddOrder(order);
        }
      
      AddOrder(32);
      SetNearestSelectComboBox(m_cmbOrder, defaultOrder);
      break;
    }
    
    case kDeflate:
    case kDeflate64:
    {
      if (defaultOrder == (UInt32)(Int32)-1)
      {
             if (level >= 9) defaultOrder = 128;
        else if (level >= 7) defaultOrder = 64;
        else                 defaultOrder = 32;
      }
      
      for (unsigned i = 3; i <= 8; i++)
        for (unsigned j = 0; j < 2; j++)
        {
          UInt32 order = ((UInt32)(2 + j) << (i - 1));;
          if (order <= 256)
            AddOrder(order);
        }
      
      AddOrder(methodID == kDeflate64 ? 257 : 258);
      SetNearestSelectComboBox(m_cmbOrder, defaultOrder);
      break;
    }
    
    case kBZip2:
      break;
    
    case kPPMdZip:
    {
      if (defaultOrder == (UInt32)(Int32)-1)
        defaultOrder = level + 3;
      for (unsigned i = 2; i <= 16; i++)
        AddOrder(i);
      SetNearestSelectComboBox(m_cmbOrder, defaultOrder);
      break;
    }
  }
}

bool CCompressDialog::GetOrderMode()
{
  switch (GetMethodID())
  {
    case kPPMd:
    case kPPMdZip:
      return true;
  }
  return false;
}

static const UInt32 kNoSolidBlockSize = 0;
static const UInt32 kSolidBlockSize = 64;

void CCompressDialog::SetSolidBlockSize()
{
  m_cmbSolid.ResetContent();
  const CFormatInfo &fi = g_Formats[GetStaticFormatIndex()];
  if (!fi.Solid)
    return;

  UInt32 level = GetLevel2();
  if (level == 0)
    return;

  UInt32 dict = GetDictionarySpec();
  if (dict == (UInt32)(Int32)-1)
    dict = 1;

  UInt32 defaultBlockSize = (UInt32)(Int32)-1;

  const CArcInfoEx &ai = (*ArcFormats)[GetFormatIndex()];
  {
    int index = FindRegistryFormat(ai.Name);
    if (index >= 0)
    {
      const NCompression::CFormatOptions &fo = m_RegistryInfo.Formats[index];
      if (fo.Method.IsEqualTo_NoCase(GetMethodSpec()))
        defaultBlockSize = fo.BlockLogSize;
    }
  }

  {
    int index = (int)m_cmbSolid.AddString(LangString(IDS_COMPRESS_NON_SOLID));
    m_cmbSolid.SetItemData(index, (UInt32)kNoSolidBlockSize);
    m_cmbSolid.SetCurSel(0);
  }
  
  bool needSet = (defaultBlockSize == (UInt32)(Int32)-1);
  
  for (unsigned i = 20; i <= 36; i++)
  {
    if (needSet && dict >= (((UInt64)1 << (i - 7))) && i <= 32)
      defaultBlockSize = i;
    TCHAR s[40];
    ConvertUInt32ToString(1 << (i % 10), s);
    if (i < 30) lstrcat(s, TEXT(" M"));
    else        lstrcat(s, TEXT(" G"));
    lstrcat(s, TEXT("B"));
    int index = (int)m_cmbSolid.AddString(s);
    m_cmbSolid.SetItemData(index, (UInt32)i);
  }
  
  {
    int index = (int)m_cmbSolid.AddString(LangString(IDS_COMPRESS_SOLID));
    m_cmbSolid.SetItemData(index, kSolidBlockSize);
  }
  
  if (defaultBlockSize == (UInt32)(Int32)-1)
    defaultBlockSize = kSolidBlockSize;
  if (defaultBlockSize != kNoSolidBlockSize)
    SetNearestSelectComboBox(m_cmbSolid, defaultBlockSize);
}

void CCompressDialog::SetNumThreads()
{
  m_cmbNumThreads.ResetContent();

  const CFormatInfo &fi = g_Formats[GetStaticFormatIndex()];
  if (!fi.MultiThread)
    return;

  UInt32 numHardwareThreads = NSystem::GetNumberOfProcessors();
  UInt32 defaultValue = numHardwareThreads;

  {
    const CArcInfoEx &ai = (*ArcFormats)[GetFormatIndex()];
    int index = FindRegistryFormat(ai.Name);
    if (index >= 0)
    {
      const NCompression::CFormatOptions &fo = m_RegistryInfo.Formats[index];
      if (fo.Method.IsEqualTo_NoCase(GetMethodSpec()) && fo.NumThreads != (UInt32)(Int32)-1)
        defaultValue = fo.NumThreads;
    }
  }

  UInt32 numAlgoThreadsMax = 1;
  int methodID = GetMethodID();
  switch (methodID)
  {
    case kLZMA: numAlgoThreadsMax = 2; break;
    case kLZMA2: numAlgoThreadsMax = 32; break;
    case kBZip2: numAlgoThreadsMax = 32; break;
    case kLIBBSC: numAlgoThreadsMax = 4; break;
  }
  if (IsZipFormat())
    numAlgoThreadsMax = 128;
  for (UInt32 i = 1; i <= numHardwareThreads * 2 && i <= numAlgoThreadsMax; i++)
  {
    TCHAR s[40];
    ConvertUInt32ToString(i, s);
    int index = (int)m_cmbNumThreads.AddString(s);
    m_cmbNumThreads.SetItemData(index, (UInt32)i);
  }
  SetNearestSelectComboBox(m_cmbNumThreads, defaultValue);
}

UInt64 CCompressDialog::GetMemoryUsage(UInt32 dict, UInt64 &decompressMemory)
{
  decompressMemory = UInt64(Int64(-1));
  UInt32 level = GetLevel2();
  if (level == 0)
  {
    decompressMemory = (1 << 20);
    return decompressMemory;
  }
  UInt64 size = 0;

  const CFormatInfo &fi = g_Formats[GetStaticFormatIndex()];
  if (fi.Filter && level >= 9)
    size += (12 << 20) * 2 + (5 << 20);
  UInt32 numThreads = GetNumThreads2();
  
  if (IsZipFormat())
  {
    UInt32 numSubThreads = 1;
    if (GetMethodID() == kLZMA && numThreads > 1 && level >= 5)
      numSubThreads = 2;
    UInt32 numMainThreads = numThreads / numSubThreads;
    if (numMainThreads > 1)
      size += (UInt64)numMainThreads << 25;
  }
  
  int methidId = GetMethodID();
  
  switch (methidId)
  {
    case kLZMA:
    case kLZMA2:
    {
      UInt32 hs = dict - 1;
      hs |= (hs >> 1);
      hs |= (hs >> 2);
      hs |= (hs >> 4);
      hs |= (hs >> 8);
      hs >>= 1;
      hs |= 0xFFFF;
      if (hs > (1 << 24))
        hs >>= 1;
      hs++;
      UInt64 size1 = (UInt64)hs * 4;
      size1 += (UInt64)dict * 4;
      if (level >= 5)
        size1 += (UInt64)dict * 4;
      size1 += (2 << 20);

      UInt32 numThreads1 = 1;
      if (numThreads > 1 && level >= 5)
      {
        size1 += (2 << 20) + (4 << 20);
        numThreads1 = 2;
      }
      
      UInt32 numBlockThreads = numThreads / numThreads1;
    
      if (methidId == kLZMA || numBlockThreads == 1)
        size1 += (UInt64)dict * 3 / 2;
      else
      {
        UInt64 chunkSize = (UInt64)dict << 2;
        chunkSize = MyMax(chunkSize, (UInt64)(1 << 20));
        chunkSize = MyMin(chunkSize, (UInt64)(1 << 28));
        chunkSize = MyMax(chunkSize, (UInt64)dict);
        size1 += chunkSize * 2;
      }
      size += size1 * numBlockThreads;

      decompressMemory = dict + (2 << 20);
      return size;
    }
  
    case kPPMd:
    {
      decompressMemory = dict + (2 << 20);
      return size + decompressMemory;
    }
    
    case kDeflate:
    case kDeflate64:
    {
      UInt32 order = GetOrder();
      if (order == (UInt32)(Int32)-1)
        order = 32;
      if (level >= 7)
        size += (1 << 20);
      size += 3 << 20;
      decompressMemory = (2 << 20);
      return size;
    }
    
    case kBZip2:
    {
      decompressMemory = (7 << 20);
      UInt64 memForOneThread = (10 << 20);
      return size + memForOneThread * numThreads;
    }
    
    case kPPMdZip:
    {
      decompressMemory = dict + (2 << 20);
      return size + (UInt64)decompressMemory * numThreads;
    }

    case kLIBBSC:
    {
      decompressMemory = (16 << 20) + 5 * dict * numThreads;
      return (UInt64)decompressMemory;
    }

    case kCSC32:
    {
      decompressMemory = (3 << 20) + dict;
      return (UInt64)decompressMemory;
    }
  }
  
  return (UInt64)(Int64)-1;
}

UInt64 CCompressDialog::GetMemoryUsage(UInt64 &decompressMemory)
{
  return GetMemoryUsage(GetDictionary(), decompressMemory);
}

void CCompressDialog::PrintMemUsage(UINT res, UInt64 value)
{
  if (value == (UInt64)(Int64)-1)
  {
    SetDlgItemText(res, TEXT("?"));
    return;
  }
  value = (value + (1 << 20) - 1) >> 20;
  TCHAR s[40];
  ConvertUInt64ToString(value, s);
  lstrcat(s, TEXT(" MB"));
  SetDlgItemText(res, s);
}
    
void CCompressDialog::SetMemoryUsage()
{
  UInt64 decompressMem;
  UInt64 memUsage = GetMemoryUsage(decompressMem);
  PrintMemUsage(IDT_COMPRESS_MEMORY_VALUE, memUsage);
  PrintMemUsage(IDT_COMPRESS_MEMORY_DE_VALUE, decompressMem);
}

void CCompressDialog::SetParams()
{
  const CArcInfoEx &ai = (*ArcFormats)[GetFormatIndex()];
  m_edtParams.SetWindowText(TEXT(""));
  int index = FindRegistryFormat(ai.Name);
  if (index >= 0)
  {
    const NCompression::CFormatOptions &fo = m_RegistryInfo.Formats[index];
    m_edtParams.SetWindowText(GetSystemString(fo.Options));
  }
}

void CCompressDialog::SaveOptionsInMem()
{
  const CArcInfoEx &ai = (*ArcFormats)[Info.FormatIndex];
  int index = FindRegistryFormatAlways(ai.Name);
  CString str;
  m_edtParams.GetWindowText(str);
  Info.Options = str;
  Info.Options.Trim();
  NCompression::CFormatOptions &fo = m_RegistryInfo.Formats[index];
  fo.Options = Info.Options;
  fo.Level = GetLevelSpec();
  fo.Dictionary = GetDictionarySpec();
  fo.Order = GetOrderSpec();
  fo.Method = GetMethodSpec();
  fo.EncryptionMethod = GetEncryptionMethodSpec();
  fo.NumThreads = GetNumThreadsSpec();
  fo.BlockLogSize = GetBlockSizeSpec();
}

unsigned CCompressDialog::GetFormatIndex()
{
  return (unsigned)m_cmbFormat.GetItemData(m_cmbFormat.GetCurSel());
}
