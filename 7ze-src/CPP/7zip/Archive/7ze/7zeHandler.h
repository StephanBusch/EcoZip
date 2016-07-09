// 7z/Handler.h

#ifndef __7ZE_HANDLER_H
#define __7ZE_HANDLER_H

#include "../../ICoder.h"
#include "../IArchive.h"

#include "../../Common/CreateCoder.h"

#ifndef EXTRACT_ONLY
#include "../Common/HandlerOut.h"
#endif

#include "7zeCompressionMode.h"
#include "7zeIn.h"
#include "7zeGroup.h"
#include "7zeUpdate.h"

namespace NArchive {
namespace N7ze {

#ifndef __7ZE_SET_PROPERTIES

#ifdef EXTRACT_ONLY
  #if !defined(_7ZIP_ST) && !defined(_SFX)
    #define __7ZE_SET_PROPERTIES
  #endif
#else
  #define __7ZE_SET_PROPERTIES
#endif

#endif


#ifndef EXTRACT_ONLY

class COutHandler: public CMultiMethodProps
{
  HRESULT SetSolidFromString(const UString &s);
  HRESULT SetSolidFromPROPVARIANT(const PROPVARIANT &value);
public:
  bool _removeSfxBlock;
  
  UInt64 _numSolidFiles;
  UInt64 _numSolidBytes;
  bool _numSolidBytesDefined;
  bool _solidExtension;
  bool _useTypeSorting;

  bool _compressHeaders;
  bool _encryptHeadersSpecified;
  bool _encryptHeaders;
  // bool _useParents; 9.26

  CBoolPair Write_CTime;
  CBoolPair Write_ATime;
  CBoolPair Write_MTime;

  bool _useMultiThreadMixer;

  // bool _volumeMode;

  void InitSolidFiles() { _numSolidFiles = (UInt64)(Int64)(-1); }
  void InitSolidSize()  { _numSolidBytes = (UInt64)(Int64)(-1); }
  void InitSolid()
  {
    InitSolidFiles();
    InitSolidSize();
    _solidExtension = false;
    _numSolidBytesDefined = false;
  }

  virtual void InitProps();

  COutHandler() { InitProps(); }

  HRESULT SetProperty(const wchar_t *name, const PROPVARIANT &value);
};

#endif

class CHandler:
  public IInArchive,
  public IArchiveGetRawProps,
  #ifdef __7ZE_SET_PROPERTIES
  public ISetProperties,
  #endif
  #ifndef EXTRACT_ONLY
  public IOutArchive,
  #endif
  PUBLIC_ISetCompressCodecsInfo
  public CMyUnknownImp
  #ifndef EXTRACT_ONLY
  , public COutHandler
  #endif
{
public:
  MY_QUERYINTERFACE_BEGIN2(IInArchive)
  MY_QUERYINTERFACE_ENTRY(IArchiveGetRawProps)
  #ifdef __7ZE_SET_PROPERTIES
  MY_QUERYINTERFACE_ENTRY(ISetProperties)
  #endif
  #ifndef EXTRACT_ONLY
  MY_QUERYINTERFACE_ENTRY(IOutArchive)
  #endif
  QUERY_ENTRY_ISetCompressCodecsInfo
  MY_QUERYINTERFACE_END
  MY_ADDREF_RELEASE

  INTERFACE_IInArchive(;)
  INTERFACE_IArchiveGetRawProps(;)

  #ifdef __7ZE_SET_PROPERTIES
  STDMETHOD(SetProperties)(const wchar_t * const *names, const PROPVARIANT *values, UInt32 numProps);
  STDMETHOD(SetProperties)(Group *group);
private:
  STDMETHOD(AfterSetProperties)();
public:
  #endif

  #ifndef EXTRACT_ONLY
  INTERFACE_IOutArchive(;)
  #endif

  DECL_ISetCompressCodecsInfo

  CHandler();

private:
  CMyComPtr<IInStream> _inStream;
  NArchive::N7ze::CDbEx _db;
  
  #ifndef _NO_CRYPTO
  bool _isEncrypted;
  bool _passwordIsDefined;
  UString _password;
  #endif

  #ifdef EXTRACT_ONLY
  
  #ifdef __7ZE_SET_PROPERTIES
  UInt32 _numThreads;
  bool _useMultiThreadMixer;
  #endif

  UInt32 _crcSize;

  #else
  
  CRecordVector<CBond2> _bonds;

  HRESULT PropsMethod_To_FullMethod(CMethodFull &dest, const COneMethodInfo &m);
  HRESULT SetHeaderMethod(CCompressionMethodMode &headerMethod);

  HRESULT SetMethodName(CCompressionMethodMode &methodMode,
      const char *methodName
      #ifndef _7ZIP_ST
      , UInt32 numThreads
      #endif
      );
  HRESULT SetMainMethod(CCompressionMethodMode &methodMode
      #ifndef _7ZIP_ST
      , UInt32 numThreads
      #endif
      );

  HRESULT Update(
      DECL_EXTERNAL_CODECS_LOC_VARS
      IInStream *inStream,
      const CDbEx *db,
      const CObjectVector<CUpdateItem> &updateItems,
    // const CObjectVector<CTreeFolder> &treeFolders,
    // const CUniqBlocks &secureBlocks,
      COutArchive &archive,
      CArchiveDatabaseOut &newDatabase,
      ISequentialOutStream *seqOutStream,
      IArchiveUpdateCallback *updateCallback,
      const CUpdateOptions &options
      #ifndef _NO_CRYPTO
      , ICryptoGetTextPassword *getDecoderPassword
      #endif
      );

  #endif

  bool IsFolderEncrypted(CNum folderIndex) const;
  #ifndef _SFX

  CRecordVector<UInt64> _fileInfoPopIDs;
  void FillPopIDs();
  void AddMethodName(AString &s, UInt64 id);
  HRESULT SetMethodToProp(CNum folderIndex, PROPVARIANT *prop) const;

  #endif

  virtual void InitProps();

  bool bLoadConfig;
  Config _groups;
  UStringVector _defaultPropNames;
  CObjectVector<NWindows::NCOM::CPropVariant> _defaultPropValues;

  DECL_EXTERNAL_CODECS_VARS
};

}}

#endif
