// SystemPage.cpp : implementation file
//

#include "stdafx.h"

#include <ShlObj.h>

#include "CPP/Common/StringConvert.h"
#include "CPP/Common/Defs.h"

#include "CPP/Windows/DLL.h"

#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/Windows/ErrorMsg.h"

#include "CPP/7zip/UI/FileManager/HelpUtils.h"

#include "7zipProFM.h"
#include "SystemPage.h"


#ifndef _UNICODE
extern bool g_IsNT;
#endif

static const UInt32 kLangIDs[] =
{
  IDT_SYSTEM_ASSOCIATE
};

static LPCWSTR kSystemTopic = L"FME/options.htm#system";

CSysString CModifiedExtInfo::GetString() const
{
  if (State == kExtState_7ZipPro)
    return TEXT("7-ZipPro");
  if (State == kExtState_Clear)
    return TEXT("");
  if (Other7ZipPro)
    return TEXT("[7-ZipPro]");
  return ProgramKey;
};

// CSystemPage dialog

IMPLEMENT_DYNAMIC(CSystemPage, CPropertyPage)

CSystemPage::CSystemPage()
: CPropertyPage(CSystemPage::IDD), WasChanged(false)
{

}

CSystemPage::~CSystemPage()
{
}

void CSystemPage::DoDataExchange(CDataExchange* pDX)
{
  CPropertyPage::DoDataExchange(pDX);
  DDX_Control(pDX, IDL_SYSTEM_ASSOCIATE, m_lstAssociate);
}


BEGIN_MESSAGE_MAP(CSystemPage, CPropertyPage)
  ON_NOTIFY(NM_RETURN, IDL_SYSTEM_ASSOCIATE, &CSystemPage::OnNMReturnSystemAssociate)
  ON_NOTIFY(NM_CLICK, IDL_SYSTEM_ASSOCIATE, &CSystemPage::OnNMClickSystemAssociate)
  ON_BN_CLICKED(IDC_SYSTEM_CURRENT, &CSystemPage::OnBnClickedSystemCurrent)
  ON_BN_CLICKED(IDC_SYSTEM_ALL, &CSystemPage::OnBnClickedSystemAll)
  ON_NOTIFY(LVN_KEYDOWN, IDL_SYSTEM_ASSOCIATE, &CSystemPage::OnLvnKeydownSystemAssociate)
END_MESSAGE_MAP()


// CSystemPage message handlers


int CSystemPage::AddIcon(const UString &iconPath, int iconIndex)
{
  if (iconPath.IsEmpty())
    return -1;
  if (iconIndex == -1)
    iconIndex = 0;
  HICON hicon;
  #ifdef UNDER_CE
  ExtractIconExW(iconPath, iconIndex, NULL, &hicon, 1);
  if (!hicon)
  #else
  // we expand path from REG_EXPAND_SZ registry item.
  UString path;
  DWORD size = MAX_PATH + 10;
  DWORD needLen = ::ExpandEnvironmentStringsW(iconPath, path.GetBuf(size + 2), size);
  path.ReleaseBuf_CalcLen(size);
  if (needLen == 0 || needLen >= size)
    path = iconPath;
  int num = ExtractIconExW(path, iconIndex, NULL, &hicon, 1);
  if (num != 1 || !hicon)
  #endif
    return -1;
  _imageList.Add(hicon);
  DestroyIcon(hicon);
  return _numIcons++;
}

void CSystemPage::RefreshListItem(int group, int listIndex)
{
  const CAssoc &assoc = _items[GetRealIndex(listIndex)];
  m_lstAssociate.SetItemText(listIndex, group + 1, assoc.Pair[group].GetString());
  LVITEMW newItem;
  memset(&newItem, 0, sizeof(newItem));
  newItem.iItem = listIndex;
  newItem.mask = LVIF_IMAGE;
  newItem.iImage = assoc.GetIconIndex();
  m_lstAssociate.SetItem(&newItem);
}

void CSystemPage::ChangeState(int group, const CIntVector &indices)
{
  if (indices.IsEmpty())
    return;

  bool thereAreClearItems = false;
  int counters[3] = { 0, 0, 0 };

  unsigned i;
  for (i = 0; i < indices.Size(); i++)
  {
    const CModifiedExtInfo &mi = _items[GetRealIndex(indices[i])].Pair[group];
    int state = kExtState_7ZipPro;
    if (mi.State == kExtState_7ZipPro)
      state = kExtState_Clear;
    else if (mi.State == kExtState_Clear)
    {
      thereAreClearItems = true;
      if (mi.Other)
        state = kExtState_Other;
    }
    counters[state]++;
  }

  int state = kExtState_Clear;
  if (counters[kExtState_Other] != 0)
    state = kExtState_Other;
  else if (counters[kExtState_7ZipPro] != 0)
    state = kExtState_7ZipPro;

  for (i = 0; i < indices.Size(); i++)
  {
    int listIndex = indices[i];
    CAssoc &assoc = _items[GetRealIndex(listIndex)];
    CModifiedExtInfo &mi = assoc.Pair[group];
    bool change = false;
    switch (state)
    {
      case kExtState_Clear: change = true; break;
      case kExtState_Other: change = mi.Other; break;
      default: change = !(mi.Other && thereAreClearItems); break;
    }
    if (change)
    {
      mi.State = state;
      RefreshListItem(group, listIndex);
    }
  }
  SetModified();
}

BOOL CSystemPage::OnInitDialog()
{
  CPropertyPage::OnInitDialog();

  LangSetDlgItems(*this, kLangIDs, ARRAY_SIZE(kLangIDs));

  ListView_SetUnicodeFormat(m_lstAssociate, TRUE);
  DWORD newFlags = LVS_EX_FULLROWSELECT;
  m_lstAssociate.SetExtendedStyle(newFlags);

  _numIcons = 0;
  _imageList.Create(16, 16, ILC_MASK | ILC_COLOR32, 0, 0);

  m_lstAssociate.SetImageList(&_imageList, LVSIL_SMALL);

  m_lstAssociate.InsertColumn(0, GetSystemString(LangString(IDS_PROP_FILE_TYPE)), LVCFMT_LEFT, 72);

  UString s;

  #if NUM_EXT_GROUPS == 1
    s.SetFromAscii("Program");
  #else
    #ifndef UNDER_CE
      const unsigned kSize = 256;
      BOOL res;

      DWORD size = kSize;
      #ifndef _UNICODE
      if (!g_IsNT)
      {
        AString s2;
        res = GetUserNameA(s2.GetBuf(size), &size);
        s2.ReleaseBuf_CalcLen(MyMin((unsigned)size, kSize));
        s = GetUnicodeString(s2);
      }
      else
      #endif
      {
        res = GetUserNameW(s.GetBuf(size), &size);
        s.ReleaseBuf_CalcLen(MyMin((unsigned)size, kSize));
      }
    
      if (!res)
    #endif
        s.SetFromAscii("Current User");
  #endif

  LV_COLUMNW ci;
  ci.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM;
  ci.cx = 128;
  ci.fmt = LVCFMT_CENTER;
  ci.pszText = (WCHAR *)(const WCHAR *)s;
  ci.iSubItem = 1;
  m_lstAssociate.InsertColumn(1, &ci);

  #if NUM_EXT_GROUPS > 1
  {
    LangString(IDS_SYSTEM_ALL_USERS, s);
    ci.pszText = (WCHAR *)(const WCHAR *)s;
    ci.iSubItem = 2;
    m_lstAssociate.InsertColumn(2, &ci);
  }
  #endif

  _extDB.Read();
  _items.Clear();

  FOR_VECTOR (i, _extDB.Exts)
  {
    const CExtPlugins &extInfo = _extDB.Exts[i];

    LVITEMW item;
    item.iItem = i;
    item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
    item.lParam = i;
    item.iSubItem = 0;
    // ListView always uses internal iImage that is 0 by default?
    // so we always use LVIF_IMAGE.
    item.iImage = -1;
    item.pszText = (wchar_t *)(const wchar_t *)(LPCWSTR)extInfo.Ext;

    CAssoc assoc;
    const CPluginToIcon &plug = extInfo.Plugins[0];
    assoc.SevenZipImageIndex = AddIcon(plug.IconPath, plug.IconIndex);

    CSysString texts[NUM_EXT_GROUPS];
    int g;
    for (g = 0; g < NUM_EXT_GROUPS; g++)
    {
      CModifiedExtInfo &mi = assoc.Pair[g];
      mi.ReadFromRegistry(GetHKey(g), GetSystemString(extInfo.Ext));
      mi.SetState(plug.IconPath);
      mi.ImageIndex = AddIcon(mi.IconPath, mi.IconIndex);
      texts[g] = mi.GetString();
    }
    item.iImage = assoc.GetIconIndex();
    int itemIndex = m_lstAssociate.InsertItem(&item);
    for (g = 0; g < NUM_EXT_GROUPS; g++)
      m_lstAssociate.SetItemText(itemIndex, 1 + g, texts[g]);
    _items.Add(assoc);
  }

  if (m_lstAssociate.GetItemCount() > 0)
    m_lstAssociate.SetItemState(0, LVIS_FOCUSED, LVIS_FOCUSED);

  return TRUE;  // return TRUE unless you set the focus to a control
  // EXCEPTION: OCX Property Pages should return FALSE
}

static UString GetProgramCommand()
{
  return L"\"" + fs2us(NWindows::NDLL::GetModuleDirPrefix()) + L"7zipProFM.exe\" \"%1\"";
}

BOOL CSystemPage::OnApply()
{
  const UString command = GetProgramCommand();

  LONG res = 0;

  FOR_VECTOR (listIndex, _extDB.Exts)
  {
    int realIndex = GetRealIndex(listIndex);
    const CExtPlugins &extInfo = _extDB.Exts[realIndex];
    CAssoc &assoc = _items[realIndex];

    for (int g = 0; g < NUM_EXT_GROUPS; g++)
    {
      CModifiedExtInfo &mi = assoc.Pair[g];
      HKEY key = GetHKey(g);
      if (mi.OldState != mi.State)
      {
        LONG res2 = 0;
        if (mi.State == kExtState_7ZipPro)
        {
          UString title = extInfo.Ext + UString(L" Archive");
          const CPluginToIcon &plug = extInfo.Plugins[0];
          res2 = NRegistryAssoc::AddShellExtensionInfo(key, GetSystemString(extInfo.Ext),
              title, command, plug.IconPath, plug.IconIndex);
        }
        else if (mi.State == kExtState_Clear)
          res2 = NRegistryAssoc::DeleteShellExtensionInfo(key, GetSystemString(extInfo.Ext));
        if (res == 0)
          res = res2;
        if (res2 == 0)
          mi.OldState = mi.State;
        mi.State = mi.OldState;
        RefreshListItem(g, listIndex);
      }
    }
  }
  #ifndef UNDER_CE
  SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
  WasChanged = true;
  #endif
  if (res != 0)
    ::MessageBoxW(*this, NWindows::NError::MyFormatMessage(res), L"7-ZipPro", MB_ICONERROR);

  return CPropertyPage::OnApply();
}


BOOL CSystemPage::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
  LPNMHDR pNMHDR = (LPNMHDR)lParam;
  if (pNMHDR->code == PSN_HELP) {
    ShowHelpWindow(NULL, kSystemTopic);
    return TRUE;
  }

  return CPropertyPage::OnNotify(wParam, lParam, pResult);
}


void CSystemPage::OnNMReturnSystemAssociate(NMHDR *pNMHDR, LRESULT *pResult)
{
  ChangeState(0);
  *pResult = 0;
}


void CSystemPage::OnNMClickSystemAssociate(NMHDR *pNMHDR, LRESULT *pResult)
{
#ifdef UNDER_CE
  NMLISTVIEW *item = (NMLISTVIEW *)pNMHDR;
#else
  NMITEMACTIVATE *item = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
  if (item->uKeyFlags == 0)
#endif
  {
    int realIndex = GetRealIndex(item->iItem);
    if (realIndex >= 0)
    {
      if (item->iSubItem >= 1 && item->iSubItem <= 2)
      {
        CIntVector indices;
        indices.Add(item->iItem);
        ChangeState(item->iSubItem < 2 ? 0 : 1, indices);
      }
    }
  }
  *pResult = 0;
}


void CSystemPage::OnBnClickedSystemCurrent()
{
  ChangeState(0);
}


void CSystemPage::OnBnClickedSystemAll()
{
  ChangeState(1);
}


void CSystemPage::ChangeState(int group)
{
  CIntVector indices;
  POSITION pos = m_lstAssociate.GetFirstSelectedItemPosition();
  while (pos) {
    int itemIndex = m_lstAssociate.GetNextSelectedItem(pos);
    indices.Add(itemIndex);
  }
  if (indices.IsEmpty())
    FOR_VECTOR (i, _items)
      indices.Add(i);
  ChangeState(group, indices);
}


void CSystemPage::OnLvnKeydownSystemAssociate(NMHDR *pNMHDR, LRESULT *pResult)
{
  LPNMLVKEYDOWN pLVKeyDow = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
  bool ctrl = IsKeyDown(VK_CONTROL);
  bool alt = IsKeyDown(VK_MENU);

  if (alt)
    return;

  if ((ctrl && pLVKeyDow->wVKey == 'A') ||
    (!ctrl && pLVKeyDow->wVKey == VK_MULTIPLY))
  {
    for (int i = 0; i < m_lstAssociate.GetItemCount(); i++)
      m_lstAssociate.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
    return;
  }

  switch (pLVKeyDow->wVKey)
  {
  case VK_SPACE:
  case VK_ADD:
  case VK_SUBTRACT:
  case VK_SEPARATOR:
  case VK_DIVIDE:
#ifndef UNDER_CE
  case VK_OEM_PLUS:
  case VK_OEM_MINUS:
#endif
    if (!ctrl)
    {
      ChangeState(pLVKeyDow->wVKey == VK_SPACE ? 0 : 1);
    }
    break;
  }
  *pResult = 0;
}
