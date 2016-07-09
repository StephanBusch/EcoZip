
// FileManagerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "EcoZipFM.h"
#include "FileManagerDlg.h"

#include <../src/mfc/afximpl.h>

#include "ImageUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CFileManagerDlg dialog

IMPLEMENT_DYNAMIC(CFileManagerDlg, CDialogEx)

CFileManagerDlg::CFileManagerDlg(UINT nIDTemplate, CWnd* pParent /*=NULL*/)
: CDialogEx(nIDTemplate, pParent), m_droptarget(this)
{
  m_hIcon = NULL;
  m_hAccelTable = NULL;
  m_bMaximized = FALSE;
}

CFileManagerDlg::~CFileManagerDlg()
{
}


void CFileManagerDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CFileManagerDlg, CDialogEx)
  ON_WM_CREATE()
  ON_WM_ERASEBKGND()
  ON_WM_PAINT()
  ON_WM_QUERYDRAGICON()
  ON_WM_GETMINMAXINFO()
  ON_WM_CTLCOLOR()
  ON_WM_NCHITTEST()
  ON_WM_NCLBUTTONDOWN()
  ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
  ON_WM_INITMENUPOPUP()
  ON_WM_SIZE()
  ON_WM_CLOSE()
  ON_WM_DROPFILES()
END_MESSAGE_MAP()


// CFileManagerDlg message handlers


int CFileManagerDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
  if (CDialogEx::OnCreate(lpCreateStruct) == -1)
    return -1;

  m_bmpBGTop.LoadBitmap(IDB_DLG_BKGND);
  CImageUtils::LoadBitmapFromPNG(IDB_BG_BOTTOM, m_bmpBGBottom);
  CImageUtils::LoadBitmapFromPNG(IDB_NAV_BAR, m_bmpNavBar);
  CImageUtils::LoadBitmapFromPNG(IDB_TITLE, m_bmpTitle);
  CImageUtils::LoadBitmapFromPNG(IDB_LOGO, m_bmpLogo);
  CImageUtils::LoadBitmapFromPNG(IDB_RB_CORNER, m_bmpRBCorner);
  CDC *pDC = GetDC();
  CImageUtils::PremultiplyBitmapAlpha(pDC->m_hDC, (HBITMAP)m_bmpTitle.m_hObject);
  CImageUtils::PremultiplyBitmapAlpha(pDC->m_hDC, (HBITMAP)m_bmpLogo.m_hObject);
  CImageUtils::PremultiplyBitmapAlpha(pDC->m_hDC, (HBITMAP)m_bmpRBCorner.m_hObject);
  ReleaseDC(pDC);

  // 	m_droptarget.Register(this);

  return 0;
}

BOOL CFileManagerDlg::OnInitDialog()
{
  CDialogEx::OnInitDialog();

  LoadState();

  return TRUE;  // return TRUE  unless you set the focus to a control
}


BOOL CFileManagerDlg::OnEraseBkgnd(CDC* pDC)
{
  return TRUE; //CDialogEx::OnEraseBkgnd(pDC);
}


// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CFileManagerDlg::OnPaint()
{
  if (IsIconic())
  {
    CPaintDC dc(this); // device context for painting

    SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

    // Center icon in client rectangle
    int cxIcon = GetSystemMetrics(SM_CXICON);
    int cyIcon = GetSystemMetrics(SM_CYICON);
    CRect rect;
    GetClientRect(&rect);
    int x = (rect.Width() - cxIcon + 1) / 2;
    int y = (rect.Height() - cyIcon + 1) / 2;

    // Draw the icon
    dc.DrawIcon(x, y, m_hIcon);
  }
  else
  {
    CRect rt;
    GetClientRect(&rt);
    CPaintDC dc(this);
    CDC dcMem;
    dcMem.CreateCompatibleDC(&dc);
    dcMem.SelectObject(&m_bmpBkgnd);

    SetDCClipArea(&dc);

    dc.BitBlt(rt.left, rt.top, rt.Width(), rt.Height(), &dcMem, 0, 0, SRCCOPY);
    //CDialogEx::OnPaint();
  }
}


CPoint CFileManagerDlg::GetLogoPos()
{
  BITMAP bmTop, bmLogo;
  m_bmpBGTop.GetBitmap(&bmTop);
  m_bmpLogo.GetBitmap(&bmLogo);

  int right_margin = max(bmTop.bmWidth * 2, 20);

  CRect rt;
  GetClientRect(&rt);
  CPoint point(rt.right - right_margin - bmLogo.bmWidth,
    (rt.top + bmTop.bmHeight - bmLogo.bmHeight) / 2);
  return point;
}

void CFileManagerDlg::DrawBackground(CDC * pDC)
{
  int xx, ww;
  int iSaved = pDC->SaveDC();

  CRect rt;
  GetClientRect(&rt);

  if (m_bmpBGTop.m_hObject == NULL)
    return;

  BITMAP bmTop, bmBottom;
  BITMAP bmNavBar, bmTitle, bmLogo, bmRBCorner;
  m_bmpBGTop.GetBitmap(&bmTop);
  m_bmpBGBottom.GetBitmap(&bmBottom);
  m_bmpNavBar.GetBitmap(&bmNavBar);
  m_bmpTitle.GetBitmap(&bmTitle);
  m_bmpLogo.GetBitmap(&bmLogo);
  m_bmpRBCorner.GetBitmap(&bmRBCorner);

  // Draw Top
  CBrush brush;
  brush.CreatePatternBrush(&m_bmpBGTop);
  CRect rtTop(rt.left, rt.top, rt.right, bmTop.bmHeight);
  pDC->FillRect(&rtTop, &brush);

  BLENDFUNCTION bf;
  memset(&bf, 0, sizeof(bf));
  bf.BlendOp = AC_SRC_OVER;
  bf.BlendFlags = 0;
  bf.SourceConstantAlpha = 255;
  bf.AlphaFormat = AC_SRC_ALPHA;

  CDC dcMem;
  dcMem.CreateCompatibleDC(pDC);

  /*if (!m_bMaximized)*/ {
    CRect rtItem;
    CWnd *pWnd = GetDlgItem(IDC_PIC_SYSMENU);
    if (pWnd != NULL) {
      pWnd->GetWindowRect(&rtItem);
      ScreenToClient(&rtItem);
      int sz = min(rtItem.Width(), rtItem.Height());
      ::DrawIconEx(*pDC, rtItem.left, rtItem.top, m_hIcon,
        sz, sz, 0, NULL, DI_NORMAL);
    }
    pWnd = GetDlgItem(IDC_PIC_TITLE);
    if (pWnd != NULL) {
      pWnd->GetWindowRect(&rtItem);
      ScreenToClient(&rtItem);
      dcMem.SelectObject(&m_bmpTitle);
      int margin = 2;
      pDC->AlphaBlend(
        rtItem.left, rtItem.top, bmTitle.bmWidth, bmTitle.bmHeight,
        &dcMem, 0, 0, bmTitle.bmWidth, bmTitle.bmHeight, bf);
    }
  }

//   CPoint pntLogo = GetLogoPos();
//   dcMem.SelectObject(&m_bmpLogo);
//   pDC->AlphaBlend(pntLogo.x, pntLogo.y, bmLogo.bmWidth, bmLogo.bmHeight,
//     &dcMem, 0, 0, bmLogo.bmWidth, bmLogo.bmHeight, bf);

  // Draw Navigation Bar
  dcMem.SelectObject(&m_bmpNavBar);
  ww = bmNavBar.bmWidth;
  for (xx = rt.left; xx < rt.right; xx += ww) {
    pDC->BitBlt(xx, rt.top + bmTop.bmHeight, ww, bmNavBar.bmHeight,
      &dcMem, 0, 0, SRCCOPY);
  }

  // Draw Bottom
  dcMem.SelectObject(&m_bmpBGBottom);
  ww = bmBottom.bmWidth;
  for (xx = rt.left; xx < rt.right; xx += ww) {
    pDC->BitBlt(xx, rt.bottom - bmBottom.bmHeight, ww, bmBottom.bmHeight,
      &dcMem, 0, 0, SRCCOPY);
  }

  // Draw Right-Bottom Corner
  dcMem.SelectObject(&m_bmpRBCorner);
  int margin = 2;
  pDC->AlphaBlend(
    rt.right - bmRBCorner.bmWidth - margin,
    rt.bottom - bmRBCorner.bmHeight - margin,
    bmRBCorner.bmWidth, bmRBCorner.bmHeight,
    &dcMem, 0, 0, bmRBCorner.bmWidth, bmRBCorner.bmHeight, bf);

  pDC->RestoreDC(iSaved);
}


void CFileManagerDlg::PrepareBackground(CDC *pDC)
{
  CDC *pCurDC = pDC;
  if (pCurDC == NULL)
    pCurDC = GetDC();

  CDC dcMem;
  dcMem.CreateCompatibleDC(pCurDC);
  if (m_bmpBkgnd.m_hObject != NULL)
    m_bmpBkgnd.DeleteObject();
  CRect rt;
  GetClientRect(&rt);
  m_bmpBkgnd.CreateCompatibleBitmap(pCurDC, rt.Width(), rt.Height());
  dcMem.SelectObject(&m_bmpBkgnd);
  DrawBackground(&dcMem);

  CButtonST **pButtons;
  UINT nButtons;
  GetButtons(pButtons, nButtons);

  for (unsigned int i = 0; i < nButtons; i++) {
    pButtons[i]->SetBk(&dcMem);
  }

  if (pDC == NULL)
    ReleaseDC(pCurDC);
}


// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CFileManagerDlg::OnQueryDragIcon()
{
  return static_cast<HCURSOR>(m_hIcon);
}


void CFileManagerDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
  // the minimum client rectangle (in that is lying the view window)
  CRect rt(0, 0, 700, 400);
  // compute the required size of the frame window rectangle
  // based on the desired client-rectangle size
  CalcWindowRect(rt);
  lpMMI->ptMinTrackSize.x = rt.Width();
  lpMMI->ptMinTrackSize.y = rt.Height();
  //CDialogEx::OnGetMinMaxInfo(lpMMI);
}


HBRUSH CFileManagerDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
  switch (pWnd->GetDlgCtrlID()) {
  case IDC_STATIC:
    pDC->SetBkMode(TRANSPARENT);
    return reinterpret_cast<HBRUSH>(::GetStockObject(NULL_BRUSH));
  }
  HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
  return hbr;
}


BOOL CFileManagerDlg::PreTranslateMessage(MSG* pMsg)
{
  if (pMsg->message == WM_KEYDOWN) {
    if (pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE)
      return FALSE;
  }
  if (m_hAccelTable) {
    if (::TranslateAccelerator(m_hWnd, m_hAccelTable, pMsg))
      return TRUE;
  }
  return CDialogEx::PreTranslateMessage(pMsg);
}


void CFileManagerDlg::GetSysMenuRect(LPRECT lpRect)
{
  lpRect->left = 7;
  lpRect->top = 8;
  lpRect->right = 23;
  lpRect->bottom = 24;
  ClientToScreen(lpRect);
}


LRESULT CFileManagerDlg::OnNcHitTest(CPoint point)
{
  LRESULT hit = CDialogEx::OnNcHitTest(point);
  if (/*!m_bMaximized &&*/ hit == HTCLIENT) {
    CRect rtSysMenu;
    GetSysMenuRect(&rtSysMenu);
    if (rtSysMenu.PtInRect(point))
      hit = HTSYSMENU;
    else {
      CWnd *pChild = GetWindow(GW_CHILD);
      while (pChild) {
        CRect rtChild;
        pChild->GetWindowRect(&rtChild);
        if (rtChild.PtInRect(point))
          return hit;
        pChild = pChild->GetWindow(GW_HWNDNEXT);
      }
      hit = HTCAPTION;
      CRect rtClient;
      GetClientRect(&rtClient);
      ClientToScreen(&rtClient);
      CRect rtWindow;
      GetWindowRect(&rtWindow);
      if (point.x + point.y >= rtClient.right + rtClient.bottom - 10)
        hit = HTBOTTOMRIGHT;
//       else if (point.x >= rtClient.right - 5)
//         hit = HTRIGHT;

    }
  }
  return hit;
}


void CFileManagerDlg::OnNcLButtonDown(UINT nHitTest, CPoint point)
{
  if (nHitTest == HTSYSMENU)
  {
    Sleep(::GetDoubleClickTime());
    MSG msg;
    if (::PeekMessage(&msg, NULL, WM_NCLBUTTONDBLCLK, WM_NCLBUTTONDBLCLK, PM_NOREMOVE)) {
      SendMessage(WM_SYSCOMMAND, SC_CLOSE, MAKELPARAM(point.x, point.y));
      return; // LEFT CONTROL TO DBL CLICK ;-)
    }
    else {
      CMenu* pMenu = GetSystemMenu(FALSE);
      pMenu->EnableMenuItem(SC_RESTORE, m_bMaximized ? MF_ENABLED : MF_DISABLED);
      pMenu->EnableMenuItem(SC_MOVE, m_bMaximized ? MF_DISABLED : MF_ENABLED);
      pMenu->EnableMenuItem(SC_SIZE, m_bMaximized ? MF_DISABLED : MF_ENABLED);
      pMenu->EnableMenuItem(SC_MAXIMIZE, m_bMaximized ? MF_DISABLED : MF_ENABLED);
      pMenu->EnableMenuItem(SC_MINIMIZE, MF_ENABLED);
      if (int cmd = pMenu->TrackPopupMenu(TPM_RETURNCMD, point.x, point.y, this))
        SendMessage(WM_SYSCOMMAND, cmd, MAKELPARAM(point.x, point.y));
    }
  }
  CDialogEx::OnNcLButtonDown(nHitTest, point);
}


LRESULT CFileManagerDlg::OnKickIdle(WPARAM, LPARAM)
{
  UpdateDialogControls(this, TRUE);
  return 0;
}


void CFileManagerDlg::OnInitMenuPopup(CMenu* pMenu, UINT nIndex, BOOL bSysMenu)
{
  // 	CDialogEx::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);
  AfxCancelModes(m_hWnd);

  if (bSysMenu)
    return;     // don't support system menu

  ENSURE_VALID(pMenu);

  // check the enabled state of various menu items

  CCmdUI state;
  state.m_pMenu = pMenu;
  ASSERT(state.m_pOther == NULL);
  ASSERT(state.m_pParentMenu == NULL);

  // determine if menu is popup in top-level menu and set m_pOther to
  //  it if so (m_pParentMenu == NULL indicates that it is secondary popup)
  HMENU hParentMenu;
  if (AfxGetThreadState()->m_hTrackingMenu == pMenu->m_hMenu)
    state.m_pParentMenu = pMenu;    // parent == child for tracking popup
  else if ((hParentMenu = ::GetMenu(m_hWnd)) != NULL)
  {
    CWnd* pParent = GetTopLevelParent();
    // child windows don't have menus -- need to go to the top!
    if (pParent != NULL &&
      (hParentMenu = pParent->GetMenu()->GetSafeHmenu()) != NULL)
    {
      int nIndexMax = ::GetMenuItemCount(hParentMenu);
      for (int nItemIndex = 0; nItemIndex < nIndexMax; nItemIndex++)
      {
        if (::GetSubMenu(hParentMenu, nItemIndex) == pMenu->m_hMenu)
        {
          // when popup is found, m_pParentMenu is containing menu
          state.m_pParentMenu = CMenu::FromHandle(hParentMenu);
          break;
        }
      }
    }
  }

  state.m_nIndexMax = pMenu->GetMenuItemCount();
  for (state.m_nIndex = 0; state.m_nIndex < state.m_nIndexMax;
    state.m_nIndex++)
  {
    state.m_nID = pMenu->GetMenuItemID(state.m_nIndex);
    if (state.m_nID == 0)
      continue; // menu separator or invalid cmd - ignore it

    ASSERT(state.m_pOther == NULL);
    ASSERT(state.m_pMenu != NULL);
    if (state.m_nID == (UINT)-1)
    {
      // possibly a popup menu, route to first item of that popup
      state.m_pSubMenu = pMenu->GetSubMenu(state.m_nIndex);
      if (state.m_pSubMenu == NULL ||
        (state.m_nID = state.m_pSubMenu->GetMenuItemID(0)) == 0 ||
        state.m_nID == (UINT)-1)
      {
        continue;       // first item of popup can't be routed to
      }
      state.DoUpdate(this, FALSE);    // popups are never auto disabled
    }
    else
    {
      // normal menu item
      // Auto enable/disable if frame window has 'm_bAutoMenuEnable'
      //    set and command is _not_ a system command.
      state.m_pSubMenu = NULL;
      state.DoUpdate(this, state.m_nID < 0xF000);
    }

    // adjust for menu deletions and additions
    UINT nCount = pMenu->GetMenuItemCount();
    if (nCount < state.m_nIndexMax)
    {
      state.m_nIndex -= (state.m_nIndexMax - nCount);
      while (state.m_nIndex < nCount &&
        pMenu->GetMenuItemID(state.m_nIndex) == state.m_nID)
      {
        state.m_nIndex++;
      }
    }
    state.m_nIndexMax = nCount;
  }
}


void CFileManagerDlg::OnSize(UINT nType, int cx, int cy)
{
  CDialogEx::OnSize(nType, cx, cy);

  if (nType == SIZE_MAXIMIZED)
    m_bMaximized = TRUE;
  else// if (nType == SIZE_RESTORED)
    m_bMaximized = FALSE;

  ArrangeControls();

  PrepareBackground();

  Invalidate();
}


void CFileManagerDlg::ArrangeControls()
{
  CRect rtClient;
  GetClientRect(&rtClient);
  ClientToScreen(&rtClient);
  CRect rtWindow;
  GetWindowRect(&rtWindow);

  CButton **pSysButtons;
  UINT nSysButtons;
  GetSystemButtons(pSysButtons, nSysButtons);
  if (nSysButtons == 0 || !::IsWindow(pSysButtons[0]->m_hWnd))
    return;
//   for (unsigned int i = 0; i < nSysButtons; i++)
//     pSysButtons[i]->ShowWindow(m_bMaximized ? SW_HIDE : SW_SHOW);

//   if (m_bMaximized)
//     SetWindowRgn(NULL, TRUE);
//   else {
//     CRgn rgn;
//     rgn.CreateRoundRectRgn(rtClient.left - rtWindow.left,
//       rtClient.top - rtWindow.top,
//       rtClient.right - rtWindow.left,
//       rtClient.bottom - rtWindow.top,
//       5, 5);
// //     rgn.CreateRectRgn(rtClient.left - rtWindow.left,
// //       rtClient.top - rtWindow.top,
// //       rtClient.right - rtWindow.left,
// //       rtClient.bottom - rtWindow.top);
//     SetWindowRgn(rgn, TRUE);
//   }

  GetClientRect(&rtClient);

  BITMAP bmTop, bmNavBar, bmBottom, bmTitle;
  m_bmpBGTop.GetBitmap(&bmTop);
  m_bmpNavBar.GetBitmap(&bmNavBar);
  m_bmpBGBottom.GetBitmap(&bmBottom);
  m_bmpTitle.GetBitmap(&bmTitle);

  int top_margin = 0;
  /*if (!m_bMaximized)*/ {
    CRect rtItem;
    GetDlgItem(IDC_PIC_TITLE)->GetWindowRect(&rtItem);
    ScreenToClient(&rtItem);
    top_margin = rtItem.top + bmTitle.bmHeight;
  }

  UINT nTotal = 0;

  CButton **pToolButtons;
  UINT nToolButtons;
  GetToolButtons(pToolButtons, nToolButtons);

  CWnd **pNavControls;
  UINT nNavControls;
  GetNavControls(pNavControls, nNavControls);

  nTotal = nToolButtons + nNavControls + 1;
  HDWP hdwp = BeginDeferWindowPos(nTotal);

  for (unsigned int i = 0; i < nToolButtons; i++) {
    CRect rtButton;
    pToolButtons[i]->GetWindowRect(&rtButton);
    ScreenToClient(&rtButton);
    hdwp = DeferWindowPos(hdwp, *pToolButtons[i], NULL,
      rtButton.left,
      rtClient.top + top_margin + (bmTop.bmHeight - top_margin - rtButton.Height()) * 2 / 3,
      rtButton.Width(), rtButton.Height(),
      SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOSIZE);
  }

  for (unsigned int i = 0; i < nNavControls - 1; i++) {
    CRect rtCtrl;
    pNavControls[i]->GetWindowRect(&rtCtrl);
    ScreenToClient(&rtCtrl);
    hdwp = DeferWindowPos(hdwp, *pNavControls[i], NULL,
      rtCtrl.left,
      rtClient.top + bmTop.bmHeight + (bmNavBar.bmHeight - rtCtrl.Height()) / 2,
      rtCtrl.Width(), rtCtrl.Height(),
      SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOSIZE);
  }
  CRect rtLast;
  pNavControls[nNavControls - 1]->GetWindowRect(&rtLast);
  ScreenToClient(&rtLast);
  hdwp = DeferWindowPos(hdwp, *pNavControls[nNavControls - 1], NULL,
    rtLast.left,
    rtClient.top + bmTop.bmHeight + (bmNavBar.bmHeight - rtLast.Height()) / 2,
    rtClient.right - rtLast.left - 5, rtLast.Height(),
    SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE);

  CRect rt(rtClient.left,
    rtClient.top + bmTop.bmHeight + bmNavBar.bmHeight,
    rtClient.right,
    rtClient.bottom - bmBottom.bmHeight);
  CFrameWnd *pFrame = GetCenterFrame();
  hdwp = DeferWindowPos(hdwp, *pFrame, NULL,
    rt.left, rt.top, rt.Width(), rt.Height(),
    SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE);
  pFrame->ShowWindow(SW_SHOW);

  CStatusBar *statusBar = GetStatusBar();
  hdwp = DeferWindowPos(hdwp, *statusBar, NULL,
    rt.left, rt.bottom, rt.Width(), rtClient.bottom - rt.bottom,
    SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE);

  EndDeferWindowPos(hdwp);
}


void CFileManagerDlg::OnClose()
{
  theApp.SaveState();
  SaveState();

  CDialogEx::OnClose();
}


void CFileManagerDlg::LoadState()
{
  CSettingsStoreSP regSP;
  CSettingsStore& reg = regSP.Create(FALSE, TRUE);

  if (reg.Open(theApp.GetRegSectionPath(RP_WINDOW_INFO))) {
    int nMaximized = 0;
    reg.Read(RK_MAXIMIZED, nMaximized);
    if (nMaximized != 0)
      SendMessage(WM_SYSCOMMAND, SC_MAXIMIZE);
    else {
      CRect rt;
      if (reg.Read(RK_POSITION, rt))
        MoveWindow(&rt);
    }
  }
}


void CFileManagerDlg::SaveState()
{
  CSettingsStoreSP regSP;
  CSettingsStore& reg = regSP.Create(FALSE, FALSE);

  if (reg.CreateKey(theApp.GetRegSectionPath(RP_WINDOW_INFO))) {
    reg.Write(RK_MAXIMIZED, m_bMaximized ? 1 : 0);
    if (!m_bMaximized) {
      CRect rt;
      GetWindowRect(&rt);
      reg.Write(RK_POSITION, rt);
    }
  }
}


void CFileManagerDlg::OnDropFiles(HDROP hDropInfo)
{
  // TODO: Add your message handler code here and/or call default

  CDialogEx::OnDropFiles(hDropInfo);
}
