// MenuPage.cpp : implementation file
//

#include "stdafx.h"

#include "CPP/Common/StringConvert.h"

#include "CPP/7zip/UI/Common/ZipRegistry.h"

#include "CPP/7zip/UI/ExplorerE/ContextMenuFlags.h"
#include "CPP/7zip/UI/ExplorerE/RegistryContextMenu.h"

#include "CPP/7zip/UI/FileManager/HelpUtils.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/FormatUtils.h"

#include "7zipProFM.h"
#include "MenuPage.h"


using namespace NContextMenuFlags;

static const UInt32 kLangIDs[] =
{
  IDX_SYSTEM_INTEGRATE_TO_CONTEXT_MENU,
  IDX_SYSTEM_CASCADED_MENU,
  IDX_SYSTEM_ICON_IN_MENU,
  IDT_SYSTEM_CONTEXT_MENU_ITEMS
};

static LPCWSTR kSystemTopic = L"fme/options.htm#sevenZip";

struct CContextMenuItem
{
  int ControlID;
  UInt32 Flag;
};

static CContextMenuItem kMenuItems[] =
{
  { IDS_CONTEXT_OPEN, kOpen},
  { IDS_CONTEXT_OPEN, kOpenAs},
  { IDS_CONTEXT_EXTRACT, kExtract},
  { IDS_CONTEXT_EXTRACT_HERE, kExtractHere },
  { IDS_CONTEXT_EXTRACT_TO, kExtractTo },

  { IDS_CONTEXT_TEST, kTest},

  { IDS_CONTEXT_COMPRESS, kCompress },
  { IDS_CONTEXT_COMPRESS_TO, kCompressTo7z },
  { IDS_CONTEXT_COMPRESS_TO, kCompressTo7ze },
  { IDS_CONTEXT_COMPRESS_TO, kCompressToZip }

  #ifndef UNDER_CE
  ,
  { IDS_CONTEXT_COMPRESS_EMAIL, kCompressEmail },
  { IDS_CONTEXT_COMPRESS_TO_EMAIL, kCompressTo7zEmail },
  { IDS_CONTEXT_COMPRESS_TO_EMAIL, kCompressTo7zeEmail },
  { IDS_CONTEXT_COMPRESS_TO_EMAIL, kCompressToZipEmail }
  #endif

  , { IDS_PROP_CHECKSUM, kCRC }
};

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
	ON_BN_CLICKED(IDX_SYSTEM_INTEGRATE_TO_CONTEXT_MENU, &CMenuPage::OnBnClickedSystemIntegrateToContextMenu)
	ON_BN_CLICKED(IDX_SYSTEM_CASCADED_MENU, &CMenuPage::OnBnClickedSystemCascadedMenu)
	ON_BN_CLICKED(IDX_SYSTEM_ICON_IN_MENU, &CMenuPage::OnBnClickedSystemIconInMenu)
	ON_NOTIFY(LVN_ITEMCHANGED, IDL_SYSTEM_OPTIONS, &CMenuPage::OnLvnItemchangedSystemOptions)
END_MESSAGE_MAP()


// CMenuPage message handlers


BOOL CMenuPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	_initMode = true;
	LangSetDlgItems(*this, kLangIDs, ARRAY_SIZE(kLangIDs));

#ifdef UNDER_CE
	EnableItem(IDX_SYSTEM_INTEGRATE_TO_CONTEXT_MENU, false);
#else
	CheckDlgButton(IDX_SYSTEM_INTEGRATE_TO_CONTEXT_MENU,
		NZipRootRegistry::CheckContextMenuHandler() ? BST_CHECKED : BST_UNCHECKED);
#endif

	CContextMenuInfo ci;
	ci.Load();

	CheckDlgButton(IDX_SYSTEM_CASCADED_MENU, ci.Cascaded ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDX_SYSTEM_ICON_IN_MENU, ci.MenuIcons ? BST_CHECKED : BST_UNCHECKED);

	UInt32 newFlags = LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT;
	m_lstOptions.SetExtendedStyle(newFlags);

	m_lstOptions.InsertColumn(0, _T(""), LVCFMT_LEFT, 100);

	for (int i = 0; i < ARRAY_SIZE(kMenuItems); i++)
	{
		CContextMenuItem &menuItem = kMenuItems[i];

		UString s = LangString(menuItem.ControlID);
		if (menuItem.Flag == kCRC)
			s = L"CRC SHA";
		if (menuItem.Flag == kOpenAs ||
			menuItem.Flag == kCRC)
			s += L" >";

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
				s2 += L".7z";
				break;
			case kCompressTo7ze:
			case kCompressTo7zeEmail:
				s2 += L".7ze";
				break;
			case kCompressToZip:
			case kCompressToZipEmail:
				s2 += L".zip";
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
HWND g_MenuPageHWND = 0;
#endif

BOOL CMenuPage::OnApply()
{
#ifndef UNDER_CE
	g_MenuPageHWND = *this;
	if (IsDlgButtonChecked(IDX_SYSTEM_INTEGRATE_TO_CONTEXT_MENU) == BST_CHECKED)
	{
		DllRegisterServer();
		NZipRootRegistry::AddContextMenuHandler();
	}
	else
	{
		DllUnregisterServer();
		NZipRootRegistry::DeleteContextMenuHandler();
	}
#endif

	CContextMenuInfo ci;
	ci.Cascaded = IsDlgButtonChecked(IDX_SYSTEM_CASCADED_MENU) == BST_CHECKED;
	ci.MenuIcons = IsDlgButtonChecked(IDX_SYSTEM_ICON_IN_MENU) == BST_CHECKED;
	ci.Flags = 0;
	for (int i = 0; i < ARRAY_SIZE(kMenuItems); i++)
		if (m_lstOptions.GetCheck(i))
			ci.Flags |= kMenuItems[i].Flag;
	ci.Save();

	return CPropertyPage::OnApply();
}


BOOL CMenuPage::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	LPNMHDR pNMHDR = (LPNMHDR) lParam;
	if (pNMHDR->code == PSN_HELP) {
		ShowHelpWindow(NULL, kSystemTopic);
		return TRUE;
	}

	return CPropertyPage::OnNotify(wParam, lParam, pResult);
}


void CMenuPage::OnBnClickedSystemIntegrateToContextMenu()
{
	SetModified();
}


void CMenuPage::OnBnClickedSystemCascadedMenu()
{
	SetModified();
}


void CMenuPage::OnBnClickedSystemIconInMenu()
{
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
