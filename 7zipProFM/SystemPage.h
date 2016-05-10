#pragma once


#include "CPP/7zip/UI/FileManager/FilePlugins.h"
#include "RegistryAssociations.h"

enum EExtState
{
  kExtState_Clear = 0,
  kExtState_Other,
  kExtState_7ZipPro
};

struct CModifiedExtInfo : public NRegistryAssoc::CShellExtInfo
{
  int OldState;
  int State;
  int ImageIndex;
  bool Other;
  bool Other7ZipPro;

  CModifiedExtInfo() : ImageIndex(-1) {}

  CSysString GetString() const;

  void SetState(const UString &iconPath)
  {
    State = kExtState_Clear;
    Other = false;
    Other7ZipPro = false;
    if (!ProgramKey.IsEmpty())
    {
      State = kExtState_Other;
      Other = true;
      if (IsIt7ZipPro())
      {
        Other7ZipPro = !iconPath.IsEqualTo_NoCase(IconPath);
        if (!Other7ZipPro)
        {
          State = kExtState_7ZipPro;
          Other = false;
        }
      }
    }
    OldState = State;
  };
};

struct CAssoc
{
  CModifiedExtInfo Pair[2];
  int SevenZipImageIndex;

  int GetIconIndex() const
  {
    for (int i = 0; i < 2; i++)
    {
      const CModifiedExtInfo &pair = Pair[i];
      if (pair.State == kExtState_Clear)
        continue;
      if (pair.State == kExtState_7ZipPro)
        return SevenZipImageIndex;
      if (pair.ImageIndex != -1)
        return pair.ImageIndex;
    }
    return -1;
  }
};

#ifdef UNDER_CE
  #define NUM_EXT_GROUPS 1
#else
  #define NUM_EXT_GROUPS 2
#endif

// CSystemPage dialog

class CSystemPage : public CPropertyPage
{
  DECLARE_DYNAMIC(CSystemPage)

public:
  CSystemPage();
  virtual ~CSystemPage();

  // Dialog Data
  enum { IDD = IDD_SYSTEM };

protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

  DECLARE_MESSAGE_MAP()
protected:
  CListCtrl m_lstAssociate;

  CExtDatabase _extDB;
  CObjectVector<CAssoc> _items;

  int _numIcons;
  CImageList _imageList;

  const HKEY GetHKey(int
      #if NUM_EXT_GROUPS != 1
        group
      #endif
      ) const
  {
    #if NUM_EXT_GROUPS == 1
      return HKEY_CLASSES_ROOT;
    #else
      return group == 0 ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;
    #endif
  }

  int AddIcon(const UString &path, int iconIndex);
  int GetRealIndex(int listIndex) const { return listIndex; }
  void RefreshListItem(int group, int listIndex);
  void ChangeState(int group, const CIntVector &indices);
  void ChangeState(int group);

public:
  bool WasChanged;
  virtual BOOL OnInitDialog();
  virtual BOOL OnApply();
  virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
  afx_msg void OnNMReturnSystemAssociate(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnNMClickSystemAssociate(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnBnClickedSystemCurrent();
  afx_msg void OnBnClickedSystemAll();
  afx_msg void OnLvnKeydownSystemAssociate(NMHDR *pNMHDR, LRESULT *pResult);
};
