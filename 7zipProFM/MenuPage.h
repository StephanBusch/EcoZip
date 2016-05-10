#pragma once


#include "CPP/7zip/UI/Common/LoadCodecs.h"
#include "afxcmn.h"

// CMenuPage dialog

class CMenuPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CMenuPage)

public:
	CMenuPage();
	virtual ~CMenuPage();

// Dialog Data
	enum { IDD = IDD_MENU };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
protected:
	bool _initMode;
public:
	CListCtrl m_lstOptions;
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	afx_msg void OnBnClickedSystemIntegrateToContextMenu();
	afx_msg void OnBnClickedSystemCascadedMenu();
	afx_msg void OnBnClickedSystemIconInMenu();
	afx_msg void OnLvnItemchangedSystemOptions(NMHDR *pNMHDR, LRESULT *pResult);
};
