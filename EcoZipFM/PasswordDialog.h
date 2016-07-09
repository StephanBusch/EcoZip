#pragma once

#include "resource.h"

// CPasswordDialog dialog

class CPasswordDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CPasswordDialog)

public:
	CPasswordDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPasswordDialog();

// Dialog Data
	enum { IDD = IDD_PASSWORD };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedPasswordShow();
	virtual void OnOK();
	DECLARE_MESSAGE_MAP()

	void SetTextSpec();
	void ReadControls();
public:
	CEdit m_edtPassword;

public:
	UString Password;
	bool ShowPassword;
  
	INT_PTR Create(HWND parentWindow = 0) { return DoModal(); }
};
