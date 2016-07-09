
// EcoZipFM.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"

#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "EcoZipFM.h"
#include "EcoZipFMDlg.h"

#include "CPP/Windows/MemoryLock.h"
#include "CPP/Windows/System.h"

#ifndef UNDER_CE
#include "CPP/Windows/SecurityUtils.h"
#endif

#include "FMUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#if defined(_WIN32) && !defined(_WIN64) && !defined(UNDER_CE)

bool g_Is_Wow64;

typedef BOOL (WINAPI *Func_IsWow64Process)(HANDLE, PBOOL);

static void Set_Wow64()
{
  g_Is_Wow64 = false;
  Func_IsWow64Process fnIsWow64Process = (Func_IsWow64Process)GetProcAddress(
      GetModuleHandleA("kernel32.dll"), "IsWow64Process");
  if (fnIsWow64Process)
  {
    BOOL isWow;
    if (fnIsWow64Process(GetCurrentProcess(), &isWow))
      g_Is_Wow64 = (isWow != FALSE);
  }
}

#endif


// CEcoZipFMApp

BEGIN_MESSAGE_MAP(CEcoZipFMApp, CWinAppEx)
  ON_COMMAND(ID_HELP, &CWinAppEx::OnHelp)
END_MESSAGE_MAP()


// CEcoZipFMApp construction

CEcoZipFMApp::CEcoZipFMApp()
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
  SetAppID(_T("EcoZipFM.super2lao.1.0"));

  // TODO: add construction code here,
  // Place all significant initialization in InitInstance
}


// The one and only CEcoZipFMApp object

CEcoZipFMApp theApp;


HINSTANCE g_hInstance;
bool g_RAM_Size_Defined;
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
  OSVERSIONINFO vi;
  vi.dwOSVersionInfoSize = sizeof(vi);
  if (!::GetVersionEx(&vi))
    return;
  if (vi.dwPlatformId != VER_PLATFORM_WIN32_NT || vi.dwMajorVersion < 6)
    return;
  g_SymLink_Supported = true;
  // if (g_SymLink_Supported)
  {
    NWindows::NSecurity::EnablePrivilege_SymLink();
  }
}

#endif

// CEcoZipFMApp initialization

BOOL CEcoZipFMApp::InitInstance()
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
  SetRegistryKey(_T(""));// _T("super2lao@gmail.com-EcoZip"));

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

  g_RAM_Size_Defined = NWindows::NSystem::GetRamSize(g_RAM_Size);

  SetLargePageSize();

  LoadLangOneTime();


  #if defined(_WIN32) && !defined(_WIN64) && !defined(UNDER_CE)
  Set_Wow64();
  #endif

#if defined(_WIN32) && !defined(UNDER_CE)
  SetMemoryLock();
  Set_SymLink_Supported();
#endif

  ReloadLang();

  CEcoZipFMDlg dlg;
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


void CEcoZipFMApp::PreLoadState()
{
  BOOL bNameValid;
  CString strName;
  bNameValid = strName.LoadString(IDR_MAINFRAME);
  ASSERT(bNameValid);
  GetContextMenuManager()->AddMenu(strName, IDR_MAINFRAME);
  GetContextMenuManager()->AddMenu(strName, IDR_MENU_POPUP);
  GetContextMenuManager()->AddMenu(strName, IDR_MENU_FOLDER);

  CFmSettings st;
  st.Load();

  ShowSystemMenu = st.ShowSystemMenu;

  return CWinAppEx::PreLoadState();
}


void CEcoZipFMApp::InitWindow(CDialog * pDlg)
{
  CEcoZipFMDlg &dlg = *(CEcoZipFMDlg *)pDlg;
}


BOOL CEcoZipFMApp::SaveState(LPCTSTR lpszSectionName, CFrameImpl* pFrameImpl)
{
  return CWinAppEx::SaveState(lpszSectionName, pFrameImpl);
}


void CEcoZipFMApp::ReloadLang()
{
  LangString(IDS_N_SELECTED_ITEMS, LangString_N_SELECTED_ITEMS);
}
