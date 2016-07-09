// FMUtils.cpp


#include "stdafx.h"
#include "FMUtils.h"

#include "CPP/Common/IntToString.h"
#include "CPP/Common/StringConvert.h"
#include "CPP/Common/Wildcard.h"

#include "CPP/7zip/PropID.h"

#include "CPP/Windows/ErrorMsg.h"
#include "CPP/Windows/FileDir.h"
#include "CPP/Windows/FileFind.h"
#include "CPP/Windows/FileName.h"
#include "CPP/Windows/PropVariant.h"

#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/SysIconUtils.h"

#include "RootFolder.h"
#include "EcoZipFM.h"

#define _WIN32_DCOM
#include <iostream>
using namespace std;
#include <comdef.h>
#include <Wbemidl.h>


using namespace NWindows;
using namespace NFile;
using namespace NFind;


CMyComPtr<IFolderFolder> GetFolder(LPCSTR fullPath)
{
  return GetFolder(GetUnicodeString(fullPath));
}


CMyComPtr<IFolderFolder> GetFolder(LPCWSTR fullPath)
{
  IFolderFolder *_folder;

  CRootFolder *rootFolderSpec = new CRootFolder;
  _folder = rootFolderSpec;
  rootFolderSpec->Init();

  // 	IFolderFolder *newFolder;
  // 
  // 	UStringVector parts;
  // 	SplitPathToParts(fullPath, parts);
  // 	FOR_VECTOR(i, parts)
  // 	{
  // 		const UString &s = parts[i];
  // 		if (/*(i == 0 || i == parts.Size() - 1) &&*/ s.IsEmpty())
  // 			continue;
  // 		HRESULT res = _folder->BindToFolder(s, &newFolder);
  // 		if (!newFolder || res != S_OK)
  // 			return NULL;
  // 		_folder->Release();
  // 		_folder = newFolder;
  // 	}

  UString sysPath = fullPath;
  CFileInfo fileInfo;

  while (!sysPath.IsEmpty())
  {
    if (fileInfo.Find(us2fs(sysPath)))
      break;
    int pos = sysPath.ReverseFind_PathSepar();
    if (pos < 0)
      sysPath.Empty();
    else
    {
      /*
      if (reducedParts.Size() > 0 || pos < (int)sysPath.Len() - 1)
        reducedParts.Add(sysPath.Ptr(pos + 1));
      */
      #if defined(_WIN32) && !defined(UNDER_CE)
      if (pos == 2 && NName::IsDrivePath2(sysPath) && sysPath.Len() > 3)
        pos++;
      #endif

      sysPath.DeleteFrom(pos);
    }
  }

  CMyComPtr<IFolderFolder> newFolder;

  if (sysPath.IsEmpty())
  {
    _folder->BindToFolder(fullPath, &newFolder);
  }
  else {
    if (!fileInfo.IsDir()) {
      int pos = sysPath.ReverseFind_PathSepar();
      if (pos < 0)
        sysPath.Empty();
      else
        sysPath.DeleteFrom(pos);
    }
    NName::NormalizeDirPathPrefix(sysPath);
    if (_folder->BindToFolder(sysPath, &newFolder) == S_OK)
      _folder = newFolder;
    else
      _folder = NULL;
  }

  // 	CRootFolder *rootFolderSpec = new CRootFolder;
  // 	_folder = rootFolderSpec;
  // 	rootFolderSpec->Init();
  // 	CMyComPtr<IFolderFolder> newFolder;
  // 	UString sysPath = fullPath;
  // 	CFileInfo fileInfo;
  // 	UStringVector reducedParts;
  // 	while (!sysPath.IsEmpty())
  // 	{
  // 		if (fileInfo.Find(us2fs(sysPath)))
  // 			break;
  // 		int pos = sysPath.ReverseFind(WCHAR_PATH_SEPARATOR);
  // 		if (pos < 0)
  // 			sysPath.Empty();
  // 		else
  // 		{
  // 			if (reducedParts.Size() > 0 || pos < (int)sysPath.Len() - 1)
  // 				reducedParts.Add(sysPath.Ptr(pos + 1));
  // 			sysPath.DeleteFrom(pos);
  // 		}
  // 	}
  // 	if (sysPath.IsEmpty())
  // 	{
  // 		if (_folder->BindToFolder(fullPath, &newFolder) == S_OK)
  // 			return newFolder;
  // 	}
  // 	else if (fileInfo.IsDir())
  // 	{
  // 		NName::NormalizeDirPathPrefix(sysPath);
  // 		if (_folder->BindToFolder(sysPath, &newFolder) == S_OK)
  // 			return newFolder;
  // 	}
  // 	else
  // 	{
  // 		FString dirPrefix, fileName;
  // 		NDir::GetFullPathAndSplit(us2fs(sysPath), dirPrefix, fileName);
  // 		if (_folder->BindToFolder(fs2us(dirPrefix), &newFolder) == S_OK)
  // 		{
  // 			SetNewFolder(newFolder);
  // 			LoadFullPath();
  // 			{
  // 				HRESULT res = OpenItemAsArchive(fs2us(fileName), arcFormat, encrypted);
  // 				if (res != S_FALSE)
  // 				{
  // 					RINOK(res);
  // 				}
  // 				/*
  // 				if (res == E_ABORT)
  // 				return res;
  // 				*/
  // 				if (res == S_OK)
  // 				{
  // 					archiveIsOpened = true;
  // 					for (int i = reducedParts.Size() - 1; i >= 0; i--)
  // 					{
  // 						CMyComPtr<IFolderFolder> newFolder;
  // 						_folder->BindToFolder(reducedParts[i], &newFolder);
  // 						if (!newFolder)
  // 							break;
  // 						SetNewFolder(newFolder);
  // 					}
  // 				}
  // 			}
  // 		}
  // 	}
  return _folder;
}


UString GetFolderTypeID(CMyComPtr<IFolderFolder> _folder)
{
  {
    NCOM::CPropVariant prop;
    if (_folder->GetFolderProperty(kpidType, &prop) == S_OK)
      if (prop.vt == VT_BSTR)
        return (const wchar_t *)prop.bstrVal;
  }
  return UString();
}

bool IsFolderTypeEqTo(CMyComPtr<IFolderFolder> _folder, const wchar_t *s)
{
  return GetFolderTypeID(_folder) == s;
}


bool IsRootFolder(CMyComPtr<IFolderFolder> _folder)
{
  return IsFolderTypeEqTo(_folder, L"RootFolder");
}


bool IsFSFolder(CMyComPtr<IFolderFolder> _folder)
{
  return IsFolderTypeEqTo(_folder, L"FSFolder");
}


bool IsFSDrivesFolder(CMyComPtr<IFolderFolder> _folder)
{
  return IsFolderTypeEqTo(_folder, L"FSDrives");
}


bool IsNetFolder(CMyComPtr<IFolderFolder> _folder)
{
  return IsFolderTypeEqTo(_folder, L"NetFolder");
}


bool IsArcFolder(CMyComPtr<IFolderFolder> _folder)
{
  return GetFolderTypeID(_folder).IsPrefixedBy(L"EcoZip");
}

bool IsDeviceDrivesPrefix(CMyComPtr<IFolderFolder> folder)
{
  UString s = GetFolderPath(folder);
  return s == L"\\\\.\\";
}

bool IsSuperDrivesPrefix(CMyComPtr<IFolderFolder> folder)
{
  UString s = GetFolderPath(folder);
  return s == L"\\\\?\\";
}

UString GetFolderPath(IFolderFolder *folder)
{
  if (!folder)
    return UString();
  {
    NCOM::CPropVariant prop;
    if (folder->GetFolderProperty(kpidPath, &prop) == S_OK)
      if (prop.vt == VT_BSTR)
        return (wchar_t *)prop.bstrVal;
  }
  return UString();
}


int GetRealIconIndex(CFSTR path, DWORD attributes)
{
  int index = -1;
  if (GetRealIconIndex(path, attributes, index) != 0)
    return index;
  return -1;
}


bool AreEqualNames(const UString &path, const wchar_t *name)
{
  unsigned len = MyStringLen(name);
  if (len > path.Len() || len + 1 < path.Len())
    return false;
  if (len + 1 == path.Len() && path[len] != WCHAR_PATH_SEPARATOR)
    return false;
  return path.IsPrefixedBy(name);
}

// it's used in CompressDialog also
bool GetBoolsVal(const CBoolPair &b1, const CBoolPair &b2)
{
  if (b1.Def) return b1.Val;
  if (b2.Def) return b2.Val;
  return b1.Val;
}

// it's used in CompressDialog also
void AddComboItems(CComboBox &combo, const UInt32 *langIDs, unsigned numItems, const int *values, int curVal)
{
  int curSel = 0;
  for (unsigned i = 0; i < numItems; i++)
  {
    UString s = LangString(langIDs[i]);
    s.RemoveChar(L'&');
    int index = (int)combo.AddString(s);
    combo.SetItemData(index, i);
    if (values[i] == curVal)
      curSel = i;
  }
  combo.SetCurSel(curSel);
}

bool IsAsciiString(const UString &s)
{
  for (unsigned i = 0; i < s.Len(); i++)
  {
    wchar_t c = s[i];
    if (c < 0x20 || c > 0x7F)
      return false;
  }
  return true;
}

void AddUniqueString(UStringVector &list, const UString &s)
{
  FOR_VECTOR(i, list)
    if (s.IsEqualTo_NoCase(list[i]))
      return;
  list.Add(s);
}

int GetSortControlID(PROPID propID)
{
  switch (propID)
  {
  case kpidName:       return ID_VIEW_ARRANGE_BY_NAME;
  case kpidExtension:  return ID_VIEW_ARRANGE_BY_TYPE;
  case kpidMTime:      return ID_VIEW_ARRANGE_BY_DATE;
  case kpidSize:       return ID_VIEW_ARRANGE_BY_SIZE;
  case kpidNoProperty: return ID_VIEW_ARRANGE_NO_SORT;
  }
  return -1;
}

void ReduceString(UString &s, unsigned size)
{
  if (s.Len() <= size)
    return;
  s.Delete(size / 2, s.Len() - size);
  s.Insert(size / 2, L" ... ");
}

void ConvertIntToDecimalString(UInt64 value, wchar_t *dest)
{
  char s[32];
  ConvertUInt64ToString(value, s);
  unsigned i = MyStringLen(s);
  unsigned pos = ARRAY_SIZE(s);
  s[--pos] = 0;
  while (i > 3)
  {
    s[--pos] = s[--i];
    s[--pos] = s[--i];
    s[--pos] = s[--i];
    s[--pos] = L',';
  }
  while (i > 0)
    s[--pos] = s[--i];

  for (;;)
  {
    char c = s[pos++];
    *dest++ = (unsigned char)c;
    if (c == 0)
      break;
  }
}

UString ConvertIntToDecimalString(UInt64 value)
{
  wchar_t s[32];
  ConvertIntToDecimalString(value, s);
  return s;
}

#define INT_TO_STR_SPEC(v) \
      while (v >= 10) { temp[i++] = (unsigned char)('0' + (unsigned)(v % 10)); v /= 10; } \
  *s++ = (unsigned char)('0' + (unsigned)v);

void ConvertSizeToString(UInt64 val, wchar_t *s) throw()
{
  unsigned char temp[32];
  unsigned i = 0;

  if (val <= (UInt32)0xFFFFFFFF)
  {
    UInt32 val32 = (UInt32)val;
    INT_TO_STR_SPEC(val32)
  }
  else
  {
    INT_TO_STR_SPEC(val)
  }

  if (i < 3)
  {
    if (i != 0)
    {
      *s++ = temp[i - 1];
      if (i == 2)
        *s++ = temp[0];
    }
    *s = 0;
    return;
  }

  unsigned r = i % 3;
  if (r != 0)
  {
    s[0] = temp[--i];
    if (r == 2)
      s[1] = temp[--i];
    s += r;
  }

  do
  {
    s[0] = ' ';
    s[1] = temp[i - 1];
    s[2] = temp[i - 2];
    s[3] = temp[i - 3];
    s += 4;
  } while (i -= 3);

  *s = 0;
}

UString ConvertSizeToString(UInt64 value)
{
  wchar_t s[32];
  ConvertSizeToString(value, s);
  return s;
}

bool IsCorrectFsName(const UString &name)
{
  const UString lastPart = name.Ptr(name.ReverseFind_PathSepar() + 1);
  return
    lastPart != L"." &&
    lastPart != L"..";
}

#ifdef _WIN32

static void RemoveDotsAndSpaces(UString &path)
{
  while (!path.IsEmpty())
  {
    wchar_t c = path.Back();
    if (c != ' ' && c != '.')
      return;
    path.DeleteBack();
  }
}


bool CorrectFsPath(const UString &relBase, const UString &path2, UString &result)
{
  result.Empty();

  UString path = path2;
  path.Replace(L'/', WCHAR_PATH_SEPARATOR);
  unsigned start = 0;
  UString base;
  if (NName::IsAbsolutePath(path))
  {
#if defined(_WIN32) && !defined(UNDER_CE)
    if (NWindows::NFile::NName::IsSuperOrDevicePath(path))
    {
      result = path;
      return true;
    }
#endif
    int pos = NWindows::NFile::NName::GetRootPrefixSize(path);
    if (pos > 0)
      start = pos;
  }
  else
  {
#if defined(_WIN32) && !defined(UNDER_CE)
    if (NWindows::NFile::NName::IsSuperOrDevicePath(relBase))
    {
      result = path;
      return true;
    }
#endif
    base = relBase;
  }

  /* We can't use backward, since we must change only disk paths */
  /*
  for (;;)
  {
  if (path.Len() <= start)
  break;
  if (DoesFileOrDirExist(us2fs(path)))
  break;
  if (path.Back() == WCHAR_PATH_SEPARATOR)
  {
  path.DeleteBack();
  result.Insert(0, WCHAR_PATH_SEPARATOR);;
  }
  int pos = path.ReverseFind(WCHAR_PATH_SEPARATOR) + 1;
  UString cur = path.Ptr(pos);
  RemoveDotsAndSpaces(cur);
  result.Insert(0, cur);
  path.DeleteFrom(pos);
  }
  result.Insert(0, path);
  return true;
  */

  result += path.Left(start);
  bool checkExist = true;
  UString cur;

  for (;;)
  {
    if (start == path.Len())
      break;
    int slashPos = path.Find(WCHAR_PATH_SEPARATOR, start);
    cur.SetFrom(path.Ptr(start), (slashPos < 0 ? path.Len() : slashPos) - start);
    if (checkExist)
    {
      CFileInfo fi;
      if (fi.Find(us2fs(base + result + cur)))
      {
        if (!fi.IsDir())
        {
          result = path;
          break;
        }
      }
      else
        checkExist = false;
    }
    if (!checkExist)
      RemoveDotsAndSpaces(cur);
    result += cur;
    if (slashPos < 0)
      break;
    result.Add_PathSepar();
    start = slashPos + 1;
  }

  return true;
}

#else
bool CorrectFsPath(const UString & /* relBase */, const UString &path, UString &result)
{
  result = path;
  return true;
}
#endif

UString HResultToMessage(HRESULT errorCode)
{
  if (errorCode == E_OUTOFMEMORY)
    return LangString(IDS_MEM_ERROR);
  else
    return NWindows::NError::MyFormatMessage(errorCode);
  return L"";
}

bool IsLargePageSupported()
{
  #ifdef _WIN64
  return true;
  #else
  OSVERSIONINFO vi;
  vi.dwOSVersionInfoSize = sizeof(vi);
  if (!::GetVersionEx(&vi))
    return false;
  if (vi.dwPlatformId != VER_PLATFORM_WIN32_NT)
    return false;
  if (vi.dwMajorVersion < 5) return false;
  if (vi.dwMajorVersion > 5) return true;
  if (vi.dwMinorVersion < 1) return false;
  if (vi.dwMinorVersion > 1) return true;
  // return g_Is_Wow64;
  return false;
  #endif
}

#define UINT_TO_STR_2(val) { s[0] = (wchar_t)('0' + (val) / 10); s[1] = (wchar_t)('0' + (val) % 10); s += 2; }

void GetTimeString(UInt64 timeValue, wchar_t *s)
{
  UInt64 hours = timeValue / 3600;
  UInt32 seconds = (UInt32)(timeValue - hours * 3600);
  UInt32 minutes = seconds / 60;
  seconds %= 60;
  if (hours > 99)
  {
    ConvertUInt64ToString(hours, s);
    for (; *s != 0; s++);
  }
  else
  {
    UInt32 hours32 = (UInt32)hours;
    UINT_TO_STR_2(hours32);
  }
  *s++ = ':'; UINT_TO_STR_2(minutes);
  *s++ = ':'; UINT_TO_STR_2(seconds);
  *s = 0;
}

// reduces path to part that exists on disk (or root prefix of path)
// output path is normalized (with WCHAR_PATH_SEPARATOR)
void ReducePathToRealFileSystemPath(UString &path)
{
  unsigned prefixSize = NName::GetRootPrefixSize(path);

  while (!path.IsEmpty())
  {
    if (NFind::DoesDirExist(us2fs(path)))
    {
      NName::NormalizeDirPathPrefix(path);
      break;
    }
    int pos = path.ReverseFind_PathSepar();
    if (pos < 0)
    {
      path.Empty();
      break;
    }
    path.DeleteFrom(pos + 1);
    if ((unsigned)pos + 1 == prefixSize)
      break;
    path.DeleteFrom(pos);
  }
}

// returns: true, if such dir exists or is root
bool CheckFolderPath(const UString &path)
{
  UString pathReduced = path;
  ReducePathToRealFileSystemPath(pathReduced);
  return (pathReduced == path);
}


UString GetFsPath(CMyComPtr<IFolderFolder> folder)
{
  if (IsFSDrivesFolder(folder) && !IsDeviceDrivesPrefix(folder) && !IsSuperDrivesPrefix(folder))
    return UString();
  return GetFolderPath(folder);
}

UString GetItemName(CMyComPtr<IFolderFolder> folder, int itemIndex)
{
  if (itemIndex == kParentIndex)
    return L"..";
  NCOM::CPropVariant prop;
  if (folder->GetProperty(itemIndex, kpidName, &prop) != S_OK)
    throw 2723400;
  if (prop.vt != VT_BSTR)
    throw 2723401;
  return prop.bstrVal;
}

void GetItemName(CMyComPtr<IFolderFolder> folder, int itemIndex, UString &s)
{
  if (itemIndex == kParentIndex) {
    s = L"..";
    return;
  }
  NCOM::CPropVariant prop;
  if (folder->GetProperty(itemIndex, kpidName, &prop) != S_OK)
    throw 2723400;
  if (prop.vt != VT_BSTR)
    throw 2723401;
  s.SetFromBstr(prop.bstrVal);
}

UString GetItemPrefix(CMyComPtr<IFolderFolder> folder, int itemIndex)
{
  if (itemIndex == kParentIndex)
    return UString();
  NCOM::CPropVariant prop;
  if (folder->GetProperty(itemIndex, kpidPrefix, &prop) != S_OK)
    throw 2723400;
  UString prefix;
  if (prop.vt == VT_BSTR)
    prefix.SetFromBstr(prop.bstrVal);
  return prefix;
}

UString GetItemRelPath(CMyComPtr<IFolderFolder> folder, int itemIndex)
{
  return GetItemPrefix(folder, itemIndex) + GetItemName(folder, itemIndex);
}

UString GetItemRelPath2(CMyComPtr<IFolderFolder> folder, int itemIndex)
{
  UString s = GetItemRelPath(folder, itemIndex);
  #if defined(_WIN32) && !defined(UNDER_CE)
  if (s.Len() == 2 && NFile::NName::IsDrivePath2(s))
  {
    if (IsFSDrivesFolder(folder) && !IsDeviceDrivesPrefix(folder))
      s.Add_PathSepar();
  }
  #endif
  return s;
}

UString GetItemFullPath(CMyComPtr<IFolderFolder> folder, int itemIndex)
{
  return GetFsPath(folder) + GetItemRelPath(folder, itemIndex);
}

bool GetItem_BoolProp(CMyComPtr<IFolderFolder> folder, UInt32 itemIndex, PROPID propID)
{
  NCOM::CPropVariant prop;
  if (folder->GetProperty(itemIndex, propID, &prop) != S_OK)
    throw 2723400;
  if (prop.vt == VT_BOOL)
    return VARIANT_BOOLToBool(prop.boolVal);
  if (prop.vt == VT_EMPTY)
    return false;
  throw 2723401;
}

bool IsItem_Deleted(CMyComPtr<IFolderFolder> folder, int itemIndex)
{
  if (itemIndex == kParentIndex)
    return false;
  return GetItem_BoolProp(folder, itemIndex, kpidIsDeleted);
}

bool IsItem_Folder(CMyComPtr<IFolderFolder> folder, int itemIndex)
{
  if (itemIndex == kParentIndex)
    return true;
  return GetItem_BoolProp(folder, itemIndex, kpidIsDir);
}

bool IsItem_AltStream(CMyComPtr<IFolderFolder> folder, int itemIndex)
{
  if (itemIndex == kParentIndex)
    return false;
  return GetItem_BoolProp(folder, itemIndex, kpidIsAltStream);
}

#pragma comment(lib, "wbemuuid.lib")

HRESULT GetCpuTemperature(LPLONG pTemperature)
{
  if (pTemperature == NULL)
    return E_INVALIDARG;

  HRESULT hr;
  *pTemperature = -1;
  //   HRESULT ci = CoInitialize(NULL);
  //   HRESULT hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
  //   if (SUCCEEDED(hr))
  {
    IWbemLocator *pLocator;
    hr = CoCreateInstance(CLSID_WbemAdministrativeLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLocator);
    if (SUCCEEDED(hr))
    {
      IWbemServices *pServices;
      BSTR ns = SysAllocString(L"root\\WMI");
      hr = pLocator->ConnectServer(ns, NULL, NULL, NULL, 0, NULL, NULL, &pServices);
      pLocator->Release();
      SysFreeString(ns);
      if (SUCCEEDED(hr))
      {
        BSTR query = SysAllocString(L"SELECT * FROM MSAcpi_ThermalZoneTemperature");
        BSTR wql = SysAllocString(L"WQL");
        IEnumWbemClassObject *pEnum;
        hr = pServices->ExecQuery(wql, query, WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &pEnum);
        SysFreeString(wql);
        SysFreeString(query);
        pServices->Release();
        if (SUCCEEDED(hr))
        {
          IWbemClassObject *pObject = NULL;
          ULONG returned;
          hr = pEnum->Next(WBEM_INFINITE, 1, &pObject, &returned);
          pEnum->Release();
          if (SUCCEEDED(hr))
          {
            BSTR temp = SysAllocString(L"CurrentTemperature");
            VARIANT v;
            VariantInit(&v);
            hr = pObject->Get(temp, 0, &v, NULL, NULL);
            pObject->Release();
            SysFreeString(temp);
            if (SUCCEEDED(hr))
            {
              *pTemperature = V_I4(&v);
            }
            VariantClear(&v);
          }
        }
      }
      //       if (ci == S_OK)
      //       {
      //         CoUninitialize();
      //       }
    }
  }
  return hr;
}

HANDLE AddResourceFont(LPCTSTR ResID, DWORD *Installed)
{
  if (Installed) *Installed = 0;

  HMODULE hMod = GetModuleHandle(NULL);
  DWORD Count, ErrorCode;

  HRSRC Resource = FindResource(hMod, ResID, RT_FONT); // or RT_FONT or whatever your actual resource type is
  if (!Resource)
  {
    ErrorCode = GetLastError();
    //...
    return NULL;
  }

  DWORD Length = SizeofResource(hMod, Resource);
  if ((Length == 0) && (GetLastError() != 0))
  {
    ErrorCode = GetLastError();
    //...
    return NULL;
  }

  HGLOBAL Address = LoadResource(hMod, Resource);
  if (!Address)
  {
    ErrorCode = GetLastError();
    //...
    return NULL;
  }

  PVOID FontData = LockResource(Address);
  if (!FontData)
  {
    ErrorCode = GetLastError();
    //...
    return NULL;
  }

  HANDLE Handle = AddFontMemResourceEx(FontData, Length, 0, &Count);
  if (!Handle)
  {
    ErrorCode = GetLastError();
    //...
    return NULL;
  }

  if (Installed) *Installed = Count;
  return Handle;
}