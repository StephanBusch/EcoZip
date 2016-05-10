// LangPage.cpp : implementation file
//

#include "stdafx.h"

#include "CPP/Common/Lang.h"
#include "CPP/Common/StringConvert.h"

#include "CPP/Windows/FileFind.h"
#include "CPP/Windows/ResourceString.h"

#include "7zipProFM.h"
#include "LangPage.h"
#include "CPP/7zip/UI/FileManager/HelpUtils.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"


static const UInt32 kLangIDs[] =
{
  IDT_LANG_LANG
};

static LPCWSTR kLangTopic = L"fme/options.htm#language";

static void NativeLangString(UString &dest, const wchar_t *s)
{
  dest += L" (";
  dest += s;
  dest += L')';
}

// CLangPage dialog

IMPLEMENT_DYNAMIC(CLangPage, CPropertyPage)

CLangPage::CLangPage()
: CPropertyPage(CLangPage::IDD)
{

}

CLangPage::~CLangPage()
{
}

void CLangPage::DoDataExchange(CDataExchange* pDX)
{
  CPropertyPage::DoDataExchange(pDX);
  DDX_Control(pDX, IDC_LANG_LANG, m_cmbLang);
}


BEGIN_MESSAGE_MAP(CLangPage, CPropertyPage)
  ON_CBN_SELCHANGE(IDC_LANG_LANG, &CLangPage::OnCbnSelchangeLangLang)
END_MESSAGE_MAP()


// CLangPage message handlers


bool LangOpen(CLang &lang, CFSTR fileName);

BOOL CLangPage::OnInitDialog()
{
  CPropertyPage::OnInitDialog();

  LangSetDlgItems(*this, kLangIDs, ARRAY_SIZE(kLangIDs));

  UString temp = NWindows::MyLoadString(IDS_LANG_ENGLISH);
  NativeLangString(temp, NWindows::MyLoadString(IDS_LANG_NATIVE));
  int index = (int)m_cmbLang.AddString(GetSystemString(temp));
  m_cmbLang.SetItemData(index, _paths.Size());
  _paths.Add(L"-");
  m_cmbLang.SetCurSel(0);

  const FString dirPrefix = GetLangDirPrefix();
  NWindows::NFile::NFind::CEnumerator enumerator(dirPrefix + FTEXT("*.txt"));
  NWindows::NFile::NFind::CFileInfo fi;
  CLang lang;
  UString error;

  while (enumerator.Next(fi))
  {
    if (fi.IsDir())
      continue;
    const int kExtSize = 4;
    if (fi.Name.Len() < kExtSize)
      continue;
    unsigned pos = fi.Name.Len() - kExtSize;
    if (!StringsAreEqualNoCase_Ascii(fi.Name.Ptr(pos), ".txt"))
      continue;

    if (!LangOpen(lang, dirPrefix + fi.Name))
    {
      error.Add_Space_if_NotEmpty();
      error += fs2us(fi.Name);
      continue;
    }

    const UString shortName = fs2us(fi.Name.Left(pos));
    UString s = shortName;
    const wchar_t *eng = lang.Get(IDS_LANG_ENGLISH);
    if (eng)
      s = eng;
    const wchar_t *native = lang.Get(IDS_LANG_NATIVE);
    if (native)
      NativeLangString(s, native);
    index = (int)m_cmbLang.AddString(GetSystemString(s));
    m_cmbLang.SetItemData(index, _paths.Size());
    _paths.Add(shortName);
    if (g_LangID.IsEqualTo_NoCase(shortName))
      m_cmbLang.SetCurSel(index);
  }

  if (!error.IsEmpty())
    ::MessageBoxW(m_hWnd, error, L"Error in Lang file", MB_OK | MB_ICONSTOP);

  return TRUE;  // return TRUE unless you set the focus to a control
  // EXCEPTION: OCX Property Pages should return FALSE
}


BOOL CLangPage::OnApply()
{
  int pathIndex = (int)m_cmbLang.GetItemData(m_cmbLang.GetCurSel());
  SaveRegLang(_paths[pathIndex]);
  ReloadLang();
  LangWasChanged = true;

  return CPropertyPage::OnApply();
}


BOOL CLangPage::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
  LPNMHDR pNMHDR = (LPNMHDR)lParam;
  if (pNMHDR->code == PSN_HELP) {
    ShowHelpWindow(NULL, kLangTopic);
    return TRUE;
  }

  return CPropertyPage::OnNotify(wParam, lParam, pResult);
}


void CLangPage::OnCbnSelchangeLangLang()
{
  SetModified();
}
