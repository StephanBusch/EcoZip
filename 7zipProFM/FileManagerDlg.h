
// FileManagerDlg.h : header file
//

#pragma once


#include "BtnST.h"
#include "FMDropTarget.h"

// CFileManagerDlg dialog

class CFileManagerDlg : public CDialogEx
{
  DECLARE_DYNAMIC(CFileManagerDlg)

public:
  CFileManagerDlg(UINT nIDTemplate, CWnd* pParent = NULL);   // standard constructor
  virtual ~CFileManagerDlg();

protected:
  virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


  // Implementation
protected:
  // Generated message map functions
  afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
  virtual BOOL OnInitDialog();
  virtual BOOL PreTranslateMessage(MSG* pMsg);
  afx_msg BOOL OnEraseBkgnd(CDC* pDC);
  afx_msg void OnPaint();
  afx_msg HCURSOR OnQueryDragIcon();
  afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
  afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
  afx_msg LRESULT OnNcHitTest(CPoint point);
  afx_msg void OnNcLButtonDown(UINT nHitTest, CPoint point);
  afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);
  afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
  afx_msg void OnSize(UINT nType, int cx, int cy);
  DECLARE_MESSAGE_MAP()

protected:
  virtual void GetSysMenuRect(LPRECT lpRect);
  virtual void DrawBackground(CDC *pDC);
  virtual void PrepareBackground(CDC *pDC = NULL);
  virtual void ArrangeControls();
  virtual void SetDCClipArea(CDC *pDC) {}
  virtual CPoint GetLogoPos();
  virtual void GetButtons(CButtonST **&pButtons, UINT &nButtons) = 0;
  virtual void GetSystemButtons(CButton **&pButtons, UINT &nButtons) = 0;
  virtual void GetToolButtons(CButton **&pButtons, UINT &nButtons) = 0;
  virtual void GetNavControls(CWnd **&pControls, UINT &nControls) = 0;
  virtual CFrameWnd * GetCenterFrame() = 0;
  virtual CStatusBar * GetStatusBar() = 0;
  virtual void LoadState();
  virtual void SaveState();

protected:
  HICON m_hIcon;
  HACCEL m_hAccelTable;
  BOOL m_bMaximized;
  CFMDropTarget m_droptarget;

  CBitmap m_bmpBkgnd;

  CBitmap m_bmpBGTop;
  CBitmap m_bmpBGBottom;
  CBitmap m_bmpNavBar;
  CBitmap m_bmpTitle;
  CBitmap m_bmpLogo;
  CBitmap m_bmpRBCorner;

public:
  afx_msg void OnClose();
  afx_msg void OnDropFiles(HDROP hDropInfo);
};
