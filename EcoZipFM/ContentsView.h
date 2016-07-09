// ContentsView.h

#pragma once

#include "CPP/Common/Defs.h"
#include "CPP/Common/MyCom.h"

#include "CPP/Windows/FileName.h"
#include "CPP/7zip/Archive/IArchive.h"

// #include "CPP/7zip/UI/FileManager/AppState.h"
#include "CPP/7zip/UI/FileManager/IFolder.h"
#include "CPP/7zip/UI/FileManager/MyCom2.h"
#include "CPP/7zip/UI/FileManager/SysIconUtils.h"

#include "ProgressDialog2.h"
#include "ExtractCallback.h"

#include "DetailView.h"
#include "PanelCopyThread.h"
#include "ThreadFolderOperations.h"

#ifdef UNDER_CE
#define NON_CE_VAR(_v_)
#else
#define NON_CE_VAR(_v_) _v_
#endif


struct CColumnInfo
{
  PROPID PropID;
  bool IsVisible;
  UInt32 Width;

  bool IsEqual(const CColumnInfo &a) const
  {
    return PropID == a.PropID && IsVisible == a.IsVisible && Width == a.Width;
  }
};

struct CListViewInfo
{
  CRecordVector<CColumnInfo> Columns;
  PROPID SortID;
  bool Ascending;
  bool IsLoaded;
  int mode;

  void Clear()
  {
    SortID = 0;
    Ascending = true;
    IsLoaded = false;
    Columns.Clear();
  }

  CListViewInfo():
    SortID(0),
    Ascending(true),
    IsLoaded(false)
    {}

  /*
  int FindColumnWithID(PROPID propID) const
  {
    FOR_VECTOR (i, Columns)
      if (Columns[i].PropID == propID)
        return i;
    return -1;
  }
  */

  bool IsEqual(const CListViewInfo &info) const
  {
    if (Columns.Size() != info.Columns.Size() ||
        SortID != info.SortID ||
        Ascending != info.Ascending)
      return false;
    FOR_VECTOR (i, Columns)
      if (!Columns[i].IsEqual(info.Columns[i]))
        return false;
    return true;
  }

  void Save(const UString &id) const;
  void Read(const UString &id);
};

class CContentsView;

class CDropTarget :
  public IDropTarget,
  public CMyUnknownImp
{
  CMyComPtr<IDataObject> m_DataObject;
  UStringVector m_SourcePaths;
  int m_SelectionIndex;
  bool m_DropIsAllowed;      // = true, if data contain fillist
  bool m_PanelDropIsAllowed; // = false, if current target_panel is source_panel.
  // check it only if m_DropIsAllowed == true
  int m_SubFolderIndex;
  UString m_SubFolderName;

  CWnd *m_pWindow;
  bool m_IsAppTarget;        // true, if we want to drop to app window (not to panel).

  bool m_SetPathIsOK;

  bool IsItSameDrive() const;

  void QueryGetData(IDataObject *dataObject);
  bool IsFsFolderPath() const;
  DWORD GetEffect(DWORD keyState, POINTL pt, DWORD allowedEffect);
  void RemoveSelection();
  void PositionCursor(POINTL ptl);
  UString GetTargetPath() const;
  bool SetPath(bool enablePath) const;
  bool SetPath();

public:
  MY_UNKNOWN_IMP1_MT(IDropTarget)
    STDMETHOD(DragEnter)(IDataObject * dataObject, DWORD keyState, POINTL pt, DWORD *effect);
  STDMETHOD(DragOver)(DWORD keyState, POINTL pt, DWORD * effect);
  STDMETHOD(DragLeave)();
  STDMETHOD(Drop)(IDataObject * dataObject, DWORD keyState, POINTL pt, DWORD *effect);

  CDropTarget(CWnd *pWnd) :
    Source(NULL),
    m_IsAppTarget(false),
    m_pWindow(pWnd),
    m_PanelDropIsAllowed(false),
    m_DropIsAllowed(false),
    m_SelectionIndex(-1),
    m_SubFolderIndex(-1),
    m_SetPathIsOK(false) {}

  CWnd *Source;
};

struct CPropColumn
{
  int Order;
  PROPID ID;
  VARTYPE Type;
  bool IsVisible;
  bool IsRawProp;
  UInt32 Width;
  UString Name;

  bool IsEqualTo(const CPropColumn &a) const
  {
    return Order == a.Order
        && ID == a.ID
        && Type == a.Type
        && IsVisible == a.IsVisible
        && IsRawProp == a.IsRawProp
        && Width == a.Width
        && Name == a.Name;
  }

  int Compare(const CPropColumn &a) const { return MyCompare(Order, a.Order); }

  int Compare_NameFirst(const CPropColumn &a) const
  {
    if (ID == kpidName)
    {
      if (a.ID != kpidName)
        return -1;
    }
    else if (a.ID == kpidName)
      return 1;
    return MyCompare(Order, a.Order);
  }
};


class CPropColumns: public CObjectVector<CPropColumn>
{
public:
  int FindItem_for_PropID(PROPID id) const
  {
    FOR_VECTOR (i, (*this))
      if ((*this)[i].ID == id)
        return i;
    return -1;
  }

  bool IsEqualTo(const CPropColumns &props) const
  {
    if (Size() != props.Size())
      return false;
    FOR_VECTOR (i, (*this))
      if (!(*this)[i].IsEqualTo(props[i]))
        return false;
    return true;
  }
};

struct CSelectedState
{
  int FocusedItem;
  UString FocusedName;
  bool SelectFocused;
  UStringVector SelectedNames;
  
  CSelectedState(): FocusedItem(-1), SelectFocused(false) {}
};


// CContentsView

class CContentsView : public CListCtrl
{
  friend class CEcoZipFMDlg;
protected: // create from serialization only
  CContentsView();
  DECLARE_DYNCREATE(CContentsView)

  // Attributes
public:
  BOOL m_bResizing;
  CRect m_rtPrev;
  CDetailView * m_pDetailView;

  // Operations
public:

  // Overrides
public:
  // for child windows, views, panes etc
  virtual BOOL Create(LPCTSTR lpszClassName,
    LPCTSTR lpszWindowName, DWORD dwStyle,
    const RECT& rect,
    CWnd* pParentWnd, UINT nID,
    CCreateContext* pContext = NULL);

  virtual HRESULT Initialize(
    LPCTSTR currentFolderPrefix,
    LPCTSTR arcFormat,
    bool needOpenArc,
    bool &archiveIsOpened, bool &encrypted);

  virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
  virtual BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult);

  // Implementation
public:
  virtual ~CContentsView();

public:

  int GetFocusedItem() const { return GetNextItem(-1, LVNI_FOCUSED); }

  BOOL GetItemParam(int itemIndex, LPARAM &param) const;

  BOOL IsItemSelected(int index) const { return GetItemState(index, LVIS_SELECTED) == LVIS_SELECTED; }

protected:
  CExtToIconMap _extToIconMap;

public:
  CDropTarget *_dropTargetSpec;
  CMyComPtr<IDropTarget> _dropTarget;

  void ReloadSettings();
  void OnCopy(bool move, bool copyToSame);

public:
  void DeleteItems(bool toRecycleBin);
  void CreateFolder();
  void CreateFile();
  bool CorrectFsPath(const UString &path, UString &result);
  // bool IsPathForPlugin(const UString &path);

protected:
  HRESULT InitColumns();
  void DeleteColumn(unsigned index);
  void AddColumn(const CPropColumn &prop);

  void SetFocusedSelectedItem(int index, bool select);
  HRESULT RefreshListCtrl(const UString &focusedName, int focusedPos, bool selectFocused,
      const UStringVector &selectedNames);

  void OnShiftSelectMessage();
  void OnArrowWithShift();

  void OnInsert();
  // void OnUpWithShift();
  // void OnDownWithShift();
public:
  void UpdateSelection();
  void SelectSpec(bool selectMode);
  void SelectByType(bool selectMode);
  void SelectAll(bool selectMode);
  void InvertSelection();
protected:

  // UString GetFileType(UInt32 index);
  LRESULT GetDispInfo(LVITEMW &item);
public:

  bool _showDots;
  bool _showRealFileIcons;
  bool _enableItemChangeNotify;
  bool _mySelectMode;
  CBoolVector _selectedStatusVector;

  CSelectedState _selectedState;
  bool _thereAreDeletedItems;
  bool _markDeletedItems;

  UInt32 GetRealIndex(const LVITEMW &item) const
  {
    /*
    if (_virtualMode)
      return _realIndices[item.iItem];
    */
    return (UInt32)item.lParam;
  }
  
  int GetRealItemIndex(int indexInListView) const
  {
    /*
    if (_virtualMode)
      return indexInListView;
    */
    LPARAM param;
    if (!this->GetItemParam(indexInListView, param))
      throw 1;
    return (int)param;
  }

  UInt32 _ListViewStyle;

  bool _flatMode;
  bool _flatModeForDisk;
  bool _flatModeForArc;

  // bool _showNtfsStrems_Mode;
  // bool _showNtfsStrems_ModeForDisk;
  // bool _showNtfsStrems_ModeForArc;

  bool _dontShowMode;


  UString _currentFolderPrefix;
  
  CObjectVector<CFolderLink> _parentFolders;
  NWindows::NDLL::CLibrary _library;
  
  CMyComPtr<IFolderFolder> _folder;
  CMyComPtr<IFolderCompare> _folderCompare;
  CMyComPtr<IFolderGetItemName> _folderGetItemName;
  CMyComPtr<IArchiveGetRawProps> _folderRawProps;
  CMyComPtr<IFolderAltStreams> _folderAltStreams;
  CMyComPtr<IFolderOperations> _folderOperations;
  int selectedIndex;

  void ReleaseFolder();
  void SetNewFolder(IFolderFolder *newFolder);

  void GetSelectedNames(UStringVector &selectedNames);
  void SaveSelectedState(CSelectedState &s);
  HRESULT RefreshListCtrl(const CSelectedState &s);
  HRESULT RefreshListCtrlSaveFocused();

  bool GetItem_BoolProp(UInt32 itemIndex, PROPID propID) const;
  bool IsItem_Deleted(int itemIndex) const;
  bool IsItem_Folder(int itemIndex) const;
  bool IsItem_AltStream(int itemIndex) const;

  UString GetItemName(int itemIndex) const;
  UString GetItemName_for_Copy(int itemIndex) const;
  void GetItemName(int itemIndex, UString &s) const;
  UString GetItemPrefix(int itemIndex) const;
  UString GetItemRelPath(int itemIndex) const;
  UString GetItemRelPath2(int itemIndex) const;
  UString GetItemFullPath(int itemIndex) const;
  UInt64 GetItemSize(int itemIndex) const;

  ////////////////////////
  // PanelFolderChange.cpp

  void SetToRootFolder();
  HRESULT BindToPath(const UString &fullPath, const UString &arcFormat, bool &archiveIsOpened, bool &encrypted); // can be prefix
  HRESULT BindToPathAndRefresh(const UString &path);
  void OpenDrivesFolder();
  
  void SetBookmark(unsigned index);
  void OpenBookmark(unsigned index);
  
  void LoadFullPath();
  void LoadFullPathAndShow();
  void FoldersHistory();
  void OpenParentFolder();
  void CloseOneLevel();
  void CloseOpenFolders();
  void OpenRootFolder();

  void SaveListViewInfo();

  bool _needSaveInfo;
  UString _typeIDString;
  CListViewInfo _listViewInfo;
  
  CPropColumns _columns;
  CPropColumns _visibleColumns;
  
  PROPID _sortID;
  // int _sortIndex;
  bool _ascending;
  Int32 _isRawSortProp;

  void SetSortRawStatus();

  void ShowColumnsContextMenu(int x, int y);

  void OnReload();

  CMyComPtr<IContextMenu> _sevenZipContextMenu;
  CMyComPtr<IContextMenu> _systemContextMenu;
  HRESULT CreateShellContextMenu(
      const CRecordVector<UInt32> &operatedIndices,
      CMyComPtr<IContextMenu> &systemContextMenu);
  void CreateSystemMenu(HMENU menu,
      const CRecordVector<UInt32> &operatedIndices,
      CMyComPtr<IContextMenu> &systemContextMenu);
  void CreateSevenZipMenu(HMENU menu,
      const CRecordVector<UInt32> &operatedIndices,
      CMyComPtr<IContextMenu> &sevenZipContextMenu);
  void CreateFileMenu(HMENU menu,
      CMyComPtr<IContextMenu> &sevenZipContextMenu,
      CMyComPtr<IContextMenu> &systemContextMenu,
      bool programMenu);
  void CreateFileMenu(HMENU menu);
  bool InvokePluginCommand(int id);
  bool InvokePluginCommand(int id, IContextMenu *sevenZipContextMenu,
      IContextMenu *systemContextMenu);

  void InvokeSystemCommand(const char *command);
  void CopyTo() { OnCopy(false, false); }
  void MoveTo() { OnCopy(true, false); }
  HRESULT CalculateCrc2(const UString &methodName);
  void CalculateCrc(const UString &methodName);
  void Split();
  void Combine();
  void Properties();
  void EditCut();
  void EditCopy();
  void EditPaste();

  int _startGroupSelect;

  bool _selectionIsDefined;
  bool _selectMark;
  int _prevFocusedItem;

 
  // void SortItems(int index);
  void SortItemsWithPropID(PROPID propID);

  void GetSelectedItemsIndices(CRecordVector<UInt32> &indices) const;
  void GetOperatedItemIndices(CRecordVector<UInt32> &indices) const;
  void GetAllItemIndices(CRecordVector<UInt32> &indices) const;
  void GetOperatedIndicesSmart(CRecordVector<UInt32> &indices) const;
  // void GetOperatedListViewIndices(CRecordVector<UInt32> &indices) const;
  void KillSelection();

  UString GetFolderTypeID() const;
  
  bool IsFolderTypeEqTo(const char *s) const;
  bool IsRootFolder() const;
  bool IsFSFolder() const;
  bool IsFSDrivesFolder() const;
  bool IsAltStreamsFolder() const;
  bool IsArcFolder() const;
  
  /*
    c:\Dir
    Computer\
    \\?\
    \\.\
  */
  bool Is_IO_FS_Folder() const
  {
    return IsFSFolder() || IsFSDrivesFolder() || IsAltStreamsFolder();
  }

  bool Is_Slow_Icon_Folder() const
  {
    return IsFSFolder() || IsAltStreamsFolder();
  }

  // bool IsFsOrDrivesFolder() const { return IsFSFolder() || IsFSDrivesFolder(); }
  bool IsDeviceDrivesPrefix() const { return _currentFolderPrefix == L"\\\\.\\"; }
  bool IsSuperDrivesPrefix() const { return _currentFolderPrefix == L"\\\\?\\"; }
  
  /*
    c:\Dir
    Computer\
    \\?\
  */
  bool IsFsOrPureDrivesFolder() const { return IsFSFolder() || (IsFSDrivesFolder() && !IsDeviceDrivesPrefix()); }

  /*
    c:\Dir
    Computer\
    \\?\
    \\SERVER\
  */
  bool IsFolder_with_FsItems() const
  {
    if (IsFsOrPureDrivesFolder())
      return true;
    #if defined(_WIN32) && !defined(UNDER_CE)
    FString prefix = us2fs(GetFsPath());
    return (prefix.Len() == NWindows::NFile::NName::GetNetworkServerPrefixSize(prefix));
    #else
    return false;
    #endif
  }

  UString GetFsPath() const;
  UString GetDriveOrNetworkPrefix() const;

  bool DoesItSupportOperations() const { return _folderOperations != NULL; }
  bool IsThereReadOnlyFolder() const;
  bool CheckBeforeUpdate(UINT resourceID);

  bool _processTimer;
  bool _processNotify;
  bool _processStatusBar;

  class CDisableTimerProcessing
  {
    CLASS_NO_COPY(CDisableTimerProcessing);

    bool _processTimer;

    CContentsView &_panel;
    
    public:

    CDisableTimerProcessing(CContentsView &panel) : _panel(panel) { Disable(); }
    ~CDisableTimerProcessing() { Restore(); }
    void Disable()
    {
      _processTimer = _panel._processTimer;
      _panel._processTimer = false;
    }
    void Restore()
    {
      _panel._processTimer = _processTimer;
    }
  };

  class CDisableNotify
  {
    CLASS_NO_COPY(CDisableNotify);

    bool _processNotify;
    bool _processStatusBar;

    CContentsView &_panel;

    public:

    CDisableNotify(CContentsView &panel) : _panel(panel) { Disable(); }
    ~CDisableNotify() { Restore(); }
    void Disable()
    {
      _processNotify = _panel._processNotify;
      _processStatusBar = _panel._processStatusBar;
      _panel._processNotify = false;
      _panel._processStatusBar = false;
    }
    void SetMemMode_Enable()
    {
      _processNotify = true;
      _processStatusBar = true;
    }
    void Restore()
    {
      _panel._processNotify = _processNotify;
      _panel._processStatusBar = _processStatusBar;
    }
  };

  // bool _passwordIsDefined;
  // UString _password;

  void InvalidateList() { this->InvalidateRect(NULL, TRUE); }

  HRESULT RefreshListCtrl();

  void MessageBoxMyError(LPCWSTR message);
  void MessageBoxError(HRESULT errorCode, LPCWSTR caption);
  void MessageBoxError(HRESULT errorCode);
  void MessageBoxError2Lines(LPCWSTR message, HRESULT errorCode);
  void MessageBoxLastError(LPCWSTR caption);
  void MessageBoxLastError();

  // void MessageBoxErrorForUpdate(HRESULT errorCode, UINT resourceID);

  void MessageBoxErrorLang(UINT resourceID);

  void OpenAltStreams();

  void OpenFocusedItemAsInternal(const wchar_t *type = NULL);
  void OpenSelectedItems(bool internal);

  void OpenFolderExternal(int index);

  void OpenFolder(int index);

  NWindows::NFile::NDir::CTempDir *m_pTempDir;
  HRESULT OpenParentArchiveFolder();
  
  HRESULT OpenAsArc(IInStream *inStream,
      const CTempFileInfo &tempFileInfo,
      const UString &virtualFilePath,
      const UString &arcFormat,
      bool &encrypted);

  HRESULT OpenAsArc_Msg(IInStream *inStream,
      const CTempFileInfo &tempFileInfo,
      const UString &virtualFilePath,
      const UString &arcFormat,
      bool &encrypted,
      bool showErrorMessage);
  
  HRESULT OpenAsArc_Name(const UString &relPath, const UString &arcFormat, bool &encrypted, bool showErrorMessage);
  HRESULT OpenAsArc_Index(int index, const wchar_t *type /* = NULL */, bool showErrorMessage);
  
  void OpenItemInArchive(int index, bool tryInternal, bool tryExternal,
      bool editMode, bool useEditor, const wchar_t *type = NULL);
  
  bool IsPreviewable(int index);
  CMyComPtr<IFolderFolder> ExtractItemToTemp(int index);
  HRESULT OnOpenItemChanged(UInt32 index, const wchar_t *fullFilePath, bool usePassword, const UString &password);
  LRESULT OnOpenItemChanged(LPARAM lParam);

  bool IsVirus_Message(const UString &name);
  void OpenItem(int index, bool tryInternal, bool tryExternal, const wchar_t *type = NULL);
  void EditItem(bool useEditor);
  void EditItem(int index, bool useEditor);

  void RenameFile();
  void ChangeComment();

  void SetListViewStyle(UInt32 index);
  UInt32 GetListViewStyle() const { return _ListViewStyle; }
  PROPID GetSortID() const { return _sortID; }

  bool GetFlatMode() const { return _flatMode; }
  // bool Get_ShowNtfsStrems_Mode() const { return _showNtfsStrems_Mode; }

  bool AutoRefresh_Mode;
  void Set_AutoRefresh_Mode(bool mode)
  {
    AutoRefresh_Mode = mode;
  }
  
  void Post_Refresh_StatusBar();

  void AddToArchive();

  void GetFilePaths(const CRecordVector<UInt32> &indices, UStringVector &paths, bool allowFolders = false);
  void ExtractArchives();
  void TestArchives();

  HRESULT CopyTo(CCopyToOptions &options,
      const CRecordVector<UInt32> &indices,
      UStringVector *messages,
      bool &usePassword, UString &password,
      bool showProgress = true);

  HRESULT CopyTo(CCopyToOptions &options,
    const CRecordVector<UInt32> &indices,
    UStringVector *messages)
  {
    bool usePassword = false;
    UString password;
    if (_parentFolders.Size() > 0)
    {
      const CFolderLink &fl = _parentFolders.Back();
      usePassword = fl.UsePassword;
      password = fl.Password;
    }
    return CopyTo(options, indices, messages, usePassword, password);
  }

  HRESULT CopyFrom(bool moveMode, const UString &folderPrefix, const UStringVector &filePaths,
      bool showErrorMessages, UStringVector *messages);

  void CopyFromNoAsk(const UStringVector &filePaths);
  void CopyFromAsk(const UStringVector &filePaths);

  // empty folderPath means create new Archive to path of first fileName.
  void DropObject(IDataObject * dataObject, const UString &folderPath);

  // empty folderPath means create new Archive to path of first fileName.
  void CompressDropFiles(const UStringVector &fileNames, const UString &folderPath);

  void RefreshTitle();

  UString GetItemsInfoString(const CRecordVector<UInt32> &indices);

protected:

  // Generated message map functions
protected:
  afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
  afx_msg void OnDestroy();
  afx_msg BOOL OnEraseBkgnd(CDC* pDC);
  afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
  afx_msg LRESULT OnNcHitTest(CPoint point);
  // PanelSort
  afx_msg void OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult);

  // PanelItems
  afx_msg void OnTimer(UINT_PTR nIDEvent);

  // PanelOperations
  afx_msg void OnLvnBeginlabeledit(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnLvnEndlabeledit(NMHDR *pNMHDR, LRESULT *pResult);

  // PanelListNotify
  afx_msg void OnLvnGetdispinfo(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult);

  afx_msg void OnLvnItemActivate(NMHDR *pNMHDR, LRESULT *pResult);

  afx_msg void OnLvnBegindrag(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult);

  afx_msg void OnNMClick(NMHDR *pNMHDR, LRESULT *pResult);

  DECLARE_MESSAGE_MAP()
};

class CMyBuffer
{
  void *_data;
public:
  CMyBuffer(): _data(0) {}
  operator void *() { return _data; }
  bool Allocate(size_t size)
  {
    if (_data != 0)
      return false;
    _data = ::MidAlloc(size);
    return _data != 0;
  }
  ~CMyBuffer() { ::MidFree(_data); }
};

class CExitEventLauncher
{
public:
  CManualResetEvent _exitEvent;
  bool _needExit;
  CRecordVector< HANDLE > _threads;
  unsigned _numActiveThreads;
    
  CExitEventLauncher()
  {
    _needExit = false;
    _needExit = true;
    _numActiveThreads = 0;
  };

  ~CExitEventLauncher() { Exit(true); }

  void Exit(bool hardExit);
};

extern CExitEventLauncher g_ExitEventLauncher;

