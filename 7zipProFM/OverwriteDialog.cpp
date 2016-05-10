// OverwriteDialog.cpp : implementation file
//

#include "stdafx.h"

#include "CPP/Common/StringConvert.h"

#include "CPP/Windows/PropVariantConv.h"

#include "CPP/7zip/UI/FileManager/FormatUtils.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "OverwriteDialog.h"

#include "FMUtils.h"

#ifdef LANG
static const UInt32 kLangIDs[] =
{
  IDT_OVERWRITE_HEADER,
  IDT_OVERWRITE_QUESTION_BEGIN,
  IDT_OVERWRITE_QUESTION_END,
  IDC_YES_TO_ALL,
  IDC_NO_TO_ALL,
  IDC_AUTO_RENAME
};
#endif

static const unsigned kCurrentFileNameSizeLimit = 82;
static const unsigned kCurrentFileNameSizeLimit2 = 30;

// COverwriteDialog dialog

IMPLEMENT_DYNAMIC(COverwriteDialog, CDialogEx)

COverwriteDialog::COverwriteDialog(CWnd* pParent /*=NULL*/)
: CDialogEx(COverwriteDialog::IDD, pParent)
{
}

COverwriteDialog::~COverwriteDialog()
{
}

void COverwriteDialog::DoDataExchange(CDataExchange* pDX)
{
  CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(COverwriteDialog, CDialogEx)
  ON_COMMAND_EX(IDYES, &COverwriteDialog::OnCommandRange)
  ON_COMMAND_EX(IDC_YES_TO_ALL, &COverwriteDialog::OnCommandRange)
  ON_COMMAND_EX(IDC_AUTO_RENAME, &COverwriteDialog::OnCommandRange)
  ON_COMMAND_EX(IDNO, &COverwriteDialog::OnCommandRange)
  ON_COMMAND_EX(IDC_NO_TO_ALL, &COverwriteDialog::OnCommandRange)
  ON_COMMAND_EX(IDCANCEL, &COverwriteDialog::OnCommandRange)
END_MESSAGE_MAP()


// COverwriteDialog message handlers


void COverwriteDialog::ReduceString(UString &s)
{
  unsigned size = _isBig ? kCurrentFileNameSizeLimit : kCurrentFileNameSizeLimit2;
  if (s.Len() > size)
  {
    s.Delete(size / 2, s.Len() - size);
    s.Insert(size / 2, L" ... ");
  }
}

void COverwriteDialog::SetFileInfoControl(int textID, int iconID,
    const NOverwriteDialog::CFileInfo &fileInfo)
{
  UString sizeString;
  if (fileInfo.SizeIsDefined)
    sizeString = MyFormatNew(IDS_FILE_SIZE, NumberToString(fileInfo.Size));

  const UString &fileName = fileInfo.Name;
  int slashPos = fileName.ReverseFind_PathSepar();
  UString s1 = fileName.Left(slashPos + 1);
  UString s2 = fileName.Ptr(slashPos + 1);

  ReduceString(s1);
  ReduceString(s2);

  UString s = s1;
  s.Add_LF();
  s += s2;
  s.Add_LF();
  s += sizeString;
  s.Add_LF();

  if (fileInfo.TimeIsDefined)
  {
    FILETIME localFileTime;
    if (!FileTimeToLocalFileTime(&fileInfo.Time, &localFileTime))
      throw 4190402;
    AddLangString(s, IDS_PROP_MTIME);
    s += L": ";
    wchar_t t[32];
    ConvertFileTimeToString(localFileTime, t);
    s += t;
  }

  ::SetDlgItemTextW(m_hWnd, textID, s);

  SHFILEINFO shellFileInfo;
  if (::SHGetFileInfo(
      GetSystemString(fileInfo.Name), FILE_ATTRIBUTE_NORMAL, &shellFileInfo,
      sizeof(shellFileInfo), SHGFI_ICON | SHGFI_USEFILEATTRIBUTES | SHGFI_LARGEICON))
  {
    CStatic staticContol;
    staticContol.Attach(*GetDlgItem(iconID));
    staticContol.SetIcon(shellFileInfo.hIcon);
    staticContol.Detach();
  }
}


BOOL COverwriteDialog::OnInitDialog()
{
  CDialogEx::OnInitDialog();

  m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

  // Set the icon for this dialog.  The framework does this automatically
  //  when the application's main window is not a dialog
  SetIcon(m_hIcon, TRUE);			// Set big icon
  SetIcon(m_hIcon, FALSE);		// Set small icon

  #ifdef LANG
  LangSetWindowText(*this, IDD_OVERWRITE);
  LangSetDlgItems(*this, kLangIDs, ARRAY_SIZE(kLangIDs));
  #endif
  SetFileInfoControl(IDT_OVERWRITE_OLD_FILE_SIZE_TIME, IDI_OVERWRITE_OLD_FILE, OldFileInfo);
  SetFileInfoControl(IDT_OVERWRITE_NEW_FILE_SIZE_TIME, IDI_OVERWRITE_NEW_FILE, NewFileInfo);

  return TRUE;  // return TRUE unless you set the focus to a control
  // EXCEPTION: OCX Property Pages should return FALSE
}


BOOL COverwriteDialog::OnCommandRange(UINT nID)
{
  switch (nID)
  {
    case IDYES:
    case IDC_YES_TO_ALL:
    case IDC_AUTO_RENAME:
    case IDNO:
    case IDC_NO_TO_ALL:
      EndDialog(nID);
      return TRUE;
    default:
      break;
  }
  return FALSE;
}
