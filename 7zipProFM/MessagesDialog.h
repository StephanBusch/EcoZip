// MessagesDialog.h

#pragma once

#include "resource.h"

// CMessagesDialog dialog

class CMessagesDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CMessagesDialog)

public:
	CMessagesDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CMessagesDialog();

// Dialog Data
	enum { IDD = IDD_MESSAGES };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

protected:
  HICON m_hIcon;

  CListCtrl m_lstMessages;

  void AddMessageDirect(LPCWSTR message);
  void AddMessage(LPCWSTR message);
public:
  const UStringVector *Messages;
  
  INT_PTR Create(HWND aWndParent = 0) { return DoModal(); }
};
