#pragma once

#include "resource.h"

// CEditPage dialog

class CEditPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CEditPage)

public:
	CEditPage();
	virtual ~CEditPage();

// Dialog Data
	enum { IDD = IDD_EDIT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CEdit m_edtViewer;
	CEdit m_edtEditor;
	CEdit m_edtDiff;
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	afx_msg void OnBnClickedEditViewer();
	afx_msg void OnBnClickedEditEditor();
	afx_msg void OnBnClickedEditDiff();
	afx_msg void OnEnChangeEditViewer();
	afx_msg void OnEnChangeEditEditor();
	afx_msg void OnEnChangeEditDiff();
};
