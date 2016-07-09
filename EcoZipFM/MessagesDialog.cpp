// MessagesDialog.cpp : implementation file
//

#include "stdafx.h"

#include "CPP/Common/IntToString.h"
#include "CPP/Common/StringConvert.h"

#ifdef LANG
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#endif

#include "MessagesDialog.h"

// CMessagesDialog dialog

IMPLEMENT_DYNAMIC(CMessagesDialog, CDialogEx)

CMessagesDialog::CMessagesDialog(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMessagesDialog::IDD, pParent)
{
}

CMessagesDialog::~CMessagesDialog()
{
}

void CMessagesDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDL_MESSAGE, m_lstMessages);
}


BEGIN_MESSAGE_MAP(CMessagesDialog, CDialogEx)
END_MESSAGE_MAP()


// CMessagesDialog message handlers


void CMessagesDialog::AddMessageDirect(LPCWSTR message)
{
  int i = m_lstMessages.GetItemCount();
  wchar_t sz[16];
  ConvertUInt32ToString((UInt32)i, sz);
  m_lstMessages.InsertItem(i, GetSystemString(sz));
  m_lstMessages.SetItemText(i, 1, GetSystemString(message));
}

void CMessagesDialog::AddMessage(LPCWSTR message)
{
  UString s = message;
  while (!s.IsEmpty())
  {
    int pos = s.Find(L'\n');
    if (pos < 0)
      break;
    AddMessageDirect(s.Left(pos));
    s.DeleteFrontal(pos + 1);
  }
  AddMessageDirect(s);
}

BOOL CMessagesDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

  #ifdef LANG
  LangSetWindowText(*this, IDD_MESSAGES);
  LangSetDlgItems(*this, NULL, 0);
  ::SetDlgItemTextW(m_hWnd, IDOK, LangString(IDS_CLOSE));
  #endif
  ListView_SetUnicodeFormat(m_lstMessages, TRUE);

  m_lstMessages.InsertColumn(0, _T(""), LVCFMT_LEFT, 30);
//   m_lstMessages.InsertColumn(1, GetSystemString(LangString(IDS_MESSAGE)), LVCFMT_LEFT, 600);
  m_lstMessages.InsertColumn(1, _T(""), LVCFMT_LEFT, 600);

  FOR_VECTOR (i, *Messages)
    AddMessage((*Messages)[i]);

  m_lstMessages.SetColumnWidth(0, LVSCW_AUTOSIZE);
  m_lstMessages.SetColumnWidth(1, LVSCW_AUTOSIZE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
