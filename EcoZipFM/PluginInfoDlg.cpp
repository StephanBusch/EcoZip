// PluginInfoDlg.cpp : implementation file
//

#include "stdafx.h"

#include "CPP/Common/IntToString.h"
#include "CPP/Common/StringConvert.h"

#include "CPP/7zip/Common/CreateCoder.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "LoadCodecs.h"

#include "EcoZipFM.h"
#include "PluginInfoDlg.h"


static const UInt32 kLangIDs[] =
{
  IDT_BASE_FOLDER
};

// CPluginInfoDlg dialog

IMPLEMENT_DYNAMIC(CPluginInfoDlg, CDialogEx)

CPluginInfoDlg::CPluginInfoDlg(CWnd* pParent /*=NULL*/)
  : CDialogEx(CPluginInfoDlg::IDD, pParent)
{

}

CPluginInfoDlg::~CPluginInfoDlg()
{
}

void CPluginInfoDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialogEx::DoDataExchange(pDX);
  DDX_Control(pDX, IDC_LST_PLUGINS, m_lstPlugins);
}


BEGIN_MESSAGE_MAP(CPluginInfoDlg, CDialogEx)
END_MESSAGE_MAP()


// CPluginInfoDlg message handlers


extern FString GetBaseFolderPrefixFromRegistry();
HRESULT ReadNumberOfStreams(ICompressCodecsInfo *codecsInfo, UInt32 index, PROPID propID, UInt32 &res);
HRESULT ReadIsAssignedProp(ICompressCodecsInfo *codecsInfo, UInt32 index, PROPID propID, bool &res);


BOOL CPluginInfoDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

  LangSetDlgItems(*this, kLangIDs, ARRAY_SIZE(kLangIDs));

  SetDlgItemText(IDT_BASE_FOLDER_VAL, m_strBaseFolder);

  // set style for tree view
  UINT uTreeStyle = TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | /*TVS_CHECKBOXES | */TVS_FULLROWSELECT;
  m_lstPlugins.GetTreeCtrl().ModifyStyle(0, uTreeStyle);

  /*
   * Set background image (works with owner-drawn tree only)
   */

  LVBKIMAGE bk;
  bk.xOffsetPercent = bk.yOffsetPercent = 70;
  bk.hbm = LoadBitmap(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_LOGO));
  m_lstPlugins.GetTreeCtrl().SetBkImage(&bk);

  /*
   * Create image list for tree & load icons
   */

  m_imgList.Create(16, 16, ILC_MASK | ILC_COLOR32, 0, 0);

  // assign image list to tree control
  m_lstPlugins.GetTreeCtrl().SetImageList(&m_imgList, TVSIL_NORMAL);

  /*
   *  Insert columns to tree control
   */

  m_lstPlugins.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 120);
  m_lstPlugins.InsertColumn(1, _T("ID"), LVCFMT_CENTER, 60);
  m_lstPlugins.InsertColumn(2, _T("Decoder"), LVCFMT_CENTER, 70);
  m_lstPlugins.InsertColumn(3, _T("Encoder"), LVCFMT_CENTER, 70);
  m_lstPlugins.InsertColumn(4, _T("InStreams"), LVCFMT_CENTER, 70);
  m_lstPlugins.InsertColumn(5, _T("OutStreams"), LVCFMT_CENTER, 75);
  m_lstPlugins.InsertColumn(6, _T("Description"), LVCFMT_CENTER, 80);
  m_lstPlugins.InsertColumn(7, _T("DigestSize"), LVCFMT_CENTER, 70);

  CCodecs *codecs = new CCodecs;
  if (FAILED(codecs->LoadExternalCodecs(m_strBaseFolder, m_codecDlls)))
    return FALSE;

  CCustomTreeChildCtrl &ctrl = m_lstPlugins.GetTreeCtrl();

  FOR_VECTOR(j, codecs->Libs) {
    const CCodecLib &lib = codecs->Libs[j];
    FString path = lib.Path;
    path.Replace(m_strBaseFolder, _T(""));
    HTREEITEM hPlugin = ctrl.InsertItem(path);

    UInt32 num;
    if (FAILED(codecs->GetNumMethods(&num)))
      return FALSE;
    if (num > 0) {
      HTREEITEM hCodecs = ctrl.InsertItem(_T("Codecs"), hPlugin);
      for (UInt32 i = 0; i < num; i++) {
        CDllCodecInfo &codecInfo = codecs->Codecs[i];
        if (codecInfo.LibIndex != j)
          continue;
        CCodecInfoEx info;
        if (FAILED(codecs->GetCodec_Id(i, info.Id)))
          continue;
        info.Name = codecs->GetCodec_Name(i);

        ReadNumberOfStreams(codecs, i, NMethodPropID::kPackStreams, info.NumStreams);
        ReadNumberOfStreams(codecs, i, NMethodPropID::kUnpackStreams, info.NumStreams);
        ReadIsAssignedProp(codecs, i, NMethodPropID::kEncoderIsAssigned, info.EncoderIsAssigned);
        ReadIsAssignedProp(codecs, i, NMethodPropID::kDecoderIsAssigned, info.DecoderIsAssigned);

        HTREEITEM hCodec = ctrl.InsertItem(GetSystemString(info.Name), hCodecs);
        CHAR szBuff[32];
        ConvertUInt64ToHex(info.Id, szBuff);
        m_lstPlugins.SetItemText(hCodec, 1, GetSystemString(szBuff));
        if (info.DecoderIsAssigned)
          m_lstPlugins.SetItemText(hCodec, 2, _T("Assigned"));
        if (info.EncoderIsAssigned)
          m_lstPlugins.SetItemText(hCodec, 3, _T("Assigned"));
        ConvertUInt32ToString(info.NumStreams, szBuff);
        m_lstPlugins.SetItemText(hCodec, 4, GetSystemString(szBuff));
        ConvertUInt32ToString(info.NumStreams, szBuff);
        m_lstPlugins.SetItemText(hCodec, 5, GetSystemString(szBuff));
      }

      ctrl.Expand(hCodecs, TVE_EXPAND);
    }

    num = codecs->GetNumHashers();
    if (num > 0) {
      HTREEITEM hHashers = ctrl.InsertItem(_T("Hashers"), hPlugin);
      for (UInt32 i = 0; i < num; i++)
      {
        CDllHasherInfo &hasherInfo = codecs->Hashers[i];
        if (hasherInfo.LibIndex != j)
          continue;
        CHasherInfoEx info;
        if (FAILED(info.Id = codecs->GetHasherId(i)))
          return FALSE;
        info.Name = codecs->GetHasherName(i);
        UInt32 DigestSize = codecs->GetHasherDigestSize(i);

        HTREEITEM hHasher = ctrl.InsertItem(GetSystemString(info.Name), hHashers);
        CHAR szBuff[32];
        ConvertUInt64ToHex(info.Id, szBuff);
        m_lstPlugins.SetItemText(hHasher, 1, GetSystemString(szBuff));
        ConvertUInt32ToString(DigestSize, szBuff);
        m_lstPlugins.SetItemText(hHasher, 7, GetSystemString(szBuff));
      }

      ctrl.Expand(hHashers, TVE_EXPAND);
    }

    ctrl.Expand(hPlugin, TVE_EXPAND);
  }

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

