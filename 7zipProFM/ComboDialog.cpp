// ComboDialog.cpp : implementation file
//

#include "stdafx.h"

#include "CPP/Common/StringConvert.h"

#ifdef LANG
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#endif

#include "ComboDialog.h"

// CComboDialog dialog

IMPLEMENT_DYNAMIC(CComboDialog, CDialogEx)

CComboDialog::CComboDialog(CWnd* pParent /*=NULL*/)
	: CDialogEx(CComboDialog::IDD, pParent)
{
}

CComboDialog::~CComboDialog()
{
}

void CComboDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO, cmb);
}


BEGIN_MESSAGE_MAP(CComboDialog, CDialogEx)
END_MESSAGE_MAP()


// CComboDialog message handlers


BOOL CComboDialog::OnInitDialog()
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

  /*
  // why it doesn't work ?
  DWORD style = _comboBox.GetStyle();
  if (Sorted)
    style |= CBS_SORT;
  else
    style &= ~CBS_SORT;
  _comboBox.SetStyle(style);
  */
  ::SetWindowTextW(m_hWnd, Title);
  
  ::SetDlgItemTextW(m_hWnd, IDT_COMBO, Static);
  ::SetDlgItemTextW(m_hWnd, IDC_COMBO, Value);
  FOR_VECTOR (i, Strings)
    cmb.AddString(GetSystemString(Strings[i]));

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


void CComboDialog::OnOK()
{
	CString str;
	cmb.GetWindowText(str);
	Value = GetUnicodeString(str);
	CDialogEx::OnOK();
}
