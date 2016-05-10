// PanelSelect.cpp

#include "StdAfx.h"

#include "CPP/Common/StringConvert.h"
#include "CPP/Common/Wildcard.h"

#include "ComboDialog.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"

#include "FMUtils.h"

#include "ContentsView.h"
#include "Messages.h"

void CContentsView::OnShiftSelectMessage()
{
  if (!_mySelectMode)
    return;
  int focusedItem = this->GetFocusedItem();
  if (focusedItem < 0)
    return;
  if (!_selectionIsDefined)
    return;
  int startItem = MyMin(focusedItem, _prevFocusedItem);
  int finishItem = MyMax(focusedItem, _prevFocusedItem);

  int numItems = this->GetItemCount();
  for (int i = 0; i < numItems; i++)
  {
    int realIndex = GetRealItemIndex(i);
    if (realIndex == kParentIndex)
      continue;
    if (i >= startItem && i <= finishItem)
      if (_selectedStatusVector[realIndex] != _selectMark)
      {
        _selectedStatusVector[realIndex] = _selectMark;
        this->RedrawItems(i, i);
      }
  }

  _prevFocusedItem = focusedItem;
}

void CContentsView::OnArrowWithShift()
{
  if (!_mySelectMode)
    return;
  int focusedItem = this->GetFocusedItem();
  if (focusedItem < 0)
    return;
  int realIndex = GetRealItemIndex(focusedItem);

  if (_selectionIsDefined)
  {
    if (realIndex != kParentIndex)
      _selectedStatusVector[realIndex] = _selectMark;
  }
  else
  {
    if (realIndex == kParentIndex)
    {
      _selectionIsDefined = true;
      _selectMark = true;
    }
    else
    {
      _selectionIsDefined = true;
      _selectMark = !_selectedStatusVector[realIndex];
      _selectedStatusVector[realIndex] = _selectMark;
    }
  }

  _prevFocusedItem = focusedItem;
  PostMessage(kShiftSelectMessage);
  this->RedrawItems(focusedItem, focusedItem);
}

void CContentsView::OnInsert()
{
  /*
  const int kState = CDIS_MARKED; // LVIS_DROPHILITED;
  UINT state = (_listView.GetItemState(focusedItem, LVIS_CUT) == 0) ?
      LVIS_CUT : 0;
  _listView.SetItemState(focusedItem, state, LVIS_CUT);
  // _listView.SetItemState_Selected(focusedItem);

  */
  int focusedItem = this->GetFocusedItem();
  if (focusedItem < 0)
    return;
  int realIndex = GetRealItemIndex(focusedItem);
  bool isSelected = !_selectedStatusVector[realIndex];
  if (realIndex != kParentIndex)
    _selectedStatusVector[realIndex] = isSelected;
  
  if (!_mySelectMode)
    this->SetItemState(focusedItem, isSelected ? LVIS_SELECTED : 0, LVIS_SELECTED);

  this->RedrawItems(focusedItem, focusedItem);

  int nextIndex = focusedItem + 1;
  if (nextIndex < this->GetItemCount())
  {
    this->SetItemState(nextIndex, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
    this->EnsureVisible(nextIndex, false);
  }
}

/*
void CPanel::OnUpWithShift()
{
  int focusedItem = _listView.GetFocusedItem();
  if (focusedItem < 0)
    return;
  int index = GetRealItemIndex(focusedItem);
  _selectedStatusVector[index] = !_selectedStatusVector[index];
  _listView.RedrawItem(index);
}

void CPanel::OnDownWithShift()
{
  int focusedItem = _listView.GetFocusedItem();
  if (focusedItem < 0)
    return;
  int index = GetRealItemIndex(focusedItem);
  _selectedStatusVector[index] = !_selectedStatusVector[index];
  _listView.RedrawItem(index);
}
*/

void CContentsView::UpdateSelection()
{
  if (!_mySelectMode)
  {
    bool enableTemp = _enableItemChangeNotify;
    _enableItemChangeNotify = false;
    int numItems = this->GetItemCount();
    for (int i = 0; i < numItems; i++)
    {
      int realIndex = GetRealItemIndex(i);
      if (realIndex != kParentIndex)
        this->SetItemState(i, _selectedStatusVector[realIndex] ? LVIS_SELECTED : 0, LVIS_SELECTED);
    }
    _enableItemChangeNotify = enableTemp;
  }
  if (this->GetItemCount() > 0)
    this->RedrawItems(0, this->GetItemCount() - 1);
}


void CContentsView::SelectSpec(bool selectMode)
{
  CComboDialog dlg;
  LangString(selectMode ? IDS_SELECT : IDS_DESELECT, dlg.Title );
  LangString(IDS_SELECT_MASK, dlg.Static);
  dlg.Value = L'*';
  if (dlg.DoModal() != IDOK)
    return;
  const UString &mask = dlg.Value;
  FOR_VECTOR (i, _selectedStatusVector)
    if (DoesWildcardMatchName(mask, GetItemName(i)))
       _selectedStatusVector[i] = selectMode;
  UpdateSelection();
}

void CContentsView::SelectByType(bool selectMode)
{
  int focusedItem = this->GetFocusedItem();
  if (focusedItem < 0)
    return;
  int realIndex = GetRealItemIndex(focusedItem);
  UString name = GetItemName(realIndex);
  bool isItemFolder = IsItem_Folder(realIndex);

  if (isItemFolder)
  {
    FOR_VECTOR (i, _selectedStatusVector)
      if (IsItem_Folder(i) == isItemFolder)
        _selectedStatusVector[i] = selectMode;
  }
  else
  {
    int pos = name.ReverseFind_Dot();
    if (pos < 0)
    {
      FOR_VECTOR (i, _selectedStatusVector)
        if (IsItem_Folder(i) == isItemFolder && GetItemName(i).ReverseFind_Dot() < 0)
          _selectedStatusVector[i] = selectMode;
    }
    else
    {
      UString mask = L'*';
      mask += name.Ptr(pos);
      FOR_VECTOR (i, _selectedStatusVector)
        if (IsItem_Folder(i) == isItemFolder && DoesWildcardMatchName(mask, GetItemName(i)))
          _selectedStatusVector[i] = selectMode;
    }
  }

  UpdateSelection();
}

void CContentsView::SelectAll(bool selectMode)
{
  FOR_VECTOR (i, _selectedStatusVector)
    _selectedStatusVector[i] = selectMode;
  UpdateSelection();
}

void CContentsView::InvertSelection()
{
  if (!_mySelectMode)
  {
    unsigned numSelected = 0;
    FOR_VECTOR (i, _selectedStatusVector)
      if (_selectedStatusVector[i])
        numSelected++;
    if (numSelected == 1)
    {
      int focused = this->GetFocusedItem();
      if (focused >= 0)
      {
        int realIndex = GetRealItemIndex(focused);
        if (realIndex >= 0)
          if (_selectedStatusVector[realIndex])
            _selectedStatusVector[realIndex] = false;
      }
    }
  }
  FOR_VECTOR (i, _selectedStatusVector)
    _selectedStatusVector[i] = !_selectedStatusVector[i];
  UpdateSelection();
}

void CContentsView::KillSelection()
{
  SelectAll(false);
  if (!_mySelectMode)
  {
    int focused = this->GetFocusedItem();
    if (focused >= 0)
    {
      // CPanel::OnItemChanged notify for LVIS_SELECTED change doesn't work here. Why?
      // so we change _selectedStatusVector[realIndex] here.
      int realIndex = GetRealItemIndex(focused);
      if (realIndex != kParentIndex)
        _selectedStatusVector[realIndex] = true;
      this->SetItemState(focused, LVIS_SELECTED, LVIS_SELECTED);
    }
  }
}


void CContentsView::OnNMClick(NMHDR *pNMHDR, LRESULT *pResult)
{
  LPNMITEMACTIVATE itemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
  // we need SetFocusToList, if we drag-select items from other panel.
  this->SetFocus();
  Post_Refresh_StatusBar();
  *pResult = 0;

  if (!_mySelectMode)
    return;

  if (itemActivate->hdr.hwndFrom != HWND(*this))
    return;
  // It will work only for Version 4.71 (IE 4);
  int indexInList = itemActivate->iItem;
  if (indexInList < 0)
    return;

  #ifndef UNDER_CE
  if ((itemActivate->uKeyFlags & LVKF_SHIFT) != 0)
  {
    // int focusedIndex = _listView.GetFocusedItem();
    int focusedIndex = _startGroupSelect;
    if (focusedIndex < 0)
      return;
    int startItem = MyMin(focusedIndex, indexInList);
    int finishItem = MyMax(focusedIndex, indexInList);

    int numItems = this->GetItemCount();
    for (int i = 0; i < numItems; i++)
    {
      int realIndex = GetRealItemIndex(i);
      if (realIndex == kParentIndex)
        continue;
      bool selected = (i >= startItem && i <= finishItem);
      if (_selectedStatusVector[realIndex] != selected)
      {
        _selectedStatusVector[realIndex] = selected;
        this->RedrawItems(i, i);
      }
    }
  }
  else
  #endif
  {
    _startGroupSelect = indexInList;

    #ifndef UNDER_CE
    if ((itemActivate->uKeyFlags & LVKF_CONTROL) != 0)
    {
      int realIndex = GetRealItemIndex(indexInList);
      if (realIndex != kParentIndex)
      {
        _selectedStatusVector[realIndex] = !_selectedStatusVector[realIndex];
        this->RedrawItems(indexInList, indexInList);
      }
    }
    #endif
  }

  return;
}
