// CopyDialog.h

#pragma once

#include "resource.h"

// CCopyDialog dialog

const int kCopyDialog_NumInfoLines = 11;

class CCopyDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CCopyDialog)

public:
	CCopyDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CCopyDialog();

// Dialog Data
	enum { IDD = IDD_COPY };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedCopySetPath();

protected:
  HICON m_hIcon;

  CComboBox m_cmbPath;

public:
  UString Title;
  UString Static;
  UString Value;
  UString Info;
  UStringVector Strings;

};
