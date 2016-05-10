#pragma once

#include "ColumnTreeCtrl.h"

// CPluginInfoDlg dialog

class CPluginInfoDlg : public CDialogEx
{
  DECLARE_DYNAMIC(CPluginInfoDlg)

public:
  CPluginInfoDlg(CWnd* pParent = NULL);   // standard constructor
  virtual ~CPluginInfoDlg();

// Dialog Data
  enum { IDD = IDD_PLUGIN_INFO };

protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

  DECLARE_MESSAGE_MAP()
protected:
  CColumnTreeCtrl m_lstPlugins;

  CImageList m_imgList;

public:
  FString m_strBaseFolder;
  FStringVector m_codecDlls;

public:
  virtual BOOL OnInitDialog();
};
