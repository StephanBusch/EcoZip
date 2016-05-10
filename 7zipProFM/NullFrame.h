#pragma once


// CNullFrame frame

class CNullFrame : public CFrameWnd
{
	DECLARE_DYNCREATE(CNullFrame)
public:
	CNullFrame();           // protected constructor used by dynamic creation
	virtual ~CNullFrame();

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
};


