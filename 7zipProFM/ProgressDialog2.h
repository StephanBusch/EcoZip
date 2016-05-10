// ProgressDialog2.h

#pragma once

#include <Pdh.h>
#include <list>

#include "BtnST.h"
#include "CircularProgressCtrl.h"

#include "Synchronization.h"

#include "resource.h"

struct CProgressMessageBoxPair
{
  UString Title;
  UString Message;
};

struct CProgressFinalMessage
{
  CProgressMessageBoxPair ErrorMessage;
  CProgressMessageBoxPair OkMessage;

  bool ThereIsMessage() const { return !ErrorMessage.Message.IsEmpty() || !OkMessage.Message.IsEmpty(); }
};

class CProgressSync
{
  bool _stopped;
  bool _paused;

public:
  bool _bytesProgressMode;
  UInt64 _totalBytes;
  UInt64 _completedBytes;
  UInt64 _totalFiles;
  UInt64 _curFiles;
  UInt64 _inSize;
  UInt64 _outSize;
  
  UString _titleFileName;
  UString _status;
  UString _filePath;
  bool _isDir;

  UStringVector Messages;
  CProgressFinalMessage FinalMessage;

  CCriticalSection _cs;

  CProgressSync();

  bool Get_Stopped()
  {
    CSingleLock lock(&_cs, TRUE);
    return _stopped;
  }
  void Set_Stopped(bool val)
  {
    CSingleLock lock(&_cs, TRUE);
    _stopped = val;
  }
  
  bool Get_Paused();
  void Set_Paused(bool val)
  {
    CSingleLock lock(&_cs, TRUE);
    _paused = val;
  }
  
  void Set_BytesProgressMode(bool bytesProgressMode)
  {
    CSingleLock lock(&_cs, TRUE);
    _bytesProgressMode = bytesProgressMode;
  }
  
  HRESULT CheckStop();
  HRESULT ScanProgress(UInt64 numFiles, UInt64 totalSize, const FString &fileName, bool isDir = false);

  HRESULT Set_NumFilesTotal(UInt64 val);
  void Set_NumBytesTotal(UInt64 val);
  void Set_NumFilesCur(UInt64 val);
  HRESULT Set_NumBytesCur(const UInt64 *val);
  HRESULT Set_NumBytesCur(UInt64 val);
  void Set_Ratio(const UInt64 *inSize, const UInt64 *outSize);

  void Set_TitleFileName(const UString &fileName);
  void Set_Status(const UString &s);
  HRESULT Set_Status2(const UString &s, const wchar_t *path, bool isDir = false);
  void Set_FilePath(const wchar_t *path, bool isDir = false);

  void AddError_Message(const wchar_t *message);
  void AddError_Message_Name(const wchar_t *message, const wchar_t *name);
  void AddError_Code_Name(DWORD systemError, const wchar_t *name);

  bool ThereIsMessage() const { return !Messages.IsEmpty() || FinalMessage.ThereIsMessage(); }
};

// CProgressDialog dialog

class CProgressDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CProgressDialog)

public:
	CProgressDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CProgressDialog();

// Dialog Data
	enum { IDD = IDD_PROGRESS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	virtual BOOL OnInitDialog();

	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg LRESULT OnExternalCloseMessage(WPARAM wParam, LPARAM lParam);
	afx_msg void OnCancel();
	afx_msg void OnBnClickedPause();
	afx_msg void OnBnClickedProgressBackground();
  afx_msg LRESULT OnNcHitTest(CPoint point);
  DECLARE_MESSAGE_MAP()

  void DrawContents(CDC *pDC);
  void DrawPerformance(CDC *pDC);
  void DrawTime(CDC *pDC);
  void DrawSystemStatus(CDC *pDC);
  void DrawCurrentLog(CDC *pDC);
  void DrawArrow(CDC *pDC, int centerX, int centerY, int radius, HBITMAP hbmArrow, double degrees);
  void DrawArrow(CDC *pDC, int centerX, int centerY, int radius, HBITMAP hbmArrow, double from, double to, double range, double pos);

protected:
  HANDLE m_hFont;
  LOGFONT m_lg;
  HICON m_hIcon;
	CBitmap m_bmpBkgnd;
  CBitmap m_bmpArrow;
  CBitmap m_bmpPerfBkgnd;
  CBitmap m_bmpPerfMemo;
  CBitmap m_bmpTimeBkgnd;
  CBitmap m_bmpTimeFrgnd;
  CBitmap m_bmpStatusBkgnd;
  CBitmap m_bmpStatusHr;
  CBitmap m_bmpLogBkgnd;
	CBitmap m_bmpMem;

	CButtonST m_btnBackground;
	CButtonST m_btnPause;
	CButtonST m_btnCancel;

  void SetBindWindow();
  void SetState(TBPFLAG tbpFlags);
  void RedrawClient();
  HWND m_hWndBind;
  CComPtr<ITaskbarList3> m_pTaskbarList;
  TBPFLAG m_tbpFlags;
  BOOL m_bTimer;
  UInt64 m_nStep;
  UInt64 m_nRange;
  UInt64 m_nCurProgress;

  PDH_HQUERY cpuQuery;
  PDH_HCOUNTER cpuTotal;
  std::list<int> _usageHistory;

private:
  UString _titleFileName;
  UString _filePath;
  UString _status;
  bool _isDir;

  UString _background_String;
  UString _backgrounded_String;
  UString _foreground_String;
  UString _pause_String;
  UString _continue_String;
  UString _paused_String;

  UINT_PTR _timer;

  UString _title;

  UString _messages;

  class CU64ToI32Converter
  {
    unsigned _numShiftBits;
    UInt64 _range;
  public:
    CU64ToI32Converter(): _numShiftBits(0), _range(1) {}
    void Init(UInt64 range)
    {
      _range = range;
      // Windows CE doesn't like big number for ProgressBar.
      for (_numShiftBits = 0; range >= ((UInt32)1 << 15); _numShiftBits++)
        range >>= 1;
    }
    int Count(UInt64 val)
    {
      int res = (int)(val >> _numShiftBits);
      if (val == _range)
        res++;
      return res;
    }
  };
  
  CU64ToI32Converter _progressConv;
  UInt64 _progressBar_Pos;
  UInt64 _progressBar_Range;
  
  int _numMessages;

  UInt32 _prevTime;
  UInt64 _elapsedTime;

  UInt64 _prevPercentValue;
  UInt64 _prevElapsedSec;
  UInt64 _prevRemainingSec;

  UInt64 _totalBytes_Prev;
  UInt64 _processed_Prev;
  UInt64 _packed_Prev;
  UInt64 _ratio_Prev;
  UString _filesStr_Prev;

  unsigned _prevSpeed_MoveBits;
  UInt64 _prevSpeed;

  bool _foreground;

  unsigned _numReduceSymbols;

  bool _wasCreated;
  bool _needClose;

  unsigned _numPostedMessages;
  UInt32 _numAutoSizeMessages;

  bool _errorsWereDisplayed;

  bool _waitCloseByCancelButton;
  bool _cancelWasPressed;
  
  bool _inCancelMessageBox;
  bool _externalCloseMessageWasReceived;

  void SetTaskbarProgressState();

  void UpdateStatInfo(bool showAll);
  void SetProgressRange(UInt64 range);
  void SetProgressPos(UInt64 pos);

  CManualResetEvent _createDialogEvent;
  CManualResetEvent _dialogCreatedEvent;
  #ifndef _SFX
  void AddToTitle(LPCWSTR string);
  #endif

  void SetPauseText();
  void SetPriorityText();

  void SetTitleText();
  void ShowSize(int id, UInt64 val, UInt64 &prev);

  void UpdateMessagesDialog();

  void AddMessage(LPCWSTR message);

  void EnableErrorsControls(bool enable);

  void CheckNeedClose();
public:
  CProgressSync Sync;
  bool CompressingMode;
  bool WaitMode;
  bool ShowCompressionInfo;
  bool MessagesDisplayed; // = true if user pressed OK on all messages or there are no messages.
  bool showProgress;

  HWND MainWindow;
  #ifndef _SFX
  UString MainTitle;
  UString MainAddTitle;
  #endif

  void WaitCreating()
  {
    _createDialogEvent.Set();
    _dialogCreatedEvent.Lock();
  }

  INT_PTR Create(const UString &title, HANDLE hThread, HWND wndParent = 0);

  void ProcessWasFinished();
};


class CProgressCloser
{
  CProgressDialog *_p;
public:
  CProgressCloser(CProgressDialog &p) : _p(&p) {}
  ~CProgressCloser() { _p->ProcessWasFinished(); }
};

class CProgressThreadVirt
{
protected:
  FStringVector ErrorPaths;
  CProgressFinalMessage FinalMessage;

  // error if any of HRESULT, ErrorMessage, ErrorPath
  virtual HRESULT ProcessVirt() = 0;
  void Process();
public:
  HRESULT Result;
  bool ThreadFinishedOK; // if there is no fatal exception
  CProgressDialog ProgressDialog;

  static THREAD_FUNC_DECL MyThreadFunction(void *param)
  {
    CProgressThreadVirt *p = (CProgressThreadVirt *)param;
    try
    {
      p->Process();
      p->ThreadFinishedOK = true;
    }
    catch (...) { p->Result = E_FAIL; }
    return 0;
  }

  void AddErrorPath(const FString &path) { ErrorPaths.Add(path); }

  HRESULT Create(const UString &title, HWND parentWindow = 0, bool showProgress = true);
  CProgressThreadVirt(): Result(E_FAIL), ThreadFinishedOK(false) {}

  CProgressMessageBoxPair &GetMessagePair(bool isError) { return isError ? FinalMessage.ErrorMessage : FinalMessage.OkMessage; }

};

UString HResultToMessage(HRESULT errorCode);

