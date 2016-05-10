// FolderView.cpp : implementation file
//

#include "stdafx.h"

#include "CPP/Common/DynamicBuffer.h"
#include "CPP/Common/IntToString.h"
#include "CPP/Common/StringConvert.h"
#include "CPP/Common/Wildcard.h"

#include "CPP/7zip/PropID.h"

#include "CPP/Windows/FileDir.h"
#include "CPP/Windows/FileFind.h"
#include "CPP/Windows/FileName.h"
#include "CPP/Windows/Menu.h"
#include "CPP/Windows/PropVariant.h"

#include "CPP/7zip/UI/Common/PropIDUtils.h"
#include "ComboDialog.h"
#include "CopyDialog.h"
#include "CPP/7zip/UI/FileManager/FormatUtils.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/PropertyName.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"

#include "RootFolder.h"
#include "FMUtils.h"
#include "ThreadFolderOperations.h"

#include "7zipProFM.h"
#include "FolderView.h"
#include "Messages.h"

// CFolderView

IMPLEMENT_DYNCREATE(CFolderView, CTreeCtrl)

CFolderView::CFolderView()
{
  hComputer = NULL;
  hNetwork = NULL;
  m_bTracking = FALSE;
}


CFolderView::~CFolderView()
{
}


BOOL CFolderView::Create(LPCTSTR lpszClassName,
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
  // 	VERIFY(AfxDeferRegisterClass(AFX_WNDCOMMCTL_TREEVIEW_REG));

  // 	CWnd* pWnd = this;
  // 	return pWnd->Create(WC_TREEVIEW, NULL, dwStyle, rect, pParentWnd, nID);

  lpszClassName = WC_TREEVIEW;
  lpszWindowName = NULL;
  dwStyle |= WS_CHILD | WS_VISIBLE;
  return CWnd::CreateEx(0, lpszClassName, lpszWindowName,
    dwStyle | WS_CHILD,
    rect.left, rect.top,
    rect.right - rect.left, rect.bottom - rect.top,
    pParentWnd->GetSafeHwnd(), (HMENU)(UINT_PTR)nID, (LPVOID)pContext);
}


BOOL CFolderView::PreCreateWindow(CREATESTRUCT& cs)
{
  cs.style |= TVS_HASBUTTONS;
  cs.style |= TVS_LINESATROOT;
  cs.style |= TVS_EDITLABELS;
  cs.style |= TVS_SHOWSELALWAYS;
  cs.style |= TVS_TRACKSELECT;
  cs.style |= TVS_FULLROWSELECT;
  cs.style |= TVS_NONEVENHEIGHT;

  return CTreeCtrl::PreCreateWindow(cs);
}


BOOL CFolderView::PreTranslateMessage(MSG* pMsg)
{
  if (pMsg->message == WM_KEYDOWN) {
    if (GetEditControl() != NULL) {
      if (pMsg->wParam == VK_DELETE || pMsg->wParam == VK_BACK) {
        ::TranslateMessage(pMsg);
        ::DispatchMessage(pMsg);
        return TRUE;
      }
    }
  }
  return CTreeCtrl::PreTranslateMessage(pMsg);
}


BEGIN_MESSAGE_MAP(CFolderView, CTreeCtrl)
  ON_WM_CREATE()
  // 	ON_WM_NCHITTEST()
  ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING, &CFolderView::OnTvnItemexpanding)
  ON_NOTIFY_REFLECT(NM_CLICK, &CFolderView::OnNMClick)
  ON_NOTIFY_REFLECT(NM_RETURN, &CFolderView::OnNMReturn)
  ON_WM_CONTEXTMENU()
  ON_NOTIFY_REFLECT(NM_RCLICK, &CFolderView::OnNMRClick)
  ON_NOTIFY_REFLECT(TVN_ENDLABELEDIT, &CFolderView::OnTvnEndlabeledit)
END_MESSAGE_MAP()


// CFolderView message handlers

int CFolderView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
  if (CTreeCtrl::OnCreate(lpCreateStruct) == -1)
    return -1;

  DWORD dwExStyle = 0;
  dwExStyle |= TVS_EX_NOSINGLECOLLAPSE;
  dwExStyle |= TVS_EX_DOUBLEBUFFER;
  dwExStyle |= TVS_EX_RICHTOOLTIP;
  dwExStyle |= TVS_EX_FADEINOUTEXPANDOS;
  dwExStyle |= TVS_EX_DRAWIMAGEASYNC;
  SetExtendedStyle(dwExStyle, dwExStyle);

  HIMAGELIST hImg = GetSysImageList(true);
  CImageList *pImgList = CImageList::FromHandlePermanent(hImg);
  if (pImgList == NULL) {
    CImageList img;
    img.Attach(hImg);
    this->SetImageList(&img, LVSIL_NORMAL);
    img.Detach();
  }
  else
    this->SetImageList(pImgList, LVSIL_NORMAL);

  this->SetIndent(10);

  FillRootItems();

  return 0;
}


LRESULT CFolderView::OnNcHitTest(CPoint point)
{
  if (GetFocus() != this)
    SetFocus();

  return CTreeCtrl::OnNcHitTest(point);
}

void CFolderView::FillRootItems()
{
  unsigned i;
  int iconIndex;
  UString name;

  HTREEITEM hDesktop, hItem;

  DeleteAllItems();

  // Desktop
  iconIndex = GetIconIndexForCSIDL(CSIDL_DESKTOP);
  name = LangString(IDS_DESKTOP);
  hDesktop = InsertItem(GetSystemString(name), iconIndex, iconIndex);
  SetItemData(hDesktop, (DWORD_PTR)TVI_TYPE_TOP);

  // This PC
  name = RootFolder_GetName_Computer(iconIndex);
  hComputer = InsertItem(GetSystemString(name), iconIndex, iconIndex, hDesktop);
  SetItemData(hComputer, (DWORD_PTR)TVI_TYPE_COMPUTER);

  // Desktop Directory
  name = RootFolder_GetName_Desktop(iconIndex);
  hItem = InsertItem(GetSystemString(name), iconIndex, iconIndex, hComputer);
  SetItemData(hItem, (DWORD_PTR)TVI_TYPE_DESKTOP);
  AddDumySingleChild(hItem);

  // Documents
  name = RootFolder_GetName_Documents(iconIndex);
  hItem = InsertItem(GetSystemString(name), iconIndex, iconIndex, hComputer);
  SetItemData(hItem, (DWORD_PTR)TVI_TYPE_DOCUMENTS);
  AddDumySingleChild(hItem);

  FStringVector driveStrings;
  NWindows::NFile::NFind::MyGetLogicalDriveStrings(driveStrings);
  for (i = 0; i < driveStrings.Size(); i++)
  {
    FString s = driveStrings[i];
    int iconIndex = GetRealIconIndex(s, 0);
    if (s.Len() > 0 && s.Back() == FCHAR_PATH_SEPARATOR)
      s.DeleteBack();

    FString volName;
    GetVolumeInformation(s + FCHAR_PATH_SEPARATOR,
      volName.GetBuf(MAX_PATH), MAX_PATH,
      NULL, NULL, NULL, NULL, 0);
    volName.ReleaseBuf_CalcLen(MAX_PATH);
    if (volName.IsEmpty()) {
      switch (GetDriveType(s))
      {
      case DRIVE_FIXED:
        volName = _T("Local Disk");
        break;
      case DRIVE_REMOVABLE:
        volName = _T("Removable Disk");
        break;
      case DRIVE_CDROM:
        volName = _T("CD Drive");
        break;
      case DRIVE_REMOTE:
        volName = _T("Network Drive");
        break;
      case DRIVE_RAMDISK:
        volName = _T("Ram Disk");
        break;
      default:
        break;
      }
    }
    if (!volName.IsEmpty())
      s = volName + _T(" (") + s + _T(")");

    hItem = InsertItem(s, iconIndex, iconIndex, hComputer);
    SetItemData(hItem, (DWORD_PTR)TVI_TYPE_DRIVE);
    AddDumySingleChild(hItem);
  }

  name = RootFolder_GetName_Network(iconIndex);
  hNetwork = InsertItem(GetSystemString(name), iconIndex, iconIndex, hDesktop);
  SetItemData(hNetwork, (DWORD_PTR)TVI_TYPE_NETWORK);
  AddDumySingleChild(hNetwork);

  Expand(hDesktop, TVE_EXPAND);
  Expand(hComputer, TVE_EXPAND);

#if 0
  for (i = 0; i < 2000; i++) {
    CString str;
    str.Format(_T("%d"), i);
    iconIndex = GetIconIndexForCSIDL(i);
    if (iconIndex < 0)
      break;
    InsertItem(str, iconIndex, iconIndex, hDesktop);
  }
#endif
}


struct TVI_PAIR {
  HTREEITEM hItem;
  INT nType;
};


CMyComPtr<IFolderFolder> CFolderView::GetFolder(HTREEITEM hItem)
{
  CObjectVector<TVI_PAIR> pathChain;
  while (hItem != NULL && hItem != TVI_ROOT) {
    INT nType;
    nType = (INT)GetItemData(hItem);
    if (nType == TVI_TYPE_TOP)
      break;
    TVI_PAIR pair = { hItem, nType };
    pathChain.Insert(0, pair);
    hItem = GetParentItem(hItem);
    if (nType == TVI_TYPE_DESKTOP || nType == TVI_TYPE_DOCUMENTS)
      break;
  }

  CMyComPtr<IFolderFolder> _folder;

  CRootFolder *rootFolderSpec = new CRootFolder;
  _folder = rootFolderSpec;
  rootFolderSpec->Init();

  CMyComPtr<IFolderFolder> newFolder;

  FOR_VECTOR(i, pathChain)
  {
    const TVI_PAIR &pair = pathChain[i];
    UString s = GetUnicodeString(GetItemText(pair.hItem));
    if (pair.nType == TVI_TYPE_DRIVE)
      s = ExtractDrivePath(s);
    else if (s.Back() == WCHAR_PATH_SEPARATOR)
      s.DeleteBack();
    // 		if (s.Back() != WCHAR_PATH_SEPARATOR)
    // 			s += WCHAR_PATH_SEPARATOR;
    if (!IsFSFolder(_folder))
      _folder->LoadItems();
    HRESULT res = _folder->BindToFolder(s, &newFolder);
    if (!newFolder || res != S_OK)
      return NULL;
    // 		_folder->Release();
    _folder = newFolder;
  }
  return _folder;
}

FString CFolderView::GetFullPath(HTREEITEM hItem)
{
  CMyComPtr<IFolderFolder> _folder = GetFolder(hItem);
  return GetSystemString(GetFolderPath(_folder));
  // 	INT nType = (INT)GetItemData(hItem);
  // 	if (nType != TVI_TYPE_DRIVE && nType != TVI_TYPE_NORMAL)
  // 		return GetUnicodeString(GetItemText(hItem)) + FCHAR_PATH_SEPARATOR;
  // 	FString fullPath;
  // 	while (hItem != NULL && hItem != TVI_ROOT &&
  // 		nType == TVI_TYPE_NORMAL)
  // 	{
  // 		FString name = GetSystemString(GetItemText(hItem));
  // 		fullPath = name + FCHAR_PATH_SEPARATOR + fullPath;
  // 
  // 		hItem = GetParentItem(hItem);
  // 		nType = (INT)GetItemData(hItem);
  // 	}
  // 
  // 	for (; nType == TVI_TYPE_DRIVE;) {
  // 		FString str = GetSystemString(GetItemText(hItem));
  // 		INT nColon = str.Find(_T(':'));
  // 		if (nColon < 1)
  // 			break;
  // 		FChar drive = str.Ptr()[nColon - 1];
  // 		if (drive < _T('A') || drive > _T('Z'))
  // 			break;
  // 		return str.Mid(nColon - 1, 2) + FCHAR_PATH_SEPARATOR + fullPath;
  // 	}
  // 	return FString(FCHAR_PATH_SEPARATOR) + fullPath;
}


BOOL CFolderView::MightHaveChild(HTREEITEM hParent)
{
  // 	UString name;

  INT nType = (INT)GetItemData(hParent);
  switch (nType) {
  case TVI_TYPE_COMPUTER:
  case TVI_TYPE_NETWORK:
  case TVI_TYPE_VOLUMES:
    return TRUE;
  case TVI_TYPE_DESKTOP:
    // 		name = LangString(IDS_DESKTOP);
    break;
  case TVI_TYPE_DOCUMENTS:
    // 		name = LangString(IDS_DOCUMENTS);
    break;
  case TVI_TYPE_DRIVE:
  case TVI_TYPE_NORMAL:
    // 		name = GetUnicodeString(GetFullPath(hParent));
    break;
  default:
    return FALSE;
  }
  return PrepareChildren(hParent, TRUE) > 0;
  // 	UInt32 numItems;
  // 	CMyComPtr<IFolderFolder> _folder;
  // 	_folder = GetFolder(name);
  // 	if (_folder) {
  // 		_folder->LoadItems();
  // 		if (_folder->GetNumberOfItems(&numItems) == S_OK && numItems > 0)
  // 			return TRUE;
  // 	}
  // 	return FALSE;
}


void CFolderView::AddDumySingleChild(HTREEITEM hParent)
{
  if (MightHaveChild(hParent)) {
    HTREEITEM hItem = InsertItem(_T("."), -1, -1, hParent);
    SetItemData(hItem, TVI_TYPE_DUMMY);
  }
}


void CFolderView::OnTvnItemexpanding(NMHDR *pNMHDR, LRESULT *pResult)
{
  LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

  if (pNMTreeView->action == TVE_EXPAND) {
    HTREEITEM hParent = pNMTreeView->itemNew.hItem;
    PrepareChildren(hParent);
  }

  *pResult = 0;
}


INT CFolderView::PrepareChildren(HTREEITEM hParent, BOOL test)
{
  INT count = 0;

  HTREEITEM hItem = GetChildItem(hParent);
  if (hItem != NULL) {
    if (GetItemData(hItem) != TVI_TYPE_DUMMY)
      return 0;
    if (test)
      return 1;
  }
  while (hItem != NULL) {
    DeleteItem(hItem);
    hItem = GetChildItem(hParent);
  }

  INT nParentType = (INT)GetItemData(hParent);

  UInt32 numItems;
  CMyComPtr<IFolderFolder> folder;
  folder = GetFolder(hParent);
  if (!folder)
    return 0;
  folder->LoadItems();
  if (folder->GetNumberOfItems(&numItems) != S_OK || numItems <= 0)
    return 0;

  CMyComPtr<IFolderGetItemName> folderGetItemName;
  folder.QueryInterface(IID_IFolderGetItemName, &folderGetItemName);

  CMyComPtr<IFolderGetSystemIconIndex> folderGetSystemIconIndex;
  if (!IsFSFolder(folder))
    folder.QueryInterface(IID_IFolderGetSystemIconIndex, &folderGetSystemIconIndex);

  UString correctedName;
  UString itemName;
  UString relPath;
  for (UInt32 i = 0; i < numItems; i++)
  {
    if (!GetItem_BoolProp(folder, i, kpidIsDir))
      continue;

    const wchar_t *name = NULL;
    unsigned nameLen = 0;
    if (folderGetItemName)
      folderGetItemName->GetItemName(i, &name, &nameLen);
    if (name == NULL)
    {
      NWindows::NCOM::CPropVariant prop;
      if (folder->GetProperty(i, kpidName, &prop) != S_OK)
        break;// throw 2723400;
      if (prop.vt != VT_BSTR)
        break;// throw 2723401;
      itemName.Empty();
      itemName += prop.bstrVal;

      name = itemName;
      nameLen = itemName.Len();
    }

    count++;
    if (test)
      continue;

    UInt32 attrib = 0;
    // for (int yyy = 0; yyy < 6000000; yyy++) {
    NWindows::NCOM::CPropVariant prop;
    if (folder->GetProperty(i, kpidAttrib, &prop) != S_OK)
      break;
    if (prop.vt == VT_UI4)
    {
      // char s[256]; sprintf(s, "attrib = %7x", attrib); OutputDebugStringA(s);
      attrib = prop.ulVal;
    }
    // }

    Int32 iImage;

    bool defined = false;
    if (folderGetSystemIconIndex)
    {
      folderGetSystemIconIndex->GetSystemIconIndex(i, &iImage);
      defined = (iImage > 0);
    }
    if (!defined)
    {
      if (!IsFSFolder(folder))
      {
        int iconIndexTemp = GetRealIconIndex(
          us2fs((UString)name) + FCHAR_PATH_SEPARATOR, attrib);
        iImage = iconIndexTemp;
      }
      else
      {
        iImage = _extToIconMap.GetIconIndex(attrib, name);
      }
    }
    if (iImage < 0)
      iImage = 0;

    hItem = InsertItem(GetSystemString(name), iImage, iImage, hParent);
    SetItemData(hItem,
      (nParentType == TVI_TYPE_DRIVE) ?
      (DWORD_PTR)TVI_TYPE_NORMAL :
      (DWORD_PTR)nParentType
      );
    AddDumySingleChild(hItem);
  }

  return count;
}


CMyComPtr<IFolderFolder> CFolderView::GetSelectedFolder()
{
  HTREEITEM hItem = GetSelectedItem();
  return GetFolder(hItem);
}


FString CFolderView::GetFullPath()
{
  CMyComPtr<IFolderFolder> _folder = GetSelectedFolder();
  if (!_folder)
    return FString();
  return GetSystemString(GetFolderPath(_folder));
}


HTREEITEM CFolderView::GetTopLevelItem(INT nType)
{
  HTREEITEM hItem = GetRootItem();
  if ((INT)GetItemData(hItem) == nType)
    return hItem;
  hItem = GetChildItem(hItem);
  while (hItem != NULL) {
    if ((INT)GetItemData(hItem) == nType)
      return hItem;
    hItem = GetNextSiblingItem(hItem);
  }
  return NULL;
}


BOOL CFolderView::SelectFolder(CMyComPtr<IFolderFolder> _folder)
{
  UString fullPath = GetFolderPath(_folder);
  UString curPath = GetUnicodeString(GetFullPath());
  if (AreEqualNames(curPath, fullPath))
    return TRUE;
  if (_folder == NULL)
    return FALSE;

  if (m_bTracking)
    return TRUE;

  if (IsFSFolder(_folder)) {
    //HTREEITEM hItem = GetTopLevelItem(TVI_TYPE_COMPUTER);
    HTREEITEM hItem = GetChildItem(hComputer);
    HTREEITEM hLastItem = hItem;
    UStringVector parts;
    SplitPathToParts(fullPath, parts);
    FOR_VECTOR(i, parts)
    {
      const UString &s = parts[i];
      if (/*(i == 0 || i == parts.Size() - 1) &&*/ s.IsEmpty())
        continue;
      while (hItem != NULL) {
        UString name = GetUnicodeString(GetItemText(hItem));
        if (i == 0) {
          name = ExtractDrivePath(name);
          if (!name.IsEmpty() && name.Back() == WCHAR_PATH_SEPARATOR)
            name.DeleteBack();
        }
        if (name.IsEqualTo_NoCase(s)) {
          if (i < parts.Size() - 1) {
            //PrepareChildren(hItem);
            Expand(hItem, TVE_EXPAND);
          }
          hLastItem = hItem;
          hItem = GetChildItem(hItem);
          break;
        }
        hItem = GetNextSiblingItem(hItem);
      }
    }
    SelectItem(hLastItem);
  }
  else if (IsNetFolder(_folder)) {
    //HTREEITEM hItem = GetTopLevelItem(TVI_TYPE_NETWORK);
    UString fullPath = GetFolderPath(_folder);
    FString computer = GetSystemString(ExtractComputerName(fullPath));

    //PrepareChildren(hNetwork);
    Expand(hNetwork, TVE_EXPAND);
    HTREEITEM hItem = GetChildItem(hNetwork);
    while (hItem) {
      //PrepareChildren(hItem);
      Expand(hItem, TVE_EXPAND);
      HTREEITEM hChild1 = GetChildItem(hItem);
      if (hChild1 == NULL)
        AddDumySingleChild(hItem);
      while (hChild1) {
        if (computer.IsEqualTo_NoCase(GetItemText(hChild1))) {
          SelectItem(hChild1);
          return TRUE;
        }
        //PrepareChildren(hChild1);
        Expand(hChild1, TVE_EXPAND);

        HTREEITEM hChild2 = GetChildItem(hChild1);
        if (hChild2 == NULL)
          AddDumySingleChild(hChild1);
        while (hChild2) {
          if (computer.IsEqualTo_NoCase(GetItemText(hChild2))) {
            SelectItem(hChild2);
            return TRUE;
          }
          hChild2 = GetNextSiblingItem(hChild2);
        }

        hChild1 = GetNextSiblingItem(hChild1);
      }
      hItem = GetNextSiblingItem(hItem);
    }
    // 		UString fullPath = GetFolderPath(_folder);
    // 		UStringVector parts;
    // 		SplitPathToParts(fullPath, parts);
    // 		FOR_VECTOR(i, parts)
    // 		{
    // 		}
  }
  else {

  }
  return TRUE;// SelectFolder(GetSystemString(GetFolderPath(_folder)));
}


BOOL CFolderView::SelectFolder(LPCTSTR lpszPath)
{
  return SelectFolder(::GetFolder(GetUnicodeString(lpszPath)));
}


void CFolderView::OnNMClick(NMHDR *pNMHDR, LRESULT *pResult)
{
  HTREEITEM hItem;
  UINT flags;
  DWORD pos = GetMessagePos();
  CPoint point(LOWORD(pos), HIWORD(pos));
  ScreenToClient(&point);
  hItem = HitTest(point, &flags);
  if (hItem != GetSelectedItem()) {
    SelectItem(hItem);

    m_bTracking = TRUE;
    GetTopLevelParent()->SendMessage(kFolderChanged, (WPARAM)this);
    m_bTracking = FALSE;
  }

  *pResult = 0;
}


void CFolderView::OnNMReturn(NMHDR *pNMHDR, LRESULT *pResult)
{
  m_bTracking = TRUE;
  GetTopLevelParent()->SendMessage(kFolderChanged, (WPARAM)this);
  m_bTracking = FALSE;

  *pResult = 0;
}


extern const int kMenuCmdID_Plugin_Start = 1000; // must be large them context menu IDs
extern const int kMenuCmdID_Toolbar_Start = 1500;

extern const int kSevenZipStartMenuID = kMenuCmdID_Plugin_Start;
extern const int kSystemStartMenuID = kMenuCmdID_Plugin_Start + 100;

#if defined(_MSC_VER) && !defined(UNDER_CE)
#define use_CMINVOKECOMMANDINFOEX
#endif

bool CFolderView::InvokePluginCommand(int id, IContextMenu *systemContextMenu)
{
  UString currentFolderPrefix = GetUnicodeString(GetFullPath());

  UInt32 offset;
  bool isSystemMenu = (id >= kSystemStartMenuID);
  if (isSystemMenu)
    offset = id - kSystemStartMenuID;
  else
    offset = id - kSevenZipStartMenuID;

#ifndef use_CMINVOKECOMMANDINFOEXR
  CMINVOKECOMMANDINFO
#else
  CMINVOKECOMMANDINFOEX
#endif
    commandInfo;
  memset(&commandInfo, 0, sizeof(commandInfo));
  commandInfo.cbSize = sizeof(commandInfo);
  commandInfo.fMask = 0
#ifdef use_CMINVOKECOMMANDINFOEXR
    | CMIC_MASK_UNICODE
#endif
    ;
  commandInfo.hwnd = *GetTopLevelParent();
  commandInfo.lpVerb = (LPCSTR)(MAKEINTRESOURCE(offset));
  commandInfo.lpParameters = NULL;
  AString currentFolderSys = GetAnsiString(currentFolderPrefix);
  commandInfo.lpDirectory = (LPCSTR)(currentFolderSys);
  commandInfo.nShow = SW_SHOW;
#ifdef use_CMINVOKECOMMANDINFOEXR
  commandInfo.lpParametersW = NULL;
  commandInfo.lpTitle = "";
  commandInfo.lpVerbW = (LPCWSTR)(MAKEINTRESOURCEW(offset));
  UString currentFolderUnicode = _currentFolderPrefix;
  commandInfo.lpDirectoryW = currentFolderUnicode;
  commandInfo.lpTitleW = L"";
  // commandInfo.ptInvoke.x = xPos;
  // commandInfo.ptInvoke.y = yPos;
  commandInfo.ptInvoke.x = 0;
  commandInfo.ptInvoke.y = 0;
#endif
  HRESULT result;
  if (isSystemMenu)
    result = systemContextMenu->InvokeCommand(LPCMINVOKECOMMANDINFO(&commandInfo));
  if (result == NOERROR)
  {
    //KillSelection();
    return true;
  }
  return false;
}

HRESULT CFolderView::CreateShellContextMenu(
  HTREEITEM hItem,
  CMyComPtr<IContextMenu> &systemContextMenu)
{
  systemContextMenu.Release();

  UString folderPath = GetUnicodeString(GetFullPath(hItem));
  UString fileName;
  HTREEITEM hParentItem = GetParentItem(hItem);
  if (hParentItem != NULL) {
    folderPath = GetUnicodeString(GetFullPath(hParentItem));
    fileName = GetItemText(hItem);
  }
  else
    folderPath = GetUnicodeString(GetFullPath(hItem));

  CMyComPtr<IShellFolder> desktopFolder;
  RINOK(::SHGetDesktopFolder(&desktopFolder));
  if (!desktopFolder)
  {
    // ShowMessage("Failed to get Desktop folder.");
    return E_FAIL;
  }

  // Separate the file from the folder.


  // Get a pidl for the folder the file
  // is located in.
  LPITEMIDLIST parentPidl;
  DWORD eaten;
  RINOK(desktopFolder->ParseDisplayName(
    *GetTopLevelParent(), 0, (wchar_t *)(const wchar_t *)folderPath,
    &eaten, &parentPidl, 0));

  // Get an IShellFolder for the folder
  // the file is located in.
  CMyComPtr<IShellFolder> parentFolder;
  RINOK(desktopFolder->BindToObject(parentPidl,
    0, IID_IShellFolder, (void**)&parentFolder));
  if (!parentFolder)
  {
    // ShowMessage("Invalid file name.");
    return E_FAIL;
  }

  // Get a pidl for the file itself.
  CRecordVector<LPITEMIDLIST> pidls;

  // Get the IContextMenu for the file.
  CMyComPtr<IContextMenu> cm;

  LPITEMIDLIST pidl;
  if (!fileName.IsEmpty()) {
    fileName += WCHAR_PATH_SEPARATOR;
    RINOK(parentFolder->ParseDisplayName(*GetTopLevelParent(), 0,
      (wchar_t *)(const wchar_t *)fileName, &eaten, &pidl, 0));
    pidls.Add(pidl);

    RINOK(parentFolder->GetUIObjectOf(*GetTopLevelParent(), pidls.Size(),
      (LPCITEMIDLIST *)&pidls.Front(), IID_IContextMenu, 0, (void**)&cm));
  }
  else {
    RINOK(desktopFolder->ParseDisplayName(*GetTopLevelParent(), 0,
      (wchar_t *)(const wchar_t *)folderPath, &eaten, &pidl, 0));
    pidls.Add(pidl);

    RINOK(desktopFolder->GetUIObjectOf(*GetTopLevelParent(), pidls.Size(),
      (LPCITEMIDLIST *)&pidls.Front(), IID_IContextMenu, 0, (void**)&cm));
  }

  if (!cm) {
    // ShowMessage("Unable to get context menu interface.");
    return E_FAIL;
  }
  systemContextMenu = cm;
  return S_OK;
}

HMENU CFolderView::CreateSystemMenu(
  HTREEITEM hItem,
  CMyComPtr<IContextMenu> &systemContextMenu)
{
  systemContextMenu.Release();

  HRESULT hr = CreateShellContextMenu(hItem, systemContextMenu);

  if (FAILED(hr) || systemContextMenu == 0)
    return NULL;

  // Set up a CMINVOKECOMMANDINFO structure.
  CMINVOKECOMMANDINFO ci;
  ZeroMemory(&ci, sizeof(ci));
  ci.cbSize = sizeof(CMINVOKECOMMANDINFO);
  ci.hwnd = *GetTopLevelParent();

  /*
  if (Sender == GoBtn)
  {
  // Verbs that can be used are cut, paste,
  // properties, delete, and so on.
  String action;
  if (CutRb->Checked)
  action = "cut";
  else if (CopyRb->Checked)
  action = "copy";
  else if (DeleteRb->Checked)
  action = "delete";
  else if (PropertiesRb->Checked)
  action = "properties";

  ci.lpVerb = action.c_str();
  result = cm->InvokeCommand(&ci);
  if (result)
  ShowMessage(
  "Error copying file to clipboard.");

  }
  else
  */
  {
    // HMENU hMenu = CreatePopupMenu();
    NWindows::CMenu popupMenu;
    // CMenuDestroyer menuDestroyer(popupMenu);
    if (!popupMenu.CreatePopup())
      throw 210503;

    DWORD Flags = CMF_EXPLORE;
    // Optionally the shell will show the extended
    // context menu on some operating systems when
    // the shift key is held down at the time the
    // context menu is invoked. The following is
    // commented out but you can uncommnent this
    // line to show the extended context menu.
    // Flags |= 0x00000080;
    systemContextMenu->QueryContextMenu(popupMenu, 0, kSystemStartMenuID, 0x7FFF, Flags);

    NWindows::CMenu menu;
    menu.CreatePopup();
    NWindows::CMenuItem menuItem;
    menuItem.fMask = MIIM_SUBMENU | MIIM_TYPE | MIIM_ID;
    menuItem.fType = MFT_STRING;
    menuItem.hSubMenu = popupMenu.Detach();
    // menuDestroyer.Disable();
    LangString(IDS_SYSTEM, menuItem.StringValue);
    menu.InsertItem(0, true, menuItem);

    return menu;
  }
}

HMENU CFolderView::CreateFileMenu(HTREEITEM hItem,
  CMyComPtr<IContextMenu> &systemContextMenu)
{
  systemContextMenu.Release();

  NWindows::CMenu menu;
  menu.Attach(CreateSystemMenu(hItem, systemContextMenu));

  /*
  if (menu.GetItemCount() > 0)
  menu.AppendItem(MF_SEPARATOR, 0, (LPCTSTR)0);
  */

  CMyComPtr<IFolderFolder> folder = GetFolder(hItem);
  LoadFolderMenu(menu, menu.GetItemCount(), IsFSFolder(folder));

  return menu;
}

void CopyPopMenu_IfRequired(NWindows::CMenuItem &item);

void CFolderView::LoadFolderMenu(HMENU hMenu, int startPos, bool allAreFiles)
{
  NWindows::CMenu destMenu;
  destMenu.Attach(hMenu);

  HMENU hFolderMenu = theApp.GetContextMenuManager()->GetMenuById(IDR_MENU_FOLDER);
  hFolderMenu = GetSubMenu(hFolderMenu, 0);
  NWindows::CMenu folderMenu;
  folderMenu.Attach(hFolderMenu);

  int numRealItems = startPos;
  for (int i = 0;; i++)
  {
    NWindows::CMenuItem item;

    item.fMask = MIIM_SUBMENU | MIIM_STATE | MIIM_ID | MIIM_TYPE;
    item.fType = MFT_STRING;
    if (!folderMenu.GetItem(i, true, item))
      break;
    {
      CopyPopMenu_IfRequired(item);
      if (destMenu.InsertItem(startPos, true, item))
      {
        startPos++;
      }

      if (!item.IsSeparator())
        numRealItems = startPos;
    }
  }
  destMenu.RemoveAllItemsFrom(numRealItems);
}

void CFolderView::OnContextMenu(CWnd* pWnd, CPoint point)
{
  NWindows::CMenu menu;
  NWindows::CMenuDestroyer menuDestroyer(menu);
  menu.CreatePopup();

  HTREEITEM hItem = GetSelectedItem();
  if (hItem == NULL)
    return;

  CMyComPtr<IContextMenu> systemContextMenu;
  menu.Attach(CreateFileMenu(hItem, systemContextMenu));

  int result = menu.Track(TPM_LEFTALIGN
#ifndef UNDER_CE
    | TPM_RIGHTBUTTON
#endif
    | TPM_RETURNCMD | TPM_NONOTIFY,
    point.x, point.y, *this);

  if (result == 0)
    return;

  HTREEITEM hParentItem = GetParentItem(hItem);
  UString fullPath = GetFullPath();

  if (result >= 0x8000)
    GetTopLevelParent()->SendMessage(WM_COMMAND, result);
  else if (result >= kMenuCmdID_Plugin_Start) {
    InvokePluginCommand(result, systemContextMenu);
    Refresh();
  }
}

void CFolderView::OnNMRClick(NMHDR *pNMHDR, LRESULT *pResult)
{
  OnNMClick(pNMHDR, pResult);

  CPoint point;
  GetCursorPos(&point);
  OnContextMenu(this, point);

  *pResult = 0;
}

void CFolderView::Refresh()
{
  UString fullPath = GetFullPath();
  if (fullPath.IsEmpty()) {
    HTREEITEM hItem = GetSelectedItem();
    if (hItem != NULL) {
      HTREEITEM hParentItem = GetParentItem(hItem);
      if (hParentItem != NULL)
        fullPath = GetFullPath(hParentItem);
    }
  }
  FillRootItems();
  SelectFolder(fullPath);
}

void CFolderView::RenameFolder()
{
  HTREEITEM hItem = GetSelectedItem();
  EditLabel(hItem);
}

void CFolderView::OnCopy(bool move)
{
  HTREEITEM hItem = GetSelectedItem();
  if (hItem == NULL)
    return;
  HTREEITEM hParentItem = GetParentItem(hItem);
  if (hParentItem == NULL)
    return;
  CMyComPtr<IFolderFolder> folder = GetFolder(hParentItem);
  UString itemName = GetUnicodeString(GetItemText(hItem));
  UString parentPath = GetFolderPath(folder);

  CMyComPtr<IFolderOperations> folderOperations;
  if (folder.QueryInterface(IID_IFolderOperations, &folderOperations) != S_OK) {
    MessageBoxErrorLang(IDS_OPERATION_IS_NOT_SUPPORTED);
    return;
  }

  CRecordVector<UInt32> indices;
  UString destPath;

  {
    int realIndex = GetRealIndex(folder, itemName);
    if (realIndex < 0)
      return;
    indices.Add(realIndex);
    destPath = parentPath;// GetItemText(hItem);
  }
  UStringVector copyFolders;
//   ReadCopyHistory(copyFolders);
  {
    CCopyDialog copyDialog;

    copyDialog.Strings = copyFolders;
    copyDialog.Value = destPath;
    LangString(move ? IDS_MOVE : IDS_COPY, copyDialog.Title);
    LangString(move ? IDS_MOVE_TO : IDS_COPY_TO, copyDialog.Static);
    copyDialog.Info = itemName;

    if (copyDialog.DoModal() != IDOK)
      return;

    destPath = copyDialog.Value;
  }

  {
    if (destPath.IsEmpty())
    {
      MessageBoxErrorLang(IDS_OPERATION_IS_NOT_SUPPORTED);
      return;
    }

    UString correctName;
    if (!CorrectFsPath(parentPath, destPath, correctName))
    {
      MessageBoxError(E_INVALIDARG);
      return;
    }

    if (NWindows::NFile::NName::IsAbsolutePath(destPath))
      destPath.Empty();
    else
      destPath = parentPath;
    destPath += correctName;

#ifndef UNDER_CE
    if (destPath.Len() > 0 && destPath[0] == '\\')
      if (destPath.Len() == 1 || destPath[1] != '\\')
      {
        MessageBoxErrorLang(IDS_OPERATION_IS_NOT_SUPPORTED);
        return;
      }
#endif

    if ((!destPath.IsEmpty() && destPath.Back() == WCHAR_PATH_SEPARATOR) ||
      NWindows::NFile::NFind::DoesDirExist(us2fs(destPath)) ||
      IsArcFolder(folder))
    {
      NWindows::NFile::NDir::CreateComplexDir(us2fs(destPath));
      NWindows::NFile::NName::NormalizeDirPathPrefix(destPath);
      if (!CheckFolderPath(destPath))
      {
        MessageBoxErrorLang(IDS_OPERATION_IS_NOT_SUPPORTED);
      }
    }
    else
    {
      if (!IsCorrectFsName(destPath))
      {
        MessageBoxError(E_INVALIDARG);
        return;
      }
      int pos = destPath.ReverseFind(WCHAR_PATH_SEPARATOR);
      if (pos >= 0)
      {
        UString prefix = destPath.Left(pos + 1);
        NWindows::NFile::NDir::CreateComplexDir(us2fs(prefix));
        if (!CheckFolderPath(prefix))
        {
          MessageBoxErrorLang(IDS_OPERATION_IS_NOT_SUPPORTED);
          return;
        }
      }
    }

//     AddUniqueStringToHeadOfList(copyFolders, destPath);
    while (copyFolders.Size() > 20)
      copyFolders.DeleteBack();
//     SaveCopyHistory(copyFolders);
  }

  bool useSrcPanel = true;

  HRESULT result = S_OK;
  if (useSrcPanel)
  {
    CCopyToOptions options;
    options.folder = destPath;
    options.moveMode = move;
    options.includeAltStreams = true;
    options.replaceAltStreamChars = false;
    options.showErrorMessages = true;

    result = CopyTo(options, indices, NULL);
  }

  if (result != S_OK)
  {
    if (result != E_ABORT)
      MessageBoxError(result, L"Error");
    // return;
  }

  Refresh();
}

HRESULT CFolderView::CopyTo(CCopyToOptions &options,
  const CRecordVector<UInt32> &indices,
  UStringVector *messages,
  bool showProgress)
{
  HTREEITEM hItem = GetSelectedItem();
  if (hItem == NULL)
    return E_FAIL;
  HTREEITEM hParentItem = GetParentItem(hItem);
  if (hParentItem == NULL)
    return E_FAIL;
  CMyComPtr<IFolderFolder> folder = GetFolder(hParentItem);

  CWaitCursor *waitCursor = NULL;
  if (!showProgress)
    waitCursor = new CWaitCursor;
  CMyComPtr<IFolderOperations> folderOperations;
  if (folder.QueryInterface(IID_IFolderOperations, &folderOperations) != S_OK)
  {
    UString errorMessage = LangString(IDS_OPERATION_IS_NOT_SUPPORTED);
    if (options.showErrorMessages)
      MessageBox(errorMessage);
    else if (messages != 0)
      messages->Add(errorMessage);
    if (waitCursor != NULL)
      delete waitCursor;
    return E_FAIL;
  }

  HRESULT res;
  {
    folder->LoadItems();

    CPanelCopyThread extracter;

    extracter.ExtractCallbackSpec = new CExtractCallbackImp;
    extracter.ExtractCallback = extracter.ExtractCallbackSpec;

    extracter.options = &options;
    extracter.ExtractCallbackSpec->ProgressDialog = &extracter.ProgressDialog;
    extracter.ProgressDialog.CompressingMode = false;

    extracter.ExtractCallbackSpec->StreamMode = options.streamMode;

    if (indices.Size() == 1)
      extracter.FirstFilePath = GetItemRelPath(folder, indices[0]);

    extracter.ExtractCallbackSpec->ProcessAltStreams = options.includeAltStreams;

    UString title;
    {
      UInt32 titleID = IDS_COPYING;
      if (options.moveMode)
        titleID = IDS_MOVING;

      title = LangString(titleID);
    }

    UString progressWindowTitle = L"FileManager"; // LangString(IDS_APP_TITLE);

    extracter.ProgressDialog.MainWindow = *GetTopLevelParent();
    extracter.ProgressDialog.MainTitle = progressWindowTitle;
    extracter.ProgressDialog.MainAddTitle = title + L' ';

    extracter.ExtractCallbackSpec->OverwriteMode =
      showProgress ? NExtract::NOverwriteMode::kAsk : NExtract::NOverwriteMode::kSkip;
    extracter.ExtractCallbackSpec->Init();
    extracter.Indices = indices;
    extracter.FolderOperations = folderOperations;

    RINOK(extracter.Create(title, *GetTopLevelParent(), showProgress));

    if (messages != 0)
      *messages = extracter.ProgressDialog.Sync.Messages;
    res = extracter.Result;

  }

  if (waitCursor != NULL)
    delete waitCursor;

  return res;
}

void CFolderView::CopyTo()
{
  OnCopy(false);
}

void CFolderView::MoveTo()
{
  OnCopy(true);
}

void CFolderView::DeleteFolder(bool toRecycleBin)
{
  HTREEITEM hItem = GetSelectedItem();
  if (hItem == NULL)
    return;
  HTREEITEM hParentItem = GetParentItem(hItem);
  if (hParentItem == NULL)
    return;
  CMyComPtr<IFolderFolder> folder = GetFolder(hParentItem);
  UString itemName = GetUnicodeString(GetItemText(hItem));
  UString parentPath = GetFolderPath(folder);

  CRecordVector<UInt32> indices;
  {
    int realIndex = GetRealIndex(folder, itemName);
    if (realIndex < 0)
      return;
    indices.Add(realIndex);
  }

  UString fsPath;
  if (!(IsFSDrivesFolder(folder) && parentPath != L"\\\\.\\"))
    fsPath = parentPath;

#ifndef UNDER_CE
  // WM6 / SHFileOperationW doesn't ask user! So we use internal delete
  if (IsFSFolder(folder) && toRecycleBin)
  {
    bool useInternalDelete = false;
#ifndef _UNICODE
    if (!g_IsNT)
    {
      CDynamicBuffer<CHAR> buffer;
      FOR_VECTOR(i, indices)
      {
        const AString path = GetSystemString(fsPath + GetItemRelPath(folder, indices[i]));
        memcpy(buffer.GetCurPtrAndGrow(path.Len() + 1), (const CHAR *)path, (path.Len() + 1) * sizeof(CHAR));
      }
      *buffer.GetCurPtrAndGrow(1) = 0;
      SHFILEOPSTRUCTA fo;
      fo.hwnd = GetParent();
      fo.wFunc = FO_DELETE;
      fo.pFrom = (const CHAR *)buffer;
      fo.pTo = 0;
      fo.fFlags = 0;
      if (toRecycleBin)
        fo.fFlags |= FOF_ALLOWUNDO;
      // fo.fFlags |= FOF_NOCONFIRMATION;
      // fo.fFlags |= FOF_NOERRORUI;
      // fo.fFlags |= FOF_SILENT;
      // fo.fFlags |= FOF_WANTNUKEWARNING;
      fo.fAnyOperationsAborted = FALSE;
      fo.hNameMappings = 0;
      fo.lpszProgressTitle = 0;
      /* int res = */ ::SHFileOperationA(&fo);
    }
    else
#endif
    {
      CDynamicBuffer<WCHAR> buffer;
      unsigned maxLen = 0;
      FOR_VECTOR(i, indices)
      {
        // L"\\\\?\\") doesn't work here.
        const UString path = fsPath + GetItemRelPath(folder, indices[i]);
        if (path.Len() > maxLen)
          maxLen = path.Len();
        memcpy(buffer.GetCurPtrAndGrow(path.Len() + 1), (const WCHAR *)path, (path.Len() + 1) * sizeof(WCHAR));
      }
      *buffer.GetCurPtrAndGrow(1) = 0;
      if (maxLen >= MAX_PATH)
      {
        if (toRecycleBin)
        {
          MessageBoxErrorLang(IDS_ERROR_LONG_PATH_TO_RECYCLE);
          return;
        }
        useInternalDelete = true;
      }
      else
      {
        SHFILEOPSTRUCTW fo;
        fo.hwnd = *GetTopLevelParent();
        fo.wFunc = FO_DELETE;
        fo.pFrom = (const WCHAR *)buffer;
        fo.pTo = 0;
        fo.fFlags = 0;
        if (toRecycleBin)
          fo.fFlags |= FOF_ALLOWUNDO;
        fo.fAnyOperationsAborted = FALSE;
        fo.hNameMappings = 0;
        fo.lpszProgressTitle = 0;
        // int res;
#ifdef _UNICODE
        /* res = */ ::SHFileOperationW(&fo);
#else
        SHFileOperationWP shFileOperationW = (SHFileOperationWP)
          ::GetProcAddress(::GetModuleHandleW(L"shell32.dll"), "SHFileOperationW");
        if (shFileOperationW == 0)
          return;
        /* res = */ shFileOperationW(&fo);
#endif
      }
    }
    /*
    if (fo.fAnyOperationsAborted)
    MessageBoxError(result, LangString(IDS_ERROR_DELETING, 0x03020217));
    */
    if (!useInternalDelete)
    {
      Refresh();
      return;
    }
  }
#endif

  // DeleteItemsInternal

  CMyComPtr<IFolderOperations> folderOperations;
  if (folder.QueryInterface(IID_IFolderOperations, &folderOperations) != S_OK)
  {
    MessageBoxErrorForUpdate(E_NOINTERFACE, IDS_ERROR_DELETING);
    return;
  }

  UInt32 titleID, messageID;
  UString messageParam;
  if (indices.Size() == 1)
  {
    int index = indices[0];
    messageParam = GetItemRelPath(folder, index);
    if (IsItem_Folder(folder, index))
    {
      titleID = IDS_CONFIRM_FOLDER_DELETE;
      messageID = IDS_WANT_TO_DELETE_FOLDER;
    }
    else
    {
      titleID = IDS_CONFIRM_FILE_DELETE;
      messageID = IDS_WANT_TO_DELETE_FILE;
    }
  }
  else
  {
    titleID = IDS_CONFIRM_ITEMS_DELETE;
    messageID = IDS_WANT_TO_DELETE_ITEMS;
    messageParam = NumberToString(indices.Size());
  }
  if (::MessageBoxW(*GetTopLevelParent(), MyFormatNew(messageID, messageParam), LangString(titleID), MB_OKCANCEL | MB_ICONQUESTION) != IDOK)
    return;

  {
    CThreadFolderOperations op(FOLDER_TYPE_DELETE);
    op.FolderOperations = folderOperations;
    op.Indices = indices;
    op.DoOperation(this, NULL,
      LangString(IDS_DELETING),
      LangString(IDS_ERROR_DELETING));
  }
  Refresh();
}

bool Dlg_CreateFolder(HWND wnd, UString &destName);

void CFolderView::CreateFolder()
{
  HTREEITEM hItem = GetSelectedItem();
  if (hItem == NULL)
    return;
  CMyComPtr<IFolderFolder> folder = GetFolder(hItem);

  CMyComPtr<IFolderOperations> folderOperations;
  if (folder.QueryInterface(IID_IFolderOperations, &folderOperations) != S_OK)
  {
    MessageBoxErrorForUpdate(E_NOINTERFACE, IDS_CREATE_FOLDER_ERROR);
    return;
  }

  UString newName;
  if (!Dlg_CreateFolder(*GetTopLevelParent(), newName))
    return;

  if (!IsCorrectFsName(newName))
  {
    MessageBoxError(E_INVALIDARG);
    return;
  }

  if (IsFSFolder(folder))
  {
    UString folderPath = GetFolderPath(folder);
    UString correctName;
    if (!CorrectFsPath(folderPath, newName, correctName))
    {
      MessageBoxError(E_INVALIDARG);
      return;
    }
    newName = correctName;
  }

  HRESULT res;
  {
    CThreadFolderOperations op(FOLDER_TYPE_CREATE_FOLDER);
    op.FolderOperations = folderOperations;
    op.Name = newName;
    res = op.DoOperation(this, NULL,
      LangString(IDS_CREATE_FOLDER),
      LangString(IDS_CREATE_FOLDER_ERROR));
    /*
    // fixed for 9.26: we must refresh always
    if (res != S_OK)
    return;
    */
  }
  if (res == S_OK)
  {
    int pos = newName.Find(WCHAR_PATH_SEPARATOR);
    if (pos >= 0)
      newName.DeleteFrom(pos);
  }
  Refresh();
}

void CFolderView::CreateFile()
{
  HTREEITEM hItem = GetSelectedItem();
  if (hItem == NULL)
    return;
  CMyComPtr<IFolderFolder> folder = GetFolder(hItem);

  CMyComPtr<IFolderOperations> folderOperations;
  if (folder.QueryInterface(IID_IFolderOperations, &folderOperations) != S_OK)
  {
    MessageBoxErrorForUpdate(E_NOINTERFACE, IDS_CREATE_FILE_ERROR);
    return;
  }
  CComboDialog dlg;
  LangString(IDS_CREATE_FILE, dlg.Title);
  LangString(IDS_CREATE_FILE_NAME, dlg.Static);
  LangString(IDS_CREATE_FILE_DEFAULT_NAME, dlg.Value);

  if (dlg.DoModal() != IDOK)
    return;

  UString newName = dlg.Value;

  if (IsFSFolder(folder))
  {
    UString folderPath = GetFolderPath(folder);
    UString correctName;
    if (!CorrectFsPath(folderPath, newName, correctName))
    {
      MessageBoxError(E_INVALIDARG);
      return;
    }
    newName = correctName;
  }

  HRESULT result = folderOperations->CreateFile(newName, 0);
  if (result != S_OK)
  {
    MessageBoxErrorForUpdate(result, IDS_CREATE_FILE_ERROR);
    return;
  }
  int pos = newName.Find(WCHAR_PATH_SEPARATOR);
  if (pos >= 0)
    newName.DeleteFrom(pos);
  Refresh();
}

void AddPropertyString(PROPID propID, const wchar_t *nameBSTR,
  const NWindows::NCOM::CPropVariant &prop, UString &s, int alignment);

static const wchar_t *kSeparator = L"----------------------------\n";
static const wchar_t *kSeparatorSmall = L"----\n";
static const wchar_t *kPropValueSeparator = L": ";

static const Byte kSpecProps[] =
{
  kpidPath,
  kpidType,
  kpidErrorType,
  kpidError,
  kpidErrorFlags,
  kpidWarning,
  kpidWarningFlags,
  kpidOffset,
  kpidPhySize,
  kpidTailSize
};

void CFolderView::Properties()
{
  HTREEITEM hItem = GetSelectedItem();
  if (hItem == NULL)
    return;
  HTREEITEM hParentItem = GetParentItem(hItem);
  if (hParentItem == NULL)
    return;
  CMyComPtr<IFolderFolder> folder = GetFolder(hParentItem);
  UString itemName = GetUnicodeString(GetItemText(hItem));
  UString parentPath = GetFolderPath(folder);

  CRecordVector<UInt32> indices;
  {
    int realIndex = GetRealIndex(folder, itemName);
    if (realIndex < 0)
      return;
    indices.Add(realIndex);
  }

  CMyComPtr<IGetFolderArcProps> getFolderArcProps;
  folder.QueryInterface(IID_IGetFolderArcProps, &getFolderArcProps);
  if (!getFolderArcProps)
  {
    NWindows::NCOM::CComInitializer comInitializer;
    if (!IsFSFolder(folder) || (IsFSDrivesFolder(folder) && parentPath != L"\\\\.\\"))
      return;
    CMyComPtr<IContextMenu> contextMenu;
    if (CreateShellContextMenu(hItem, contextMenu) != S_OK)
      return;

    CMINVOKECOMMANDINFO ci;
    ZeroMemory(&ci, sizeof(ci));
    ci.cbSize = sizeof(CMINVOKECOMMANDINFO);
    ci.hwnd = *GetTopLevelParent();
    ci.lpVerb = "properties";
    contextMenu->InvokeCommand(&ci);
    return;
  }

  {
    UString message;

    if (indices.Size() == 1)
    {
      UInt32 index = indices[0];
      // message += L"Item:\n";
      UInt32 numProps;
      if (folder->GetNumberOfProperties(&numProps) == S_OK)
      {
        for (UInt32 i = 0; i < numProps; i++)
        {
          CMyComBSTR name;
          PROPID propID;
          VARTYPE varType;

          if (folder->GetPropertyInfo(i, &name, &propID, &varType) != S_OK)
            continue;

          NWindows::NCOM::CPropVariant prop;
          if (folder->GetProperty(index, propID, &prop) != S_OK)
            continue;
          AddPropertyString(propID, name, prop, message, 16);
        }
      }

      CMyComPtr<IArchiveGetRawProps> folderRawProps;
      folder.QueryInterface(IID_IArchiveGetRawProps, &folderRawProps);
      if (folderRawProps)
      {
        folderRawProps->GetNumRawProps(&numProps);
        for (UInt32 i = 0; i < numProps; i++)
        {
          CMyComBSTR name;
          PROPID propID;
          if (folderRawProps->GetRawPropInfo(i, &name, &propID) != S_OK)
            continue;

          const void *data;
          UInt32 dataSize;
          UInt32 propType;
          if (folderRawProps->GetRawProp(index, propID, &data, &dataSize, &propType) != S_OK)
            continue;

          if (dataSize != 0)
          {
            AString s;
            if (propID == kpidNtSecure)
              ConvertNtSecureToString((const Byte *)data, dataSize, s);
            else
            {
              const UInt32 kMaxDataSize = 64;
              if (dataSize > kMaxDataSize)
              {
                char temp[64];
                s += "data:";
                ConvertUInt32ToString(dataSize, temp);
                s += temp;
              }
              else
              {
                for (UInt32 i = 0; i < dataSize; i++)
                {
                  Byte b = ((const Byte *)data)[i];
                  s += GetHex((Byte)((b >> 4) & 0xF));
                  s += GetHex((Byte)(b & 0xF));
                }
              }
            }
            message += GetNameOfProperty(propID, name);
            message += kPropValueSeparator;
            message += GetUnicodeString(s);
            message += L'\n';
          }
        }
      }

      message += kSeparator;
    }

    /*
    message += LangString(IDS_PROP_FILE_TYPE, 0x02000214);
    message += kPropValueSeparator;
    message += GetFolderTypeID();
    message += L"\n";
    */

    {
      NWindows::NCOM::CPropVariant prop;
      if (folder->GetFolderProperty(kpidPath, &prop) == S_OK)
      {
        AddPropertyString(kpidName, L"Path", prop, message, 16);
      }
    }

    CMyComPtr<IFolderProperties> folderProperties;
    folder.QueryInterface(IID_IFolderProperties, &folderProperties);
    if (folderProperties)
    {
      UInt32 numProps;
      if (folderProperties->GetNumberOfFolderProperties(&numProps) == S_OK)
      {
        for (UInt32 i = 0; i < numProps; i++)
        {
          CMyComBSTR name;
          PROPID propID;
          VARTYPE vt;
          if (folderProperties->GetFolderPropertyInfo(i, &name, &propID, &vt) != S_OK)
            continue;
          NWindows::NCOM::CPropVariant prop;
          if (folder->GetFolderProperty(propID, &prop) != S_OK)
            continue;
          AddPropertyString(propID, name, prop, message, 16);
        }
      }
    }

    CMyComPtr<IGetFolderArcProps> getFolderArcProps;
    folder.QueryInterface(IID_IGetFolderArcProps, &getFolderArcProps);
    if (getFolderArcProps)
    {
      CMyComPtr<IFolderArcProps> getProps;
      getFolderArcProps->GetFolderArcProps(&getProps);
      if (getProps)
      {
        UInt32 numLevels;
        if (getProps->GetArcNumLevels(&numLevels) != S_OK)
          numLevels = 0;
        for (UInt32 level2 = 0; level2 < numLevels; level2++)
        {
          {
            UInt32 level = numLevels - 1 - level2;
            UInt32 numProps;
            if (getProps->GetArcNumProps(level, &numProps) == S_OK)
            {
              const int kNumSpecProps = ARRAY_SIZE(kSpecProps);

              message += kSeparator;

              for (Int32 i = -kNumSpecProps; i < (Int32)numProps; i++)
              {
                CMyComBSTR name;
                PROPID propID;
                VARTYPE vt;
                if (i < 0)
                  propID = kSpecProps[i + kNumSpecProps];
                else if (getProps->GetArcPropInfo(level, i, &name, &propID, &vt) != S_OK)
                  continue;
                NWindows::NCOM::CPropVariant prop;
                if (getProps->GetArcProp(level, propID, &prop) != S_OK)
                  continue;
                AddPropertyString(propID, name, prop, message, 16);
              }
            }
          }

          if (level2 != numLevels - 1)
          {
            UInt32 level = numLevels - 1 - level2;
            UInt32 numProps;
            if (getProps->GetArcNumProps2(level, &numProps) == S_OK)
            {
              message += kSeparatorSmall;
              for (Int32 i = 0; i < (Int32)numProps; i++)
              {
                CMyComBSTR name;
                PROPID propID;
                VARTYPE vt;
                if (getProps->GetArcPropInfo2(level, i, &name, &propID, &vt) != S_OK)
                  continue;
                NWindows::NCOM::CPropVariant prop;
                if (getProps->GetArcProp2(level, propID, &prop) != S_OK)
                  continue;
                AddPropertyString(propID, name, prop, message, 16);
              }
            }
          }
        }
      }
    }
    ::MessageBoxW(*(this), message, LangString(IDS_PROPERTIES), MB_OK);
  }
}


void CFolderView::MessageBoxError(HRESULT errorCode, LPCWSTR caption)
{
  MessageBox(HResultToMessage(errorCode), caption);
}

void CFolderView::MessageBoxError(HRESULT errorCode)
{
  MessageBoxError(errorCode, L"7-Zip");
}

void CFolderView::MessageBoxErrorLang(UINT resourceID)
{
  MessageBox(LangString(resourceID));
}

void CFolderView::MessageBoxErrorForUpdate(HRESULT errorCode, UINT resourceID)
{
  if (errorCode == E_NOINTERFACE)
    MessageBoxErrorLang(IDS_OPERATION_IS_NOT_SUPPORTED);
  else
    MessageBoxError(errorCode, LangString(resourceID));
}

void CFolderView::OnTvnEndlabeledit(NMHDR *pNMHDR, LRESULT *pResult)
{
  LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);
  if (pTVDispInfo->item.pszText == NULL)
    return;

  HTREEITEM hParentItem = GetParentItem(pTVDispInfo->item.hItem);
  CMyComPtr<IFolderFolder> folder = GetFolder(hParentItem);
  UString itemName = GetUnicodeString(GetItemText(pTVDispInfo->item.hItem));
  UString parentPath = GetFolderPath(folder);

  CMyComPtr<IFolderOperations> folderOperations;
  if (folder.QueryInterface(IID_IFolderOperations, &folderOperations) != S_OK) {
    MessageBoxErrorForUpdate(E_NOINTERFACE, IDS_ERROR_RENAMING);
    return;
  }
  UString newName = pTVDispInfo->item.pszText;
  if (!IsCorrectFsName(newName)) {
    MessageBoxError(E_INVALIDARG);
    return;
  }

  if (IsFSFolder(folder)) {
    UString correctName;
    if (!CorrectFsPath(parentPath, newName, correctName)) {
      MessageBoxError(E_INVALIDARG);
      return;
    }
    newName = correctName;
  }

  int realIndex = GetRealIndex(folder, itemName);
  if (realIndex < 0)
    return;
  NWindows::NCOM::CPropVariant prop;
  if (folder->GetProperty(realIndex, kpidPrefix, &prop) != S_OK) {
    MessageBoxError(E_INVALIDARG);
    return;
  }
  UString prefix;
  if (prop.vt == VT_BSTR)
    prefix = prop.bstrVal;

  {
    CThreadFolderOperations op(FOLDER_TYPE_RENAME);
    op.FolderOperations = folderOperations;
    op.Index = realIndex;
    op.Name = newName;
     HRESULT res = op.DoOperation(this, NULL,
      LangString(IDS_RENAMING),
      LangString(IDS_ERROR_RENAMING));
    // fixed in 9.26: we refresh list even after errors
    // (it's more safe, since error can be at different stages, so list can be incorrect).
    if (res != S_OK)
      return;
  }

  SetItemText(pTVDispInfo->item.hItem, newName);

  *pResult = 0;
}

int CFolderView::GetRealIndex(CMyComPtr<IFolderFolder> folder, const wchar_t *szItem)
{
  UInt32 numItems;

  folder->LoadItems();
  if (folder->GetNumberOfItems(&numItems) != S_OK || numItems <= 0)
    return -1;

  CMyComPtr<IFolderGetItemName> folderGetItemName;
  folder.QueryInterface(IID_IFolderGetItemName, &folderGetItemName);

  UString correctedName;
  UString itemName;
  for (UInt32 i = 0; i < numItems; i++)
  {
    if (!GetItem_BoolProp(folder, i, kpidIsDir))
      continue;

    const wchar_t *name = NULL;
    unsigned nameLen = 0;
    if (folderGetItemName)
      folderGetItemName->GetItemName(i, &name, &nameLen);
    if (name == NULL)
    {
      NWindows::NCOM::CPropVariant prop;
      if (folder->GetProperty(i, kpidName, &prop) != S_OK)
        break;// throw 2723400;
      if (prop.vt != VT_BSTR)
        break;// throw 2723401;
      itemName.Empty();
      itemName += prop.bstrVal;

      name = itemName;
      nameLen = itemName.Len();
    }

    if (wcscmp(name, szItem) == 0)
      return (int)i;
  }

  return -1;
}
