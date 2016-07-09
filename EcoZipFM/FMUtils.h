// FMUtils.h

#ifndef _FM_UTILS_H_
#define _FM_UTILS_H_

#include <string>
#include <sstream>
#include <iomanip>
#include <locale>

#include "CPP/Common/MyCom.h"

#include "CPP/7zip/UI/FileManager/IFolder.h"

extern CMyComPtr<IFolderFolder> GetFolder(LPCSTR fullPath);
extern CMyComPtr<IFolderFolder> GetFolder(LPCWSTR fullPath);

extern UString GetFolderTypeID(CMyComPtr<IFolderFolder> _folder);
extern bool IsFolderTypeEqTo(CMyComPtr<IFolderFolder> _folder, const wchar_t *s);
extern bool IsRootFolder(CMyComPtr<IFolderFolder> _folder);
extern bool IsFSFolder(CMyComPtr<IFolderFolder> _folder);
extern bool IsFSDrivesFolder(CMyComPtr<IFolderFolder> _folder);
bool IsNetFolder(CMyComPtr<IFolderFolder> _folder);
extern bool IsArcFolder(CMyComPtr<IFolderFolder> _folder);
bool IsDeviceDrivesPrefix(CMyComPtr<IFolderFolder> folder);
bool IsSuperDrivesPrefix(CMyComPtr<IFolderFolder> folder);

extern UString GetFolderPath(IFolderFolder *folder);


template <typename T_STRING>
T_STRING ExtractDrivePath(const T_STRING &volName)
{
	INT nColon = volName.Find(_T(':'));
	if (nColon < 1)
		return T_STRING();
	UINT drive = (UINT)volName.Ptr()[nColon - 1];
	if (drive < 'A' || drive > 'Z')
		return T_STRING();
	return volName.Mid(nColon - 1, 2) + FCHAR_PATH_SEPARATOR;
}


template <typename T_STRING>
T_STRING ExtractComputerName(const T_STRING &fullPath)
{
	T_STRING computer = fullPath;
	int pos = fullPath.Find(L"\\\\");
	if (pos >= 0)
		computer.DeleteFrontal(pos + 2);
	pos = computer.Find(WCHAR_PATH_SEPARATOR);
	if (pos >= 0)
		computer.DeleteFrom(pos);
	return computer;
}

int GetRealIconIndex(CFSTR path, DWORD attributes);

bool AreEqualNames(const UString &path, const wchar_t *name);

bool GetBoolsVal(const CBoolPair &b1, const CBoolPair &b2);
void AddComboItems(CComboBox &combo, const UInt32 *langIDs, unsigned numItems, const int *values, int curVal);
bool IsAsciiString(const UString &s);
void AddUniqueString(UStringVector &list, const UString &s);

int GetSortControlID(PROPID propID);

void ReduceString(UString &s, unsigned size);
void ConvertIntToDecimalString(UInt64 value, wchar_t *dest);
UString ConvertIntToDecimalString(UInt64 value);
void ConvertSizeToString(UInt64 val, wchar_t *s) throw();
UString ConvertSizeToString(UInt64 value);
bool IsCorrectFsName(const UString &name);
bool CorrectFsPath(const UString & relBase, const UString &path, UString &result);

UString HResultToMessage(HRESULT errorCode);

bool IsLargePageSupported();

void GetTimeString(UInt64 timeValue, wchar_t *s);

template<class T>
std::string FormatWithCommasA(T value)
{
	std::stringstream ss;
	ss.imbue(std::locale(""));
	ss << std::fixed << value;
	return ss.str();
}

template<class T>
std::wstring FormatWithCommasW(T value)
{
	std::wstringstream ss;
	ss.imbue(std::locale(""));
	ss << std::fixed << value;
	return ss.str();
}

#ifdef _UNICODE
#define FormatWithCommas FormatWithCommasW
#else
#define FormatWithCommas FormatWithCommasA
#endif


inline char GetHex(Byte value)
{
	return (char)((value < 10) ? ('0' + value) : ('A' + (value - 10)));
}


template<class T>
HRESULT AssignToOutputPointer(T** pp, const CComPtr<T> &p)
{
	assert(pp);
	*pp = p;
	if (nullptr != (*pp))
	{
		(*pp)->AddRef();
	}

	return S_OK;
}

// reduces path to part that exists on disk
void ReducePathToRealFileSystemPath(UString &path);
bool CheckFolderPath(const UString &path);

const int kParentIndex = -1;

bool GetItem_BoolProp(CMyComPtr<IFolderFolder> folder,
  UInt32 itemIndex, PROPID propID);
bool IsItem_Deleted(CMyComPtr<IFolderFolder> folder, int itemIndex);
bool IsItem_Folder(CMyComPtr<IFolderFolder> folder, int itemIndex);
bool IsItem_AltStream(CMyComPtr<IFolderFolder> folder, int itemIndex);

UString GetItemName(CMyComPtr<IFolderFolder> folder, int itemIndex);
void GetItemName(CMyComPtr<IFolderFolder> folder, int itemIndex, UString &s);
UString GetItemPrefix(CMyComPtr<IFolderFolder> folder, int itemIndex);
UString GetItemRelPath(CMyComPtr<IFolderFolder> folder, int itemIndex);
UString GetItemRelPath2(CMyComPtr<IFolderFolder> folder, int itemIndex);
UString GetItemFullPath(CMyComPtr<IFolderFolder> folder, int itemIndex);

HRESULT GetCpuTemperature(LPLONG pTemperature);

HANDLE AddResourceFont(LPCTSTR ResID, DWORD *Installed);

#endif // _FM_UTILS_H_
