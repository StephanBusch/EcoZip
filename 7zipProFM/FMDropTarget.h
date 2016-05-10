//////////////////////////////////////////////////////////////////////
//
// FMDropTarget.h: interface for the CFMDropTarget class.
//
//////////////////////////////////////////////////////////////////////

#pragma once


//////////////////////////////////////////////////////////////////////
// CFMDropTarget implements a drop target for the main dialog.
// I was also messing around with the IDropTargetHelper stuff in 
// Win2K, which lets the shell draw the nifty shaded drag image when
// you drag into the dialog.  If you're using 9x or NT 4, that stuff
// is disabled.
 
struct IDropTargetHelper;   // forward reference, in case the latest PSDK isn't installed.

class CFMDropTarget : public COleDropTarget
{
public:
	CFMDropTarget(CDialog *pMainWnd);
	virtual ~CFMDropTarget();

	DROPEFFECT OnDragEnter(CWnd* pWnd, COleDataObject* pDataObject,
		DWORD dwKeyState, CPoint point);

	DROPEFFECT OnDragOver(CWnd* pWnd, COleDataObject* pDataObject,
		DWORD dwKeyState, CPoint point);

	BOOL OnDrop(CWnd* pWnd, COleDataObject* pDataObject,
		DROPEFFECT dropEffect, CPoint point);

	void OnDragLeave(CWnd* pWnd);

protected:
	DECLARE_MESSAGE_MAP()

protected:
	CDialog *          m_pMainWnd;
    IDropTargetHelper* m_piDropHelper;
    bool               m_bUseDnDHelper;
	UINT               m_uCustomClipbrdFormat;

    BOOL ReadHdropData ( COleDataObject* pDataObject );
};


