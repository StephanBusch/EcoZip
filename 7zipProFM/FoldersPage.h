#pragma once

#include "CPP/7zip/UI/Common/ZipRegistry.h"


// CFoldersPage dialog

class CFoldersPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CFoldersPage)

public:
	CFoldersPage();
	virtual ~CFoldersPage();

// Dialog Data
	enum { IDD = IDD_FOLDERS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	NWorkDir::CInfo m_WorkDirInfo;
public:
	void MyEnableControls();
	int GetWorkMode() const;
	void GetWorkDir(NWorkDir::CInfo &workDirInfo);
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	afx_msg void OnEnChangeFoldersWorkPath();
	afx_msg void OnBnClickedFoldersWorkPath();
	afx_msg void OnBnClickedFoldersWorkSystem();
	afx_msg void OnBnClickedFoldersWorkCurrent();
	afx_msg void OnBnClickedFoldersWorkSpecified();
	afx_msg void OnBnClickedFoldersWorkForRemovable();
};
