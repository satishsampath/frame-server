/**
 * Debugmode Frameserver
 * Copyright (C) 2002-2009 Satish Kumar, All Rights Reserved
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
#include <mmsystem.h>
#include <commctrl.h>
#include <stdio.h>
#include <shlobj.h>
#include "fscommon_resource.h"
#include "fscommon.h"
#include "blankavi.h"
#include "utils.h"
#include "ImageSequence.h"

#pragma warning(disable:4996)

#define APPNAME _T("Debugmode FrameServer")
#ifndef BIF_NEWDIALOGSTYLE
    #define BIF_NEWDIALOGSTYLE 0x0040
#endif

BOOL saveAsImageSequence = FALSE;
TCHAR imageSequencePath[MAX_PATH * 2] = _T("");
UINT imageSequenceFormat = ImageSequenceFormatPNG;

BOOL pcmAudioInAvi = FALSE, networkServing = FALSE;
UINT networkPort = 8278, serveFormat = sfRGB24;
BOOL stopServing = FALSE;
HWND appwnd;
TCHAR installDir[MAX_PATH] = _T("C:\\Program Files\\Debugmode\\Frameserver");
extern HINSTANCE ghInst;
HINSTANCE ghResInst;
unsigned char* clip, * clipr, * clipb;
HMODULE imageSequenceModule = NULL;

extern void ShowUpdateNotificationIfRequired(HWND dlg);
extern void StartUpdateCheck();
extern void CancelUpdateCheck();

void SetFsIconForWindow(HWND wnd) {
  SendMessage(wnd, WM_SETICON, ICON_BIG, (LPARAM)LoadImage(ghResInst, MAKEINTRESOURCE(IDI_FRAMESERVER),
          IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR));
  SendMessage(wnd, WM_SETICON, ICON_SMALL, (LPARAM)LoadImage(ghResInst, MAKEINTRESOURCE(IDI_FRAMESERVER),
          IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR));
}

bool LoadImageSequenceModule() {
  if (imageSequenceModule != NULL)
    return true;

  TCHAR libpath[MAX_PATH];
  _tcscpy(libpath, installDir);
#if defined(WIN64)
  _tcscat(libpath, _T("\\ImageSequence-x64.dll"));
#else
  _tcscat(libpath, _T("\\ImageSequence.dll"));
#endif
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
  char* pathParam = NULL;
#ifdef _UNICODE
  char filepath[MAX_PATH * 2];
  WideCharToMultiByte(CP_ACP, 0, imageSequencePath, -1, filepath, sizeof(filepath), NULL, NULL);
  pathParam = filepath;
#else
  pathParam = imageSequencePath;
#endif
  Save((DfscData*)param, pathParam, imageSequenceFormat);

  return 0;
}

DLGPROCRET CALLBACK AboutDlgProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
  TCHAR copyright[1024];

  switch (msg) {
  case WM_INITDIALOG: LoadString(ghResInst, IDS_ABOUTDLG_COPYRIGHT, copyright, 1024);
    ::SetDlgItemText(dlg, IDC_WARNING, copyright);
    SetFsIconForWindow(dlg);
    return TRUE;
  case WM_COMMAND:
    if (LOWORD(wp) == IDOK || LOWORD(wp) == IDCANCEL)
      EndDialog(dlg, 0);
  }
  return FALSE;
}

void UpdateImageSequenceControls(HWND dlg) {
  EnableWindow(GetDlgItem(dlg, IDC_SERVEASRGB24), !saveAsImageSequence);
  EnableWindow(GetDlgItem(dlg, IDC_SERVEASRGB32), !saveAsImageSequence);
  EnableWindow(GetDlgItem(dlg, IDC_SERVEASYUY2), !saveAsImageSequence);
  EnableWindow(GetDlgItem(dlg, IDC_PCMAUDIOINAVI), !saveAsImageSequence);
  EnableWindow(GetDlgItem(dlg, IDC_NETSERVE), !saveAsImageSequence);
  EnableWindow(GetDlgItem(dlg, IDC_NETPORT), !saveAsImageSequence);
  EnableWindow(GetDlgItem(dlg, IDC_IMAGESEQUENCEPATH), saveAsImageSequence);
  EnableWindow(GetDlgItem(dlg, IDC_IMAGESEQUENCEBROWSE), saveAsImageSequence);
  EnableWindow(GetDlgItem(dlg, IDC_IMAGESEQUENCEFORMAT), saveAsImageSequence);
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
    RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\DebugMode\\FrameServer"), 0, 0,
        REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0, &key, 0);
    if (key) {
      DWORD size = sizeof(pcmAudioInAvi);
      RegQueryValueEx(key, _T("pcmAudioInAvi"), 0, 0, (LPBYTE)&pcmAudioInAvi, &size);
      RegQueryValueEx(key, _T("networkServing"), 0, 0, (LPBYTE)&networkServing, &size);
      size = sizeof(serveFormat);
      RegQueryValueEx(key, _T("serveFormat"), 0, 0, (LPBYTE)&serveFormat, &size);
      size = sizeof(networkPort);
      RegQueryValueEx(key, _T("networkPort"), 0, 0, (LPBYTE)&networkPort, &size);
      size = sizeof(saveAsImageSequence);
      RegQueryValueEx(key, _T("saveAsImageSequence"), 0, 0, (LPBYTE)&saveAsImageSequence, &size);
      size = sizeof(imageSequenceFormat);
      RegQueryValueEx(key, _T("imageSequenceFormat"), 0, 0, (LPBYTE)&imageSequenceFormat, &size);
      size = sizeof(imageSequencePath);
      RegQueryValueEx(key, _T("imageSequencePath"), 0, 0, (LPBYTE)imageSequencePath, &size);
      RegCloseKey(key);
    }
    if (_tcslen(imageSequencePath) == 0)
      _tcscpy(imageSequencePath, _T("C:\\FrameServer Images\\Image######.png"));

    CheckDlgButton(dlg, IDC_PCMAUDIOINAVI, pcmAudioInAvi);
    CheckRadioButton(dlg, IDC_SERVEASRGB24, IDC_SERVEASYUY2, IDC_SERVEASRGB24 + serveFormat);
    CheckDlgButton(dlg, IDC_NETSERVE, networkServing);
    SetDlgItemInt(dlg, IDC_NETPORT, networkPort, FALSE);
    EnableWindow(GetDlgItem(dlg, IDC_NETPORT), networkServing);

    SendMessage(GetDlgItem(dlg, IDC_IMAGESEQUENCEFORMAT), CB_ADDSTRING, 0, (LPARAM)_T("JPEG"));
    SendMessage(GetDlgItem(dlg, IDC_IMAGESEQUENCEFORMAT), CB_ADDSTRING, 0, (LPARAM)_T("PNG"));
    SendMessage(GetDlgItem(dlg, IDC_IMAGESEQUENCEFORMAT), CB_ADDSTRING, 0, (LPARAM)_T("TIFF"));
    SendMessage(GetDlgItem(dlg, IDC_IMAGESEQUENCEFORMAT), CB_ADDSTRING, 0, (LPARAM)_T("BMP"));
    SendMessage(GetDlgItem(dlg, IDC_IMAGESEQUENCEFORMAT), CB_SETCURSEL, imageSequenceFormat, 0);

    SetDlgItemText(dlg, IDC_IMAGESEQUENCEPATH, imageSequencePath);
    CheckRadioButton(dlg, IDC_OUTPUTVIDEO, IDC_OUTPUTIMAGESEQUENCE,
        (saveAsImageSequence ? IDC_OUTPUTIMAGESEQUENCE : IDC_OUTPUTVIDEO));
    UpdateImageSequenceControls(dlg);
  }
  break;
  case WM_COMMAND:
    if (LOWORD(wp) == IDC_NETSERVE)
      EnableWindow(GetDlgItem(dlg, IDC_NETPORT), IsDlgButtonChecked(dlg, IDC_NETSERVE));

    if (LOWORD(wp) == IDC_IMAGESEQUENCEBROWSE) {
      TCHAR filepath[MAX_PATH], filename[MAX_PATH];
      GetDlgItemText(dlg, IDC_IMAGESEQUENCEPATH, filepath, sizeof(filepath));
      filename[0] = 0;
      if (_tcsrchr(filepath, '\\') != NULL) {
        _tcscpy(filename, _tcsrchr(filepath, '\\') + 1);
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
        _tcscat(filepath, _T("\\"));

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
          _tcscat(filepath, _T("\\"));
          _tcscat(filepath, filename);
        }
        SetDlgItemText(dlg, IDC_IMAGESEQUENCEPATH, filepath);
      }

      CoUninitialize();
    }

    if (LOWORD(wp) == IDC_IMAGESEQUENCEFORMAT && HIWORD(wp) == CBN_SELCHANGE) {
      TCHAR filepath[MAX_PATH];
      GetDlgItemText(dlg, IDC_IMAGESEQUENCEPATH, filepath, sizeof(filepath));
      if (_tcsrchr(filepath, '.') != NULL)
        _tcsrchr(filepath, '.')[0] = 0;

      LRESULT sel = SendMessage(GetDlgItem(dlg, IDC_IMAGESEQUENCEFORMAT), CB_GETCURSEL, 0, 0);
      if (sel == 0) _tcscat(filepath, _T(".jpg"));
      if (sel == 1) _tcscat(filepath, _T(".png"));
      if (sel == 2) _tcscat(filepath, _T(".tif"));
      if (sel == 3) _tcscat(filepath, _T(".bmp"));
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

    if (LOWORD(wp) == IDCANCEL)
      EndDialog(dlg, IDCANCEL);

    if (LOWORD(wp) == IDOK) {
      if (saveAsImageSequence) {
        // Check if the path and filenames are good enough
        char dstpath[MAX_PATH * 2];
        GetDlgItemTextA(dlg, IDC_IMAGESEQUENCEPATH, dstpath, sizeof(dstpath));
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
      networkServing = IsDlgButtonChecked(dlg, IDC_NETSERVE);
      networkPort = GetDlgItemInt(dlg, IDC_NETPORT, NULL, FALSE);
      saveAsImageSequence = IsDlgButtonChecked(dlg, IDC_OUTPUTIMAGESEQUENCE);
      GetDlgItemText(dlg, IDC_IMAGESEQUENCEPATH, imageSequencePath, sizeof(imageSequencePath));
      imageSequenceFormat = (UINT)SendMessage(GetDlgItem(dlg, IDC_IMAGESEQUENCEFORMAT), CB_GETCURSEL, 0, 0);

      HKEY key = 0;
      RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\DebugMode\\FrameServer"), 0, 0, REG_OPTION_NON_VOLATILE,
          KEY_ALL_ACCESS, 0, &key, 0);
      if (key) {
        RegSetValueEx(key, _T("pcmAudioInAvi"), 0, REG_DWORD, (LPBYTE)&pcmAudioInAvi, sizeof(pcmAudioInAvi));
        RegSetValueEx(key, _T("networkServing"), 0, REG_DWORD, (LPBYTE)&networkServing, sizeof(networkServing));
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
  return FALSE;
}

DLGPROCRET CALLBACK ServingDlgProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
  switch (msg) {
  case WM_INITDIALOG:
    SetFsIconForWindow(dlg);
    CheckDlgButton(dlg, IDC_PCMAUDIOINAVI, pcmAudioInAvi);
    CheckRadioButton(dlg, IDC_SERVEASRGB24, IDC_SERVEASYUY2, IDC_SERVEASRGB24 + serveFormat);
    CheckDlgButton(dlg, IDC_NETSERVE, networkServing);
    SetDlgItemInt(dlg, IDC_NETPORT, networkPort, FALSE);
    break;
  case WM_COMMAND:
    if (LOWORD(wp) == IDCANCEL)
      stopServing = TRUE;
    if (LOWORD(wp) == IDC_ABOUT)
      DialogBox(ghResInst, MAKEINTRESOURCE(IDD_DFABOUTDLG), dlg, AboutDlgProc);
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
  case WM_CTLCOLORSTATIC:
  {
    HDC hdc = (HDC)wp;
    HWND wnd = (HWND)lp;
    int id = GetDlgCtrlID(wnd);
    if (id == IDC_UPDATE_URL || id == IDC_UPDATE_LABEL) {
      SetBkColor(hdc, GetSysColor(COLOR_INFOBK));
      return (BOOL)GetSysColorBrush(COLOR_INFOBK);
    }
  }
  break;
  }
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
    }
  } else if (serveFormat == sfRGB24) {
    if (inDataFormat == idfRGB32) {
      BYTE* rgbaData = (LPBYTE)pFrame;
      BYTE* rgbData = ((LPBYTE)vars) + vars->videooffset;
      int srcRowBytes = rowBytes,
          dstRowBytes = (vars->encBi.biWidth * 3 + 3) & (~3);
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
    }
  } else if (serveFormat == sfRGB32) {
    if (inDataFormat == idfRGB32) {
      vars->videoBytesRead = vars->encBi.biWidth * vars->encBi.biHeight * 4;
      BYTE* src = (LPBYTE)pFrame;
      BYTE* dst = ((LPBYTE)vars) + vars->videooffset;
      int srcRowBytes = rowBytes,
          dstRowBytes = (vars->encBi.biWidth * 4 + 3) & (~3);
      for (int i = 0; i < vars->encBi.biHeight; i++, dst += dstRowBytes, src += srcRowBytes)
        memcpy(dst, src, dstRowBytes);
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
  _tcscpy(filename, _filename);

  LoadCommonResource();
}

bool LoadCommonResource() {
  HKEY key;

  RegCreateKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\DebugMode\\FrameServer"), 0, 0,
      REG_OPTION_NON_VOLATILE, KEY_READ, 0, &key, 0);
  if (key) {
    DWORD size = sizeof(installDir);
    RegQueryValueEx(key, _T("InstallDir"), 0, 0, (LPBYTE)installDir, &size);
    installDir[size] = 0;
  }
  TCHAR libpath[MAX_PATH];
  _tcscpy(libpath, installDir);
  _tcscat(libpath, _T("\\fscommon.dll"));
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

  SendMessage(GetDlgItem(me->servingdlg, IDC_PROGRESS1), PBM_SETPOS, second, 0);
  MSG msg;
  while (PeekMessage(&msg, 0, 0, 0, 1)) {
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

  if (DialogBox(ghResInst, MAKEINTRESOURCE(IDD_OPTIONS), parentWnd, OptionsDlgProc) != IDOK)
    return false;

  if (!hasAudio)
    pcmAudioInAvi = FALSE;
  stopServing = FALSE;
  EnableWindow(parentWnd, FALSE);
  appwnd = parentWnd;
  servingdlg = CreateDialog(ghResInst, MAKEINTRESOURCE(IDD_WRITINGSIGNPOST), parentWnd, WritingSignpostDlgProc);
  ShowWindow(servingdlg, SW_SHOW);

#ifndef _DEBUG
  EXCEPTION_RECORD exrec;
  __try
#endif
  {
    DWORD nfvideo = numVideoFrames;
    DWORD nfaudio = (DWORD)((double)nfvideo * 100.0 / fps);
    ULONG videosize = width * height * 4;
    ULONG audiosize = (hasAudio) ? (audioBitsPerSample * (audioSamplingRate * 2 + 10000)) / 8 : 0;

    // --------------------- init vars ---------------
    DWORD stream = 0;
    for (stream = 0; ; stream++) {
      char str[64] = "DfscData";
      ultoa(stream, str + strlen(str), 10);
      varFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,
          sizeof(DfscData) + videosize + audiosize, str);
      if (varFile) {
        if (GetLastError() == ERROR_ALREADY_EXISTS)
          CloseHandle(varFile);
        else
          break;
      }
    }
    vars = (DfscData*)MapViewOfFile(varFile, FILE_MAP_WRITE, 0, 0, 0);
    if (!vars) {
      EnableWindow(parentWnd, TRUE);
      return false;
    }
    vars->audioRequestFullSecond = FALSE;
    vars->videooffset = sizeof(DfscData);
    vars->audiooffset = vars->videooffset + videosize;
    vars->totalAVDataSize = videosize + audiosize;

    strcpy(vars->videoEncEventName, "DfscVideoEncEventName");
    strcpy(vars->videoDecEventName, "DfscVideoDecEventName");
    strcpy(vars->audioEncEventName, "DfscAudioEncEventName");
    strcpy(vars->audioDecEventName, "DfscAudioDecEventName");

    ultoa(stream, vars->videoEncEventName + strlen(vars->videoEncEventName), 10);
    ultoa(stream, vars->videoDecEventName + strlen(vars->videoDecEventName), 10);
    ultoa(stream, vars->audioEncEventName + strlen(vars->audioEncEventName), 10);
    ultoa(stream, vars->audioDecEventName + strlen(vars->audioDecEventName), 10);

    videoEncEvent = CreateEventA(NULL, TRUE, FALSE, vars->videoEncEventName);
    videoDecEvent = CreateEventA(NULL, FALSE, FALSE, vars->videoDecEventName);
    audioEncEvent = CreateEventA(NULL, TRUE, FALSE, vars->audioEncEventName);
    audioDecEvent = CreateEventA(NULL, FALSE, FALSE, vars->audioDecEventName);

    vars->encStatus = 0;
    // --------------------- init vars done ---------------

    SendMessage(GetDlgItem(servingdlg, IDC_PROGRESS1), PBM_SETRANGE, 0, MAKELONG(0, (nfaudio + 99) / 100 - 1));
    SendMessage(GetDlgItem(servingdlg, IDC_PROGRESS1), PBM_SETPOS, 0, 0);

    WAVEFORMATEX wfx;
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = (WORD)audioChannels;
    wfx.nSamplesPerSec = (audioSamplingRate ? audioSamplingRate : 44100);
    wfx.wBitsPerSample = (WORD)audioBitsPerSample;
    wfx.cbSize = 0;
    wfx.nBlockAlign = (wfx.nChannels * wfx.wBitsPerSample) / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    vars->encBi.biWidth = width;
    vars->encBi.biHeight = height;
    vars->encBi.biBitCount = (serveFormat == sfYUY2) ? 16 : ((serveFormat == sfRGB24) ? 24 : 32);
    vars->encBi.biSizeImage = vars->encBi.biWidth * vars->encBi.biHeight * vars->encBi.biBitCount / 8;
    vars->wfx = wfx;

    bool filecreated = false;
    if (IsAudioCompatible(wfx.wBitsPerSample, wfx.nSamplesPerSec)) {
      if (pcmAudioInAvi) {
        wfx.wFormatTag = WAVE_FORMAT_PCM;
        wfx.nBlockAlign = 4;
        wfx.wBitsPerSample = 16;
        wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
        filecreated = CreateBlankAviPcmAudio(nfvideo, (int)(fps * 1000),
            1000, width, height, vars->encBi.biBitCount,
            filename, mmioFOURCC('D', 'F', 'S', 'C'),
            (ULONG)(((double)nfaudio * wfx.nSamplesPerSec) / 100), &wfx, stream,
            BlankAviReadAudioSamples, this);
      } else {
        wfx.wFormatTag = WAVE_FORMAT_DFSC;
        wfx.nAvgBytesPerSec = 800;
        wfx.nBlockAlign = 1;
        wfx.wBitsPerSample = 0;
        filecreated = CreateBlankAvi(nfvideo, (int)(fps * 1000),
            1000, width, height, vars->encBi.biBitCount,
            filename, mmioFOURCC('D', 'F', 'S', 'C'), nfaudio, &wfx, stream);
      }
    }
    if (!filecreated) {
      MessageBox(NULL, _T("Unable to create signpost AVI file. Please close any applications that may be ")
          _T("using this file and try again."),
          APPNAME, MB_OK | MB_ICONEXCLAMATION);
      stopServing = true;
    }

    DestroyWindow(servingdlg);
    servingdlg = CreateDialog(ghResInst, MAKEINTRESOURCE(IDD_SERVING), parentWnd, ServingDlgProc);
    SetDlgItemText(servingdlg, IDC_EDIT1, filename);
    ShowUpdateNotificationIfRequired(servingdlg);
    ShowWindow(servingdlg, SW_SHOW);

    // Start checking for updates for our app, if the time is right.
    StartUpdateCheck();

    memset(vars + 1, 0, (vars->encBi.biWidth * vars->encBi.biHeight * vars->encBi.biBitCount) >> 3);
    vars->videoFrameIndex = -1;
    vars->audioFrameIndex = -1;
#ifdef _UNICODE
    WideCharToMultiByte(CP_ACP, 0, filename, -1, vars->signpostPath, sizeof(vars->signpostPath), NULL, NULL);
#else
    strcpy(vars->signpostPath, filename);
#endif

    LPVOID pFrame = 0;
    DWORD framesread = 0, samplesread = 0;
    DWORD lasttime = timeGetTime() - 2000;

    if (networkServing) {
      TCHAR netServer[MAX_PATH * 2];
      _tcscpy(netServer, installDir);
      _tcscat(netServer, _T("\\DFsNetServer.exe"));
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
    }

    while (!stopServing) {
      TCHAR str[64];
      DWORD curtime = timeGetTime();
      DWORD msec = curtime - lasttime;
      if (msec >= 1000) {
        _stprintf(str, _T("(%d client%c connected over the network)"), vars->numNetworkClientsConnected,
            (vars->numNetworkClientsConnected == 1) ? 's' : ' ');
        SetDlgItemText(servingdlg, IDC_NETCLIENTSTATS, str);
        _stprintf(str, _T("%d%% realtime (%.2lf frames/sec)"),
            (int)((framesread * 100) / fps), ((double)framesread * 1000) / msec);
        SetDlgItemText(servingdlg, IDC_VIDEOSTATS, str);
        _stprintf(str, _T("%d%% realtime (%.2lf samples/sec)"),
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
      evs[nevs++] = videoEncEvent;
      if (!pcmAudioInAvi)
        evs[nevs++] = audioEncEvent;
      if (imageSequenceThreadHandle)
        evs[nevs++] = imageSequenceThreadHandle;

      MsgWaitForMultipleObjects(nevs, evs, FALSE, INFINITE,
          QS_ALLEVENTS | QS_SENDMESSAGE | QS_MOUSE | QS_MOUSEBUTTON);

      MSG msg;
      while (PeekMessage(&msg, 0, 0, 0, TRUE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
      if (imageSequenceThreadHandle &&
          WaitForSingleObject(imageSequenceThreadHandle, 0) == WAIT_OBJECT_0) {
        stopServing = true;
      }
      if (WaitForSingleObject(videoEncEvent, 0) == WAIT_OBJECT_0) {
        ResetEvent(videoEncEvent);

        char str[32]; strcpy(str, "video - ");
        itoa(vars->videoFrameIndex, str + strlen(str), 10);
        strcat(str, "\n");
        OutputDebugStringA(str);

        if (vars->videoFrameIndex < nfvideo)
          OnVideoRequest();
        else
          vars->videoBytesRead = 0;

        SetEvent(videoDecEvent);
        framesread++;
      }
      if ((!pcmAudioInAvi) && (WaitForSingleObject(audioEncEvent, 0) == WAIT_OBJECT_0)) {
        ResetEvent(audioEncEvent);

        char str[32]; strcpy(str, "audio - ");
        itoa(vars->audioFrameIndex, str + strlen(str), 10);
        strcat(str, "\n");
        OutputDebugStringA(str);

        vars->audioBytesRead = 0;
        if (vars->audioRequestFullSecond) {
          LPBYTE data = (LPBYTE)vars + vars->audiooffset;
          if (vars->audioFrameIndex * 100 < nfaudio)
            OnAudioRequestOneSecond(vars->audioFrameIndex, &data, &vars->audioBytesRead);
        } else {
          if (vars->audioFrameIndex < nfaudio)
            OnAudioRequest();
        }
        vars->audioRequestFullSecond = FALSE;

        SetEvent(audioDecEvent);
        samplesread += audioSamplingRate / 100;
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
#ifndef _DEBUG
  __except(exrec = *((EXCEPTION_POINTERS*)_exception_info())->ExceptionRecord, 1) {
    TCHAR str[128];

    _stprintf(str, _T("Exception %X at %X, %X"), exrec.ExceptionCode, exrec.ExceptionAddress, ghInst);
    MessageBox(NULL, str, APPNAME, MB_OK);
    exit(0);
  }
#endif
  // remove the signpost file to prevent issues after closing the FS.
  BOOL rval = FALSE;
  while (1) {
    // try to delete it only if it exists.
    HANDLE hfile = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hfile == INVALID_HANDLE_VALUE) break;
    CloseHandle(hfile);
    TCHAR str[512];
    if (DeleteFile(filename)) break;
    _stprintf(str, _T("Unable to delete the output file \"%s\"\n\n")
        _T("This signpost AVI file cannot be used after closing the FrameServer. Please close any applications ")
        _T("that use the signpost AVI and select \"Retry\" to delete it (or \"Cancel\" to ignore)"),
        filename);
    if (MessageBox(NULL, str, APPNAME, MB_RETRYCANCEL) == IDCANCEL)
      break;
  }
  EnableWindow(parentWnd, TRUE);

  // If we were doing an update check and it is in progress, end it now.
  CancelUpdateCheck();

  return true;
}

