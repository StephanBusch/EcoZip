
// EcoZipFM.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols
#include "RegistryPath.h"


// CEcoZipFMApp:
// See EcoZipFM.cpp for the implementation of this class
//

class CEcoZipFMApp : public CWinAppEx
{
public:
  CEcoZipFMApp();

  // Attributes
public:
  bool ShowSystemMenu;
  UString LangString_N_SELECTED_ITEMS;

  // Overrides
public:
  virtual BOOL InitInstance();
  virtual void PreLoadState();
  virtual BOOL SaveState(LPCTSTR lpszSectionName = NULL, CFrameImpl* pFrameImpl = NULL);
  void ReloadLang();
protected:
  virtual void InitWindow(CDialog * pDlg);

public:

  // Implementation

  DECLARE_MESSAGE_MAP()
};

#if !defined(_WIN32) || defined(UNDER_CE)
#define ROOT_FS_FOLDER L"\\"
#else
#define ROOT_FS_FOLDER L"C:\\"
#endif

#include "C/7zTypes.h"

extern CEcoZipFMApp theApp;
extern HWND g_HWND;
extern UInt64 g_RAM_Size;
