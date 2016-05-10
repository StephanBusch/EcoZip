// SettingsPage.cpp : implementation file
//

#include "stdafx.h"

#ifndef UNDER_CE
#include "CPP/Windows/MemoryLock.h"
#endif

#include "CPP/7zip/UI/FileManager/HelpUtils.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"

#include "7zipProFM.h"
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

	CheckDlgButton(IDX_SETTINGS_SHOW_DOTS, ReadShowDots() ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDX_SETTINGS_SHOW_SYSTEM_MENU, Read_ShowSystemMenu() ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDX_SETTINGS_SHOW_REAL_FILE_ICONS, ReadShowRealFileIcons() ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDX_SETTINGS_FULL_ROW, ReadFullRow() ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDX_SETTINGS_SHOW_GRID, ReadShowGrid() ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDX_SETTINGS_ALTERNATIVE_SELECTION, ReadAlternativeSelection() ? BST_CHECKED : BST_UNCHECKED);
	if (IsLargePageSupported())
		CheckDlgButton(IDX_SETTINGS_LARGE_PAGES, ReadLockMemoryEnable() ? BST_CHECKED : BST_UNCHECKED);
	else
		GetDlgItem(IDX_SETTINGS_LARGE_PAGES)->EnableWindow(FALSE);
	CheckDlgButton(IDX_SETTINGS_SINGLE_CLICK, ReadSingleClick() ? BST_CHECKED : BST_UNCHECKED);
	// CheckButton(IDX_SETTINGS_UNDERLINE, ReadUnderline());

	// EnableSubItems();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


BOOL CSettingsPage::OnApply()
{
	SaveShowDots(IsDlgButtonChecked(IDX_SETTINGS_SHOW_DOTS) == BST_CHECKED);
	Save_ShowSystemMenu(IsDlgButtonChecked(IDX_SETTINGS_SHOW_SYSTEM_MENU) == BST_CHECKED);
	SaveShowRealFileIcons(IsDlgButtonChecked(IDX_SETTINGS_SHOW_REAL_FILE_ICONS) == BST_CHECKED);

	SaveFullRow(IsDlgButtonChecked(IDX_SETTINGS_FULL_ROW) == BST_CHECKED);
	SaveShowGrid(IsDlgButtonChecked(IDX_SETTINGS_SHOW_GRID) == BST_CHECKED);
	SaveAlternativeSelection(IsDlgButtonChecked(IDX_SETTINGS_ALTERNATIVE_SELECTION) == BST_CHECKED);
#ifndef UNDER_CE
	if (IsLargePageSupported())
	{
		bool enable = (IsDlgButtonChecked(IDX_SETTINGS_LARGE_PAGES) == BST_CHECKED);
		NWindows::NSecurity::EnablePrivilege_LockMemory(enable);
		SaveLockMemoryEnable(enable);
	}
#endif

	SaveSingleClick(IsDlgButtonChecked(IDX_SETTINGS_SINGLE_CLICK) == BST_CHECKED);
	// SaveUnderline(IsButtonCheckedBool(IDX_SETTINGS_UNDERLINE));

	return CPropertyPage::OnApply();
}


BOOL CSettingsPage::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	LPNMHDR pNMHDR = (LPNMHDR) lParam;
	if (pNMHDR->code == PSN_HELP) {
		ShowHelpWindow(NULL, kEditTopic);
		return TRUE;
	}

	return CPropertyPage::OnNotify(wParam, lParam, pResult);
}


void CSettingsPage::OnCommandRange(UINT nID)
{
	SetModified();
}