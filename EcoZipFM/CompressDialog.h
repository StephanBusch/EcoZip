// CompressDialog.h

#pragma once

#include "CPP/Common/Wildcard.h"

#include "CPP/7zip/UI/Common/LoadCodecs.h"
#include "CPP/7zip/UI/Common/ZipRegistry.h"

#include "resource.h"

namespace NCompressDialog
{
  namespace NUpdateMode
  {
    enum EEnum
    {
      kAdd,
      kUpdate,
      kFresh,
      kSync
    };
  }
  
  struct CInfo
  {
    NUpdateMode::EEnum UpdateMode;
    NWildcard::ECensorPathMode PathMode;

    bool SolidIsSpecified;
    bool MultiThreadIsAllowed;
    UInt64 SolidBlockSize;
    UInt32 NumThreads;

    CRecordVector<UInt64> VolumeSizes;

    UInt32 Level;
    UString Method;
    UInt32 Dictionary;
    bool OrderMode;
    UInt32 Order;
    UString Options;

    UString EncryptionMethod;

    bool SFXMode;
    bool OpenShareForWrite;
    bool LoadConfig;
    bool DeleteAfterCompressing;
    
    CBoolPair SymLinks;
    CBoolPair HardLinks;
    CBoolPair AltStreams;
    CBoolPair NtSecurity;
    
    UString ArcPath; // in: Relative or abs ; out: Relative or abs
    
    // FString CurrentDirPrefix;
    bool KeepName;

    bool GetFullPathName(UString &result) const;

    int FormatIndex;

    UString Password;
    bool EncryptHeadersIsAllowed;
    bool EncryptHeaders;

    CInfo():
        UpdateMode(NCompressDialog::NUpdateMode::kAdd),
        PathMode(NWildcard::k_RelatPath),
        SFXMode(false),
        OpenShareForWrite(false),
        DeleteAfterCompressing(false),
        FormatIndex(-1)
    {
      Level = Dictionary = Order = UInt32(-1);
      OrderMode = false;
      Method.Empty();
      Options.Empty();
      EncryptionMethod.Empty();
    }
  };
}

// CCompressDialog dialog

class CCompressDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CCompressDialog)

public:
	CCompressDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CCompressDialog();

// Dialog Data
	enum { IDD = IDD_COMPRESS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	virtual BOOL OnInitDialog();

  DECLARE_MESSAGE_MAP()

protected:
  HICON m_hIcon;

  CComboBox m_cmbArchivePath;
  CComboBox m_cmbFormat;
  CComboBox m_cmbLevel;
  CComboBox m_cmbMethod;
  CComboBox m_cmbDictionary;
  CComboBox m_cmbOrder;
  CComboBox m_cmbSolid;
  CComboBox m_cmbNumThreads;

  CComboBox m_cmbUpdateMode;
  CComboBox m_cmbPathMode;

  CComboBox m_cmbVolume;
  CEdit m_edtParams;

  CEdit m_edtPassword1;
  CEdit m_edtPassword2;
  CComboBox m_cmbEncryptionMethod;

protected:
  NCompression::CInfo m_RegistryInfo;

  int m_PrevFormat;
  UString DirPrefix;
  UString StartDirPrefix;

  bool EnableItem(int itemID, bool enable) const
  { return BOOLToBool(GetDlgItem(itemID)->EnableWindow(BoolToBOOL(enable))); }

  bool ShowItem_Bool(int itemID, bool show) const
  { return BOOLToBool(GetDlgItem(itemID)->ShowWindow(show ? SW_SHOW : SW_HIDE)); }

  void CheckButton_TwoBools(UINT id, const CBoolPair &b1, const CBoolPair &b2);
  void GetButton_Bools(UINT id, CBoolPair &b1, CBoolPair &b2);

  void SetArchiveName(const UString &name);
  int FindRegistryFormat(const UString &name);
  int FindRegistryFormatAlways(const UString &name);

  void CheckSFXNameChange();
  void SetArchiveName2(bool prevWasSFX);

  int GetStaticFormatIndex();

  void SetNearestSelectComboBox(CComboBox &comboBox, UInt32 value);

  void SetLevel();

  void SetMethod(int keepMethodId = -1);
  int GetMethodID();
  UString GetMethodSpec();
  UString GetEncryptionMethodSpec();

  bool IsZipFormat();

  void SetEncryptionMethod();

  void AddDictionarySize(UInt32 size);

  void SetDictionary();

  UInt32 GetComboValue(CComboBox &c, int defMax = 0);

  UInt32 GetLevel()  { return GetComboValue(m_cmbLevel); }
  UInt32 GetLevelSpec()  { return GetComboValue(m_cmbLevel, 1); }
  UInt32 GetLevel2();
  UInt32 GetDictionary() { return GetComboValue(m_cmbDictionary); }
  UInt32 GetDictionarySpec() { return GetComboValue(m_cmbDictionary, 1); }
  UInt32 GetOrder() { return GetComboValue(m_cmbOrder); }
  UInt32 GetOrderSpec() { return GetComboValue(m_cmbOrder, 1); }
  UInt32 GetNumThreadsSpec() { return GetComboValue(m_cmbNumThreads, 1); }
  UInt32 GetNumThreads2() { UInt32 num = GetNumThreadsSpec(); if (num == UInt32(-1)) num = 1; return num; }
  UInt32 GetBlockSizeSpec() { return GetComboValue(m_cmbSolid, 1); }

  int AddOrder(UInt32 size);
  void SetOrder();
  bool GetOrderMode();

  void SetSolidBlockSize();
  void SetNumThreads();

  UInt64 GetMemoryUsage(UInt32 dict, UInt64 &decompressMemory);
  UInt64 GetMemoryUsage(UInt64 &decompressMemory);
  void PrintMemUsage(UINT res, UInt64 value);
  void SetMemoryUsage();
  void SetParams();
  void SaveOptionsInMem();

  void UpdatePasswordControl();
  bool IsShowPasswordChecked() const;

  unsigned GetFormatIndex();
  bool SetArcPathFields(const UString &path, UString &name, bool always);
  bool GetFinalPath_Smart(UString &resPath);

public:
  CObjectVector<CArcInfoEx> *ArcFormats;
  CUIntVector ArcIndices; // can not be empty, must contain Info.FormatIndex, if Info.FormatIndex >= 0

  NCompressDialog::CInfo Info;
  UString OriginalFileName; // for bzip2, gzip2
  bool CurrentDirWasChanged;

  INT_PTR Create(HWND wndParent = 0)
  {
    return DoModal();
  }

protected:

  void CheckSFXControlsEnable();
  void CheckCFGControlsEnable();
  // void CheckVolumeEnable();
  void CheckControlsEnable();
  void CheckCompressControlsEnable();

  afx_msg void OnBnClickedCompressSetArchive();
  bool IsSFX();
  void OnButtonSFX();
  bool IsLoadConfig();
  afx_msg void OnCbnSelchangeCompressArchive();
  afx_msg void OnCbnSelchangeCompressFormat();
  afx_msg void OnCbnSelchangeCompressLevel();
  afx_msg void OnCbnSelchangeCompressMethod();
  afx_msg void OnCbnSelchangeCompressDictionary();
  afx_msg void OnCbnSelchangeCompressSolid();
  afx_msg void OnCbnSelchangeCompressThreads();
  afx_msg void OnBnClickedCompressSfx();
  afx_msg void OnBnClickedPasswordShow();
  afx_msg void OnBnClickedLoadConfig();
  virtual void OnOK();
  virtual void OnHelp();
};
