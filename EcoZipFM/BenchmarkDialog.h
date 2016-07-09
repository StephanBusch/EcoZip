#pragma once

#include "Synchronization.h"

#include "Bench.h"

#include "resource.h"

struct CBenchInfo2 : public CBenchInfo
{
  void Init()  { GlobalTime = UserTime = 0; }

  UInt64 GetCompressRating(UInt32 dictSize) const
  {
    return ::GetCompressRating(dictSize, GlobalTime, GlobalFreq, UnpackSize * NumIterations);
  }

  UInt64 GetDecompressRating() const
  {
    return ::GetDecompressRating(GlobalTime, GlobalFreq, UnpackSize, PackSize, NumIterations);
  }
};

class CProgressSyncInfo
{
public:
  bool Stopped;
  bool Paused;
  bool Changed;
  UInt32 DictionarySize;
  UInt32 NumThreads;
  UInt64 NumPasses;
  CManualResetEvent _startEvent;
  CCriticalSection CS;

  CBenchInfo2 CompressingInfoTemp;
  CBenchInfo2 CompressingInfo;
  UInt64 ProcessedSize;

  CBenchInfo2 DecompressingInfoTemp;
  CBenchInfo2 DecompressingInfo;

  AString Text;
  bool TextWasChanged;

  // UString Freq;
  // bool FreqWasChanged;

  CProgressSyncInfo()
  {
  }

  void Init()
  {
    Changed = false;
    Stopped = false;
    Paused = false;
    CompressingInfoTemp.Init();
    CompressingInfo.Init();
    ProcessedSize = 0;
    
    DecompressingInfoTemp.Init();
    DecompressingInfo.Init();

    NumPasses = 0;

    // Freq.SetFromAscii("MHz: ");
    // FreqWasChanged = true;

    Text.Empty();
    TextWasChanged = true;
  }

  void Stop()
  {
    CSingleLock lock(&CS, TRUE);
    Stopped = true;
  }
  bool WasStopped()
  {
    CSingleLock lock(&CS, TRUE);
    return Stopped;
  }
  void Pause()
  {
    CSingleLock lock(&CS, TRUE);
    Paused = true;
  }
  void Start()
  {
    CSingleLock lock(&CS, TRUE);
    Paused = false;
  }
  bool WasPaused()
  {
    CSingleLock lock(&CS, TRUE);
    return Paused;
  }
  void WaitCreating() { _startEvent.Lock(); }
};

struct CMyFont
{
  HFONT _font;
  CMyFont(): _font(NULL) {}
  ~CMyFont()
  {
    if (_font)
      DeleteObject(_font);
  }
  void Create(const LOGFONT *lplf)
  {
    _font = CreateFontIndirect(lplf);
  }
};


// CBenchmarkDialog dialog

class CBenchmarkDialog : public CDialogEx
{
  DECLARE_DYNAMIC(CBenchmarkDialog)

public:
  CBenchmarkDialog(CWnd* pParent = NULL);   // standard constructor
  virtual ~CBenchmarkDialog();

// Dialog Data
  enum { IDD = IDD_BENCH };

protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

  DECLARE_MESSAGE_MAP()
  virtual BOOL OnInitDialog();
  afx_msg void OnDestroy();
  virtual void OnHelp();
  afx_msg void OnTimer(UINT_PTR nIDEvent);
  afx_msg void OnNMCustomdrawBench(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnSize(UINT nType, int cx, int cy);

protected:
  CListCtrl m_lstBench;
  CBitmap m_bmpLevel1;
  CBitmap m_bmpLevel2;
  UInt64 m_nEncMax;
  UInt64 m_nDecMax;
  CIntVector m_encodings;
  CIntVector m_decodings;
  CString m_strSystemInfo;
  CString m_strModules;
  CString m_strTotalComp;
  CString m_strTotalDecomp;

protected:
  UINT_PTR _timer;
  UInt32 _startTime;

  void PrintTime();
  void PrintRating(UInt64 rating, UINT controlID);
  void PrintUsage(UInt64 usage, UINT controlID);
  void PrintResults(
    UInt32 dictionarySize,
    const CBenchInfo2 &info, UINT usageID, UINT speedID, UINT rpuID, UINT ratingID,
    bool decompressMode = false);

  void OnChangeSettings();
  void AdjustWidth();

  void DrawLevel(CDC *pDC, LPRECT lprt, UInt64 nLevel, BOOL enc);
public:
  CProgressSyncInfo Sync;
  void AppendBenchInfo(LPCWSTR methodName, BOOL finalize = FALSE);
  void UpdateLastBenchInfo();

  void MessageBoxError(LPCWSTR message)
  {
    MessageBox(message, _T("EcoZip"), MB_ICONERROR);
  }
  CString m_strAverage;
};

HRESULT Benchmark(
    DECL_EXTERNAL_CODECS_LOC_VARS
    const CObjectVector<CProperty> props);
