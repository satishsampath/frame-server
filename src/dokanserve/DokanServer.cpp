#include "AviFrameManagerImpl.h"
#include "AviFileWrapperImpl.h"
#include "DokanServer.h"
#include <dokan/dokan.h>
#include <dokan/fileinfo.h>

#if defined(_DEBUG) || defined(FSDEV)
#define DbgPrintA(x) OutputDebugStringA(x)

static void DbgPrint(LPCWSTR format, ...) {
  const wchar_t *outputString;
  wchar_t *buffer = NULL;
  size_t length;
  va_list argp;

  va_start(argp, format);
  length = _vscwprintf(format, argp) + 1;
  buffer = (wchar_t*)_malloca(length * sizeof(WCHAR));
  if (buffer) {
    vswprintf_s(buffer, length, format, argp);
    outputString = buffer;
  } else {
    outputString = format;
  }
  OutputDebugStringW(outputString);
  if (buffer)
    _freea(buffer);
  va_end(argp);
}

#define MirrorCheckFlag(val, flag)                                             \
  if (val & flag) {                                                            \
    DbgPrint(L"\t" L#flag L"\n");                                              \
  }

#else
#define DbgPrintA(x)
#define DbgPrint(...)
#define MirrorCheckFlag(val, flag)
#endif

class DokanServer {
public:
  DokanServer(std::wstring& mountDir, std::wstring& mirrorDir, AviFileWrapper* signpostAvif, std::wstring& signpostFilename);
  ~DokanServer();

  void Start();
  void Stop();
  DWORD ThreadProc();

  // Functions that handle signpost AVI. If the filename doesn't match signpost, they'll call the mirror
  // functions below automatically.
  NTSTATUS fs_createfile(LPCWSTR filename, PDOKAN_IO_SECURITY_CONTEXT securityContext,
    ACCESS_MASK desiredAccess, ULONG fileAttributes, ULONG shareAccess, ULONG createDisposition,
    ULONG createOptions, PDOKAN_FILE_INFO dokanFileInfo);
  NTSTATUS fs_getfileinformation(LPCWSTR filename, LPBY_HANDLE_FILE_INFORMATION buffer,
    PDOKAN_FILE_INFO dokanFileInfo);
  NTSTATUS fs_findfiles(LPCWSTR filename, PFillFindData fillFindData, PDOKAN_FILE_INFO dokanFileInfo);
  NTSTATUS fs_readfile(LPCWSTR filename, LPVOID buffer, DWORD bufferLength,
    LPDWORD readLength, LONGLONG offset, PDOKAN_FILE_INFO dokanFileInfo);

  // Functions that handle the mirror directory, called from within the corresponding functions above.
  NTSTATUS mirror_createfile(LPCWSTR filename, PDOKAN_IO_SECURITY_CONTEXT securityContext,
    ACCESS_MASK desiredAccess, ULONG fileAttributes, ULONG shareAccess, ULONG createDisposition,
    ULONG createOptions, PDOKAN_FILE_INFO dokanFileInfo);
  void mirror_closefile(LPCWSTR filename, PDOKAN_FILE_INFO dokanFileInfo);
  void mirror_cleanup(LPCWSTR filename, PDOKAN_FILE_INFO dokanFileInfo);
  NTSTATUS mirror_getfileinformation(LPCWSTR filename, LPBY_HANDLE_FILE_INFORMATION buffer,
    PDOKAN_FILE_INFO dokanFileInfo);
  NTSTATUS mirror_flushfilebuffers(LPCWSTR filename, PDOKAN_FILE_INFO dokanFileInfo);
  NTSTATUS mirror_readfile(LPCWSTR filename, LPVOID buffer, DWORD bufferLength,
    LPDWORD readLength, LONGLONG offset, PDOKAN_FILE_INFO dokanFileInfo);
  NTSTATUS mirror_writefile(LPCWSTR filename, LPCVOID buffer, DWORD numBytesToWrite,
    LPDWORD numBytesWritten, LONGLONG offset, PDOKAN_FILE_INFO dokanFileInfo);
  NTSTATUS mirror_findfiles(LPCWSTR filename, PFillFindData fillFindData, PDOKAN_FILE_INFO dokanFileInfo);
  NTSTATUS mirror_deletefile(LPCWSTR filename, PDOKAN_FILE_INFO dokanFileInfo);
  NTSTATUS mirror_deletedirectory(LPCWSTR filename, PDOKAN_FILE_INFO dokanFileInfo);
  NTSTATUS mirror_movefile(LPCWSTR filename, LPCWSTR newFilename, BOOL replaceIfExisting,
    PDOKAN_FILE_INFO dokanFileInfo);
  NTSTATUS mirror_lockfile(LPCWSTR filename, LONGLONG byteOffset, LONGLONG length,
    PDOKAN_FILE_INFO dokanFileInfo);
  NTSTATUS mirror_setendoffile(LPCWSTR filename, LONGLONG byteOffset, PDOKAN_FILE_INFO dokanFileInfo);
  NTSTATUS mirror_setallocationsize(LPCWSTR filename, LONGLONG allocSize, PDOKAN_FILE_INFO dokanFileInfo);
  NTSTATUS mirror_setfileattributes(LPCWSTR filename, DWORD fileAttributes, PDOKAN_FILE_INFO dokanFileInfo);
  NTSTATUS mirror_setfiletime(LPCWSTR filename, CONST FILETIME *creationTime,
      CONST FILETIME *lastAccessTime, CONST FILETIME *lastWriteTime, PDOKAN_FILE_INFO dokanFileInfo);
  NTSTATUS mirror_unlockfile(LPCWSTR filename, LONGLONG byteOffset, LONGLONG length,
      PDOKAN_FILE_INFO dokanFileInfo);

private:
  bool IsSignpostFilename(LPCWSTR filename);

private:
  std::wstring mountDir;
  std::wstring mirrorDir;
  std::wstring signpostFilename;
  HANDLE initDoneEvent;
  HANDLE threadHandle;
  AviFileWrapper* signpostAvif;
  FILETIME fileTime;
  int64_t filesize;
};

void ServeSignpostAndWait(std::wstring& mountDir, std::wstring& mirrorDir, AviFileWrapper* signpostAvif, std::wstring& signpostFilename) {
  DokanServer ds(mountDir, mirrorDir, signpostAvif, signpostFilename);
  ds.Start();

  MSG msg;
  while (GetMessage(&msg, 0, 0, 0) > 0) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  ds.Stop();
}

#define GET_DS_INSTANCE \
  reinterpret_cast<DokanServer*>(dokanFileInfo->DokanOptions->GlobalContext); DbgPrintA(__func__)
#define PRINT_AND_RETURN(x) \
  NTSTATUS ret = (x); DbgPrint(L"returns %X\n", ret); return ret

static NTSTATUS DOKAN_CALLBACK fs_getfileInformation(LPCWSTR filename, LPBY_HANDLE_FILE_INFORMATION buffer,
    PDOKAN_FILE_INFO dokanFileInfo) {
  auto ds = GET_DS_INSTANCE;
  PRINT_AND_RETURN(ds->fs_getfileinformation(filename, buffer, dokanFileInfo));
}

static NTSTATUS DOKAN_CALLBACK fs_findfiles(LPCWSTR filename, PFillFindData fillFindData, PDOKAN_FILE_INFO dokanFileInfo) {
  auto ds = GET_DS_INSTANCE;
  PRINT_AND_RETURN(ds->fs_findfiles(filename, fillFindData, dokanFileInfo));
}

static NTSTATUS DOKAN_CALLBACK fs_getfilesecurity(
    LPCWSTR filename, PSECURITY_INFORMATION security_information,
    PSECURITY_DESCRIPTOR security_descriptor, ULONG bufferLength,
    PULONG length_needed, PDOKAN_FILE_INFO dokanFileInfo) {
  // This will make dokan library return a default security descriptor
  return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS DOKAN_CALLBACK fs_createfile(LPCWSTR filename, PDOKAN_IO_SECURITY_CONTEXT securityContext,
    ACCESS_MASK desiredAccess, ULONG fileAttributes,
    ULONG shareAccess, ULONG createDisposition,
    ULONG createOptions, PDOKAN_FILE_INFO dokanFileInfo) {
  auto ds = GET_DS_INSTANCE;
  PRINT_AND_RETURN(ds->fs_createfile(filename, securityContext, desiredAccess, fileAttributes, shareAccess, createDisposition, createOptions, dokanFileInfo));
}

static void DOKAN_CALLBACK fs_closefile(LPCWSTR filename, PDOKAN_FILE_INFO dokanFileInfo) {
  auto ds = GET_DS_INSTANCE;
  ds->mirror_closefile(filename, dokanFileInfo);
}

static void DOKAN_CALLBACK fs_cleanup(LPCWSTR filename, PDOKAN_FILE_INFO dokanFileInfo) {
  auto ds = GET_DS_INSTANCE;
  ds->mirror_cleanup(filename, dokanFileInfo);
}

static NTSTATUS DOKAN_CALLBACK fs_readfile(LPCWSTR filename, LPVOID buffer, DWORD bufferLength,
    LPDWORD readLength, LONGLONG offset, PDOKAN_FILE_INFO dokanFileInfo) {
  auto ds = GET_DS_INSTANCE;
  PRINT_AND_RETURN(ds->fs_readfile(filename, buffer, bufferLength, readLength, offset, dokanFileInfo));
}

static NTSTATUS DOKAN_CALLBACK fs_flushfilebuffers(LPCWSTR filename, PDOKAN_FILE_INFO dokanFileInfo) {
  auto ds = GET_DS_INSTANCE;
  PRINT_AND_RETURN(ds->mirror_flushfilebuffers(filename, dokanFileInfo));
}

static NTSTATUS DOKAN_CALLBACK fs_writefile(LPCWSTR filename, LPCVOID buffer, DWORD numberOfBytesToWrite,
    LPDWORD numberOfBytesWritten, LONGLONG offset, PDOKAN_FILE_INFO dokanFileInfo) {
  auto ds = GET_DS_INSTANCE;
  PRINT_AND_RETURN(ds->mirror_writefile(filename, buffer, numberOfBytesToWrite, numberOfBytesWritten, offset, dokanFileInfo));
}

static NTSTATUS DOKAN_CALLBACK fs_deletefile(LPCWSTR filename, PDOKAN_FILE_INFO dokanFileInfo) {
  auto ds = GET_DS_INSTANCE;
  PRINT_AND_RETURN(ds->mirror_deletefile(filename, dokanFileInfo));
}

static NTSTATUS DOKAN_CALLBACK fs_deletedirectory(LPCWSTR filename, PDOKAN_FILE_INFO dokanFileInfo) {
  auto ds = GET_DS_INSTANCE;
  PRINT_AND_RETURN(ds->mirror_deletedirectory(filename, dokanFileInfo));
}

static NTSTATUS DOKAN_CALLBACK fs_movefile(LPCWSTR filename, LPCWSTR newfilename, BOOL replaceifexisting,
    PDOKAN_FILE_INFO dokanFileInfo) {
  auto ds = GET_DS_INSTANCE;
  PRINT_AND_RETURN(ds->mirror_movefile(filename, newfilename, replaceifexisting, dokanFileInfo));
}

static NTSTATUS DOKAN_CALLBACK fs_lockfile(LPCWSTR filename, LONGLONG byteoffset, LONGLONG length,
    PDOKAN_FILE_INFO dokanFileInfo) {
  auto ds = GET_DS_INSTANCE;
  PRINT_AND_RETURN(ds->mirror_lockfile(filename, byteoffset, length, dokanFileInfo));
}

static NTSTATUS DOKAN_CALLBACK fs_setendoffile(LPCWSTR filename, LONGLONG byteoffset,
    PDOKAN_FILE_INFO dokanFileInfo) {
  auto ds = GET_DS_INSTANCE;
  PRINT_AND_RETURN(ds->mirror_setendoffile(filename, byteoffset, dokanFileInfo));
}

static NTSTATUS DOKAN_CALLBACK fs_setallocationsize(LPCWSTR filename, LONGLONG allocsize,
    PDOKAN_FILE_INFO dokanFileInfo) {
  auto ds = GET_DS_INSTANCE;
  PRINT_AND_RETURN(ds->mirror_setallocationsize(filename, allocsize, dokanFileInfo));
}

static NTSTATUS DOKAN_CALLBACK fs_setfileattributes(LPCWSTR filename, DWORD fileAttributes,
    PDOKAN_FILE_INFO dokanFileInfo) {
  auto ds = GET_DS_INSTANCE;
  PRINT_AND_RETURN(ds->mirror_setfileattributes(filename, fileAttributes, dokanFileInfo));
}

static NTSTATUS DOKAN_CALLBACK fs_setfiletime(LPCWSTR filename, CONST FILETIME *creationtime,
    CONST FILETIME *lastaccesstime, CONST FILETIME *lastwritetime, PDOKAN_FILE_INFO dokanFileInfo) {
  auto ds = GET_DS_INSTANCE;
  PRINT_AND_RETURN(ds->mirror_setfiletime(filename, creationtime, lastaccesstime, lastwritetime, dokanFileInfo));
}

static NTSTATUS DOKAN_CALLBACK fs_unlockfile(LPCWSTR filename, LONGLONG byteoffset, LONGLONG length,
    PDOKAN_FILE_INFO dokanFileInfo) {
  auto ds = GET_DS_INSTANCE;
  PRINT_AND_RETURN(ds->mirror_unlockfile(filename, byteoffset, length, dokanFileInfo));
}

DOKAN_OPERATIONS dokanFsOperations = {
  fs_createfile,
  fs_cleanup,
  fs_closefile,
  fs_readfile,
  fs_writefile,
  fs_flushfilebuffers,
  fs_getfileInformation,
  fs_findfiles,
  0,// FindFilesWithPattern
  fs_setfileattributes,
  fs_setfiletime,
  fs_deletefile,
  fs_deletedirectory,
  fs_movefile,
  fs_setendoffile,
  fs_setallocationsize,
  fs_lockfile,
  fs_unlockfile,
  0,//memfs_getdiskfreespace,
  0,//memfs_getvolumeinformation,
  0,//memfs_mounted,
  0,//memfs_unmounted,
  fs_getfilesecurity,
  0,//memfs_setfilesecurity,
  0,//memfs_findstreams
};

DokanServer::DokanServer(std::wstring& mntd, std::wstring& mird, AviFileWrapper* af, std::wstring& sf) {
  mountDir = mntd;
  mirrorDir = mird;

  signpostFilename = sf;
  signpostAvif = af;
  threadHandle = NULL;
  initDoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

  SYSTEMTIME st;
  GetSystemTime(&st);
  SystemTimeToFileTime(&st, &fileTime);
  signpostAvif->seekg(0, std::ios::end);
  filesize = signpostAvif->tellg();
  signpostAvif->seekg(0);
}

bool DokanServer::IsSignpostFilename(LPCWSTR filename) {
  return (filename[0] == '\\' && _wcsicmp(filename + 1, signpostFilename.c_str()) == 0);
}

DokanServer::~DokanServer() {
  Stop();
}

DWORD WINAPI DokanServerThreadProc(LPVOID lp) {
  DokanServer* me = (DokanServer*)lp;
  return me->ThreadProc();
}

DWORD DokanServer::ThreadProc() {
  SetEvent(initDoneEvent);
  DOKAN_OPTIONS dokan_options;
  ZeroMemory(&dokan_options, sizeof(DOKAN_OPTIONS));
  dokan_options.Version = DOKAN_VERSION;
  dokan_options.Options = DOKAN_OPTION_MOUNT_MANAGER;
  dokan_options.MountPoint = mountDir.c_str();
  dokan_options.ThreadCount = 1;
  dokan_options.Timeout = 0;
  dokan_options.GlobalContext = reinterpret_cast<ULONG64>(this);
  NTSTATUS status = DokanMain(&dokan_options, &dokanFsOperations);
  return status;
}

void DokanServer::Start() {
  if (threadHandle)
    return;
  threadHandle = CreateThread(NULL, 0, DokanServerThreadProc, this, 0, NULL);
  WaitForSingleObject(initDoneEvent, INFINITE);
}

void DokanServer::Stop() {
  if (!threadHandle)
    return;
  //BOOL ret = DokanRemoveMountPoint(mountpoint);
  //if (WaitForSingleObject(threadHandle, 5000) == WAIT_TIMEOUT)
  //  TerminateThread(threadHandle, 0);
  CloseHandle(threadHandle);
  threadHandle = NULL;
}

void LlongToDwLowHigh(const int64_t& v, DWORD& low, DWORD& high) {
  high = v >> 32;
  low = static_cast<DWORD>(v);
}

NTSTATUS DokanServer::fs_getfileinformation(LPCWSTR filename, LPBY_HANDLE_FILE_INFORMATION buffer,
    PDOKAN_FILE_INFO dokanFileInfo) {
  if (!IsSignpostFilename(filename))
    return mirror_getfileinformation(filename, buffer, dokanFileInfo);

  DbgPrint(L"fs_getfileinformation %s\n", filename);

  std::wstring filePath = mirrorDir + L"\\vdub.avi";
  WIN32_FIND_DATAW find;
  ZeroMemory(&find, sizeof(WIN32_FIND_DATAW));
  HANDLE findHandle = FindFirstFile(filePath.c_str(), &find);
  buffer->dwFileAttributes = find.dwFileAttributes;
  buffer->ftCreationTime = find.ftCreationTime;
  buffer->ftLastAccessTime = find.ftLastAccessTime;
  buffer->ftLastWriteTime = find.ftLastWriteTime;
  LlongToDwLowHigh(filesize, buffer->nFileSizeLow, buffer->nFileSizeHigh);
  FindClose(findHandle);
  return STATUS_SUCCESS;


/*
  buffer->dwFileAttributes = FILE_ATTRIBUTE_READONLY;
  if (_wcsicmp(filename, L"\\") == 0) {
    buffer->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
    LlongToDwLowHigh(0, buffer->nFileIndexLow, buffer->nFileIndexHigh);
    LlongToDwLowHigh(0, buffer->nFileSizeLow, buffer->nFileSizeHigh);
  } else if (IsSignpostFilename(filename)) {
    buffer->dwFileAttributes |= FILE_ATTRIBUTE_NORMAL;
    LlongToDwLowHigh(1, buffer->nFileIndexLow, buffer->nFileIndexHigh);
    LlongToDwLowHigh(filesize, buffer->nFileSizeLow, buffer->nFileSizeHigh);
  } else {
    return STATUS_OBJECT_NAME_NOT_FOUND;
  }
  buffer->ftCreationTime = fileTime;
  buffer->ftLastAccessTime = fileTime;
  buffer->ftLastAccessTime = fileTime;
  buffer->nNumberOfLinks = 1;
  buffer->dwVolumeSerialNumber = 0x19770505;
*/
  return STATUS_SUCCESS;
}

NTSTATUS DokanServer::fs_findfiles(LPCWSTR filename, PFillFindData fillFindData, PDOKAN_FILE_INFO dokanFileInfo) {
  if (wcscmp(filename, L"\\") == 0) { // root folder
    WIN32_FIND_DATAW findData;
    ZeroMemory(&findData, sizeof(WIN32_FIND_DATAW));
    wcscpy_s(findData.cFileName, _countof(findData.cFileName), signpostFilename.c_str());
    GetShortPathName(findData.cFileName, findData.cAlternateFileName, _countof(findData.cAlternateFileName));
    findData.dwFileAttributes = FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_NORMAL;
    findData.ftCreationTime = fileTime;
    findData.ftLastAccessTime = fileTime;
    findData.ftLastWriteTime = fileTime;
    LlongToDwLowHigh(filesize, findData.nFileSizeLow, findData.nFileSizeHigh);
    fillFindData(&findData, dokanFileInfo);
  }
  return mirror_findfiles(filename, fillFindData, dokanFileInfo);
}

NTSTATUS DokanServer::fs_createfile(LPCWSTR filename, PDOKAN_IO_SECURITY_CONTEXT securityContext,
    ACCESS_MASK desiredAccess, ULONG fileAttributes, ULONG shareAccess, ULONG createDisposition,
    ULONG createOptions, PDOKAN_FILE_INFO dokanFileInfo) {
  if (!IsSignpostFilename(filename)) {
    return mirror_createfile(filename, securityContext, desiredAccess, fileAttributes,
      shareAccess, createDisposition, createOptions, dokanFileInfo);
  }

  ACCESS_MASK generic_desiredaccess;
  DWORD creation_disposition;
  DWORD file_attributes_and_flags;

  DokanMapKernelToUserCreateFileFlags(
    desiredAccess, fileAttributes, createOptions, createDisposition,
    &generic_desiredaccess, &file_attributes_and_flags,
    &creation_disposition);
/*
  // Windows will automatically try to create and access different system
  // directories.
  if (_wcsicmp(filename, L"\\System Volume Information") == 0 ||
      _wcsicmp(filename, L"\\$RECYCLE.BIN") == 0) {
    return STATUS_NO_SUCH_FILE;
  }
  if (_wcsicmp(filename, L"\\") == 0) {
    dokanFileInfo->IsDirectory = true;
    return STATUS_SUCCESS;
  }
*/
  // Only allow opening the signpost file, not creating a new one in the same place.
  DbgPrint(L"fs_createfile creation_disposition=%X\n", creation_disposition);
  if (creation_disposition != OPEN_EXISTING && creation_disposition != OPEN_ALWAYS)
    return STATUS_ACCESS_DENIED;
  if (dokanFileInfo->IsDirectory)
    return STATUS_NOT_A_DIRECTORY;

  return STATUS_SUCCESS;

}

NTSTATUS DokanServer::fs_readfile(LPCWSTR filename, LPVOID buffer, DWORD bufferLength,
    LPDWORD readLength, LONGLONG offset, PDOKAN_FILE_INFO dokanFileInfo) {
  if (!IsSignpostFilename(filename))
    return mirror_readfile(filename, buffer, bufferLength, readLength, offset, dokanFileInfo);

  DbgPrint(L"fs_readfile offset=%lld, length=%u\n", offset, bufferLength);
  signpostAvif->seekg(offset);
  bufferLength = (DWORD) max(0, min(filesize, offset + bufferLength) - offset);
  *readLength = bufferLength;
  signpostAvif->read((char*)buffer, bufferLength);
  DbgPrint(L"\treadLength = %u\n", *readLength);
  return STATUS_SUCCESS;
}

NTSTATUS DokanServer::mirror_createfile(LPCWSTR filename, PDOKAN_IO_SECURITY_CONTEXT securityContext,
    ACCESS_MASK desiredAccess, ULONG fileAttributes,
    ULONG shareAccess, ULONG createDisposition,
    ULONG createOptions, PDOKAN_FILE_INFO dokanFileInfo) {
  HANDLE handle;
  DWORD fileAttr;
  NTSTATUS status = STATUS_SUCCESS;
  DWORD creationDisposition;
  DWORD fileAttributesAndFlags;
  DWORD error = 0;
  SECURITY_ATTRIBUTES securityAttrib;
  ACCESS_MASK genericDesiredAccess;
  // userTokenHandle is for Impersonate Caller User Option
  HANDLE userTokenHandle = INVALID_HANDLE_VALUE;

  securityAttrib.nLength = sizeof(securityAttrib);
  securityAttrib.lpSecurityDescriptor =
    securityContext->AccessState.SecurityDescriptor;
  securityAttrib.bInheritHandle = FALSE;

  DokanMapKernelToUserCreateFileFlags(
    desiredAccess, fileAttributes, createOptions, createDisposition,
    &genericDesiredAccess, &fileAttributesAndFlags, &creationDisposition);

  std::wstring filePath = mirrorDir + filename;

  DbgPrint(L"CreateFile : %s\n", filePath.c_str());

  DbgPrint(L"\tShareMode = 0x%x\n", shareAccess);

  MirrorCheckFlag(shareAccess, FILE_SHARE_READ);
  MirrorCheckFlag(shareAccess, FILE_SHARE_WRITE);
  MirrorCheckFlag(shareAccess, FILE_SHARE_DELETE);

  DbgPrint(L"\tDesiredAccess = 0x%x\n", desiredAccess);

  MirrorCheckFlag(desiredAccess, GENERIC_READ);
  MirrorCheckFlag(desiredAccess, GENERIC_WRITE);
  MirrorCheckFlag(desiredAccess, GENERIC_EXECUTE);

  MirrorCheckFlag(desiredAccess, DELETE);
  MirrorCheckFlag(desiredAccess, FILE_READ_DATA);
  MirrorCheckFlag(desiredAccess, FILE_READ_ATTRIBUTES);
  MirrorCheckFlag(desiredAccess, FILE_READ_EA);
  MirrorCheckFlag(desiredAccess, READ_CONTROL);
  MirrorCheckFlag(desiredAccess, FILE_WRITE_DATA);
  MirrorCheckFlag(desiredAccess, FILE_WRITE_ATTRIBUTES);
  MirrorCheckFlag(desiredAccess, FILE_WRITE_EA);
  MirrorCheckFlag(desiredAccess, FILE_APPEND_DATA);
  MirrorCheckFlag(desiredAccess, WRITE_DAC);
  MirrorCheckFlag(desiredAccess, WRITE_OWNER);
  MirrorCheckFlag(desiredAccess, SYNCHRONIZE);
  MirrorCheckFlag(desiredAccess, FILE_EXECUTE);
  MirrorCheckFlag(desiredAccess, STANDARD_RIGHTS_READ);
  MirrorCheckFlag(desiredAccess, STANDARD_RIGHTS_WRITE);
  MirrorCheckFlag(desiredAccess, STANDARD_RIGHTS_EXECUTE);

  // When filePath is a directory, needs to change the flag so that the file can
  // be opened.
  fileAttr = GetFileAttributes(filePath.c_str());

  if (fileAttr != INVALID_FILE_ATTRIBUTES &&
    fileAttr & FILE_ATTRIBUTE_DIRECTORY) {
    if (createOptions & FILE_NON_DIRECTORY_FILE) {
      DbgPrint(L"\tCannot open a dir as a file\n");
      return STATUS_FILE_IS_A_DIRECTORY;
    }
    dokanFileInfo->IsDirectory = TRUE;
    // Needed by FindFirstFile to list files in it
    // TODO: use ReOpenFile in MirrorFindFiles to set share read temporary
    shareAccess |= FILE_SHARE_READ;
  }

  DbgPrint(L"\tFlagsAndAttributes = 0x%x\n", fileAttributesAndFlags);

  MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_ARCHIVE);
  MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_COMPRESSED);
  MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_DEVICE);
  MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_DIRECTORY);
  MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_ENCRYPTED);
  MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_HIDDEN);
  MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_INTEGRITY_STREAM);
  MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_NORMAL);
  MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_NOT_CONTENT_INDEXED);
  MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_NO_SCRUB_DATA);
  MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_OFFLINE);
  MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_READONLY);
  MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_REPARSE_POINT);
  MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_SPARSE_FILE);
  MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_SYSTEM);
  MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_TEMPORARY);
  MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_VIRTUAL);
  MirrorCheckFlag(fileAttributesAndFlags, FILE_FLAG_WRITE_THROUGH);
  MirrorCheckFlag(fileAttributesAndFlags, FILE_FLAG_OVERLAPPED);
  MirrorCheckFlag(fileAttributesAndFlags, FILE_FLAG_NO_BUFFERING);
  MirrorCheckFlag(fileAttributesAndFlags, FILE_FLAG_RANDOM_ACCESS);
  MirrorCheckFlag(fileAttributesAndFlags, FILE_FLAG_SEQUENTIAL_SCAN);
  MirrorCheckFlag(fileAttributesAndFlags, FILE_FLAG_DELETE_ON_CLOSE);
  MirrorCheckFlag(fileAttributesAndFlags, FILE_FLAG_BACKUP_SEMANTICS);
  MirrorCheckFlag(fileAttributesAndFlags, FILE_FLAG_POSIX_SEMANTICS);
  MirrorCheckFlag(fileAttributesAndFlags, FILE_FLAG_OPEN_REPARSE_POINT);
  MirrorCheckFlag(fileAttributesAndFlags, FILE_FLAG_OPEN_NO_RECALL);
  MirrorCheckFlag(fileAttributesAndFlags, SECURITY_ANONYMOUS);
  MirrorCheckFlag(fileAttributesAndFlags, SECURITY_IDENTIFICATION);
  MirrorCheckFlag(fileAttributesAndFlags, SECURITY_IMPERSONATION);
  MirrorCheckFlag(fileAttributesAndFlags, SECURITY_DELEGATION);
  MirrorCheckFlag(fileAttributesAndFlags, SECURITY_CONTEXT_TRACKING);
  MirrorCheckFlag(fileAttributesAndFlags, SECURITY_EFFECTIVE_ONLY);
  MirrorCheckFlag(fileAttributesAndFlags, SECURITY_SQOS_PRESENT);

  if (creationDisposition == CREATE_NEW) {
    DbgPrint(L"\tCREATE_NEW\n");
  } else if (creationDisposition == OPEN_ALWAYS) {
    DbgPrint(L"\tOPEN_ALWAYS\n");
  } else if (creationDisposition == CREATE_ALWAYS) {
    DbgPrint(L"\tCREATE_ALWAYS\n");
  } else if (creationDisposition == OPEN_EXISTING) {
    DbgPrint(L"\tOPEN_EXISTING\n");
  } else if (creationDisposition == TRUNCATE_EXISTING) {
    DbgPrint(L"\tTRUNCATE_EXISTING\n");
  } else {
    DbgPrint(L"\tUNKNOWN creationDisposition!\n");
  }

  if (dokanFileInfo->IsDirectory) {
    // It is a create directory request

    if (creationDisposition == CREATE_NEW ||
      creationDisposition == OPEN_ALWAYS) {

      //We create folder
      if (!CreateDirectory(filePath.c_str(), &securityAttrib)) {
        error = GetLastError();
        // Fail to create folder for OPEN_ALWAYS is not an error
        if (error != ERROR_ALREADY_EXISTS ||
          creationDisposition == CREATE_NEW) {
          DbgPrint(L"\terror code = %d\n\n", error);
          status = DokanNtStatusFromWin32(error);
        }
      }
    }

    if (status == STATUS_SUCCESS) {
      //Check first if we're trying to open a file as a directory.
      if (fileAttr != INVALID_FILE_ATTRIBUTES &&
        !(fileAttr & FILE_ATTRIBUTE_DIRECTORY) &&
        (createOptions & FILE_DIRECTORY_FILE)) {
        return STATUS_NOT_A_DIRECTORY;
      }

      // FILE_FLAG_BACKUP_SEMANTICS is required for opening directory handles
      handle =
        CreateFile(filePath.c_str(), genericDesiredAccess, shareAccess,
          &securityAttrib, OPEN_EXISTING,
          fileAttributesAndFlags | FILE_FLAG_BACKUP_SEMANTICS, NULL);

      if (handle == INVALID_HANDLE_VALUE) {
        error = GetLastError();
        DbgPrint(L"\terror code = %d\n\n", error);

        status = DokanNtStatusFromWin32(error);
      } else {
        dokanFileInfo->Context =
          (ULONG64)handle; // save the file handle in Context

        // Open succeed but we need to inform the driver
        // that the dir open and not created by returning STATUS_OBJECT_NAME_COLLISION
        if (creationDisposition == OPEN_ALWAYS &&
          fileAttr != INVALID_FILE_ATTRIBUTES)
          return STATUS_OBJECT_NAME_COLLISION;
      }
    }
  } else {
    // It is a create file request

    // Cannot overwrite a hidden or system file if flag not set
    if (fileAttr != INVALID_FILE_ATTRIBUTES &&
      ((!(fileAttributesAndFlags & FILE_ATTRIBUTE_HIDDEN) &&
      (fileAttr & FILE_ATTRIBUTE_HIDDEN)) ||
        (!(fileAttributesAndFlags & FILE_ATTRIBUTE_SYSTEM) &&
        (fileAttr & FILE_ATTRIBUTE_SYSTEM))) &&
          (creationDisposition == TRUNCATE_EXISTING ||
            creationDisposition == CREATE_ALWAYS))
      return STATUS_ACCESS_DENIED;

    // Cannot delete a read only file
    if ((fileAttr != INVALID_FILE_ATTRIBUTES &&
      (fileAttr & FILE_ATTRIBUTE_READONLY) ||
      (fileAttributesAndFlags & FILE_ATTRIBUTE_READONLY)) &&
      (fileAttributesAndFlags & FILE_FLAG_DELETE_ON_CLOSE))
      return STATUS_CANNOT_DELETE;

    // Truncate should always be used with write access
    if (creationDisposition == TRUNCATE_EXISTING)
      genericDesiredAccess |= GENERIC_WRITE;

    handle = CreateFile(
      filePath.c_str(),
      genericDesiredAccess, // GENERIC_READ|GENERIC_WRITE|GENERIC_EXECUTE,
      shareAccess,
      &securityAttrib, // security attribute
      creationDisposition,
      fileAttributesAndFlags, // |FILE_FLAG_NO_BUFFERING,
      NULL);                  // template file handle

    if (handle == INVALID_HANDLE_VALUE) {
      error = GetLastError();
      DbgPrint(L"\terror code = %d\n\n", error);

      status = DokanNtStatusFromWin32(error);
    } else {

      //Need to update fileAttributes with previous when Overwrite file
      if (fileAttr != INVALID_FILE_ATTRIBUTES &&
        creationDisposition == TRUNCATE_EXISTING) {
        SetFileAttributes(filePath.c_str(), fileAttributesAndFlags | fileAttr);
      }

      dokanFileInfo->Context =
        (ULONG64)handle; // save the file handle in Context

      if (creationDisposition == OPEN_ALWAYS ||
        creationDisposition == CREATE_ALWAYS) {
        error = GetLastError();
        if (error == ERROR_ALREADY_EXISTS) {
          DbgPrint(L"\tOpen an already existing file\n");
          // Open succeed but we need to inform the driver
          // that the file open and not created by returning STATUS_OBJECT_NAME_COLLISION
          status = STATUS_OBJECT_NAME_COLLISION;
        }
      }
    }
  }

  DbgPrint(L"\n");
  return status;
}

#pragma warning(push)
#pragma warning(disable : 4305)

void DokanServer::mirror_closefile(LPCWSTR filename, PDOKAN_FILE_INFO dokanFileInfo) {
  if (IsSignpostFilename(filename))
    return;

  std::wstring filePath = mirrorDir + filename;

  if (dokanFileInfo->Context) {
    DbgPrint(L"CloseFile: %s\n", filePath.c_str());
    DbgPrint(L"\terror : not cleanuped file\n\n");
    CloseHandle((HANDLE)dokanFileInfo->Context);
    dokanFileInfo->Context = 0;
  } else {
    DbgPrint(L"Close: %s\n\n", filePath.c_str());
  }
}

void DokanServer::mirror_cleanup(LPCWSTR filename, PDOKAN_FILE_INFO dokanFileInfo) {
  if (IsSignpostFilename(filename))
    return;

  std::wstring filePath = mirrorDir + filename;

  if (dokanFileInfo->Context) {
    DbgPrint(L"Cleanup: %s\n\n", filePath.c_str());
    CloseHandle((HANDLE)(dokanFileInfo->Context));
    dokanFileInfo->Context = 0;
  } else {
    DbgPrint(L"Cleanup: %s\n\tinvalid handle\n\n", filePath.c_str());
  }

  if (dokanFileInfo->DeleteOnClose) {
    // Should already be deleted by CloseHandle
    // if open with FILE_FLAG_DELETE_ON_CLOSE
    DbgPrint(L"\tDeleteOnClose\n");
    if (dokanFileInfo->IsDirectory) {
      DbgPrint(L"  DeleteDirectory ");
      if (!RemoveDirectory(filePath.c_str())) {
        DbgPrint(L"error code = %d\n\n", GetLastError());
      } else {
        DbgPrint(L"success\n\n");
      }
    } else {
      DbgPrint(L"  DeleteFile ");
      if (DeleteFile(filePath.c_str()) == 0) {
        DbgPrint(L" error code = %d\n\n", GetLastError());
      } else {
        DbgPrint(L"success\n\n");
      }
    }
  }
}

NTSTATUS DokanServer::mirror_readfile(LPCWSTR filename, LPVOID buffer, DWORD bufferLength,
    LPDWORD readLength, LONGLONG offset, PDOKAN_FILE_INFO dokanFileInfo) {
  HANDLE handle = (HANDLE)dokanFileInfo->Context;
  BOOL opened = FALSE;

  std::wstring filePath = mirrorDir + filename;

  DbgPrint(L"ReadFile : %s\n", filePath.c_str());

  if (!handle || handle == INVALID_HANDLE_VALUE) {
    DbgPrint(L"\tinvalid handle, cleanuped?\n");
    handle = CreateFile(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
      OPEN_EXISTING, 0, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
      DWORD error = GetLastError();
      DbgPrint(L"\tCreateFile error : %d\n\n", error);
      return DokanNtStatusFromWin32(error);
    }
    opened = TRUE;
  }

  LARGE_INTEGER distanceToMove;
  distanceToMove.QuadPart = offset;
  if (!SetFilePointerEx(handle, distanceToMove, NULL, FILE_BEGIN)) {
    DWORD error = GetLastError();
    DbgPrint(L"\tseek error, offset = %lld\n\n", offset);
    if (opened)
      CloseHandle(handle);
    return DokanNtStatusFromWin32(error);
  }

  if (!ReadFile(handle, buffer, bufferLength, readLength, NULL)) {
    DWORD error = GetLastError();
    DbgPrint(L"\tread error = %u, buffer length = %d, read length = %d\n\n",
      error, bufferLength, *readLength);
    if (opened)
      CloseHandle(handle);
    return DokanNtStatusFromWin32(error);

  } else {
    DbgPrint(L"\tByte to read: %d, Byte read %d, offset %lld\n\n", bufferLength,
      *readLength, offset);
  }

  if (opened)
    CloseHandle(handle);

  return STATUS_SUCCESS;
}

NTSTATUS DokanServer::mirror_writefile(LPCWSTR filename, LPCVOID buffer, DWORD numBytesToWrite,
    LPDWORD numBytesWritten, LONGLONG offset, PDOKAN_FILE_INFO dokanFileInfo) {
  if (IsSignpostFilename(filename))
    return STATUS_ACCESS_DENIED;

  HANDLE handle = (HANDLE)dokanFileInfo->Context;
  BOOL opened = FALSE;

  std::wstring filePath = mirrorDir + filename;

  DbgPrint(L"WriteFile : %s, offset %I64d, length %d\n", filePath.c_str(), offset,
    numBytesToWrite);

  // reopen the file
  if (!handle || handle == INVALID_HANDLE_VALUE) {
    DbgPrint(L"\tinvalid handle, cleanuped?\n");
    handle = CreateFile(filePath.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
      OPEN_EXISTING, 0, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
      DWORD error = GetLastError();
      DbgPrint(L"\tCreateFile error : %d\n\n", error);
      return DokanNtStatusFromWin32(error);
    }
    opened = TRUE;
  }

  UINT64 fileSize = 0;
  DWORD fileSizeLow = 0;
  DWORD fileSizeHigh = 0;
  fileSizeLow = GetFileSize(handle, &fileSizeHigh);
  if (fileSizeLow == INVALID_FILE_SIZE) {
    DWORD error = GetLastError();
    DbgPrint(L"\tcan not get a file size error = %d\n", error);
    if (opened)
      CloseHandle(handle);
    return DokanNtStatusFromWin32(error);
  }

  fileSize = ((UINT64)fileSizeHigh << 32) | fileSizeLow;

  LARGE_INTEGER distanceToMove;
  if (dokanFileInfo->WriteToEndOfFile) {
    LARGE_INTEGER z;
    z.QuadPart = 0;
    if (!SetFilePointerEx(handle, z, NULL, FILE_END)) {
      DWORD error = GetLastError();
      DbgPrint(L"\tseek error, offset = EOF, error = %d\n", error);
      if (opened)
        CloseHandle(handle);
      return DokanNtStatusFromWin32(error);
    }
  } else {
    // Paging IO cannot write after allocate file size.
    if (dokanFileInfo->PagingIo) {
      if ((UINT64)offset >= fileSize) {
        *numBytesWritten = 0;
        if (opened)
          CloseHandle(handle);
        return STATUS_SUCCESS;
      }

      if (((UINT64)offset + numBytesToWrite) > fileSize) {
        UINT64 bytes = fileSize - offset;
        if (bytes >> 32) {
          numBytesToWrite = (DWORD)(bytes & 0xFFFFFFFFUL);
        } else {
          numBytesToWrite = (DWORD)bytes;
        }
      }
    }

    if ((UINT64)offset > fileSize) {
      // In the mirror sample helperZeroFileData is not necessary. NTFS will
      // zero a hole.
      // But if user's file system is different from NTFS( or other Windows's
      // file systems ) then  users will have to zero the hole themselves.
    }

    distanceToMove.QuadPart = offset;
    if (!SetFilePointerEx(handle, distanceToMove, NULL, FILE_BEGIN)) {
      DWORD error = GetLastError();
      DbgPrint(L"\tseek error, offset = %I64d, error = %d\n", offset, error);
      if (opened)
        CloseHandle(handle);
      return DokanNtStatusFromWin32(error);
    }
  }

  if (!WriteFile(handle, buffer, numBytesToWrite, numBytesWritten,
    NULL)) {
    DWORD error = GetLastError();
    DbgPrint(L"\twrite error = %u, buffer length = %d, write length = %d\n",
      error, numBytesToWrite, *numBytesWritten);
    if (opened)
      CloseHandle(handle);
    return DokanNtStatusFromWin32(error);

  } else {
    DbgPrint(L"\twrite %d, offset %I64d\n\n", *numBytesWritten, offset);
  }

  // close the file when it is reopened
  if (opened)
    CloseHandle(handle);

  return STATUS_SUCCESS;
}

NTSTATUS DokanServer::mirror_flushfilebuffers(LPCWSTR filename, PDOKAN_FILE_INFO dokanFileInfo) {
  if (IsSignpostFilename(filename))
    return STATUS_SUCCESS;

  HANDLE handle = (HANDLE)dokanFileInfo->Context;

  std::wstring filePath = mirrorDir + filename;

  DbgPrint(L"FlushFileBuffers : %s\n", filePath.c_str());

  if (!handle || handle == INVALID_HANDLE_VALUE) {
    DbgPrint(L"\tinvalid handle\n\n");
    return STATUS_SUCCESS;
  }

  if (FlushFileBuffers(handle)) {
    return STATUS_SUCCESS;
  } else {
    DWORD error = GetLastError();
    DbgPrint(L"\tflush error code = %d\n", error);
    return DokanNtStatusFromWin32(error);
  }
}

NTSTATUS DokanServer::mirror_getfileinformation(
    LPCWSTR filename, LPBY_HANDLE_FILE_INFORMATION HandleFileInformation,
    PDOKAN_FILE_INFO dokanFileInfo) {
  HANDLE handle = (HANDLE)dokanFileInfo->Context;
  BOOL opened = FALSE;

  std::wstring filePath = mirrorDir + filename;

  DbgPrint(L"GetFileInfo : %s\n", filePath.c_str());

  if (!handle || handle == INVALID_HANDLE_VALUE) {
    DbgPrint(L"\tinvalid handle, cleanuped?\n");
    handle = CreateFile(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
      OPEN_EXISTING, 0, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
      DWORD error = GetLastError();
      DbgPrint(L"\tCreateFile error : %d\n\n", error);
      return DokanNtStatusFromWin32(error);
    }
    opened = TRUE;
  }

  if (!GetFileInformationByHandle(handle, HandleFileInformation)) {
    DbgPrint(L"\terror code = %d\n", GetLastError());

    // filename is a root directory
    // in this case, FindFirstFile can't get directory information
    if (wcslen(filename) == 1) {
      DbgPrint(L"  root dir\n");
      HandleFileInformation->dwFileAttributes = GetFileAttributes(filePath.c_str());
    } else {
      WIN32_FIND_DATAW find;
      ZeroMemory(&find, sizeof(WIN32_FIND_DATAW));
      HANDLE findHandle = FindFirstFile(filePath.c_str(), &find);
      if (findHandle == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        DbgPrint(L"\tFindFirstFile error code = %d\n\n", error);
        if (opened)
          CloseHandle(handle);
        return DokanNtStatusFromWin32(error);
      }
      HandleFileInformation->dwFileAttributes = find.dwFileAttributes;
      HandleFileInformation->ftCreationTime = find.ftCreationTime;
      HandleFileInformation->ftLastAccessTime = find.ftLastAccessTime;
      HandleFileInformation->ftLastWriteTime = find.ftLastWriteTime;
      HandleFileInformation->nFileSizeHigh = find.nFileSizeHigh;
      HandleFileInformation->nFileSizeLow = find.nFileSizeLow;
      DbgPrint(L"\tFindFiles OK, file size = %d\n", find.nFileSizeLow);
      FindClose(findHandle);
    }
  } else {
    DbgPrint(L"\tGetFileInformationByHandle success, file size = %d\n",
      HandleFileInformation->nFileSizeLow);
  }

  DbgPrint(L"FILE ATTRIBUTE  = %d\n", HandleFileInformation->dwFileAttributes);

  if (opened)
    CloseHandle(handle);

  return STATUS_SUCCESS;
}

NTSTATUS DokanServer::mirror_findfiles(LPCWSTR filename, PFillFindData fillFindData,  PDOKAN_FILE_INFO dokanFileInfo) {
  HANDLE hFind;
  WIN32_FIND_DATAW findData;
  DWORD error;
  int count = 0;

  std::wstring filePath = mirrorDir + filename;

  DbgPrint(L"FindFiles : %s\n", filePath.c_str());

  if (filePath.back() != L'\\')
    filePath += L"\\";
  filePath += L"*";

  hFind = FindFirstFile(filePath.c_str(), &findData);

  if (hFind == INVALID_HANDLE_VALUE) {
    error = GetLastError();
    DbgPrint(L"\tinvalid file handle. Error is %u\n\n", error);
    return DokanNtStatusFromWin32(error);
  }

  // Root folder does not have . and .. folder - we remove them
  BOOLEAN rootFolder = (wcscmp(filename, L"\\") == 0);
  do {
    if (!rootFolder || (wcscmp(findData.cFileName, L".") != 0 &&
      wcscmp(findData.cFileName, L"..") != 0))
      fillFindData(&findData, dokanFileInfo);
    count++;
  } while (FindNextFile(hFind, &findData) != 0);

  error = GetLastError();
  FindClose(hFind);

  if (error != ERROR_NO_MORE_FILES) {
    DbgPrint(L"\tFindNextFile error. Error is %u\n\n", error);
    return DokanNtStatusFromWin32(error);
  }

  DbgPrint(L"\tFindFiles return %d entries in %s\n\n", count, filePath.c_str());

  return STATUS_SUCCESS;
}

NTSTATUS DokanServer::mirror_deletefile(LPCWSTR filename, PDOKAN_FILE_INFO dokanFileInfo) {
  HANDLE handle = (HANDLE)dokanFileInfo->Context;
  std::wstring filePath = mirrorDir + filename;
  DbgPrint(L"DeleteFile %s - %d\n", filePath.c_str(), dokanFileInfo->DeleteOnClose);

  DWORD dwAttrib = GetFileAttributes(filePath.c_str());

  if (dwAttrib != INVALID_FILE_ATTRIBUTES &&
    (dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
    return STATUS_ACCESS_DENIED;

  if (handle && handle != INVALID_HANDLE_VALUE) {
    FILE_DISPOSITION_INFO fdi;
    fdi.DeleteFile = dokanFileInfo->DeleteOnClose;
    if (!SetFileInformationByHandle(handle, FileDispositionInfo, &fdi,
      sizeof(FILE_DISPOSITION_INFO)))
      return DokanNtStatusFromWin32(GetLastError());
  }

  return STATUS_SUCCESS;
}

NTSTATUS DokanServer::mirror_deletedirectory(LPCWSTR filename, PDOKAN_FILE_INFO dokanFileInfo) {
  HANDLE hFind;
  WIN32_FIND_DATAW findData;
  std::wstring filePath = mirrorDir + filename;

  DbgPrint(L"DeleteDirectory %s - %d\n", filePath.c_str(),
    dokanFileInfo->DeleteOnClose);

  if (!dokanFileInfo->DeleteOnClose)
    //Dokan notify that the file is requested not to be deleted.
    return STATUS_SUCCESS;

  if (filePath.back() != L'\\')
    filePath += L"\\";
  filePath += L"*";

  hFind = FindFirstFile(filePath.c_str(), &findData);

  if (hFind == INVALID_HANDLE_VALUE) {
    DWORD error = GetLastError();
    DbgPrint(L"\tDeleteDirectory error code = %d\n\n", error);
    return DokanNtStatusFromWin32(error);
  }

  do {
    if (wcscmp(findData.cFileName, L"..") != 0 &&
      wcscmp(findData.cFileName, L".") != 0) {
      FindClose(hFind);
      DbgPrint(L"\tDirectory is not empty: %s\n", findData.cFileName);
      return STATUS_DIRECTORY_NOT_EMPTY;
    }
  } while (FindNextFile(hFind, &findData) != 0);

  DWORD error = GetLastError();

  FindClose(hFind);

  if (error != ERROR_NO_MORE_FILES) {
    DbgPrint(L"\tDeleteDirectory error code = %d\n\n", error);
    return DokanNtStatusFromWin32(error);
  }

  return STATUS_SUCCESS;
}

NTSTATUS DokanServer::mirror_movefile(LPCWSTR filename, LPCWSTR newFilename, BOOL replaceIfExisting,
    PDOKAN_FILE_INFO dokanFileInfo) {
  if (IsSignpostFilename(filename))
    return STATUS_ACCESS_DENIED;

  PFILE_RENAME_INFO renameInfo = NULL;
  std::wstring filePath = mirrorDir + filename;
  std::wstring newFilePath = mirrorDir + newFilename;
  if (wcslen(newFilename) && newFilename[0] == ':') {
    // For a stream rename, FileRenameInfo expect the filename param without the filename
    // like :<stream name>:<stream type>
    newFilePath = newFilename;
  }

  DbgPrint(L"MoveFile %s -> %s\n\n", filePath.c_str(), newFilePath.c_str());
  HANDLE handle = (HANDLE)dokanFileInfo->Context;
  if (!handle || handle == INVALID_HANDLE_VALUE) {
    DbgPrint(L"\tinvalid handle\n\n");
    return STATUS_INVALID_HANDLE;
  }

  // the PFILE_RENAME_INFO struct has space for one WCHAR for the name at
  // the end, so that
  // accounts for the null terminator

  DWORD bufferSize = (DWORD)(sizeof(FILE_RENAME_INFO) +
    newFilePath.length() * sizeof(wchar_t));

  renameInfo = (PFILE_RENAME_INFO)malloc(bufferSize);
  if (!renameInfo) {
    return STATUS_BUFFER_OVERFLOW;
  }
  ZeroMemory(renameInfo, bufferSize);

  renameInfo->ReplaceIfExists = replaceIfExisting ? TRUE : FALSE; // some warning about converting BOOL to BOOLEAN
  renameInfo->RootDirectory = NULL; // hope it is never needed, shouldn't be
  renameInfo->FileNameLength = (DWORD)(newFilePath.length() * sizeof(wchar_t)); // they want length in bytes

  wcscpy_s(renameInfo->FileName, newFilePath.length() + 1, newFilePath.c_str());

  BOOL result = SetFileInformationByHandle(handle, FileRenameInfo, renameInfo, bufferSize);

  free(renameInfo);

  if (result) {
    return STATUS_SUCCESS;
  } else {
    DWORD error = GetLastError();
    DbgPrint(L"\tMoveFile error = %u\n", error);
    return DokanNtStatusFromWin32(error);
  }
}

NTSTATUS DokanServer::mirror_lockfile(LPCWSTR filename, LONGLONG byteOffset, LONGLONG length,
    PDOKAN_FILE_INFO dokanFileInfo) {
  if (IsSignpostFilename(filename))
    return STATUS_SUCCESS;

  std::wstring filePath = mirrorDir + filename;
  DbgPrint(L"LockFile %s\n", filePath.c_str());

  HANDLE handle = (HANDLE)dokanFileInfo->Context;
  if (!handle || handle == INVALID_HANDLE_VALUE) {
    DbgPrint(L"\tinvalid handle\n\n");
    return STATUS_INVALID_HANDLE;
  }

  LARGE_INTEGER liOffset;
  LARGE_INTEGER liLength;
  liLength.QuadPart = length;
  liOffset.QuadPart = byteOffset;

  if (!LockFile(handle, liOffset.LowPart, liOffset.HighPart, liLength.LowPart, liLength.HighPart)) {
    DWORD error = GetLastError();
    DbgPrint(L"\terror code = %d\n\n", error);
    return DokanNtStatusFromWin32(error);
  }

  DbgPrint(L"\tsuccess\n\n");
  return STATUS_SUCCESS;
}

NTSTATUS DokanServer::mirror_setendoffile(LPCWSTR filename, LONGLONG byteOffset,
    PDOKAN_FILE_INFO dokanFileInfo) {
  if (IsSignpostFilename(filename))
    return STATUS_ACCESS_DENIED;

  std::wstring filePath = mirrorDir + filename;
  DbgPrint(L"SetEndOfFile %s, %I64d\n", filePath.c_str(), byteOffset);

  HANDLE handle = (HANDLE)dokanFileInfo->Context;
  if (!handle || handle == INVALID_HANDLE_VALUE) {
    DbgPrint(L"\tinvalid handle\n\n");
    return STATUS_INVALID_HANDLE;
  }

  LARGE_INTEGER offset;
  offset.QuadPart = byteOffset;
  if (!SetFilePointerEx(handle, offset, NULL, FILE_BEGIN)) {
    DWORD error = GetLastError();
    DbgPrint(L"\tSetFilePointer error: %d, offset = %I64d\n\n", error, byteOffset);
    return DokanNtStatusFromWin32(error);
  }

  if (!SetEndOfFile(handle)) {
    DWORD error = GetLastError();
    DbgPrint(L"\tSetEndOfFile error code = %d\n\n", error);
    return DokanNtStatusFromWin32(error);
  }

  return STATUS_SUCCESS;
}

NTSTATUS DokanServer::mirror_setallocationsize(LPCWSTR filename, LONGLONG allocSize, PDOKAN_FILE_INFO dokanFileInfo) {
  if (IsSignpostFilename(filename))
    return STATUS_ACCESS_DENIED;

  std::wstring filePath = mirrorDir + filename;
  DbgPrint(L"SetAllocationSize %s, %I64d\n", filePath.c_str(), allocSize);

  HANDLE handle = (HANDLE)dokanFileInfo->Context;
  if (!handle || handle == INVALID_HANDLE_VALUE) {
    DbgPrint(L"\tinvalid handle\n\n");
    return STATUS_INVALID_HANDLE;
  }

  LARGE_INTEGER fileSize;
  if (GetFileSizeEx(handle, &fileSize)) {
    if (allocSize < fileSize.QuadPart) {
      fileSize.QuadPart = allocSize;
      if (!SetFilePointerEx(handle, fileSize, NULL, FILE_BEGIN)) {
        DWORD error = GetLastError();
        DbgPrint(L"\tSetAllocationSize: SetFilePointer eror: %d, offset = %I64d\n\n", error, allocSize);
        return DokanNtStatusFromWin32(error);
      }
      if (!SetEndOfFile(handle)) {
        DWORD error = GetLastError();
        DbgPrint(L"\tSetEndOfFile error code = %d\n\n", error);
        return DokanNtStatusFromWin32(error);
      }
    }
  } else {
    DWORD error = GetLastError();
    DbgPrint(L"\terror code = %d\n\n", error);
    return DokanNtStatusFromWin32(error);
  }
  return STATUS_SUCCESS;
}

NTSTATUS DokanServer::mirror_setfileattributes(LPCWSTR filename, DWORD fileAttributes, PDOKAN_FILE_INFO dokanFileInfo) {
  UNREFERENCED_PARAMETER(dokanFileInfo);
  if (IsSignpostFilename(filename))
    return STATUS_ACCESS_DENIED;

  std::wstring filePath = mirrorDir + filename;
  DbgPrint(L"SetFileAttributes %s 0x%x\n", filePath.c_str(), fileAttributes);

  if (fileAttributes != 0) {
    if (!SetFileAttributes(filePath.c_str(), fileAttributes)) {
      DWORD error = GetLastError();
      DbgPrint(L"\terror code = %d\n\n", error);
      return DokanNtStatusFromWin32(error);
    }
  } else {
    // case fileAttributes == 0 :
    // MS-FSCC 2.6 File Attributes : There is no file attribute with the value 0x00000000
    // because a value of 0x00000000 in the fileAttributes field means that the file attributes for this file MUST NOT be changed when setting basic information for the file
    DbgPrint(L"Set 0 to fileAttributes means MUST NOT be changed. Didn't call "
      L"SetFileAttributes function. \n");
  }

  DbgPrint(L"\n");
  return STATUS_SUCCESS;
}

NTSTATUS DokanServer::mirror_setfiletime(LPCWSTR filename, CONST FILETIME *creationTime,
    CONST FILETIME *lastAccessTime, CONST FILETIME *lastWriteTime, PDOKAN_FILE_INFO dokanFileInfo) {
  if (IsSignpostFilename(filename))
    return STATUS_ACCESS_DENIED;

  std::wstring filePath = mirrorDir + filename;
  DbgPrint(L"SetFileTime %s\n", filePath.c_str());

  HANDLE handle = (HANDLE)dokanFileInfo->Context;

  if (!handle || handle == INVALID_HANDLE_VALUE) {
    DbgPrint(L"\tinvalid handle\n\n");
    return STATUS_INVALID_HANDLE;
  }

  if (!SetFileTime(handle, creationTime, lastAccessTime, lastWriteTime)) {
    DWORD error = GetLastError();
    DbgPrint(L"\terror code = %d\n\n", error);
    return DokanNtStatusFromWin32(error);
  }

  DbgPrint(L"\n");
  return STATUS_SUCCESS;
}

NTSTATUS DokanServer::mirror_unlockfile(LPCWSTR filename, LONGLONG byteOffset, LONGLONG length,
    PDOKAN_FILE_INFO dokanFileInfo) {
  if (IsSignpostFilename(filename))
    return STATUS_ACCESS_DENIED;

  std::wstring filePath = mirrorDir + filename;
  DbgPrint(L"UnlockFile %s\n", filePath.c_str());

  HANDLE handle = (HANDLE)dokanFileInfo->Context;
  if (!handle || handle == INVALID_HANDLE_VALUE) {
    DbgPrint(L"\tinvalid handle\n\n");
    return STATUS_INVALID_HANDLE;
  }

  LARGE_INTEGER liLength;
  LARGE_INTEGER liOffset;
  liLength.QuadPart = length;
  liOffset.QuadPart = byteOffset;

  if (!UnlockFile(handle, liOffset.LowPart, liOffset.HighPart, liLength.LowPart, liLength.HighPart)) {
    DWORD error = GetLastError();
    DbgPrint(L"\terror code = %d\n\n", error);
    return DokanNtStatusFromWin32(error);
  }

  DbgPrint(L"\tsuccess\n\n");
  return STATUS_SUCCESS;
}

