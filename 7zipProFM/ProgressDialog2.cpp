// ProgressDialog.cpp : implementation file
//

#include "stdafx.h"

#include "CPP/Common/IntToString.h"
#include "CPP/Common/StringConvert.h"

#include "CPP/Windows/ErrorMsg.h"

#include "CPP/7zip/UI/FileManager/LangUtils.h"

#include "ProgressDialog2.h"
#include "FMUtils.h"
#include "ImageUtils.h"

#define _USE_MATH_DEFINES
#include <math.h>

#pragma comment(lib, "Pdh.lib")


static const UINT_PTR kTimerID = 3;

static const UINT kCloseMessage = WM_APP + 1;
// we can't use WM_USER, since WM_USER can be used by standard Windows procedure for Dialog

static const UINT kTimerElapse =
  #ifdef UNDER_CE
  500
  #else
  200
  #endif
  ;

static const UINT kCreateDelay =
  #ifdef UNDER_CE
  2500
  #else
  500
  #endif
  ;

static const DWORD kPauseSleepTime = 100;

#ifdef LANG

static const UInt32 kLangIDs[] =
{
  IDC_PROGRESS_BACKGROUND,
  IDC_PAUSE
};

#endif


#define UNDEFINED_VAL ((UInt64)(Int64)-1)
#define INIT_AS_UNDEFINED(v) v = UNDEFINED_VAL;
#define IS_UNDEFINED_VAL(v) ((v) == UNDEFINED_VAL)
#define IS_DEFINED_VAL(v) ((v) != UNDEFINED_VAL)

CProgressSync::CProgressSync():
    _stopped(false), _paused(false),
    _bytesProgressMode(true),
    _totalBytes(UNDEFINED_VAL), _completedBytes(0),
    _totalFiles(UNDEFINED_VAL), _curFiles(0),
    _inSize(UNDEFINED_VAL),
    _outSize(UNDEFINED_VAL),
    _isDir(false)
    {}

#define CHECK_STOP  if (_stopped) return E_ABORT; if (!_paused) return S_OK;
#define CRITICAL_LOCK CSingleLock lock(&_cs, TRUE);

bool CProgressSync::Get_Paused()
{
  CRITICAL_LOCK
  return _paused;
}

HRESULT CProgressSync::CheckStop()
{
  for (;;)
  {
    {
      CRITICAL_LOCK
      CHECK_STOP
    }
    ::Sleep(kPauseSleepTime);
  }
}

HRESULT CProgressSync::ScanProgress(UInt64 numFiles, UInt64 totalSize, const FString &fileName, bool isDir)
{
  {
    CRITICAL_LOCK
    _totalFiles = numFiles;
    _totalBytes = totalSize;
    _filePath = fs2us(fileName);
    _isDir = isDir;
    // _completedBytes = 0;
    CHECK_STOP
  }
  return CheckStop();
}

HRESULT CProgressSync::Set_NumFilesTotal(UInt64 val)
{
  {
    CRITICAL_LOCK
    _totalFiles = val;
    CHECK_STOP
  }
  return CheckStop();
}

void CProgressSync::Set_NumBytesTotal(UInt64 val)
{
  CRITICAL_LOCK
  _totalBytes = val;
}

void CProgressSync::Set_NumFilesCur(UInt64 val)
{
  CRITICAL_LOCK
  _curFiles = val;
}

HRESULT CProgressSync::Set_NumBytesCur(const UInt64 *val)
{
  {
    CRITICAL_LOCK
    if (val)
      _completedBytes = *val;
    CHECK_STOP
  }
  return CheckStop();
}

HRESULT CProgressSync::Set_NumBytesCur(UInt64 val)
{
  {
    CRITICAL_LOCK
    _completedBytes = val;
    CHECK_STOP
  }
  return CheckStop();
}

void CProgressSync::Set_Ratio(const UInt64 *inSize, const UInt64 *outSize)
{
  CRITICAL_LOCK
  if (inSize)
    _inSize = *inSize;
  if (outSize)
    _outSize = *outSize;
}

void CProgressSync::Set_TitleFileName(const UString &fileName)
{
  CRITICAL_LOCK
  _titleFileName = fileName;
}

void CProgressSync::Set_Status(const UString &s)
{
  CRITICAL_LOCK
  _status = s;
}

HRESULT CProgressSync::Set_Status2(const UString &s, const wchar_t *path, bool isDir)
{
  {
    CRITICAL_LOCK
    _status = s;
    if (path)
      _filePath = path;
    else
      _filePath.Empty();
    _isDir = isDir;
  }
  return CheckStop();
}

void CProgressSync::Set_FilePath(const wchar_t *path, bool isDir)
{
  CRITICAL_LOCK
  if (path)
    _filePath = path;
  else
    _filePath.Empty();
  _isDir = isDir;
}


void CProgressSync::AddError_Message(const wchar_t *message)
{
  CRITICAL_LOCK
  Messages.Add(message);
}

void CProgressSync::AddError_Message_Name(const wchar_t *message, const wchar_t *name)
{
  UString s;
  if (name && *name != 0)
    s += name;
  if (message && *message != 0 )
  {
    if (!s.IsEmpty())
      s.Add_LF();
    s += message;
    if (!s.IsEmpty() && s.Back() == L'\n')
      s.DeleteBack();
  }
  AddError_Message(s);
}

void CProgressSync::AddError_Code_Name(DWORD systemError, const wchar_t *name)
{
  UString s = NWindows::NError::MyFormatMessage(systemError);
  if (systemError == 0)
    s = L"Error";
  AddError_Message_Name(s, name);
}


// CProgressDialog dialog

IMPLEMENT_DYNAMIC(CProgressDialog, CDialogEx)

CProgressDialog::CProgressDialog(CWnd* pParent /*=NULL*/)
  : CDialogEx(CProgressDialog::IDD, pParent)
{
  _timer = 0;
  CompressingMode = true;
  _isDir = false;

  _numMessages = 0;
  MessagesDisplayed = false;
  _wasCreated = false;
  _needClose = false;
  _inCancelMessageBox = false;
  _externalCloseMessageWasReceived = false;
  
  _numPostedMessages = 0;
  _numAutoSizeMessages = 0;
  _errorsWereDisplayed = false;
  _waitCloseByCancelButton = false;
  _cancelWasPressed = false;
  ShowCompressionInfo = true;
  WaitMode = false;

  showProgress = true;

  _progressBar_Range = 0;
  _progressBar_Pos = 0;

  m_hFont = NULL;

  ZeroMemory(&m_lg, sizeof(m_lg));
  m_lg.lfHeight = -11;
  m_lg.lfWeight = 400;
  _tcscpy(m_lg.lfFaceName, _T("Aldo"));

  m_hIcon = NULL;

  m_tbpFlags = TBPF_NORMAL;
  m_hWndBind = NULL;
  m_bTimer = FALSE;
  m_nStep = 0;
  m_nRange = 0;
  m_nCurProgress = 0;

  PdhOpenQuery(NULL, NULL, &cpuQuery);
  PdhAddCounter(cpuQuery, L"\\Processor(_Total)\\% Processor Time", NULL, &cpuTotal);
  PdhCollectQueryData(cpuQuery);

  DWORD installed = 0;
  m_hFont = AddResourceFont(MAKEINTRESOURCE(IDF_ALDO_PC), &installed);
}

CProgressDialog::~CProgressDialog()
{
#ifndef _SFX
  AddToTitle(L"");
#endif
  if (m_hFont != NULL)
    RemoveFontMemResourceEx(m_hFont);
}

void CProgressDialog::DoDataExchange(CDataExchange* pDX)
{
  CDialogEx::DoDataExchange(pDX);
  DDX_Control(pDX, IDC_PROGRESS_BACKGROUND, m_btnBackground);
  DDX_Control(pDX, IDC_PAUSE, m_btnPause);
  DDX_Control(pDX, IDCANCEL, m_btnCancel);
}


BEGIN_MESSAGE_MAP(CProgressDialog, CDialogEx)
  ON_WM_ERASEBKGND()
  ON_WM_PAINT()
  ON_WM_SIZE()
  ON_WM_TIMER()
  ON_MESSAGE(kCloseMessage, &CProgressDialog::OnExternalCloseMessage)
  ON_BN_CLICKED(IDC_PROGRESS_BACKGROUND, &CProgressDialog::OnBnClickedProgressBackground)
  ON_BN_CLICKED(IDC_PAUSE, &CProgressDialog::OnBnClickedPause)
  ON_BN_CLICKED(IDCANCEL, &CProgressDialog::OnCancel)
  ON_WM_NCHITTEST()
END_MESSAGE_MAP()


static const unsigned kTitleFileNameSizeLimit = 36;
static const unsigned kCurrentFileNameSizeLimit = 56;


// CProgressDialog message handlers


BOOL CProgressDialog::OnInitDialog()
{
  CDialogEx::OnInitDialog();

  INIT_AS_UNDEFINED(_progressBar_Range);
  INIT_AS_UNDEFINED(_progressBar_Pos);

  INIT_AS_UNDEFINED(_prevPercentValue);
  INIT_AS_UNDEFINED(_prevElapsedSec);
  INIT_AS_UNDEFINED(_prevRemainingSec);

  INIT_AS_UNDEFINED(_prevSpeed);
  _prevSpeed_MoveBits = 0;
  
  _prevTime = ::GetTickCount();
  _elapsedTime = 0;

  INIT_AS_UNDEFINED(_totalBytes_Prev);
  INIT_AS_UNDEFINED(_processed_Prev);
  INIT_AS_UNDEFINED(_packed_Prev);
  INIT_AS_UNDEFINED(_ratio_Prev);
  _filesStr_Prev.Empty();

  _foreground = true;

  _wasCreated = true;
  _dialogCreatedEvent.Set();

  #ifdef LANG
  LangSetDlgItems(*this, kLangIDs, ARRAY_SIZE(kLangIDs));
  #endif

  CString str;
  m_btnBackground.GetWindowText(str);
  _background_String = GetUnicodeString(str);
  _backgrounded_String = _background_String;
  _backgrounded_String.RemoveChar(L'&');

  m_btnPause.GetWindowText(str);
  _pause_String = GetUnicodeString(str);

  LangString(IDS_PROGRESS_FOREGROUND, _foreground_String);
  LangString(IDS_CONTINUE, _continue_String);
  LangString(IDS_PROGRESS_PAUSED, _paused_String);

  ::SetWindowTextW(m_hWnd, _title);
  SetPauseText();
  SetPriorityText();

  EnableErrorsControls(false);

  _numReduceSymbols = kCurrentFileNameSizeLimit;

  _timer = SetTimer(kTimerID, kTimerElapse, NULL);
  #ifdef UNDER_CE
  Foreground();
  #endif

  CheckNeedClose();

  SetTaskbarProgressState();

  m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

  // Set the icon for this dialog.  The framework does this automatically
  //  when the application's main window is not a dialog
  SetIcon(m_hIcon, TRUE);			// Set big icon
  SetIcon(m_hIcon, FALSE);		// Set small icon

  CDC *pDC = GetDC();
  m_bmpBkgnd.LoadBitmap(IDB_DLG_BKGND);
  CBitmap *pBitmaps[] = {
    &m_bmpArrow,
    &m_bmpPerfBkgnd, &m_bmpPerfMemo,
    &m_bmpTimeBkgnd, &m_bmpTimeFrgnd,
    &m_bmpStatusBkgnd, &m_bmpStatusHr,
    &m_bmpLogBkgnd,
  };
  INT nBmpIDs[] = {
    IDB_PROG_ARROW,
    IDB_PROG_PERF_BKGND, IDB_PROG_PERF_MEMO,
    IDB_PROG_TIME_BKGND, IDB_PROG_TIME_FRGND,
    IDB_PROG_STATUS_BKGND, IDB_PROG_STATUS_HR,
    IDB_PROG_LOG_BKGND,
  };
  for (unsigned i = 0; i < ARRAY_SIZE(pBitmaps); i++) {
    CImageUtils::LoadBitmapFromPNG(nBmpIDs[i], *pBitmaps[i]);
    CImageUtils::PremultiplyBitmapAlpha(*pDC, *pBitmaps[i]);
  }
  ReleaseDC(pDC);

  CButtonST *pButtons[] = {
    &m_btnBackground, &m_btnPause, &m_btnCancel,
  };
#ifdef DRAW_BUTTON_IMAGE
  INT nButtonIDs[] = {
    IDB_PROG_BTN_BKGND, IDB_PROG_BTN_PAUSE, IDB_PROG_BTN_CANCEL
  };
#endif
  for (unsigned i = 0; i < ARRAY_SIZE(pButtons); i++) {
#ifdef DRAW_BUTTON_IMAGE
    pButtons[i]->SetPNGBitmaps(nButtonIDs[i]);
    pButtons[i]->DrawBorder(FALSE);
    pButtons[i]->SetWindowTextW(L"");
#endif
    pButtons[i]->DrawTransparent();
    pButtons[i]->SetAlign(CButtonST::ST_ALIGN_VERT);
    pButtons[i]->SetColor(CButtonST::BTNST_COLOR_FG_IN, RGB(255, 255, 255));
    pButtons[i]->SetColor(CButtonST::BTNST_COLOR_FG_OUT, RGB(255, 255, 255));
    pButtons[i]->SetColor(CButtonST::BTNST_COLOR_FG_FOCUS, RGB(255, 255, 255));
  }

  SetBindWindow();

  CRect rtClient;
  GetClientRect(&rtClient);
  ClientToScreen(&rtClient);
  CRect rtWindow;
  GetWindowRect(&rtWindow);

  CRgn rgn;
  rgn.CreateRoundRectRgn(rtClient.left - rtWindow.left,
    rtClient.top - rtWindow.top,
    rtClient.right - rtWindow.left,
    rtClient.bottom - rtWindow.top,
    5, 5);
  SetWindowRgn(rgn, TRUE);

  return TRUE;  // return TRUE unless you set the focus to a control
  // EXCEPTION: OCX Property Pages should return FALSE
}


BOOL CProgressDialog::OnEraseBkgnd(CDC* pDC)
{
  CRect rt;
  GetClientRect(&rt);
  CBrush brush;
  brush.CreatePatternBrush(&m_bmpBkgnd);
  pDC->FillRect(&rt, &brush);
  return TRUE;
  //return CDialogEx::OnEraseBkgnd(pDC);
}


void CProgressDialog::OnPaint()
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
    CPaintDC dc(this); // device context for painting
    CRect rt;
    GetClientRect(&rt);
    CDC dcMem;
    dcMem.CreateCompatibleDC(&dc);
    dcMem.SelectObject(&m_bmpMem);
    DrawContents(&dcMem);
    dc.BitBlt(0, 0, rt.Width(), rt.Height(), &dcMem, 0, 0, SRCCOPY);
//     CDialogEx::OnPaint();
  }
}


static int baseLeft = 65;
static int baseTop = 70;

void CProgressDialog::DrawContents(CDC *pDC)
{
  int nSaved = pDC->SaveDC();

  CRect rt;
  GetClientRect(&rt);

  CBrush brush;
  brush.CreatePatternBrush(&m_bmpBkgnd);
  pDC->FillRect(&rt, &brush);

  pDC->SetBkMode(TRANSPARENT);
  pDC->SetTextColor(RGB(255, 255, 255));
  m_lg.lfHeight = 20;
  m_lg.lfWidth = 0;
  CFont font;
  font.CreateFontIndirect(&m_lg);
  pDC->SelectObject(font);

  CString strTitle;
  GetWindowText(strTitle);
  CRect rtTitle(rt.left, rt.top + baseTop / 4, rt.right, baseTop);
  pDC->DrawText(strTitle, strTitle.GetLength(), &rtTitle, DT_CENTER | DT_VCENTER | DT_NOCLIP);

  DrawPerformance(pDC);
  DrawTime(pDC);
  DrawSystemStatus(pDC);
  DrawCurrentLog(pDC);

  pDC->RestoreDC(nSaved);
}


/*
 * perf-background
 * arrows
 * perf-memo
 * perf-text
 */
void CProgressDialog::DrawPerformance(CDC *pDC)
{
  int nSaved = pDC->SaveDC();

  BITMAP bmBkgnd, bmMemo;
  m_bmpPerfBkgnd.GetBitmap(&bmBkgnd);
  m_bmpPerfMemo.GetBitmap(&bmMemo);

  CDC dcMem;
  dcMem.CreateCompatibleDC(pDC);

  int marginLeft = baseLeft;
  int marginTop = baseTop;
  int radius = bmBkgnd.bmHeight / 2;

  BLENDFUNCTION bf;
  memset(&bf, 0, sizeof(bf));
  bf.BlendOp = AC_SRC_OVER;
  bf.BlendFlags = 0;
  bf.SourceConstantAlpha = 255;
  bf.AlphaFormat = AC_SRC_ALPHA;

  dcMem.SelectObject(&m_bmpPerfBkgnd);
  pDC->AlphaBlend(marginLeft, marginTop, bmBkgnd.bmWidth, bmBkgnd.bmHeight,
    &dcMem, 0, 0, bmBkgnd.bmWidth, bmBkgnd.bmHeight, bf);

  // Mbps
  double Mbps;
  if (IS_UNDEFINED_VAL(_prevSpeed))
    Mbps = 0;
  else
    Mbps = _prevSpeed / 1024.0;
  if (Mbps < 2)
    DrawArrow(pDC, marginLeft + radius, marginTop + radius, (int)(radius * 0.85), m_bmpArrow, 60, 90, 2, 2 - Mbps);
  else if (Mbps < 5)
    DrawArrow(pDC, marginLeft + radius, marginTop + radius, (int)(radius * 0.85), m_bmpArrow, 45, 60, 3, 5 - Mbps);
  else if (Mbps < 10)
    DrawArrow(pDC, marginLeft + radius, marginTop + radius, (int)(radius * 0.85), m_bmpArrow, 30, 45, 5, 10 - Mbps);
  else if (Mbps < 50)
    DrawArrow(pDC, marginLeft + radius, marginTop + radius, (int)(radius * 0.85), m_bmpArrow, -30, 30, 40, 50 - Mbps);
  else if (Mbps < 100)
    DrawArrow(pDC, marginLeft + radius, marginTop + radius, (int)(radius * 0.85), m_bmpArrow, -60, -30, 50, 100 - Mbps);
  else
    DrawArrow(pDC, marginLeft + radius, marginTop + radius, (int)(radius * 0.85), m_bmpArrow, -62);

  // bpb
  if (IS_DEFINED_VAL(_packed_Prev) && IS_DEFINED_VAL(_processed_Prev) &&
    _processed_Prev > 0) {
    UInt64 NEnc = _packed_Prev;
    UInt64 NDec = _processed_Prev;
    double bpb = (8.0 * NEnc) / NDec;
    DrawArrow(pDC, marginLeft + radius, marginTop + radius, (int)(radius * 0.85), m_bmpArrow, 90, 180, 8, bpb);
  }

  dcMem.SelectObject(&m_bmpPerfMemo);
  pDC->AlphaBlend(marginLeft + bmBkgnd.bmWidth - bmMemo.bmWidth - 15,
    marginTop + radius - bmMemo.bmHeight / 2,
    bmMemo.bmWidth, bmMemo.bmHeight,
    &dcMem, 0, 0, bmMemo.bmWidth, bmMemo.bmHeight, bf);

  // Draw Text
  char szBuff[128];
  FString str;
  CFont font;

  pDC->SetBkMode(TRANSPARENT);

  // Draw RATIO
  int ratio;
  if (IS_UNDEFINED_VAL(_ratio_Prev))
    ratio = 0;
  else
    ratio = (int)_ratio_Prev;
  sprintf_s(szBuff, "%d%%", ratio);
  str = GetSystemString(szBuff);

  pDC->SetTextColor(RGB(255, 255, 255));
  m_lg.lfHeight = 35;
  m_lg.lfWidth = 10;
  font.CreateFontIndirect(&m_lg);
  pDC->SelectObject(font);

  CRect rt(marginLeft + radius - 25, marginTop + radius - 10,
    marginLeft + radius + 25, marginTop + radius + 30);
  pDC->DrawText(str, str.Len(), &rt, DT_CENTER | DT_VCENTER | DT_NOCLIP);

  // Draw Files
  font.DeleteObject();
  m_lg.lfHeight = 30;
  m_lg.lfWidth = 9;
  font.CreateFontIndirect(&m_lg);
  pDC->SelectObject(font);
  str = GetSystemString(_filesStr_Prev);
  rt = CRect(marginLeft + radius + 40, marginTop + radius + radius / 2 - 15,
    marginLeft + radius + 120, marginTop + radius + radius / 2 + 20);
  pDC->DrawText(str, str.Len(), &rt, DT_CENTER | DT_VCENTER | DT_NOCLIP);

  // Draw Compressed & Decompressed Size
  pDC->SetTextColor(RGB(255, 237, 0));
  font.DeleteObject();
  m_lg.lfHeight = 20;
  m_lg.lfWidth = 7;
  font.CreateFontIndirect(&m_lg);
  pDC->SelectObject(font);

  // Draw Compressed Size
  if (IS_UNDEFINED_VAL(_packed_Prev))
    str = _T("");
  else
    str = GetSystemString(ConvertIntToDecimalString(_packed_Prev) + L" bytes");
  rt = CRect(marginLeft + radius + 35, marginTop + radius - 25,
    marginLeft + radius * 2 + 15, marginTop + radius);
  pDC->DrawText(str, str.Len(), &rt, DT_RIGHT | DT_VCENTER | DT_NOCLIP);

  // Draw Decompressed Size
  if (IS_UNDEFINED_VAL(_processed_Prev))
    str = _T("");
  else
    str = GetSystemString(ConvertIntToDecimalString(_processed_Prev) + L" bytes");
  rt = CRect(marginLeft + radius + 35, marginTop + radius + 17,
    marginLeft + radius * 2 + 15, marginTop + radius + 40);
  pDC->DrawText(str, str.Len(), &rt, DT_RIGHT | DT_VCENTER | DT_NOCLIP);

  pDC->RestoreDC(nSaved);
}


/*
 * time text
 * time-background
 * time-foreground
 */
void CProgressDialog::DrawTime(CDC *pDC)
{
  int nSaved = pDC->SaveDC();

  BITMAP bmPerfBkgnd, bmPerfMemo, bmBkgnd, bmFrgnd;
  m_bmpPerfBkgnd.GetBitmap(&bmPerfBkgnd);
  m_bmpPerfMemo.GetBitmap(&bmPerfMemo);
  m_bmpTimeBkgnd.GetBitmap(&bmBkgnd);
  m_bmpTimeFrgnd.GetBitmap(&bmFrgnd);

  CDC dcMem;
  dcMem.CreateCompatibleDC(pDC);

  int marginLeft = baseLeft + bmPerfBkgnd.bmWidth - bmBkgnd.bmWidth + 32;
  int marginTop = baseTop + (bmPerfBkgnd.bmHeight - bmPerfMemo.bmHeight) / 2 - bmBkgnd.bmHeight - 5;
  int radius = bmBkgnd.bmHeight / 2;

  BLENDFUNCTION bf;
  memset(&bf, 0, sizeof(bf));
  bf.BlendOp = AC_SRC_OVER;
  bf.BlendFlags = 0;
  bf.SourceConstantAlpha = 255;
  bf.AlphaFormat = AC_SRC_ALPHA;

  dcMem.SelectObject(&m_bmpTimeBkgnd);
  pDC->AlphaBlend(marginLeft, marginTop, bmBkgnd.bmWidth, bmBkgnd.bmHeight,
    &dcMem, 0, 0, bmBkgnd.bmWidth, bmBkgnd.bmHeight, bf);

  // Foreground
  double ratio = (double)(_progressBar_Pos * 100 / _progressBar_Range) / 100.0;
  CBitmap bmpRatio;
  bmpRatio.CreateCompatibleBitmap(pDC, bmFrgnd.bmWidth, bmFrgnd.bmHeight);
  CImageUtils::PremultiplyBitmapAlpha(*pDC, m_bmpTimeFrgnd, (float)ratio, bmpRatio);
  dcMem.SelectObject(&bmpRatio);
  pDC->AlphaBlend(marginLeft, marginTop, bmFrgnd.bmWidth, bmFrgnd.bmHeight,
    &dcMem, 0, 0, bmFrgnd.bmWidth, bmFrgnd.bmHeight, bf);

  // Draw Text
  char szBuff[128];
  FString str;
  CFont font;

  pDC->SetBkMode(TRANSPARENT);
  pDC->SetTextColor(RGB(255, 255, 255));

  // Draw time
  UInt64 elapsedSec = _elapsedTime / 1000;
  UInt64 hours = elapsedSec / 3600;
  UInt32 seconds = (UInt32)(elapsedSec - hours * 3600);
  UInt32 minutes = seconds / 60;
  seconds %= 60;
  sprintf_s(szBuff, "%02d.%02d.%02d", hours, minutes, seconds);
  str = GetSystemString(szBuff);

  m_lg.lfHeight = 30;
  m_lg.lfWidth = 10;
  font.CreateFontIndirect(&m_lg);
  pDC->SelectObject(font);

  CRect rt(marginLeft + 15, marginTop + radius - 15,
    marginLeft + bmBkgnd.bmWidth - 15, marginTop + radius + 15);
  pDC->DrawText(str, str.Len(), &rt, DT_CENTER | DT_VCENTER | DT_NOCLIP);

  font.DeleteObject();
  m_lg.lfHeight = 9;
  m_lg.lfWidth = 0;
  font.CreateFontIndirect(&m_lg);
  pDC->SelectObject(font);
  pDC->TextOut(marginLeft + radius - 16, marginTop + radius - 11, _T("h"), 1);
  pDC->TextOut(marginLeft + radius + 11, marginTop + radius - 11, _T("m"), 1);
  pDC->TextOut(marginLeft + radius + 39, marginTop + radius - 11, _T("s"), 1);

  pDC->RestoreDC(nSaved);
}


/*
 * status-background
 * status text
 * arrows
 * status-hr
 */
void CProgressDialog::DrawSystemStatus(CDC *pDC)
{
  int nSaved = pDC->SaveDC();

  BITMAP bmPerfBkgnd, bmPerfMemo, bmBkgnd, bmHr;
  m_bmpPerfBkgnd.GetBitmap(&bmPerfBkgnd);
  m_bmpPerfMemo.GetBitmap(&bmPerfMemo);
  m_bmpStatusBkgnd.GetBitmap(&bmBkgnd);
  m_bmpStatusHr.GetBitmap(&bmHr);

  CDC dcMem;
  dcMem.CreateCompatibleDC(pDC);

  int marginLeft = baseLeft + bmPerfBkgnd.bmWidth - bmBkgnd.bmWidth + 32;
  int marginTop = baseTop + (bmPerfBkgnd.bmHeight + bmPerfMemo.bmHeight) / 2 + 5;
  int radius = bmBkgnd.bmHeight / 2;

  BLENDFUNCTION bf;
  memset(&bf, 0, sizeof(bf));
  bf.BlendOp = AC_SRC_OVER;
  bf.BlendFlags = 0;
  bf.SourceConstantAlpha = 255;
  bf.AlphaFormat = AC_SRC_ALPHA;

  dcMem.SelectObject(&m_bmpStatusBkgnd);
  pDC->AlphaBlend(marginLeft, marginTop, bmBkgnd.bmWidth, bmBkgnd.bmHeight,
    &dcMem, 0, 0, bmBkgnd.bmWidth, bmBkgnd.bmHeight, bf);

  // Draw Text
  WCHAR szBuff[128];
  CFont font;

  pDC->SetBkMode(TRANSPARENT);
  pDC->SetTextColor(RGB(255, 255, 255));

  m_lg.lfHeight = 20;
  m_lg.lfWidth = 6;
  font.CreateFontIndirect(&m_lg);
  pDC->SelectObject(font);

  // Draw Usage
  int usage = 0;
  if (!_usageHistory.empty()) {
    for (std::list<int>::iterator it = _usageHistory.begin(); it != _usageHistory.end(); it++)
      usage += *it;
    usage = usage / (int)_usageHistory.size();
  }
  wsprintf(szBuff, L"%d%%", usage);
  CRect rt(marginLeft + radius, marginTop + radius / 2 - 2,
    marginLeft + radius + 40, marginTop + radius / 2 + 15);
  ::DrawTextExW(*pDC, szBuff, (int)wcslen(szBuff), &rt, DT_CENTER | DT_VCENTER | DT_NOCLIP, NULL);
  DrawArrow(pDC, marginLeft + radius, marginTop + radius, (int)(radius * 0.85), m_bmpArrow, -90, 90, 100, usage);

  // Draw Temperature
  LONG temp;
  GetCpuTemperature(&temp);
//   if (temp < 0)
//     temp = (50 + 273) * 10;
  if (temp > 0) {
    temp = (int)(temp / 10 - 273);
    wsprintf(szBuff, L"%d\x2103", temp);
    rt = CRect(marginLeft + radius, marginTop + radius * 3 / 2 - 17,
      marginLeft + radius + 40, marginTop + radius * 3 / 2 + 2);
    ::DrawTextExW(*pDC, szBuff, (int)wcslen(szBuff), &rt, DT_CENTER | DT_VCENTER | DT_NOCLIP, NULL);
    DrawArrow(pDC, marginLeft + radius, marginTop + radius, (int)(radius * 0.85), m_bmpArrow, 90, 270, 100, temp);
  }

  dcMem.SelectObject(&m_bmpStatusHr);
  pDC->AlphaBlend(marginLeft, marginTop, bmBkgnd.bmWidth, bmBkgnd.bmHeight,
    &dcMem, 0, 0, bmBkgnd.bmWidth, bmBkgnd.bmHeight, bf);

  pDC->RestoreDC(nSaved);
}


/*
 * log-background
 * log text
 */
void CProgressDialog::DrawCurrentLog(CDC *pDC)
{
  int nSaved = pDC->SaveDC();

  BITMAP bmPerfBkgnd, bmBkgnd;
  m_bmpPerfBkgnd.GetBitmap(&bmPerfBkgnd);
  m_bmpLogBkgnd.GetBitmap(&bmBkgnd);

  CDC dcMem;
  dcMem.CreateCompatibleDC(pDC);

  int marginLeft = baseLeft;
  int marginTop = baseTop + bmPerfBkgnd.bmHeight + 30;

  BLENDFUNCTION bf;
  memset(&bf, 0, sizeof(bf));
  bf.BlendOp = AC_SRC_OVER;
  bf.BlendFlags = 0;
  bf.SourceConstantAlpha = 255;
  bf.AlphaFormat = AC_SRC_ALPHA;

  dcMem.SelectObject(&m_bmpLogBkgnd);
  pDC->AlphaBlend(marginLeft, marginTop, bmBkgnd.bmWidth, bmBkgnd.bmHeight,
    &dcMem, 0, 0, bmBkgnd.bmWidth, bmBkgnd.bmHeight, bf);

  // Draw Text
  FString str;
  CFont font;

  pDC->SetBkMode(TRANSPARENT);
  pDC->SetTextColor(RGB(255, 255, 255));

  m_lg.lfHeight = 14;
  m_lg.lfWeight = 0;
  font.CreateFontIndirect(&m_lg);
  pDC->SelectObject(font);

  str = _filePath;
//   if (!_status.IsEmpty())
//     str = _status;
  ReduceString(str, _numReduceSymbols);
  CRect rt(marginLeft + bmBkgnd.bmHeight / 2, marginTop + bmBkgnd.bmHeight / 4,
    marginLeft + bmBkgnd.bmWidth - bmBkgnd.bmHeight / 2, marginTop + bmBkgnd.bmHeight * 3 / 4);
  pDC->DrawText(str, str.Len(), &rt, DT_LEFT | DT_VCENTER | DT_NOCLIP);

  pDC->RestoreDC(nSaved);
}


void CProgressDialog::DrawArrow(CDC *pDC, int centerX, int centerY, int radius, HBITMAP hbmArrow, double degrees)
{
  CDC dcMem;
  dcMem.CreateCompatibleDC(pDC);

  // Get logical coordinates
  BITMAP bm;
  ::GetObject(hbmArrow, sizeof(bm), &bm);
  int h = radius;
  int w = radius * bm.bmWidth / bm.bmHeight / 2;

  double radians = degrees * M_PI / 180.0;

  float cosine = (float)cos(radians);
  float sine = (float)sin(radians);

  // Compute dimensions of the resulting bitmap
  // First get the coordinates of the 4 corners
  int x1 = (int)(-w * cosine - h * sine);
  int y1 = (int)( w * sine - h * cosine);
  int x2 = (int)( w * cosine - h * sine);
  int y2 = (int)(-w * sine - h * cosine);
  int x3 = (int)(-w * cosine);
  int y3 = (int)( w * sine);
  int x4 = (int)( w * cosine);
  int y4 = (int)(-w * sine);

  int minx = min(x1, min(x2, min(x3, x4)));
  int miny = min(y1, min(y2, min(y3, y4)));
  int maxx = max(x1, max(x2, max(x3, x4)));
  int maxy = max(y1, max(y2, max(y3, y4)));

  BLENDFUNCTION bf;
  memset(&bf, 0, sizeof(bf));
  bf.BlendOp = AC_SRC_OVER;
  bf.BlendFlags = 0;
  bf.SourceConstantAlpha = 255;
  bf.AlphaFormat = AC_SRC_ALPHA;

  hbmArrow = CImageUtils::GetRotatedBitmap(hbmArrow, radians);
  BITMAP bmRotated;
  ::GetObject(hbmArrow, sizeof(bmRotated), &bmRotated);
  dcMem.SelectObject(hbmArrow);
  pDC->AlphaBlend(centerX + minx, centerY + miny, maxx - minx, maxy - miny,
    &dcMem, 0, 0, bmRotated.bmWidth, bmRotated.bmHeight, bf);

  ::DeleteObject(hbmArrow);
}


void CProgressDialog::DrawArrow(CDC *pDC, int centerX, int centerY, int radius, HBITMAP hbmArrow, double from, double to, double range, double pos)
{
  double degrees = from + (to - from) * pos / range;
  DrawArrow(pDC, centerX, centerY, radius, hbmArrow, degrees);
}


void CProgressDialog::OnSize(UINT nType, int cx, int cy)
{
  CDialogEx::OnSize(nType, cx, cy);

  m_btnBackground.ResetBackground();
  m_btnPause.ResetBackground();
  m_btnCancel.ResetBackground();

  if (m_bmpMem.m_hObject != NULL) {
    BITMAP bm;
    m_bmpMem.GetBitmap(&bm);
    if (bm.bmWidth < cx || bm.bmHeight < cy)
      m_bmpMem.DeleteObject();
  }
  CDC *pDC = GetDC();
  if (m_bmpMem.m_hObject == NULL)
    m_bmpMem.CreateCompatibleBitmap(pDC, cx, cy);
  if (m_bmpBkgnd.m_hObject != NULL) {
    CDC dc;
    dc.CreateCompatibleDC(pDC);
    dc.SelectObject(&m_bmpMem);
    CBrush brush;
    brush.CreatePatternBrush(&m_bmpBkgnd);
    CRect rt(0, 0, cx, cy);
    dc.FillRect(&rt, &brush);
  }
  ReleaseDC(pDC);
}


void CProgressDialog::SetTaskbarProgressState()
{
  #ifdef __ITaskbarList3_INTERFACE_DEFINED__
  TBPFLAG tbpFlags;
  if (Sync.Get_Paused())
    tbpFlags = TBPF_PAUSED;
  else
    tbpFlags = _errorsWereDisplayed ? TBPF_ERROR: TBPF_NORMAL;
  m_tbpFlags = tbpFlags;
  if (m_pTaskbarList != NULL)
    m_pTaskbarList->SetProgressState(m_hWndBind, tbpFlags);
  #endif
}

void CProgressDialog::EnableErrorsControls(bool enable)
{
}


#ifndef _SFX
void CProgressDialog::AddToTitle(LPCWSTR s)
{
  if (MainWindow != 0)
    ::SetWindowTextW(MainWindow, (UString)s + MainTitle);
}
#endif


void CProgressDialog::SetProgressRange(UInt64 range)
{
  if (range == _progressBar_Range)
    return;
  _progressBar_Range = range;
  INIT_AS_UNDEFINED(_progressBar_Pos);
  _progressConv.Init(range);
}

void CProgressDialog::SetProgressPos(UInt64 pos)
{
  if (IS_UNDEFINED_VAL(pos))
    pos = 0;
//   if (pos >= _progressBar_Range ||
//       pos <= _progressBar_Pos ||
//       pos - _progressBar_Pos >= (_progressBar_Range >> 10))
  {
    if (m_bTimer && m_nCurProgress != pos) {
      SetTimer(kTimerID + 1, kTimerElapse, NULL);
      m_nStep = (pos - m_nCurProgress) / 10;
      if (m_nStep == 0)
        m_nStep = pos - m_nCurProgress;
    }
    else if (m_pTaskbarList != NULL) {
      m_pTaskbarList->SetProgressValue(m_hWndBind, pos, _progressBar_Range);
      m_pTaskbarList->SetProgressState(m_hWndBind, m_tbpFlags);
    }

    _progressBar_Pos = pos;
  }
}

static void ConvertSizeToString(UInt64 v, wchar_t *s)
{
  Byte c = 0;
       if (v >= ((UInt64)100000 << 20)) { v >>= 30; c = 'G'; }
  else if (v >= ((UInt64)100000 << 10)) { v >>= 20; c = 'M'; }
  else if (v >= ((UInt64)100000 <<  0)) { v >>= 10; c = 'K'; }
  ConvertUInt64ToString(v, s);
  if (c != 0)
  {
    s += MyStringLen(s);
    *s++ = ' ';
    *s++ = c;
    *s++ = 0;
  }
}

void CProgressDialog::ShowSize(int id, UInt64 val, UInt64 &prev)
{
  if (val == prev)
    return;
  prev = val;
  wchar_t s[40];
  s[0] = 0;
  if (IS_DEFINED_VAL(val))
    ConvertSizeToString(val, s);
  ::SetDlgItemText(m_hWnd, id, s);
}

static void GetChangedString(const UString &newStr, UString &prevStr, bool &hasChanged)
{
  hasChanged = !(prevStr == newStr);
  if (hasChanged)
    prevStr = newStr;
}

static unsigned GetPower32(UInt32 val)
{
  const unsigned kStart = 32;
  UInt32 mask = ((UInt32)1 << (kStart - 1));
  for (unsigned i = kStart;; i--)
  {
    if (i == 0 || (val & mask) != 0)
      return i;
    mask >>= 1;
  }
}

static unsigned GetPower64(UInt64 val)
{
  UInt32 high = (UInt32)(val >> 32);
  if (high == 0)
    return GetPower32((UInt32)val);
  return GetPower32(high) + 32;
}

static UInt64 MyMultAndDiv(UInt64 mult1, UInt64 mult2, UInt64 divider)
{
  unsigned pow1 = GetPower64(mult1);
  unsigned pow2 = GetPower64(mult2);
  while (pow1 + pow2 > 64)
  {
    if (pow1 > pow2) { pow1--; mult1 >>= 1; }
    else             { pow2--; mult2 >>= 1; }
    divider >>= 1;
  }
  UInt64 res = mult1 * mult2;
  if (divider != 0)
    res /= divider;
  return res;
}

void CProgressDialog::UpdateStatInfo(bool showAll)
{
  UInt64 total, completed, totalFiles, completedFiles, inSize, outSize;
  bool bytesProgressMode;

  bool titleFileName_Changed;
  bool curFilePath_Changed;
  bool status_Changed;
  unsigned numErrors;
  {
    CSingleLock lock(&Sync._cs, TRUE);
    total = Sync._totalBytes;
    completed = Sync._completedBytes;
    totalFiles = Sync._totalFiles;
    completedFiles = Sync._curFiles;
    inSize = Sync._inSize;
    outSize = Sync._outSize;
    bytesProgressMode = Sync._bytesProgressMode;

    GetChangedString(Sync._titleFileName, _titleFileName, titleFileName_Changed);
    GetChangedString(Sync._filePath, _filePath, curFilePath_Changed);
    GetChangedString(Sync._status, _status, status_Changed);
    if (_isDir != Sync._isDir)
    {
      curFilePath_Changed = true;
      _isDir = Sync._isDir;
    }
    numErrors = Sync.Messages.Size();
  }

  UInt32 curTime = ::GetTickCount();

  {
    UInt64 progressTotal = bytesProgressMode ? total : totalFiles;
    UInt64 progressCompleted = bytesProgressMode ? completed : completedFiles;
    
    if (IS_UNDEFINED_VAL(progressTotal))
    {
      // SetPos(0);
      // SetRange(progressCompleted);
    }
    else
    {
      if (_progressBar_Pos != 0 || progressCompleted != 0 ||
          (_progressBar_Range == 0 && progressTotal != 0))
      {
        SetProgressRange(progressTotal);
        SetProgressPos(progressCompleted);
      }
    }
  }

  _totalBytes_Prev = total;

  _elapsedTime += (curTime - _prevTime);
  _prevTime = curTime;
  UInt64 elapsedSec = _elapsedTime / 1000;
  bool elapsedChanged = false;
  if (elapsedSec != _prevElapsedSec)
  {
    _prevElapsedSec = elapsedSec;
    elapsedChanged = true;
  }

  bool needSetTitle = false;
  if (elapsedChanged || showAll)
  {
    if (numErrors > _numPostedMessages)
    {
      UpdateMessagesDialog();
      if (!_errorsWereDisplayed)
      {
        _errorsWereDisplayed = true;
        EnableErrorsControls(true);
        SetTaskbarProgressState();
      }
    }

    if (completed != 0)
    {
      if (IS_UNDEFINED_VAL(total))
      {
        if (IS_DEFINED_VAL(_prevRemainingSec))
        {
          INIT_AS_UNDEFINED(_prevRemainingSec);
        }
      }
      else
      {
        UInt64 remainingTime = 0;
        if (completed < total)
          remainingTime = MyMultAndDiv(_elapsedTime, total - completed, completed);
        UInt64 remainingSec = remainingTime / 1000;
        if (remainingSec != _prevRemainingSec)
        {
          _prevRemainingSec = remainingSec;
        }
      }
      {
        UInt64 elapsedTime = (_elapsedTime == 0) ? 1 : _elapsedTime;
        UInt64 v = (completed * 1000) / elapsedTime;
        Byte c = 0;
        unsigned moveBits = 0;
             if (v >= ((UInt64)10000 << 10)) { moveBits = 20; c = 'M'; }
        else if (v >= ((UInt64)10000 <<  0)) { moveBits = 10; c = 'K'; }
        v >>= moveBits;
        if (moveBits != _prevSpeed_MoveBits || v != _prevSpeed)
        {
          _prevSpeed_MoveBits = moveBits;
          _prevSpeed = v;
        }
      }
    }

    {
      UInt64 percent = 0;
      {
        if (IS_DEFINED_VAL(total))
        {
          percent = completed * 100;
          if (total != 0)
            percent /= total;
        }
      }
      if (percent != _prevPercentValue)
      {
        _prevPercentValue = percent;
        needSetTitle = true;
      }
    }
    
    {
      wchar_t s[64];
      ConvertUInt64ToString(completedFiles, s);
      // if (IS_DEFINED_VAL(totalFiles))
      // {
      //   wcscat(s, L".");
      //   ConvertUInt64ToString(totalFiles, s + wcslen(s));
      // }
      if (_filesStr_Prev != s)
      {
        _filesStr_Prev = s;
      }
    }
    
    const UInt64 packSize   = CompressingMode ? outSize : inSize;
    const UInt64 unpackSize = CompressingMode ? inSize : outSize;

    if (IS_UNDEFINED_VAL(unpackSize) &&
      IS_UNDEFINED_VAL(packSize))
    {
      _processed_Prev = completed;
      _packed_Prev = UNDEFINED_VAL;
    }
    else
    {
      _processed_Prev = unpackSize;
      _packed_Prev = packSize;

      if (IS_DEFINED_VAL(packSize) &&
        IS_DEFINED_VAL(unpackSize) &&
        unpackSize != 0)
      {
        UInt64 ratio = packSize * 100 / unpackSize;
        if (_ratio_Prev != ratio)
        {
          _ratio_Prev = ratio;
        }
      }
    }
  }

  if (needSetTitle || titleFileName_Changed)
    SetTitleText();

  if (curFilePath_Changed)
  {
    UString s1, s2;
    if (_isDir)
      s1 = _filePath;
    else
    {
      int slashPos = _filePath.ReverseFind_PathSepar();
      if (slashPos >= 0)
      {
        s1.SetFrom(_filePath, slashPos + 1);
        s2 = _filePath.Ptr(slashPos + 1);
      }
      else
        s2 = _filePath;
    }
    ReduceString(s1, _numReduceSymbols);
    ReduceString(s2, _numReduceSymbols);
    s1.Add_LF();
    s1 += s2;
  }

  {
    PDH_FMT_COUNTERVALUE counterVal;
    PdhCollectQueryData(cpuQuery);
    PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal);
    _usageHistory.push_back((int)(counterVal.doubleValue));
    if (_usageHistory.size() > 20)
      _usageHistory.erase(_usageHistory.begin());
  }

  RedrawClient();
}

void CProgressDialog::OnTimer(UINT_PTR nIDEvent)
{
  if (nIDEvent == kTimerID) {
    if (Sync.Get_Paused())
      return;
    CheckNeedClose();
    UpdateStatInfo(false);
  }

  if (nIDEvent == kTimerID + 1) {
    UInt64 nCurPos = m_nCurProgress;
    UInt64 nDiff = _progressBar_Pos - nCurPos;
    if (m_nStep != 0) {
      if (m_nStep > 0)
        nDiff = min(nDiff, m_nStep);
      else
        nDiff = max(nDiff, m_nStep);
    }
    if (nDiff != 0) {
      m_nCurProgress += nDiff;
      if (m_pTaskbarList != NULL) {
        m_pTaskbarList->SetProgressValue(m_hWndBind, m_nCurProgress, _progressBar_Range);
        m_pTaskbarList->SetProgressState(m_hWndBind, m_tbpFlags);
      }
    }
    else
      KillTimer(kTimerID);
  }
  return;
}

INT_PTR CProgressDialog::Create(const UString &title, HANDLE hThread,
  HWND wndParent)
{
  INT_PTR res = 0;
  try
  {
    if (WaitMode)
    {
      CWaitCursor waitCursor;
      HANDLE h[] = { hThread, _createDialogEvent };
      
      WRes res = WaitForMultipleObjects(ARRAY_SIZE(h), h, FALSE, kCreateDelay);
      if (res == WAIT_OBJECT_0 && !Sync.ThereIsMessage())
        return 0;
    }
    _title = title;
    //res = CDialogEx::Create(MAKEINTRESOURCE(IDD), CWnd::FromHandlePermanent(wndParent));
    if (showProgress)
      res = (DoModal() == IDOK);
    else {
      CreateDlg(MAKEINTRESOURCE(IDD), NULL);
      WaitForSingleObject(m_hWnd, INFINITE);
      res = 1;
    }
  }
  catch(...)
  {
    _wasCreated = true;
    _dialogCreatedEvent.Set();
  }
  WaitForSingleObject(hThread, INFINITE);
  if (!MessagesDisplayed)
    if (showProgress)
      ::MessageBoxW(wndParent, L"Progress Error", L"7-ZipPro", MB_ICONERROR);
  return res;
}


LRESULT CProgressDialog::OnExternalCloseMessage(WPARAM wParam, LPARAM lParam)
{
  KillTimer(_timer);
  _timer = 0;
  if (_inCancelMessageBox)
  {
    _externalCloseMessageWasReceived = true;
    return TRUE;
  }
  m_tbpFlags = TBPF_NOPROGRESS;
  if (m_pTaskbarList != NULL)
    m_pTaskbarList->SetProgressState(m_hWndBind, m_tbpFlags);
  // AddToTitle(L"Finished ");
  // SetText(L"Finished2 ");

  UpdateStatInfo(true);
  
  ::SetDlgItemTextW(m_hWnd, IDCANCEL, LangString(IDS_CLOSE));
  m_btnCancel.SendMessage(BM_SETSTYLE, BS_DEFPUSHBUTTON, MAKELPARAM(TRUE, 0));
  GetDlgItem(IDC_PROGRESS_BACKGROUND)->ShowWindow(SW_HIDE);
  GetDlgItem(IDC_PAUSE)->ShowWindow(SW_HIDE);
  
  bool thereAreMessages;
  CProgressFinalMessage fm;
  {
    CSingleLock lock(&Sync._cs, TRUE);
    thereAreMessages = !Sync.Messages.IsEmpty();
    fm = Sync.FinalMessage;
  }
  if (!fm.ErrorMessage.Message.IsEmpty())
  {
    MessagesDisplayed = true;
    if (fm.ErrorMessage.Title.IsEmpty())
      fm.ErrorMessage.Title = L"7-Zip";
    ::MessageBoxW(*this, fm.ErrorMessage.Message, fm.ErrorMessage.Title, MB_ICONERROR);
  }
  else if (!thereAreMessages)
  {
    MessagesDisplayed = true;
    if (!fm.OkMessage.Message.IsEmpty())
    {
      if (fm.OkMessage.Title.IsEmpty())
        fm.OkMessage.Title = L"7-Zip";
      ::MessageBoxW(*this, fm.OkMessage.Message, fm.OkMessage.Title, MB_OK);
    }
  }

  if (thereAreMessages && !_cancelWasPressed)
  {
    _waitCloseByCancelButton = true;
    UpdateMessagesDialog();
    if (!_messages.IsEmpty())
      MessageBoxW(_messages);
    //return TRUE;
  }

  EndDialog(0);
  return TRUE;
}


void CProgressDialog::SetTitleText()
{
  UString s;
  if (Sync.Get_Paused())
  {
    s += _paused_String;
    s.Add_Space();
  }
  if (IS_DEFINED_VAL(_prevPercentValue))
  {
    char temp[32];
    ConvertUInt64ToString(_prevPercentValue, temp);
    s.AddAscii(temp);
    s += L'%';
  }
  if (!_foreground)
  {
    s.Add_Space();
    s += _backgrounded_String;
  }

  s.Add_Space();
  #ifndef _SFX
  {
    unsigned len = s.Len();
    s += MainAddTitle;
    AddToTitle(s);
    s.DeleteFrom(len);
  }
  #endif

  s += _title;
  if (!_titleFileName.IsEmpty())
  {
    UString fileName = _titleFileName;
    ReduceString(fileName, kTitleFileNameSizeLimit);
    s.Add_Space();
    s += fileName;
  }
  ::SetWindowTextW(m_hWnd, s);
}

void CProgressDialog::SetPauseText()
{
  ::SetDlgItemTextW(m_hWnd, IDC_PAUSE, Sync.Get_Paused() ? _continue_String : _pause_String);
  SetTitleText();
}


void CProgressDialog::OnBnClickedPause()
{
  bool paused = !Sync.Get_Paused();
  Sync.Set_Paused(paused);
  UInt32 curTime = ::GetTickCount();
  if (paused)
    _elapsedTime += (curTime - _prevTime);
  SetTaskbarProgressState();
  _prevTime = curTime;
  SetPauseText();
}

void CProgressDialog::SetPriorityText()
{
  ::SetDlgItemTextW(m_hWnd, IDC_PROGRESS_BACKGROUND, _foreground ?
      _background_String :
      _foreground_String);
  SetTitleText();
}

void CProgressDialog::OnBnClickedProgressBackground()
{
  _foreground = !_foreground;
#ifndef UNDER_CE
  SetPriorityClass(GetCurrentProcess(), _foreground ? NORMAL_PRIORITY_CLASS : IDLE_PRIORITY_CLASS);
#endif
  SetPriorityText();
}

void CProgressDialog::AddMessage(LPCWSTR message)
{
  UString s = message;
  if (!_messages.IsEmpty())
    _messages += L'\n';
  _messages += s;
  _numMessages++;
}

static unsigned GetNumDigits(UInt32 val)
{
  unsigned i;
  for (i = 0; val >= 10; i++)
    val /= 10;
  return i;
}

void CProgressDialog::UpdateMessagesDialog()
{
  UStringVector messages;
  {
    CSingleLock lock(&Sync._cs, TRUE);
    unsigned num = Sync.Messages.Size();
    if (num > _numPostedMessages)
    {
      messages.ClearAndReserve(num - _numPostedMessages);
      for (unsigned i = _numPostedMessages; i < num; i++)
        messages.AddInReserved(Sync.Messages[i]);
      _numPostedMessages = num;
    }
  }
  if (!messages.IsEmpty())
  {
    FOR_VECTOR (i, messages)
      AddMessage(messages[i]);
    if (_numAutoSizeMessages < 256 || GetNumDigits(_numPostedMessages) > GetNumDigits(_numAutoSizeMessages))
    {
      _numAutoSizeMessages = _numPostedMessages;
    }
  }
}


void CProgressDialog::OnCancel()
{
  if (_waitCloseByCancelButton)
  {
    MessagesDisplayed = true;
    CDialogEx::OnCancel();
    return;
  }

  bool paused = Sync.Get_Paused();
  if (!paused)
    OnBnClickedPause();
  _inCancelMessageBox = true;
  int res = ::MessageBoxW(*this, LangString(IDS_PROGRESS_ASK_CANCEL), _title, MB_YESNOCANCEL);
  _inCancelMessageBox = false;
  if (!paused)
    OnBnClickedPause();
  if (res == IDCANCEL || res == IDNO)
  {
    if (_externalCloseMessageWasReceived)
      OnExternalCloseMessage(0, 0);
    return;
  }

  _cancelWasPressed = true;
  MessagesDisplayed = true;
  _wasCreated = false;
  Sync.Set_Stopped(true);
  CDialogEx::OnCancel();
}

void CProgressDialog::CheckNeedClose()
{
  if (_needClose)
  {
    PostMessage(kCloseMessage);
    _needClose = false;
  }
}

void CProgressDialog::ProcessWasFinished()
{
  // Set Window title here.
  if (!WaitMode)
    WaitCreating();
  
  if (_wasCreated)
    PostMessage(kCloseMessage);
  else
    _needClose = true;
}


HRESULT CProgressThreadVirt::Create(const UString &title,
  HWND parentWindow, bool showProgress)
{
  unsigned threadID;
  HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, &MyThreadFunction, this, 0, &threadID);
  if (hThread == NULL)
    return E_FAIL;
  ProgressDialog.showProgress = showProgress;
  ProgressDialog.Create(title, hThread, parentWindow);
  return S_OK;
}

static void AddMessageToString(UString &dest, const UString &src)
{
  if (!src.IsEmpty())
  {
    if (!dest.IsEmpty())
      dest.Add_LF();
    dest += src;
  }
}

void CProgressThreadVirt::Process()
{
  CProgressCloser closer(ProgressDialog);
  UString m;
  try { Result = ProcessVirt(); }
  catch(const wchar_t *s) { m = s; }
  catch(const UString &s) { m = s; }
  catch(const char *s) { m = GetUnicodeString(s); }
  catch(int v)
  {
    wchar_t s[16];
    ConvertUInt32ToString(v, s);
    m = L"Error #";
    m += s;
  }
  catch(...) { m = L"Error"; }
  if (Result != E_ABORT)
  {
    if (m.IsEmpty() && Result != S_OK)
      m = HResultToMessage(Result);
  }
  AddMessageToString(m, FinalMessage.ErrorMessage.Message);

  {
    FOR_VECTOR(i, ErrorPaths)
    {
      if (i >= 32)
        break;
      AddMessageToString(m, fs2us(ErrorPaths[i]));
    }
  }

  CProgressSync &sync = ProgressDialog.Sync;
//   NSynchronization::CCriticalSectionLock lock(sync._cs);
  if (m.IsEmpty())
  {
    if (!FinalMessage.OkMessage.Message.IsEmpty())
      sync.FinalMessage.OkMessage = FinalMessage.OkMessage;
  }
  else
  {
    sync.FinalMessage.ErrorMessage.Message = m;
    if (Result == S_OK)
      Result = E_FAIL;
  }
}


LRESULT CProgressDialog::OnNcHitTest(CPoint point)
{
  return HTCAPTION;// CDialogEx::OnNcHitTest(point);
}


void CProgressDialog::SetBindWindow()
{
  CWnd *pBindWindow = CWnd::FromHandlePermanent(MainWindow);
  if (!pBindWindow)
    pBindWindow = GetParent();
  if (!pBindWindow)
    pBindWindow = this;

  m_hWndBind = *pBindWindow;
  CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_ITaskbarList3, (void**)&m_pTaskbarList);
  if (m_pTaskbarList)
    m_pTaskbarList->HrInit();
  SetProgressPos(_progressBar_Pos);
}


void CProgressDialog::RedrawClient()
{
  CRect rt, rt1, rt2, rt3;
  CRgn rgn, rgn1, rgn2, rgn3;
  GetClientRect(&rt);
  rgn.CreateRectRgn(0, 0, rt.Width(), rt.Height());
  m_btnBackground.GetWindowRect(&rt1); ScreenToClient(&rt1);
  m_btnPause.GetWindowRect(&rt2); ScreenToClient(&rt2);
  m_btnCancel.GetWindowRect(&rt3); ScreenToClient(&rt3);
  rgn1.CreateRectRgn(rt1.left, rt1.top, rt1.right, rt1.bottom);
  rgn2.CreateRectRgn(rt2.left, rt2.top, rt2.right, rt2.bottom);
  rgn3.CreateRectRgn(rt3.left, rt3.top, rt3.right, rt3.bottom);
  rgn.CombineRgn(&rgn, &rgn1, RGN_XOR);
  rgn.CombineRgn(&rgn, &rgn2, RGN_XOR);
  rgn.CombineRgn(&rgn, &rgn3, RGN_XOR);
  InvalidateRgn(&rgn, FALSE);

  m_btnBackground.Invalidate();
  m_btnPause.Invalidate();
  m_btnCancel.Invalidate();
}
