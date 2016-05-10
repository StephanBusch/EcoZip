// PasswordDialog.cpp : implementation file
//

#include "stdafx.h"

#include "PasswordDialog.h"

#include "CPP/Common/StringConvert.h"

#ifdef LANG
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#endif

#ifdef LANG
static const UInt32 kLangIDs[] =
{
  IDT_PASSWORD_ENTER,
  IDX_PASSWORD_SHOW
};
#endif


// CPasswordDialog dialog

IMPLEMENT_DYNAMIC(CPasswordDialog, CDialogEx)

CPasswordDialog::CPasswordDialog(CWnd* pParent /*=NULL*/)
	: CDialogEx(CPasswordDialog::IDD, pParent)
	, ShowPassword(false)
{

}

CPasswordDialog::~CPasswordDialog()
{
}

void CPasswordDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDE_PASSWORD_PASSWORD, m_edtPassword);
}


BEGIN_MESSAGE_MAP(CPasswordDialog, CDialogEx)
	ON_BN_CLICKED(IDX_PASSWORD_SHOW, &CPasswordDialog::OnBnClickedPasswordShow)
END_MESSAGE_MAP()


// CPasswordDialog message handlers


void CPasswordDialog::ReadControls()
{
	CString str;
	m_edtPassword.GetWindowText(str);
	Password = GetUnicodeString(str);
	ShowPassword = (IsDlgButtonChecked(IDX_PASSWORD_SHOW) == BST_CHECKED);
}

void CPasswordDialog::SetTextSpec()
{
	m_edtPassword.SetPasswordChar(ShowPassword ? 0 : TEXT('*'));
	::SetDlgItemTextW(m_hWnd, IDE_PASSWORD_PASSWORD, Password);
}

BOOL CPasswordDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	#ifdef LANG
	LangSetWindowText(*this, IDD_PASSWORD);
	LangSetDlgItems(*this, kLangIDs, ARRAY_SIZE(kLangIDs));
	#endif
	CheckDlgButton(IDX_PASSWORD_SHOW, ShowPassword ? BST_CHECKED : BST_UNCHECKED);
	SetTextSpec();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


void CPasswordDialog::OnBnClickedPasswordShow()
{
	ReadControls();
	SetTextSpec();
}


void CPasswordDialog::OnOK()
{
	ReadControls();

	CDialogEx::OnOK();
}
