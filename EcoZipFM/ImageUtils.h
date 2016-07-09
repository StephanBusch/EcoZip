#pragma once
class CImageUtils
{
public:
	CImageUtils();
	~CImageUtils();

	static BOOL LoadBitmapFromPNG(UINT uResourceID, CBitmap& BitmapOut);
	static BOOL LoadBitmapFromPNG(UINT uResourceID, HBITMAP& BitmapOut);
	static void PremultiplyBitmapAlpha(HDC hDC, HBITMAP hBmp, HBITMAP hDstBmp = NULL);
  static BOOL CheckPosition(int width, int height, float rate, float ftan, int x, int y);
  static void PremultiplyBitmapAlpha(HDC hDC, HBITMAP hBmp, float rate, HBITMAP hDstBmp);
  static void SelectOpaquePixels(HDC hDC, HBITMAP hBmp, HBITMAP hDstBmp, int threshold = 250);
  static HBITMAP GetRotatedBitmap(HBITMAP hBitmap, double radians, COLORREF clrBack);
  static HBITMAP GetRotatedBitmap(HBITMAP hBitmap, double radians);
};

