// OverwriteDialog.h

#pragma once

#include "resource.h"

namespace NOverwriteDialog
{
  struct CFileInfo
  {
    bool SizeIsDefined;
    bool TimeIsDefined;
    UInt64 Size;
    FILETIME Time;
    UString Name;
    
    void SetTime(const FILETIME *t)
    {
      if (t == 0)
        TimeIsDefined = false;
      else
      {
        TimeIsDefined = true;
        Time = *t;
      }
    }
    void SetSize(const UInt64 *size)
    {
      if (size == 0)
        SizeIsDefined = false;
      else
      {
        SizeIsDefined = true;
        Size = *size;
      }
    }
  };
}

// COverwriteDialog dialog

class COverwriteDialog : public CDialogEx
{
	DECLARE_DYNAMIC(COverwriteDialog)

public:
	COverwriteDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~COverwriteDialog();

// Dialog Data
	enum { IDD = IDD_OVERWRITE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
	afx_msg BOOL OnCommandRange(UINT nID);

protected:
  HICON m_hIcon;

protected:
  bool _isBig;

  void SetFileInfoControl(int textID, int iconID, const NOverwriteDialog::CFileInfo &fileInfo);
  void ReduceString(UString &s);
public:
  INT_PTR Create(HWND aWndParent = 0)
  {
    return DoModal();
  }

  NOverwriteDialog::CFileInfo OldFileInfo;
  NOverwriteDialog::CFileInfo NewFileInfo;
};
