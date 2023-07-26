#include "AviFrameManagerImpl.h"
#include "DokanServer.h"
#include "FsProxy.h"
#include <shlobj.h>
#include <dokan/dokan.h>

#define AUDIO_CACHE_FILE_SUFFIX L".audiocache.avi"

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
  int ret = -1;
  do {
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    if (argc == 2 && _wcsicmp(argv[1], L"/getdv") == 0)
      ExitProcess(DokanDriverVersion());

    AviFrameManagerImpl afmi;
    if (argc < 2 || !afmi.Init(argv[1]))
      break;

    // Create the directory where DMFS files are stored
    DfscData* vars = afmi.getDfscData();
    std::wstring filesDir = vars->signpostPath;
    if (filesDir.length() < 2 || filesDir[1] != ':') {  // no drive letter in path?
      // use user's default appdata drive
      wchar_t appDataDir[MAX_PATH];
      SHGetSpecialFolderPath(vars->servingDlg, appDataDir, CSIDL_APPDATA, FALSE);
      filesDir = appDataDir;
    }
    filesDir = filesDir.erase(filesDir.find(':') + 2) + L"DMFS\\";
    CreateDirectory(filesDir.c_str(), NULL);

    // Create the audio cache filepath, create the file, and send the file path to the Frameserver
    std::wstring nameOnly = vars->signpostPath;
    nameOnly = nameOnly.substr(nameOnly.rfind('\\') + 1);

    std::wstring audioCacheFilename = filesDir;
    audioCacheFilename += nameOnly + AUDIO_CACHE_FILE_SUFFIX;
    AviFileWrapperImpl signpostAvi(&afmi);
    if (!afmi.CreateSignpostAndAudioCacheAVIs(&signpostAvi, audioCacheFilename.c_str()))
      break;
    DWORD_PTR copydatares = 0;
    COPYDATASTRUCT cds1 = {
      WM_DOKANSERVE_AUDIOCACHE_PATH, (DWORD)((audioCacheFilename.length() + 1) * sizeof(wchar_t)), (void*) audioCacheFilename.c_str()
    };
    SendMessageTimeout(vars->servingDlg, WM_COPYDATA, (WPARAM)vars->servingDlg, (LPARAM)&cds1,
      SMTO_ABORTIFHUNG, 5000, &copydatares);

    // Create the mount point directory, ideally an empty or non-existent directory
    std::wstring mountDir;
    for (int i = 0; i < 10; ++i) {
      wchar_t suffix[10] = { 0 };
      if (i > 0) {
        wcscpy_s(suffix, _countof(suffix), L"-");
        _itow_s(i, suffix + 1, _countof(suffix) - 1, 10);
      }
      std::wstring mountPointTest = std::wstring(filesDir) + L"virtual" + suffix;
      std::wstring findPath = mountPointTest + L"\\*";
      WIN32_FIND_DATA fd = { 0 };
      HANDLE file = FindFirstFile(findPath.c_str(), &fd);
      BOOL foundAFile = false;
      do {
        foundAFile = (file != INVALID_HANDLE_VALUE &&
                      wcscmp(fd.cFileName, L".") != 0 &&
                      wcscmp(fd.cFileName, L"..") != 0);
      } while (!foundAFile && FindNextFile(file, &fd));
      if (!foundAFile) {
        mountDir = mountPointTest;
        break;
      }
      FindClose(file);
    }
    if (mountDir.empty())
      break;
    CreateDirectory(mountDir.c_str(), NULL);

    // Also mirror the 'virtualmirror' directory under filesDir so that we allow writing files into the mountDir
    std::wstring mirrorDir = filesDir + L"virtualmirror";
    CreateDirectory(mirrorDir.c_str(), NULL);  // if it exists already, no problem.

    // Send the signpost path under the mountDir to Frameserver
    std::wstring signpostPath = mountDir + L"\\" + nameOnly;
    COPYDATASTRUCT cds2 = { WM_DOKANSERVE_SIGNPOST_PATH, (DWORD)((signpostPath.length() + 1) * sizeof(wchar_t)), (void*)signpostPath.c_str() };
    SendMessageTimeout(vars->servingDlg, WM_COPYDATA, (WPARAM)vars->servingDlg, (LPARAM)&cds2,
      SMTO_ABORTIFHUNG, 5000, &copydatares);

    ServeSignpostAndWait(mountDir, mirrorDir, &signpostAvi, nameOnly);
    ret = 0;
  } while (0);

  return ret;
}
