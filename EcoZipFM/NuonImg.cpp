#include "stdafx.h"
#include "NuonImg.h"
#include "WICImagingFactory.h"


#define DIB_WIDTHBYTES(bits) ((((bits) + 31)>>5)<<2)

CNuonImg::CNuonImg()
  : m_pDecoder(nullptr)
  , m_pFrame(nullptr)
  , m_pConvertedFrame(nullptr)
  , m_nWidth(0)
  , m_nHeight(0)
  , m_hBitmap(NULL)
{
  // Initialize COM
  CoInitialize(nullptr);
}


CNuonImg::~CNuonImg()
{
  Cleanup();

  // Uninitialize COM
  CoUninitialize();
}


void CNuonImg::Cleanup()
{
  m_nWidth = m_nHeight = 0;

  CloseFile();
}


void CNuonImg::CloseFile()
{
  if (m_pConvertedFrame)
  {
    m_pConvertedFrame.Release();
    m_pConvertedFrame = nullptr;
  }
  if (m_pFrame)
  {
    m_pFrame.Release();
    m_pFrame = nullptr;
  }
  if (m_pDecoder)
  {
    m_pDecoder.Release();
    m_pDecoder = nullptr;
  }
}


#define IfFailedThrowHR(expr) {HRESULT hr = (expr); if (FAILED(hr)) throw hr;}

HRESULT CNuonImg::Open(const wchar_t* pszFile, UINT nFrame/* = 0*/)
{
  try
  {
    // Cleanup a previously loaded image
    Cleanup();

    // Get the WIC factory from the singleton wrapper class
    IWICImagingFactory* pFactory = CWICImagingFactory::GetInstance().GetFactory();
    ASSERT(pFactory);
    if (!pFactory)
      return E_NOT_SET; //WINCODEC_ERR_NOTINITIALIZED;

    // Create a decoder for the given image file
    RINOK(pFactory->CreateDecoderFromFilename(
      pszFile, NULL, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &m_pDecoder));

    // Validate the given frame index nFrame
    UINT nCount = 0;
    // Get the number of frames in this image
    if (SUCCEEDED(m_pDecoder->GetFrameCount(&nCount)))
    {
      if (nFrame >= nCount)
        nFrame = nCount - 1; // If the requested frame number is too big, default to the last frame
    }
    // Retrieve the requested frame of the image from the decoder
    IfFailedThrowHR(m_pDecoder->GetFrame(nFrame, &m_pFrame));

    // Retrieve the image dimensions
    IfFailedThrowHR(m_pFrame->GetSize(&m_nWidth, &m_nHeight));

    // Convert the format of the image frame to 32bppBGR
    IfFailedThrowHR(pFactory->CreateFormatConverter(&m_pConvertedFrame));
    IfFailedThrowHR(m_pConvertedFrame->Initialize(
      m_pFrame,                        // Source frame to convert
      GUID_WICPixelFormat32bppBGR,     // The desired pixel format
      WICBitmapDitherTypeNone,         // The desired dither pattern
      NULL,                            // The desired palette 
      0.f,                             // The desired alpha threshold
      WICBitmapPaletteTypeCustom       // Palette translation type
      ));

    HDC hdcScreen = GetDC(NULL);
    if (!hdcScreen)
      return E_FAIL;
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    if (m_hBitmap != NULL)
      DeleteObject(m_hBitmap);
    m_hBitmap = CreateCompatibleBitmap(hdcScreen, m_nWidth, m_nHeight);
    SelectObject(hdcMem, m_hBitmap);
    PreRender(hdcMem, 0, 0, m_nWidth, m_nHeight);

    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);

    CloseFile();
    return S_OK;
  }
  catch (...)
  {
    if (m_hBitmap == NULL)
      DeleteObject(m_hBitmap);
    m_hBitmap = NULL;
    // Cleanup after something went wrong
    Cleanup();
    // Rethrow the exception, so the client code can handle it
    return E_FAIL;
  }
}


bool CNuonImg::IsLoaded() const
{
  return m_hBitmap != NULL;
  //return m_pConvertedFrame != nullptr;
}


UINT CNuonImg::GetWidth() const
{
  return m_nWidth;
}


UINT CNuonImg::GetHeight() const
{
  return m_nHeight;
}


void CNuonImg::Render(HDC hDC, UINT x, UINT y, UINT cx, UINT cy)
{
  // Make sure an image has been loaded
  if (!IsLoaded())
    throw WINCODEC_ERR_WRONGSTATE;

  int nSaved = SaveDC(hDC);
  BITMAP bm;
  ::GetObject(m_hBitmap, sizeof(BITMAP), &bm);
  HDC hMemDC = CreateCompatibleDC(hDC);
  SelectObject(hMemDC, m_hBitmap);
  SetStretchBltMode(hDC, HALFTONE);
  StretchBlt(hDC, x, y, cx, cy, hMemDC, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
  DeleteDC(hMemDC);
  RestoreDC(hDC, nSaved);
}

void CNuonImg::PreRender(HDC hDC, UINT x, UINT y, UINT cx, UINT cy)
{
  // Make sure an image has been loaded
  if (!IsLoaded())
    throw WINCODEC_ERR_WRONGSTATE;

  // Get the WIC factory from the singleton wrapper class
  IWICImagingFactory* pFactory = CWICImagingFactory::GetInstance().GetFactory();
  if (!pFactory)
    throw WINCODEC_ERR_NOTINITIALIZED;

  // Create a WIC image scaler to scale the image to the requested size
  CComPtr<IWICBitmapScaler> pScaler = nullptr;
  IfFailedThrowHR(pFactory->CreateBitmapScaler(&pScaler));
  IfFailedThrowHR(pScaler->Initialize(m_pConvertedFrame, cx, cy, WICBitmapInterpolationModeFant));

  // Render the image to a GDI device context
  HBITMAP hDIBBitmap = NULL;
  try
  {
    // Get a DC for the full screen
    HDC hdcScreen = GetDC(NULL);
    if (!hdcScreen)
      throw 1;

    BITMAPINFO bminfo;
    ZeroMemory(&bminfo, sizeof(bminfo));
    bminfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bminfo.bmiHeader.biWidth = cx;
    bminfo.bmiHeader.biHeight = -(LONG)cy;
    bminfo.bmiHeader.biPlanes = 1;
    bminfo.bmiHeader.biBitCount = 32;
    bminfo.bmiHeader.biCompression = BI_RGB;

    void* pvImageBits = nullptr;    // Freed with DeleteObject(hDIBBitmap)
    hDIBBitmap = CreateDIBSection(hdcScreen, &bminfo, DIB_RGB_COLORS, &pvImageBits, NULL, 0);
    if (!hDIBBitmap)
      throw 2;

    ReleaseDC(NULL, hdcScreen);

    // Calculate the number of bytes in 1 scanline
    UINT nStride = DIB_WIDTHBYTES(cx * 32);
    // Calculate the total size of the image
    UINT nImage = nStride * cy;
    // Copy the pixels to the DIB section
    IfFailedThrowHR(pScaler->CopyPixels(nullptr, nStride, nImage, reinterpret_cast<BYTE*>(pvImageBits)));

    // Copy the bitmap to the target device context
    ::SetDIBitsToDevice(hDC, x, y, cx, cy, 0, 0, 0, cy, pvImageBits, &bminfo, DIB_RGB_COLORS);

    DeleteObject(hDIBBitmap);
  }
  catch (...)
  {
    if (hDIBBitmap)
      DeleteObject(hDIBBitmap);
    // Rethrow the exception, so the client code can handle it
    throw;
  }
}

