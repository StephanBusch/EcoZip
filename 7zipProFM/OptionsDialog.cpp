// OptionsDialog.cpp

#include "StdAfx.h"

#include "CPP/Common/StringConvert.h"

#include "CPP/Windows/DLL.h"

#include "EditPage.h"
#include "FoldersPage.h"
#include "LangPage.h"
#include "MenuPage.h"
// #include "PluginsPage.h"
#include "SettingsPage.h"
#include "SystemPage.h"
#include "PluginsPage.h"

#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"

#include "7zipProFM.h"
#include "7zipProFMDlg.h"
#include "FMUtils.h"

using namespace NWindows;

#ifndef UNDER_CE
typedef UINT32 (WINAPI * DllRegisterServerPtr)();

extern HWND g_MenuPageHWND;

static void ShowMenuErrorMessage(const wchar_t *m)
{
  MessageBoxW(g_MenuPageHWND, m, L"7-ZipPro", MB_ICONERROR);
}

static int DllRegisterServer2(const char *name)
{
  NDLL::CLibrary lib;

  FString prefix = NDLL::GetModuleDirPrefix();
  if (!lib.Load(prefix + FTEXT("7-zip.dll")))
  {
    ShowMenuErrorMessage(L"7-ZipPro cannot load 7-zip.dll");
    return E_FAIL;
  }
  DllRegisterServerPtr f = (DllRegisterServerPtr)lib.GetProc(name);
  if (f == NULL)
  {
    ShowMenuErrorMessage(L"Incorrect plugin");
    return E_FAIL;
  }
  HRESULT res = f();
  if (res != S_OK)
    ShowMenuErrorMessage(HResultToMessage(res));
  return (int)res;
}

STDAPI DllRegisterServer(void)
{
  #ifdef UNDER_CE
  return S_OK;
  #else
  return DllRegisterServer2("DllRegisterServer");
  #endif
}

STDAPI DllUnregisterServer(void)
{
  #ifdef UNDER_CE
  return S_OK;
  #else
  return DllRegisterServer2("DllUnregisterServer");
  #endif
}

#endif

void C7zipProFMDlg::OnToolsOptions()
{
  CSystemPage systemPage;
#ifdef EXTERNAL_CODECS
  CPluginsPage pluginsPage;
#endif
  CEditPage editPage;
  CSettingsPage settingsPage;
  CLangPage langPage;
  CMenuPage menuPage;
  CFoldersPage foldersPage;

  CMFCPropertySheet sheet;
  UINT pageIDs[] = {
    IDD_SYSTEM,
#ifdef EXTERNAL_CODECS
    IDD_PLUGINS,
#endif
    0, IDD_FOLDERS, IDD_EDIT, IDD_SETTINGS, IDD_LANG };
  FString titles[ARRAY_SIZE(pageIDs)];
  CPropertyPage *pagePinters[] = {
    &systemPage,
#ifdef EXTERNAL_CODECS
    &pluginsPage,
#endif
    &menuPage, &foldersPage, &editPage, &settingsPage, &langPage };
  for (unsigned i = 0; i < ARRAY_SIZE(pagePinters); i++) {
    if (pageIDs[i] > 0) {
      LangString_OnlyFromLangFile(pageIDs[i], titles[i]);
      if (!titles[i].IsEmpty()) {
        pagePinters[i]->m_psp.dwFlags |= PSP_USETITLE;
        pagePinters[i]->m_psp.pszTitle = titles[i];
      }
    }
    sheet.AddPage(pagePinters[i]);
  }

  UString title = LangString(IDS_OPTIONS);
  sheet.SetTitle(GetSystemString(title));
  INT_PTR res = sheet.DoModal();
  if (res > 0 && res != IDCANCEL)
  {
    if (langPage.LangWasChanged)
    {
      // g_App._window.SetText(LangString(IDS_APP_TITLE, 0x03000000));
//       MyLoadMenu();
      theApp.ReloadLang();
    }
    /*
    if (systemPage.WasChanged)
    {
      // probably it doesn't work, since image list is locked?
      g_App.SysIconsWereChanged();
    }
    */

    for (unsigned int i = 0; i < nButtons; i++) {
      if (nButtonIDs[i * 3] > 0)
        pButtons[i]->SetWindowTextW(LangString(nButtonIDs[i * 3]));
    }

    theApp.ShowSystemMenu = Read_ShowSystemMenu();
    m_pContentsView->ReloadSettings();
    m_pContentsView->RefreshListCtrlSaveFocused();
    // ::PostMessage(hwndOwner, kLangWasChangedMessage, 0 , 0);
  }
}
