#pragma once

#include "CPP/Common/MyCom.h"
#include "CPP/7zip/Archive/IArchive.h"
#include "CPP/7zip/UI/FileManager/IFolder.h"
#include "CPP/7zip/UI/Filemanager/SysIconUtils.h"

#include "NuonImg.h"


// CDetailView

class CDetailView : public CWnd
{
protected: // create from serialization only
	CDetailView();
	DECLARE_DYNCREATE(CDetailView)

	// Operations
public:

	// Overrides
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

	// Implementation
public:
	virtual ~CDetailView();

	HRESULT SetSelItem(CMyComPtr<IFolderFolder> folder, int itemIndex,
		UString &currentFolderPrefix, bool preview);

protected:
	virtual void DrawContents(CDC *pDC);
	void DrawLogo(CDC *pDC);
	void DrawDetails(CDC *pDC);
	void DrawPreview(CDC *pDC);
	void LoadPreviewData();

	// Generated message map functions
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	DECLARE_MESSAGE_MAP()

	// Attributes
public:
protected:
  HICON         m_hIcon;
  CBitmap			  m_bmpBkgnd;

	enum DisplayMode {
		Logo = 0,
		Details = 1,
		Preview = 2,
	};
	DisplayMode		m_mode;
	CBitmap			  m_bmpLogo;
	CNuonImg		  m_imgPreview;

	CMyComPtr<IFolderFolder> _folder;
	CMyComPtr<IFolderCompare> _folderCompare;
	CMyComPtr<IFolderGetItemName> _folderGetItemName;
	CMyComPtr<IArchiveGetRawProps> _folderRawProps;
	CMyComPtr<IFolderGetSystemIconIndex> folderGetSystemIconIndex;
	int				    m_nItemIndex;

	CExtToIconMap	_extToIconMap;
	UString			  m_strItemName;
	int				    m_nImage;
	UStringVector	m_propNames;
	UStringVector	m_propValues;
	UInt32			  m_attrib;
};
