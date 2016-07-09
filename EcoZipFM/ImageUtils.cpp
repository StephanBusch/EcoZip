#include "stdafx.h"
#include "ImageUtils.h"

#define _USE_MATH_DEFINES
#include <math.h>


CImageUtils::CImageUtils()
{
}


CImageUtils::~CImageUtils()
{
}


BOOL CImageUtils::LoadBitmapFromPNG(UINT uResourceID, CBitmap& BitmapOut)
{
	HBITMAP hbmp;
	BOOL bRet = LoadBitmapFromPNG(uResourceID, hbmp);
	if (bRet)
		BitmapOut.Attach(hbmp);
	return bRet;
}


BOOL CImageUtils::LoadBitmapFromPNG(UINT uResourceID, HBITMAP& BitmapOut)
{
	BOOL bRet = FALSE;

	HINSTANCE hModuleInstance = AfxGetInstanceHandle();
	HRSRC hResourceHandle = ::FindResource(hModuleInstance, MAKEINTRESOURCE(uResourceID), _T("PNG"));
	if (0 == hResourceHandle)
	{
		return bRet;
	}

	DWORD nImageSize = ::SizeofResource(hModuleInstance, hResourceHandle);
	if (0 == nImageSize)
	{
		return bRet;
	}

	HGLOBAL hResourceInstance = ::LoadResource(hModuleInstance, hResourceHandle);
	if (0 == hResourceInstance)
	{
		return bRet;
	}

	const void* pResourceData = ::LockResource(hResourceInstance);
	if (0 == pResourceData)
	{
		FreeResource(hResourceInstance);
		return bRet;
	}

	HGLOBAL hBuffer = ::GlobalAlloc(GMEM_MOVEABLE, nImageSize);
	if (0 == hBuffer)
	{
		FreeResource(hResourceInstance);
		return bRet;
	}

	void* pBuffer = ::GlobalLock(hBuffer);
	if (0 != pBuffer)
	{
		CopyMemory(pBuffer, pResourceData, nImageSize);
		IStream* pStream = 0;
		if (S_OK == ::CreateStreamOnHGlobal(hBuffer, FALSE, &pStream))
		{
			CImage ImageFromResource;
			ImageFromResource.Load(pStream);
			pStream->Release();
			BitmapOut = ImageFromResource.Detach();
			bRet = TRUE;
		}
		::GlobalUnlock(hBuffer);
	}
	::GlobalFree(hBuffer);

	UnlockResource(hResourceInstance);
	FreeResource(hResourceInstance);

	return bRet;
}


void CImageUtils::PremultiplyBitmapAlpha(HDC hDC, HBITMAP hBmp, HBITMAP hDstBmp)
{
	BITMAP bm = { 0 };
	GetObject(hBmp, sizeof(bm), &bm);
	BITMAPINFO* bmi = (BITMAPINFO*)_alloca(sizeof(BITMAPINFOHEADER) + (256 * sizeof(RGBQUAD)));
	::ZeroMemory(bmi, sizeof(BITMAPINFOHEADER) + (256 * sizeof(RGBQUAD)));
	bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	BOOL bRes = ::GetDIBits(hDC, hBmp, 0, bm.bmHeight, NULL, bmi, DIB_RGB_COLORS);
  if (!bRes || bmi->bmiHeader.biBitCount != 32) {
    _freea(bmi);
    return;
  }
	LPBYTE pBitData = (LPBYTE) ::LocalAlloc(LPTR, bm.bmWidth * bm.bmHeight * sizeof(DWORD));
  if (pBitData == NULL) {
    _freea(bmi);
    return;
  }
	LPBYTE pData = pBitData;
	::GetDIBits(hDC, hBmp, 0, bm.bmHeight, pData, bmi, DIB_RGB_COLORS);

	for (int y = 0; y < bm.bmHeight; y++) {
		for (int x = 0; x < bm.bmWidth; x++) {
			pData[0] = (BYTE)((DWORD)pData[0] * pData[3] / 255);
			pData[1] = (BYTE)((DWORD)pData[1] * pData[3] / 255);
			pData[2] = (BYTE)((DWORD)pData[2] * pData[3] / 255);
			pData += 4;
		}
	}
	if (hDstBmp == NULL)
		hDstBmp = hBmp;
	::SetDIBits(hDC, hDstBmp, 0, bm.bmHeight, pBitData, bmi, DIB_RGB_COLORS);
	::LocalFree(pBitData);
  _freea(bmi);
}


BOOL CImageUtils::CheckPosition(int width, int height, float rate, float ftan, int x, int y)
{
  y = height - y;
  int cx = width / 2;
  int cy = height / 2;
  if (rate < 0.25f) {
    if (x >= cx && y < cy) {
      float cur = (float)(x - cx) / (cy - y);
      if (cur < ftan)
        return TRUE;
    }
    return FALSE;
  }
  if (rate == 0.25f) {
    if (x >= cx && y < cy)
      return TRUE;
    return FALSE;
  }
  if (rate < 0.5f) {
    if (x >= cx && y <= cy)
      return TRUE;
    if (x >= cx && y > cy) {
      float cur = (float)(x - cx) / (cy - y);
      if (cur < ftan)
        return TRUE;
    }
    return FALSE;
  }
  if (rate == 0.5f) {
    if (x >= cx)
      return TRUE;
    return FALSE;
  }
  if (rate < 0.75f) {
    if (x >= cx)
      return TRUE;
    if (y > cy) {
      float cur = (float)(x - cx) / (cy - y);
      if (cur < ftan)
        return TRUE;
    }
    return FALSE;
  }
  if (rate == 0.75f) {
    if (x < cx && y < cy)
      return FALSE;
    return TRUE;
  }
  if (rate < 1.0f) {
    if (x < cx && y < cy) {
      float cur = (float)(x - cx) / (cy - y);
      if (cur > ftan)
        return FALSE;
    }
    return TRUE;
  }
  return TRUE;
}


void CImageUtils::PremultiplyBitmapAlpha(HDC hDC, HBITMAP hBmp, float rate, HBITMAP hDstBmp)
{
  BITMAP bm = { 0 };
  GetObject(hBmp, sizeof(bm), &bm);
  BITMAPINFO* bmi = (BITMAPINFO*)_alloca(sizeof(BITMAPINFOHEADER) + (256 * sizeof(RGBQUAD)));
  ::ZeroMemory(bmi, sizeof(BITMAPINFOHEADER) + (256 * sizeof(RGBQUAD)));
  bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  BOOL bRes = ::GetDIBits(hDC, hBmp, 0, bm.bmHeight, NULL, bmi, DIB_RGB_COLORS);
  if (!bRes || bmi->bmiHeader.biBitCount != 32) return;
  LPBYTE pBitData = (LPBYTE) ::LocalAlloc(LPTR, bm.bmWidth * bm.bmHeight * sizeof(DWORD));
  if (pBitData == NULL) return;
  LPBYTE pData = pBitData;
  ::GetDIBits(hDC, hBmp, 0, bm.bmHeight, pData, bmi, DIB_RGB_COLORS);

  float ftan = (float)tan(rate * M_PI * 2);
  for (int y = 0; y < bm.bmHeight; y++) {
    for (int x = 0; x < bm.bmWidth; x++) {
      if (CheckPosition(bm.bmWidth, bm.bmHeight, rate, ftan, x, y)) {
        pData[0] = (BYTE)((DWORD)pData[0] * pData[3] / 255);
        pData[1] = (BYTE)((DWORD)pData[1] * pData[3] / 255);
        pData[2] = (BYTE)((DWORD)pData[2] * pData[3] / 255);
      }
      else
        pData[3] = 0;
      pData += 4;
    }
  }
  if (hDstBmp == NULL)
    hDstBmp = hBmp;
  ::SetDIBits(hDC, hDstBmp, 0, bm.bmHeight, pBitData, bmi, DIB_RGB_COLORS);
  ::LocalFree(pBitData);
}


void CImageUtils::SelectOpaquePixels(HDC hDC, HBITMAP hBmp, HBITMAP hDstBmp, int threshold)
{
	BITMAP bm = { 0 };
	GetObject(hBmp, sizeof(bm), &bm);
	BITMAPINFO* bmi = (BITMAPINFO*)_alloca(sizeof(BITMAPINFOHEADER) + (256 * sizeof(RGBQUAD)));
	::ZeroMemory(bmi, sizeof(BITMAPINFOHEADER) + (256 * sizeof(RGBQUAD)));
	bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	BOOL bRes = ::GetDIBits(hDC, hBmp, 0, bm.bmHeight, NULL, bmi, DIB_RGB_COLORS);
  if (!bRes || bmi->bmiHeader.biBitCount != 32) {
    _freea(bmi);
    return;
  }
  LPBYTE pBitData = (LPBYTE) ::LocalAlloc(LPTR, bm.bmWidth * bm.bmHeight * sizeof(DWORD));
  if (pBitData == NULL) {
    _freea(bmi);
    return;
  }
  LPBYTE pData = pBitData;
	::GetDIBits(hDC, hBmp, 0, bm.bmHeight, pData, bmi, DIB_RGB_COLORS);

	for (int y = 0; y < bm.bmHeight; y++) {
		for (int x = 0; x < bm.bmWidth; x++) {
			if (pData[3] < threshold) {
				pData[0] = 0;
				pData[1] = 0;
				pData[2] = 0;
				pData[3] = 0;
			}
			pData += 4;
		}
	}
	::SetDIBits(hDC, hDstBmp, 0, bm.bmHeight, pBitData, bmi, DIB_RGB_COLORS);
	::LocalFree(pBitData);
  _freea(bmi);
}


// GetRotatedBitmap	- Create a new bitmap with rotated image
// Returns		- Returns new bitmap with rotated image
// hBitmap		- Bitmap to rotate
// radians		- Angle of rotation in radians
// clrBack		- Color of pixels in the resulting bitmap that do
//			  not get covered by source pixels
// Note			- If the bitmap uses colors not in the system palette 
//			  then the result is unexpected. You can fix this by
//			  adding an argument for the logical palette.
HBITMAP CImageUtils::GetRotatedBitmap(HBITMAP hBitmap, double radians, COLORREF clrBack)
{
  // Create a memory DC compatible with the display
  CDC sourceDC, destDC;
  sourceDC.CreateCompatibleDC(NULL);
  destDC.CreateCompatibleDC(NULL);

  // Get logical coordinates
  BITMAP bm;
  ::GetObject(hBitmap, sizeof(bm), &bm);

  float cosine = (float)cos(radians);
  float sine = (float)sin(radians);

  // Compute dimensions of the resulting bitmap
  // First get the coordinates of the 3 corners other than origin
  int x1 = (int)(-bm.bmHeight * sine);
  int y1 = (int)(bm.bmHeight * cosine);
  int x2 = (int)(bm.bmWidth * cosine - bm.bmHeight * sine);
  int y2 = (int)(bm.bmHeight * cosine + bm.bmWidth * sine);
  int x3 = (int)(bm.bmWidth * cosine);
  int y3 = (int)(bm.bmWidth * sine);

  int minx = min(0, min(x1, min(x2, x3)));
  int miny = min(0, min(y1, min(y2, y3)));
  int maxx = max(x1, max(x2, x3));
  int maxy = max(y1, max(y2, y3));

  int w = maxx - minx;
  int h = maxy - miny;


  // Create a bitmap to hold the result
  HBITMAP hbmResult = ::CreateCompatibleBitmap(CClientDC(NULL), w, h);

  HBITMAP hbmOldSource = (HBITMAP)::SelectObject(sourceDC.m_hDC, hBitmap);
  HBITMAP hbmOldDest = (HBITMAP)::SelectObject(destDC.m_hDC, hbmResult);

  // Draw the background color before we change mapping mode
  HBRUSH hbrBack = CreateSolidBrush(clrBack);
  HBRUSH hbrOld = (HBRUSH)::SelectObject(destDC.m_hDC, hbrBack);
  destDC.PatBlt(0, 0, w, h, PATCOPY);
  ::DeleteObject(::SelectObject(destDC.m_hDC, hbrOld));

  // Set mapping mode so that +ve y axis is upwords
  sourceDC.SetMapMode(MM_ISOTROPIC);
  sourceDC.SetWindowExt(1, 1);
  sourceDC.SetViewportExt(1, -1);
  sourceDC.SetViewportOrg(0, bm.bmHeight - 1);

  destDC.SetMapMode(MM_ISOTROPIC);
  destDC.SetWindowExt(1, 1);
  destDC.SetViewportExt(1, -1);
  destDC.SetWindowOrg(minx, maxy);

  // Now do the actual rotating - a pixel at a time
  // Computing the destination point for each source point
  // will leave a few pixels that do not get covered
  // So we use a reverse transform - e.i. compute the source point
  // for each destination point

  for (int y = miny; y < maxy; y++)
  {
    for (int x = minx; x < maxx; x++)
    {
      int sourcex = (int)(x*cosine + y*sine);
      int sourcey = (int)(y*cosine - x*sine);
      if (sourcex >= 0 && sourcex < bm.bmWidth && sourcey >= 0
        && sourcey < bm.bmHeight)
        destDC.SetPixel(x, y, sourceDC.GetPixel(sourcex, sourcey));
    }
  }

  // Restore DCs
  ::SelectObject(sourceDC.m_hDC, hbmOldSource);
  ::SelectObject(destDC.m_hDC, hbmOldDest);

  return hbmResult;
}


// GetRotatedBitmap	- Create a new bitmap with rotated image
// Returns		- Returns new bitmap with rotated image
// hBitmap		- Bitmap to rotate
// radians		- Angle of rotation in radians
// clrBack		- Color of pixels in the resulting bitmap that do
//			  not get covered by source pixels
// Note			- If the bitmap uses colors not in the system palette 
//			  then the result is unexpected. You can fix this by
//			  adding an argument for the logical palette.
HBITMAP CImageUtils::GetRotatedBitmap(HBITMAP hBitmap, double radians)
{
  // Get logical coordinates
  BITMAP bm;
  ::GetObject(hBitmap, sizeof(bm), &bm);

  float cosine = (float)cos(radians);
  float sine = (float)sin(radians);

  CClientDC dc(NULL);

  // Compute dimensions of the resulting bitmap
  // First get the coordinates of the 3 corners other than origin
  int x1 = (int)(-bm.bmHeight * sine);
  int y1 = (int)(bm.bmHeight * cosine);
  int x2 = (int)(bm.bmWidth * cosine - bm.bmHeight * sine);
  int y2 = (int)(bm.bmHeight * cosine + bm.bmWidth * sine);
  int x3 = (int)(bm.bmWidth * cosine);
  int y3 = (int)(bm.bmWidth * sine);

  int minx = min(0, min(x1, min(x2, x3)));
  int miny = min(0, min(y1, min(y2, y3)));
  int maxx = max(0, max(x1, max(x2, x3)));
  int maxy = max(0, max(y1, max(y2, y3)));

  int w = maxx - minx;
  int h = maxy - miny;

  // Create a bitmap to hold the result
  HBITMAP hbmResult = ::CreateCompatibleBitmap(CClientDC(NULL), w, h);

  // Get Pixel Data
  BITMAPINFO *bmiSource = (BITMAPINFO*)_alloca(sizeof(BITMAPINFOHEADER) + (256 * sizeof(RGBQUAD)));
  ::ZeroMemory(bmiSource, sizeof(BITMAPINFOHEADER) + (256 * sizeof(RGBQUAD)));
  bmiSource->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  BOOL bRes = ::GetDIBits(dc, hBitmap, 0, bm.bmHeight, NULL, bmiSource, DIB_RGB_COLORS);
  if (!bRes || bmiSource->bmiHeader.biBitCount != 32) {
    _freea(bmiSource);
    return NULL;
  }
  LPBYTE pSource = (LPBYTE) ::LocalAlloc(LPTR, bm.bmWidth * bm.bmHeight * sizeof(DWORD));
  if (pSource == NULL) {
    _freea(bmiSource);
    return NULL;
  }
  ::GetDIBits(dc, hBitmap, 0, bm.bmHeight, pSource, bmiSource, DIB_RGB_COLORS);

  BITMAPINFO *bmiDest = (BITMAPINFO*)_alloca(sizeof(BITMAPINFOHEADER) + (256 * sizeof(RGBQUAD)));
  ::ZeroMemory(bmiDest, sizeof(BITMAPINFOHEADER) + (256 * sizeof(RGBQUAD)));
  bmiDest->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bRes = ::GetDIBits(dc, hbmResult, 0, h, NULL, bmiDest, DIB_RGB_COLORS);
  if (!bRes || bmiDest->bmiHeader.biBitCount != 32) {
    _freea(bmiDest);
    return NULL;
  }
  LPBYTE pDest = (LPBYTE) ::LocalAlloc(LPTR, w * h * sizeof(DWORD));
  if (pDest == NULL) {
    _freea(bmiDest);
    return NULL;
  }
  ::GetDIBits(dc, hbmResult, 0, h, pDest, bmiDest, DIB_RGB_COLORS);
  ::ZeroMemory(pDest, w * h * sizeof(DWORD));

  // Now do the actual rotating - a pixel at a time
  // Computing the destination point for each source point
  // will leave a few pixels that do not get covered
  // So we use a reverse transform - e.i. compute the source point
  // for each destination point

  LPBYTE pData = pDest;
  for (int y = miny; y < maxy; y++)
  {
    for (int x = minx; x < maxx; x++)
    {
      int sourcex = (int)(x*cosine + y*sine);
      int sourcey = (int)(y*cosine - x*sine);
      if (sourcex >= 0 && sourcex < bm.bmWidth && sourcey >= 0
        && sourcey < bm.bmHeight) {
        int index = (sourcex + bm.bmWidth * sourcey) * sizeof(DWORD);
        pData[0] = pSource[index + 0];
        pData[1] = pSource[index + 1];
        pData[2] = pSource[index + 2];
        pData[3] = pSource[index + 3];
      }
			pData += 4;
    }
  }

  ::SetDIBits(dc, hbmResult, 0, h, pDest, bmiDest, DIB_RGB_COLORS);

  ::LocalFree(pSource);
  _freea(bmiSource);
  ::LocalFree(pDest);
  _freea(bmiDest);

  return hbmResult;
}

