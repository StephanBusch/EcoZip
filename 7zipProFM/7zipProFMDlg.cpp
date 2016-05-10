
// 7zipProFMDlg.cpp : implementation file
//

#include "stdafx.h"
#include "7zipProFM.h"
#include "7zipProFMDlg.h"
#include "FolderView.h"
#include "DetailView.h"
#include "ContentsView.h"
#include "NullFrame.h"
#include "Messages.h"

#include <../src/mfc/afximpl.h>

#include "CPP/7zip/MyVersion.h"

#include "CPP/Common/IntToString.h"
#include "CPP/Common/StringConvert.h"
#include "CPP/Common/Wildcard.h"

#include "CPP/7zip/UI/FileManager/HelpUtils.h"
#include "CPP/7zip/UI/FileManager/SysIconUtils.h"
#include "CompressCall.h"
#include "CPP/7zip/UI/Common/PropIDUtils.h"
// #include "CPP/7zip/PropID.h"

#include "CPP/7zip/UI/FileManager/FormatUtils.h"

#include "ImageUtils.h"
#include "FMUtils.h"
#include "MyLoadMenu.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


using namespace NWindows;

static LPCWSTR kFMHelpTopic = L"FME/index.htm";


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
  CAboutDlg();

  // Dialog Data
  enum { IDD = IDD_ABOUTBOX };

protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  virtual BOOL OnInitDialog();

  // Implementation
protected:
  DECLARE_MESSAGE_MAP()
public:
  afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
  DWORD installed = 0;
  AddResourceFont(MAKEINTRESOURCE(IDF_ALDO_PC), &installed);
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
  ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

static const UInt32 kLangIDs[] =
{
  IDT_ABOUT_INFO
};

static LPCTSTR kHomePageURL = TEXT("http://www.7-zip.org/");
static LPCWSTR kHelpTopic = L"start.htm";

#define LLL_(quote) L##quote
#define LLL(quote) LLL_(quote)

BOOL CAboutDlg::OnInitDialog()
{
  CDialogEx::OnInitDialog();

  LangSetDlgItems(*this, kLangIDs, ARRAY_SIZE(kLangIDs));
  UString s = L"7-ZipPro " LLL(MY_VERSION);
#ifdef MY_CPU_64BIT
  s += L" [";
  s += LangString(IDS_PROP_BIT64);
  s += L']';
#endif

  SetDlgItemText(IDT_ABOUT_VERSION, s);
  //SetDlgItemText(IDT_ABOUT_DATE, LLL(MY_DATE));

  LangSetWindowText(*this, IDD_ABOUTBOX);

  return TRUE;
}


// C7zipProFMDlg dialog


HWND g_HWND = NULL;

C7zipProFMDlg::C7zipProFMDlg(CWnd* pParent /*=NULL*/)
  : CFileManagerDlg(C7zipProFMDlg::IDD, pParent)
  , m_strFolderPrefix(_T(""))
  , m_pFolderView(NULL)
  , m_pDetailView(NULL)
  , m_pContentsView(NULL)
  , m_bTracking(FALSE)
  , m_nHistoryPos(0)
  , m_nContentsViewStyle(3)
{
  m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

  static CButtonST *pButtons[] = {
    &m_btnMenu,
    //&m_btnSkin,
    &m_btnMinimize,
    &m_btnMaximize,
    &m_btnClose,
    &m_btnAddToArchive,
    &m_btnExtractFiles,
    &m_btnTestArchive,
    &m_btnCopyTo,
    &m_btnMoveTo,
    &m_btnDelete,
    &m_btnProperties,
    &m_btnBenchmark,
    &m_btnNavBackward,
    &m_btnNavForward,
    &m_btnNavBackTo,
    &m_btnNavUp,
    &m_btnSwitchViewStyle,
    &m_btnViewStyle,
  };
  this->pButtons = pButtons;
  static UINT nButtonIDs[] = {
    0, IDB_SYS_MENU, IDB_SYS_MENU_H,
    //0,                      IDB_SYS_SKIN,         IDB_SYS_SKIN_H,
    0, IDB_SYS_MINIMIZE, IDB_SYS_MINIMIZE_H,
    0, IDB_SYS_MAXIMIZE, IDB_SYS_MAXIMIZE_H,
    0, IDB_SYS_CLOSE, IDB_SYS_CLOSE_H,
    IDS_ADD, IDB_ADD_TO_ARCHIVE, IDB_ADD_TO_ARCHIVE_H,
    IDS_EXTRACT, IDB_EXTRACT_FILES, IDB_EXTRACT_FILES_H,
    IDS_TEST, IDB_TEST_ARCHIVE, IDB_TEST_ARCHIVE_H,
    IDS_BUTTON_COPY, IDB_COPY_TO, IDB_COPY_TO_H,
    IDS_BUTTON_MOVE, IDB_MOVE_TO, IDB_MOVE_TO_H,
    IDS_BUTTON_DELETE, IDB_DELETE_N, IDB_DELETE_H,
    IDS_BUTTON_INFO, IDB_PROPERTIES, IDB_PROPERTIES_H,
    IDD_BENCH, IDB_BENCHMARK, IDB_BENCHMARK_H,
    0, IDB_NAV_BACKWARD, IDB_NAV_BACKWARD_H,
    0, IDB_NAV_FORWARD, IDB_NAV_FORWARD_H,
    0, IDB_DOWN_ARROW, IDB_DOWN_ARROW_H,
    0, IDB_NAV_UP, IDB_NAV_UP_H,
    0, IDB_VIEW_LIST, IDB_VIEW_LIST_H,
    0, IDB_DOWN_ARROW, IDB_DOWN_ARROW_H,
  };
  this->nButtonIDs = nButtonIDs;
  this->nButtons = sizeof(pButtons) / sizeof(pButtons[0]);
}

C7zipProFMDlg::~C7zipProFMDlg()
{
}


void C7zipProFMDlg::DoDataExchange(CDataExchange* pDX)
{
  CFileManagerDlg::DoDataExchange(pDX);
  DDX_Control(pDX, IDC_BTN_MENU, m_btnMenu);
  //DDX_Control(pDX, IDC_BTN_SKIN, m_btnSkin);
  DDX_Control(pDX, IDC_BTN_MINIMIZE, m_btnMinimize);
  DDX_Control(pDX, IDC_BTN_MAXIMIZE, m_btnMaximize);
  DDX_Control(pDX, IDC_BTN_CLOSE, m_btnClose);
  DDX_Control(pDX, IDC_BTN_ADD_TO_ARCHIVE, m_btnAddToArchive);
  DDX_Control(pDX, IDC_BTN_EXTRACT_FILES, m_btnExtractFiles);
  DDX_Control(pDX, IDC_BTN_TEST_ARCHIVE, m_btnTestArchive);
  DDX_Control(pDX, IDC_BTN_COPY_TO, m_btnCopyTo);
  DDX_Control(pDX, IDC_BTN_MOVE_TO, m_btnMoveTo);
  DDX_Control(pDX, IDC_BTN_DELETE, m_btnDelete);
  DDX_Control(pDX, IDC_BTN_PROPERTIES, m_btnProperties);
  DDX_Control(pDX, IDC_BTN_BENCHMARK, m_btnBenchmark);
  DDX_Control(pDX, IDC_BTN_NAV_BACKWARD, m_btnNavBackward);
  DDX_Control(pDX, IDC_BTN_NAV_FORWARD, m_btnNavForward);
  DDX_Control(pDX, IDC_BTN_NAV_BACK_TO, m_btnNavBackTo);
  DDX_Control(pDX, IDC_BTN_NAV_UP, m_btnNavUp);
  DDX_Control(pDX, IDC_BTN_SWITCH_VIEW_STYLE, m_btnSwitchViewStyle);
  DDX_Control(pDX, IDC_BTN_VIEW_STYLE, m_btnViewStyle);
  DDX_Control(pDX, IDC_CMB_PATH, m_cmbPath);
  DDX_CBString(pDX, IDC_CMB_PATH, m_strFolderPrefix);
}


#define HISTORY_CMD_FIRST 10000
#define HISTORY_CMD_LAST 10099

BEGIN_MESSAGE_MAP(C7zipProFMDlg, CFileManagerDlg)
  ON_WM_CREATE()
  ON_WM_ENTERSIZEMOVE()
  ON_WM_EXITSIZEMOVE()
  ON_WM_SYSCOMMAND()
  ON_UPDATE_COMMAND_UI_RANGE(HISTORY_CMD_FIRST, HISTORY_CMD_LAST, &C7zipProFMDlg::OnUpdateHistory)
  ON_BN_CLICKED(IDC_BTN_MENU, &C7zipProFMDlg::OnBnClickedBtnMenu)
  ON_BN_CLICKED(IDC_BTN_SKIN, &C7zipProFMDlg::OnBnClickedBtnSkin)
  ON_BN_CLICKED(IDC_BTN_MINIMIZE, &C7zipProFMDlg::OnBnClickedBtnMinimize)
  ON_BN_CLICKED(IDC_BTN_MAXIMIZE, &C7zipProFMDlg::OnBnClickedBtnMaximize)
  ON_BN_CLICKED(IDC_BTN_CLOSE, &C7zipProFMDlg::OnBnClickedBtnClose)

  ON_UPDATE_COMMAND_UI_RANGE(IDC_BTN_ADD_TO_ARCHIVE, IDC_BTN_PROPERTIES, &C7zipProFMDlg::OnUpdateItemSelected)
  ON_UPDATE_COMMAND_UI_RANGE(ID_7ZE_OPEN_ARCHIVE, ID_7ZE_COMPRESS_AND_EMAIL, &C7zipProFMDlg::OnUpdateItemSelected)
  ON_UPDATE_COMMAND_UI_RANGE(ID_FILE_OPEN_INSIDE, ID_FILE_COMMENT, &C7zipProFMDlg::OnUpdateItemSelected)
  ON_UPDATE_COMMAND_UI(ID_EDIT_DESELECT_ALL, &C7zipProFMDlg::OnUpdateItemSelected)
  ON_UPDATE_COMMAND_UI(ID_EDIT_DESELECT_WITH_MASK, &C7zipProFMDlg::OnUpdateItemSelected)
  ON_UPDATE_COMMAND_UI(ID_EDIT_DESELECT_BY_TYPE, &C7zipProFMDlg::OnUpdateItemSelected)
  ON_UPDATE_COMMAND_UI_RANGE(IDC_BTN_NAV_BACKWARD, IDC_BTN_NAV_FORWARD, &C7zipProFMDlg::OnUpdateRange)
  ON_UPDATE_COMMAND_UI(IDC_BTN_NAV_BACK_TO, &C7zipProFMDlg::OnUpdateRange)
  ON_UPDATE_COMMAND_UI(IDC_BTN_NAV_UP, &C7zipProFMDlg::OnUpdateRange)
  ON_UPDATE_COMMAND_UI(ID_VIEW_AUTO_REFRESH, &C7zipProFMDlg::OnUpdateRange)

  ON_COMMAND_RANGE(IDM_ABOUTBOX, IDM_ABOUTBOX, &C7zipProFMDlg::OnCommandRange)
  ON_COMMAND_RANGE(ID_APP_ABOUT, ID_APP_ABOUT, &C7zipProFMDlg::OnCommandRange)
  ON_COMMAND_RANGE(ID_MENU_BEGIN, ID_MENU_END, &C7zipProFMDlg::OnCommandRange)
  ON_COMMAND_RANGE(ID_EDIT_SELECT_ALL, ID_EDIT_SELECT_ALL, &C7zipProFMDlg::OnCommandRange)
  ON_COMMAND_RANGE(IDC_BTN_ADD_TO_ARCHIVE, IDC_BTN_BENCHMARK, &C7zipProFMDlg::OnCommandRange)
  ON_COMMAND_RANGE(IDC_BTN_BENCHMARK, IDC_BTN_BENCHMARK, &C7zipProFMDlg::OnCommandRange)
  ON_COMMAND_RANGE(IDC_BTN_NAV_BACKWARD, IDC_BTN_NAV_FORWARD, &C7zipProFMDlg::OnCommandRange)
  ON_COMMAND_RANGE(IDC_BTN_NAV_UP, IDC_BTN_NAV_UP, &C7zipProFMDlg::OnCommandRange)

  ON_WM_CONTEXTMENU()

  ON_BN_CLICKED(IDC_BTN_NAV_BACK_TO, &C7zipProFMDlg::OnBnClickedBtnNavBackTo)
  ON_BN_CLICKED(IDC_BTN_SWITCH_VIEW_STYLE, &C7zipProFMDlg::OnBnClickedBtnSwitchViewStyle)
  ON_BN_CLICKED(IDC_BTN_VIEW_STYLE, &C7zipProFMDlg::OnBnClickedBtnViewStyle)
  ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_SMALLICON, ID_VIEW_DETAILS, &C7zipProFMDlg::OnUpdateViewStyle)
  ON_COMMAND_RANGE(ID_VIEW_SMALLICON, ID_VIEW_DETAILS, &C7zipProFMDlg::OnViewStyle)
  ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_ARRANGE_BY_NAME, ID_VIEW_ARRANGE_NO_SORT, &C7zipProFMDlg::OnUpdateViewArrange)

  ON_COMMAND(ID_CMB_DROPDOWN, &C7zipProFMDlg::OnCmbDropdown)
  ON_NOTIFY(CBEN_ENDEDIT, IDC_CMB_PATH, &C7zipProFMDlg::OnCbenEndeditCmbPath)
  ON_CBN_DROPDOWN(IDC_CMB_PATH, &C7zipProFMDlg::OnCbnDropdownCmbPath)
  ON_CBN_SELENDOK(IDC_CMB_PATH, &C7zipProFMDlg::OnCbnSelendokCmbPath)
  ON_MESSAGE(kFolderChanged, &C7zipProFMDlg::OnFolderChanged)
  ON_COMMAND(ID_NEXT_PANE, &C7zipProFMDlg::OnNextPane)
  ON_COMMAND(ID_TOOLS_OPTIONS, &C7zipProFMDlg::OnToolsOptions)
  ON_MESSAGE(kRefresh_StatusBar, &C7zipProFMDlg::OnRefresh_StatusBar)
END_MESSAGE_MAP()


// C7zipProFMDlg message handlers


static UINT BASED_CODE indicators[] = {
  IDS_STATUS_PANE1, IDS_STATUS_PANE2,
  IDS_STATUS_PANE3, IDS_STATUS_PANE4,
};
static int indicator_sizes[] = { 250, 200, 200, 250 };

int C7zipProFMDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
  if (CFileManagerDlg::OnCreate(lpCreateStruct) == -1)
    return -1;

  m_hAccelTable = LoadAccelerators(theApp.m_hInstance, MAKEINTRESOURCE(IDR_MAINFRAME));

  // Initialize a context for the view. CDialog1 is my view and
  // is defined as :  class CDIalog1 : public CTreeView.
  CCreateContext context;
  context.m_pNewViewClass = RUNTIME_CLASS(CWnd);
  context.m_pCurrentDoc = NULL;
  context.m_pNewDocTemplate = NULL;
  context.m_pLastView = NULL;
  context.m_pCurrentFrame = NULL;

  // Because the CFRameWnd needs a window class, we will create
  // a new one. I just copied the sample from MSDN Help.
  // When using it in your project, you may keep CS_VREDRAW and
  // CS_HREDRAW and then throw the other three parameters.
  CString strMyClass = AfxRegisterWndClass(CS_VREDRAW |
    CS_HREDRAW,
    ::LoadCursor(NULL, IDC_ARROW),
    (HBRUSH) ::GetStockObject(WHITE_BRUSH),
    ::LoadIcon(NULL, IDI_APPLICATION));

  // Create the frame window with "this" as the parent
  m_pFrameWnd = new CNullFrame;
  m_pFrameWnd->Create(strMyClass, _T(""), WS_CHILD,
    CRect(0, 0, 1, 1), this);
  m_pFrameWnd->ShowWindow(SW_SHOW);


  if (!m_wndSplitterLR.CreateStatic(m_pFrameWnd, 1, 2))
  {
    TRACE0("Failed to CreateStaticSplitter\n");
    return -1;
  }

  if (!m_wndSplitterLR.CreateView(0, 1, RUNTIME_CLASS(CContentsView), CSize(0, 0), &context))
  {
    TRACE0("Failed to create first pane\n");
    return -1;
  }

  m_wndSplitterLR.SetColumnInfo(0, 200, 0);

  if (!m_wndSplitterUD.CreateStatic(&m_wndSplitterLR, 2, 1, WS_CHILD | WS_VISIBLE, m_wndSplitterLR.IdFromRowCol(0, 0)))
  {
    TRACE0("Failed to CreateStaticSplitter\n");
    return -1;
  }
  if (!m_wndSplitterUD.CreateView(0, 0, RUNTIME_CLASS(CFolderView), CSize(0, 200), &context))
  {
    TRACE0("Failed to create second pane\n");
    return -1;
  }
  if (!m_wndSplitterUD.CreateView(1, 0, RUNTIME_CLASS(CDetailView), CSize(0, 0), &context))
  {
    TRACE0("Failed to create third pane\n");
    return -1;
  }

  CWnd * pWnd;

  pWnd = m_wndSplitterUD.GetPane(0, 0);
  if (!pWnd->IsKindOf(RUNTIME_CLASS(CFolderView)))
    return -1;
  m_pFolderView = dynamic_cast<CFolderView *>(pWnd);

  pWnd = m_wndSplitterUD.GetPane(1, 0);
  if (!pWnd->IsKindOf(RUNTIME_CLASS(CDetailView)))
    return -1;
  m_pDetailView = dynamic_cast<CDetailView *>(pWnd);

  pWnd = m_wndSplitterLR.GetPane(0, 1);
  if (!pWnd->IsKindOf(RUNTIME_CLASS(CContentsView)))
    return -1;
  m_pContentsView = dynamic_cast<CContentsView *>(pWnd);
  m_pContentsView->m_pDetailView = m_pDetailView;

  if (!m_wndStatusBar.Create(this) ||
    !m_wndStatusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(UINT)))
  {
    TRACE0("Failed to create status bar\n");
    return -1;
  }
  RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, IDS_STATUS_PANE2);
  m_wndStatusBar.GetStatusBarCtrl().SetBkColor(RGB(255, 0, 0));
  m_wndStatusBar.GetStatusBarCtrl().SetParts(4, indicator_sizes);
  for (unsigned i = 0; i < sizeof(indicators) / sizeof(UINT); i++) {
    m_wndStatusBar.SetPaneInfo(i, indicators[i], SBPS_STRETCH, indicator_sizes[i]);
  }

  g_HWND = m_hWnd;

  return 0;
}


BOOL C7zipProFMDlg::OnInitDialog()
{
  CFileManagerDlg::OnInitDialog();

  // Add "About..." menu item to system menu.

  // IDM_ABOUTBOX must be in the system command range.
  ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
  ASSERT(IDM_ABOUTBOX < 0xF000);

  CMenu* pSysMenu = GetSystemMenu(FALSE);
  if (pSysMenu != NULL)
  {
    BOOL bNameValid;
    CString strAboutMenu;
    bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
    ASSERT(bNameValid);
    if (!strAboutMenu.IsEmpty())
    {
      pSysMenu->AppendMenu(MF_SEPARATOR);
      pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
    }
  }

  HMENU hMenu = theApp.GetContextMenuManager()->GetMenuById(IDR_MAINFRAME);
  MyChangeMenu(hMenu, 0, 0);

  // Set the icon for this dialog.  The framework does this automatically
  //  when the application's main window is not a dialog
  SetIcon(m_hIcon, TRUE);			// Set big icon
  SetIcon(m_hIcon, FALSE);		// Set small icon

  // TODO: Add extra initialization here

  for (unsigned int i = 0; i < nButtons; i++) {
    pButtons[i]->SetPNGBitmaps(nButtonIDs[i * 3 + 2], 0, nButtonIDs[i * 3 + 1]);
    pButtons[i]->DrawBorder(FALSE);
    pButtons[i]->DrawTransparent();
    pButtons[i]->SetAlign(CButtonST::ST_ALIGN_VERT);
    pButtons[i]->SetColor(CButtonST::BTNST_COLOR_FG_IN, RGB(255, 255, 255));
    pButtons[i]->SetColor(CButtonST::BTNST_COLOR_FG_OUT, RGB(255, 255, 255));
    pButtons[i]->SetColor(CButtonST::BTNST_COLOR_FG_FOCUS, RGB(255, 255, 255));
    if (nButtonIDs[i * 3] > 0) {
      UString title = LangString(nButtonIDs[i * 3]);
      if (!title.IsEmpty())
        pButtons[i]->SetWindowTextW(title);
    }
  }

  ArrangeControls();

  PrepareBackground();

  CMFCToolBar::AddToolBarForImageCollection(IDR_MAINFRAME);

  m_pFolderView->SelectFolder(m_strFolderPrefix);

  // 	LPCTSTR mainPath = m_strFolderPrefix;
  // 	LPCTSTR arcFormat = NULL;
  // 	bool archiveIsOpened;
  // 	bool encrypted;
  // 	if (FAILED(m_pContentsView->Initialize(mainPath, arcFormat, archiveIsOpened, encrypted)))
  // 		return FALSE;

  m_cmbPath.ModifyStyle(0, CBS_AUTOHSCROLL);
  m_cmbPath.SendMessage(CBEM_SETUNICODEFORMAT, TRUE, 0);
  m_cmbPath.SetExtendedStyle(CBES_EX_PATHWORDBREAKPROC, CBES_EX_PATHWORDBREAKPROC);

  HIMAGELIST hImg = GetSysImageList(true);
  CImageList *pImgList = CImageList::FromHandlePermanent(hImg);
  if (pImgList == NULL) {
    CImageList img;
    img.Attach(hImg);
    m_cmbPath.SetImageList(&img);
    img.Detach();
  }
  else
    m_cmbPath.SetImageList(pImgList);

  m_pContentsView->SetListViewStyle(m_nContentsViewStyle);

  UpdateFolder();

  return TRUE;  // return TRUE  unless you set the focus to a control
}


void C7zipProFMDlg::LoadState()
{
  CFileManagerDlg::LoadState();

  CRect rtClient;
  GetClientRect(&rtClient);
  CRect rtFolder(0, 0, rtClient.Width() / 3, rtClient.Height() * 3 / 4);
  {
    CSettingsStoreSP regSP;
    CSettingsStore& reg = regSP.Create(FALSE, TRUE);
    if (reg.Open(theApp.GetRegSectionPath(RP_WINDOW_INFO))) {
      reg.Read(RK_FOLDER_SIZE, rtFolder);
    }
  }
  {
    CSettingsStoreSP regSP;
    CSettingsStore& reg = regSP.Create(FALSE, TRUE);
    if (reg.Open(theApp.GetRegSectionPath(RP_FILE_MANAGER))) {
      reg.Read(RK_FOLDER_PREFIX, m_strFolderPrefix);
      reg.Read(RK_CONTENTS_VIEW_STYLE, m_nContentsViewStyle);
      INT nMaxHistory = 20;
      reg.Read(RK_MAX_HISTORY, nMaxHistory);
      m_nMaxHistory = (INT)nMaxHistory;
    }
  }

  m_wndSplitterLR.SetColumnInfo(0, rtFolder.Width(), 0);
  m_wndSplitterLR.SetColumnInfo(1, rtClient.Width() - rtFolder.Width(), 0);
  m_wndSplitterLR.RecalcLayout();
  m_wndSplitterUD.SetRowInfo(0, rtFolder.Height(), 0);
  m_wndSplitterUD.SetRowInfo(1, rtClient.Height() - rtFolder.Height(), 0);
  m_wndSplitterUD.RecalcLayout();
}


void C7zipProFMDlg::SaveState()
{
  CFileManagerDlg::SaveState();

  {
    CSettingsStoreSP regSP;
    CSettingsStore& reg = regSP.Create(FALSE, FALSE);
    if (reg.CreateKey(theApp.GetRegSectionPath(RP_WINDOW_INFO))) {
      CRect rt;
      m_wndSplitterUD.GetPane(0, 0)->GetWindowRect(&rt);
      reg.Write(RK_FOLDER_SIZE, rt);
    }
  }
  {
    CSettingsStoreSP regSP;
    CSettingsStore& reg = regSP.Create(FALSE, FALSE);
    if (reg.CreateKey(theApp.GetRegSectionPath(RP_FILE_MANAGER))) {
      reg.Write(RK_FOLDER_PREFIX, m_strFolderPrefix);
      reg.Write(RK_CONTENTS_VIEW_STYLE, m_nContentsViewStyle);
      INT nMaxHistory = (INT)m_nMaxHistory;
      reg.Write(RK_MAX_HISTORY, nMaxHistory);
    }
  }
}


CPoint C7zipProFMDlg::GetLogoPos()
{
  BITMAP bmTop, bmLogo;
  m_bmpBGTop.GetBitmap(&bmTop);
  m_bmpLogo.GetBitmap(&bmLogo);

  int top_margin = 0;
  int right_margin = max(bmTop.bmWidth * 2, 20);
  if (::IsWindow(m_btnClose) && !m_bMaximized) {
    CRect rtItem;
    m_btnClose.GetWindowRect(&rtItem);
    ScreenToClient(&rtItem);
    top_margin = rtItem.bottom;
  }

  CRect rt;
  GetClientRect(&rt);
  CPoint point(rt.right - right_margin - bmLogo.bmWidth,
    top_margin + (rt.top + bmTop.bmHeight - top_margin - bmLogo.bmHeight) / 2);
  return point;
}


void C7zipProFMDlg::OnEnterSizeMove()
{
  m_pContentsView->m_bResizing = TRUE;

  CFileManagerDlg::OnEnterSizeMove();
}


void C7zipProFMDlg::OnExitSizeMove()
{
  m_pContentsView->m_bResizing = FALSE;
  // 	m_pContentsView->Invalidate();

  CFileManagerDlg::OnExitSizeMove();
}


void C7zipProFMDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
  if ((nID & 0xFFF0) == IDM_ABOUTBOX)
  {
    CAboutDlg dlgAbout;
    dlgAbout.DoModal();
  }
  else
  {
    CFileManagerDlg::OnSysCommand(nID, lParam);
  }
}


void C7zipProFMDlg::SetDCClipArea(CDC *pDC)
{
  // 	CRect rt;
  // 	GetClientRect(&rt);

  // 	pDC->IntersectClipRect(&rt);
  for (unsigned int i = 0; i < nButtons; i++) {
    CRect rtButton;
    if (!pButtons[i]->IsWindowVisible())
      continue;
    pButtons[i]->GetWindowRect(rtButton);
    ScreenToClient(rtButton);
    pDC->ExcludeClipRect(&rtButton);
  }
  CWnd *pExtra[] = { m_pFrameWnd, &m_cmbPath, &m_wndStatusBar };
  for (unsigned int i = 0; i < sizeof(pExtra) / sizeof(pExtra[0]); i++) {
    CRect rtContents;
    if (!pExtra[i]->IsWindowVisible())
      continue;
    pExtra[i]->GetWindowRect(rtContents);
    ScreenToClient(rtContents);
    pDC->ExcludeClipRect(&rtContents);
  }
}


void C7zipProFMDlg::GetSysMenuRect(LPRECT lpRect)
{
  GetDlgItem(IDC_PIC_SYSMENU)->GetWindowRect(lpRect);
}


void C7zipProFMDlg::GetButtons(CButtonST **&pButtons, UINT &nButtons)
{
  pButtons = this->pButtons;
  nButtons = this->nButtons;
}


void C7zipProFMDlg::GetSystemButtons(CButton **&pButtons, UINT &nButtons)
{
  static CButton *pSysButtons[] = {
    &m_btnMenu,
    //&m_btnSkin,
    &m_btnMinimize,
    &m_btnMaximize,
    &m_btnClose,
  };
  pButtons = pSysButtons;
  nButtons = sizeof(pSysButtons) / sizeof(pSysButtons[0]);
}


BOOL C7zipProFMDlg::PreTranslateMessage(MSG* pMsg)
{
  if (pMsg->message == WM_KEYDOWN) {
    if (pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE)
      return FALSE;
    CWnd *pWndFocus = CWnd::GetFocus();
    if (pWndFocus->GetParent() == m_pContentsView)
      return FALSE;
  }
  return CFileManagerDlg::PreTranslateMessage(pMsg);
}


void C7zipProFMDlg::GetToolButtons(CButton **&pButtons, UINT &nButtons)
{
  static CButton * pToolButtons[] = {
    &m_btnAddToArchive,
    &m_btnExtractFiles,
    &m_btnTestArchive,
    &m_btnCopyTo,
    &m_btnMoveTo,
    &m_btnDelete,
    &m_btnProperties,
    &m_btnBenchmark,
  };
  pButtons = pToolButtons;
  nButtons = sizeof(pToolButtons) / sizeof(pToolButtons[0]);
}


void C7zipProFMDlg::GetNavControls(CWnd **&pControls, UINT &nControls)
{
  static CWnd * pNavControls[] = {
    &m_btnNavBackward,
    &m_btnNavForward,
    &m_btnNavBackTo,
    &m_btnNavUp,
    &m_btnSwitchViewStyle,
    &m_btnViewStyle,
    &m_cmbPath,
  };
  pControls = pNavControls;
  nControls = sizeof(pNavControls) / sizeof(pNavControls[0]);
}


CFrameWnd * C7zipProFMDlg::GetCenterFrame()
{
  return m_pFrameWnd;
}


CStatusBar *C7zipProFMDlg::GetStatusBar()
{
  return &m_wndStatusBar;
}


void C7zipProFMDlg::ArrangeControls()
{
  if (!::IsWindow(m_btnMinimize))
    return;

  CRect rtClient;
  GetClientRect(&rtClient);

  CRect rt;
  m_btnMenu.GetWindowRect(&rt);
  ScreenToClient(&rt);
  int xMargin = 5;
  int yMargin = rt.top - rtClient.top;
  m_btnMenu.SetWindowPos(NULL,
    rtClient.Width() - rt.Width() * 4 - xMargin * 5,
    yMargin, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
  //   m_btnSkin.SetWindowPos(NULL,
  //     rtClient.Width() - rt.Width() * 4 - xMargin * 5,
  //     yMargin, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
  m_btnMinimize.SetWindowPos(NULL,
    rtClient.Width() - rt.Width() * 3 - xMargin * 3,
    yMargin, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
  m_btnMaximize.SetWindowPos(NULL,
    rtClient.Width() - rt.Width() * 2 - xMargin * 2,
    yMargin, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
  m_btnClose.SetWindowPos(NULL,
    rtClient.Width() - rt.Width() * 1 - xMargin * 1,
    yMargin, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

  int sum = 0;
  for (unsigned i = 0; i < sizeof(indicator_sizes) / sizeof(int); i++)
    sum += indicator_sizes[i];
  for (unsigned i = 0; i < sizeof(indicator_sizes) / sizeof(int); i++) {
    m_wndStatusBar.SetPaneInfo(i, indicators[i], SBPS_STRETCH,
      rtClient.Width() * indicator_sizes[i] / sum);
  }

  CFileManagerDlg::ArrangeControls();
}


void C7zipProFMDlg::OnOK()
{

}


void C7zipProFMDlg::OnCancel()
{
  CFileManagerDlg::OnCancel();
}


void C7zipProFMDlg::OnBnClickedBtnMenu()
{
#if _DEBUG
  //   CProgressDialog dlg;
  //   dlg.DoModal();
#endif

  CRect rt;
  m_btnMenu.GetWindowRect(&rt);
  HMENU hMenu = theApp.GetContextMenuManager()->GetMenuById(IDR_MAINFRAME);
  MyChangeMenu(hMenu, 0, 0);
  theApp.GetContextMenuManager()->ShowPopupMenu(hMenu, rt.left, rt.bottom, this, TRUE);
}


void C7zipProFMDlg::OnBnClickedBtnSkin()
{
  CProgressDialog dlg;
  dlg.DoModal();
}


void C7zipProFMDlg::OnBnClickedBtnMinimize()
{
  CPoint point;
  GetCursorPos(&point);
  SendMessage(WM_SYSCOMMAND, SC_MINIMIZE, MAKELPARAM(point.x, point.y));
}


void C7zipProFMDlg::OnBnClickedBtnMaximize()
{
  CPoint point;
  GetCursorPos(&point);
  SendMessage(WM_SYSCOMMAND,
    m_bMaximized ? SC_RESTORE : SC_MAXIMIZE,
    MAKELPARAM(point.x, point.y));
}


void C7zipProFMDlg::OnBnClickedBtnClose()
{
  CPoint point;
  GetCursorPos(&point);
  SendMessage(WM_SYSCOMMAND, SC_CLOSE, MAKELPARAM(point.x, point.y));
}


void C7zipProFMDlg::OnUpdateItemSelected(CCmdUI *pCmdUI)
{
  CWnd *pWndFocused = CWnd::GetFocus();
  if (pWndFocused == m_pContentsView) {
    CRecordVector<UInt32> indices;
    m_pContentsView->GetOperatedItemIndices(indices);
    pCmdUI->Enable(!indices.IsEmpty());
  }
  else
    pCmdUI->Enable(TRUE);
}


void C7zipProFMDlg::OnUpdateRange(CCmdUI *pCmdUI)
{
  switch (pCmdUI->m_nID)
  {
  case IDC_BTN_NAV_BACKWARD:
    pCmdUI->Enable(m_nHistoryPos > 1);
    break;
  case IDC_BTN_NAV_FORWARD:
    pCmdUI->Enable(m_nHistoryPos < m_vecHistories.Size());
    break;
  case IDC_BTN_NAV_BACK_TO:
    pCmdUI->Enable(!m_vecHistories.IsEmpty());
    break;
  case IDC_BTN_NAV_UP:
    pCmdUI->Enable(!m_pContentsView->IsRootFolder());
    break;
  case ID_VIEW_AUTO_REFRESH:
    pCmdUI->SetCheck(BoolToBOOL(m_pContentsView->AutoRefresh_Mode));
    break;
  default:
    break;
  }
}


void C7zipProFMDlg::OnCommandRange(UINT nID)
{
  CWnd *pWndFocused = CWnd::GetFocus();
  switch (nID)
  {
    // File
  case ID_FILE_OPEN:                m_pContentsView->OpenSelectedItems(true); break;
  case ID_FILE_OPEN_INSIDE:         m_pContentsView->OpenFocusedItemAsInternal(NULL); break;
  //case ID_FILE_OPEN_INSIDE_ONE:     m_pContentsView->OpenFocusedItemAsInternal(L"*"); break;
  //case ID_FILE_OPEN_INSIDE_PARSER:  m_pContentsView->OpenFocusedItemAsInternal(L"#"); break;
  case ID_FILE_OPEN_OUTSIDE:        m_pContentsView->OpenSelectedItems(false); break;
  case ID_FILE_VIEW:                m_pContentsView->EditItem(false); break;
  case ID_FILE_EDIT:                m_pContentsView->EditItem(true); break;
  case ID_FILE_RENAME:
    if (pWndFocused == m_pFolderView)
      m_pFolderView->RenameFolder();
    else
      m_pContentsView->RenameFile();
    break;
  case IDC_BTN_COPY_TO:
  case ID_FILE_COPY_TO:
    if (pWndFocused == m_pFolderView)
      m_pFolderView->CopyTo();
    else
      m_pContentsView->CopyTo();
    break;
  case IDC_BTN_MOVE_TO:
  case ID_FILE_MOVE_TO:
    if (pWndFocused == m_pFolderView)
      m_pFolderView->MoveTo();
    else
      m_pContentsView->MoveTo();
    break;
  case IDC_BTN_DELETE:
  case ID_FILE_DELETE:
    if (pWndFocused == m_pFolderView)
      m_pFolderView->DeleteFolder(!IsKeyDown(VK_SHIFT));
    else
      m_pContentsView->DeleteItems(!IsKeyDown(VK_SHIFT));
    break;

  case ID_CRC_HASH_ALL:        m_pContentsView->CalculateCrc(L"*"); break;
  case ID_CRC_CRC32:           m_pContentsView->CalculateCrc(L"CRC32"); break;
  case ID_CRC_CRC64:           m_pContentsView->CalculateCrc(L"CRC64"); break;
  case ID_CRC_SHA1:            m_pContentsView->CalculateCrc(L"SHA1"); break;
  case ID_CRC_SHA256:          m_pContentsView->CalculateCrc(L"SHA256"); break;

    // 	case ID_FILE_DIFF:           m_pContentsView->DiffFiles(); break;
  case ID_FILE_SPLIT_FILE:     m_pContentsView->Split(); break;
  case ID_FILE_COMBINE_FILES:  m_pContentsView->Combine(); break;
  case IDC_BTN_PROPERTIES:
  case ID_FILE_PROPERTIES:
    if (pWndFocused == m_pFolderView)
      m_pFolderView->Properties();
    else
      m_pContentsView->Properties();
    break;
  case ID_FILE_COMMENT:        m_pContentsView->ChangeComment(); break;
  case ID_FILE_CREATE_FOLDER:
    if (pWndFocused == m_pFolderView)
      m_pFolderView->CreateFolder();
    else
      m_pContentsView->CreateFolder();
    break;
  case ID_FILE_CREATE_FILE:
    if (pWndFocused == m_pFolderView)
      m_pFolderView->CreateFile();
    else
      m_pContentsView->CreateFile();
    break;
#ifndef UNDER_CE
    // 	case ID_FILE_LINK:          m_pContentsView->Link(); break;
    //  case IDM_ALT_STREAMS:       m_pContentsView->OpenAltStreams(); break;
#endif
  case IDC_BTN_ADD_TO_ARCHIVE:
  case ID_7ZE_ADD_TO_ARCHIVE:  m_pContentsView->AddToArchive(); break;
  case IDC_BTN_EXTRACT_FILES:
  case ID_7ZE_EXTRACT_FILES:   m_pContentsView->ExtractArchives(); break;
  case IDC_BTN_TEST_ARCHIVE:   m_pContentsView->TestArchives(); break;

  case ID_EDIT_COPY: m_pContentsView->EditCopy(); break;
  case ID_EDIT_CUT: m_pContentsView->EditCut(); break;
  case ID_EDIT_PASTE: m_pContentsView->EditPaste(); break;

  case ID_VIEW_OPEN_DRIVES_FOLDER: m_pContentsView->OpenDrivesFolder(); break;
  case ID_EDIT_SELECT_ALL:
    m_pContentsView->SelectAll(true);
    Refresh_StatusBar();
    break;
  case ID_EDIT_DESELECT_ALL:
    m_pContentsView->SelectAll(false);
    Refresh_StatusBar();
    break;
  case ID_EDIT_INVERT_SELECTION:
    m_pContentsView->InvertSelection();
    break;
  case ID_EDIT_SELECT_WITH_MASK:
    m_pContentsView->SelectSpec(true);
    Refresh_StatusBar();
    break;
  case ID_EDIT_DESELECT_WITH_MASK:
    m_pContentsView->SelectSpec(false);
    Refresh_StatusBar();
    break;
  case ID_EDIT_SELECT_BY_TYPE:
    m_pContentsView->SelectByType(true);
    Refresh_StatusBar();
    break;
  case ID_EDIT_DESELECT_BY_TYPE:
    m_pContentsView->SelectByType(false);
    Refresh_StatusBar();
    break;

  case ID_VIEW_ARRANGE_BY_NAME:     m_pContentsView->SortItemsWithPropID(kpidName); break;
  case ID_VIEW_ARRANGE_BY_TYPE:     m_pContentsView->SortItemsWithPropID(kpidExtension); break;
  case ID_VIEW_ARRANGE_BY_DATE:     m_pContentsView->SortItemsWithPropID(kpidMTime); break;
  case ID_VIEW_ARRANGE_BY_SIZE:     m_pContentsView->SortItemsWithPropID(kpidSize); break;
  case ID_VIEW_ARRANGE_NO_SORT:     m_pContentsView->SortItemsWithPropID(kpidNoProperty); break;

  case ID_VIEW_OPEN_ROOT_FOLDER:    m_pContentsView->OpenDrivesFolder(); break;
  case IDC_BTN_NAV_UP:
  case ID_VIEW_OPEN_PARENT_FOLDER:  m_pContentsView->OpenParentFolder(); break;
  case IDC_BTN_NAV_BACKWARD:
    m_nHistoryPos--;
    UpdateFolder(m_vecHistories[m_nHistoryPos - 1], TRUE, FALSE);
    break;
  case IDC_BTN_NAV_FORWARD:
    m_nHistoryPos++;
    UpdateFolder(m_vecHistories[m_nHistoryPos - 1], TRUE, FALSE);
    break;
  case ID_VIEW_FOLDERS_HISTORY:     m_pContentsView->FoldersHistory(); break;
    // 	case ID_VIEW_FLAT_VIEW:           m_pContentsView->ChangeFlatMode(); break;
  case ID_VIEW_REFRESH:
    m_pFolderView->Refresh();
    m_pContentsView->OnReload();
    break;
  case ID_VIEW_AUTO_REFRESH:
    m_pContentsView->AutoRefresh_Mode = !m_pContentsView->AutoRefresh_Mode; break;
  case IDC_BTN_BENCHMARK:
  case ID_TOOLS_BENCHMARK:          Benchmark(true); break;
  case ID_HELP:
    ShowHelpWindow(NULL, kFMHelpTopic);
    break;
  case ID_APP_ABOUT:
  {
    CAboutDlg dlgAbout;
    dlgAbout.DoModal();
    break;
  }
  default:
    break;
  }
}


void C7zipProFMDlg::OnContextMenu(CWnd* pWnd, CPoint point)
{
  // TODO: Add your message handler code here
}


void C7zipProFMDlg::OnUpdateHistory(CCmdUI *pCmdUI)
{
  pCmdUI->Enable(pCmdUI->m_nID - HISTORY_CMD_FIRST <= (int)m_vecHistories.Size());
}


void C7zipProFMDlg::OnBnClickedBtnNavBackTo()
{
  CRect rt;
  m_btnNavBackTo.GetWindowRect(&rt);
  CMenu menu;
  menu.CreatePopupMenu();
  FOR_VECTOR(i, m_vecHistories)
  {
    menu.AppendMenu(
      MF_BYPOSITION | MF_STRING | MF_ENABLED |
      ((i == m_nHistoryPos - 1) ? MF_CHECKED : 0),
      i + HISTORY_CMD_FIRST, m_vecHistories[i]);
  }
  int cmd = menu.TrackPopupMenu(TPM_RETURNCMD, rt.left, rt.bottom, this);
  if (cmd == 0 || cmd - HISTORY_CMD_FIRST + 1 == m_nHistoryPos)
    return;
  m_nHistoryPos = cmd - HISTORY_CMD_FIRST + 1;
  UpdateFolder(m_vecHistories[m_nHistoryPos - 1], TRUE, FALSE);
}


void C7zipProFMDlg::OnBnClickedBtnSwitchViewStyle()
{
  m_nContentsViewStyle = (m_nContentsViewStyle + 1) % 4;
  m_pContentsView->SetListViewStyle(m_nContentsViewStyle);
}


void C7zipProFMDlg::OnBnClickedBtnViewStyle()
{
  CRect rt;
  m_btnViewStyle.GetWindowRect(&rt);
  HMENU hMenu = theApp.GetContextMenuManager()->GetMenuById(IDR_MENU_POPUP);
  hMenu = ::GetSubMenu(hMenu, 0);
  theApp.GetContextMenuManager()->ShowPopupMenu(hMenu, rt.left, rt.bottom, this, TRUE);
}


void C7zipProFMDlg::OnUpdateViewStyle(CCmdUI *pCmdUI)
{
  pCmdUI->SetCheck(pCmdUI->m_nID - ID_VIEW_SMALLICON == m_nContentsViewStyle);
}


void C7zipProFMDlg::OnViewStyle(UINT nID)
{
  m_nContentsViewStyle = nID - ID_VIEW_SMALLICON;
  m_pContentsView->SetListViewStyle(m_nContentsViewStyle);
}


void C7zipProFMDlg::OnUpdateViewArrange(CCmdUI *pCmdUI)
{
  pCmdUI->SetRadio(
    pCmdUI->m_nID == GetSortControlID(m_pContentsView->GetSortID()));
}


void C7zipProFMDlg::OnCmbDropdown()
{
  m_bTracking = TRUE;
  m_cmbPath.SetFocus();
  m_cmbPath.ShowDropDown();
  m_bTracking = FALSE;
}


void C7zipProFMDlg::OnCbenEndeditCmbPath(NMHDR *pNMHDR, LRESULT *pResult)
{
  if (m_bTracking)
    return;
  PNMCBEENDEDIT info = (PNMCBEENDEDIT)pNMHDR;
  if (info->iWhy == CBENF_ESCAPE)
  {
    UpdateData(FALSE);
  }

  /*
  else if (info->iWhy == CBENF_DROPDOWN)
  {
  result = FALSE;
  return true;
  }
  */

  else if (info->iWhy == CBENF_RETURN /*|| info->iWhy == CBENF_KILLFOCUS*/)
  {
    // When we use Edit control and press Enter.
    UpdateData();
    UpdateFolder();
  }
  *pResult = 0;
}


void C7zipProFMDlg::AddComboBoxItem(
  LPCTSTR name, int iconIndex, int indent, bool addToList)
{
#ifdef UNDER_CE

  FString s;
  iconIndex = iconIndex;
  for (int i = 0; i < indent; i++)
    s += _T("  ");
  m_cmbPath.AddString(s + name);

#else

  COMBOBOXEXITEM item;
  item.mask = CBEIF_TEXT | CBEIF_INDENT;
  item.iSelectedImage = item.iImage = iconIndex;
  if (iconIndex >= 0)
    item.mask |= (CBEIF_IMAGE | CBEIF_SELECTEDIMAGE);
  item.iItem = -1;
  item.iIndent = indent;
  item.pszText = (LPTSTR)name;
  m_cmbPath.InsertItem(&item);

#endif

  if (addToList)
    m_vecPaths.Add(name);
}


void C7zipProFMDlg::OnCbnDropdownCmbPath()
{
  m_vecPaths.Clear();
  m_cmbPath.ResetContent();

  UString fullPath = GetUnicodeString(m_pFolderView->GetFullPath());
  UStringVector pathParts;

  SplitPathToParts(fullPath, pathParts);
  FString sumPass;
  if (!pathParts.IsEmpty())
    pathParts.DeleteBack();
  FOR_VECTOR(i, pathParts)
  {
    FString name = GetSystemString(pathParts[i]);
    sumPass += name;
    sumPass.Add_PathSepar();
    NWindows::NFile::NFind::CFileInfo info;
    DWORD attrib = FILE_ATTRIBUTE_DIRECTORY;
    if (info.Find(sumPass))
      attrib = info.Attrib;
    AddComboBoxItem(name.IsEmpty() ? _T("\\") : name, GetRealIconIndex(sumPass, attrib), i, false);
    m_vecPaths.Add(sumPass);
  }

#ifndef UNDER_CE

  int iconIndex;
  FString name;
  name = RootFolder_GetName_Desktop(iconIndex);
  AddComboBoxItem(name, iconIndex, 0, true);

  name = RootFolder_GetName_Documents(iconIndex);
  AddComboBoxItem(name, iconIndex, 0, true);

  name = RootFolder_GetName_Computer(iconIndex);
  AddComboBoxItem(name, iconIndex, 0, true);

  FStringVector driveStrings;
  NWindows::NFile::NFind::MyGetLogicalDriveStrings(driveStrings);
  FOR_VECTOR(i, driveStrings)
  {
    FString s = driveStrings[i];
    m_vecPaths.Add(s);
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
      default:
        break;
      }
    }
    if (!volName.IsEmpty())
      s = volName + _T(" (") + s + _T(")");

    AddComboBoxItem(s, iconIndex, 1, false);
  }

  name = RootFolder_GetName_Network(iconIndex);
  AddComboBoxItem(name, iconIndex, 0, true);

#endif
}


void C7zipProFMDlg::OnCbnSelendokCmbPath()
{
  int index = m_cmbPath.GetCurSel();
  if (index >= 0) {
    m_strFolderPrefix = m_vecPaths[index];
    m_cmbPath.SetCurSel(-1);
    UpdateFolder();
  }
}


LRESULT C7zipProFMDlg::OnFolderChanged(WPARAM wParam, LPARAM lParam)
{
  if (m_bTracking)
    return 0;
  m_bTracking = TRUE;
  CWnd *pWnd = (CWnd *)wParam;
  if (pWnd == m_pFolderView) {
    UString fullPath = m_pFolderView->GetFullPath();
    m_pContentsView->BindToPathAndRefresh(fullPath);
    UpdatePath(GetSystemString(fullPath));
  }
  else if (pWnd == m_pContentsView) {
    FString fullPath = GetSystemString(m_pContentsView->_currentFolderPrefix);
    m_pFolderView->SelectFolder(fullPath);
    m_pFolderView->SendMessage(kFolderChanged);
    UpdatePath(fullPath);
  }
  else if (pWnd == &m_cmbPath) {
    m_pFolderView->SelectFolder(m_strFolderPrefix);
    m_pContentsView->BindToPathAndRefresh(GetUnicodeString(m_strFolderPrefix));
    UpdatePath(NULL);
  }
  m_bTracking = FALSE;
  return 0;
}


void C7zipProFMDlg::UpdateFolder(
  LPCTSTR lpszNewPath /* = NULL */,
  BOOL updateChildren /* = TRUE */,
  BOOL updateHistory /* = TRUE */)
{
  if (lpszNewPath != NULL)
    m_strFolderPrefix = lpszNewPath;
  if (updateChildren) {
    m_bTracking = TRUE;
    m_pFolderView->SelectFolder(m_strFolderPrefix);
    m_pContentsView->BindToPathAndRefresh(GetUnicodeString(m_strFolderPrefix));
    m_bTracking = FALSE;
  }
  UpdatePath(NULL, updateHistory);
}


void C7zipProFMDlg::UpdatePath(
  LPCTSTR lpszNewPath /* = NULL */,
  BOOL updateHistory /* = TRUE */)
{
  if (lpszNewPath != NULL)
    m_strFolderPrefix = lpszNewPath;

#ifndef UNDER_CE

  COMBOBOXEXITEM item;
  item.mask = 0;

  FString path = m_strFolderPrefix;
  if (path.Len() >
#ifdef _WIN32
    3
#else
    1
#endif
    && path.Back() == FCHAR_PATH_SEPARATOR)
    path.DeleteBack();

  DWORD attrib = FILE_ATTRIBUTE_DIRECTORY;

  // GetRealIconIndex is slow for direct DVD/UDF path. So we use dummy path
  if (path.IsPrefixedBy(L"\\\\.\\"))
    path = L"_TestFolder_";
  else
  {
    NWindows::NFile::NFind::CFileInfo fi;
    if (fi.Find(path))
      attrib = fi.Attrib;
  }
  item.iImage = GetRealIconIndex(path, attrib);
  if (item.iImage >= 0)
  {
    item.iSelectedImage = item.iImage;
    item.mask |= (CBEIF_IMAGE | CBEIF_SELECTEDIMAGE);
  }
  item.iItem = -1;
  m_cmbPath.SetItem(&item);

#endif

  if (updateHistory) {
    if (m_nHistoryPos < m_vecHistories.Size())
      m_vecHistories.DeleteFrom(m_nHistoryPos);
    FString newPath = m_strFolderPrefix;
    FOR_VECTOR(i, m_vecHistories)
    {
      if (m_vecHistories[i].IsEqualTo_NoCase(newPath))
        m_vecHistories.Delete(i--);
    }
    m_vecHistories.Add(newPath);
    if (m_vecHistories.Size() > m_nMaxHistory)
      m_vecHistories.DeleteFrontal(1);
    m_nHistoryPos = m_vecHistories.Size();
  }

  UpdateData(FALSE);

  CString title = m_strFolderPrefix;
  if (title.IsEmpty())
    title = LangString(IDR_MAINFRAME);
  SetWindowText(title);
}


void C7zipProFMDlg::OnNextPane()
{
  CWnd *pWndFocus = CWnd::GetFocus();
  while (pWndFocus != NULL) {
    CWnd *pNewFocus = NULL;
    if (pWndFocus->m_hWnd == m_cmbPath.m_hWnd)
      pNewFocus = m_pFolderView;
    else if (pWndFocus->m_hWnd == m_pFolderView->m_hWnd)
      pNewFocus = m_pContentsView;
    if (pNewFocus != NULL) {
      pNewFocus->SetFocus();
      return;
    }
    pWndFocus = pWndFocus->GetParent();
  }
  m_cmbPath.SetFocus();
}


LRESULT C7zipProFMDlg::OnRefresh_StatusBar(WPARAM wParam, LPARAM lParam)
{
  if (m_pContentsView->_processStatusBar)
    Refresh_StatusBar();
  return 0;
}


void C7zipProFMDlg::Refresh_StatusBar()
{
  /*
  g_name_cnt++;
  char s[256];
  sprintf(s, "g_name_cnt = %8d", g_name_cnt);
  OutputDebugStringA(s);
  */
  // DWORD dw = GetTickCount();

  CRecordVector<UInt32> indices;
  m_pContentsView->GetOperatedItemIndices(indices);

  TCHAR temp[32];
  ConvertUInt32ToString(indices.Size(), temp);

  // UString s1 = MyFormatNew(g_App.LangString_N_SELECTED_ITEMS, NumberToString(indices.Size()));
  // UString s1 = MyFormatNew(IDS_N_SELECTED_ITEMS, NumberToString(indices.Size()));
  m_wndStatusBar.SetPaneText(0, GetSystemString(MyFormatNew(theApp.LangString_N_SELECTED_ITEMS, temp)));
  // _statusBar.SetText(0, MyFormatNew(IDS_N_SELECTED_ITEMS, NumberToString(indices.Size())));

  TCHAR selectSizeString[32];
  selectSizeString[0] = 0;

  if (indices.Size() > 0)
  {
    // for (int ttt = 0; ttt < 1000; ttt++) {
    UInt64 totalSize = 0;
    FOR_VECTOR(i, indices)
      totalSize += m_pContentsView->GetItemSize(indices[i]);
    ConvertIntToDecimalString(totalSize, selectSizeString);
    // }
  }
  m_wndStatusBar.SetPaneText(1, selectSizeString);

  int focusedItem = m_pContentsView->GetFocusedItem();
  TCHAR sizeString[32];
  sizeString[0] = 0;
  TCHAR dateString[32];
  dateString[0] = 0;
  if (focusedItem >= 0 && m_pContentsView->GetSelectedCount() > 0)
  {
    int realIndex = m_pContentsView->GetRealItemIndex(focusedItem);
    if (realIndex != kParentIndex)
    {
      ConvertIntToDecimalString(m_pContentsView->GetItemSize(realIndex), sizeString);
      NCOM::CPropVariant prop;
      if (m_pContentsView->_folder->GetProperty(realIndex, kpidMTime, &prop) == S_OK)
      {
        char dateString2[32];
        dateString2[0] = 0;
        ConvertPropertyToShortString(dateString2, prop, kpidMTime, false);
        for (int i = 0;; i++)
        {
          char c = dateString2[i];
          dateString[i] = c;
          if (c == 0)
            break;
        }
      }
    }
  }
  m_wndStatusBar.SetPaneText(2, sizeString);
  m_wndStatusBar.SetPaneText(3, dateString);

  // _statusBar.SetText(4, nameString);
  // _statusBar2.SetText(1, MyFormatNew(L"{0} bytes", NumberToStringW(totalSize)));
  // }
  /*
  dw = GetTickCount() - dw;
  sprintf(s, "status = %8d ms", dw);
  OutputDebugStringA(s);
  */
}


HBRUSH CAboutDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
  CFont font;
  LOGFONT lg;
  CFont *oldfont = GetFont();
  oldfont->GetLogFont(&lg);
  HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
  int id = pWnd->GetDlgCtrlID();
  switch (id) {
  case IDC_STATIC:
  case IDT_ABOUT_VERSION:
  case IDT_ABOUT_INFO:
  case IDT_ABOUT_INFO2:
    ZeroMemory(&lg, sizeof(lg));
    lg.lfWeight = 400;
    //_tcscpy(lg.lfFaceName, _T("Aldo"));
    if (id == IDT_ABOUT_VERSION)
      lg.lfHeight = 20;
    else if (id == IDT_ABOUT_INFO)
      lg.lfHeight = 18;
    else
      lg.lfHeight = 16;
    lg.lfWidth = 0;
    font.CreateFontIndirect(&lg);
    ::SelectObject(*pDC, font);
    //     pDC->SetBkMode(TRANSPARENT);
    //     return reinterpret_cast<HBRUSH>(::GetStockObject(NULL_BRUSH));
  }
  return hbr;
}
