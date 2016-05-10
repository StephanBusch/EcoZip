#pragma once


// CSettingsPage dialog

class CSettingsPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CSettingsPage)

public:
	CSettingsPage();
	virtual ~CSettingsPage();

// Dialog Data
	enum { IDD = IDD_SETTINGS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	afx_msg void OnCommandRange(UINT nID);
};
