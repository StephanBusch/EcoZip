#pragma once

#include "resource.h"

// CSplitDialog dialog

class CSplitDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CSplitDialog)

public:
	CSplitDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSplitDialog();

// Dialog Data
	enum { IDD = IDD_SPLIT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnBnClickedSplitSetPath();
	DECLARE_MESSAGE_MAP()
public:
	CComboBox m_cmbPath;
	CComboBox m_cmbVolume;
public:
	UString FilePath;
	UString Path;
	CRecordVector<UInt64> VolumeSizes;
};
