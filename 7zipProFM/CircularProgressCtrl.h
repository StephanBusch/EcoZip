#pragma once

class CCircularProgressCtrl :
	public CProgressCtrl
{
public:
	CCircularProgressCtrl();
	~CCircularProgressCtrl();

	void LoadBitmaps(UINT nIDBkgnd, UINT nID1, UINT nID2 = 0,
		UINT nIDNumL = 0, UINT nIDNumS = 0);
	void LoadPNGBitmaps(UINT nIDBkgnd, UINT nID1, UINT nID2 = 0,
		UINT nIDNumL = 0, UINT nIDNumS = 0);
	BOOL IsTransparent() const { return m_bTransparent; }
	void SetTransparent(BOOL bTransparent);
	int SetPos(_In_ int nPos);
	int GetPos1() const { return GetPos(); }
	int SetPos1(_In_ int nPos) { return SetPos(nPos); }
	int GetPos2() const;
	int SetPos2(_In_ int nPos);
	void SetBindWindow(HWND hWnd);
	void SetProgressState(TBPFLAG tbpFlags) { m_tbpFlags = tbpFlags; }
	void ResetBackground();
	void CCircularProgressCtrl::SetBk(CDC* pDC);
	void RemoveBorder();
	void SetState(TBPFLAG tbpFlags);
	void EnableTimer(BOOL bEnable) { m_bTimer = bEnable; }
	void EnableSubbar(BOOL bEnable) { m_bSubbur = bEnable; }

protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	int GetCurPos1() const;
	int SetCurPos1(_In_ int nPos);
	int GetCurPos2() const;
	int SetCurPos2(_In_ int nPos);

	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	DECLARE_MESSAGE_MAP()

protected:
	void DrawParentBackground(CDC *pDC);
	void DrawRatio(CDC *pDC, CRect &rt, CBitmap &bmp, int nPos, BOOL displayUnit = TRUE);
	void DrawDigit(CDC *pDC, CRect &rt, CBitmap &bmp, int ch);
	static CRect PrepareRect(CRect &rt1, int w1, int h1, int w2, int h2);
	static int GetDigitCount(int val, int base = 10);

	int m_nPos2;
	int m_nCurPos1, m_nCurPos2;
	int m_nStep1, m_nStep2;

	BOOL m_bTimer;
	BOOL m_bSubbur;
	BOOL m_bTransparent;

	CDC m_dcBkgnd;
	CBitmap m_bmpParent;

	CBitmap m_bmpBkgnd;
	CBitmap m_bmpContents1;
	BOOL m_bBkgndPremultiplied;
	CBitmap m_bmpData1;
	CBitmap m_bmpContents2;
	CBitmap m_bmpData2;
	CBitmap m_bmpNumL;
	CBitmap m_bmpNumS;

	CComPtr<ITaskbarList3> m_pTaskbarList;
	HWND m_hWndBind;
	TBPFLAG m_tbpFlags;
};

