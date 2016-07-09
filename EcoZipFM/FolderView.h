#pragma once


#include "CPP/7zip/UI/FileManager/SysIconUtils.h"

#include "RootFolder.h"
#include "PanelCopyThread.h"

// CFolderView

class CFolderView : public CTreeCtrl
{
protected: // create from serialization only
  CFolderView();
  DECLARE_DYNCREATE(CFolderView)

  // Attributes
public:
protected:
  CExtToIconMap _extToIconMap;
  HTREEITEM hComputer;
  HTREEITEM hNetwork;
  BOOL m_bTracking;

  // Operations
public:
  CMyComPtr<IFolderFolder> GetSelectedFolder();
  FString GetFullPath();
  BOOL SelectFolder(CMyComPtr<IFolderFolder> _folder);
  BOOL SelectFolder(LPCTSTR lpszPath);
  void RenameFolder();
  void CopyTo();
  void MoveTo();
  void DeleteFolder(bool toRecycleBin);
  void CreateFolder();
  void CreateFile();
  void Properties();

protected:
  void FillRootItems();
  FString GetFullPath(HTREEITEM hItem);
  CMyComPtr<IFolderFolder> GetFolder(HTREEITEM hItem);
  BOOL MightHaveChild(HTREEITEM hParent);
  void AddDumySingleChild(HTREEITEM hParent);
  INT PrepareChildren(HTREEITEM hParent, BOOL test = FALSE);

  HTREEITEM GetTopLevelItem(INT nType);

  bool InvokePluginCommand(int id, IContextMenu *systemContextMenu);

  HRESULT CFolderView::CreateShellContextMenu(
    HTREEITEM hItem,
    CMyComPtr<IContextMenu> &systemContextMenu);
  HMENU CreateSystemMenu(
    HTREEITEM hItem,
    CMyComPtr<IContextMenu> &systemContextMenu);
  HMENU CreateFileMenu(HTREEITEM hItem,
    CMyComPtr<IContextMenu> &systemContextMenu);
  void LoadFolderMenu(HMENU hMenu, int startPos, bool allAreFiles);

  void MessageBoxError(HRESULT errorCode, LPCWSTR caption);
  void MessageBoxError(HRESULT errorCode);
  void MessageBoxErrorForUpdate(HRESULT errorCode, UINT resourceID);
  void MessageBoxErrorLang(UINT resourceID);

  int GetRealIndex(CMyComPtr<IFolderFolder> folder, const wchar_t *szItem);

  void OnCopy(bool move);
  HRESULT CopyTo(CCopyToOptions &options,
    const CRecordVector<UInt32> &indices,
    UStringVector *messages,
    bool showProgress = true);

  // Overrides
public:
  // for child windows, views, panes etc
  virtual BOOL Create(LPCTSTR lpszClassName,
    LPCTSTR lpszWindowName, DWORD dwStyle,
    const RECT& rect,
    CWnd* pParentWnd, UINT nID,
    CCreateContext* pContext = NULL);
  virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
  virtual BOOL PreTranslateMessage(MSG* pMsg);

  void Refresh();

  // Implementation
public:
  virtual ~CFolderView();

  // Generated message map functions
protected:
  afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
  afx_msg LRESULT OnNcHitTest(CPoint point);
  afx_msg void OnTvnItemexpanding(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnNMClick(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnNMReturn(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
  afx_msg void OnNMRClick(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnTvnEndlabeledit(NMHDR *pNMHDR, LRESULT *pResult);
  DECLARE_MESSAGE_MAP()
};

enum {
  TVI_TYPE_COMPUTER = ROOT_INDEX_COMPUTER,
  TVI_TYPE_DESKTOP = ROOT_INDEX_DESKTOP,
  TVI_TYPE_DOCUMENTS = ROOT_INDEX_DOCUMENTS,
  TVI_TYPE_NETWORK = ROOT_INDEX_NETWORK,
  TVI_TYPE_VOLUMES = ROOT_INDEX_VOLUMES,

  TVI_TYPE_TOP = -1,
  TVI_TYPE_DRIVE = -2,
  TVI_TYPE_NORMAL = -3,
  TVI_TYPE_DUMMY = -4
};

