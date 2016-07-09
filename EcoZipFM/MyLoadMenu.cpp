// MyLoadMenu.cpp

#include "StdAfx.h"

#include "CPP/Windows/Menu.h"

#include "CPP/7zip/PropID.h"

#include "CPP/7zip/UI/Common/CompressCall.h"

// #include "AboutDialog.h"
#include "CPP/7zip/UI/FileManager/HelpUtils.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/resource.h"
#include "MyLoadMenu.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"

#include "EcoZipFM.h"

using namespace NWindows;

enum
{
  kMenuIndex_File = 0,
  kMenuIndex_Edit,
  kMenuIndex_View,
};

static const UInt32 kTopMenuLangIDs[] = { 500, 501, 502, /*503, */504, 505 };

static const UInt32 kAddToFavoritesLangID = 800;
static const UInt32 kToolbarsLangID = 733;

static const CIDLangPair kIDLangPairs[] =
{
  { IDCLOSE, 557 },
  { ID_VIEW_ARRANGE_BY_NAME, 1004 },
  { ID_VIEW_ARRANGE_BY_TYPE, 1020 },
  { ID_VIEW_ARRANGE_BY_DATE, 1012 },
  { ID_VIEW_ARRANGE_BY_SIZE, 1007 },

  { ID_FILE_OPEN, IDM_OPEN },
  { ID_FILE_OPEN_INSIDE, IDM_OPEN_INSIDE },
  { ID_FILE_OPEN_OUTSIDE, IDM_OPEN_OUTSIDE },
  { ID_FILE_VIEW, IDM_FILE_VIEW },
  { ID_FILE_EDIT, IDM_FILE_EDIT },
  { ID_FILE_RENAME, IDM_RENAME },
  { ID_FILE_COPY_TO, IDM_COPY_TO },
  { ID_FILE_MOVE_TO, IDM_MOVE_TO },
  { ID_FILE_DELETE, IDM_DELETE },
  { ID_FILE_SPLIT_FILE, IDM_SPLIT },
  { ID_FILE_COMBINE_FILES, IDM_COMBINE },
  { ID_FILE_PROPERTIES, IDM_PROPERTIES },
  { ID_FILE_COMMENT, IDM_COMMENT },
  { ID_CRC_CRC32, IDM_CRC32 },
  { ID_CRC_CRC64, IDM_CRC64 },
  { ID_CRC_SHA1, IDM_SHA1 },
  { ID_CRC_SHA256, IDM_SHA256 },
  { ID_CRC_HASH_ALL, IDM_HASH_ALL },
  { ID_FILE_DIFF, IDM_DIFF },
  { ID_FILE_CREATE_FOLDER, IDM_CREATE_FOLDER },
  { ID_FILE_CREATE_FILE, IDM_CREATE_FILE },
  { ID_FILE_LINK, IDM_LINK },
  { ID_APP_EXIT, 557 },

  { ID_EDIT_SELECT_ALL, IDM_SELECT_ALL },
  { ID_EDIT_DESELECT_ALL, IDM_DESELECT_ALL },
  { ID_EDIT_INVERT_SELECTION, IDM_INVERT_SELECTION },
  { ID_EDIT_SELECT_WITH_MASK, IDM_SELECT },
  { ID_EDIT_DESELECT_WITH_MASK, IDM_DESELECT },
  { ID_EDIT_SELECT_BY_TYPE, IDM_SELECT_BY_TYPE },
  { ID_EDIT_DESELECT_BY_TYPE, IDM_DESELECT_BY_TYPE },

  { ID_VIEW_LARGEICON, IDM_VIEW_LARGE_ICONS },
  { ID_VIEW_SMALLICON, IDM_VIEW_SMALL_ICONS },
  { ID_VIEW_LIST, IDM_VIEW_LIST },
  { ID_VIEW_DETAILS, IDM_VIEW_DETAILS },
  { ID_VIEW_ARRANGE_NO_SORT, IDM_VIEW_ARANGE_NO_SORT },

  { ID_VIEW_OPEN_ROOT_FOLDER, IDM_OPEN_ROOT_FOLDER },
  { ID_VIEW_OPEN_PARENT_FOLDER, IDM_OPEN_PARENT_FOLDER },
  { ID_VIEW_FOLDERS_HISTORY, IDM_FOLDERS_HISTORY },
  { ID_VIEW_REFRESH, IDM_VIEW_REFRESH },
  { ID_VIEW_AUTO_REFRESH, IDM_VIEW_AUTO_REFRESH },

  { ID_TOOLS_OPTIONS, IDM_OPTIONS },
  { ID_TOOLS_BENCHMARK, IDM_BENCHMARK },
  { ID_APP_ABOUT, IDM_ABOUT },
};

static int FindLangItem(unsigned controlID)
{
  for (unsigned i = 0; i < ARRAY_SIZE(kIDLangPairs); i++)
    if (kIDLangPairs[i].ControlID == controlID)
      return i;
  return -1;
}

static inline UINT Get_fMask_for_String() { return MIIM_TYPE; }
static inline UINT Get_fMask_for_FType_and_String() { return MIIM_TYPE; }

void MyChangeMenu(HMENU menuLoc, int level, int menuIndex)
{
  NWindows::CMenu menu;
  menu.Attach(menuLoc);
  for (int i = 0;; i++)
  {
    CMenuItem item;
    item.fMask = Get_fMask_for_String() | MIIM_SUBMENU | MIIM_ID;
    item.fType = MFT_STRING;
    if (!menu.GetItem(i, true, item))
      break;
    {
      UString newString;
      if (item.hSubMenu)
      {
        UInt32 langID = 0;
//         if (level == 1 && menuIndex == kMenuIndex_Bookmarks)
//           langID = kAddToFavoritesLangID;
//         else
        {
          MyChangeMenu(item.hSubMenu, level + 1, i);
          if (level == 1 && menuIndex == kMenuIndex_View)
            langID = kToolbarsLangID;
          else if (level == 0 && i < ARRAY_SIZE(kTopMenuLangIDs))
            langID = kTopMenuLangIDs[i];
          else
            continue;
        }
        
        LangString_OnlyFromLangFile(langID, newString);
        
        if (newString.IsEmpty())
          continue;
      }
      else
      {
        if (item.IsSeparator())
          continue;
        int langPos = FindLangItem(item.wID);

        // we don't need lang change for CRC items!!!

        UInt32 langID = langPos >= 0 ? kIDLangPairs[langPos].LangID : item.wID;
        
        if (langID == IDM_OPEN_INSIDE_ONE || langID == IDM_OPEN_INSIDE_PARSER)
        {
          LangString_OnlyFromLangFile(IDM_OPEN_INSIDE, newString);
          if (newString.IsEmpty())
            continue;
          newString.Replace(L"&", L"");
          int tabPos = newString.Find(L"\t");
          if (tabPos >= 0)
            newString.DeleteFrom(tabPos);
          newString += (langID == IDM_OPEN_INSIDE_ONE ? L" *" : L" #");
        }
        else
          LangString_OnlyFromLangFile(langID, newString);

        if (newString.IsEmpty())
          continue;

        int tabPos = item.StringValue.ReverseFind(L'\t');
        if (tabPos >= 0)
          newString += item.StringValue.Ptr(tabPos);
      }

      {
        item.StringValue = newString;
        item.fMask = Get_fMask_for_String();
        item.fType = MFT_STRING;
        menu.SetItem(i, true, item);
      }
    }
  }
}

static void CopyMenu(HMENU srcMenuSpec, HMENU destMenuSpec);

void CopyPopMenu_IfRequired(CMenuItem &item)
{
  if (item.hSubMenu)
  {
    NWindows::CMenu popup;
    popup.CreatePopup();
    CopyMenu(item.hSubMenu, popup);
    item.hSubMenu = popup;
  }
}

static void CopyMenu(HMENU srcMenuSpec, HMENU destMenuSpec)
{
  NWindows::CMenu srcMenu;
  srcMenu.Attach(srcMenuSpec);
  NWindows::CMenu destMenu;
  destMenu.Attach(destMenuSpec);
  int startPos = 0;
  for (int i = 0;; i++)
  {
    CMenuItem item;
    item.fMask = MIIM_SUBMENU | MIIM_STATE | MIIM_ID | Get_fMask_for_FType_and_String();
    item.fType = MFT_STRING;

    if (!srcMenu.GetItem(i, true, item))
      break;

    CopyPopMenu_IfRequired(item);
    if (destMenu.InsertItem(startPos, true, item))
      startPos++;
  }
}

/*
It doesn't help
void OnMenuUnActivating(HWND hWnd, HMENU hMenu, int id)
{
  if (::GetSubMenu(::GetMenu(g_HWND), 0) != hMenu)
    return;
}
*/

void CFileMenu::Load(HMENU hMenu, unsigned startPos)
{
  NWindows::CMenu destMenu;
  destMenu.Attach(hMenu);

  UString diffPath;
  ReadRegDiff(diffPath);
  
  HMENU hFileMenu = theApp.GetContextMenuManager()->GetMenuById(IDR_MAINFRAME);
  hFileMenu = GetSubMenu(hFileMenu, 0);
  NWindows::CMenu fileMenu;
  fileMenu.Attach(hFileMenu);

  unsigned numRealItems = startPos;
  
  for (unsigned i = 0;; i++)
  {
    CMenuItem item;

    item.fMask = MIIM_SUBMENU | MIIM_STATE | MIIM_ID | Get_fMask_for_FType_and_String();
    item.fType = MFT_STRING;
    
    if (!fileMenu.GetItem(i, true, item))
      break;
    
    {
      if (!programMenu && item.wID == IDCLOSE)
        continue;

      if (item.wID == ID_FILE_DIFF && diffPath.IsEmpty())
        continue;

      if (item.wID == IDM_OPEN_INSIDE_ONE || item.wID == IDM_OPEN_INSIDE_PARSER)
      {
        // We use diff as "super mode" marker for additional commands.
        if (diffPath.IsEmpty())
          continue;
      }

      bool isOneFsFile = (isFsFolder && numItems == 1 && allAreFiles);
      bool disable = (!isOneFsFile && (item.wID == ID_FILE_SPLIT_FILE || item.wID == ID_FILE_COMBINE_FILES));

      if (readOnly)
      {
        switch (item.wID)
        {
          case IDM_RENAME:
          case IDM_MOVE_TO:
          case IDM_DELETE:
          case IDM_COMMENT:
          case IDM_CREATE_FOLDER:
          case IDM_CREATE_FILE:
            disable = true;
        }
      }

      if (item.wID == IDM_LINK && numItems != 1)
        disable = true;

      if (item.wID == IDM_ALT_STREAMS)
        disable = !isAltStreamsSupported;

      if (disable || item.IsSeparator())
        continue;

      CopyPopMenu_IfRequired(item);
      if (destMenu.InsertItem(startPos, true, item))
      {
        if (disable)
          destMenu.EnableItem(startPos, MF_BYPOSITION | MF_GRAYED);
        startPos++;
      }

      if (!item.IsSeparator())
        numRealItems = startPos;
    }
  }
  
  destMenu.RemoveAllItemsFrom(numRealItems);
}

// static void MyBenchmark(bool totalMode)
// {
//   CPanel::CDisableTimerProcessing disableTimerProcessing1(g_App.Panels[0]);
//   CPanel::CDisableTimerProcessing disableTimerProcessing2(g_App.Panels[1]);
//   Benchmark(totalMode);
// }
