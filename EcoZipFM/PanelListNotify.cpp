// PanelListNotify.cpp

#include "stdafx.h"

#include "CPP/Common/IntToString.h"

#include "CPP/Windows/PropVariant.h"
#include "CPP/Windows/PropVariantConv.h"

#include "CPP/7zip/UI/Common/PropIDUtils.h"

#include "EcoZipFM.h"
#include "ContentsView.h"
#include "FMUtils.h"

using namespace NWindows;


static inline unsigned GetHex(unsigned v)
{
  return (v < 10) ? ('0' + v) : ('A' + (v - 10));
}

/*
void HexToString(char *dest, const Byte *data, UInt32 size)
{
  for (UInt32 i = 0; i < size; i++)
  {
    unsigned b = data[i];
    dest[0] = GetHex((b >> 4) & 0xF);
    dest[1] = GetHex(b & 0xF);
    dest += 2;
  }
  *dest = 0;
}
*/

bool IsSizeProp(UINT propID) throw()
{
  switch (propID)
  {
    case kpidSize:
    case kpidPackSize:
    case kpidNumSubDirs:
    case kpidNumSubFiles:
    case kpidOffset:
    case kpidLinks:
    case kpidNumBlocks:
    case kpidNumVolumes:
    case kpidPhySize:
    case kpidHeadersSize:
    case kpidTotalSize:
    case kpidFreeSpace:
    case kpidClusterSize:
    case kpidNumErrors:
    case kpidNumStreams:
    case kpidNumAltStreams:
    case kpidAltStreamsSize:
    case kpidVirtualSize:
    case kpidUnpackSize:
    case kpidTotalPhySize:
    case kpidTailSize:
    case kpidEmbeddedStubSize:
      return true;
  }
  return false;
}

void CContentsView::OnLvnGetdispinfo(NMHDR *pNMHDR, LRESULT *pResult)
{
  NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
  *pResult = 0;

  //is the sub-item information being requested?

  if ((pDispInfo->item.mask & LVIF_TEXT) != 0 ||
    (pDispInfo->item.mask & LVIF_IMAGE) != 0)
    *pResult = GetDispInfo(pDispInfo->item);
}


LRESULT CContentsView::GetDispInfo(LVITEMW &item)
{
  if (_dontShowMode)
    return 0;
  UInt32 realIndex = GetRealIndex(item);

  /*
  if ((item.mask & LVIF_IMAGE) != 0)
  {
    bool defined  = false;
    CComPtr<IFolderGetSystemIconIndex> folderGetSystemIconIndex;
    _folder.QueryInterface(&folderGetSystemIconIndex);
    if (folderGetSystemIconIndex)
    {
      folderGetSystemIconIndex->GetSystemIconIndex(index, &item.iImage);
      defined = (item.iImage > 0);
    }
    if (!defined)
    {
      NCOM::CPropVariant prop;
      _folder->GetProperty(index, kpidAttrib, &prop);
      UINT32 attrib = 0;
      if (prop.vt == VT_UI4)
        attrib = prop.ulVal;
      else if (IsItemFolder(index))
        attrib |= FILE_ATTRIBUTE_DIRECTORY;
      if (_currentFolderPrefix.IsEmpty())
        throw 1;
      else
        item.iImage = _extToIconMap.GetIconIndex(attrib, GetSystemString(GetItemName(index)));
    }
    // item.iImage = 1;
  }
  */

  if ((item.mask & LVIF_TEXT) == 0)
    return 0;

  LPWSTR text = item.pszText;

  if (item.cchTextMax > 0)
    text[0] = 0;

  if (item.cchTextMax <= 1)
    return 0;
  
  const CPropColumn &property = _visibleColumns[item.iSubItem];
  PROPID propID = property.ID;

  if (realIndex == kParentIndex)
  {
    if (propID == kpidName)
    {
      if (item.cchTextMax > 2)
      {
        text[0] = '.';
        text[1] = '.';
        text[2] = 0;
      }
    }
    return 0;
  }

 
  if (property.IsRawProp)
  {
    const void *data;
    UInt32 dataSize;
    UInt32 propType;
    RINOK(_folderRawProps->GetRawProp(realIndex, propID, &data, &dataSize, &propType));
    unsigned limit = item.cchTextMax - 1;
    if (dataSize == 0)
    {
      text[0] = 0;
      return 0;
    }
    
    if (propID == kpidNtReparse)
    {
      UString s;
      ConvertNtReparseToString((const Byte *)data, dataSize, s);
      if (!s.IsEmpty())
      {
        unsigned i;
        for (i = 0; i < limit; i++)
        {
          wchar_t c = s[i];
          if (c == 0)
            break;
          text[i] = c;
        }
        text[i] = 0;
        return 0;
      }
    }
    else if (propID == kpidNtSecure)
    {
      AString s;
      ConvertNtSecureToString((const Byte *)data, dataSize, s);
      if (!s.IsEmpty())
      {
        unsigned i;
        for (i = 0; i < limit; i++)
        {
          wchar_t c = (Byte)s[i];
          if (c == 0)
            break;
          text[i] = c;
        }
        text[i] = 0;
        return 0;
      }
    }
    {
      const unsigned kMaxDataSize = 64;
      if (dataSize > kMaxDataSize)
      {
        char temp[32];
        MyStringCopy(temp, "data:");
        ConvertUInt32ToString(dataSize, temp + 5);
        unsigned i;
        for (i = 0; i < limit; i++)
        {
          wchar_t c = (Byte)temp[i];
          if (c == 0)
            break;
          text[i] = c;
        }
        text[i] = 0;
      }
      else
      {
        if (dataSize > limit)
          dataSize = limit;
        WCHAR *dest = text;
        for (UInt32 i = 0; i < dataSize; i++)
        {
          unsigned b = ((const Byte *)data)[i];
          dest[0] = (WCHAR)GetHex((b >> 4) & 0xF);
          dest[1] = (WCHAR)GetHex(b & 0xF);
          dest += 2;
        }
        *dest = 0;
      }
    }
    return 0;
  }
  /*
  {
    NCOM::CPropVariant prop;
    if (propID == kpidType)
      string = GetFileType(index);
    else
    {
      HRESULT result = m_ArchiveFolder->GetProperty(index, propID, &prop);
      if (result != S_OK)
      {
        // PrintMessage("GetPropertyValue error");
        return 0;
      }
      string = ConvertPropertyToString(prop, propID, false);
    }
  }
  */
  // const NFind::CFileInfo &aFileInfo = m_Files[index];

  NCOM::CPropVariant prop;
  /*
  bool needRead = true;
  if (propID == kpidSize)
  {
    CComPtr<IFolderGetItemFullSize> getItemFullSize;
    if (_folder.QueryInterface(&getItemFullSize) == S_OK)
    {
      if (getItemFullSize->GetItemFullSize(index, &prop) == S_OK)
        needRead = false;
    }
  }
  if (needRead)
  */

  if (item.cchTextMax < 32)
    return 0;

  if (propID == kpidName)
  {
    if (_folderGetItemName)
    {
      const wchar_t *name = NULL;
      unsigned nameLen = 0;
      _folderGetItemName->GetItemName(realIndex, &name, &nameLen);
      
      if (name)
      {
        unsigned dest = 0;
        unsigned limit = item.cchTextMax - 1;
        
        for (unsigned i = 0; dest < limit;)
        {
          wchar_t c = name[i++];
          if (c == 0)
            break;
          text[dest++] = c;
          
          if (c != ' ')
          {
            if (c != 0x202E) // RLO
              continue;
            text[dest - 1] = '_';
            continue;
          }
          
          if (name[i + 1] != ' ')
            continue;
          
          unsigned t = 2;
          for (; name[i + t] == ' '; t++);
        
          if (t >= 4 && dest + 4 <= limit)
          {
            text[dest++] = '.';
            text[dest++] = '.';
            text[dest++] = '.';
            text[dest++] = ' ';
            i += t;
          }
        }
      
        text[dest] = 0;
        return 0;
      }
    }
  }
  
  if (propID == kpidPrefix)
  {
    if (_folderGetItemName)
    {
      const wchar_t *name = NULL;
      unsigned nameLen = 0;
      _folderGetItemName->GetItemPrefix(realIndex, &name, &nameLen);
      if (name)
      {
        unsigned dest = 0;
        unsigned limit = item.cchTextMax - 1;
        for (unsigned i = 0; dest < limit;)
        {
          wchar_t c = name[i++];
          if (c == 0)
            break;
          text[dest++] = c;
        }
        text[dest] = 0;
        return 0;
      }
    }
  }
  
  HRESULT res = _folder->GetProperty(realIndex, propID, &prop);
  
  if (res != S_OK)
  {
    MyStringCopy(text, L"Error: ");
    // s = UString(L"Error: ") + HResultToMessage(res);
  }
  else if ((prop.vt == VT_UI8 || prop.vt == VT_UI4 || prop.vt == VT_UI2) && IsSizeProp(propID))
  {
    UInt64 v = 0;
    ConvertPropVariantToUInt64(prop, v);
    ConvertIntToDecimalString(v, text);
  }
  else if (prop.vt == VT_BSTR)
  {
    unsigned limit = item.cchTextMax - 1;
    const wchar_t *src = prop.bstrVal;
    unsigned i;
    for (i = 0; i < limit; i++)
    {
      wchar_t c = src[i];
      if (c == 0) break;
      if (c == 0xA) c = ' ';
      if (c == 0xD) c = ' ';
      text[i] = c;
    }
    text[i] = 0;
  }
  else
  {
    char temp[64];
    ConvertPropertyToShortString(temp, prop, propID, false);
    unsigned i;
    unsigned limit = item.cchTextMax - 1;
    for (i = 0; i < limit; i++)
    {
      wchar_t c = (Byte)temp[i];
      if (c == 0)
        break;
      text[i] = c;
    }
    text[i] = 0;
  }
  
  return 0;
}

#ifndef UNDER_CE
extern DWORD g_ComCtl32Version;
#endif


void CContentsView::OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult)
{
  LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

  if (_enableItemChangeNotify)
  {
    if (!_mySelectMode) {
      LPNMLISTVIEW item = pNMLV;
      int index = (int)item->lParam;
      if (index == kParentIndex)
        return;
      bool oldSelected = (item->uOldState & LVIS_SELECTED) != 0;
      bool newSelected = (item->uNewState & LVIS_SELECTED) != 0;
      // Don't change this code. It works only with such check
      if (oldSelected != newSelected) {
        _selectedStatusVector[index] = newSelected;
        if (index != kParentIndex && index != selectedIndex && newSelected) {
          selectedIndex = index;
          CMyComPtr<IFolderFolder> folder = _folder;
          bool preview = false;
          int curIndex = index;
          if (IsPreviewable(index)) {
            if (IsFSFolder()) {
              folder = _folder;
              preview = true;
            }
            else {
              folder = ExtractItemToTemp(index);
              if (folder != NULL) {
                preview = true;
                curIndex = 0;
              }
            }
          }
          m_pDetailView->SetSelItem(folder, curIndex, _currentFolderPrefix, preview);
        }
      }
    }

    Post_Refresh_StatusBar();
    /* 9.26: we don't call Post_Refresh_StatusBar.
    it was very slow if we select big number of files
    and then clead slection by selecting just new file.
    probably it called slow Refresh_StatusBar for each item deselection.
    I hope Refresh_StatusBar still will be called for each key / mouse action.
    */
  }

  *pResult = 0;
}


void CContentsView::OnLvnItemActivate(NMHDR *pNMHDR, LRESULT *pResult)
{
  bool alt = IsKeyDown(VK_MENU);
  bool ctrl = IsKeyDown(VK_CONTROL);
  bool shift = IsKeyDown(VK_SHIFT);
  if (!shift && alt && !ctrl)
    Properties();
  else
    OpenSelectedItems(!shift || alt || ctrl);
}


void CContentsView::OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult)
{
  if (!(_mySelectMode || (_markDeletedItems && _thereAreDeletedItems)))
    return;

  LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
  LPNMLVCUSTOMDRAW lplvcd = reinterpret_cast<LPNMLVCUSTOMDRAW>(pNMCD);

  switch (lplvcd->nmcd.dwDrawStage)
  {
  case CDDS_PREPAINT:
    *pResult = CDRF_NOTIFYITEMDRAW;
    return;

  case CDDS_ITEMPREPAINT:
    /*
    SelectObject(lplvcd->nmcd.hdc,
    GetFontForItem(lplvcd->nmcd.dwItemSpec,
    lplvcd->nmcd.lItemlParam) );
    lplvcd->clrText = GetColorForItem(lplvcd->nmcd.dwItemSpec,
    lplvcd->nmcd.lItemlParam);
    lplvcd->clrTextBk = GetBkColorForItem(lplvcd->nmcd.dwItemSpec,
    lplvcd->nmcd.lItemlParam);
    */
    int realIndex = (int)lplvcd->nmcd.lItemlParam;
    lplvcd->clrTextBk = this->GetBkColor();
    if (_mySelectMode)
    {
      if (realIndex != kParentIndex && _selectedStatusVector[realIndex])
        lplvcd->clrTextBk = RGB(255, 192, 192);
    }

    if (_markDeletedItems && _thereAreDeletedItems)
    {
      if (IsItem_Deleted(realIndex))
        lplvcd->clrText = RGB(255, 0, 0);
    }
    // lplvcd->clrText = RGB(0, 0, 0);
    // result = CDRF_NEWFONT;
    *pResult = CDRF_NOTIFYITEMDRAW;
    return;

    // return false;
    // return true;
    /*
    case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
    if (lplvcd->iSubItem == 0)
    {
    // lplvcd->clrText = RGB(255, 0, 0);
    lplvcd->clrTextBk = RGB(192, 192, 192);
    }
    else
    {
    lplvcd->clrText = RGB(0, 0, 0);
    lplvcd->clrTextBk = RGB(255, 255, 255);
    }
    return true;
    */

    /* At this point, you can change the background colors for the item
    and any subitems and return CDRF_NEWFONT. If the list-view control
    is in report mode, you can simply return CDRF_NOTIFYSUBITEMREDRAW
    to customize the item's subitems individually */
  }
  *pResult = 0;
}
