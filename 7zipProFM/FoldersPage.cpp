// FoldersPage.cpp : implementation file
//

#include "stdafx.h"

#include "7zipProFM.h"
#include "FoldersPage.h"

#include "CPP/Common/StringConvert.h"

#include "CPP/Windows/Defs.h"
#include "CPP/Windows/Shell.h"

#include "CPP/7zip/UI/FileManager/HelpUtils.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"

static const UInt32 kLangIDs[] =
{
  IDT_FOLDERS_WORKING_FOLDER,
  IDR_FOLDERS_WORK_SYSTEM,
  IDR_FOLDERS_WORK_CURRENT,
  IDR_FOLDERS_WORK_SPECIFIED,
  IDX_FOLDERS_WORK_FOR_REMOVABLE
};

static const int kWorkModeButtons[] =
{
  IDR_FOLDERS_WORK_SYSTEM,
  IDR_FOLDERS_WORK_CURRENT,
  IDR_FOLDERS_WORK_SPECIFIED
};

static const int kNumWorkModeButtons = ARRAY_SIZE(kWorkModeButtons);
 

// CFoldersPage dialog

IMPLEMENT_DYNAMIC(CFoldersPage, CPropertyPage)

CFoldersPage::CFoldersPage()
	: CPropertyPage(CFoldersPage::IDD)
{

}

CFoldersPage::~CFoldersPage()
{
}

void CFoldersPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CFoldersPage, CPropertyPage)
	ON_EN_CHANGE(IDE_FOLDERS_WORK_PATH, &CFoldersPage::OnEnChangeFoldersWorkPath)
	ON_BN_CLICKED(IDC_FOLDERS_WORK_PATH, &CFoldersPage::OnBnClickedFoldersWorkPath)
	ON_BN_CLICKED(IDR_FOLDERS_WORK_SYSTEM, &CFoldersPage::OnBnClickedFoldersWorkSystem)
	ON_BN_CLICKED(IDR_FOLDERS_WORK_CURRENT, &CFoldersPage::OnBnClickedFoldersWorkCurrent)
	ON_BN_CLICKED(IDR_FOLDERS_WORK_SPECIFIED, &CFoldersPage::OnBnClickedFoldersWorkSpecified)
	ON_BN_CLICKED(IDX_FOLDERS_WORK_FOR_REMOVABLE, &CFoldersPage::OnBnClickedFoldersWorkForRemovable)
END_MESSAGE_MAP()


// CFoldersPage message handlers


int CFoldersPage::GetWorkMode() const
{
	for (int i = 0; i < kNumWorkModeButtons; i++)
		if (IsDlgButtonChecked(kWorkModeButtons[i]) == BST_CHECKED)
			return i;
	throw 0;
}

void CFoldersPage::MyEnableControls()
{
	bool enablePath = (GetWorkMode() == NWorkDir::NMode::kSpecified);
	GetDlgItem(IDE_FOLDERS_WORK_PATH)->EnableWindow(BoolToBOOL(enablePath));
	GetDlgItem(IDC_FOLDERS_WORK_PATH)->EnableWindow(BoolToBOOL(enablePath));
}

void CFoldersPage::GetWorkDir(NWorkDir::CInfo &workDirInfo)
{
	CString s;
	GetDlgItemText(IDE_FOLDERS_WORK_PATH, s);
	workDirInfo.Path = s;
	workDirInfo.ForRemovableOnly = (IsDlgButtonChecked(IDX_FOLDERS_WORK_FOR_REMOVABLE) == BST_CHECKED);
	workDirInfo.Mode = NWorkDir::NMode::EEnum(GetWorkMode());
}

BOOL CFoldersPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	LangSetDlgItems(*this, kLangIDs, ARRAY_SIZE(kLangIDs));
	m_WorkDirInfo.Load();

	CheckDlgButton(IDX_FOLDERS_WORK_FOR_REMOVABLE,
		m_WorkDirInfo.ForRemovableOnly ? BST_CHECKED : BST_UNCHECKED);

	CheckRadioButton(kWorkModeButtons[0], kWorkModeButtons[kNumWorkModeButtons - 1],
		kWorkModeButtons[m_WorkDirInfo.Mode]);

	::SetDlgItemTextW(m_hWnd, IDE_FOLDERS_WORK_PATH, fs2us(m_WorkDirInfo.Path));

	MyEnableControls();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


BOOL CFoldersPage::OnApply()
{
	GetWorkDir(m_WorkDirInfo);
	m_WorkDirInfo.Save();

	return CPropertyPage::OnApply();
}


static LPCWSTR kFoldersTopic = L"fme/options.htm#folders";

BOOL CFoldersPage::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	LPNMHDR pNMHDR = (LPNMHDR) lParam;
	if (pNMHDR->code == PSN_HELP) {
		ShowHelpWindow(NULL, kFoldersTopic);
		return TRUE;
	}

	return CPropertyPage::OnNotify(wParam, lParam, pResult);
}


void CFoldersPage::OnEnChangeFoldersWorkPath()
{
	SetModified();
}


void CFoldersPage::OnBnClickedFoldersWorkPath()
{
	MyEnableControls();
	SetModified();
	CString str;
	UString path;
	GetDlgItemText(IDE_FOLDERS_WORK_PATH, str);
	path = GetUnicodeString(str);
	UString title = LangString(IDS_FOLDERS_SET_WORK_PATH_TITLE);
	UString resultPath;

	if (!NWindows::NShell::BrowseForFolder(m_hWnd, title, path, resultPath))
		return;

	::SetDlgItemTextW(m_hWnd, IDE_FOLDERS_WORK_PATH, resultPath);
}


void CFoldersPage::OnBnClickedFoldersWorkSystem()
{
	MyEnableControls();
	SetModified();
}


void CFoldersPage::OnBnClickedFoldersWorkCurrent()
{
	MyEnableControls();
	SetModified();
}


void CFoldersPage::OnBnClickedFoldersWorkSpecified()
{
	MyEnableControls();
	SetModified();
}


void CFoldersPage::OnBnClickedFoldersWorkForRemovable()
{
	MyEnableControls();
	SetModified();
}
