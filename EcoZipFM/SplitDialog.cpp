// SplitDialog.cpp : implementation file
//

#include "stdafx.h"

#include "CPP/Common/StringConvert.h"

#include "CPP/Windows/FileName.h"
#include "CPP/Windows/Shell.h"

#ifdef LANG
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#endif

#include "SplitUtils.h"

#include "SplitDialog.h"

#ifdef LANG
static const UInt32 kLangIDs[] =
{
  IDT_SPLIT_PATH,
  IDT_SPLIT_VOLUME
};
#endif

// CSplitDialog dialog

IMPLEMENT_DYNAMIC(CSplitDialog, CDialogEx)

CSplitDialog::CSplitDialog(CWnd* pParent /*=NULL*/)
: CDialogEx(CSplitDialog::IDD, pParent)
{

}

CSplitDialog::~CSplitDialog()
{
}

void CSplitDialog::DoDataExchange(CDataExchange* pDX)
{
  CDialogEx::DoDataExchange(pDX);
  DDX_Control(pDX, IDC_SPLIT_PATH, m_cmbPath);
  DDX_Control(pDX, IDC_SPLIT_VOLUME, m_cmbVolume);
}


BEGIN_MESSAGE_MAP(CSplitDialog, CDialogEx)
  ON_BN_CLICKED(IDC_SPLIT_SET_PATH, &CSplitDialog::OnBnClickedSplitSetPath)
END_MESSAGE_MAP()


// CSplitDialog message handlers


BOOL CSplitDialog::OnInitDialog()
{
  CDialogEx::OnInitDialog();

#ifdef LANG
  LangSetWindowText(*this, IDD_SPLIT);
  LangSetDlgItems(*this, kLangIDs, ARRAY_SIZE(kLangIDs));
#endif

  if (!FilePath.IsEmpty())
  {
    CString str;
    UString title;
    GetWindowText(str);
    title = str;
    title.Add_Space();
    title += FilePath;
    ::SetWindowTextW(m_hWnd, title);
  }
  ::SetDlgItemTextW(m_hWnd, IDC_SPLIT_PATH, Path);
  AddVolumeItems(m_cmbVolume);
  m_cmbVolume.SetCurSel(0);

  return TRUE;  // return TRUE unless you set the focus to a control
  // EXCEPTION: OCX Property Pages should return FALSE
}


void CSplitDialog::OnOK()
{
  CString str;
  m_cmbPath.GetWindowText(str);
  Path = GetUnicodeString(str);
  UString volumeString;
  m_cmbVolume.GetWindowText(str);
  volumeString = GetUnicodeString(str);
  volumeString.Trim();
  if (!ParseVolumeSizes(volumeString, VolumeSizes) || VolumeSizes.Size() == 0)
  {
    ::MessageBoxW(*this, LangString(IDS_INCORRECT_VOLUME_SIZE), L"EcoZip", MB_ICONERROR);
    return;
  }

  CDialogEx::OnOK();
}


void CSplitDialog::OnBnClickedSplitSetPath()
{
  CString str;
  UString path;
  m_cmbPath.GetWindowText(str);
  path = GetUnicodeString(str);
  UString title;// = LangString(IDS_SET_FOLDER);
  UString resultPath;

#ifndef UNDER_CE
  if (!NWindows::NShell::BrowseForFolder(m_hWnd, title, path, resultPath))
    return;
#endif

  m_cmbPath.SetCurSel(-1);
  ::SetWindowTextW(m_cmbPath, resultPath);
}
