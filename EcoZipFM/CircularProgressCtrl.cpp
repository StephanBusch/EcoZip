#include "stdafx.h"
#include "CircularProgressCtrl.h"
#include "ImageUtils.h"

#define _USE_MATH_DEFINES
#include <cmath>

#define BMP_DIGITS 11
#define BMP_UNIT 10
#define BMP_FIRST_NUMBER 0
#define BMP_LAST_NUMBER 9
#define PROGRESS_STEPS 10

static const int kTimerID = 10;
static const int kTimerElapse = 100;

CCircularProgressCtrl::CCircularProgressCtrl()
{
	m_nPos2 = 0;
	m_nCurPos1 = m_nCurPos2 = 0;
	m_nStep1 = m_nStep2 = 0;
	m_bTimer = FALSE;
	m_bSubbur = FALSE;
	m_bTransparent = FALSE;
	m_bBkgndPremultiplied = FALSE;
	m_tbpFlags = TBPF_NORMAL;
}


CCircularProgressCtrl::~CCircularProgressCtrl()
{
}

BEGIN_MESSAGE_MAP(CCircularProgressCtrl, CProgressCtrl)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_DESTROY()
	ON_WM_TIMER()
END_MESSAGE_MAP()


void CCircularProgressCtrl::LoadBitmaps(
	UINT nIDBkgnd, UINT nID1, UINT nID2,
	UINT nIDNumL, UINT nIDNumS)
{
	m_bmpBkgnd.LoadBitmap(nIDBkgnd);
	m_bmpContents1.LoadBitmap(nID1);
	if (nID2 != 0)
		m_bmpContents2.LoadBitmap(nID2);
	m_bmpNumL.LoadBitmap(nIDNumL);
	m_bmpNumS.LoadBitmap(nIDNumS);
	Invalidate();
}


void CCircularProgressCtrl::LoadPNGBitmaps(
	UINT nIDBkgnd, UINT nID1, UINT nID2,
	UINT nIDNumL, UINT nIDNumS)
{
	CImageUtils::LoadBitmapFromPNG(nIDBkgnd, m_bmpBkgnd);
	CImageUtils::LoadBitmapFromPNG(nID1, m_bmpContents1);
	if (nID2 != 0)
		CImageUtils::LoadBitmapFromPNG(nID2, m_bmpContents2);
	CImageUtils::LoadBitmapFromPNG(nIDNumL, m_bmpNumL);
	CImageUtils::LoadBitmapFromPNG(nIDNumS, m_bmpNumS);
	CDC *pDC = GetDC();
	CImageUtils::PremultiplyBitmapAlpha(*pDC, m_bmpNumL, 1.0, NULL);
  CImageUtils::PremultiplyBitmapAlpha(*pDC, m_bmpNumS, 1.0, NULL);
	ReleaseDC(pDC);
	Invalidate();
}


void CCircularProgressCtrl::SetTransparent(BOOL bTransparent)
{
	m_bTransparent = bTransparent;
	Invalidate();
}


int CCircularProgressCtrl::SetPos(_In_ int nPos)
{
	int rlt = CProgressCtrl::SetPos(nPos);

	if (m_bTimer && GetCurPos1() != GetPos1()) {
		SetTimer(kTimerID, kTimerElapse, NULL);
		m_nStep1 = (GetPos1() - GetCurPos1()) / PROGRESS_STEPS;
	}
	else if (m_pTaskbarList != NULL) {
		int nLower, nUpper;
		GetRange(nLower, nUpper);
		int range = nUpper - nLower;
		int pos = GetPos1() - nLower;
		m_pTaskbarList->SetProgressValue(m_hWndBind, pos, range);
		m_pTaskbarList->SetProgressState(m_hWndBind, m_tbpFlags);
	}

	return rlt;
}


int CCircularProgressCtrl::GetPos2() const
{
	int nLower, nUpper;
	GetRange(nLower, nUpper);
	int nPos1 = GetPos1();
	int nPos2 = m_nPos2;
	nPos2 = max(nPos2, nLower);
	nPos2 = min(nPos2, nPos1);
	return nPos2;
}


int CCircularProgressCtrl::SetPos2(_In_ int nPos)
{
	int nPrev = GetPos2();
	int nLower, nUpper;
	GetRange(nLower, nUpper);
	int nPos1 = GetPos1();
	nPos = max(nPos, nLower);
	nPos = min(nPos, nPos1);
	m_nPos2 = nPos;

	if (m_bTimer && GetCurPos2() != GetPos2()) {
		SetTimer(kTimerID, kTimerElapse, NULL);
		m_nStep2 = (GetPos2() - GetCurPos2()) / PROGRESS_STEPS;
	}

	return nPrev;
}


int CCircularProgressCtrl::GetCurPos1() const
{
	int nLower, nUpper;
	GetRange(nLower, nUpper);
	int nPos = max(m_nCurPos1, nLower);
	nPos = min(nPos, nUpper);
	return nPos;
}


int CCircularProgressCtrl::SetCurPos1(_In_ int nPos)
{
	int nPrev = m_nCurPos1;
	int nLower, nUpper;
	GetRange(nLower, nUpper);
	nPos = max(nPos, nLower);
	nPos = min(nPos, nUpper);
	m_nCurPos1 = nPos;
	return nPrev;
}


int CCircularProgressCtrl::GetCurPos2() const
{
	int nLower, nUpper;
	GetRange(nLower, nUpper);
	int nPos = m_nCurPos2;
	nPos = max(nPos, nLower);
	nPos = min(nPos, m_nCurPos1);
	return nPos;
}


int CCircularProgressCtrl::SetCurPos2(_In_ int nPos)
{
	int nPrev = m_nCurPos2;
	int nLower, nUpper;
	GetRange(nLower, nUpper);
	nPos = max(nPos, nLower);
	nPos = min(nPos, m_nCurPos1);
	m_nCurPos2 = nPos;
	return nPrev;
}


void CCircularProgressCtrl::SetBindWindow(HWND hWnd)
{
	m_hWndBind = hWnd;
	CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_ITaskbarList3, (void**)&m_pTaskbarList);
	if (m_pTaskbarList)
		m_pTaskbarList->HrInit();
	SetPos(GetPos());
}


CRect CCircularProgressCtrl::PrepareRect(CRect &rt1, int w1, int h1, int w2, int h2)
{
	CRect rt2 = rt1;
	if (w2 < w1) {
		int gap = (w1 - w2) / 2 * rt1.Width() / w1;
		rt2.left += gap;
		rt2.right -= gap;
	}
	if (h2 < h1) {
		int gap = (h1 - h2) / 2 * rt1.Height() / h1;
		rt2.top += gap;
		rt2.bottom -= gap;
	}
	return rt2;
}


int CCircularProgressCtrl::GetDigitCount(int val, int base)
{
	int nCount = 0;
	while (val > 0) {
		nCount++;
		val /= base;
	}
	if (nCount == 0)
		nCount++;
	return nCount;
}


BOOL CCircularProgressCtrl::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.dwExStyle &= ~WS_EX_STATICEDGE;
	return CProgressCtrl::PreCreateWindow(cs);
}


void CCircularProgressCtrl::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	CRect rt, rt1, rt2;
	GetClientRect(&rt);

	BITMAP bm;
	m_bmpBkgnd.GetBitmap(&bm);
	BITMAP bm1, bm2;
	if (m_bmpContents1.m_hObject != NULL) {
		m_bmpContents1.GetBitmap(&bm1);
		rt1 = PrepareRect(rt, bm.bmWidth, bm.bmHeight, bm1.bmWidth, bm1.bmHeight);
	}
	if (m_bmpContents2.m_hObject != NULL) {
		m_bmpContents2.GetBitmap(&bm2);
		rt2 = PrepareRect(rt, bm.bmWidth, bm.bmHeight, bm2.bmWidth, bm2.bmHeight);
	}

#if 1
	CDC dcClient;
	CBitmap bmpClient;
	bmpClient.CreateCompatibleBitmap(&dc, rt.Width(), rt.Height());
	dcClient.CreateCompatibleDC(&dc);
	dcClient.SelectObject(&bmpClient);
	dcClient.BitBlt(rt.left, rt.top, rt.Width(), rt.Height(), &dc, 0, 0, SRCCOPY);
#else
 	CDC &dcClient = dc;
#endif

	if (!m_bBkgndPremultiplied) {
		CImageUtils::PremultiplyBitmapAlpha(dcClient, m_bmpBkgnd, 1.0, NULL);
		m_bBkgndPremultiplied = TRUE;
	}

	int nUpper, nLower;
	GetRange(nLower, nUpper);
	int nPos1 = GetCurPos1(), nPos2 = GetCurPos2();
	float fRate1 = 0.f, fRate2 = 0.f;
	if (nUpper > nLower) {
		fRate1 = (float)(nPos1 - nLower) / (nUpper - nLower);
		fRate2 = (float)(nPos2 - nLower) / (nUpper - nLower);
	}
	if (m_bmpContents1.m_hObject != NULL && m_bmpData1.m_hObject == NULL)
		m_bmpData1.CreateCompatibleBitmap(&dcClient, bm.bmWidth, bm.bmHeight);
	if (m_bmpContents2.m_hObject != NULL && m_bmpData2.m_hObject == NULL)
		m_bmpData2.CreateCompatibleBitmap(&dcClient, bm.bmWidth, bm.bmHeight);

	DrawParentBackground(&dcClient);
	if (!m_bTransparent) {
		CProgressCtrl::OnEraseBkgnd(&dcClient);
	}

	BLENDFUNCTION bf;
	memset(&bf, 0, sizeof(bf));
	bf.BlendOp = AC_SRC_OVER;
	bf.BlendFlags = 0;
	bf.SourceConstantAlpha = 255;
	bf.AlphaFormat = AC_SRC_ALPHA;

	CDC  dcMem;
	dcMem.CreateCompatibleDC(&dc);
	CBitmap *pOldBitmap = dcMem.SelectObject(&m_bmpBkgnd);
	dcClient.AlphaBlend(rt.left, rt.top, rt.Width(), rt.Height(),
		&dcMem, 0, 0, bm.bmWidth, bm.bmHeight, bf);

	if (m_bmpContents1.m_hObject != NULL && nPos1 > 0) {
		CImageUtils::PremultiplyBitmapAlpha(dcClient, m_bmpContents1, fRate1, m_bmpData1);
		dcMem.SelectObject(&m_bmpData1);
		dcClient.AlphaBlend(rt1.left, rt1.top, rt1.Width(), rt1.Height(),
			&dcMem, 0, 0, bm1.bmWidth, bm1.bmHeight, bf);
		if (m_bSubbur && m_bmpContents2.m_hObject != NULL && nPos2 > 0) {
			CImageUtils::PremultiplyBitmapAlpha(dcClient, m_bmpContents2, fRate2, m_bmpData2);
			dcMem.SelectObject(&m_bmpData2);
			dcClient.AlphaBlend(rt2.left, rt2.top, rt2.Width(), rt2.Height(),
				&dcMem, 0, 0, bm2.bmWidth, bm2.bmHeight, bf);
		}
	}

	CRect rtRatio;
	rtRatio = CRect(
		(int)(rt.left + rt.Width() * 0.2), (int)(rt.top + rt.Height() * 0.32),
		(int)(rt.left + rt.Width() * 0.8), (int)(rt.top + rt.Height() * 0.52));
	DrawRatio(&dcClient, rtRatio, m_bmpNumL, (int)(fRate1 * 100.0f));
	rtRatio = CRect(
		(int)(rt.left + rt.Width() * 0.3), (int)(rt.top + rt.Height() * 0.55),
		(int)(rt.left + rt.Width() * 0.7), (int)(rt.top + rt.Height() * 0.72));
  if (fRate1 != 0)
    fRate2 /= fRate1;
	DrawRatio(&dcClient, rtRatio, m_bmpNumS, (int)round(fRate2 * 100.0f));

	dc.BitBlt(rt.left, rt.top, rt.Width(), rt.Height(), &dcClient, 0, 0, SRCCOPY);

	dcMem.SelectObject(pOldBitmap);
}


void CCircularProgressCtrl::DrawParentBackground(CDC* pDC)
{
	CClientDC clDC(GetParent());
	CRect rect;
	CRect rect1;

	GetClientRect(rect);

	GetWindowRect(rect1);
	GetParent()->ScreenToClient(rect1);

	if (m_dcBkgnd.m_hDC == NULL)
	{
		m_dcBkgnd.CreateCompatibleDC(&clDC);
		m_bmpParent.CreateCompatibleBitmap(&clDC, rect.Width(), rect.Height());
		m_dcBkgnd.SelectObject(&m_bmpParent);
		m_dcBkgnd.BitBlt(0, 0, rect.Width(), rect.Height(), &clDC, rect1.left, rect1.top, SRCCOPY);
	} // if

	pDC->BitBlt(0, 0, rect.Width(), rect.Height(), &m_dcBkgnd, 0, 0, SRCCOPY);
} // End of PaintBk


void CCircularProgressCtrl::DrawRatio(
	CDC *pDC, CRect &rt, CBitmap &bmp, int nPos, BOOL displayUnit)
{
	if (bmp.m_hObject == NULL)
		return;
	if (rt.IsRectEmpty())
		return;

	BITMAP bm;
	bmp.GetBitmap(&bm);
	int nChW = bm.bmWidth / BMP_DIGITS;
	int nDigits = GetDigitCount(nPos) + (displayUnit ? 1 : 0);
	CSize sz(nChW * nDigits, bm.bmHeight);
	if (sz.cx > rt.Width()) {
		sz.cy = sz.cy * rt.Width() / sz.cx;
		sz.cx = rt.Width();
	}
	if (sz.cy > rt.Height()) {
		sz.cx = sz.cx * rt.Height() / sz.cy;
		sz.cy = rt.Height();
	}
	if (sz.cx == 0 || sz.cy == 0)
		return;

	rt.DeflateRect((rt.Width() - sz.cx) / 2, (rt.Height() - sz.cy) / 2);

// 	float ratio = sz.cx / (float)(nChW * nDigits);
	nChW = sz.cx / nDigits;

	CRect rtCh = rt;
	if (displayUnit) {
		rtCh.left = rt.right - nChW;
		DrawDigit(pDC, rtCh, bmp, BMP_UNIT);
		nDigits--;
	}
	for (int i = 0; i < nDigits; i++) {
		int ch = nPos % 10;
		rtCh.left -= nChW;
		rtCh.right -= nChW;
		DrawDigit(pDC, rtCh, bmp, ch);
		nPos /= 10;
	}
}


void CCircularProgressCtrl::DrawDigit(CDC *pDC, CRect &rt, CBitmap &bmp, int ch)
{
	BITMAP bm;
	bmp.GetBitmap(&bm);
	int nChW = bm.bmWidth / BMP_DIGITS;
	if (ch >= BMP_DIGITS)
		return;

	CDC dcMem;
	dcMem.CreateCompatibleDC(pDC);
	dcMem.SelectObject(&bmp);

	BLENDFUNCTION bf;
	memset(&bf, 0, sizeof(bf));
	bf.BlendOp = AC_SRC_OVER;
	bf.BlendFlags = 0;
	bf.SourceConstantAlpha = 255;
	bf.AlphaFormat = AC_SRC_ALPHA;
	pDC->AlphaBlend(rt.left, rt.top, rt.Width(), rt.Height(),
		&dcMem, nChW * ch, 0, nChW, bm.bmHeight, bf);
}



BOOL CCircularProgressCtrl::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;// CProgressCtrl::OnEraseBkgnd(pDC);
}


void CCircularProgressCtrl::ResetBackground()
{
	m_bmpParent.DeleteObject();
	m_dcBkgnd.DeleteDC();
}

void CCircularProgressCtrl::SetBk(CDC* pDC)
{
	if (m_bTransparent && pDC)
	{
		m_bmpParent.DeleteObject();
		m_dcBkgnd.DeleteDC();

		CRect rect;
		CRect rect1;

		GetClientRect(rect);

		GetWindowRect(rect1);
		GetParent()->ScreenToClient(rect1);

		m_dcBkgnd.CreateCompatibleDC(pDC);
		m_bmpParent.CreateCompatibleBitmap(pDC, rect.Width(), rect.Height());
		m_dcBkgnd.SelectObject(&m_bmpParent);
		m_dcBkgnd.BitBlt(0, 0, rect.Width(), rect.Height(), pDC, rect1.left, rect1.top, SRCCOPY);
	} // if
} // End of SetBk


void CCircularProgressCtrl::RemoveBorder()
{
	ModifyStyleEx(WS_EX_STATICEDGE, 0);
}


void CCircularProgressCtrl::OnDestroy()
{
	CProgressCtrl::OnDestroy();
	SetState(TBPF_NOPROGRESS);
}


void CCircularProgressCtrl::SetState(TBPFLAG tbpFlags)
{
	m_tbpFlags = tbpFlags;
	if (m_pTaskbarList != NULL)
		m_pTaskbarList->SetProgressState(m_hWndBind, tbpFlags);
}


void CCircularProgressCtrl::OnTimer(UINT_PTR nIDEvent)
{
	BOOL bNeedContinue = FALSE;

	int nCurPos = GetCurPos1();
	int nDiff = GetPos1() - nCurPos;
	if (m_nStep1 != 0)
		nDiff = min(nDiff, m_nStep1);
	if (nDiff > 0) {
		m_nCurPos1 += nDiff;
		if (m_pTaskbarList != NULL) {
			int nLower, nUpper;
			GetRange(nLower, nUpper);
			int range = nUpper - nLower;
			int pos = m_nCurPos1 - nLower;
			m_pTaskbarList->SetProgressValue(m_hWndBind, pos, range);
			m_pTaskbarList->SetProgressState(m_hWndBind, m_tbpFlags);
		}
		bNeedContinue = TRUE;
	}

	nCurPos = GetCurPos2();
	nDiff = GetPos2() - nCurPos;
	if (m_nStep2 != 0)
		nDiff = min(nDiff, m_nStep2);
	m_nCurPos2 += nDiff;
	if (nDiff != 0)
		bNeedContinue = TRUE;

	if (!bNeedContinue)
		KillTimer(kTimerID);

	CProgressCtrl::OnTimer(nIDEvent);
}
