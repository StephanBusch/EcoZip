// MenuPage.cpp : implementation file
//

#include "stdafx.h"

#include "CPP/Common/StringConvert.h"

#include "CPP/7zip/UI/Common/ZipRegistry.h"

#include "CPP/Windows/ErrorMsg.h"
#include "CPP/Windows/FileFind.h"

#include "CPP/7zip/UI/ExplorerE/ContextMenuFlags.h"
#include "CPP/7zip/UI/ExplorerE/RegistryContextMenu.h"

#include "CPP/7zip/UI/FileManager/HelpUtils.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/FormatUtils.h"

#include "EcoZipFM.h"
#include "MenuPage.h"


using namespace NContextMenuFlags;

static const UInt32 kLangIDs[] =
{
  IDX_SYSTEM_INTEGRATE_TO_MENU,
  IDX_SYSTEM_CASCADED_MENU,
  IDX_SYSTEM_ICON_IN_MENU,
  IDX_EXTRACT_ELIM_DUP,
  IDT_SYSTEM_CONTEXT_MENU_ITEMS
};

static LPCWSTR kSystemTopic = L"fme/options.htm#sevenZip";

struct CContextMenuItem
{
  int ControlID;
  UInt32 Flag;
};

static const CContextMenuItem kMenuItems[] =
{
  { IDS_CONTEXT_OPEN, kOpen },
  { IDS_CONTEXT_OPEN, kOpenAs },
  { IDS_CONTEXT_EXTRACT, kExtract },
  { IDS_CONTEXT_EXTRACT_HERE, kExtractHere },
  { IDS_CONTEXT_EXTRACT_TO, kExtractTo },

  { IDS_CONTEXT_TEST, kTest },

  { IDS_CONTEXT_COMPRESS, kCompress },
  { IDS_CONTEXT_COMPRESS_TO, kCompressTo7z },
  { IDS_CONTEXT_COMPRESS_TO, kCompressTo7ze },
  { IDS_CONTEXT_COMPRESS_TO, kCompressToZip },

  #ifndef UNDER_CE
  { IDS_CONTEXT_COMPRESS_EMAIL, kCompressEmail },
  { IDS_CONTEXT_COMPRESS_TO_EMAIL, kCompressTo7zEmail },
  { IDS_CONTEXT_COMPRESS_TO_EMAIL, kCompressTo7zeEmail },
  { IDS_CONTEXT_COMPRESS_TO_EMAIL, kCompressToZipEmail },
  #endif

  { IDS_PROP_CHECKSUM, kCRC }
};


#if !defined(_WIN64)
extern bool g_Is_Wow64;
#endif

// CMenuPage dialog

IMPLEMENT_DYNAMIC(CMenuPage, CPropertyPage)

CMenuPage::CMenuPage()
: CPropertyPage(CMenuPage::IDD)
{

}

CMenuPage::~CMenuPage()
{
}

void CMenuPage::DoDataExchange(CDataExchange* pDX)
{
  CPropertyPage::DoDataExchange(pDX);
  DDX_Control(pDX, IDL_SYSTEM_OPTIONS, m_lstOptions);
}


BEGIN_MESSAGE_MAP(CMenuPage, CPropertyPage)
  ON_BN_CLICKED(IDX_SYSTEM_INTEGRATE_TO_MENU, &CMenuPage::OnBnClickedSystemIntegrateToMenu)
  ON_BN_CLICKED(IDX_SYSTEM_INTEGRATE_TO_MENU_2, &CMenuPage::OnBnClickedSystemIntegrateToMenu2)
  ON_BN_CLICKED(IDX_SYSTEM_CASCADED_MENU, &CMenuPage::OnBnClickedSystemCascadedMenu)
  ON_BN_CLICKED(IDX_SYSTEM_ICON_IN_MENU, &CMenuPage::OnBnClickedSystemIconInMenu)
  ON_BN_CLICKED(IDX_EXTRACT_ELIM_DUP, &CMenuPage::OnBnClickedExtractElimDup)
  ON_NOTIFY(LVN_ITEMCHANGED, IDL_SYSTEM_OPTIONS, &CMenuPage::OnLvnItemchangedSystemOptions)
END_MESSAGE_MAP()


// CMenuPage message handlers


BOOL CMenuPage::OnInitDialog()
{
  CPropertyPage::OnInitDialog();

  _initMode = true;

  Clear_MenuChanged();
  
  LangSetDlgItems(*this, kLangIDs, ARRAY_SIZE(kLangIDs));

  #ifdef UNDER_CE

  HideItem(IDX_SYSTEM_INTEGRATE_TO_MENU);
  HideItem(IDX_SYSTEM_INTEGRATE_TO_MENU_2);

  #else

  {
    CString s;
    GetDlgItemText(IDX_SYSTEM_INTEGRATE_TO_MENU, s);
    UString bit64 = LangString(IDS_PROP_BIT64);
    if (bit64.IsEmpty())
      bit64.SetFromAscii("64-bit");
    #ifdef _WIN64
      bit64.Replace(L"64", L"32");
    #endif
    s += _T(' ');
    s += _T('(');
    s += GetSystemString(bit64);
    s += _T(')');
    SetDlgItemText(IDX_SYSTEM_INTEGRATE_TO_MENU_2, s);
  }

  const FString prefix = NWindows::NDLL::GetModuleDirPrefix();
  
  _dlls[0].ctrl = IDX_SYSTEM_INTEGRATE_TO_MENU;
  _dlls[1].ctrl = IDX_SYSTEM_INTEGRATE_TO_MENU_2;
  
  _dlls[0].wow = 0;
  _dlls[1].wow =
      #ifdef _WIN64
        KEY_WOW64_32KEY
      #else
        KEY_WOW64_64KEY
      #endif
      ;

  for (unsigned d = 0; d < 2; d++)
  {
    CShellDll &dll = _dlls[d];

    dll.wasChanged = false;

    #ifndef _WIN64
    if (d != 0 && !g_Is_Wow64)
    {
      GetDlgItem(dll.ctrl)->ShowWindow(SW_HIDE);
      continue;
    }
    #endif

    FString &path = dll.Path;
    path = prefix;
    path.AddAscii(d == 0 ? "7-zip.dll" :
        #ifdef _WIN64
          "7-zip32.dll"
        #else
          "7-zip64.dll"
        #endif
        );


    if (!NWindows::NFile::NFind::DoesFileExist(path))
    {
      path.Empty();
      GetDlgItem(dll.ctrl)->EnableWindow(FALSE);
    }
    else
    {
      dll.prevValue = CheckContextMenuHandler(fs2us(path), dll.wow);
      CheckDlgButton(dll.ctrl, dll.prevValue ? BST_CHECKED : BST_UNCHECKED);
    }
  }

  #endif


  CContextMenuInfo ci;
  ci.Load();

  CheckDlgButton(IDX_SYSTEM_CASCADED_MENU, ci.Cascaded.Val ? BST_CHECKED : BST_UNCHECKED);
  CheckDlgButton(IDX_SYSTEM_ICON_IN_MENU, ci.MenuIcons.Val ? BST_CHECKED : BST_UNCHECKED);
  CheckDlgButton(IDX_EXTRACT_ELIM_DUP, ci.ElimDup.Val ? BST_CHECKED : BST_UNCHECKED);

  UInt32 newFlags = LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT;
  m_lstOptions.SetExtendedStyle(newFlags);

  m_lstOptions.InsertColumn(0, _T(""), LVCFMT_LEFT, 100);

  for (unsigned i = 0; i < ARRAY_SIZE(kMenuItems); i++)
  {
    const CContextMenuItem &menuItem = kMenuItems[i];

    UString s = LangString(menuItem.ControlID);
    if (menuItem.Flag == kCRC)
      s.SetFromAscii("CRC SHA");
    if (menuItem.Flag == kOpenAs ||
        menuItem.Flag == kCRC)
      s.AddAscii(" >");

    switch (menuItem.ControlID)
    {
      case IDS_CONTEXT_EXTRACT_TO:
      {
        s = MyFormatNew(s, LangString(IDS_CONTEXT_FOLDER));
        break;
      }
      case IDS_CONTEXT_COMPRESS_TO:
      case IDS_CONTEXT_COMPRESS_TO_EMAIL:
      {
        UString s2 = LangString(IDS_CONTEXT_ARCHIVE);
        switch (menuItem.Flag)
        {
          case kCompressTo7z:
          case kCompressTo7zEmail:
            s2.AddAscii(".7z");
            break;
        case kCompressTo7ze:
        case kCompressTo7zeEmail:
          s2.AddAscii(".7ze");
          break;
          case kCompressToZip:
          case kCompressToZipEmail:
            s2.AddAscii(".zip");
            break;
        }
        s = MyFormatNew(s, s2);
        break;
      }
    }

    int itemIndex = m_lstOptions.InsertItem(i, GetSystemString(s));
    m_lstOptions.SetCheck(itemIndex, ((ci.Flags & menuItem.Flag) != 0));
  }

  m_lstOptions.SetColumnWidth(0, LVSCW_AUTOSIZE);
  _initMode = false;

  return TRUE;  // return TRUE unless you set the focus to a control
  // EXCEPTION: OCX Property Pages should return FALSE
}


#ifndef UNDER_CE

static void ShowMenuErrorMessage(const wchar_t *m, HWND hwnd)
{
  MessageBoxW(hwnd, m, L"EcoZip", MB_ICONERROR);
}

#endif

BOOL CMenuPage::OnApply()
{
  #ifndef UNDER_CE
  
  for (unsigned d = 2; d != 0;)
  {
    d--;
    CShellDll &dll = _dlls[d];
    if (dll.wasChanged && !dll.Path.IsEmpty())
    {
      bool newVal = IsDlgButtonChecked(dll.ctrl) == BST_CHECKED;
      LONG res = SetContextMenuHandler(newVal, fs2us(dll.Path), dll.wow);
      if (res != ERROR_SUCCESS && (dll.prevValue != newVal || newVal))
        ShowMenuErrorMessage(NWindows::NError::MyFormatMessage(res), *this);
      dll.prevValue = CheckContextMenuHandler(fs2us(dll.Path), dll.wow);
      CheckDlgButton(dll.ctrl, dll.prevValue ? BST_CHECKED : BST_UNCHECKED);
      dll.wasChanged = false;
    }
  }

  #endif

  if (_cascaded_Changed || _menuIcons_Changed || _elimDup_Changed || _flags_Changed)
  {
    CContextMenuInfo ci;
    ci.Cascaded.Val = IsDlgButtonChecked(IDX_SYSTEM_CASCADED_MENU) == BST_CHECKED;
    ci.Cascaded.Def = _cascaded_Changed;

    ci.MenuIcons.Val = IsDlgButtonChecked(IDX_SYSTEM_ICON_IN_MENU) == BST_CHECKED;
    ci.MenuIcons.Def = _menuIcons_Changed;
    
    ci.ElimDup.Val = IsDlgButtonChecked(IDX_EXTRACT_ELIM_DUP) == BST_CHECKED;
    ci.ElimDup.Def = _elimDup_Changed;

    ci.Flags = 0;
    
    for (unsigned i = 0; i < ARRAY_SIZE(kMenuItems); i++)
      if (m_lstOptions.GetCheck(i))
        ci.Flags |= kMenuItems[i].Flag;
    
    ci.Flags_Def = _flags_Changed;
    ci.Save();

    Clear_MenuChanged();
  }

  // UnChanged();

  return CPropertyPage::OnApply();
}


BOOL CMenuPage::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
  LPNMHDR pNMHDR = (LPNMHDR)lParam;
  if (pNMHDR->code == PSN_HELP) {
    ShowHelpWindow(NULL, kSystemTopic);
    return TRUE;
  }

  return CPropertyPage::OnNotify(wParam, lParam, pResult);
}


void CMenuPage::OnBnClickedSystemIntegrateToMenu()
{
  for (unsigned d = 0; d < 2; d++)
  {
    CShellDll &dll = _dlls[d];
    if (IDX_SYSTEM_INTEGRATE_TO_MENU == dll.ctrl && !dll.Path.IsEmpty())
      dll.wasChanged = true;
  }
  SetModified();
}


void CMenuPage::OnBnClickedSystemIntegrateToMenu2()
{
  for (unsigned d = 0; d < 2; d++)
  {
    CShellDll &dll = _dlls[d];
    if (IDX_SYSTEM_INTEGRATE_TO_MENU_2 == dll.ctrl && !dll.Path.IsEmpty())
      dll.wasChanged = true;
  }
  SetModified();
}


void CMenuPage::OnBnClickedSystemCascadedMenu()
{
  _cascaded_Changed = true;
  SetModified();
}


void CMenuPage::OnBnClickedSystemIconInMenu()
{
  _menuIcons_Changed = true;
  SetModified();
}

void CMenuPage::OnBnClickedExtractElimDup()
{
  _elimDup_Changed = true;
  SetModified();
}


void CMenuPage::OnLvnItemchangedSystemOptions(NMHDR *pNMHDR, LRESULT *pResult)
{
  LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
  if (_initMode)
    return;
  if ((pNMLV->uChanged & LVIF_STATE) != 0)
  {
    UINT oldState = pNMLV->uOldState & LVIS_STATEIMAGEMASK;
    UINT newState = pNMLV->uNewState & LVIS_STATEIMAGEMASK;
    if (oldState != newState)
      SetModified();
  }
  *pResult = 0;
}
