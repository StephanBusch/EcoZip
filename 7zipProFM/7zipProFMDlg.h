
// 7zipProFMDlg.h : header file
//

#pragma once

#include "BtnST.h"
#include "FileManagerDlg.h"
#include "FolderView.h"
#include "DetailView.h"
#include "ContentsView.h"

// C7zipProFMDlg dialog
class C7zipProFMDlg : public CFileManagerDlg
{
  friend CDropTarget;
  // Construction
public:
  C7zipProFMDlg(CWnd* pParent = NULL);	// standard constructor
  virtual ~C7zipProFMDlg();

  // Dialog Data
  enum { IDD = IDD_7ZIPPROFM_DIALOG };

protected:
  virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


  // Implementation
protected:
  // Generated message map functions
  afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
  virtual BOOL OnInitDialog();
  virtual BOOL PreTranslateMessage(MSG* pMsg);
  afx_msg void OnEnterSizeMove();
  afx_msg void OnExitSizeMove();
  afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
  afx_msg void OnBnClickedBtnMenu();
  afx_msg void OnBnClickedBtnSkin();
  afx_msg void OnBnClickedBtnMinimize();
  afx_msg void OnBnClickedBtnMaximize();
  afx_msg void OnBnClickedBtnClose();
  afx_msg void OnUpdateItemSelected(CCmdUI *pCmdUI);
  afx_msg void OnUpdateRange(CCmdUI *pCmdUI);
  afx_msg void OnCommandRange(UINT nID);
  afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);

  afx_msg void OnUpdateHistory(CCmdUI *pCmdUI);
  afx_msg void OnBnClickedBtnNavBackTo();
  afx_msg void OnBnClickedBtnSwitchViewStyle();
  afx_msg void OnBnClickedBtnViewStyle();
  afx_msg void OnUpdateViewStyle(CCmdUI *pCmdUI);
  afx_msg void OnViewStyle(UINT nID);
  afx_msg void OnUpdateViewArrange(CCmdUI *pCmdUI);

  afx_msg void OnCmbDropdown();
  afx_msg void OnCbenEndeditCmbPath(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnCbnDropdownCmbPath();
  afx_msg void OnCbnSelendokCmbPath();
  afx_msg LRESULT OnFolderChanged(WPARAM wParam, LPARAM lParam);

  afx_msg LRESULT OnRefresh_StatusBar(WPARAM wParam, LPARAM lParam);

  afx_msg void OnNextPane();
  afx_msg void OnToolsOptions();
  DECLARE_MESSAGE_MAP()

protected:
  virtual void GetSysMenuRect(LPRECT lpRect);
  virtual void ArrangeControls();
  virtual void SetDCClipArea(CDC *pDC);
  virtual CPoint GetLogoPos();
  virtual void GetButtons(CButtonST **&pButtons, UINT &nButtons);
  virtual void GetSystemButtons(CButton **&pButtons, UINT &nButtons);
  virtual void GetToolButtons(CButton **&pButtons, UINT &nButtons);
  virtual void GetNavControls(CWnd **&pControls, UINT &nControls);
  virtual CFrameWnd * GetCenterFrame();
  virtual CStatusBar * GetStatusBar();
  virtual void LoadState();
  virtual void SaveState();
  virtual void OnOK();
  virtual void OnCancel();

  void UpdateFolder(
    LPCTSTR lpszNewPath = NULL,
    BOOL updateChildren = TRUE,
    BOOL updateHistory = TRUE);
  void UpdatePath(LPCTSTR lpszNewPath = NULL, BOOL updateHistory = TRUE);
  void C7zipProFMDlg::AddComboBoxItem(
    LPCTSTR name, int iconIndex, int indent, bool addToList);
  void Refresh_StatusBar();

protected:
  CButtonST **pButtons;
  UINT *nButtonIDs;
  UINT nButtons;

  CButtonST m_btnMenu;
  CButtonST m_btnSkin;
  CButtonST m_btnMinimize;
  CButtonST m_btnMaximize;
  CButtonST m_btnClose;

  CButtonST m_btnAddToArchive;
  CButtonST m_btnExtractFiles;
  CButtonST m_btnTestArchive;
  CButtonST m_btnCopyTo;
  CButtonST m_btnMoveTo;
  CButtonST m_btnDelete;
  CButtonST m_btnProperties;
  CButtonST m_btnBenchmark;

  CButtonST m_btnNavBackward;
  CButtonST m_btnNavForward;
  CButtonST m_btnNavBackTo;
  CButtonST m_btnNavUp;
  CButtonST m_btnSwitchViewStyle;
  CButtonST m_btnViewStyle;
  CComboBoxEx m_cmbPath;
  CString m_strFolderPrefix;
  FStringVector m_vecPaths;

  CFrameWnd *m_pFrameWnd;
  CSplitterWnd m_wndSplitter;
  CSplitterWnd m_wndSplitterLR;
  CSplitterWnd m_wndSplitterUD;
  CFolderView *m_pFolderView;
  CDetailView *m_pDetailView;
  CContentsView *m_pContentsView;

  CStatusBar m_wndStatusBar;

  BOOL m_bTracking;
  FStringVector m_vecHistories;
  unsigned m_nHistoryPos;
  unsigned m_nMaxHistory;

  int m_nContentsViewStyle;

  // Dialog Activities
};
