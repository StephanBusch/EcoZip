#pragma once

#include "ColumnTreeCtrl.h"

// CPluginsPage dialog

class CPluginsPage : public CPropertyPage
{
  DECLARE_DYNAMIC(CPluginsPage)

public:
  CPluginsPage();
  virtual ~CPluginsPage();

// Dialog Data
  enum { IDD = IDD_PLUGINS };

protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

  DECLARE_MESSAGE_MAP()
protected:
  CColumnTreeCtrl m_lstPlugins;

  CImageList m_imgList;

public:
  virtual BOOL OnInitDialog();
  virtual BOOL OnApply();
  afx_msg void OnBnClickedAdd();
};
