// ContentsView.cpp : implementation file
//

#include "stdafx.h"
#include "7zipProFM.h"
#include "ContentsView.h"
#include "Messages.h"

#include "C/CpuArch.h"

#include "CPP/Common/StringConvert.h"

#include "CPP/Windows/FileDir.h"
#include "CPP/Windows/FileName.h"
#include "CPP/Windows/PropVariant.h"
#include "CPP/Windows/PropVariantConv.h"

#include "CPP/7zip/UI/Common/ArchiveName.h"
#include "CPP/7zip/UI/Common/CompressCall.h"

#include "CopyDialog.h"
#include "CPP/7zip/UI/FileManager/FormatUtils.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/SysIconUtils.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"

#include "FMUtils.h"

using namespace NWindows;
using namespace NFile;
using namespace NDir;
using namespace NFind;
using namespace NName;

static CFSTR kTempDirPrefix = FTEXT("7zeE");


static const UINT_PTR kTimerID = 1;
static const UINT kTimerElapse = 1000;

static DWORD kStyles[4] = { LVS_SMALLICON, LVS_ICON, LVS_LIST, LVS_REPORT };

// CListViewInfo

#define Set32(p, v) SetUi32(((Byte *)p), v)
#define SetBool(p, v) Set32(p, ((v) ? 1 : 0))

#define Get32(p, dest) dest = GetUi32((const Byte *)p)
#define GetBool(p, dest) dest = (GetUi32(p) != 0);

static const UInt32 kListViewHeaderSize = 3 * 4;
static const UInt32 kColumnInfoSize = 3 * 4;
static const UInt32 kListViewVersion = 1;

void CListViewInfo::Save(const UString &id) const
{
  const UInt32 dataSize = kListViewHeaderSize + kColumnInfoSize * Columns.Size();
  CByteArr buf(dataSize);

  Set32(buf, kListViewVersion);
  Set32(buf + 4, SortID);
  SetBool(buf + 8, Ascending);
  FOR_VECTOR(i, Columns)
  {
    const CColumnInfo &column = Columns[i];
    Byte *p = buf + kListViewHeaderSize + i * kColumnInfoSize;
    Set32(p, column.PropID);
    SetBool(p + 4, column.IsVisible);
    Set32(p + 8, column.Width);
  }
  {
    CSettingsStoreSP regSP;
    CSettingsStore& reg = regSP.Create(FALSE, FALSE);
    if (reg.CreateKey(theApp.GetRegSectionPath(RK_COLUMNS))) {
      CRect rt;
      reg.Write(GetSystemString(id), buf, dataSize);
    }
  }
}

void CListViewInfo::Read(const UString &id)
{
  Clear();
  BYTE * buf = NULL;
  UInt32 size = 0;
  {
    CSettingsStoreSP regSP;
    CSettingsStore& reg = regSP.Create(FALSE, TRUE);
    if (reg.Open(theApp.GetRegSectionPath(RK_COLUMNS))) {
      if (!reg.Read(GetSystemString(id), &buf, &size))
        return;
    }
  }
  if (size >= kListViewHeaderSize) {
    UInt32 version;
    Get32(buf, version);
    if (version == kListViewVersion) {
      Get32(buf + 4, SortID);
      GetBool(buf + 8, Ascending);

      size -= kListViewHeaderSize;
      if (size % kColumnInfoSize == 0) {
        unsigned numItems = size / kColumnInfoSize;
        Columns.ClearAndReserve(numItems);
        for (unsigned i = 0; i < numItems; i++)
        {
          CColumnInfo column;
          const Byte *p = buf + kListViewHeaderSize + i * kColumnInfoSize;
          Get32(p, column.PropID);
          GetBool(p + 4, column.IsVisible);
          Get32(p + 8, column.Width);
          Columns.AddInReserved(column);
        }
      }
    }
  }
  delete[] buf;
}


// ContentsView

IMPLEMENT_DYNCREATE(CContentsView, CListCtrl)

CContentsView::CContentsView() :
m_pDetailView(NULL),
// _virtualMode(flase),
//       _exStyle(0),
_showDots(false),
_showRealFileIcons(false),
_needSaveInfo(false),
_startGroupSelect(0),
_selectionIsDefined(false),
_ListViewStyle(3),
_flatMode(false),
//       _flatModeForDisk(false),
_flatModeForArc(false),

// _showNtfsStrems_Mode(false),
// _showNtfsStrems_ModeForDisk(false),
// _showNtfsStrems_ModeForArc(false),

//       _xSize(300),
_mySelectMode(false),
_thereAreDeletedItems(false),
_markDeletedItems(true),
_enableItemChangeNotify(true),
_dontShowMode(false),
AutoRefresh_Mode(true),
m_pTempDir(NULL)
{
  m_bResizing = FALSE;

  _processTimer = true;
  _processNotify = true;
  _processStatusBar = true;
}

CContentsView::~CContentsView()
{
  CloseOpenFolders();
}


BOOL CContentsView::Create(LPCTSTR lpszClassName,
  LPCTSTR lpszWindowName, DWORD dwStyle,
  const RECT& rect,
  CWnd* pParentWnd, UINT nID,
  CCreateContext* pContext)
{
  // can't use for desktop or pop-up windows (use CreateEx instead)
  ASSERT(pParentWnd != NULL);
  ASSERT((dwStyle & WS_POPUP) == 0);

  if (((dwStyle & WS_TABSTOP) == WS_TABSTOP) && (nID == 0))
  {
    // Warn about nID == 0.  A zero ID will be overridden in CWnd::PreCreateWindow when the
    // check is done for (cs.hMenu == NULL).  This will cause the dialog control ID to be
    // different than passed in, so ::GetDlgItem(nID) will not return the control HWND.
    TRACE(traceAppMsg, 0, _T("Warning: creating a dialog control with nID == 0; ")
      _T("nID will overridden in CWnd::PreCreateWindow and GetDlgItem with nID == 0 will fail.\n"));
  }
  // initialize common controls
  // 	VERIFY(AfxDeferRegisterClass(AFX_WNDCOMMCTL_LISTVIEW_REG));

  // 	CWnd* pWnd = this;
  // 	return pWnd->Create(WC_LISTVIEW, NULL, dwStyle, rect, pParentWnd, nID);

  lpszClassName = WC_LISTVIEW;
  lpszWindowName = NULL;
  dwStyle |= WS_CHILD | WS_VISIBLE;
  return CWnd::CreateEx(0, lpszClassName, lpszWindowName,
    dwStyle | WS_CHILD,
    rect.left, rect.top,
    rect.right - rect.left, rect.bottom - rect.top,
    pParentWnd->GetSafeHwnd(), (HMENU)(UINT_PTR)nID, (LPVOID)pContext);
}


HRESULT CContentsView::Initialize(
  LPCTSTR currentFolderPrefix,
  LPCTSTR arcFormat,
  bool &archiveIsOpened, bool &encrypted)
{
  UString cfp = (currentFolderPrefix == NULL) ? _T("") : currentFolderPrefix;

  if (!currentFolderPrefix != NULL && _tcslen(currentFolderPrefix) != 0)
    if (currentFolderPrefix[0] == L'.')
    {
    FString cfpF;
    if (NFile::NDir::MyGetFullPathName(us2fs(currentFolderPrefix), cfpF))
      cfp = fs2us(cfpF);
    }

  UString strArcFormat = (arcFormat == NULL) ? _T("") : arcFormat;
  RINOK(BindToPath(cfp, strArcFormat, archiveIsOpened, encrypted));

  RefreshListCtrl();

  return S_OK;
}


BOOL CContentsView::PreCreateWindow(CREATESTRUCT& cs)
{
  DWORD style = cs.style; //  | WS_BORDER ; // | LVS_SHAREIMAGELISTS; //  | LVS_SHOWSELALWAYS;;

  style |= LVS_SHAREIMAGELISTS;
  // style  |= LVS_AUTOARRANGE;
  style |= WS_CLIPCHILDREN;
  style |= WS_CLIPSIBLINGS;

  const UInt32 kNumListModes = ARRAY_SIZE(kStyles);
  if (_ListViewStyle >= kNumListModes)
    _ListViewStyle = kNumListModes - 1;

  style |= kStyles[_ListViewStyle]
    | WS_TABSTOP
    | LVS_EDITLABELS
    | LVS_SHOWSELALWAYS;
  if (_mySelectMode)
    style |= LVS_SINGLESEL;

  /*
  if (_virtualMode)
  style |= LVS_OWNERDATA;
  */

  DWORD exStyle = cs.dwExStyle;
  exStyle |= WS_EX_CLIENTEDGE;

  cs.style = style;
  cs.dwExStyle = exStyle;

  return CListCtrl::PreCreateWindow(cs);
}


BOOL CContentsView::OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
  switch (message)
  {
  case kReLoadMessage:
    RefreshListCtrl(_selectedState);
    *pResult = 0;
    break;
  case kOpenItemChanged:
    *pResult = OnOpenItemChanged(lParam);
    return TRUE;
  default:
    break;
  }
  return CListCtrl::OnWndMsg(message, wParam, lParam, pResult);
}


BEGIN_MESSAGE_MAP(CContentsView, CListCtrl)
  ON_WM_CREATE()
  ON_WM_DESTROY()
  ON_WM_ERASEBKGND()
  ON_WM_CONTEXTMENU()
  // 	ON_WM_NCHITTEST()
  ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, &CContentsView::OnLvnColumnclick)
  ON_WM_TIMER()
  ON_NOTIFY_REFLECT(LVN_GETDISPINFO, &CContentsView::OnLvnGetdispinfo)
  ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, &CContentsView::OnLvnItemchanged)
  ON_NOTIFY_REFLECT(LVN_ITEMACTIVATE, &CContentsView::OnLvnItemActivate)
  ON_NOTIFY_REFLECT(LVN_BEGINDRAG, &CContentsView::OnLvnBegindrag)
  ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, &CContentsView::OnNMCustomdraw)

  ON_NOTIFY_REFLECT(NM_CLICK, &CContentsView::OnNMClick)
  ON_NOTIFY_REFLECT(LVN_BEGINLABELEDIT, &CContentsView::OnLvnBeginlabeledit)
  ON_NOTIFY_REFLECT(LVN_ENDLABELEDIT, &CContentsView::OnLvnEndlabeledit)
END_MESSAGE_MAP()


using namespace NWindows;

// ContentsView message handlers


int CContentsView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
  if (CListCtrl::OnCreate(lpCreateStruct) == -1)
    return -1;

  ReloadSettings();

  // _virtualMode = false;
  // _sortIndex = 0;
  _sortID = kpidName;
  _ascending = true;

  ListView_SetUnicodeFormat(*this, TRUE);

  HIMAGELIST hImg = GetSysImageList(true);
  CImageList *pImgList = CImageList::FromHandlePermanent(hImg);
  if (pImgList == NULL) {
    CImageList img;
    img.Attach(hImg);
    this->SetImageList(&img, LVSIL_SMALL);
    img.Detach();
  }
  else
    this->SetImageList(pImgList, LVSIL_SMALL);
  hImg = GetSysImageList(false);
  pImgList = CImageList::FromHandlePermanent(hImg);
  if (pImgList == NULL) {
    CImageList img;
    img.Attach(hImg);
    this->SetImageList(&img, LVSIL_NORMAL);
    img.Detach();
  }
  else
    this->SetImageList(pImgList, LVSIL_NORMAL);

  CWnd *pParent = GetTopLevelParent();
  _dropTargetSpec = new CDropTarget(pParent);
  _dropTarget = _dropTargetSpec;
  // ::DragAcceptFiles(hWnd, TRUE);
  RegisterDragDrop(*pParent, _dropTarget);

  // _exStyle |= LVS_EX_HEADERDRAGDROP;
  // DWORD extendedStyle = _listView.GetExtendedListViewStyle();
  // extendedStyle |= _exStyle;
  //  _listView.SetExtendedListViewStyle(extendedStyle);
  // this->SetExtendedStyle(_exStyle);

  SetTimer(kTimerID, kTimerElapse, NULL);

  return 0;
}


void CContentsView::ReloadSettings()
{
  _mySelectMode = ReadAlternativeSelection();
  _showDots = ReadShowDots();
  _showRealFileIcons = ReadShowRealFileIcons();
  // 	bool mySelectionMode = ReadAlternativeSelection();
  // 	DWORD style = (DWORD)this->GetStyle();
  // 	if (mySelectionMode)
  // 		style |= LVS_SINGLESEL;
  // 	else
  // 		style &= ~LVS_SINGLESEL;
  // 	this->ModifyStyle(mySelectionMode ? 0 : LVS_SINGLESEL,
  // 		mySelectionMode ? LVS_SINGLESEL : 1);

  DWORD extendedStyle = LVS_EX_HEADERDRAGDROP;
  if (ReadFullRow())
    extendedStyle |= LVS_EX_FULLROWSELECT;
  if (ReadShowGrid())
    extendedStyle |= LVS_EX_GRIDLINES;
  if (ReadSingleClick())
  {
    extendedStyle |= LVS_EX_ONECLICKACTIVATE | LVS_EX_TRACKSELECT;
    /*
    if (ReadUnderline())
    extendedStyle |= LVS_EX_UNDERLINEHOT;
    */
  }
  this->SetExtendedStyle(extendedStyle);
}


void CContentsView::OnDestroy()
{
  CListCtrl::OnDestroy();

  // It's for unloading COM dll's: don't change it.
  CloseOpenFolders();
  _sevenZipContextMenu.Release();
  _systemContextMenu.Release();
}


BOOL CContentsView::OnEraseBkgnd(CDC* pDC)
{
  CRect rt;
  GetClientRect(&rt);
  if (m_bResizing) {
    pDC->ExcludeClipRect(&m_rtPrev);
    m_rtPrev = rt;
  }
  return CListCtrl::OnEraseBkgnd(pDC);
}


LRESULT CContentsView::OnNcHitTest(CPoint point)
{
  if (GetFocus() != this)
    SetFocus();

  return CListCtrl::OnNcHitTest(point);
}


BOOL CContentsView::GetItemParam(int index, LPARAM &param) const
{
  LVITEM item;
  item.iItem = index;
  item.iSubItem = 0;
  item.mask = LVIF_PARAM;
  BOOL aResult = GetItem(&item);
  param = item.lParam;
  return aResult;
}

static void AddSizeValue(UString &s, UInt64 size)
{
  s += MyFormatNew(IDS_FILE_SIZE, ConvertSizeToString(size));
}

static void AddValuePair1(UString &s, UINT resourceID, UInt64 size)
{
  AddLangString(s, resourceID);
  s += L": ";
  AddSizeValue(s, size);
  s.Add_LF();
}

void AddValuePair2(UString &s, UINT resourceID, UInt64 num, UInt64 size)
{
  if (num == 0)
    return;
  AddLangString(s, resourceID);
  s += L": ";
  s += ConvertIntToDecimalString(num);

  if (size != (UInt64)(Int64)-1)
  {
    s += L"    ( ";
    AddSizeValue(s, size);
    s += L" )";
  }
  s.Add_LF();
}

static void AddPropValueToSum(IFolderFolder *folder, int index, PROPID propID, UInt64 &sum)
{
  if (sum == (UInt64)(Int64)-1)
    return;
  NCOM::CPropVariant prop;
  folder->GetProperty(index, propID, &prop);
  UInt64 val = 0;
  if (ConvertPropVariantToUInt64(prop, val))
    sum += val;
  else
    sum = (UInt64)(Int64)-1;
}

UString CContentsView::GetItemsInfoString(const CRecordVector<UInt32> &indices)
{
  UString info;
  UInt64 numDirs, numFiles, filesSize, foldersSize;
  numDirs = numFiles = filesSize = foldersSize = 0;
  unsigned i;
  for (i = 0; i < indices.Size(); i++)
  {
    int index = indices[i];
    if (IsItem_Folder(index))
    {
      AddPropValueToSum(_folder, index, kpidSize, foldersSize);
      numDirs++;
    }
    else
    {
      AddPropValueToSum(_folder, index, kpidSize, filesSize);
      numFiles++;
    }
  }

  AddValuePair2(info, IDS_PROP_FOLDERS, numDirs, foldersSize);
  AddValuePair2(info, IDS_PROP_FILES, numFiles, filesSize);
  int numDefined = ((foldersSize != (UInt64)(Int64)-1) && foldersSize != 0) ? 1 : 0;
  numDefined += ((filesSize != (UInt64)(Int64)-1) && filesSize != 0) ? 1 : 0;
  if (numDefined == 2)
    AddValuePair1(info, IDS_PROP_SIZE, filesSize + foldersSize);

  info.Add_LF();
  info += _currentFolderPrefix;

  for (i = 0; i < indices.Size() && (int)i < (int)kCopyDialog_NumInfoLines - 6; i++)
  {
    info += L"\n  ";
    int index = indices[i];
    info += GetItemRelPath(index);
    if (IsItem_Folder(index))
      info.Add_PathSepar();
  }
  if (i != indices.Size())
    info += L"\n  ...";
  return info;
}

void CContentsView::OnCopy(bool move, bool copyToSame)
{
  CContentsView &srcPanel = *this;
  CContentsView &destPanel = *this;

  CDisableTimerProcessing disableTimerProcessing1(destPanel);
  CDisableTimerProcessing disableTimerProcessing2(srcPanel);

  if (move)
  {
    if (!srcPanel.CheckBeforeUpdate(IDS_MOVE))
      return;
  }
  else if (!srcPanel.DoesItSupportOperations())
  {
    srcPanel.MessageBoxErrorLang(IDS_OPERATION_IS_NOT_SUPPORTED);
    return;
  }

  CRecordVector<UInt32> indices;
  UString destPath;
  bool useDestPanel = false;

  {
    if (copyToSame)
    {
      int focusedItem = srcPanel.GetFocusedItem();
      if (focusedItem < 0)
        return;
      int realIndex = srcPanel.GetRealItemIndex(focusedItem);
      if (realIndex == kParentIndex)
        return;
      indices.Add(realIndex);
      destPath = srcPanel.GetItemName(realIndex);
    }
    else
    {
      srcPanel.GetOperatedIndicesSmart(indices);
      if (indices.Size() == 0)
        return;
      destPath = destPanel.GetFsPath();
      ReducePathToRealFileSystemPath(destPath);
    }
  }

  UStringVector copyFolders;
  //   ReadCopyHistory(copyFolders);

  {
    CCopyDialog copyDialog;

    copyDialog.Strings = copyFolders;
    copyDialog.Value = destPath;
    LangString(move ? IDS_MOVE : IDS_COPY, copyDialog.Title);
    LangString(move ? IDS_MOVE_TO : IDS_COPY_TO, copyDialog.Static);
    copyDialog.Info = srcPanel.GetItemsInfoString(indices);

    if (copyDialog.DoModal() != IDOK)
      return;

    destPath = copyDialog.Value;
  }

  {
    if (destPath.IsEmpty())
    {
      srcPanel.MessageBoxErrorLang(IDS_OPERATION_IS_NOT_SUPPORTED);
      return;
    }

    UString correctName;
    if (!srcPanel.CorrectFsPath(destPath, correctName))
    {
      srcPanel.MessageBoxError(E_INVALIDARG);
      return;
    }

    if (IsAbsolutePath(destPath))
      destPath.Empty();
    else
      destPath = srcPanel.GetFsPath();
    destPath += correctName;

#if defined(_WIN32) && !defined(UNDER_CE)
    if (destPath.Len() > 0 && destPath[0] == '\\')
      if (destPath.Len() == 1 || destPath[1] != '\\')
      {
        srcPanel.MessageBoxErrorLang(IDS_OPERATION_IS_NOT_SUPPORTED);
        return;
      }
#endif

    if (CompareFileNames(destPath, destPanel.GetFsPath()) == 0)
    {
      srcPanel.MessageBoxMyError(L"Can not copy files onto itself");
      return;
    }

    bool destIsFsPath = false;

    if (IsAltPathPrefix(us2fs(destPath)))
    {
      // we allow alt streams dest only to alt stream folder in second panel
      srcPanel.MessageBoxErrorLang(IDS_OPERATION_IS_NOT_SUPPORTED);
      return;
      /*
      FString basePath = us2fs(destPath);
      basePath.DeleteBack();
      if (!DoesFileOrDirExist(basePath))
      {
      srcPanel.MessageBoxError2Lines(basePath, ERROR_FILE_NOT_FOUND); // GetLastError()
      return;
      }
      destIsFsPath = true;
      */
    }
    else
    {
      if (indices.Size() == 1 &&
        !destPath.IsEmpty() && destPath.Back() != WCHAR_PATH_SEPARATOR)
      {
        int pos = destPath.ReverseFind_PathSepar();
        if (pos < 0)
        {
          srcPanel.MessageBoxErrorLang(IDS_OPERATION_IS_NOT_SUPPORTED);
          return;
        }
        {
          /*
          #ifdef _WIN32
          UString name = destPath.Ptr(pos + 1);
          if (name.Find(L':') >= 0)
          {
          srcPanel.MessageBoxErrorLang(IDS_OPERATION_IS_NOT_SUPPORTED);
          return;
          }
          #endif
          */
          UString prefix = destPath.Left(pos + 1);
          if (!CreateComplexDir(us2fs(prefix)))
          {
            srcPanel.MessageBoxError2Lines(prefix, GetLastError());
            return;
          }
        }
        // bool isFolder = srcPanael.IsItem_Folder(indices[0]);
      }
      else
      {
        NName::NormalizeDirPathPrefix(destPath);
        if (!CreateComplexDir(us2fs(destPath)))
        {
          srcPanel.MessageBoxError2Lines(destPath, GetLastError());
          return;
        }
      }
      destIsFsPath = true;
    }

    if (!destIsFsPath)
      useDestPanel = true;

    //     AddUniqueStringToHeadOfList(copyFolders, destPath);
    while (copyFolders.Size() > 20)
      copyFolders.DeleteBack();
    //     SaveCopyHistory(copyFolders);
  }

  bool useSrcPanel = !useDestPanel || !srcPanel.Is_IO_FS_Folder();

  bool useTemp = useSrcPanel && useDestPanel;
  if (useTemp)
  {
    srcPanel.MessageBoxErrorLang(IDS_OPERATION_IS_NOT_SUPPORTED);
    return;
  }

  CTempDir tempDirectory;
  FString tempDirPrefix;
  if (useTemp)
  {
    tempDirectory.Create(kTempDirPrefix);
    tempDirPrefix = tempDirectory.GetPath();
    NFile::NName::NormalizeDirPathPrefix(tempDirPrefix);
  }

  CSelectedState srcSelState;
  CSelectedState destSelState;
  srcPanel.SaveSelectedState(srcSelState);
  destPanel.SaveSelectedState(destSelState);

  CDisableNotify disableNotify1(destPanel);
  CDisableNotify disableNotify2(srcPanel);

  HRESULT result = S_OK;

  if (useSrcPanel)
  {
    CCopyToOptions options;
    options.folder = useTemp ? fs2us(tempDirPrefix) : destPath;
    options.moveMode = move;
    options.includeAltStreams = true;
    options.replaceAltStreamChars = false;
    options.showErrorMessages = true;

    result = srcPanel.CopyTo(options, indices, NULL);
  }

  if (result == S_OK && useDestPanel)
  {
    UStringVector filePaths;
    UString folderPrefix;
    if (useTemp)
      folderPrefix = fs2us(tempDirPrefix);
    else
      folderPrefix = srcPanel.GetFsPath();
    filePaths.ClearAndReserve(indices.Size());
    FOR_VECTOR(i, indices)
      filePaths.AddInReserved(srcPanel.GetItemRelPath2(indices[i]));
    result = destPanel.CopyFrom(move, folderPrefix, filePaths, true, 0);
  }

  if (result != S_OK)
  {
    // disableNotify1.Restore();
    // disableNotify2.Restore();
    // For Password:
    // srcPanel.SetFocusToList();
    // srcPanel.InvalidateList(NULL, true);

    if (result != E_ABORT)
      srcPanel.MessageBoxError(result, L"Error");
    // return;
  }

  RefreshTitle();

  if (copyToSame || move)
  {
    srcPanel.RefreshListCtrl(srcSelState);
  }

  if (!copyToSame)
  {
    destPanel.RefreshListCtrl(destSelState);
    srcPanel.KillSelection();
  }

  disableNotify1.Restore();
  disableNotify2.Restore();
  srcPanel.SetFocus();
}

void CContentsView::RefreshTitle()
{
  UString path = _currentFolderPrefix;
  if (path.IsEmpty())
    path = LangString(IDR_MAINFRAME);
  CWnd * pWnd = GetTopLevelParent();
  if (pWnd != NULL)
    ::SetWindowTextW(pWnd->m_hWnd, path);
}

void CContentsView::MessageBoxMyError(LPCWSTR message)
{
  MessageBox(message, L"7-ZipPro");
}

void CContentsView::MessageBoxError(HRESULT errorCode, LPCWSTR caption)
{
  MessageBox(HResultToMessage(errorCode), caption);
}

void CContentsView::MessageBoxError2Lines(LPCWSTR message, HRESULT errorCode)
{
  UString m = message;
  if (errorCode != 0)
  {
    m.Add_LF();
    m += HResultToMessage(errorCode);
  }
  MessageBoxMyError(m);
}

void CContentsView::MessageBoxError(HRESULT errorCode)
{
  MessageBoxError(errorCode, L"7-Zip");
}
void CContentsView::MessageBoxLastError(LPCWSTR caption)
{
  MessageBoxError(::GetLastError(), caption);
}
void CContentsView::MessageBoxLastError()
{
  MessageBoxLastError(L"7-Zip");
}

void CContentsView::MessageBoxErrorLang(UINT resourceID)
{
  MessageBox(LangString(resourceID));
}

UString CContentsView::GetFolderTypeID() const
{
  if (_folder != NULL) {
    NCOM::CPropVariant prop;
    if (_folder->GetFolderProperty(kpidType, &prop) == S_OK)
      if (prop.vt == VT_BSTR)
        return (const wchar_t *)prop.bstrVal;
  }
  return UString();
}

bool CContentsView::IsFolderTypeEqTo(const char *s) const
{
  return StringsAreEqual_Ascii(GetFolderTypeID(), s);
}

bool CContentsView::IsRootFolder() const { return IsFolderTypeEqTo("RootFolder"); }
bool CContentsView::IsFSFolder() const { return IsFolderTypeEqTo("FSFolder"); }
bool CContentsView::IsFSDrivesFolder() const { return IsFolderTypeEqTo("FSDrives"); }
bool CContentsView::IsAltStreamsFolder() const { return IsFolderTypeEqTo("AltStreamsFolder"); }
bool CContentsView::IsArcFolder() const
{
  return GetFolderTypeID().IsPrefixedBy_Ascii_NoCase("7-Zip");
}

UString CContentsView::GetFsPath() const
{
  if (IsFSDrivesFolder() && !IsDeviceDrivesPrefix() && !IsSuperDrivesPrefix())
    return UString();
  return _currentFolderPrefix;
}

UString CContentsView::GetDriveOrNetworkPrefix() const
{
  if (!IsFSFolder())
    return UString();
  UString drive = GetFsPath();
  drive.DeleteFrom(NFile::NName::GetRootPrefixSize(drive));
  return drive;
}

void CContentsView::SetListViewStyle(UInt32 index)
{
  if (index >= 4)
    return;
  _ListViewStyle = index;
  DWORD oldStyle = (DWORD)this->GetStyle();
  DWORD newStyle = kStyles[index];

  // DWORD tickCount1 = GetTickCount();
  if ((oldStyle & LVS_TYPEMASK) != newStyle)
    SetWindowLongPtr(*this, GWL_STYLE, (oldStyle & ~LVS_TYPEMASK) | newStyle);
  // RefreshListCtrlSaveFocused();
  /*
  DWORD tickCount2 = GetTickCount();
  char s[256];
  sprintf(s, "SetStyle = %5d",
  tickCount2 - tickCount1
  );
  OutputDebugStringA(s);
  */

}

void CContentsView::Post_Refresh_StatusBar()
{
  if (_processStatusBar)
    GetTopLevelParent()->PostMessage(kRefresh_StatusBar);
}

void CContentsView::AddToArchive()
{
  CRecordVector<UInt32> indices;
  GetOperatedItemIndices(indices);
  if (!Is_IO_FS_Folder())
  {
    MessageBoxErrorLang(IDS_OPERATION_IS_NOT_SUPPORTED);
    return;
  }
  if (indices.Size() == 0)
  {
    MessageBoxErrorLang(IDS_SELECT_FILES);
    return;
  }
  UStringVector names;

  const UString curPrefix = GetFsPath();
  UString destCurDirPrefix = curPrefix;
  if (IsFSDrivesFolder())
    destCurDirPrefix = ROOT_FS_FOLDER;

  FOR_VECTOR (i, indices)
    names.Add(curPrefix + GetItemRelPath2(indices[i]));
  bool fromPrev = (names.Size() > 1);
  const UString arcName = CreateArchiveName(names.Front(), fromPrev, false);
  HRESULT res = CompressFiles(destCurDirPrefix, arcName, L"",
      true, // addExtension
      names, false, true, false);
  if (res != S_OK)
  {
    if (destCurDirPrefix.Len() >= MAX_PATH)
      MessageBoxErrorLang(IDS_MESSAGE_UNSUPPORTED_OPERATION_FOR_LONG_PATH_FOLDER);
  }
  // KillSelection();
}

static UString GetSubFolderNameForExtract(const UString &arcPath)
{
  UString s = arcPath;
  int slashPos = s.ReverseFind_PathSepar();
  int dotPos = s.ReverseFind_Dot();
  if (dotPos <= slashPos + 1)
    s += L'~';
  else
  {
    s.DeleteFrom(dotPos);
    s.TrimRight();
  }
  return s;
}

void CContentsView::GetFilePaths(const CRecordVector<UInt32> &indices, UStringVector &paths, bool allowFolders)
{
  const UString prefix = GetFsPath();
  FOR_VECTOR (i, indices)
  {
    int index = indices[i];
    if (!allowFolders && IsItem_Folder(index))
    {
      paths.Clear();
      break;
    }
    paths.Add(prefix + GetItemRelPath2(index));
  }
  if (paths.Size() == 0)
  {
    MessageBoxErrorLang(IDS_SELECT_FILES);
    return;
  }
}

void CContentsView::ExtractArchives()
{
  if (_parentFolders.Size() > 0)
  {
    OnCopy(false, false);
    return;
  }
  CRecordVector<UInt32> indices;
  GetOperatedItemIndices(indices);
  UStringVector paths;
  GetFilePaths(indices, paths);
  if (paths.IsEmpty())
    return;

  UString outFolder = GetFsPath();
  if (indices.Size() == 1)
    outFolder += GetSubFolderNameForExtract(GetItemRelPath(indices[0]));
  else
    outFolder += L'*';
  outFolder.Add_PathSepar();

  ::ExtractArchives(paths, outFolder
      , true // showDialog
      , false // elimDup
      );
}


void CContentsView::TestArchives()
{
  CRecordVector<UInt32> indices;
  GetOperatedIndicesSmart(indices);
  CMyComPtr<IArchiveFolder> archiveFolder;
  _folder.QueryInterface(IID_IArchiveFolder, &archiveFolder);
  if (archiveFolder)
  {
    CCopyToOptions options;
    options.streamMode = true;
    options.showErrorMessages = true;
    options.testMode = true;

    UStringVector messages;
    HRESULT res = CopyTo(options, indices, &messages);
    if (res != S_OK)
    {
      if (res != E_ABORT)
        MessageBoxError(res);
    }
    return;

    /*
    {
    CThreadTest extracter;

    extracter.ArchiveFolder = archiveFolder;
    extracter.ExtractCallbackSpec = new CExtractCallbackImp;
    extracter.ExtractCallback = extracter.ExtractCallbackSpec;
    extracter.ExtractCallbackSpec->ProgressDialog = &extracter.ProgressDialog;
    if (!_parentFolders.IsEmpty())
    {
      const CFolderLink &fl = _parentFolders.Back();
      extracter.ExtractCallbackSpec->PasswordIsDefined = fl.UsePassword;
      extracter.ExtractCallbackSpec->Password = fl.Password;
    }

    if (indices.IsEmpty())
      return;

    extracter.Indices = indices;

    UString title = LangString(IDS_PROGRESS_TESTING);
    UString progressWindowTitle = L"7-Zip"; // LangString(IDS_APP_TITLE);

    extracter.ProgressDialog.CompressingMode = false;
    extracter.ProgressDialog.MainWindow = GetParent();
    extracter.ProgressDialog.MainTitle = progressWindowTitle;
    extracter.ProgressDialog.MainAddTitle = title + L' ';

    extracter.ExtractCallbackSpec->OverwriteMode = NExtract::NOverwriteMode::kAskBefore;
    extracter.ExtractCallbackSpec->Init();

    if (extracter.Create(title, GetParent()) != S_OK)
      return;

    }
    RefreshTitleAlways();
    return;
    */
  }

  if (!IsFSFolder())
  {
    MessageBoxErrorLang(IDS_OPERATION_IS_NOT_SUPPORTED);
    return;
  }
  UStringVector paths;
  GetFilePaths(indices, paths, true);
  if (paths.IsEmpty())
    return;
  ::TestArchives(paths);
}
