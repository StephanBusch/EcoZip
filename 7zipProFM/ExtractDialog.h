// ExtractDialog.h

#pragma once

#include "CPP/7zip/UI/Common/ExtractMode.h"

#ifndef NO_REGISTRY
#include "CPP/7zip/UI/Common/ZipRegistry.h"
#endif

#include "resource.h"

// CExtractDialog dialog

class CExtractDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CExtractDialog)

public:
	CExtractDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CExtractDialog();

// Dialog Data
	enum { IDD = IDD_EXTRACT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	virtual BOOL OnInitDialog();

  DECLARE_MESSAGE_MAP()
  afx_msg void OnBnClickedExtractSetPath();
  afx_msg void OnBnClickedExtractNameEnable();
  afx_msg void OnBnClickedPasswordShow();

protected:
  HICON m_hIcon;

  #ifdef NO_REGISTRY
  CEdit m_edtPath;
  #else
  CComboBox m_cmbPath;
  #endif

  #ifndef _SFX
  CEdit m_edtPathName;
  CComboBox m_cmbPathMode;
  CComboBox m_cmbOverwriteMode;
  CEdit m_edtPasswordControl;
  CString m_strPassword;
  #endif

protected:
  #ifndef _SFX
  // int GetFilesMode() const;
  void UpdatePasswordControl();
  #endif
  
  void CheckButton_TwoBools(UINT id, const CBoolPair &b1, const CBoolPair &b2);
  void GetButton_Bools(UINT id, CBoolPair &b1, CBoolPair &b2);
  virtual void OnOK();
  
  #ifndef NO_REGISTRY

  virtual void OnHelp();

  NExtract::CInfo _info;
  
  #endif

  bool IsShowPasswordChecked() const { return IsDlgButtonChecked(IDX_PASSWORD_SHOW) == BST_CHECKED; }
public:
  UString DirPath;
  UString ArcPath;

  #ifndef _SFX
  UString Password;
  #endif
  bool PathMode_Force;
  bool OverwriteMode_Force;
  NExtract::NPathMode::EEnum PathMode;
  NExtract::NOverwriteMode::EEnum OverwriteMode;

  #ifndef _SFX
  // CBoolPair AltStreams;
  CBoolPair NtSecurity;
  #endif

  CBoolPair ElimDup;

  INT_PTR Create(HWND aWndParent = 0)
  {
    return DoModal();
  }
};
