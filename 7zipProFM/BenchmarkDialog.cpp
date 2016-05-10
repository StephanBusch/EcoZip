// BenchmarkDialog.cpp : implementation file
//

#include "stdafx.h"

#include "C/CpuArch.h"

#include "CPP/Common/Defs.h"
#include "CPP/Common/IntToString.h"
#include "CPP/Common/MyException.h"
#include "CPP/Common/StringConvert.h"
#include "CPP/Common/StringToInt.h"

#include "CPP/Windows/System.h"

#include "CPP/7zip/Common/MethodProps.h"

#include "CPP/7zip/UI/FileManager/HelpUtils.h"

#include "CPP/7zip/MyVersion.h"

#include "7zipProFM.h"
#include "BenchmarkDialog.h"
#include "FMUtils.h"
#include "ImageUtils.h"


static LPCWSTR kHelpTopic = L"fm/benchmark.htm";

static const UINT_PTR kTimerID = 4;
static const UINT kTimerElapse = 1000;

#ifdef LANG
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#endif

using namespace NWindows;

void GetCpuName(AString &s);

UString HResultToMessage(HRESULT errorCode);

#ifdef LANG
static const UInt32 kLangIDs[] =
{
  IDT_BENCH_TOTAL,
  IDX_BENCH_MULTITHREAD,
};

static const UInt32 kLangIDs_Colon[] =
{
  IDT_BENCH_SYSTEM
};

#endif

static const LPCTSTR kProcessingString = TEXT("...");
static const LPCTSTR kMB = TEXT(" MB");
static const LPCTSTR kMIPS = TEXT(" MIPS");
static const LPCTSTR kKBs = TEXT(" KB/s");

#ifdef UNDER_CE
static const int kMinDicLogSize = 20;
#else
static const int kMinDicLogSize = 21;
#endif
static const UInt32 kMinDicSize = (1 << kMinDicLogSize);
static const UInt32 kMaxDicSize =
    #ifdef MY_CPU_64BIT
    (1 << 30);
    #else
    (1 << 27);
    #endif


// CBenchmarkDialog dialog

IMPLEMENT_DYNAMIC(CBenchmarkDialog, CDialogEx)

CBenchmarkDialog::CBenchmarkDialog(CWnd* pParent /*=NULL*/)
  : CDialogEx(CBenchmarkDialog::IDD, pParent)
  , _timer(0)
  , m_strAverage(_T(""))
  , m_strSystemInfo(_T(""))
  , m_strModules(_T(""))
  , m_strTotalComp(_T(""))
  , m_strTotalDecomp(_T(""))
{

}

CBenchmarkDialog::~CBenchmarkDialog()
{
}

void CBenchmarkDialog::DoDataExchange(CDataExchange* pDX)
{
  CDialogEx::DoDataExchange(pDX);
  DDX_Control(pDX, IDL_BENCH, m_lstBench);
  DDX_Text(pDX, IDT_BENCH_SYSTEM_VAL, m_strSystemInfo);
  DDX_Text(pDX, IDE_BENCH_MODULES, m_strModules);
  DDX_Text(pDX, IDT_BENCH_TOTAL_COMP, m_strTotalComp);
  DDX_Text(pDX, IDT_BENCH_TOTAL_DECOMP, m_strTotalDecomp);
}


BEGIN_MESSAGE_MAP(CBenchmarkDialog, CDialogEx)
  ON_WM_DESTROY()
  ON_WM_TIMER()
  ON_NOTIFY(NM_CUSTOMDRAW, IDL_BENCH, &CBenchmarkDialog::OnNMCustomdrawBench)
  ON_WM_SIZE()
END_MESSAGE_MAP()


// CBenchmarkDialog message handlers


static FString GetProcessorName()
{
  int CPUInfo[4] = { -1 };
  char CPUBrandString[0x40];
  __cpuid(CPUInfo, 0x80000000);
  unsigned int nExIds = CPUInfo[0];

  memset(CPUBrandString, 0, sizeof(CPUBrandString));

  // Get the information associated with each extended ID.
  for (unsigned i = 0x80000000; i <= nExIds; ++i)
  {
    __cpuid(CPUInfo, i);
    // Interpret CPU brand string.
    if (i == 0x80000002)
      memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
    else if (i == 0x80000003)
      memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
    else if (i == 0x80000004)
      memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
  }
  return GetSystemString(CPUBrandString);
}


#pragma pack(push) 
#pragma pack(1)
typedef struct _RawSMBIOSData
{
  BYTE	Used20CallingMethod;
  BYTE	SMBIOSMajorVersion;
  BYTE	SMBIOSMinorVersion;
  BYTE	DmiRevision;
  DWORD	Length;
  PBYTE	SMBIOSTableData;
} RawSMBIOSData, *PRawSMBIOSData;

typedef struct _SMBIOSHEADER_
{
  BYTE Type;
  BYTE Length;
  WORD Handle;
} SMBIOSHEADER, *PSMBIOSHEADER;

typedef struct _TYPE_17_ {
  SMBIOSHEADER Header;
  UINT16	PhysicalArrayHandle;
  UINT16	ErrorInformationHandle;
  UINT16	TotalWidth;
  UINT16	DataWidth;
  UINT16	Size;
  UCHAR	FormFactor;
  UCHAR	DeviceSet;
  UCHAR	DeviceLocator;
  UCHAR	BankLocator;
  UCHAR	MemoryType;
  UINT16	TypeDetail;
  UINT16	Speed;
  UCHAR	Manufacturer;
  UCHAR	SN;
  UCHAR	AssetTag;
  UCHAR	PN;
  UCHAR	Attributes;
} MemoryDevice, *PMemoryDevice;
#pragma pack(push) 

#ifdef UNICODE
#define getHeaderString  getHeaderStringW
#define LocateString	LocateStringW
#else
#define getHeaderString  getHeaderStringA
#define LocateString	LocateStringA
#endif

static const char* LocateStringA(const char* str, UINT i)
{
  static const char strNull[] = "Null String";

  if (0 == i || 0 == *str) return strNull;

  while (--i)
  {
    str += strlen((char*)str) + 1;
  }
  return str;
}

static const wchar_t* LocateStringW(const char* str, UINT i)
{
  static wchar_t buff[2048];
  const char *pStr = LocateStringA(str, i);
  SecureZeroMemory(buff, sizeof(buff));
  MultiByteToWideChar(CP_OEMCP, 0, pStr, (int)strlen(pStr), buff, sizeof(buff));
  return buff;
}

static const char* toPointString(void* p)
{
  return (char*)p + ((PSMBIOSHEADER)p)->Length;
}

static FString GetRamType()
{
  FString strRamType;
  Byte *pBuffer = NULL;
  UINT retVal = GetSystemFirmwareTable('RSMB', 'PCAF', NULL, 0);
  pBuffer = (Byte *)malloc(retVal);
  retVal = GetSystemFirmwareTable('RSMB', 'PCAF', pBuffer, retVal);
  unsigned pos = sizeof(RawSMBIOSData);
  while (pos < retVal) {
    SMBIOSHEADER *hdr = (SMBIOSHEADER *)(pBuffer + pos);
    unsigned end = hdr->Length;
    while (pos + end < retVal) {
      if (*(WORD *)(pBuffer + pos + end) == 0)
        break;
      end++;
    }
    if (pos + end >= retVal || *(WORD *)(pBuffer + pos + end) != 0)
      break;
    if (hdr->Type == 17) {
      PMemoryDevice pMD = (PMemoryDevice)(pBuffer + pos);
      const char *str = toPointString(pBuffer + pos);
      strRamType = LocateString(str, pMD->DeviceLocator);
      if (!strRamType.IsEmpty()) {
        TCHAR chLast = strRamType.Ptr()[strRamType.Len() - 1];
        if (chLast >= _T('0') && chLast <= _T('9'))
          strRamType.DeleteBack();
      }
      return strRamType;
    }
    pos += end + sizeof(WORD);
  }
  free(pBuffer);
  return strRamType;
}

static FString GetRamSizeString()
{
  TCHAR szRam[32];
  UInt64 numRam = NSystem::GetRamSize();
  numRam = (numRam + 1023) / 1024;
  if (numRam > 1024) {
    numRam = (numRam + 1023) / 1024;
    if (numRam > 1024) {
      numRam = (numRam + 1023) / 1024;
      ConvertUInt64ToString(numRam, szRam);
      lstrcat(szRam, _T(" GB"));
    }
    else {
      ConvertUInt64ToString(numRam, szRam);
      lstrcat(szRam, _T(" MB"));
    }
  }
  else {
    ConvertUInt64ToString(numRam, szRam);
    lstrcat(szRam, _T(" KB"));
  }
  return szRam;
}

BOOL CBenchmarkDialog::OnInitDialog()
{
  CDialogEx::OnInitDialog();

  #ifdef LANG
  LangSetWindowText(*this, IDD_BENCH);
  LangSetDlgItems(*this, kLangIDs, ARRAY_SIZE(kLangIDs));
  LangSetDlgItems_Colon(*this, kLangIDs_Colon, ARRAY_SIZE(kLangIDs_Colon));
  #endif

  const int count = 5;
  const wchar_t *names[] = {
    _T("Name"),
    _T("Compression"),
    _T("Unit"),
    _T("Decompression"),
    _T("Unit"),
  };
  const int width[] = { 150, 200, 70, 200, 70 };
  for (int i = 0; i < count; i++) {
    m_lstBench.InsertColumn(i, names[i], LVCFMT_LEFT, -1, -1);
    m_lstBench.SetColumnWidth(i, width[i]);
  }
//   m_lstBench.InsertColumn(1, _T("Compression"), LVCFMT_LEFT, -1, 1);
//   m_lstBench.InsertColumn(2, _T("Unit"), LVCFMT_LEFT, -1, 2);
//   m_lstBench.InsertColumn(3, _T("Decompression"), LVCFMT_LEFT, -1, 3);
//   m_lstBench.InsertColumn(4, _T("Unit"), LVCFMT_LEFT, -1, 4);
  AdjustWidth();
  m_lstBench.SetExtendedStyle(m_lstBench.GetExtendedStyle() | LVS_EX_FULLROWSELECT);

  CImageUtils::LoadBitmapFromPNG(IDB_LEVEL1, m_bmpLevel1);
  CImageUtils::LoadBitmapFromPNG(IDB_LEVEL2, m_bmpLevel2);
  CDC *pDC = GetDC();
// 	CImageUtils::PremultiplyBitmapAlpha(pDC->m_hDC, m_bmpLevel1);
// 	CImageUtils::PremultiplyBitmapAlpha(pDC->m_hDC, m_bmpLevel2);
  ReleaseDC(pDC);

  m_nEncMax = 0;
  m_nDecMax = 0;

  Sync.Init();

  /************************************************************************/
  /* Get System Info                                                      */
  /************************************************************************/
  m_strSystemInfo = GetProcessorName();


  {
    TCHAR s[40];
    s[0] = '/';
    s[1] = ' ';
    ConvertUInt32ToString(NSystem::GetNumberOfProcessors(), s + 2);
    //SetDlgItemText(IDT_BENCH_HARDWARE_THREADS, s);
  }

  {
    UString s;
    {
      AString cpuName;
      GetCpuName(cpuName);
      s.SetFromAscii(cpuName);
      //SetDlgItemText(IDT_BENCH_CPU, s);
    }

    s.SetFromAscii("7-ZipPro " MY_VERSION " ["
        #ifdef MY_CPU_64BIT
          "64-bit"
        #elif defined MY_CPU_32BIT
          "32-bit"
        #endif
        "]");
    //SetDlgItemText(IDT_BENCH_VER, s);
  }

  UInt32 numCPUs = NSystem::GetNumberOfProcessors();
  if (numCPUs < 1)
    numCPUs = 1;
  numCPUs = MyMin(numCPUs, (UInt32)(1 << 8));

  TCHAR szCPUs[8];
  ConvertUInt32ToString(numCPUs, szCPUs);

  m_strSystemInfo += _T(" (");
  m_strSystemInfo += szCPUs;
  m_strSystemInfo += _T(" CPUs)");

  m_strSystemInfo += _T(", ");
  m_strSystemInfo += GetRamSizeString();
  m_strSystemInfo += _T(' ');
  m_strSystemInfo += GetRamType();

  if (Sync.NumThreads == (UInt32)(Int32)-1)
  {
    Sync.NumThreads = numCPUs;
    if (Sync.NumThreads > 1)
      Sync.NumThreads &= ~1;
  }
// 	m_NumThreads.Attach(GetItem(IDC_BENCH_NUM_THREADS));
// 	int cur = 0;
// 	for (UInt32 num = 1; num <= numCPUs * 2;)
// 	{
// 		TCHAR s[16];
// 		ConvertUInt32ToString(num, s);
// 		int index = (int)m_NumThreads.AddString(s);
// 		m_NumThreads.SetItemData(index, num);
// 		if (num <= Sync.NumThreads)
// 			cur = index;
// 		if (num > 1)
// 			num++;
// 		num++;
// 	}
// 	m_NumThreads.SetCurSel(cur);
// 	Sync.NumThreads = GetNumberOfThreads();

// 	cur = 0;
  UInt64 ramSize = NSystem::GetRamSize();

#ifdef UNDER_CE
  const UInt32 kNormalizedCeSize = (16 << 20);
  if (ramSize > kNormalizedCeSize && ramSize < (33 << 20))
    ramSize = kNormalizedCeSize;
#endif

  if (Sync.DictionarySize == (UInt32)(Int32)-1)
  {
    int dicSizeLog;
    for (dicSizeLog = 25; dicSizeLog > kBenchMinDicLogSize; dicSizeLog--)
      if (GetBenchMemoryUsage(Sync.NumThreads, ((UInt32)1 << dicSizeLog)) + (8 << 20) <= ramSize)
        break;
    Sync.DictionarySize = (1 << dicSizeLog);
  }
  if (Sync.DictionarySize < kMinDicSize)
    Sync.DictionarySize = kMinDicSize;
  if (Sync.DictionarySize > kMaxDicSize)
    Sync.DictionarySize = kMaxDicSize;

// 	for (int i = kMinDicLogSize; i <= 30; i++)
// 		for (int j = 0; j < 2; j++)
// 		{
// 		UInt32 dictionary = (1 << i) + (j << (i - 1));
// 		if (dictionary > kMaxDicSize)
// 			continue;
// 		TCHAR s[16];
// 		ConvertUInt32ToString((dictionary >> 20), s);
// 		lstrcat(s, kMB);
// 		int index = (int)m_Dictionary.AddString(s);
// 		m_Dictionary.SetItemData(index, dictionary);
// 		if (dictionary <= Sync.DictionarySize)
// 			cur = index;
// 		}
// 	m_Dictionary.SetCurSel(cur);

  OnChangeSettings();

  Sync._startEvent.Set();
  _timer = SetTimer(kTimerID, kTimerElapse, NULL);

  UpdateData(FALSE);

  return TRUE;  // return TRUE unless you set the focus to a control
  // EXCEPTION: OCX Property Pages should return FALSE
}


static const UInt32 g_IDs[] =
{
  IDT_BENCH_SYSTEM_VAL,
  IDE_BENCH_MODULES,
  IDT_BENCH_TOTAL_COMP,
  IDT_BENCH_TOTAL_DECOMP,
};

void CBenchmarkDialog::OnChangeSettings()
{
// 	EnableItem(IDB_STOP, true);
// 	UInt32 dictionary = OnChangeDictionary();
  for (int i = 0; i < ARRAY_SIZE(g_IDs); i++)
    SetDlgItemText(g_IDs[i], kProcessingString);
  _startTime = GetTickCount();
  PrintTime();
  CSingleLock lock(&Sync.CS, TRUE);
  Sync.Init();
// 	Sync.DictionarySize = dictionary;
// 	Sync.Changed = true;
// 	Sync.NumThreads = GetNumberOfThreads();
}


void CBenchmarkDialog::OnHelp()
{
  ShowHelpWindow(NULL, kHelpTopic);
  CDialogEx::OnHelp();
}


void CBenchmarkDialog::OnDestroy()
{
  CDialogEx::OnDestroy();

  Sync.Stop();
  KillTimer(_timer);
}


void CBenchmarkDialog::PrintTime()
{
  UInt32 curTime = ::GetTickCount();
  UInt32 elapsedTime = (curTime - _startTime);
  UInt32 elapsedSec = elapsedTime / 1000;
  if (elapsedSec != 0 && Sync.WasPaused())
    return;
  WCHAR s[40];
  GetTimeString(elapsedSec, s);
// 	SetDlgItemText(IDT_BENCH_ELAPSED_VAL, s);
}


void CBenchmarkDialog::PrintRating(UInt64 rating, UINT controlID)
{
  TCHAR s[40];
  ConvertUInt64ToString(rating / 1000000, s);
  lstrcat(s, kMIPS);
  SetDlgItemText(controlID, s);
}


void CBenchmarkDialog::PrintUsage(UInt64 usage, UINT controlID)
{
  TCHAR s[40];
  ConvertUInt64ToString((usage + 5000) / 10000, s);
  lstrcat(s, TEXT("%"));
  SetDlgItemText(controlID, s);
}


void CBenchmarkDialog::PrintResults(
  UInt32 dictionarySize,
  const CBenchInfo2 &info,
  UINT usageID, UINT speedID, UINT rpuID, UINT ratingID,
  bool decompressMode)
{
  if (info.GlobalTime == 0)
    return;

  TCHAR s[40];
  {
    UInt64 speed = info.UnpackSize * info.NumIterations * info.GlobalFreq / info.GlobalTime;
    ConvertUInt64ToString(speed / 1024, s);
    lstrcat(s, kKBs);
    SetDlgItemText(speedID, s);
  }
  UInt64 rating;
  if (decompressMode)
    rating = info.GetDecompressRating();
  else
    rating = info.GetCompressRating(dictionarySize);

  PrintRating(rating, ratingID);
  PrintRating(info.GetRatingPerUsage(rating), rpuID);
  PrintUsage(info.GetUsage(), usageID);
}


void CBenchmarkDialog::OnTimer(UINT_PTR nIDEvent)
{
  // Update changed info
  UpdateData(FALSE);

  bool printTime = true;
  if (Sync.WasStopped())
    printTime = false;

  if (printTime)
    PrintTime();
  CSingleLock lock(&Sync.CS, TRUE);

// 	if (TotalMode)
// 	{
    if (Sync.TextWasChanged)
    {
// 			_consoleEdit.SetText(GetSystemString(Sync.Text));
      Sync.TextWasChanged = false;
    }
    return;
// 	}

  TCHAR s[40];
  ConvertUInt64ToString((Sync.ProcessedSize >> 20), s);
  lstrcat(s, kMB);
// 	SetItemText(IDT_BENCH_SIZE_VAL, s);

  ConvertUInt64ToString(Sync.NumPasses, s);
// 	SetItemText(IDT_BENCH_PASSES_VAL, s);

  /*
  if (Sync.FreqWasChanged)
  {
    SetItemText(IDT_BENCH_FREQ, Sync.Freq);
    Sync.FreqWasChanged  = false;
  }
  */

  {
// 		UInt32 dicSizeTemp = (UInt32)MyMax(Sync.ProcessedSize, UInt64(1) << 20);
// 		dicSizeTemp = MyMin(dicSizeTemp, Sync.DictionarySize),
// 			PrintResults(dicSizeTemp,
// 			Sync.CompressingInfoTemp,
// 			IDT_BENCH_COMPRESS_USAGE1,
// 			IDT_BENCH_COMPRESS_SPEED1,
// 			IDT_BENCH_COMPRESS_RPU1,
// 			IDT_BENCH_COMPRESS_RATING1);
  }

  {
// 		PrintResults(
// 			Sync.DictionarySize,
// 			Sync.CompressingInfo,
// 			IDT_BENCH_COMPRESS_USAGE2,
// 			IDT_BENCH_COMPRESS_SPEED2,
// 			IDT_BENCH_COMPRESS_RPU2,
// 			IDT_BENCH_COMPRESS_RATING2);
  }

// 	{
// 		PrintResults(
// 			Sync.DictionarySize,
// 			Sync.DecompressingInfoTemp,
// 			IDT_BENCH_DECOMPR_USAGE1,
// 			IDT_BENCH_DECOMPR_SPEED1,
// 			IDT_BENCH_DECOMPR_RPU1,
// 			IDT_BENCH_DECOMPR_RATING1,
// 			true);
// 	}
// 	{
// 		PrintResults(
// 			Sync.DictionarySize,
// 			Sync.DecompressingInfo,
// 			IDT_BENCH_DECOMPR_USAGE2,
// 			IDT_BENCH_DECOMPR_SPEED2,
// 			IDT_BENCH_DECOMPR_RPU2,
// 			IDT_BENCH_DECOMPR_RATING2,
// 			true);
// 		if (Sync.DecompressingInfo.GlobalTime > 0 &&
// 			Sync.CompressingInfo.GlobalTime > 0)
// 		{
// 			UInt64 comprRating = Sync.CompressingInfo.GetCompressRating(Sync.DictionarySize);
// 			UInt64 decomprRating = Sync.DecompressingInfo.GetDecompressRating();
// 			PrintRating((comprRating + decomprRating) / 2, IDT_BENCH_TOTAL_RATING_VAL);
// 			PrintRating((
// 				Sync.CompressingInfo.GetRatingPerUsage(comprRating) +
// 				Sync.DecompressingInfo.GetRatingPerUsage(decomprRating)) / 2, IDT_BENCH_TOTAL_RPU_VAL);
// 			PrintUsage(
// 				(Sync.CompressingInfo.GetUsage() +
// 				Sync.DecompressingInfo.GetUsage()) / 2, IDT_BENCH_TOTAL_USAGE_VAL);
// 		}
// 	}

  CDialogEx::OnTimer(nIDEvent);
}

struct CThreadBenchmark
{
  CBenchmarkDialog *BenchmarkDialog;
  DECL_EXTERNAL_CODECS_LOC_VARS2;
  // UInt32 dictionarySize;
  // UInt32 numThreads;

  HRESULT Process();
  HRESULT Result;
  static THREAD_FUNC_DECL MyThreadFunction(void *param)
  {
    ((CThreadBenchmark *)param)->Result = ((CThreadBenchmark *)param)->Process();
    return 0;
  }
};

struct CBenchCallback: public IBenchCallback
{
  UInt32 dictionarySize;
  CProgressSyncInfo *Sync;
  
  // void AddCpuFreq(UInt64 cpuFreq);
  HRESULT SetFreq(bool showFreq, UInt64 cpuFreq);
  HRESULT SetEncodeResult(const CBenchInfo &info, bool final);
  HRESULT SetDecodeResult(const CBenchInfo &info, bool final);
};

/*
void CBenchCallback::AddCpuFreq(UInt64 cpuFreq)
{
  NSynchronization::CCriticalSectionLock lock(Sync->CS);
  {
    wchar_t s[32];
    ConvertUInt64ToString(cpuFreq, s);
    Sync->Freq.Add_Space_if_NotEmpty();
    Sync->Freq += s;
    Sync->FreqWasChanged = true;
  }
}
*/

HRESULT CBenchCallback::SetFreq(bool /* showFreq */, UInt64 /* cpuFreq */)
{
  return S_OK;
}

HRESULT CBenchCallback::SetEncodeResult(const CBenchInfo &info, bool final)
{
  CSingleLock lock(&Sync->CS, TRUE);
  if (Sync->Changed || Sync->Paused || Sync->Stopped)
    return E_ABORT;
  Sync->ProcessedSize = info.UnpackSize * info.NumIterations;
  if (final && Sync->CompressingInfo.GlobalTime == 0)
  {
    (CBenchInfo&)Sync->CompressingInfo = info;
    if (Sync->CompressingInfo.GlobalTime == 0)
      Sync->CompressingInfo.GlobalTime = 1;
  }
  else
    (CBenchInfo&)Sync->CompressingInfoTemp = info;

  return S_OK;
}

HRESULT CBenchCallback::SetDecodeResult(const CBenchInfo &info, bool final)
{
  CSingleLock lock(&Sync->CS, TRUE);
  if (Sync->Changed || Sync->Paused || Sync->Stopped)
    return E_ABORT;
  CBenchInfo info2 = info;
  if (final && Sync->DecompressingInfo.GlobalTime == 0)
  {
    info2.UnpackSize *= info2.NumIterations;
    info2.PackSize *= info2.NumIterations;
    info2.NumIterations = 1;
    (CBenchInfo&)Sync->DecompressingInfo = info2;
    if (Sync->DecompressingInfo.GlobalTime == 0)
      Sync->DecompressingInfo.GlobalTime = 1;
  }
  else
    (CBenchInfo&)Sync->DecompressingInfoTemp = info2;
  return S_OK;
}

struct CBenchCallback2: public IBenchPrintCallback
{
  CProgressSyncInfo *Sync;

  void Print(const char *s);
  void NewLine();
  HRESULT CheckBreak();
};

void CBenchCallback2::Print(const char *s)
{
  CSingleLock lock(&Sync->CS, TRUE);
  Sync->Text += s;
  Sync->TextWasChanged = true;
}

void CBenchCallback2::NewLine()
{
  Print("\xD\n");
}

HRESULT CBenchCallback2::CheckBreak()
{
  if (Sync->Changed || Sync->Paused || Sync->Stopped)
    return E_ABORT;
  return S_OK;
}


HRESULT CThreadBenchmark::Process()
{
  CProgressSyncInfo &sync = BenchmarkDialog->Sync;
  sync.WaitCreating();

  UInt32 numCPUs = 1;
  UInt64 ramSize = (UInt64)512 << 20;
#ifndef _7ZIP_ST
  numCPUs = NSystem::GetNumberOfProcessors();
#endif
#if !defined(_7ZIP_ST) || defined(_WIN32)
  ramSize = NSystem::GetRamSize();
#endif
  UInt32 numThreads = 1;// numCPUs;

  UInt32 testTime = (UInt32)kComplexInSeconds;

  UInt64 complexInCommands = kComplexInCommands;

  {
    UInt64 numMilCommands = (1 << 6);

    for (int jj = 0;; jj++)
    {
      UInt64 start = ::GetTimeCount();
      UInt32 sum = (UInt32)start;
      sum = CountCpuFreq(sum, (UInt32)(numMilCommands * 1000000 / kNumFreqCommands), g_BenchCpuFreqTemp);
      start = ::GetTimeCount() - start;
      if (start == 0)
        start = 1;
      UInt64 freq = GetFreq();
      UInt64 mipsVal = numMilCommands * freq / start;
      if (jj >= 3)
      {
        SetComplexCommands(testTime, mipsVal * 1000000, complexInCommands);
        if (jj >= 8 || start >= freq)
          break;
        // break; // change it
        numMilCommands <<= 1;
      }
    }
  }

  UInt32 dict;
  int dicSizeLog;
  for (dicSizeLog = 25; dicSizeLog > kBenchMinDicLogSize; dicSizeLog--)
    if (GetBenchMemoryUsage(numThreads, ((UInt32)1 << dicSizeLog)) + (8 << 20) <= ramSize)
      break;
  dict = (1 << dicSizeLog);

//   IBenchPrintCallback &f = *printCallback;
//   PrintRequirements(f, "usage:", GetBenchMemoryUsage(numThreads, dict), "Benchmark threads:   ", numThreads);

  bool totalBenchMode = true;
  bool showFreq = totalBenchMode;
  UInt64 cpuFreq = 0;

  dict =
#ifdef UNDER_CE
    (UInt64)1 << 20;
#else
    (UInt64)1 << 24;
#endif

  CBenchCallback2 printCallback;
  printCallback.Sync = &sync;

  try
  {
    int freqTest;
    const int kNumCpuTests = 3;
    for (freqTest = 0; freqTest < kNumCpuTests; freqTest++)
    {
      UInt32 resVal;
      RINOK(FreqBench(complexInCommands, numThreads, &printCallback, freqTest == kNumCpuTests - 1, cpuFreq, resVal));

      if (freqTest == kNumCpuTests - 1)
        SetComplexCommands(testTime, cpuFreq, complexInCommands);
    }

//     callback.SetFreq(true, cpuFreq);
//     res = TotalBench(EXTERNAL_CODECS_LOC_VARS complexInCommands, numThreads, dictIsDefined, dict, printCallback, &callback);
//     RINOK(res);
    bool forceUnpackSize = false;
    UInt32 unpackSize = dict;
    for (unsigned i = 0; ; i++)
    {
//       sync.Init();

      if (sync.WasStopped())
        return 0;
      if (sync.WasPaused())
      {
        Sleep(200);
        continue;
      }
      {
        CSingleLock lock(&sync.CS, TRUE);
        if (sync.Stopped || sync.Paused)
          continue;
        //if (sync.Changed)
        sync.Init();
        sync.NumThreads = numThreads;
      }

      CBenchCallback callback;
      callback.Sync = &sync;
      CBenchProps BenchProps;

      callback.SetFreq(true, cpuFreq);

      CBenchMethod bench = g_Bench[i];
      if (bench.Name == NULL)
        break;

      BenchmarkDialog->AppendBenchInfo(GetUnicodeString(bench.Name));

      BenchProps.DecComplexUnc = bench.DecComplexUnc;
      BenchProps.DecComplexCompr = bench.DecComplexCompr;
      BenchProps.EncComplex = bench.EncComplex;
      COneMethodInfo method;
      NCOM::CPropVariant propVariant;
      propVariant = bench.Name;
      RINOK(method.ParseMethodFromPROPVARIANT(L"", propVariant));

      UInt32 unpackSize2 = unpackSize;
      if (!forceUnpackSize && bench.DictBits == 0)
        unpackSize2 = kFilterUnpackSize;

      HRESULT res = MethodBench(
        EXTERNAL_CODECS_LOC_VARS
        complexInCommands,
        false, numThreads, method, unpackSize2, bench.DictBits,
        NULL, &callback, &BenchProps);
      if (res == E_NOTIMPL) {
        // callback->Print(" ---");
        // we need additional empty line as line for decompression results
        //if (!callback.Use2Columns)
        //  callback.NewLine();
        continue;
      }
      else {
        if (FAILED(res))
          continue;
        RINOK(res);
      }

      BenchmarkDialog->UpdateLastBenchInfo();
    }

//     res = TotalBench_Hash(EXTERNAL_CODECS_LOC_VARS complexInCommands, numThreads,
//       1 << kNumHashDictBits, printCallback, &callback, &callback.EncodeRes, true, cpuFreq);
//     RINOK(res);
//     for (unsigned i = 0; ; i++)
//     {
// //       sync.Init();
// 
//       if (sync.WasStopped())
//         return 0;
//       if (sync.WasPaused())
//       {
//         Sleep(200);
//         continue;
//       }
//       {
//         CSingleLock lock(&sync.CS, TRUE);
//         if (sync.Stopped || sync.Paused)
//           continue;
//         //if (sync.Changed)
//         sync.Init();
//         sync.NumThreads = numThreads;
//       }
// 
//       CBenchCallback callback;
//       callback.Sync = &sync;
//       CTotalBenchRes EncodeRes;
// 
//       callback.SetFreq(true, cpuFreq);
// 
//       const CBenchHash &bench = g_Hash[i];
//       if (bench.Name == NULL)
//         break;
// 
//       BenchmarkDialog->AppendBenchInfo(GetUnicodeString(bench.Name));
// 
//       // callback->BenchProps.DecComplexUnc = bench.DecComplexUnc;
//       // callback->BenchProps.DecComplexCompr = bench.DecComplexCompr;
//       // callback->BenchProps.EncComplex = bench.EncComplex;
// 
//       COneMethodInfo method;
//       NCOM::CPropVariant propVariant;
//       propVariant = bench.Name;
//       RINOK(method.ParseMethodFromPROPVARIANT(L"", propVariant));
// 
//       UInt32 bufSize = 1 << kNumHashDictBits;
// 
//       UInt64 speed;
//       HRESULT res = CrcBench(
//         EXTERNAL_CODECS_LOC_VARS
//         complexInCommands,
//         numThreads, bufSize,
//         speed,
//         bench.Complex, &bench.CheckSum, method,
//         NULL, &EncodeRes, showFreq, cpuFreq);
//       if (res == E_NOTIMPL) {
//         // callback->Print(" ---");
//       }
//       else {
//         RINOK(res);
//       }
// 
//       BenchmarkDialog->UpdateLastBenchInfo();
//     }

    {
      UInt32 resVal;
      UInt64 cpuFreqLastTemp = cpuFreq;
      RINOK(FreqBench(complexInCommands, numThreads, &printCallback, false, cpuFreqLastTemp, resVal));
    }

    return S_OK;
  }
  catch (CSystemException &e)
  {
    BenchmarkDialog->MessageBoxError(HResultToMessage(e.ErrorCode));
    return E_FAIL;
  }
  catch (...)
  {
    BenchmarkDialog->MessageBoxError(HResultToMessage(E_FAIL));
    return E_FAIL;
  }
}

static void ParseNumberString(const UString &s, NWindows::NCOM::CPropVariant &prop)
{
  const wchar_t *end;
  UInt64 result = ConvertStringToUInt64(s, &end);
  if (*end != 0 || s.IsEmpty())
    prop = s;
  else if (result <= (UInt32)0xFFFFFFFF)
    prop = (UInt32)result;
  else
    prop = result;
}

HRESULT Benchmark(
    DECL_EXTERNAL_CODECS_LOC_VARS
    const CObjectVector<CProperty> /*props*/)
{
  CThreadBenchmark benchmarker;
  #ifdef EXTERNAL_CODECS
  benchmarker.__externalCodecs = __externalCodecs;
  #endif

  CBenchmarkDialog bd;
  bd.Sync.DictionarySize = (UInt32)(Int32)-1;
  bd.Sync.NumThreads = (UInt32)(Int32)-1;

  COneMethodInfo method;

  UInt32 numCPUs = 1;
  #ifndef _7ZIP_ST
  numCPUs = NWindows::NSystem::GetNumberOfProcessors();
  #endif
  UInt32 numThreads = numCPUs;

//   FOR_VECTOR (i, props)
//   {
//     const CProperty &prop = props[i];
//     UString name = prop.Name;
//     name.MakeLower_Ascii();
//     if (name.IsEqualToNoCase(L"m") && prop.Value == L"*")
//     {
//       continue;
//     }
// 
//     NWindows::NCOM::CPropVariant propVariant;
//     if (!prop.Value.IsEmpty())
//       ParseNumberString(prop.Value, propVariant);
//     if (name.IsPrefixedBy(L"mt"))
//     {
//       #ifndef _7ZIP_ST
//       RINOK(ParseMtProp(name.Ptr(2), propVariant, numCPUs, numThreads));
//       if (numThreads != numCPUs)
//         bd.Sync.NumThreads = numThreads;
//       #endif
//       continue;
//     }
//     if (name.IsEqualTo("testtime"))
//     {
//       // UInt32 testTime = 4;
//       // RINOK(ParsePropToUInt32(L"", propVariant, testTime));
//       continue;
//     }
//     RINOK(method.ParseMethodFromPROPVARIANT(name, propVariant));
//   }

  // bool totalBenchMode = (method.MethodName == L"*");

  {
    UInt32 dict;
    if (method.Get_DicSize(dict))
      bd.Sync.DictionarySize = dict;
  }

  benchmarker.BenchmarkDialog = &bd;

  unsigned threadID;
  HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, &CThreadBenchmark::MyThreadFunction, &benchmarker, 0, &threadID);
  if (hThread == NULL)
    return E_FAIL;
  bd.DoModal();
  if (WaitForSingleObject(hThread, INFINITE) == WAIT_FAILED)
    return E_FAIL;
  return S_OK;
}


void CBenchmarkDialog::AppendBenchInfo(LPCWSTR methodName, BOOL finalize)
{
  if (m_lstBench.m_hWnd == NULL)
    return;

  FString strMethodName = GetSystemString(methodName);

  int nIndex = m_lstBench.InsertItem(m_lstBench.GetItemCount(), strMethodName);

  if (finalize) {
    WCHAR s[16];
    UInt64 speed;

    speed = Sync.CompressingInfo.GetSpeed(Sync.CompressingInfo.UnpackSize);
    speed = speed / 1024;
    ConvertUInt64ToString(speed, s);
    m_lstBench.SetItemText(nIndex, 1, GetSystemString(s));
    m_lstBench.SetItemText(nIndex, 2, _T("KB/s"));

    speed = Sync.DecompressingInfo.GetSpeed(Sync.DecompressingInfo.UnpackSize);
    speed = speed / 1024;
    ConvertUInt64ToString(speed, s);
    m_lstBench.SetItemText(nIndex, 3, GetSystemString(s));
    m_lstBench.SetItemText(nIndex, 4, _T("KB/s"));
  }
  else {
    m_lstBench.SetItemText(nIndex, 1, kProcessingString);
    m_lstBench.SetItemText(nIndex, 2, kProcessingString);
    m_lstBench.SetItemText(nIndex, 3, kProcessingString);
    m_lstBench.SetItemText(nIndex, 4, kProcessingString);
  }
  AdjustWidth();

  m_lstBench.UpdateWindow();

  if (!m_strModules.IsEmpty())
    m_strModules += _T(", ");
  m_strModules += strMethodName;
}


void CBenchmarkDialog::UpdateLastBenchInfo()
{
  if (m_lstBench.m_hWnd == NULL)
    return;

  int nIndex = m_lstBench.GetItemCount() - 1;

  WCHAR s[16];
  UInt64 speed;

  speed = Sync.CompressingInfo.GetSpeed(
    Sync.CompressingInfo.UnpackSize * Sync.CompressingInfo.NumIterations);
  speed = speed / 1024;
  m_encodings.Add((int)speed);
  m_nEncMax = max(m_nEncMax, speed);
  ConvertUInt64ToString(speed, s);
  m_lstBench.SetItemText(nIndex, 1, GetSystemString(s));
  m_lstBench.SetItemText(nIndex, 2, _T("KB/s"));

  speed = Sync.DecompressingInfo.GetSpeed(
    Sync.DecompressingInfo.UnpackSize * Sync.DecompressingInfo.NumIterations);
  speed = speed / 1024;
  m_decodings.Add((int)speed);
  m_nDecMax = max(m_nDecMax, speed);
  ConvertUInt64ToString(speed, s);
  m_lstBench.SetItemText(nIndex, 3, GetSystemString(s));
  m_lstBench.SetItemText(nIndex, 4, _T("KB/s"));

  AdjustWidth();

  UInt64 nEncSum = 0;
  FOR_VECTOR(i, m_encodings) {
    nEncSum += m_encodings[i];
  }
  m_strTotalComp = FormatWithCommas(nEncSum / m_encodings.Size()).c_str();
  m_strTotalComp += _T(" KB/s");

  UInt64 nDecSum = 0;
  FOR_VECTOR(i, m_decodings) {
    nDecSum += m_decodings[i];
  }
  m_strTotalDecomp = FormatWithCommas(nDecSum / m_decodings.Size()).c_str();
  m_strTotalDecomp += _T(" KB/s");
}


void CBenchmarkDialog::OnNMCustomdrawBench(NMHDR *pNMHDR, LRESULT *pResult)
{
  LPNMLVCUSTOMDRAW pLVCD = reinterpret_cast<LPNMLVCUSTOMDRAW>(pNMHDR);
  int nRow = (int)pLVCD->nmcd.dwItemSpec;
  int nRowItemData = (int)pLVCD->nmcd.lItemlParam;

  *pResult = CDRF_DODEFAULT;

  switch (pLVCD->nmcd.dwDrawStage)
  {
  case CDDS_PREPAINT:
    *pResult |= CDRF_NOTIFYITEMDRAW;
    break;
  case CDDS_ITEMPREPAINT:
    *pResult = CDRF_NOTIFYSUBITEMDRAW;
    break;
  case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
    if (pLVCD->iSubItem == 1 || pLVCD->iSubItem == 3) {
      int row = (int)pLVCD->nmcd.dwItemSpec;
      int col = pLVCD->iSubItem;

      CIntVector &speeds = ((col == 1) ? m_encodings : m_decodings);
      if (row < (int)speeds.Size()) {
        CRect rt;
        CDC *pDC = CDC::FromHandle(pLVCD->nmcd.hdc);

        int level = speeds[row];
        m_lstBench.GetSubItemRect(row, col, LVIR_BOUNDS, rt);
        DrawLevel(pDC, &rt, level, pLVCD->iSubItem == 1);
        *pResult = CDRF_SKIPDEFAULT;
      }
      break;
    }
  default:
    break;
  }
}


void CBenchmarkDialog::DrawLevel(CDC *pDC, LPRECT lprt, UInt64 nLevel, BOOL enc)
{
  UInt64 nMax = enc ? m_nEncMax : m_nDecMax;
  if (nMax <= 0)
    return;

  int nSaved = pDC->SaveDC();

  BITMAP bm;
  CBitmap &bmp = enc ? m_bmpLevel1 : m_bmpLevel2;
  bmp.GetBitmap(&bm);

  CDC dcMem;
  dcMem.CreateCompatibleDC(pDC);
  dcMem.SelectObject(&bmp);

  CRect rt = *lprt;
  int nWidth = rt.Width();
  rt.right = (int)(rt.left + nLevel * nWidth / nMax);

  pDC->FillSolidRect(lprt, RGB(255, 255, 255));

  pDC->IntersectClipRect(&rt);
  pDC->SetStretchBltMode(HALFTONE);
  pDC->StretchBlt(lprt->left, lprt->top,
    lprt->right - lprt->left, lprt->bottom - lprt->top,
    &dcMem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);

// 	BLENDFUNCTION bf;
// 	memset(&bf, 0, sizeof(bf));
// 	bf.BlendOp = AC_SRC_OVER;
// 	bf.BlendFlags = 0;
// 	bf.SourceConstantAlpha = 255;
// 	bf.AlphaFormat = AC_SRC_ALPHA;
// 	pDC->AlphaBlend(rt.left, rt.top, rt.Width(), rt.Height(),
// 		&dcMem, 0, 0,
// 		bm.bmWidth * rt.Width() / nWidth, bm.bmHeight, bf);

  pDC->SelectClipRgn(NULL);

  pDC->SetROP2(R2_XORPEN);
  CString str = FormatWithCommas(nLevel).c_str();
  pDC->SetBkMode(TRANSPARENT);
  pDC->DrawText(str, lprt, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

  pDC->RestoreDC(nSaved);
}


void CBenchmarkDialog::OnSize(UINT nType, int cx, int cy)
{
  CDialogEx::OnSize(nType, cx, cy);

  if (!IsWindow(m_lstBench))
    return;

  AdjustWidth();
}

void CBenchmarkDialog::AdjustWidth()
{
  const int count = 5;

  CRect rt;
  m_lstBench.GetWindowRect(&rt);
  int width = 0;
  for (int i = 0; i < count; i++) {
    width += m_lstBench.GetColumnWidth(i);
  }

//   for (int i = 0; i < count; i++)
//     m_lstBench.SetColumnWidth(i, LVSCW_AUTOSIZE_USEHEADER);

  for (int i = 0; i < count; i++) {
    m_lstBench.SetColumnWidth(i, m_lstBench.GetColumnWidth(i) * rt.Width() / width);
  }
  m_lstBench.RedrawItems(0, count - 1);
}
