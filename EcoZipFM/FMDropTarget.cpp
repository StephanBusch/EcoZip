// FMDrapTarget.cpp : implementation file
//

#include "stdafx.h"
#include "EcoZipFM.h"
#include "FMDropTarget.h"


//********************************************************************
// If you don't have a recent Platform SDK installed, you'll get linker
// errors on CLSID_DragDropHelper and IID_IDropTargetHelper, and you
// won't have the definition of IDropTargetHelper.  Uncomment the
// following stuff to fix that.
/*
struct IDropTargetHelper : public IUnknown
{
    // IUnknown methods
    STDMETHOD (QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG, AddRef) ( THIS ) PURE;
    STDMETHOD_(ULONG, Release) ( THIS ) PURE;

    // IDropTargetHelper
    STDMETHOD (DragEnter)(THIS_ HWND hwndTarget, IDataObject* pDataObject,
                          POINT* ppt, DWORD dwEffect) PURE;
    STDMETHOD (DragLeave)(THIS) PURE;
    STDMETHOD (DragOver)(THIS_ POINT* ppt, DWORD dwEffect) PURE;
    STDMETHOD (Drop)(THIS_ IDataObject* pDataObject, POINT* ppt,
                     DWORD dwEffect) PURE;
    STDMETHOD (Show)(THIS_ BOOL fShow) PURE;
};

// {4657278A-411B-11d2-839A-00C04FD918D0}
extern "C" const GUID __declspec(selectany) CLSID_DragDropHelper = 
    { 0x4657278a, 0x411b, 0x11d2, { 0x83, 0x9a, 0x0, 0xc0, 0x4f, 0xd9, 0x18, 0xd0 }};

// {4657278B-411B-11d2-839A-00C04FD918D0}
extern "C" const GUID __declspec(selectany) IID_IDropTargetHelper = 
    { 0x4657278b, 0x411b, 0x11d2, { 0x83, 0x9a, 0x0, 0xc0, 0x4f, 0xd9, 0x18, 0xd0 }};
*/
//********************************************************************


// CFMDropTarget

CFMDropTarget::CFMDropTarget(CDialog *pMainWnd) : m_pMainWnd(pMainWnd),
                                                  m_piDropHelper(NULL),
                                                  m_bUseDnDHelper(false)
{
	m_uCustomClipbrdFormat =
		RegisterClipboardFormat(_T("EcoZip_B723AE15-06AA-41DB-BD34-905F4FBE5DB2"));

    // Create an instance of the shell DnD helper object.

    if ( SUCCEEDED( CoCreateInstance ( CLSID_DragDropHelper, NULL, 
                                       CLSCTX_INPROC_SERVER,
                                       IID_IDropTargetHelper, 
                                       (void**) &m_piDropHelper ) ))
    {
        m_bUseDnDHelper = true;
    }
}

CFMDropTarget::~CFMDropTarget()
{
    if ( NULL != m_piDropHelper )
        m_piDropHelper->Release();
}


BEGIN_MESSAGE_MAP(CFMDropTarget, COleDropTarget)
END_MESSAGE_MAP()



// CFMDropTarget message handlers

DROPEFFECT CFMDropTarget::OnDragEnter(CWnd* pWnd, COleDataObject* pDataObject,
	DWORD dwKeyState, CPoint point)
{
	DROPEFFECT dwEffect = DROPEFFECT_NONE;

	// Check for our own custom clipboard format in the data object.  If it's
	// present, then the DnD was initiated from our own window, and we won't
	// accept the drop.
	// If it's not present, then we check for CF_HDROP data in the data object.

	if (NULL == pDataObject->GetGlobalData(m_uCustomClipbrdFormat))
	{
		// Look for CF_HDROP data in the data object, and accept the drop if
		// it's there.

		if (NULL != pDataObject->GetGlobalData(CF_HDROP))
			dwEffect = DROPEFFECT_COPY;
	}

	// Call the DnD helper.

	if (m_bUseDnDHelper)
	{
		// The DnD helper needs an IDataObject interface, so get one from
		// the COleDataObject.  Note that the FALSE param means that
		// GetIDataObject will not AddRef() the returned interface, so 
		// we do not Release() it.

		IDataObject* piDataObj = pDataObject->GetIDataObject(FALSE);

		m_piDropHelper->DragEnter(pWnd->GetSafeHwnd(), piDataObj,
			&point, dwEffect);
	}

	return dwEffect;
}

DROPEFFECT CFMDropTarget::OnDragOver(CWnd* pWnd, COleDataObject* pDataObject,
	DWORD dwKeyState, CPoint point)
{
	DROPEFFECT dwEffect = DROPEFFECT_NONE;

	// Check for our own custom clipboard format in the data object.  If it's
	// present, then the DnD was initiated from our own window, and we won't
	// accept the drop.
	// If it's not present, then we check for CF_HDROP data in the data object.

	if (NULL == pDataObject->GetGlobalData(m_uCustomClipbrdFormat))
	{
		// Look for CF_HDROP data in the data object, and accept the drop if
		// it's there.

		if (NULL != pDataObject->GetGlobalData(CF_HDROP))
			dwEffect = DROPEFFECT_COPY;
	}

	// Call the DnD helper.

	if (m_bUseDnDHelper)
	{
		m_piDropHelper->DragOver(&point, dwEffect);
	}

	return dwEffect;
}

BOOL CFMDropTarget::OnDrop(CWnd* pWnd, COleDataObject* pDataObject,
	DROPEFFECT dropEffect, CPoint point)
{
	BOOL bRet;

	// Read the CF_HDROP data and put the files in the main window's list.

	bRet = ReadHdropData(pDataObject);

	// Call the DnD helper.

	if (m_bUseDnDHelper)
	{
		// The DnD helper needs an IDataObject interface, so get one from
		// the COleDataObject.  Note that the FALSE param means that
		// GetIDataObject will not AddRef() the returned interface, so 
		// we do not Release() it.

		IDataObject* piDataObj = pDataObject->GetIDataObject(FALSE);

		m_piDropHelper->Drop(piDataObj, &point, dropEffect);
	}

	return bRet;
}

void CFMDropTarget::OnDragLeave(CWnd* pWnd)
{
	if (m_bUseDnDHelper)
	{
		m_piDropHelper->DragLeave();
	}
}

// ReadHdropData() reads CF_HDROP data from the passed-in data object, and 
// puts all dropped files/folders into the main window's list control.
BOOL CFMDropTarget::ReadHdropData(COleDataObject* pDataObject)
{
	HGLOBAL     hg;
	HDROP       hdrop;

	hg = pDataObject->GetGlobalData(CF_HDROP);

	if (NULL == hg)
	{
		return FALSE;
	}

	hdrop = (HDROP)GlobalLock(hg);

	if (NULL == hdrop)
	{
		GlobalUnlock(hg);
		return FALSE;
	}

	LRESULT lResult = m_pMainWnd->SendMessage(WM_DROPFILES, (WPARAM)hdrop);

	GlobalUnlock(hg);

	return lResult == 0;

// 	HGLOBAL     hg;
// 	HDROP       hdrop;
// 	UINT        uNumFiles;
// 	TCHAR       szNextFile[MAX_PATH];
// 	SHFILEINFO  rFileInfo;
// 	LVFINDINFO  rlvFind = { LVFI_STRING, szNextFile };
// 	LVITEM      rItem;
// 	int         nIndex = m_pList->GetItemCount();
// 	HANDLE      hFind;
// 	WIN32_FIND_DATA rFind;
// 	TCHAR       szFileLen[64];
// 
// 	// Get the HDROP data from the data object.
// 
// 	hg = pDataObject->GetGlobalData(CF_HDROP);
// 
// 	if (NULL == hg)
// 	{
// 		return FALSE;
// 	}
// 
// 	hdrop = (HDROP)GlobalLock(hg);
// 
// 	if (NULL == hdrop)
// 	{
// 		GlobalUnlock(hg);
// 		return FALSE;
// 	}
// 
// 	// Get the # of files being dropped.
// 
// 	uNumFiles = DragQueryFile(hdrop, -1, NULL, 0);
// 
// 	for (UINT uFile = 0; uFile < uNumFiles; uFile++)
// 	{
// 		// Get the next filename from the HDROP info.
// 
// 		if (DragQueryFile(hdrop, uFile, szNextFile, MAX_PATH) > 0)
// 		{
// 			// If the filename is already in the list, skip it.
// 
// 			if (-1 != m_pList->FindItem(&rlvFind, -1))
// 				continue;
// 
// 			// Get the index of the file's icon in the system image list and
// 			// it's type description.
// 
// 			SHGetFileInfo(szNextFile, 0, &rFileInfo, sizeof(rFileInfo),
// 				SHGFI_SYSICONINDEX | SHGFI_ATTRIBUTES |
// 				SHGFI_TYPENAME);
// 
// 			// Set up an LVITEM for the file we're about to insert.
// 
// 			ZeroMemory(&rItem, sizeof(LVITEM));
// 
// 			rItem.mask = LVIF_IMAGE | LVIF_TEXT;
// 			rItem.iItem = nIndex;
// 			rItem.pszText = szNextFile;
// 			rItem.iImage = rFileInfo.iIcon;
// 
// 			// If the file has the hidden attribute set, its attributes in
// 			// the SHFILEINFO struct will include SFGAO_GHOSTED.  We'll mark
// 			// the item as "cut" in our list to give it the same ghosted look.
// 
// 			if (rFileInfo.dwAttributes & SFGAO_GHOSTED)
// 			{
// 				rItem.mask |= LVIF_STATE;
// 				rItem.state = rItem.stateMask = LVIS_CUT;
// 			}
// 
// 			// Insert it into the list!
// 
// 			m_pList->InsertItem(&rItem);
// 
// 			// Set column #1 to the file's type description.
// 
// 			m_pList->SetItemText(nIndex, 1, rFileInfo.szTypeName);
// 
// 			// Get the file size, and put that in column #2.
// 
// 			hFind = FindFirstFile(szNextFile, &rFind);
// 
// 			if (INVALID_HANDLE_VALUE != hFind)
// 			{
// 				StrFormatByteSize(rFind.nFileSizeLow, szFileLen, 64);
// 				FindClose(hFind);
// 			}
// 
// 			m_pList->SetItemText(nIndex, 2, szFileLen);
// 
// 			nIndex++;
// 		}
// 	}   // end for
// 
// 	GlobalUnlock(hg);
// 
// 	// Resize columns so all text is visible.
// 
// 	m_pList->SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
// 	m_pList->SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);
// 	m_pList->SetColumnWidth(2, LVSCW_AUTOSIZE_USEHEADER);
// 
// 	m_pList->EnsureVisible(nIndex - 1, FALSE);

	return TRUE;
}
