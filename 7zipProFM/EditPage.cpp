// EditPage.cpp : implementation file
//

#include "stdafx.h"
#include "7zipProFM.h"
#include "EditPage.h"

#include "CPP/Common/StringConvert.h"
#include "CPP/7zip/UI/FileManager/HelpUtils.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"


static const UInt32 kLangIDs[] =
{
  IDT_EDIT_EDITOR,
  IDT_EDIT_DIFF
};

static const UInt32 kLangIDs_Colon[] =
{
  IDT_EDIT_VIEWER
};

static LPCWSTR kEditTopic = L"FME/options.htm#editor";

// CEditPage dialog

IMPLEMENT_DYNAMIC(CEditPage, CPropertyPage)

CEditPage::CEditPage()
	: CPropertyPage(CEditPage::IDD)
{

}

CEditPage::~CEditPage()
{
}

void CEditPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDE_EDIT_VIEWER, m_edtViewer);
	DDX_Control(pDX, IDE_EDIT_EDITOR, m_edtEditor);
	DDX_Control(pDX, IDE_EDIT_DIFF, m_edtDiff);
}


BEGIN_MESSAGE_MAP(CEditPage, CPropertyPage)
	ON_BN_CLICKED(IDC_EDIT_VIEWER, &CEditPage::OnBnClickedEditViewer)
	ON_BN_CLICKED(IDC_EDIT_EDITOR, &CEditPage::OnBnClickedEditEditor)
	ON_BN_CLICKED(IDC_EDIT_DIFF, &CEditPage::OnBnClickedEditDiff)
	ON_EN_CHANGE(IDE_EDIT_VIEWER, &CEditPage::OnEnChangeEditViewer)
	ON_EN_CHANGE(IDE_EDIT_EDITOR, &CEditPage::OnEnChangeEditEditor)
	ON_EN_CHANGE(IDE_EDIT_DIFF, &CEditPage::OnEnChangeEditDiff)
END_MESSAGE_MAP()


// CEditPage message handlers


BOOL CEditPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	LangSetDlgItems(*this, kLangIDs, ARRAY_SIZE(kLangIDs));
	LangSetDlgItems_Colon(*this, kLangIDs_Colon, ARRAY_SIZE(kLangIDs_Colon));

	{
		UString path;
		ReadRegEditor(false, path);
		::SetDlgItemTextW(m_hWnd, IDE_EDIT_VIEWER, path);
	}
	{
		UString path;
		ReadRegEditor(true, path);
		::SetDlgItemTextW(m_hWnd, IDE_EDIT_EDITOR, path);
	}
	{
		UString path;
		ReadRegDiff(path);
		::SetDlgItemTextW(m_hWnd, IDE_EDIT_DIFF, path);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


BOOL CEditPage::OnApply()
{
	CString path;

	GetDlgItemText(IDE_EDIT_VIEWER, path);
	SaveRegEditor(false, GetUnicodeString(path));

	GetDlgItemText(IDE_EDIT_EDITOR, path);
	SaveRegEditor(true, GetUnicodeString(path));


	GetDlgItemText(IDE_EDIT_DIFF, path);
	SaveRegDiff(GetUnicodeString(path));

	return CPropertyPage::OnApply();
}


BOOL CEditPage::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	LPNMHDR pNMHDR = (LPNMHDR) lParam;
	if (pNMHDR->code == PSN_HELP) {
		ShowHelpWindow(NULL, kEditTopic);
		return TRUE;
	}

	return CPropertyPage::OnNotify(wParam, lParam, pResult);
}


static void Edit_BrowseForFile(CEdit &edit, CWnd *pParentWnd)
{
	CString strPath;
	edit.GetWindowText(strPath);
	CString strResult;
	CFileDialog dlgFile(TRUE, NULL, strPath,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("*.exe"), pParentWnd);
	if (dlgFile.DoModal() == IDCANCEL)
		return;
	strResult = dlgFile.GetPathName();

	edit.SetWindowText(strResult);
}


void CEditPage::OnBnClickedEditViewer()
{
	Edit_BrowseForFile(m_edtViewer, this);
}


void CEditPage::OnBnClickedEditEditor()
{
	Edit_BrowseForFile(m_edtEditor, this);
}


void CEditPage::OnBnClickedEditDiff()
{
	Edit_BrowseForFile(m_edtDiff, this);
}


void CEditPage::OnEnChangeEditViewer()
{
	SetModified();
}


void CEditPage::OnEnChangeEditEditor()
{
	SetModified();
}


void CEditPage::OnEnChangeEditDiff()
{
	SetModified();
}
