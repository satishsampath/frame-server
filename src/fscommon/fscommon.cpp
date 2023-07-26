/**
 * Debugmode Frameserver
 * Copyright (C) 2002-2019 Satish Kumar, All Rights Reserved
 * http://www.debugmode.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 * http://www.gnu.org/copyleft/gpl.html
 */

#define WIN32_NO_STATUS
#include <windows.h>
#include <mmsystem.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdint.h>
#include <shlobj.h>
#include "fscommon_resource.h"
#include "fscommon.h"
#include "blankavi.h"
#include "utils.h"
#include "ImageSequence.h"
#include <dokan/dokan.h>
#include "../dokanserve/FsProxy.h"

#define APPNAME _T("Debugmode FrameServer")
#define REGISTRY_FOLDER _T("Software\\Debugmode\\FrameServer")
#ifndef BIF_NEWDIALOGSTYLE
    #define BIF_NEWDIALOGSTYLE 0x0040
#endif

BOOL saveAsImageSequence = FALSE;
TCHAR imageSequencePath[MAX_PATH * 2] = _T("");
UINT imageSequenceFormat = ImageSequenceFormatPNG;
wchar_t commandToRunOnFsStart[MAX_PATH] = L"";
BOOL runCommandOnFsStart = FALSE, endAfterRunningCommand = FALSE;

BOOL pcmAudioInAvi = FALSE, networkServing = FALSE, serveVirtualUncompressedAvi = TRUE;
UINT networkPort = 8278, serveFormat = sfRGB32;
BOOL stopServing = FALSE, pauseServing = FALSE;
HWND appwnd;
TCHAR installDir[MAX_PATH] = _T("C:\\Program Files\\Debugmode\\Frameserver");
extern HINSTANCE ghInst;
HINSTANCE ghResInst;
unsigned char* clip, * clipr, * clipb;
HMODULE imageSequenceModule = NULL;

extern void ShowUpdateNotificationIfRequired(HWND dlg);
extern void StartUpdateCheck();
extern void CancelUpdateCheck();

bool CheckForDokan(HWND parentWnd, const wchar_t* msgBoxTitle) {
  if (DokanDriverVersion() > 0)
    return true;

  MessageBoxW(parentWnd,
    L"The 'serve virtual uncompressed AVI' feature requires a tool called 'Dokan Library'. "
    L"This was included in the FrameServer install.\n"
    L"Please reinstall FrameServer and enable 'Dokan Library' during install.\n\n"
    L"If this problem remains after reinstall, please check if you have a lower version of "
    L"Dokan Library installed in your system already. If yes, uninstall that and try again.\n\n",
    msgBoxTitle, MB_ICONEXCLAMATION | MB_OK);

  return false;
}

BOOL CreateProcessInJob(HANDLE hJob, LPCTSTR lpApplicationName, LPTSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes,
                        LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags,
                        LPVOID lpEnvironment, LPCTSTR lpCurrentDirectory, LPSTARTUPINFO lpStartupInfo, LPPROCESS_INFORMATION ppi) {
  BOOL fRc = CreateProcess(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes,
                           bInheritHandles, dwCreationFlags | CREATE_SUSPENDED, lpEnvironment,
                           lpCurrentDirectory, lpStartupInfo, ppi);
  if (fRc) {
    fRc = AssignProcessToJobObject(hJob, ppi->hProcess);
    if (fRc && !(dwCreationFlags & CREATE_SUSPENDED)) {
      fRc = ResumeThread(ppi->hThread) != (DWORD)-1;
    }
    if (!fRc) {
      TerminateProcess(ppi->hProcess, 0);
      CloseHandle(ppi->hProcess);
      CloseHandle(ppi->hThread);
      ppi->hProcess = ppi->hThread = nullptr;
    }
  }
  return fRc;
}

void SetFsIconForWindow(HWND wnd) {
  SendMessage(wnd, WM_SETICON, ICON_BIG, (LPARAM)LoadImage(ghResInst, MAKEINTRESOURCE(IDI_FRAMESERVER),
          IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR));
  SendMessage(wnd, WM_SETICON, ICON_SMALL, (LPARAM)LoadImage(ghResInst, MAKEINTRESOURCE(IDI_FRAMESERVER),
          IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR));
}

BOOL HandleBannerBackgroundForHighDPI(UINT msg, WPARAM wp, LPARAM lp) {
  if (msg == WM_DRAWITEM) {
    DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lp;
    if (dis->CtlID == IDC_BANNER_BACKGROUND) {
      ::FillRect(dis->hDC, &dis->rcItem, (HBRUSH)::GetStockObject(WHITE_BRUSH));
      return TRUE;
    }
  }
  return FALSE;
}

bool LoadImageSequenceModule() {
  if (imageSequenceModule != NULL)
    return true;

  TCHAR libpath[MAX_PATH];
  _tcscpy_s(libpath, MAX_PATH, installDir);
  _tcscat_s(libpath, MAX_PATH, _T("\\ImageSequence.dll"));
  imageSequenceModule = LoadLibrary(libpath);
  if (!imageSequenceModule) {
    MessageBox(NULL, _T("Unable to load \"ImageSequence.dll\".\n\n")
        _T("Please check if you have GDI+ available in your machine. For more information visit\n")
        _T("http://www.microsoft.com/downloads/details.aspx?FamilyID=6a63ab9c-df12-4d41-933c-be590feaa05a."),
        APPNAME, MB_OK | MB_ICONEXCLAMATION);
    return false;
  }
  return true;
}

DWORD WINAPI ImageSequenceExportThreadProc(LPVOID param) {
  if (!LoadImageSequenceModule())
    return 0;

  fxnSaveImageSequence Save = (fxnSaveImageSequence)GetProcAddress(imageSequenceModule, "SaveImageSequence");
  Save((DfscData*)param, imageSequencePath, imageSequenceFormat);

  return 0;
}

DLGPROCRET CALLBACK AboutDlgProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
  switch (msg) {
  case WM_INITDIALOG: {
      HRSRC rc = ::FindResource(ghResInst, MAKEINTRESOURCE(IDS_ABOUTDLG_COPYRIGHT), MAKEINTRESOURCE(TEXTFILE));
      DWORD size = ::SizeofResource(ghResInst, rc);
      HGLOBAL rcData = ::LoadResource(ghResInst, rc);
      char* str = new char[size + 1];
      memcpy(str, static_cast<const char*>(::LockResource(rcData)), size);
      str[size] = 0;
      ::SetDlgItemTextA(dlg, IDC_WARNING, str);
      delete[] str;
      SetFsIconForWindow(dlg);
      return TRUE;
    }
  case WM_COMMAND:
    if (LOWORD(wp) == IDOK || LOWORD(wp) == IDCANCEL)
      EndDialog(dlg, 0);
    break;
  }
  if (HandleBannerBackgroundForHighDPI(msg, wp, lp))
    return TRUE;
  return FALSE;
}

void UpdateImageSequenceControls(HWND dlg) {
  int videoShow = saveAsImageSequence ? SW_HIDE : SW_SHOW;
  int imageSequenceShow = saveAsImageSequence ? SW_SHOW : SW_HIDE;
  ShowWindow(GetDlgItem(dlg, IDC_STATIC_FORMAT), videoShow);
  ShowWindow(GetDlgItem(dlg, IDC_SERVEASRGB24), videoShow);
  ShowWindow(GetDlgItem(dlg, IDC_SERVEASRGB32), videoShow);
  ShowWindow(GetDlgItem(dlg, IDC_SERVEASYUY2), videoShow);
  ShowWindow(GetDlgItem(dlg, IDC_SERVEASV210), videoShow);
  ShowWindow(GetDlgItem(dlg, IDC_SERVEVIRTUALUNCOMPRESSEDAVI), videoShow);
  ShowWindow(GetDlgItem(dlg, IDC_PCMAUDIOINAVI), videoShow);
  EnableWindow(GetDlgItem(dlg, IDC_PCMAUDIOINAVI), !saveAsImageSequence && !serveVirtualUncompressedAvi);
  ShowWindow(GetDlgItem(dlg, IDC_RUNCOMMANDONFSSTART), videoShow);
  ShowWindow(GetDlgItem(dlg, IDC_COMMANDTORUN), videoShow);
  ShowWindow(GetDlgItem(dlg, IDC_ENDAFTERRUNNINGCOMMAND), videoShow);
  //ShowWindow(GetDlgItem(dlg, IDC_NETSERVE), !saveAsImageSequence);
  //ShowWindow(GetDlgItem(dlg, IDC_NETPORT), !saveAsImageSequence);
  ShowWindow(GetDlgItem(dlg, IDC_STATIC_IMAGESEQUENCEPATH), imageSequenceShow);
  ShowWindow(GetDlgItem(dlg, IDC_IMAGESEQUENCEPATH), imageSequenceShow);
  ShowWindow(GetDlgItem(dlg, IDC_IMAGESEQUENCEBROWSE), imageSequenceShow);
  ShowWindow(GetDlgItem(dlg, IDC_STATIC_IMAGESEQUENCEFORMAT), imageSequenceShow);
  ShowWindow(GetDlgItem(dlg, IDC_IMAGESEQUENCEFORMAT), imageSequenceShow);
}

void UpdateRunCommandOnFsStartControls(HWND dlg) {
  EnableWindow(GetDlgItem(dlg, IDC_COMMANDTORUN), IsDlgButtonChecked(dlg, IDC_RUNCOMMANDONFSSTART));
  EnableWindow(GetDlgItem(dlg, IDC_ENDAFTERRUNNINGCOMMAND), IsDlgButtonChecked(dlg, IDC_RUNCOMMANDONFSSTART));
}

int CALLBACK DirBrowseDlgCallback(HWND wnd, UINT msg, LPARAM lp, LPARAM data) {
  // set the initial dir in the dialog to what we have been given.
  if (msg == BFFM_INITIALIZED)
    SendMessage(wnd, BFFM_SETSELECTION, TRUE, data);

  return (msg == BFFM_VALIDATEFAILED) ? 1 : 0;
}

DLGPROCRET CALLBACK OptionsDlgProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
  switch (msg) {
  case WM_INITDIALOG: {
    SetFsIconForWindow(dlg);
    HKEY key = 0;
    RegCreateKeyEx(HKEY_CURRENT_USER, REGISTRY_FOLDER, 0, 0,
        REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0, &key, 0);
    if (key) {
      DWORD size = sizeof(pcmAudioInAvi);
      RegQueryValueEx(key, _T("pcmAudioInAvi"), 0, 0, (LPBYTE)&pcmAudioInAvi, &size);
      size = sizeof(networkServing);
      RegQueryValueEx(key, _T("networkServing"), 0, 0, (LPBYTE)&networkServing, &size);
      size = sizeof(serveVirtualUncompressedAvi);
      RegQueryValueEx(key, _T("serveVirtualUncompressedAvi"), 0, 0, (LPBYTE)&serveVirtualUncompressedAvi, &size);
      size = sizeof(serveFormat);
      RegQueryValueEx(key, _T("serveFormat"), 0, 0, (LPBYTE)&serveFormat, &size);
      size = sizeof(networkPort);
      RegQueryValueEx(key, _T("networkPort"), 0, 0, (LPBYTE)&networkPort, &size);
      size = sizeof(runCommandOnFsStart);
      RegQueryValueEx(key, _T("runCommandOnFsStart"), 0, 0, (LPBYTE)&runCommandOnFsStart, &size);
      size = sizeof(commandToRunOnFsStart);
      RegQueryValueEx(key, _T("commandToRunOnFsStart"), 0, 0, (LPBYTE)commandToRunOnFsStart, &size);
      size = sizeof(endAfterRunningCommand);
      RegQueryValueEx(key, _T("endAfterRunningCommand"), 0, 0, (LPBYTE)&endAfterRunningCommand, &size);
      size = sizeof(saveAsImageSequence);
      RegQueryValueEx(key, _T("saveAsImageSequence"), 0, 0, (LPBYTE)&saveAsImageSequence, &size);
      size = sizeof(imageSequenceFormat);
      RegQueryValueEx(key, _T("imageSequenceFormat"), 0, 0, (LPBYTE)&imageSequenceFormat, &size);
      size = sizeof(imageSequencePath);
      RegQueryValueEx(key, _T("imageSequencePath"), 0, 0, (LPBYTE)imageSequencePath, &size);
      RegCloseKey(key);
    }
    if (_tcslen(imageSequencePath) == 0)
      _tcscpy_s(imageSequencePath, _countof(imageSequencePath), _T("C:\\FrameServer Images\\Image######.png"));

    CheckDlgButton(dlg, IDC_PCMAUDIOINAVI, pcmAudioInAvi);
    CheckRadioButton(dlg, IDC_SERVEASRGB24, IDC_SERVEASV210, IDC_SERVEASRGB24 + serveFormat);
    CheckDlgButton(dlg, IDC_NETSERVE, networkServing);
    SetDlgItemInt(dlg, IDC_NETPORT, networkPort, FALSE);
    EnableWindow(GetDlgItem(dlg, IDC_NETPORT), networkServing);
    CheckDlgButton(dlg, IDC_SERVEVIRTUALUNCOMPRESSEDAVI, serveVirtualUncompressedAvi);
    CheckDlgButton(dlg, IDC_RUNCOMMANDONFSSTART, runCommandOnFsStart);
    CheckDlgButton(dlg, IDC_ENDAFTERRUNNINGCOMMAND, endAfterRunningCommand);
    SetDlgItemText(dlg, IDC_COMMANDTORUN, commandToRunOnFsStart);

    SendMessage(GetDlgItem(dlg, IDC_IMAGESEQUENCEFORMAT), CB_ADDSTRING, 0, (LPARAM)_T("JPEG"));
    SendMessage(GetDlgItem(dlg, IDC_IMAGESEQUENCEFORMAT), CB_ADDSTRING, 0, (LPARAM)_T("PNG"));
    SendMessage(GetDlgItem(dlg, IDC_IMAGESEQUENCEFORMAT), CB_ADDSTRING, 0, (LPARAM)_T("TIFF"));
    SendMessage(GetDlgItem(dlg, IDC_IMAGESEQUENCEFORMAT), CB_ADDSTRING, 0, (LPARAM)_T("BMP"));
    SendMessage(GetDlgItem(dlg, IDC_IMAGESEQUENCEFORMAT), CB_SETCURSEL, imageSequenceFormat, 0);

    SetDlgItemText(dlg, IDC_IMAGESEQUENCEPATH, imageSequencePath);
    CheckRadioButton(dlg, IDC_OUTPUTVIDEO, IDC_OUTPUTIMAGESEQUENCE,
        (saveAsImageSequence ? IDC_OUTPUTIMAGESEQUENCE : IDC_OUTPUTVIDEO));

    SendMessage(GetDlgItem(dlg, IDC_COMMANDTORUN), EM_SETCUEBANNER, FALSE,
      (LPARAM) L"e.g. c:\\ffmpeg\\ffmpeg.exe -i \"%~1\" -c:v libx264 \"%~1_new.mp4\"");
    UpdateImageSequenceControls(dlg);
    UpdateRunCommandOnFsStartControls(dlg);
  }
  break;
  case WM_COMMAND:
    if (LOWORD(wp) == IDC_NETSERVE)
      EnableWindow(GetDlgItem(dlg, IDC_NETPORT), IsDlgButtonChecked(dlg, IDC_NETSERVE));

    if (LOWORD(wp) == IDC_SERVEVIRTUALUNCOMPRESSEDAVI) {
      if (CheckForDokan(dlg, APPNAME)) {
        EnableWindow(GetDlgItem(dlg, IDC_PCMAUDIOINAVI), !IsDlgButtonChecked(dlg, IDC_SERVEVIRTUALUNCOMPRESSEDAVI));
      }
    }

    if (LOWORD(wp) == IDC_IMAGESEQUENCEBROWSE) {
      TCHAR filepath[MAX_PATH], filename[MAX_PATH];
      GetDlgItemText(dlg, IDC_IMAGESEQUENCEPATH, filepath, _countof(filepath));
      filename[0] = 0;
      if (_tcsrchr(filepath, '\\') != NULL) {
        _tcscpy_s(filename, _countof(filename), _tcsrchr(filepath, '\\') + 1);
        *_tcsrchr(filepath, '\\') = 0;
      }

      // remove non-existant dirs from the end until we reach a dir which is valid.
      while (1) {
        WIN32_FILE_ATTRIBUTE_DATA attrs;
        if (GetFileAttributesEx(filepath, GetFileExInfoStandard, &attrs) &&
            (attrs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
          break;
        }
        if (!_tcsrchr(filepath, '\\'))
          break;            // only a single \ remaining so stop with this.
        _tcsrchr(filepath, '\\')[0] = 0;          // remove the last dir from the string
      }
      if (_tcschr(filepath, '\\') == NULL)
        _tcscat_s(filepath, _countof(filepath), _T("\\"));

      CoInitialize(0);

      BROWSEINFO info = { 0 };
      info.hwndOwner = dlg;
      info.pszDisplayName = filepath;
      info.lpszTitle = _T("Choose the destination folder where you want the images to be saved.");
      info.ulFlags = BIF_VALIDATE | BIF_NEWDIALOGSTYLE;
      info.lParam = (LPARAM)filepath;
      info.lpfn = DirBrowseDlgCallback;
      LPITEMIDLIST item = SHBrowseForFolder(&info);
      if (item) {
        SHGetPathFromIDList(item, filepath);
        if (_tcslen(filename)) {
          _tcscat_s(filepath, _countof(filepath), _T("\\"));
          _tcscat_s(filepath, _countof(filepath), filename);
        }
        SetDlgItemText(dlg, IDC_IMAGESEQUENCEPATH, filepath);
      }

      CoUninitialize();
    }

    if (LOWORD(wp) == IDC_IMAGESEQUENCEFORMAT && HIWORD(wp) == CBN_SELCHANGE) {
      TCHAR filepath[MAX_PATH];
      GetDlgItemText(dlg, IDC_IMAGESEQUENCEPATH, filepath, _countof(filepath));
      if (_tcsrchr(filepath, '.') != NULL)
        _tcsrchr(filepath, '.')[0] = 0;

      LRESULT sel = SendMessage(GetDlgItem(dlg, IDC_IMAGESEQUENCEFORMAT), CB_GETCURSEL, 0, 0);
      if (sel == 0) _tcscat_s(filepath, _countof(filepath), _T(".jpg"));
      if (sel == 1) _tcscat_s(filepath, _countof(filepath), _T(".png"));
      if (sel == 2) _tcscat_s(filepath, _countof(filepath), _T(".tif"));
      if (sel == 3) _tcscat_s(filepath, _countof(filepath), _T(".bmp"));
      SetDlgItemText(dlg, IDC_IMAGESEQUENCEPATH, filepath);
    }

    if (LOWORD(wp) == IDC_OUTPUTVIDEO) {
      saveAsImageSequence = FALSE;
      UpdateImageSequenceControls(dlg);
    }
    if (LOWORD(wp) == IDC_OUTPUTIMAGESEQUENCE) {
      saveAsImageSequence = TRUE;
      UpdateImageSequenceControls(dlg);
    }
    if (LOWORD(wp) == IDC_RUNCOMMANDONFSSTART)
      UpdateRunCommandOnFsStartControls(dlg);

    if (LOWORD(wp) == IDCANCEL)
      EndDialog(dlg, IDCANCEL);

    if (LOWORD(wp) == IDOK) {
      if (saveAsImageSequence) {
        // Check if the path and filenames are good enough
        TCHAR dstpath[MAX_PATH * 2];
        GetDlgItemText(dlg, IDC_IMAGESEQUENCEPATH, dstpath, _countof(dstpath));
        if (!LoadImageSequenceModule())
          break;
        fxnCheckAndPreparePath CheckAndPreparePath = (fxnCheckAndPreparePath)
                                                     GetProcAddress(imageSequenceModule, "CheckAndPreparePath");

        if (!CheckAndPreparePath(dstpath))
          break;
      }

      pcmAudioInAvi = IsDlgButtonChecked(dlg, IDC_PCMAUDIOINAVI);
      serveFormat = sfRGB24;
      if (IsDlgButtonChecked(dlg, IDC_SERVEASRGB32)) serveFormat = sfRGB32;
      if (IsDlgButtonChecked(dlg, IDC_SERVEASYUY2)) serveFormat = sfYUY2;
      if (IsDlgButtonChecked(dlg, IDC_SERVEASV210)) serveFormat = sfV210;
      networkServing = IsDlgButtonChecked(dlg, IDC_NETSERVE);
      networkPort = GetDlgItemInt(dlg, IDC_NETPORT, NULL, FALSE);
      serveVirtualUncompressedAvi = IsDlgButtonChecked(dlg, IDC_SERVEVIRTUALUNCOMPRESSEDAVI);
      runCommandOnFsStart = IsDlgButtonChecked(dlg, IDC_RUNCOMMANDONFSSTART);
      endAfterRunningCommand = IsDlgButtonChecked(dlg, IDC_ENDAFTERRUNNINGCOMMAND);
      GetDlgItemText(dlg, IDC_COMMANDTORUN, commandToRunOnFsStart, _countof(commandToRunOnFsStart));
      saveAsImageSequence = IsDlgButtonChecked(dlg, IDC_OUTPUTIMAGESEQUENCE);
      GetDlgItemText(dlg, IDC_IMAGESEQUENCEPATH, imageSequencePath, _countof(imageSequencePath));
      imageSequenceFormat = (UINT)SendMessage(GetDlgItem(dlg, IDC_IMAGESEQUENCEFORMAT), CB_GETCURSEL, 0, 0);

      HKEY key = 0;
      RegCreateKeyEx(HKEY_CURRENT_USER, REGISTRY_FOLDER, 0, 0, REG_OPTION_NON_VOLATILE,
          KEY_ALL_ACCESS, 0, &key, 0);
      if (key) {
        RegSetValueEx(key, _T("pcmAudioInAvi"), 0, REG_DWORD, (LPBYTE)&pcmAudioInAvi, sizeof(pcmAudioInAvi));
        RegSetValueEx(key, _T("networkServing"), 0, REG_DWORD, (LPBYTE)&networkServing, sizeof(networkServing));
        RegSetValueEx(key, _T("serveVirtualUncompressedAvi"), 0, REG_DWORD, (LPBYTE)&serveVirtualUncompressedAvi,
          sizeof(serveVirtualUncompressedAvi));
        RegSetValueEx(key, _T("runCommandOnFsStart"), 0, REG_DWORD, (LPBYTE)&runCommandOnFsStart, sizeof(runCommandOnFsStart));
        RegSetValueEx(key, _T("commandToRunOnFsStart"), 0, REG_SZ, (LPBYTE)commandToRunOnFsStart,
          (DWORD)(wcslen(commandToRunOnFsStart) + 1) * sizeof(wchar_t));
        RegSetValueEx(key, _T("endAfterRunningCommand"), 0, REG_DWORD, (LPBYTE)&endAfterRunningCommand, sizeof(endAfterRunningCommand));
        RegSetValueEx(key, _T("serveFormat"), 0, REG_DWORD, (LPBYTE)&serveFormat, sizeof(serveFormat));
        RegSetValueEx(key, _T("networkPort"), 0, REG_DWORD, (LPBYTE)&networkPort, sizeof(networkPort));
        RegSetValueEx(key, _T("saveAsImageSequence"), 0, REG_DWORD, (LPBYTE)&saveAsImageSequence, sizeof(saveAsImageSequence));
        RegSetValueEx(key, _T("imageSequenceFormat"), 0, REG_DWORD, (LPBYTE)&imageSequenceFormat, sizeof(imageSequenceFormat));
        RegSetValueEx(key, _T("imageSequencePath"), 0, REG_SZ, (LPBYTE)imageSequencePath, (DWORD)(_tcslen(imageSequencePath) + 1) * sizeof(TCHAR));
        RegCloseKey(key);
      }
      EndDialog(dlg, IDOK);
    }
    break;
  }
  if (HandleBannerBackgroundForHighDPI(msg, wp, lp))
    return TRUE;
  return FALSE;
}

DLGPROCRET CALLBACK WritingSignpostDlgProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
  switch (msg) {
  case WM_INITDIALOG:
    SetFsIconForWindow(dlg);
    return TRUE;
  case WM_COMMAND:
    if (LOWORD(wp) == IDCANCEL) {
      stopServing = TRUE;
      EndDialog(dlg, IDCANCEL);
    }
  }
  if (HandleBannerBackgroundForHighDPI(msg, wp, lp))
    return TRUE;
  return FALSE;
}

DLGPROCRET CALLBACK ServingDlgProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
  switch (msg) {
  case WM_INITDIALOG:
    SetFsIconForWindow(dlg);
    CheckDlgButton(dlg, IDC_PCMAUDIOINAVI, pcmAudioInAvi);
    CheckRadioButton(dlg, IDC_SERVEASRGB24, IDC_SERVEASV210, IDC_SERVEASRGB24 + serveFormat);
    CheckDlgButton(dlg, IDC_NETSERVE, networkServing);
    CheckDlgButton(dlg, IDC_SERVEVIRTUALUNCOMPRESSEDAVI, serveVirtualUncompressedAvi);
    SetDlgItemInt(dlg, IDC_NETPORT, networkPort, FALSE);
    SetDlgItemText(dlg, IDC_COMMANDTORUN, commandToRunOnFsStart);
    CheckDlgButton(dlg, IDC_ENDAFTERRUNNINGCOMMAND, endAfterRunningCommand);
    if (!runCommandOnFsStart) {
      ShowWindow(GetDlgItem(dlg, IDC_STATIC_COMMANDTORUN), SW_HIDE);
      ShowWindow(GetDlgItem(dlg, IDC_COMMANDTORUN), SW_HIDE);
      ShowWindow(GetDlgItem(dlg, IDC_ENDAFTERRUNNINGCOMMAND), SW_HIDE);
    }
    break;
  case WM_COMMAND:
    if (LOWORD(wp) == IDCANCEL) {
      stopServing = TRUE;
    } else if (LOWORD(wp) == IDC_PAUSE) {
      pauseServing = TRUE;
      ShowWindow(GetDlgItem(dlg, IDC_PAUSE), SW_HIDE);
      ShowWindow(GetDlgItem(dlg, IDC_RESUME), SW_SHOW);
    } else if (LOWORD(wp) == IDC_RESUME) {
      pauseServing = FALSE;
      ShowWindow(GetDlgItem(dlg, IDC_RESUME), SW_HIDE);
      ShowWindow(GetDlgItem(dlg, IDC_PAUSE), SW_SHOW);
    } else if (LOWORD(wp) == IDC_ABOUT) {
      DialogBox(ghResInst, MAKEINTRESOURCE(IDD_DFABOUTDLG), dlg, AboutDlgProc);
    } else if (LOWORD(wp) == IDC_COPYPATH) {
      wchar_t value[MAX_PATH];
      GetDlgItemText(dlg, IDC_EDIT1, value, _countof(value));
      HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (wcslen(value) + 1) * sizeof(wchar_t));
      wcscpy_s((wchar_t*)GlobalLock(hMem), wcslen(value) + 1, value);
      GlobalUnlock(hMem);
      OpenClipboard(0);
      EmptyClipboard();
      SetClipboardData(CF_UNICODETEXT, hMem);
      CloseClipboard();
    }
    break;
  case WM_SIZE:
    if (wp == SIZE_MINIMIZED) {
      EnableWindow(appwnd, TRUE);
      ShowWindow(appwnd, SW_MINIMIZE);
    }
    if (wp == SIZE_RESTORED) {
      EnableWindow(appwnd, FALSE);
    }
    break;
  case WM_CTLCOLORSTATIC: {
      HDC hdc = (HDC)wp;
      HWND wnd = (HWND)lp;
      int id = GetDlgCtrlID(wnd);
      if (id == IDC_UPDATE_URL || id == IDC_UPDATE_LABEL) {
        SetTextColor(hdc, RGB(255, 255, 255));
        SetBkColor(hdc, RGB(255, 0, 0));
#pragma warning(disable:4311 4302)
        return (BOOL)CreateSolidBrush(RGB(255, 0, 0));
      }
      break;
    }
  case WM_COPYDATA: {
      FrameServerImpl* fs = (FrameServerImpl*)GetWindowLongPtr(dlg, GWLP_USERDATA);
      COPYDATASTRUCT* cds = (COPYDATASTRUCT*) lp;
      if (cds->dwData == WM_DOKANSERVE_SIGNPOST_PATH) {
        wcscpy_s(fs->dokanSignpostFilePath, _countof(fs->dokanSignpostFilePath), (const wchar_t*)cds->lpData);
        PostQuitMessage(0);
      } else if (cds->dwData == WM_DOKANSERVE_AUDIOCACHE_PATH) {
        wcscpy_s(fs->dokanAudioCacheFilePath, _countof(fs->dokanAudioCacheFilePath), (const wchar_t*)cds->lpData);
      }
    }
  }
  if (HandleBannerBackgroundForHighDPI(msg, wp, lp))
    return TRUE;
  return FALSE;
}

void InitClip() {
  clip = (unsigned char*)malloc(896 * sizeof(unsigned char));
  clipr = (unsigned char*)malloc(32768 * sizeof(unsigned char));
  clipb = (unsigned char*)malloc(32768 * sizeof(unsigned char));
  memset(clip, 0, 320);
  int i;
  for (i = 0; i < 256; ++i) clip[i + 320] = i;
  memset(clip + 320 + 256, 255, 320);
  for (i = 0; i < 32768; i++) {
    clipb[i] = clip[((i - 16383) * 507 + 0x1C08000) >> 16];
    clipr[i] = clip[((i - 16383) * 642 + 0x1C08000) >> 16];
  }
}

void ConvertVideoFrame(LPCVOID pFrame, int rowBytes, DfscData* vars, int inDataFormat /*=idfRGB32*/) {
  vars->videoBytesRead = 0;
  if (serveFormat == sfYUY2) {
    if (inDataFormat == idfAYUV) {
      // AYUV -> YUY2
#if defined(_M_X64)
      BYTE* src = (LPBYTE)pFrame;
      BYTE* dst = ((LPBYTE)vars) + vars->videooffset;
      mmx_VUYx_to_YUY2(src, dst, vars->encBi.biHeight, vars->encBi.biWidth, rowBytes);
#else
      for (int i = 0; i < vars->encBi.biHeight; i++) {
        BYTE* src;
        src = (LPBYTE)pFrame + rowBytes * (vars->encBi.biHeight - 1 - i);
        BYTE* dst = ((LPBYTE)vars) + vars->videooffset + i * vars->encBi.biWidth * 2;
        BYTE* srcend = src + vars->encBi.biWidth * 4;
        for (int j = 0; j < (vars->encBi.biWidth & 7); j += 2) {
          dst[0] = src[2];
          dst[1] = src[1];
          dst[2] = src[6];
          dst[3] = src[0];
          dst += 4; src += 8;
        }
        while (src < srcend) {
          *(DWORD*)&dst[4 * 0 + 0] = (src[8 * 0 + 2]) + (src[8 * 0 + 1] << 8) + (src[8 * 0 + 6] << 16) + (src[8 * 0 + 0] << 24);
          *(DWORD*)&dst[4 * 1 + 0] = (src[8 * 1 + 2]) + (src[8 * 1 + 1] << 8) + (src[8 * 1 + 6] << 16) + (src[8 * 1 + 0] << 24);
          *(DWORD*)&dst[4 * 2 + 0] = (src[8 * 2 + 2]) + (src[8 * 2 + 1] << 8) + (src[8 * 2 + 6] << 16) + (src[8 * 2 + 0] << 24);
          *(DWORD*)&dst[4 * 3 + 0] = (src[8 * 3 + 2]) + (src[8 * 3 + 1] << 8) + (src[8 * 3 + 6] << 16) + (src[8 * 3 + 0] << 24);
          dst += 16;
          src += 32;
        }
      }
#endif
      vars->videoBytesRead = vars->encBi.biWidth * vars->encBi.biHeight * 2;
    } else if (inDataFormat == idfRGB32) {
      if (MMX_enabled) {
        mmx_ConvertRGB32toYUY2((LPBYTE)pFrame, ((LPBYTE)vars) + vars->videooffset,
            rowBytes, vars->encBi.biWidth * 2, vars->encBi.biWidth, vars->encBi.biHeight);
      } else {
        #define cyb int(0.114 * 219 / 255 * 65536 + 0.5)
        #define cyg int(0.587 * 219 / 255 * 65536 + 0.5)
        #define cyr int(0.299 * 219 / 255 * 65536 + 0.5)

        for (int i = 0; i < vars->encBi.biHeight; i++) {
          BYTE* rgb = (LPBYTE)pFrame + rowBytes * (vars->encBi.biHeight - 1 - i);
          BYTE* yuv = ((LPBYTE)vars) + vars->videooffset + i * vars->encBi.biWidth * 2;
          BYTE* rgbend = rgb + rowBytes;
          while (rgb < rgbend) {
            // y1 and y2 can't overflow
            int y1 = (cyb * rgb[0] + cyg * rgb[1] + cyr * rgb[2] + 0x108000) >> 16;
            yuv[0] = y1;
            int y2 = (cyb * rgb[4] + cyg * rgb[5] + cyr * rgb[6] + 0x108000) >> 16;
            yuv[2] = y2;
            int scaled_y = (((y1 + y2) * 38155 - (32 * 38155 + 0xfffc00)) >> 10);
            int b_y = ((rgb[0] + rgb[4]) << 5) - scaled_y;
            int r_y = ((rgb[2] + rgb[6]) << 5) - scaled_y;
            yuv[1] = clipb[b_y];
            yuv[3] = clipr[r_y];
            yuv += 4;
            rgb += 8;
          }
        }
      }
      vars->videoBytesRead = vars->encBi.biWidth * vars->encBi.biHeight * 2;
    } else {
      OutputDebugString(L"Unexpected inDataFormat for sfYUY2");
    }
  } else if (serveFormat == sfRGB24) {
    if (inDataFormat == idfRGB32) {
      BYTE* rgbaData = (LPBYTE)pFrame;
      BYTE* rgbData = ((LPBYTE)vars) + vars->videooffset;
      int srcRowBytes = rowBytes,
          dstRowBytes = (vars->encBi.biWidth * 3 + 3) & (~3);
#if defined(_M_X64)
      mmx_BGRx_to_RGB24(rgbaData, rgbData, vars->encBi.biHeight, vars->encBi.biWidth, srcRowBytes);
      vars->videoBytesRead = dstRowBytes * vars->encBi.biHeight;
#else
      for (int i = 0; i < vars->encBi.biHeight; i++, rgbData += dstRowBytes, rgbaData += srcRowBytes) {
        int j;
        for (j = 0; j < (vars->encBi.biWidth & 3); j++) {
          rgbData[j * 3 + 0] = rgbaData[j * 4 + 0];
          rgbData[j * 3 + 1] = rgbaData[j * 4 + 1];
          rgbData[j * 3 + 2] = rgbaData[j * 4 + 2];
        }
        BYTE* src = rgbaData + j * 4;
        BYTE* dst = rgbData + j * 3;
        for (j = vars->encBi.biWidth / 4; j > 0; j--) {
          dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2];
          dst[3] = src[4]; dst[4] = src[5]; dst[5] = src[6];
          dst[6] = src[8]; dst[7] = src[9]; dst[8] = src[10];
          dst[9] = src[12]; dst[10] = src[13]; dst[11] = src[14];
          dst += 12; src += 16;
        }
      }
      vars->videoBytesRead = dstRowBytes * vars->encBi.biHeight;
#endif
    } else {
      OutputDebugString(L"Unexpected inDataFormat for sfRGB24");
    }
  } else if (serveFormat == sfRGB32) {
    if (inDataFormat == idfRGB32) {
      vars->videoBytesRead = vars->encBi.biWidth * vars->encBi.biHeight * 4;
      BYTE* src = (LPBYTE)pFrame;
      BYTE* dst = ((LPBYTE)vars) + vars->videooffset;
      int srcRowBytes = rowBytes,
          dstRowBytes = (vars->encBi.biWidth * 4 + 3) & (~3);
#if defined(_M_X64)
      mmx_CopyVideoFrame(src, dst, vars->encBi.biHeight, dstRowBytes, srcRowBytes);
#else
      for (int i = 0; i < vars->encBi.biHeight; i++, dst += dstRowBytes, src += srcRowBytes)
        memcpy(dst, src, dstRowBytes);
#endif
    } else {
      OutputDebugString(L"Unexpected inDataFormat for sfRGB32");
    }
  } else if (serveFormat == sfV210) {
    if (inDataFormat == idfV210) {
      vars->videoBytesRead = rowBytes * vars->encBi.biHeight;
      BYTE* src = (LPBYTE)pFrame;
      BYTE* dst = ((LPBYTE)vars) + vars->videooffset;
      memcpy(dst, src, vars->videoBytesRead);
    } else {
      OutputDebugString(L"Unexpected inDataFormat for sfV210");
    }
  }
}

void FrameServerImpl::Init(bool _hasAudio, DWORD _audioSamplingRate, DWORD _audioBitsPerSample, DWORD _audioChannels,
    DWORD _numVideoFrames, double _fps, DWORD _width, DWORD _height,
    HWND _parentWnd, TCHAR* _filename) {
  hasAudio = _hasAudio;
  audioSamplingRate = _audioSamplingRate;
  audioBitsPerSample = _audioBitsPerSample;
  audioChannels = _audioChannels;
  numVideoFrames = _numVideoFrames;
  fps = _fps;
  width = _width;
  height = _height;
  parentWnd = _parentWnd;
  _tcscpy_s(filename, _countof(filename), _filename);

  LoadCommonResource();
}

bool LoadCommonResource() {
  HKEY key;

  RegCreateKeyEx(HKEY_LOCAL_MACHINE, REGISTRY_FOLDER, 0, 0,
      REG_OPTION_NON_VOLATILE, KEY_READ, 0, &key, 0);
  if (key) {
    DWORD size = sizeof(installDir);
    RegQueryValueEx(key, _T("InstallDir"), 0, 0, (LPBYTE)installDir, &size);
    installDir[size] = 0;
  }
  TCHAR libpath[MAX_PATH];
  _tcscpy_s(libpath, _countof(libpath), installDir);
  _tcscat_s(libpath, _countof(libpath), _T("\\fscommon.dll"));
  ghResInst = LoadLibrary(libpath);
  if (!ghResInst) {
    MessageBox(NULL, _T("Unable to load \"fscommon.dll\". Please reinstall."), APPNAME, MB_OK | MB_ICONEXCLAMATION);
    return false;
  }
  return true;
}

bool FrameServerImpl::BlankAviReadAudioSamples(unsigned long second, void* readData,
    unsigned char** data, unsigned long* datalen) {
  FrameServerImpl* me = (FrameServerImpl*)readData;
  bool rval = me->OnAudioRequestOneSecond(second, data, datalen);

  SendMessage(GetDlgItem(me->servingdlg, IDC_PROGRESS1), PBM_SETPOS, second + 1, 0);
  SendMessage(GetDlgItem(me->servingdlg, IDC_PROGRESS1), PBM_SETPOS, second, 0);
  MSG msg;
  while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return (!stopServing && rval);
}

bool FrameServerImpl::Run() {
  if (!ghResInst)
    return false;
  InitClip();
  CpuDetect();
  if ((audioSamplingRate % 100) && hasAudio) {
    MessageBox(parentWnd, _T("Only audio sampling rates of 16000, 32000, 44100 and 48000 supported"),
        APPNAME, MB_OK | MB_ICONEXCLAMATION);
    return false;
  }

  // Start checking for updates for our app, if the time is right.
  StartUpdateCheck();

  if (DialogBox(ghResInst, MAKEINTRESOURCE(IDD_OPTIONS), parentWnd, OptionsDlgProc) != IDOK)
    return false;

  if (serveVirtualUncompressedAvi && !CheckForDokan(parentWnd, APPNAME))
    return false;

#ifdef _DEBUG
  EXCEPTION_RECORD exrec;
  __try
#endif
  {
    RunImpl();
  }
#ifdef _DEBUG
  __except (exrec = *((EXCEPTION_POINTERS*)_exception_info())->ExceptionRecord, 1) {
    TCHAR str[128];

    _stprintf_s(str, _countof(str), _T("Exception %X at %p, %p"), exrec.ExceptionCode, exrec.ExceptionAddress, ghInst);
    MessageBox(NULL, str, APPNAME, MB_OK);
    exit(0);
  }
#endif
  if (childProcessesJob) {
    if (dokanServeProcInfo.hProcess != NULL) {
      PostThreadMessage(dokanServeProcInfo.dwThreadId, WM_QUIT, 0, 0);
      WaitForSingleObject(dokanServeProcInfo.hThread, 30000);
      CloseHandle(dokanServeProcInfo.hProcess);
      CloseHandle(dokanServeProcInfo.hThread);
    }
    if (commandToRunOnFsStartProcInfo.hProcess != NULL) {
      CloseHandle(commandToRunOnFsStartProcInfo.hProcess);
      CloseHandle(commandToRunOnFsStartProcInfo.hThread);
    }
    CloseHandle(childProcessesJob);
  }

  // remove the signpost file to prevent issues after closing the FS.
  // try to delete it only if it exists.
  for (int i = 0; i < 2; ++i) {
    wchar_t* fileToDelete = i == 0 ? filename : dokanAudioCacheFilePath;
    DWORD dwAttrib = GetFileAttributes(fileToDelete);
    if (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) {
      while (1) {
        TCHAR str[512];
        if (DeleteFile(fileToDelete)) break;
        _stprintf_s(str, _countof(str),
          _T("Unable to delete the output file \"%s\"\n\n")
          _T("This signpost AVI file cannot be used after closing the FrameServer. Please close any applications ")
          _T("that use the signpost AVI and select \"Retry\" to delete it (or \"Cancel\" to ignore)"),
          fileToDelete);
        if (MessageBox(NULL, str, APPNAME, MB_RETRYCANCEL) == IDCANCEL)
          break;
      }
    }
  }
  EnableWindow(parentWnd, TRUE);

  // If we were doing an update check and it is in progress, end it now.
  CancelUpdateCheck();

  return true;
}

void FrameServerImpl::RunImpl() {
  if (!hasAudio)
    pcmAudioInAvi = FALSE;
  stopServing = FALSE;
  pauseServing = FALSE;
  appwnd = parentWnd;

  DWORD nfvideo = numVideoFrames;
  DWORD nfaudio = (DWORD)((double)nfvideo * 100.0 / fps);
  ULONG videosize = width * height * 4;
  ULONG audiosize = (hasAudio) ? (audioBitsPerSample * (audioSamplingRate * 2 + 10000)) / 8 : 0;

  // --------------------- init vars ---------------
  DWORD stream = 0;
  for (stream = 0; ; stream++) {
    char str[64] = "DfscData";
    _ultoa_s(stream, str + strlen(str), _countof(str) - strlen(str), 10);
    varFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,
        sizeof(DfscData) + videosize + audiosize, str);
    if (varFile) {
      if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(varFile);
      } else {
        break;
      }
    }
  }
  vars = (DfscData*)MapViewOfFile(varFile, FILE_MAP_WRITE, 0, 0, 0);
  if (!vars) {
    EnableWindow(parentWnd, TRUE);
    return;
  }
  wcscpy_s(vars->signpostPath, _countof(vars->signpostPath), filename);
  vars->audioRequestFullSecond = FALSE;
  vars->videooffset = sizeof(DfscData);
  vars->audiooffset = vars->videooffset + videosize;
  vars->totalAVDataSize = videosize + audiosize;
  vars->encStatus = 0;
  WAVEFORMATEX wfx;
  wfx.wFormatTag = WAVE_FORMAT_PCM;
  wfx.nChannels = (WORD)audioChannels;
  wfx.nSamplesPerSec = (audioSamplingRate ? audioSamplingRate : 44100);
  wfx.wBitsPerSample = (WORD)audioBitsPerSample;
  wfx.cbSize = 0;
  wfx.nBlockAlign = (wfx.nChannels * wfx.wBitsPerSample) / 8;
  wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
  vars->serveFormat = serveFormat;
  vars->numVideoFrames = numVideoFrames;
  vars->fpsNumerator = (int)(fps * 1000);
  vars->fpsDenominator = 1000;
  vars->encBi.biWidth = width;
  vars->encBi.biHeight = height;
  vars->encBi.biBitCount = (serveFormat == sfYUY2) ? 16 : ((serveFormat == sfRGB24) ? 24 : 32);
  vars->encBi.biSizeImage = vars->encBi.biWidth * vars->encBi.biHeight * vars->encBi.biBitCount / 8;
  vars->wfx = wfx;

#define DefineNameAndCreateEvent(evt, evtManualReset, nameStr, prefix) \
  strcpy_s(nameStr, _countof(nameStr), prefix); \
  _ultoa_s(stream, nameStr + strlen(nameStr), _countof(nameStr) - strlen(nameStr), 10); \
  evt = CreateEventA(NULL, evtManualReset, FALSE, nameStr);

  DefineNameAndCreateEvent(videoEncEvent, TRUE, vars->videoEncEventName, "DfscVideoEncEventName");
  DefineNameAndCreateEvent(videoDecEvent, FALSE, vars->videoDecEventName, "DfscVideoDecEventName");
  DefineNameAndCreateEvent(audioEncEvent, TRUE, vars->audioEncEventName, "DfscAudioEncEventName");
  DefineNameAndCreateEvent(audioDecEvent, FALSE, vars->audioDecEventName, "DfscAudioDecEventName");
  strcpy_s(vars->videoEncSemName, _countof(vars->videoEncSemName), "DfscVideoEncSemName");

  // --------------------- init vars done ---------------

  EnableWindow(parentWnd, FALSE);
  servingdlg = CreateDialog(ghResInst, MAKEINTRESOURCE(IDD_SERVING), parentWnd, ServingDlgProc);
  SetWindowLongPtr(servingdlg, GWLP_USERDATA, (LONG_PTR)this);
  ShowUpdateNotificationIfRequired(servingdlg);
  vars->servingDlg = servingdlg;
  ShowWindow(servingdlg, SW_SHOW);
  signpostProgressDlg = CreateDialog(ghResInst, MAKEINTRESOURCE(IDD_WRITINGSIGNPOST), servingdlg, WritingSignpostDlgProc);
  ShowWindow(signpostProgressDlg, SW_SHOW);
  SendMessage(GetDlgItem(servingdlg, IDC_PROGRESS1), PBM_SETRANGE, 0, MAKELONG(0, (nfaudio + 99) / 100 - 1));
  SendMessage(GetDlgItem(servingdlg, IDC_PROGRESS1), PBM_SETPOS, 0, 0);

  wchar_t signpostPathToShow[MAX_PATH] = { 0 };
  DWORD fourccs[] = {
    /* sfRGB24 */ BI_RGB,
    /* sfRGB32 */ BI_RGB,
    /* sfYUY2 */ mmioFOURCC('Y', 'U', 'Y', '2'),
    /* sfV210 */ mmioFOURCC('v', '2', '1', '0')
  };

  // Should we serve this AVI as a virtual file system AVI?
  childProcessesJob = NULL;
  ZeroMemory(&dokanServeProcInfo, sizeof(dokanServeProcInfo));
  ZeroMemory(&commandToRunOnFsStartProcInfo, sizeof(commandToRunOnFsStartProcInfo));
  if (serveVirtualUncompressedAvi || (runCommandOnFsStart && wcslen(commandToRunOnFsStart) > 0)) {
    // Start a win32 Job to keep track & auto-terminate child processes if we crash.
    childProcessesJob = CreateJobObject(NULL, NULL);
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = { 0 };
    jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    SetInformationJobObject(childProcessesJob, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli));
  }

  if (serveVirtualUncompressedAvi) {
    // Run the child process to proxy the frameserved file as a virtual uncompressed AVI
    wchar_t command[MAX_PATH];
    wcscpy_s(command, _countof(command), installDir);
    wcscat_s(command, _countof(command), L"\\fsproxy.exe");
    wchar_t params[MAX_PATH];
    swprintf_s(params, _countof(params), L"\"%s\" DfscData%d", command, stream);
    STARTUPINFO sinfo = { 0 };
    sinfo.cb = sizeof(sinfo);
    BOOL ret = CreateProcessInJob(childProcessesJob, command, params, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL,
      &sinfo, &dokanServeProcInfo);
    if (!ret || !dokanServeProcInfo.hProcess) {
      MessageBox(servingdlg,
        L"Unable to run the proxy process.\n\n"
        L"Please check the file path you are rendering to and try again.\n\n"
        L"If this issue persists, please reinstall FrameServer report it as an issue.",
        APPNAME, MB_OK | MB_ICONEXCLAMATION);
      stopServing = true;
    }

    // Message loop to serve the audio to proxy process and wait until it creates both the
    // audio cache AVI and virtual uncompressed signpost AVI
    BOOL signpostCreated = FALSE;
    while (!stopServing && !signpostCreated) {
      HANDLE evs[] = { audioEncEvent, dokanServeProcInfo.hProcess };
      MsgWaitForMultipleObjects(_countof(evs), evs, FALSE, INFINITE,
        QS_ALLEVENTS | QS_SENDMESSAGE | QS_MOUSE | QS_MOUSEBUTTON);

      MSG msg;
      while (!signpostCreated && PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_QUIT)  // will be posted by WM_COPYDATA hanlder in WritingSignpostDlgProc
          signpostCreated = TRUE;
      }
      if (WaitForSingleObject(dokanServeProcInfo.hProcess, 0) == WAIT_OBJECT_0) {
        MessageBox(servingdlg,
            L"The proxy process has unexpectedly died. Please stop frameserving and try again.\n\n"
            L"If this issue persists, please report it as an issue.",
            APPNAME, MB_OK | MB_ICONEXCLAMATION);
          stopServing = true;
      }
      if (WaitForSingleObject(audioEncEvent, 0) == WAIT_OBJECT_0) {
        ResetEvent(audioEncEvent);
        vars->audioBytesRead = 0;
        if (vars->audioRequestFullSecond) {
          LPBYTE data = (LPBYTE)vars + vars->audiooffset;
          OnAudioRequestOneSecond(vars->audioFrameIndex, &data, &vars->audioBytesRead);
          SendMessage(GetDlgItem(signpostProgressDlg, IDC_PROGRESS1), PBM_SETPOS, vars->audioFrameIndex + 1, 0);
          SendMessage(GetDlgItem(signpostProgressDlg, IDC_PROGRESS1), PBM_SETPOS, vars->audioFrameIndex, 0);
        } else {
          if (vars->audioFrameIndex < nfaudio)
            OnAudioRequest();
        }
        vars->audioRequestFullSecond = FALSE;
        SetEvent(audioDecEvent);
      }
    }

    // We would've received the virtual uncompressed signpost AVI path via WM_COPYDATA, so show that in the UI
    wcscpy_s(signpostPathToShow, _countof(signpostPathToShow), dokanSignpostFilePath);
  } else {
    // Regular frameserving via a signpost AVI/vfw codecs.
    bool filecreated = false;
    if (IsAudioCompatible(wfx.wBitsPerSample, wfx.nSamplesPerSec)) {
      if (pcmAudioInAvi) {
        wfx.wFormatTag = WAVE_FORMAT_PCM;
        wfx.nBlockAlign = 4;
        wfx.wBitsPerSample = 16;
        wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
        filecreated = CreateBlankAviPcmAudio(nfvideo, (int)(fps * 1000),
            1000, width, height, vars->encBi.biBitCount,
            filename, FOURCC_DFSC, fourccs[serveFormat],
            (ULONG)(((double)nfaudio * wfx.nSamplesPerSec) / 100), &wfx, stream,
            BlankAviReadAudioSamples, this);
      } else {
        wfx.wFormatTag = WAVE_FORMAT_DFSC;
        wfx.nAvgBytesPerSec = 800;
        wfx.nBlockAlign = 1;
        wfx.wBitsPerSample = 0;
        filecreated = CreateBlankAvi(nfvideo, (int)(fps * 1000),
            1000, width, height, vars->encBi.biBitCount,
            filename, FOURCC_DFSC, fourccs[serveFormat],
            nfaudio, &wfx, stream);
      }
    }
    if (filecreated) {
      wcscat_s(signpostPathToShow, _countof(signpostPathToShow), filename);
    } else {
      MessageBox(NULL, _T("Unable to create signpost AVI file. Please close any applications that may be ")
          _T("using this file and try again."),
          APPNAME, MB_OK | MB_ICONEXCLAMATION);
      stopServing = true;
    }
  }

  // This loop seems required to flush out pending messages for the signpost writing dialog we created above.
  // Without this loop, updates to servingdlg such as setting the signpost text below don't seem to show up
  // immediately.
  MSG msg;
  while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  DestroyWindow(signpostProgressDlg);
  signpostProgressDlg = NULL;

  SetDlgItemText(servingdlg, IDC_EDIT1, signpostPathToShow);

  memset(vars + 1, 0, (vars->encBi.biWidth * vars->encBi.biHeight * vars->encBi.biBitCount) >> 3);
  vars->videoFrameIndex = -1;
  vars->audioFrameIndex = -1;
  _tcscpy_s(vars->signpostPath, _countof(vars->signpostPath), filename);

  LPVOID pFrame = 0;
  DWORD framesread = 0, samplesread = 0;
  DWORD lasttime = timeGetTime() - 2000;

  if (networkServing) {
    TCHAR netServer[MAX_PATH * 2];
    _tcscpy_s(netServer, _countof(netServer), installDir);
    _tcscat_s(netServer, _countof(netServer), _T("\\DFsNetServer.exe"));
    STARTUPINFO stinfo;
    PROCESS_INFORMATION pinfo;
    memset(&stinfo, 0, sizeof(stinfo));
    stinfo.cb = sizeof(stinfo);
    if (!CreateProcess(NULL, netServer, NULL, NULL, FALSE, 0, NULL, NULL, &stinfo, &pinfo)) {
      MessageBox(NULL, _T("Unable to invoke network frameserver. Please reinstall."),
          APPNAME, MB_OK | MB_ICONEXCLAMATION);
      stopServing = true;
    }
  }

  HANDLE imageSequenceThreadHandle = NULL;
  if (saveAsImageSequence) {
    imageSequenceThreadHandle = CreateThread(NULL, 0,
        ImageSequenceExportThreadProc, vars, 0, NULL);
  } else if (runCommandOnFsStart && wcslen(commandToRunOnFsStart) > 0) {
    wchar_t commandline[MAX_PATH * 2] = L"";
    wchar_t* ptr = commandToRunOnFsStart;
    while (*ptr != 0) {
      wchar_t* searchPos = wcsstr(ptr, L"%~1");
      if (searchPos) {
        wcsncat_s(commandline, _countof(commandline), ptr, searchPos - ptr);
        wcscat_s(commandline, _countof(commandline), signpostPathToShow);
        ptr = searchPos + 3;
      } else {
        wcscat_s(commandline, _countof(commandline), ptr);
        ptr += wcslen(ptr);
      }
    }

    STARTUPINFO sinfo = { 0 };
    sinfo.cb = sizeof(sinfo);
    BOOL ret = CreateProcessInJob(childProcessesJob, NULL, commandline, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL,
      &sinfo, &commandToRunOnFsStartProcInfo);
    if (!ret || !commandToRunOnFsStartProcInfo.hProcess) {
      wcscat_s(commandline, _countof(commandline), L"\n\nUnable to execute this command. Please check and try again.");
      MessageBox(servingdlg, commandline, APPNAME, MB_OK | MB_ICONEXCLAMATION);
      stopServing = true;
    }
  }

  while (!stopServing) {
    TCHAR str[64];
    DWORD curtime = timeGetTime();
    DWORD msec = curtime - lasttime;
    if (msec >= 1000) {
      _stprintf_s(str, _countof(str), _T("(%d client%c connected over the network)"), vars->numNetworkClientsConnected,
          (vars->numNetworkClientsConnected == 1) ? 's' : ' ');
      SetDlgItemText(servingdlg, IDC_NETCLIENTSTATS, str);
      _stprintf_s(str, _countof(str), _T("%d%% realtime (%.2lf frames/sec)"),
          (int)((framesread * 100) / fps), ((double)framesread * 1000) / msec);
      SetDlgItemText(servingdlg, IDC_VIDEOSTATS, str);
      _stprintf_s(str, _countof(str), _T("%d%% realtime (%.2lf samples/sec)"),
          (int)((samplesread * 100) / (audioSamplingRate ? audioSamplingRate : 1)),
          ((double)samplesread * 1000) / msec);
      SetDlgItemText(servingdlg, IDC_AUDIOSTATS, str);
      framesread = 0;
      samplesread = 0;
      lasttime = curtime;
    }

    if (GetActiveWindow() != servingdlg)
      SetWindowPos(servingdlg, GetActiveWindow(), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    // Regular frameserving
    HANDLE evs[10];
    int nevs = 0;
    if (!pauseServing) {
      evs[nevs++] = videoEncEvent;
      if (!pcmAudioInAvi)
        evs[nevs++] = audioEncEvent;
    }
    if (imageSequenceThreadHandle)
      evs[nevs++] = imageSequenceThreadHandle;
    if (dokanServeProcInfo.hProcess)
      evs[nevs++] = dokanServeProcInfo.hProcess;
    if (endAfterRunningCommand && commandToRunOnFsStartProcInfo.hProcess)
      evs[nevs++] = commandToRunOnFsStartProcInfo.hProcess;

    MsgWaitForMultipleObjects(nevs, evs, FALSE, INFINITE,
        QS_ALLEVENTS | QS_SENDMESSAGE | QS_MOUSE | QS_MOUSEBUTTON);

    MSG msg;
    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    if (imageSequenceThreadHandle && WaitForSingleObject(imageSequenceThreadHandle, 0) == WAIT_OBJECT_0) {
      stopServing = true;
    }
    if (dokanServeProcInfo.hProcess && WaitForSingleObject(dokanServeProcInfo.hProcess, 0) == WAIT_OBJECT_0) {
      MessageBox(servingdlg, 
        L"The proxy process has unexpectedly died. Please stop frameserving and try again.\n\n"
        L"If this issue persists, please report it as an issue.",
        APPNAME, MB_OK | MB_ICONEXCLAMATION);
      stopServing = true;
    }
    if (endAfterRunningCommand &&
        commandToRunOnFsStartProcInfo.hProcess &&
        WaitForSingleObject(commandToRunOnFsStartProcInfo.hProcess, 0) == WAIT_OBJECT_0) {
      stopServing = true;
    }
    if (!pauseServing) {
      if (WaitForSingleObject(videoEncEvent, 0) == WAIT_OBJECT_0) {
        ResetEvent(videoEncEvent);
#ifdef DEBUG
        char str[32];
        strcpy_s(str, _countof(str), "video - ");
        _itoa_s(vars->videoFrameIndex, str + strlen(str), _countof(str) - strlen(str), 10);
        strcat_s(str, _countof(str), "\n");
        OutputDebugStringA(str);
#endif
        if (vars->videoFrameIndex < nfvideo)
          OnVideoRequest();
        else
          vars->videoBytesRead = 0;

        SetEvent(videoDecEvent);
        framesread++;
      }
      if ((!pcmAudioInAvi) && (WaitForSingleObject(audioEncEvent, 0) == WAIT_OBJECT_0)) {
        ResetEvent(audioEncEvent);
#ifdef DEBUG
        char str[32];
        strcpy_s(str, _countof(str), "audio - ");
        _itoa_s(vars->audioFrameIndex, str + strlen(str), _countof(str) - strlen(str), 10);
        strcat_s(str, _countof(str), "\n");
        OutputDebugStringA(str);
#endif
        vars->audioBytesRead = 0;
        if (vars->audioRequestFullSecond) {
          LPBYTE data = (LPBYTE)vars + vars->audiooffset;
          if (vars->audioFrameIndex * 100 < nfaudio)
            OnAudioRequestOneSecond(vars->audioFrameIndex, &data, &vars->audioBytesRead);
        }
        else {
          if (vars->audioFrameIndex < nfaudio)
            OnAudioRequest();
        }
        vars->audioRequestFullSecond = FALSE;

        SetEvent(audioDecEvent);
        samplesread += audioSamplingRate / 100;
      }
    }
  }

  if (varFile) {
    vars->encStatus = 1;          // encoder closing.
    SetEvent(videoDecEvent);
    SetEvent(audioDecEvent);

    CloseHandle(videoEncEvent);
    CloseHandle(videoDecEvent);
    CloseHandle(audioEncEvent);
    CloseHandle(audioDecEvent);
    UnmapViewOfFile(vars);
    CloseHandle(varFile);
    varFile = NULL;
  }
  if (imageSequenceThreadHandle) CloseHandle(imageSequenceThreadHandle);
  DestroyWindow(servingdlg);
  SetForegroundWindow(parentWnd);
}
