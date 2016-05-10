// CopyDialog.cpp : implementation file
//

#include "stdafx.h"

#include "CPP/Common/StringConvert.h"

#include "CPP/Windows/Shell.h"

#ifdef LANG
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#endif

#include "CopyDialog.h"

// CCopyDialog dialog

IMPLEMENT_DYNAMIC(CCopyDialog, CDialogEx)

CCopyDialog::CCopyDialog(CWnd* pParent /*=NULL*/)
	: CDialogEx(CCopyDialog::IDD, pParent)
{
}

CCopyDialog::~CCopyDialog()
{
}

void CCopyDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COPY, m_cmbPath);
}


BEGIN_MESSAGE_MAP(CCopyDialog, CDialogEx)
	ON_BN_CLICKED(IDC_COPY_SET_PATH, &CCopyDialog::OnBnClickedCopySetPath)
END_MESSAGE_MAP()


// CCopyDialog message handlers


BOOL CCopyDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	#ifdef LANG
	LangSetDlgItems(*this, NULL, 0);
	#endif
	::SetWindowTextW(m_hWnd, Title);

	::SetDlgItemTextW(m_hWnd, IDT_COPY, Static);
	#ifdef UNDER_CE
	// we do it, since WinCE selects Value\something instead of Value !!!!
	m_cmbPath.AddString(GetSystemString(Value));
	#endif
	FOR_VECTOR(i, Strings)
		m_cmbPath.AddString(GetSystemString(Strings[i]));
	::SetDlgItemTextW(m_hWnd, IDC_COPY, Value);
	::SetDlgItemTextW(m_hWnd, IDT_COPY_INFO, Info);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


void CCopyDialog::OnBnClickedCopySetPath()
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


void CCopyDialog::OnOK()
{
	CString str;
	m_cmbPath.GetWindowText(str);
	Value = GetUnicodeString(str);

	CDialogEx::OnOK();
}
