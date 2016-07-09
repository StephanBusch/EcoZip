// SettingsPage.cpp : implementation file
//

#include "stdafx.h"

#ifndef UNDER_CE
#include "CPP/Windows/MemoryLock.h"
#endif

#include "CPP/7zip/UI/FileManager/HelpUtils.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"

#include "EcoZipFM.h"
#include "SettingsPage.h"
#include "FMUtils.h"


static const UInt32 kLangIDs[] =
{
  IDX_SETTINGS_SHOW_DOTS,
  IDX_SETTINGS_SHOW_REAL_FILE_ICONS,
  IDX_SETTINGS_SHOW_SYSTEM_MENU,
  IDX_SETTINGS_FULL_ROW,
  IDX_SETTINGS_SHOW_GRID,
  IDX_SETTINGS_SINGLE_CLICK,
  IDX_SETTINGS_ALTERNATIVE_SELECTION,
  IDX_SETTINGS_LARGE_PAGES
};

static LPCWSTR kEditTopic = L"FME/options.htm#settings";

// CSettingsPage dialog

IMPLEMENT_DYNAMIC(CSettingsPage, CPropertyPage)

CSettingsPage::CSettingsPage()
: CPropertyPage(CSettingsPage::IDD)
{

}

CSettingsPage::~CSettingsPage()
{
}

void CSettingsPage::DoDataExchange(CDataExchange* pDX)
{
  CPropertyPage::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSettingsPage, CPropertyPage)
  ON_COMMAND_RANGE(IDX_SETTINGS_SHOW_DOTS, IDX_SETTINGS_LARGE_PAGES, &CSettingsPage::OnCommandRange)
END_MESSAGE_MAP()


// CSettingsPage message handlers


BOOL CSettingsPage::OnInitDialog()
{
  CPropertyPage::OnInitDialog();

  LangSetDlgItems(*this, kLangIDs, ARRAY_SIZE(kLangIDs));

  CFmSettings st;
  st.Load();

  CheckDlgButton(IDX_SETTINGS_SHOW_DOTS, st.ShowDots ? BST_CHECKED : BST_UNCHECKED);
  CheckDlgButton(IDX_SETTINGS_SHOW_REAL_FILE_ICONS, st.ShowRealFileIcons ? BST_CHECKED : BST_UNCHECKED);
  CheckDlgButton(IDX_SETTINGS_FULL_ROW, st.FullRow ? BST_CHECKED : BST_UNCHECKED);
  CheckDlgButton(IDX_SETTINGS_SHOW_GRID, st.ShowGrid ? BST_CHECKED : BST_UNCHECKED);
  CheckDlgButton(IDX_SETTINGS_SINGLE_CLICK, st.SingleClick ? BST_CHECKED : BST_UNCHECKED);
  CheckDlgButton(IDX_SETTINGS_ALTERNATIVE_SELECTION, st.AlternativeSelection ? BST_CHECKED : BST_UNCHECKED);
  // CheckDlgButton(IDX_SETTINGS_UNDERLINE, st.Underline ? BST_CHECKED : BST_UNCHECKED);

  CheckDlgButton(IDX_SETTINGS_SHOW_SYSTEM_MENU, st.ShowSystemMenu ? BST_CHECKED : BST_UNCHECKED);
  
  if (IsLargePageSupported())
    CheckDlgButton(IDX_SETTINGS_LARGE_PAGES, ReadLockMemoryEnable() ? BST_CHECKED : BST_UNCHECKED);
  else
    GetDlgItem(IDX_SETTINGS_LARGE_PAGES)->EnableWindow(FALSE);
  
  // EnableSubItems();

  return TRUE;  // return TRUE unless you set the focus to a control
  // EXCEPTION: OCX Property Pages should return FALSE
}

/*
void CSettingsPage::EnableSubItems()
{
  EnableItem(IDX_SETTINGS_UNDERLINE, IsButtonCheckedBool(IDX_SETTINGS_SINGLE_CLICK));
}
*/

BOOL CSettingsPage::OnApply()
{
  CFmSettings st;
  st.ShowDots = (IsDlgButtonChecked(IDX_SETTINGS_SHOW_DOTS) == BST_CHECKED);
  st.ShowRealFileIcons = (IsDlgButtonChecked(IDX_SETTINGS_SHOW_REAL_FILE_ICONS) == BST_CHECKED);
  st.FullRow = (IsDlgButtonChecked(IDX_SETTINGS_FULL_ROW) == BST_CHECKED);
  st.ShowGrid = (IsDlgButtonChecked(IDX_SETTINGS_SHOW_GRID) == BST_CHECKED);
  st.SingleClick = (IsDlgButtonChecked(IDX_SETTINGS_SINGLE_CLICK) == BST_CHECKED);
  st.AlternativeSelection = (IsDlgButtonChecked(IDX_SETTINGS_ALTERNATIVE_SELECTION) == BST_CHECKED);
  // st.Underline = IsButtonCheckedBool(IDX_SETTINGS_UNDERLINE);
  
  st.ShowSystemMenu = (IsDlgButtonChecked(IDX_SETTINGS_SHOW_SYSTEM_MENU) == BST_CHECKED);
  st.Save();
  
  #ifndef UNDER_CE
  if (IsLargePageSupported())
  {
    bool enable = (IsDlgButtonChecked(IDX_SETTINGS_LARGE_PAGES) == BST_CHECKED);
    NWindows::NSecurity::EnablePrivilege_LockMemory(enable);
    SaveLockMemoryEnable(enable);
  }
  #endif
  
  return CPropertyPage::OnApply();
}


BOOL CSettingsPage::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
  LPNMHDR pNMHDR = (LPNMHDR)lParam;
  if (pNMHDR->code == PSN_HELP) {
    ShowHelpWindow(NULL, kEditTopic); // change it
    return TRUE;
  }

  return CPropertyPage::OnNotify(wParam, lParam, pResult);
}


void CSettingsPage::OnCommandRange(UINT nID)
{
  SetModified();
}