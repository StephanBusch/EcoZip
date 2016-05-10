// DetailView.cpp : implementation file
//

#include "stdafx.h"
#include "7zipProFM.h"
#include "DetailView.h"
#include "ImageUtils.h"

#include "CPP/Common/IntToString.h"
#include "CPP/Common/StringConvert.h"
#include "CPP/Windows/PropVariant.h"
#include "CPP/Windows/PropVariantConv.h"
#include "CPP/7zip/UI/Common/PropIDUtils.h"
#include "CPP/7zip/UI/FileManager/PropertyName.h"

#include "FMUtils.h"


using namespace NWindows;

// CDetailView

IMPLEMENT_DYNCREATE(CDetailView, CWnd)

CDetailView::CDetailView()
{
  m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
  m_mode = Logo;
  m_nItemIndex = -1;
  m_nImage = -1;
  m_attrib = 0;
}

CDetailView::~CDetailView()
{
}




BOOL CDetailView::PreCreateWindow(CREATESTRUCT& cs)
{
  // TODO: Modify the Window class or styles here by modifying
  //  the CREATESTRUCT cs

  return CWnd::PreCreateWindow(cs);
}


BEGIN_MESSAGE_MAP(CDetailView, CWnd)
  ON_WM_PAINT()
  ON_WM_CREATE()
  ON_WM_SIZE()
END_MESSAGE_MAP()



// DetailView message handlers


int CDetailView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
  if (CWnd::OnCreate(lpCreateStruct) == -1)
    return -1;

  CDC *pDC = GetDC();

  CImageUtils::LoadBitmapFromPNG(IDB_ECO_LARGE, m_bmpLogo);
  CImageUtils::PremultiplyBitmapAlpha(*pDC, m_bmpLogo);

  ReleaseDC(pDC);

  return 0;
}


void CDetailView::OnPaint()
{
  CPaintDC dc(this); // device context for painting

  CRect rt;
  GetClientRect(&rt);

  CDC dcMem;
  dcMem.CreateCompatibleDC(&dc);
  dcMem.SelectObject(&m_bmpBkgnd);
  dc.BitBlt(rt.left, rt.top, rt.Width(), rt.Height(), &dcMem, 0, 0, SRCCOPY);
}


void CDetailView::OnSize(UINT nType, int cx, int cy)
{
  CWnd::OnSize(nType, cx, cy);

  if (m_bmpBkgnd.m_hObject != NULL)
    m_bmpBkgnd.DeleteObject();

  CDC * pDC = GetDC();

  m_bmpBkgnd.CreateCompatibleBitmap(pDC, cx, cy);
  CDC dcMem;
  dcMem.CreateCompatibleDC(pDC);
  dcMem.SelectObject(&m_bmpBkgnd);
  DrawContents(&dcMem);

  ReleaseDC(pDC);
}


void CDetailView::DrawContents(CDC *pDC)
{
  switch (m_mode)
  {
  case CDetailView::Logo:
    DrawLogo(pDC);
    break;
  case CDetailView::Details:
    DrawDetails(pDC);
    break;
  case CDetailView::Preview:
    DrawPreview(pDC);
    break;
  default:
    break;
  }
}


void CDetailView::DrawLogo(CDC *pDC)
{
  CRect rt;
  GetClientRect(&rt);

  pDC->FillSolidRect(&rt, RGB(255, 255, 255));

  BITMAP bmLogo;
  m_bmpLogo.GetBitmap(&bmLogo);

  CDC dcMem;
  dcMem.CreateCompatibleDC(pDC);

  //   int sz = 128;// min(rt.Width(), rt.Height()) / 2;
  //   ::DrawIconEx(*pDC, (rt.left + rt.right - sz) / 2,
  //     (rt.top + rt.bottom - sz) / 2, m_hIcon,
  //     sz, sz, 0, NULL, DI_NORMAL);

  BLENDFUNCTION bf;
  memset(&bf, 0, sizeof(bf));
  bf.BlendOp = AC_SRC_OVER;
  bf.BlendFlags = 0;
  bf.SourceConstantAlpha = 255;
  bf.AlphaFormat = AC_SRC_ALPHA;

  dcMem.SelectObject(&m_bmpLogo);
  pDC->AlphaBlend(
    (rt.right - bmLogo.bmWidth) / 2, (rt.bottom - bmLogo.bmHeight) / 2,
    bmLogo.bmWidth, bmLogo.bmHeight,
    &dcMem, 0, 0, bmLogo.bmWidth, bmLogo.bmHeight, bf);
}


void CDetailView::DrawDetails(CDC *pDC)
{
  int nSaved = pDC->SaveDC();

  CFont font;
  font.CreateFont(16, 0, 0, 0, FW_NORMAL,
    FALSE, FALSE, FALSE, 0, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
    DEFAULT_QUALITY, DEFAULT_PITCH | FF_ROMAN, _T("Segoe UI"));
  pDC->SelectObject(&font);

  CRect rt;
  GetClientRect(&rt);

  pDC->FillSolidRect(&rt, RGB(255, 255, 255));

  int x = 10;
  int y = 10;
  int margine = 16;
  int maxwidth = 0;
  for (unsigned i = 0; i < m_propNames.Size(); i++) {
    UString name = GetSystemString(m_propNames[i]) + _T(" :");
    CSize sz = pDC->GetTextExtent(name, name.Len());
    maxwidth = max(maxwidth, sz.cx);
  }

  for (unsigned i = 0; i < m_propNames.Size(); i++) {
    UString &name = m_propNames[i];
    UString &value = m_propValues[i];
    pDC->TextOutW(x, y, name.Ptr());
    pDC->TextOutW(x + maxwidth + margine, y, value.Ptr());
    y += margine;
  }

  pDC->RestoreDC(nSaved);
}


void CDetailView::DrawPreview(CDC *pDC)
{
  if (!m_imgPreview.IsLoaded()) {
    DrawDetails(pDC);
    return;
  }

  CRect rt;
  GetClientRect(&rt);
  if (rt.IsRectEmpty())
    return;

  pDC->FillSolidRect(&rt, RGB(255, 255, 255));

  int width = rt.Width();
  int height = (int)m_imgPreview.GetHeight() * width / m_imgPreview.GetWidth();
  if (height > rt.Height()) {
    height = rt.Height();
    width = (int)m_imgPreview.GetWidth() * height / m_imgPreview.GetHeight();
  }

  m_imgPreview.Render(pDC->m_hDC,
    (rt.right - width) / 2, (rt.bottom - height) / 2, width, height);
}


HRESULT CDetailView::SetSelItem(CMyComPtr<IFolderFolder> folder, int itemIndex,
  UString &currentFolderPrefix, bool preview)
{
  if (!folder)
    return S_FALSE;

  _folderCompare.Release();
  _folderGetItemName.Release();
  _folderRawProps.Release();
  _folder.Release();

  _folder = folder;
  UString fullPath = GetFolderPath(_folder);

  const wchar_t *name = NULL;
  int iImage = 0;

  _folder.QueryInterface(IID_IFolderCompare, &_folderCompare);
  _folder.QueryInterface(IID_IFolderGetItemName, &_folderGetItemName);
  _folder.QueryInterface(IID_IArchiveGetRawProps, &_folderRawProps);

  if (!IsFSFolder(_folder))
    _folder.QueryInterface(IID_IFolderGetSystemIconIndex, &folderGetSystemIconIndex);

  m_nItemIndex = itemIndex;

  UString correctedName;
  UString itemName;
  UString relPath;

  m_propNames.Clear();
  m_propValues.Clear();

  unsigned nameLen = 0;
  if (_folderGetItemName)
    _folderGetItemName->GetItemName(itemIndex, &name, &nameLen);
  if (name == NULL) {
    NCOM::CPropVariant prop;
    if (_folder->GetProperty(itemIndex, kpidName, &prop) != S_OK)
      throw 2723400;
    if (prop.vt != VT_BSTR)
      throw 2723401;
    itemName = prop.bstrVal;
    name = itemName;
    nameLen = itemName.Len();
  }

  m_attrib = 0;
  // for (int yyy = 0; yyy < 6000000; yyy++) {
  NCOM::CPropVariant prop;
  RINOK(_folder->GetProperty(itemIndex, kpidAttrib, &prop));
  if (prop.vt == VT_UI4)
  {
    // char s[256]; sprintf(s, "attrib = %7x", attrib); OutputDebugStringA(s);
    m_attrib = prop.ulVal;
  }
  else if (GetItem_BoolProp(_folder, itemIndex, kpidIsDir))
    m_attrib |= FILE_ATTRIBUTE_DIRECTORY;
  // }

  bool defined = false;
  if (folderGetSystemIconIndex) {
    folderGetSystemIconIndex->GetSystemIconIndex(itemIndex, &iImage);
    defined = (iImage > 0);
  }
  if (!defined) {
    if (currentFolderPrefix.IsEmpty()) {
      int iconIndexTemp;
      GetRealIconIndex(us2fs((UString)name) + FCHAR_PATH_SEPARATOR, m_attrib, iconIndexTemp);
      iImage = iconIndexTemp;
    }
    else
      iImage = _extToIconMap.GetIconIndex(m_attrib, name);
  }
  if (iImage < 0)
    iImage = 0;

  m_strItemName = name;
  m_nImage = iImage;

  if (_folderRawProps) {
    UInt32 numProps = 0;
    _folderRawProps->GetNumRawProps(&numProps);
    for (unsigned i = 0; i < numProps; i++) {
      UString strName;
      UString strValue;

      CMyComBSTR name;
      PROPID propID;
      RINOK(_folderRawProps->GetRawPropInfo(i, &name, &propID));

      const void *data;
      UInt32 dataSize;
      UInt32 propType;
      RINOK(_folderRawProps->GetRawProp(itemIndex, propID, &data, &dataSize, &propType));
      if (dataSize == 0)
        continue;

      strName = GetNameOfProperty(propID, name);

      if (propID == kpidNtReparse)
        ConvertNtReparseToString((const Byte *)data, dataSize, strValue);
      else if (propID == kpidNtSecure) {
        AString s;
        ConvertNtSecureToString((const Byte *)data, dataSize, s);
        if (!s.IsEmpty())
          strValue = GetUnicodeString(s);
      }
      else {
        const UInt32 kMaxDataSize = 64;
        if (dataSize > kMaxDataSize) {
          wchar_t temp[64] = L"data:";
          ConvertUInt32ToString(dataSize, temp + 5);
          strValue = temp;
        }
        else
        {
          WCHAR *dest = strValue.GetBuf(dataSize * 2 + 1);
          for (UInt32 i = 0; i < dataSize; i++)
          {
            Byte b = ((const Byte *)data)[i];
            dest[0] = GetHex((Byte)((b >> 4) & 0xF));
            dest[1] = GetHex((Byte)(b & 0xF));
            dest += 2;
          }
          *dest = 0;
          strValue.ReleaseBuf_CalcLen(dataSize * 2 + 1);
        }
      }

      if (!strValue.IsEmpty()) {
        m_propNames.Add(strName);
        m_propValues.Add(strValue);
      }
    }
  }

  UInt32 numProps;
  _folder->GetNumberOfProperties(&numProps);

  for (UInt32 i = 0; i < numProps; i++) {
    UString strName;
    UString strValue;

    CMyComBSTR name;
    PROPID propID;
    VARTYPE varType;
    HRESULT res = _folder->GetPropertyInfo(i, &name, &propID, &varType);

    if (res != S_OK)
    {
      /* We can return ERROR, but in that case, other code will not be called,
      and user can see empty window without error message. So we just ignore that field */
      continue;
    }
    if (propID == kpidIsDir)
      continue;
    strName = GetNameOfProperty(propID, name);
    // 		CItemProperty prop;
    // 		prop.Type = varType;
    // 		prop.ID = propID;
    // 		prop.Name = GetNameOfProperty(propID, name);
    // 		prop.Order = -1;
    // 		prop.IsVisible = GetColumnVisible(propID, isFsFolder);
    // 		prop.Width = ::GetColumnWidth(propID, varType);
    // 		prop.IsRawProp = false;
    // 		_properties.Add(prop);

    NCOM::CPropVariant prop;

    if (propID == kpidName) {
    }
    if (propID == kpidPrefix) {
      if (_folderGetItemName) {
        const wchar_t *name = NULL;
        unsigned nameLen = 0;
        _folderGetItemName->GetItemPrefix(itemIndex, &name, &nameLen);
        if (name)
          strValue = name;
      }
    }
    else {
      HRESULT res = _folder->GetProperty(itemIndex, propID, &prop);
      if (res != S_OK)
        continue;
      else if ((prop.vt == VT_UI8 || prop.vt == VT_UI4 || prop.vt == VT_UI2) && (
        propID == kpidSize ||
        propID == kpidPackSize ||
        propID == kpidNumSubDirs ||
        propID == kpidNumSubFiles ||
        propID == kpidPosition ||
        propID == kpidNumBlocks ||
        propID == kpidClusterSize ||
        propID == kpidTotalSize ||
        propID == kpidFreeSpace ||
        propID == kpidUnpackSize
        ))
      {
        UInt64 v = 0;
        ConvertPropVariantToUInt64(prop, v);
        strValue = ConvertIntToDecimalString(v);
      }
      else if (prop.vt == VT_BSTR)
        strValue = prop.bstrVal;
      else
        ConvertPropertyToString(strValue, prop, propID, false);
    }

    if (!strValue.IsEmpty()) {
      m_propNames.Add(strName);
      m_propValues.Add(strValue);
    }
  }

  m_mode = preview ? Preview : Details;

  if (m_mode == Preview)
    LoadPreviewData();

  CDC * pDC = GetDC();

  CDC dcMem;
  dcMem.CreateCompatibleDC(pDC);
  dcMem.SelectObject(&m_bmpBkgnd);
  DrawContents(&dcMem);
  ReleaseDC(pDC);

  Invalidate();

  return S_OK;
}


void CDetailView::LoadPreviewData()
{
  if (IsFSFolder(_folder)) {
    UString strPath = GetFolderPath(_folder);
    strPath += m_strItemName;
    strPath = GetSystemString(strPath);

    if (FAILED(m_imgPreview.Open(strPath.Ptr())))
      m_mode = Details;
  }
}

