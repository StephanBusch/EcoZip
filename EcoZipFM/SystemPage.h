#pragma once


#include "CPP/7zip/UI/FileManager/FilePlugins.h"
#include "RegistryAssociations.h"

enum EExtState
{
  kExtState_Clear = 0,
  kExtState_Other,
  kExtState_EcoZip
};

struct CModifiedExtInfo : public NRegistryAssoc::CShellExtInfo
{
  int OldState;
  int State;
  int ImageIndex;
  bool Other;
  bool OtherEcoZip;

  CModifiedExtInfo() : ImageIndex(-1) {}

  CSysString GetString() const;

  void SetState(const UString &iconPath)
  {
    State = kExtState_Clear;
    Other = false;
    OtherEcoZip = false;
    if (!ProgramKey.IsEmpty())
    {
      State = kExtState_Other;
      Other = true;
      if (IsItEcoZip())
      {
        OtherEcoZip = !iconPath.IsEqualTo_NoCase(IconPath);
        if (!OtherEcoZip)
        {
          State = kExtState_EcoZip;
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
    for (unsigned i = 0; i < 2; i++)
    {
      const CModifiedExtInfo &pair = Pair[i];
      if (pair.State == kExtState_Clear)
        continue;
      if (pair.State == kExtState_EcoZip)
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

  unsigned _numIcons;
  CImageList _imageList;

  const HKEY GetHKey(unsigned
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
  unsigned GetRealIndex(unsigned listIndex) const { return listIndex; }
  void RefreshListItem(unsigned group, unsigned listIndex);
  void ChangeState(unsigned group, const CUIntVector &indices);
  void ChangeState(unsigned group);

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
