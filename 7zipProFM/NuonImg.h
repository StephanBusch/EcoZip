#pragma once

#include <atlbase.h>
#include <wincodec.h>
#include <wincodecsdk.h>

class CNuonImg
{
public:
	CNuonImg();
	virtual ~CNuonImg();

	// Opens the nFrame-th frame of the given image file.
	// Throws HRESULT in case of failure.
	virtual HRESULT Open(const wchar_t* pszFile, UINT nFrame = 0);

	// Returns true if an image is loaded successfully, false otherwise
	virtual bool IsLoaded() const;

	// Renders the loaded image to the given device context hDC,
	// at position x,y and size cx, cy.
	// Throws HRESULT in case of failure.
	virtual void Render(HDC hDC, UINT x, UINT y, UINT cx, UINT cy);

	// Returns the width of the loaded image.
	virtual UINT GetWidth() const;

	// Returns the height of the loaded image.
	virtual UINT GetHeight() const;

protected:
	virtual void Cleanup();
	virtual void CloseFile();
	virtual void PreRender(HDC hDC, UINT x, UINT y, UINT cx, UINT cy);

	CComPtr<IWICBitmapDecoder> m_pDecoder;
	CComPtr<IWICBitmapFrameDecode> m_pFrame;
	CComPtr<IWICFormatConverter> m_pConvertedFrame;
	UINT m_nWidth;
	UINT m_nHeight;
	HBITMAP m_hBitmap;
};
