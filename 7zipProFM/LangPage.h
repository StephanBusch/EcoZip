#pragma once


// CLangPage dialog

class CLangPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CLangPage)

public:
	CLangPage();
	virtual ~CLangPage();

// Dialog Data
	enum { IDD = IDD_LANG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	UStringVector _paths;
	bool LangWasChanged;
	CComboBox m_cmbLang;
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	afx_msg void OnCbnSelchangeLangLang();
};
