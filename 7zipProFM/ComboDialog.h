// ComboDialog.h

#pragma once

#include "resource.h"

// CComboDialog dialog

class CComboDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CComboDialog)

public:
	CComboDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CComboDialog();

// Dialog Data
	enum { IDD = IDD_COMBO };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()

protected:
  HICON m_hIcon;

  CComboBox cmb;
public:
  // bool Sorted;
  UString Title;
  UString Static;
  UString Value;
  UStringVector Strings;
  
  // CComboDialog(): Sorted(false) {};

};
