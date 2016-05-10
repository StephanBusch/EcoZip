PROG = 7ze.dll
DEF_FILE = ../Archive.def
CFLAGS = $(CFLAGS) \
  -DEXTERNAL_CODECS \
  -DPROJ_7ZIPEX

AR_OBJS = \
  $O\ArchiveExports.obj \
  $O\DllExports.obj \

7Z_OBJS = \
  $O\7zSpecStream.obj \
7ZE_OBJS = \
  $O\7zeCompressionMode.obj \
  $O\7zeDecode.obj \
  $O\7zeEncode.obj \
  $O\7zeExtract.obj \
  $O\7zeFolderInStream.obj \
  $O\7zeFolderOutStream.obj \
  $O\7zeHandler.obj \
  $O\7zeHandlerOut.obj \
  $O\7zeHeader.obj \
  $O\7zeIn.obj \
  $O\7zeOut.obj \
  $O\7zeProperties.obj \
  $O\7zeUpdate.obj \
  $O\7zeRegister.obj \

COMMON_OBJS = \
  $O\CRC.obj \
  $O\IntToString.obj \
  $O\NewHandler.obj \
  $O\MyString.obj \
  $O\StringConvert.obj \
  $O\StringToInt.obj \
  $O\MyVector.obj \
  $O\Wildcard.obj \
  $O\GetImagePath.obj \

WIN_OBJS = \
  $O\DLL.obj \
  $O\FileDir.obj \
  $O\FileFind.obj \
  $O\FileIO.obj \
  $O\FileName.obj \
  $O\PropVariant.obj \
  $O\Synchronization.obj \
  $O\System.obj \

7ZIP_COMMON_OBJS = \
  $O\CreateCoder.obj \
  $O\InOutTempBuffer.obj \
  $O\FilterCoder.obj \
  $O\LimitedStreams.obj \
  $O\LockedStream.obj \
  $O\MethodId.obj \
  $O\MethodProps.obj \
  $O\OutBuffer.obj \
  $O\ProgressUtils.obj \
  $O\PropId.obj \
  $O\StreamBinder.obj \
  $O\StreamObjects.obj \
  $O\StreamUtils.obj \
  $O\VirtThread.obj \

COMPRESS_OBJS = \
  $O\CopyCoder.obj \

AR_COMMON_OBJS = \
  $O\CoderMixer2.obj \
  $O\CrossThreadProgress.obj \
  $O\HandlerOut.obj \
  $O\InStreamWithCRC.obj \
  $O\ItemNameUtils.obj \
  $O\MultiStream.obj \
  $O\OutStreamWithCRC.obj \
  $O\ParseProperties.obj \

C_OBJS = \
  $O\Alloc.obj \
  $O\CpuArch.obj \
  $O\Threads.obj \

!include "../../Crc.mak"

!include "../../7zip.mak"
