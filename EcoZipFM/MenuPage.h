#pragma once


#include "CPP/7zip/UI/Common/LoadCodecs.h"
#include "afxcmn.h"

struct CShellDll
{
  FString Path;
  bool wasChanged;
  bool prevValue;
  int ctrl;
  UInt32 wow;

  CShellDll(): wasChanged (false), prevValue(false), ctrl(0), wow(0) {}
};

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

  bool _cascaded_Changed;
  bool _menuIcons_Changed;
  bool _elimDup_Changed;
  bool _flags_Changed;

  void Clear_MenuChanged()
  {
    _cascaded_Changed = false;
    _menuIcons_Changed = false;
    _elimDup_Changed = false;
    _flags_Changed = false;
  }
  
  #ifndef UNDER_CE
  CShellDll _dlls[2];
  #endif
  
public:
  CListCtrl m_lstOptions;
  virtual BOOL OnInitDialog();
  virtual BOOL OnApply();
  virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
  afx_msg void OnBnClickedSystemIntegrateToMenu();
  afx_msg void OnBnClickedSystemIntegrateToMenu2();
  afx_msg void OnBnClickedSystemCascadedMenu();
  afx_msg void OnBnClickedSystemIconInMenu();
  afx_msg void OnBnClickedExtractElimDup();
  afx_msg void OnLvnItemchangedSystemOptions(NMHDR *pNMHDR, LRESULT *pResult);
};
