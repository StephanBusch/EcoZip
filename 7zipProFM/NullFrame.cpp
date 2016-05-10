// NullFrame.cpp : implementation file
//

#include "stdafx.h"
#include "7zipProFM.h"
#include "NullFrame.h"


// CNullFrame

IMPLEMENT_DYNCREATE(CNullFrame, CFrameWnd)

CNullFrame::CNullFrame()
{

}

CNullFrame::~CNullFrame()
{
}


BEGIN_MESSAGE_MAP(CNullFrame, CFrameWnd)
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()


// CNullFrame message handlers


BOOL CNullFrame::OnEraseBkgnd(CDC* pDC)
{
	// TODO: Add your message handler code here and/or call default

	return TRUE;// CFrameWnd::OnEraseBkgnd(pDC);
}


BOOL CNullFrame::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	if (CFrameWnd::OnNotify(wParam, lParam, pResult))
		return TRUE;

	// route commands to the splitter to the parent frame window
	*pResult = GetParent()->SendMessage(WM_NOTIFY, wParam, lParam);
	return TRUE;
}
