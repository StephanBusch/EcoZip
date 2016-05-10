
// 7zipProFM.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"

#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "7zipProFM.h"
#include "7zipProFMDlg.h"

#include "CPP/Windows/MemoryLock.h"
#include "CPP/Windows/System.h"

#ifndef UNDER_CE
#include "CPP/Windows/SecurityUtils.h"
#endif

#include "FMUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// C7zipProFMApp

BEGIN_MESSAGE_MAP(C7zipProFMApp, CWinAppEx)
  ON_COMMAND(ID_HELP, &CWinAppEx::OnHelp)
END_MESSAGE_MAP()


// C7zipProFMApp construction

C7zipProFMApp::C7zipProFMApp()
{
  // support Restart Manager
  m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;
#ifdef _MANAGED
  // If the application is built using Common Language Runtime support (/clr):
  //     1) This additional setting is needed for Restart Manager support to work properly.
  //     2) In your project, you must add a reference to System.Windows.Forms in order to build.
  System::Windows::Forms::Application::SetUnhandledExceptionMode(System::Windows::Forms::UnhandledExceptionMode::ThrowException);
#endif

  // TODO: replace application ID string below with unique ID string; recommended
  // format for string is CompanyName.ProductName.SubProduct.VersionInformation
  SetAppID(_T("7zipProFM.super2lao.1.0"));

  // TODO: add construction code here,
  // Place all significant initialization in InitInstance
}


// The one and only C7zipProFMApp object

C7zipProFMApp theApp;


HINSTANCE g_hInstance;
UInt64 g_RAM_Size;


#ifndef UNDER_CE

static void SetMemoryLock()
{
  if (!IsLargePageSupported())
    return;
  // if (ReadLockMemoryAdd())
  NWindows::NSecurity::AddLockMemoryPrivilege();

  if (ReadLockMemoryEnable())
    NWindows::NSecurity::EnablePrivilege_LockMemory();
}

bool g_SymLink_Supported = false;

static void Set_SymLink_Supported()
{
  g_SymLink_Supported = false;
  OSVERSIONINFO versionInfo;
  versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
  if (!::GetVersionEx(&versionInfo))
    return;
  if (versionInfo.dwPlatformId != VER_PLATFORM_WIN32_NT || versionInfo.dwMajorVersion < 6)
    return;
  g_SymLink_Supported = true;
  // if (g_SymLink_Supported)
  {
    NWindows::NSecurity::EnablePrivilege_SymLink();
  }
}

#endif

// C7zipProFMApp initialization

BOOL C7zipProFMApp::InitInstance()
{
  g_hInstance = m_hInstance;

  // InitCommonControlsEx() is required on Windows XP if an application
  // manifest specifies use of ComCtl32.dll version 6 or later to enable
  // visual styles.  Otherwise, any window creation will fail.
  INITCOMMONCONTROLSEX InitCtrls;
  InitCtrls.dwSize = sizeof(InitCtrls);
  // Set this to include all the common control classes you want to use
  // in your application.
  InitCtrls.dwICC = ICC_WIN95_CLASSES;
  InitCommonControlsEx(&InitCtrls);

  CWinAppEx::InitInstance();

  // Initialize OLE libraries
  if (!AfxOleInit())
  {
    AfxMessageBox(IDP_OLE_INIT_FAILED);
    return FALSE;
  }
  CoInitialize(NULL);
  if (FAILED(CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL)))
    return FALSE;

  AfxEnableControlContainer();

  // Create the shell manager, in case the dialog contains
  // any shell tree view or shell list view controls.
  InitShellManager();

  // Activate "Windows Native" visual manager for enabling themes in MFC controls
  CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows7));

  // Standard initialization
  // If you are not using these features and wish to reduce the size
  // of your final executable, you should remove from the following
  // the specific initialization routines you do not need
  // Change the registry key under which our settings are stored
  // TODO: You should modify this string to be something appropriate
  // such as the name of your company or organization
  m_strRegSection.Empty();
  SetRegistryKey(_T(""));// _T("super2lao@gmail.com-7zipPro"));

  InitContextMenuManager();

  InitKeyboardManager();

  InitTooltipManager();
  CMFCToolTipInfo ttParams;
  ttParams.m_bVislManagerTheme = TRUE;
  theApp.GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL,
    RUNTIME_CLASS(CMFCToolTipCtrl), &ttParams);

  LoadState();
//   DWORD dwRouting;
//   if (SystemParametersInfo(SPI_GETMOUSEWHEELROUTING, 0, &dwRouting, 0)) {
// 	  DWORD dwNewRouting = MOUSEWHEEL_ROUTING_HYBRID;
// 	  SystemParametersInfo(SPI_SETMOUSEWHEELROUTING, 0, &dwNewRouting, 0);
//   }

  g_RAM_Size = NWindows::NSystem::GetRamSize();

  SetLargePageSize();

  LoadLangOneTime();

#if defined(_WIN32) && !defined(UNDER_CE)
  SetMemoryLock();
  Set_SymLink_Supported();
#endif

  ReloadLang();

  C7zipProFMDlg dlg;
  InitWindow(&dlg);

  m_pMainWnd = &dlg;
  INT_PTR nResponse = dlg.DoModal();
  if (nResponse == IDOK)
  {
    // TODO: Place code here to handle when the dialog is
    //  dismissed with OK
  }
  else if (nResponse == IDCANCEL)
  {
    // TODO: Place code here to handle when the dialog is
    //  dismissed with Cancel
  }
  else if (nResponse == -1)
  {
    TRACE(traceAppMsg, 0, "Warning: dialog creation failed, so application is terminating unexpectedly.\n");
    TRACE(traceAppMsg, 0, "Warning: if you are using MFC controls on the dialog, you cannot #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS.\n");
  }

  SaveState();
  CoUninitialize();

//   SystemParametersInfo(SPI_SETMOUSEWHEELROUTING, 0, &dwRouting, 0);

  // Since the dialog has been closed, return FALSE so that we exit the
  //  application, rather than start the application's message pump.
  return FALSE;
}


void C7zipProFMApp::PreLoadState()
{
  BOOL bNameValid;
  CString strName;
  bNameValid = strName.LoadString(IDR_MAINFRAME);
  ASSERT(bNameValid);
  GetContextMenuManager()->AddMenu(strName, IDR_MAINFRAME);
  GetContextMenuManager()->AddMenu(strName, IDR_MENU_POPUP);
  GetContextMenuManager()->AddMenu(strName, IDR_MENU_FOLDER);

  ShowSystemMenu = Read_ShowSystemMenu();

  return CWinAppEx::PreLoadState();
}


void C7zipProFMApp::InitWindow(CDialog * pDlg)
{
  C7zipProFMDlg &dlg = *(C7zipProFMDlg *)pDlg;

}


BOOL C7zipProFMApp::SaveState(LPCTSTR lpszSectionName, CFrameImpl* pFrameImpl)
{
  Save_ShowSystemMenu(ShowSystemMenu);

  return CWinAppEx::SaveState(lpszSectionName, pFrameImpl);
}


void C7zipProFMApp::ReloadLang()
{
  LangString(IDS_N_SELECTED_ITEMS, LangString_N_SELECTED_ITEMS);
}
