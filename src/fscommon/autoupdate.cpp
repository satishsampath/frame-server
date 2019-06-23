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

#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <wininet.h>
#include "fscommon_resource.h"
#include "fscommon.h"

// ----------------------------------------------------------------------------------------
// The following code comes from http://www.wischik.com/lu/programmer/setdlgitemurl.html
// Modifications were made to pass in the 'bkcolor' for the url's background color.
// ----------------------------------------------------------------------------------------

#ifndef IDC_HAND
#define IDC_HAND MAKEINTRESOURCE(32649)
#endif

// Implementation notes:
// We have to subclass both the static control (to set its cursor, to respond to click)
// and the dialog procedure (to set the font of the static control). Here are the two
// subclasses:
LRESULT CALLBACK UrlCtlProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK UrlDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
// When the user calls SetDlgItemUrl, then the static-control-subclass is added
// if it wasn't already there, and the dialog-subclass is added if it wasn't
// already there. Both subclasses are removed in response to their respective
// WM_DESTROY messages. Also, each subclass stores a property in its window,
// which is a HGLOBAL handle to a GlobalAlloc'd structure:
typedef struct {
  TCHAR* url;
  WNDPROC oldproc;
  HFONT hf;
  LONG fontWeight;
  COLORREF bkcolor;
} TUrlData;
// I'm a miser and only defined a single structure, which is used by both
// the control-subclass and the dialog-subclass. Both of them use 'oldproc' of course.
// The control-subclass only uses 'url' (in response to WM_LBUTTONDOWN),
// and the dialog-subclass only uses 'hf' and 'hb' (in response to WM_CTLCOLORSTATIC)
// There is one sneaky thing to note. We create our underlined font *lazily*.
// Initially, hf is just NULL. But the first time the subclassed dialog received
// WM_CTLCOLORSTATIC, we sneak a peak at the font that was going to be used for
// the control, and we create our own copy of it but including the underline style.
// This way our code works fine on dialogs of any font.

// SetDlgItemUrl: this is the routine that sets up the subclassing.
void SetDlgItemUrl(HWND hdlg, int id, TCHAR* url, COLORREF bkcolor, LONG fontWeight) { // nb. vc7 has crummy warnings about 32/64bit. My code's perfect! That's why I hide the warnings.
  #pragma warning( push )
  #pragma warning( disable: 4312 4244 )
  // First we'll subclass the edit control
  HWND hctl = GetDlgItem(hdlg, id);
  SetWindowText(hctl, url);
  HGLOBAL hold = (HGLOBAL)GetProp(hctl, _T("href_dat"));
  if (hold != NULL) { // if it had been subclassed before, we merely need to tell it the new url
    TUrlData* ud = (TUrlData*)GlobalLock(hold);
    delete[] ud->url;
    size_t len = _tcslen(url) + 1;
    ud->url = new TCHAR[len]; _tcscpy_s(ud->url, len, url);
  } else {HGLOBAL hglob = GlobalAlloc(GMEM_MOVEABLE, sizeof(TUrlData));
    TUrlData* ud = (TUrlData*)GlobalLock(hglob);
    ud->oldproc = (WNDPROC)GetWindowLongPtr(hctl, GWLP_WNDPROC);
    size_t len = _tcslen(url) + 1;
    ud->url = new TCHAR[len]; _tcscpy_s(ud->url, len, url);
    ud->hf = 0; ud->bkcolor = 0;
    GlobalUnlock(hglob);
    SetProp(hctl, _T("href_dat"), hglob);
    SetWindowLongPtr(hctl, GWLP_WNDPROC, (LONG_PTR)UrlCtlProc);
  }
  //
  // Second we subclass the dialog
  hold = (HGLOBAL)GetProp(hdlg, _T("href_dlg"));
  if (hold == NULL) {
    HGLOBAL hglob = GlobalAlloc(GMEM_MOVEABLE, sizeof(TUrlData));
    TUrlData* ud = (TUrlData*)GlobalLock(hglob);
    ud->url = 0;
    ud->oldproc = (WNDPROC)GetWindowLongPtr(hdlg, GWLP_WNDPROC);
    ud->bkcolor = bkcolor;
    ud->fontWeight = fontWeight;
    ud->hf = 0; // the font will be created lazilly, the first time WM_CTLCOLORSTATIC gets called
    GlobalUnlock(hglob);
    SetProp(hdlg, _T("href_dlg"), hglob);
    SetWindowLongPtr(hdlg, GWLP_WNDPROC, (LONG_PTR)UrlDlgProc);
  }
  #pragma warning( pop )
}

// UrlCtlProc: this is the subclass procedure for the static control
LRESULT CALLBACK UrlCtlProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  HGLOBAL hglob = (HGLOBAL)GetProp(hwnd, _T("href_dat"));

  if (hglob == NULL) return DefWindowProc(hwnd, msg, wParam, lParam);
  TUrlData* oud = (TUrlData*)GlobalLock(hglob); TUrlData ud = *oud;
  GlobalUnlock(hglob); // I made a copy of the structure just so I could GlobalUnlock it now, to be more local in my code
  switch (msg) {
  case WM_DESTROY:
  { RemoveProp(hwnd, _T("href_dat")); GlobalFree(hglob);
    if (ud.url != 0) delete[] ud.url;
    // nb. remember that ud.url is just a pointer to a memory block. It might look weird
    // for us to delete ud.url instead of oud->url, but they're both equivalent.
  } break;
  case WM_LBUTTONDOWN:
  { HWND hdlg = GetParent(hwnd); if (hdlg == 0) hdlg = hwnd;
    ShellExecute(hdlg, _T("open"), ud.url, NULL, NULL, SW_SHOWNORMAL);} break;
  case WM_SETCURSOR:
  { SetCursor(LoadCursor(NULL, IDC_HAND));
    return TRUE;}
  case WM_NCHITTEST:
  { return HTCLIENT;   // because normally a static returns HTTRANSPARENT, so disabling WM_SETCURSOR
  }
  }
  return CallWindowProc(ud.oldproc, hwnd, msg, wParam, lParam);
}

// UrlDlgProc: this is the subclass procedure for the dialog
LRESULT CALLBACK UrlDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  HGLOBAL hglob = (HGLOBAL)GetProp(hwnd, _T("href_dlg"));

  if (hglob == NULL) return DefWindowProc(hwnd, msg, wParam, lParam);
  TUrlData* oud = (TUrlData*)GlobalLock(hglob); TUrlData ud = *oud;
  GlobalUnlock(hglob);
  switch (msg) {
  case WM_DESTROY:
  { RemoveProp(hwnd, _T("href_dlg")); GlobalFree(hglob);
    if (ud.hf != 0) DeleteObject(ud.hf);} break;
  case WM_CTLCOLORSTATIC:
  { HDC hdc = (HDC)wParam; HWND hctl = (HWND)lParam;
    // To check whether to handle this control, we look for its subclassed property!
    HANDLE hprop = GetProp(hctl, _T("href_dat"));
    if (hprop == NULL) return CallWindowProc(ud.oldproc, hwnd, msg, wParam, lParam);
    // There has been a lot of faulty discussion in the newsgroups about how to change
    // the text colour of a static control. Lots of people mess around with the
    // TRANSPARENT text mode. That is incorrect. The correct solution is here:
    // (1) Leave the text opaque. This will allow us to re-SetDlgItemText without it looking wrong.
    // (2) SetBkColor. This background colour will be used underneath each character cell.
    // (3) return HBRUSH. This background colour will be used where there's no text.
    SetTextColor(hdc, RGB(255, 255, 255));
    SetBkColor(hdc, ud.bkcolor);
    if (ud.hf == 0) { // we use lazy creation of the font. That's so we can see font was currently being used.
      TEXTMETRIC tm; GetTextMetrics(hdc, &tm);
      LOGFONT lf;
      lf.lfHeight = tm.tmHeight;
      lf.lfWidth = 0;
      lf.lfEscapement = 0;
      lf.lfOrientation = 0;
      lf.lfWeight = ud.fontWeight;
      lf.lfItalic = tm.tmItalic;
      lf.lfUnderline = TRUE;
      lf.lfStrikeOut = tm.tmStruckOut;
      lf.lfCharSet = tm.tmCharSet;
      lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
      lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
      lf.lfQuality = DEFAULT_QUALITY;
      lf.lfPitchAndFamily = tm.tmPitchAndFamily;
      GetTextFace(hdc, LF_FACESIZE, lf.lfFaceName);
      ud.hf = CreateFontIndirect(&lf);
      TUrlData* oud = (TUrlData*)GlobalLock(hglob); oud->hf = ud.hf; GlobalUnlock(hglob);
    }
    SelectObject(hdc, ud.hf);
    // Note: the win32 docs say to return an HBRUSH, typecast as a BOOL. But they
    // fail to explain how this will work in 64bit windows where an HBRUSH is 64bit.
    // I have supressed the warnings for now, because I hate them...
      #pragma warning( push )
      #pragma warning( disable: 4311 )
    return (BOOL)CreateSolidBrush(ud.bkcolor);
      #pragma warning( pop )
  }
  }
  return CallWindowProc(ud.oldproc, hwnd, msg, wParam, lParam);
}

// ----------------------------------------------------------------------------------------
// End of code from http://www.wischik.com/lu/programmer/setdlgitemurl.html
// ----------------------------------------------------------------------------------------

void ShowUpdateNotificationIfRequired(HWND dlg) {
  TCHAR updateUrl[MAX_PATH * 2] = _T("");
  HKEY key = 0;

  RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\DebugMode\\FrameServer"), 0, 0,
      REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0, &key, 0);
  if (key) {
    DWORD size = sizeof(updateUrl);
    RegQueryValueEx(key, _T("updateUrl"), 0, 0, (LPBYTE)updateUrl, &size);
    RegCloseKey(key);
  }

  RECT labelRect, urlRect;
  GetWindowRect(GetDlgItem(dlg, IDC_UPDATE_LABEL), &labelRect);
  GetWindowRect(GetDlgItem(dlg, IDC_UPDATE_URL), &urlRect);
  ScreenToClient(dlg, ((POINT*)&labelRect));
  ScreenToClient(dlg, ((POINT*)&labelRect) + 1);
  ScreenToClient(dlg, ((POINT*)&urlRect));
  ScreenToClient(dlg, ((POINT*)&urlRect) + 1);

  RECT clientRect = { 0 };
  GetClientRect(dlg, &clientRect);
  RECT windowRect = { 0 };
  GetWindowRect(dlg, &windowRect);
  int diffHeight = 0;
  if (_tcslen(updateUrl) != 0) {
    SetDlgItemUrl(dlg, IDC_UPDATE_URL, updateUrl, RGB(255,0,0), FW_BOLD);
    diffHeight = clientRect.bottom - urlRect.bottom;
  } else {
    diffHeight = labelRect.top - clientRect.bottom;
  }

  windowRect.bottom += diffHeight;
  MoveWindow(dlg, windowRect.left, windowRect.top, windowRect.right - windowRect.left,
      windowRect.bottom - windowRect.top, TRUE);
}

extern HINSTANCE ghResInst;
HANDLE updateCheckThreadHandle = NULL;
HINTERNET updateCheckInternetHandle = NULL;

void HandleUpdateResponse(char* response) {
  TCHAR updateUrl[300];

  updateUrl[0] = 0;
  int nextUpdateIntervalSeconds = 0;
  bool updateUrlFound = false;
#ifndef _DEBUG
  EXCEPTION_RECORD exrec;
  __try
#endif
  {
    char* end = response + strlen(response);
    while (response < end) {
      size_t endOfLine;
      if (strchr(response, '\n') != NULL) {
        endOfLine = strchr(response, '\n') - response;
      } else {
        endOfLine = strlen(response);
      }
      response[endOfLine] = 0;
      if (strstr(response, "nu=") == response) {
        nextUpdateIntervalSeconds = atoi(response + 3);
      } else if (strstr(response, "uu=") == response) {
        updateUrlFound = true;
        int i = 0;
        while (response[i + 3] != 0) {
          updateUrl[i] = response[i + 3];
          ++i;
        }
        updateUrl[i] = 0;
      }
      response += endOfLine + 1;
    }
  }
#ifndef _DEBUG
  __except(exrec = *((EXCEPTION_POINTERS*)_exception_info())->ExceptionRecord, 1) {
    // Silently treat this as a parse failure and go with defaults below.
  }
#endif

  if (nextUpdateIntervalSeconds == 0)
    nextUpdateIntervalSeconds = 60 * 60 * 24;      // 1 day by default

  HKEY key = 0;
  RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\DebugMode\\FrameServer"), 0, 0,
      REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0, &key, 0);
  if (key) {
    nextUpdateIntervalSeconds += (rand() % 1000) - 500;
    __int64 nextUpdateTime = 0;
    GetSystemTimeAsFileTime((FILETIME*)&nextUpdateTime);
    nextUpdateTime += nextUpdateIntervalSeconds * 10000;
    RegSetValueEx(key, _T("nextUpdateCheckTime"), 0, REG_BINARY, (LPBYTE)&nextUpdateTime, sizeof(nextUpdateTime));
    if (updateUrlFound) {      // change what is in the registry only if we got a response from the server.
      if (_tcslen(updateUrl) > 0) {
        RegSetValueEx(key, _T("updateUrl"), 0, REG_SZ, (LPBYTE)updateUrl, (DWORD)(_tcslen(updateUrl) + 1) * sizeof(TCHAR));
      } else {
        RegDeleteValue(key, _T("updateUrl"));
      }
    }
    RegCloseKey(key);
  }
}

DLGPROCRET CALLBACK DummyAboutDlgProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
  return FALSE;
}

bool FormatUpdateRequestUrl(TCHAR* updateUrl, int updateUrlSize) {
  HKEY key = 0;

  RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\DebugMode\\FrameServer"), 0, 0,
      REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0, &key, 0);
  if (!key)
    return false;

  // Install ID
  TCHAR guidstr[40];
  guidstr[0] = 0;
  DWORD size = sizeof(guidstr);
  RegQueryValueEx(key, _T("installId"), 0, 0, (LPBYTE)guidstr, &size);
  if (_tcslen(guidstr) == 0) {
    // There is no install ID yet, create one.
    GUID guid = GUID_NULL;
    TCHAR* guidstrptr = NULL;
    UuidCreate(&guid);
#ifdef UNICODE
    UuidToString(&guid, (LPWORD*)(&guidstrptr));
#else
    UuidToString(&guid, (LPBYTE*)(&guidstrptr));
#endif
    int guidstri = 0;
    for (int i = 0; guidstrptr[i] != 0; ++i)
      if (guidstrptr[i] != '-')
        guidstr[guidstri++] = guidstrptr[i];
    guidstr[guidstri++] = 0;
#ifdef UNICODE
    RpcStringFree((LPWORD*)(&guidstrptr));
#else
    RpcStringFree((LPBYTE*)(&guidstrptr));
#endif

    RegSetValueEx(key, _T("installId"), 0, REG_SZ, (LPBYTE)guidstr, (DWORD)(_tcslen(guidstr) + 1) * sizeof(TCHAR));
  }
  RegCloseKey(key);

  // Load version info from the about dialog.
  HWND aboutDlg = CreateDialog(ghResInst, MAKEINTRESOURCE(IDD_DFABOUTDLG), NULL,
      DummyAboutDlgProc);
  TCHAR version[32];
  GetDlgItemText(aboutDlg, IDC_VERSION, version, sizeof(version) / sizeof(TCHAR));
  DestroyWindow(aboutDlg);

  // Process name
  TCHAR appPath[MAX_PATH * 2];
  GetModuleFileName(NULL, appPath, sizeof(appPath));
  appPath[MAX_PATH * 2 - 1] = 0;
  TCHAR* appFileName = _tcsrchr(appPath, '\\');
  if (appFileName == NULL) {
    appFileName = appPath;
  } else {
    appFileName++;
  }

  // OS version
  OSVERSIONINFO osver = { sizeof(OSVERSIONINFO) };
  GetVersionEx(&osver);

  // an = application name, "fs" in this case
  // rv = version of the URL request format, version 1 now.
  // id = installId
  // ov = os version
  // av = app version
  _tcscpy_s(updateUrl, updateUrlSize, _T("https://www.debugmode.com/bin/update.php?an=fs&rv=1"));
  _tcscat_s(updateUrl, updateUrlSize, _T("&pn="));
  _tcscat_s(updateUrl, updateUrlSize, appFileName);
  _tcscat_s(updateUrl, updateUrlSize, _T("&ov="));
  _itot_s(osver.dwMajorVersion, updateUrl + _tcslen(updateUrl), updateUrlSize - _tcslen(updateUrl), 10);
  _tcscat_s(updateUrl, updateUrlSize, _T("."));
  _itot_s(osver.dwMinorVersion, updateUrl + _tcslen(updateUrl), updateUrlSize - _tcslen(updateUrl), 10);
  _tcscat_s(updateUrl, updateUrlSize, _T("."));
  _itot_s(osver.dwBuildNumber, updateUrl + _tcslen(updateUrl), updateUrlSize - _tcslen(updateUrl), 10);
  _tcscat_s(updateUrl, updateUrlSize, _T("&av="));
  _tcscat_s(updateUrl, updateUrlSize, version);
  _tcscat_s(updateUrl, updateUrlSize, _T("&id="));
  _tcscat_s(updateUrl, updateUrlSize, guidstr);

  return true;
}

DWORD WINAPI UpdateCheckThreadProc(LPVOID param) {
  bool connected = false;
  DWORD connFlags = 0;
  
  if (InternetGetConnectedState(&connFlags, 0)) {    // are we connected?
    TCHAR updateUrl[300];
    if (FormatUpdateRequestUrl(updateUrl, _countof(updateUrl))) {
      HINTERNET hwi = InternetOpen(_T("Debugmode update check"), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL,
          INTERNET_FLAG_NO_CACHE_WRITE);
      if (hwi != NULL) {
        updateCheckInternetHandle = InternetOpenUrl(hwi, updateUrl, NULL, 0,
          INTERNET_FLAG_SECURE | INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS, 0);

        if (updateCheckInternetHandle != NULL) {
          connected = true;

          char response[4000];
          DWORD responseSize = 0;
          if (InternetReadFile(updateCheckInternetHandle, response, sizeof(response)-1, &responseSize)) {
            response[responseSize] = 0;
            HandleUpdateResponse(response);
          }
          InternetCloseHandle(updateCheckInternetHandle);
          updateCheckInternetHandle = NULL;
        }
        InternetCloseHandle(hwi);
      }
    }
  }

  if (!connected) {
    // Process an empty response so that the next update time is calculated properly and we don't
    // keep connecting to the server on each run.
    HandleUpdateResponse("");
  }

  CloseHandle(updateCheckThreadHandle);
  updateCheckThreadHandle = NULL;
  return 0;
}

void StartUpdateCheck() {
  if (updateCheckThreadHandle != NULL)
    return;      // already in progress.

  __int64 curTime = 0;
  GetSystemTimeAsFileTime((FILETIME*)&curTime);
  __int64 nextCheckTime = 0;

  HKEY key = 0;
  RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\DebugMode\\FrameServer"), 0, 0,
      REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0, &key, 0);
  if (key) {
    DWORD size = sizeof(nextCheckTime);
    RegQueryValueEx(key, _T("nextUpdateCheckTime"), 0, 0, (LPBYTE)&nextCheckTime, &size);
    RegCloseKey(key);
  }

  if (nextCheckTime > curTime)
    return;      // Have to wait for more time.

  // Create a thread which checks for update asynchronously.
  DWORD updateCheckThreadId = 0;
  updateCheckThreadHandle = CreateThread(NULL, 0, UpdateCheckThreadProc, NULL, 0, &updateCheckThreadId);
}

void CancelUpdateCheck() {
  if (updateCheckThreadHandle == NULL)
    return;      // Not running.

  // Try for 3 seconds to see if the update thread initialized, and if not abandon it.
  for (int i = 0; i < 30 && updateCheckInternetHandle == NULL; ++i) {
    Sleep(100);
    if (updateCheckThreadHandle == NULL)
      return;        // terminated already.
  }
  if (updateCheckInternetHandle != NULL) {
    InternetCloseHandle(updateCheckInternetHandle);
    WaitForSingleObject(updateCheckThreadHandle, 5000);
    updateCheckInternetHandle = NULL;
  }

  updateCheckThreadHandle = NULL;
}
