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

#include "../dfsc.h"
#include "../../utils/utils.h"
#include <crtdbg.h>
#include <STDIO.H>
#include <TCHAR.H>

class DfscInstance {
public:
  bool decompressing;

  DfscData* vars;
  HANDLE varFile;
  HANDLE videoEncSem, videoEncEvent, videoDecEvent;
  int curCmpIndex;

public:
  DfscInstance();
  BOOL QueryAbout();
  DWORD About(HWND hwnd);

  BOOL QueryConfigure();
  DWORD Configure(HWND hwnd);

  DWORD GetState(LPVOID pv, DWORD dwSize);
  DWORD SetState(LPVOID pv, DWORD dwSize);

  DWORD GetInfo(ICINFO* icinfo, DWORD dwSize);

  DWORD CompressFramesInfo(ICCOMPRESSFRAMES* icif);
  DWORD CompressQuery(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
  DWORD CompressGetFormat(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
  DWORD CompressBegin(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
  DWORD CompressGetSize(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
  DWORD Compress(ICCOMPRESS* icinfo, DWORD dwSize);
  DWORD CompressEnd();

  void ConvertRGB24toYUY2(const unsigned char* src, unsigned char* dst, int width, int height);

  DWORD DecompressQuery(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
  DWORD DecompressGetFormat(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
  DWORD DecompressBegin(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
  DWORD Decompress(ICDECOMPRESS* icinfo, DWORD dwSize);
  DWORD DecompressGetPalette(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
  DWORD DecompressEnd();
};

// ------------ Defines ---------------------------

TCHAR szDescription[] = TEXT("DebugMode FSVFWC (internal use)");
TCHAR szName[] = TEXT("DebugMode FSVFWC (internal use)");

#define VERSION         0x00010000   // 1.0

HMODULE hmoduleDfsc = 0;

DfscInstance * Open(ICOPEN* icinfo);
DWORD Close(DfscInstance* pinst);

// ------------ Code ---------------------------

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD, LPVOID) {
  hmoduleDfsc = (HMODULE)hinstDLL;
  return TRUE;
}

// NOTE x64 platform:
//	64-bit pointers are being passed via the 32-bit lParam parameters.
//	This effectively zeroes the highest 32 bits of a pointer.
//	HWND is officially safe to pass (https://msdn.microsoft.com/en-us/library/aa384203.aspx).
//	Not sure about the other things, but nobody seems to care and it actually works...
//
LRESULT PASCAL DriverProc(DWORD_PTR dwDriverID, HDRVR hDriver, UINT uiMessage, LPARAM lParam1, LPARAM lParam2) {
  DfscInstance* pi = (DfscInstance*)(DWORD_PTR)dwDriverID;

  switch (uiMessage) {
  case DRV_LOAD:
    return (LRESULT)1L;

  case DRV_FREE:
    return (LRESULT)1L;

  case DRV_OPEN:
    return (LRESULT)(DWORD_PTR)Open((ICOPEN*)lParam2);

  case DRV_CLOSE:
    if (pi) Close(pi);
    return (LRESULT)1L;

  /*********************************************************************
     state messages
  *********************************************************************/

  case DRV_QUERYCONFIGURE:      // configuration from drivers applet
    return (LRESULT)1L;

  case DRV_CONFIGURE:
    pi->Configure((HWND)(DWORD_PTR)lParam1);
    return DRV_OK;

  case ICM_CONFIGURE:
    //
    // return ICERR_OK if you will do a configure box, error otherwise
    //
    if (lParam1 == -1)
      return pi->QueryConfigure() ? ICERR_OK : ICERR_UNSUPPORTED;
    else
      return pi->Configure((HWND)(DWORD_PTR)lParam1);

  case ICM_ABOUT:
    //
    // return ICERR_OK if you will do a about box, error otherwise
    //
    if (lParam1 == -1)
      return pi->QueryAbout() ? ICERR_OK : ICERR_UNSUPPORTED;
    else
      return pi->About((HWND)(DWORD_PTR)lParam1);

  case ICM_GETSTATE:
    return pi->GetState((LPVOID)(DWORD_PTR)lParam1, (DWORD)lParam2);

  case ICM_SETSTATE:
    return pi->SetState((LPVOID)(DWORD_PTR)lParam1, (DWORD)lParam2);

  case ICM_GETINFO:
    return pi->GetInfo((ICINFO*)(DWORD_PTR)lParam1, (DWORD)lParam2);

  case ICM_GETDEFAULTQUALITY:
    if (lParam1) {
      *((LPDWORD)(DWORD_PTR)lParam1) = 7500;
      return ICERR_OK;
    }
    break;

  /*********************************************************************
     compression messages
  *********************************************************************/

  case ICM_COMPRESS_FRAMES_INFO:
    return pi->CompressFramesInfo((ICCOMPRESSFRAMES*)(DWORD_PTR)lParam1);

  case ICM_COMPRESS_QUERY:
    return pi->CompressQuery((LPBITMAPINFOHEADER)(DWORD_PTR)lParam1, (LPBITMAPINFOHEADER)(DWORD_PTR)lParam2);

  case ICM_COMPRESS_BEGIN:
    return pi->CompressBegin((LPBITMAPINFOHEADER)(DWORD_PTR)lParam1, (LPBITMAPINFOHEADER)(DWORD_PTR)lParam2);

  case ICM_COMPRESS_GET_FORMAT:
    return pi->CompressGetFormat((LPBITMAPINFOHEADER)(DWORD_PTR)lParam1, (LPBITMAPINFOHEADER)(DWORD_PTR)lParam2);

  case ICM_COMPRESS_GET_SIZE:
    return pi->CompressGetSize((LPBITMAPINFOHEADER)(DWORD_PTR)lParam1, (LPBITMAPINFOHEADER)(DWORD_PTR)lParam2);

  case ICM_COMPRESS:
    return pi->Compress((ICCOMPRESS*)(DWORD_PTR)lParam1, (DWORD)lParam2);

  case ICM_COMPRESS_END:
    return pi->CompressEnd();

  /*********************************************************************
     decompress messages
  *********************************************************************/

  case ICM_DECOMPRESS_QUERY:
    return pi->DecompressQuery((LPBITMAPINFOHEADER)(DWORD_PTR)lParam1, (LPBITMAPINFOHEADER)(DWORD_PTR)lParam2);

  case ICM_DECOMPRESS_BEGIN:
    return pi->DecompressBegin((LPBITMAPINFOHEADER)(DWORD_PTR)lParam1, (LPBITMAPINFOHEADER)(DWORD_PTR)lParam2);

  case ICM_DECOMPRESS_GET_FORMAT:
    return pi->DecompressGetFormat((LPBITMAPINFOHEADER)(DWORD_PTR)lParam1, (LPBITMAPINFOHEADER)(DWORD_PTR)lParam2);

  case ICM_DECOMPRESS_GET_PALETTE:
    return pi->DecompressGetPalette((LPBITMAPINFOHEADER)(DWORD_PTR)lParam1, (LPBITMAPINFOHEADER)(DWORD_PTR)lParam2);

  case ICM_DECOMPRESS:
    return pi->Decompress((ICDECOMPRESS*)(DWORD_PTR)lParam1, (DWORD)lParam2);

  case ICM_DECOMPRESS_END:
    return pi->DecompressEnd();

  /*********************************************************************
     standard driver messages
  *********************************************************************/

  case DRV_DISABLE:
  case DRV_ENABLE:
    return (LRESULT)1L;

  case DRV_INSTALL:
  case DRV_REMOVE:
    return (LRESULT)DRV_OK;
  }

  if (uiMessage < DRV_USER)
    return DefDriverProc(dwDriverID, hDriver, uiMessage, lParam1, lParam2);
  else
    return ICERR_UNSUPPORTED;
}

/********************************************************************
********************************************************************/

DfscInstance * Open(ICOPEN* icinfo) {
  if (icinfo && icinfo->fccType != ICTYPE_VIDEO)
    return NULL;

  DfscInstance* pinst = new DfscInstance();

  if (icinfo) icinfo->dwError = pinst ? ICERR_OK : ICERR_MEMORY;

  return pinst;
}

DWORD Close(DfscInstance* pinst) {
// delete pinst;       // this caused problems when deleting at app close time
  return 1;
}

static BOOL CALLBACK AboutDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  if (uMsg == WM_COMMAND) {
    switch (LOWORD(wParam)) {
    case IDOK:
      EndDialog(hwndDlg, 0);
      break;
    }
  }
  return FALSE;
}

DfscInstance::DfscInstance() {
  varFile = 0;
  vars = 0;
  curCmpIndex = 0;
}

BOOL DfscInstance::QueryAbout() {
  return TRUE;
}

DWORD DfscInstance::About(HWND hwnd) {
  MessageBox(NULL, _T(
          "DebugMode FrameServer VFW Codec\nCopyright 2000-2019 Satish Kumar. S\n\nhttp://www.debugmode.com/"),
      _T("About"), MB_OK);
  return ICERR_OK;
}

BOOL DfscInstance::QueryConfigure() {
  return FALSE;
}

DWORD DfscInstance::Configure(HWND hwnd) {
  return ICERR_OK;
}

/********************************************************************
********************************************************************/

DWORD DfscInstance::GetState(LPVOID pv, DWORD dwSize) {
  return 0;
}

DWORD DfscInstance::SetState(LPVOID pv, DWORD dwSize) {
  return 0;
}

DWORD DfscInstance::GetInfo(ICINFO* icinfo, DWORD dwSize) {
  if (icinfo == NULL)
    return sizeof(ICINFO);

  if (dwSize < sizeof(ICINFO))
    return 0;

  icinfo->dwSize = sizeof(ICINFO);
  icinfo->fccType = ICTYPE_VIDEO;
  icinfo->fccHandler = FOURCC_DFSC;
  icinfo->dwFlags = 0;

  icinfo->dwVersion = VERSION;
  icinfo->dwVersionICM = ICVERSION;
  wcsncpy_s(icinfo->szName, _countof(icinfo->szName), szName, _TRUNCATE);
  wcsncpy_s(icinfo->szDescription, _countof(icinfo->szDescription), szDescription, _TRUNCATE);

  return sizeof(ICINFO);
}

/********************************************************************
********************************************************************/

DWORD DfscInstance::CompressFramesInfo(ICCOMPRESSFRAMES* icif) {
  return ICERR_OK;
}

DWORD DfscInstance::CompressQuery(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut) {
  return ICERR_OK;
}

DWORD DfscInstance::CompressGetFormat(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut) {
  if (!lpbiOut)
    return sizeof(BITMAPINFOHEADER);

  *lpbiOut = *lpbiIn;
  lpbiOut->biCompression = FOURCC_DFSC;
  return ICERR_OK;
}

DWORD DfscInstance::CompressBegin(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut) {
  CompressEnd();
  curCmpIndex = 0;
  return ICERR_OK;
}

DWORD DfscInstance::CompressGetSize(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut) {
  return 4;
}

DWORD DfscInstance::Compress(ICCOMPRESS* icinfo, DWORD dwSize) {
  if (icinfo->lpckid)
    *icinfo->lpckid = FOURCC_DFSC;
  *icinfo->lpdwFlags = AVIIF_KEYFRAME;

  *(DWORD*)icinfo->lpOutput = curCmpIndex++;
  icinfo->lpbiOutput->biSizeImage = 4;
  return ICERR_OK;
}

DWORD DfscInstance::CompressEnd() {
  return ICERR_OK;
}

/********************************************************************
********************************************************************/

DWORD DfscInstance::DecompressQuery(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut) {
  if (lpbiIn->biCompression != FOURCC_DFSC)
    return ICERR_BADFORMAT;

  if (lpbiOut) {
    char st[32];
    int outHeight = lpbiOut->biHeight;
    bool isYUY2 = (lpbiIn->biBitCount == 16);
    DWORD inComp = BI_RGB;
    WORD inBitCount = lpbiIn->biBitCount;
    if (isYUY2) {
      outHeight = abs(outHeight);
      inComp = MAKEFOURCC('Y', 'U', 'Y', '2');
      inBitCount = lpbiOut->biBitCount;
    }
    if (inBitCount != lpbiOut->biBitCount ||
        inComp != lpbiOut->biCompression ||
        lpbiIn->biHeight != outHeight ||
        lpbiIn->biWidth != lpbiOut->biWidth ||
        lpbiIn->biPlanes != lpbiOut->biPlanes) {
      sprintf_s(st, _countof(st), "0x%X - 0x%X\n", lpbiIn->biBitCount, lpbiOut->biBitCount);
      OutputDebugStringA(st);
      sprintf_s(st, _countof(st), "0x%X - 0x%X\n", lpbiIn->biHeight, lpbiOut->biHeight);
      OutputDebugStringA(st);
      sprintf_s(st, _countof(st), "0x%X - 0x%X\n", lpbiIn->biWidth, lpbiOut->biWidth);
      OutputDebugStringA(st);
      sprintf_s(st, _countof(st), "0x%X - 0x%X\n", lpbiIn->biPlanes, lpbiOut->biPlanes);
      OutputDebugStringA(st);
      return ICERR_BADFORMAT;
    }
  }
  return ICERR_OK;
}

DWORD DfscInstance::DecompressGetFormat(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut) {
  if (lpbiOut == NULL)
    return sizeof(BITMAPINFOHEADER);

  *lpbiOut = *lpbiIn;
  lpbiOut->biSize = sizeof(BITMAPINFOHEADER);
  lpbiOut->biPlanes = 1;
  lpbiOut->biSizeImage = lpbiIn->biWidth * abs(lpbiIn->biHeight) * (lpbiIn->biBitCount / 8);
  lpbiOut->biCompression = BI_RGB;
  if (lpbiIn->biBitCount == 16) {
    lpbiOut->biCompression = MAKEFOURCC('Y', 'U', 'Y', '2');
    lpbiOut->biBitCount = 32;
    lpbiOut->biSizeImage /= 2;
  }

  return ICERR_OK;
}

DWORD DfscInstance::DecompressBegin(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut) {
  int outHeight = lpbiOut->biHeight;
  bool isYUY2 = (lpbiIn->biBitCount == 16);
  DWORD inComp = BI_RGB;
  WORD inBitCount = lpbiIn->biBitCount;

  if (isYUY2) {
    outHeight = abs(outHeight);
    inComp = MAKEFOURCC('Y', 'U', 'Y', '2');
    inBitCount = lpbiOut->biBitCount;
  }
  if (inBitCount != lpbiOut->biBitCount ||
      inComp != lpbiOut->biCompression ||
      lpbiIn->biHeight != outHeight ||
      lpbiIn->biWidth != lpbiOut->biWidth ||
      lpbiIn->biPlanes != lpbiOut->biPlanes) {
    return ICERR_BADFORMAT;
  }
  DecompressEnd();

  decompressing = true;

  return ICERR_OK;
}

DWORD DfscInstance::Decompress(ICDECOMPRESS* icinfo, DWORD dwSize) {
#ifndef _DEBUG
  EXCEPTION_RECORD exrec;
  __try
#endif
  {
    if (!decompressing) {
      DWORD retval = DecompressBegin(icinfo->lpbiInput, icinfo->lpbiOutput);
      if (retval != ICERR_OK)
        return retval;
    }

    if (!vars) {
      varFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(DfscData), "DfscNetData");
      if (GetLastError() != ERROR_ALREADY_EXISTS) {
        CloseHandle(varFile);
        varFile = NULL;
      }
      if (!varFile) {
        DWORD stream = ((DWORD*)icinfo->lpInput)[1];
        char str[64] = "DfscData";
        _ultoa_s(stream, str + strlen(str), _countof(str) - strlen(str), 10);
        varFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(DfscData), str);
        if (GetLastError() != ERROR_ALREADY_EXISTS) {
          CloseHandle(varFile);
          varFile = NULL;
        }
      }
      if (!varFile)
        return ICERR_ABORT;

      vars = (DfscData*)MapViewOfFile(varFile, FILE_MAP_WRITE, 0, 0, 0);
      if (!vars)
        return ICERR_BADFORMAT;

      videoEncSem = CreateSemaphoreA(NULL, 1, 1, vars->videoEncSemName);
      videoEncEvent = CreateEventA(NULL, FALSE, FALSE, vars->videoEncEventName);
      videoDecEvent = CreateEventA(NULL, FALSE, FALSE, vars->videoDecEventName);
    }

    WaitForSingleObject(videoEncSem, 10000);
    {
      if (vars->encStatus == 1)         // encoder closed
        return ICERR_ABORT;

      icinfo->lpbiOutput->biSizeImage = vars->videoBytesRead;

      vars->videoFrameIndex = ((DWORD*)icinfo->lpInput)[0];
      SetEvent(videoEncEvent);
      // OutputDebugString("Waiting for video...");

      if (WaitForSingleObject(videoDecEvent, INFINITE) != WAIT_OBJECT_0)
        return ICERR_OK;            // some error.
      if (vars->encStatus == 1)         // encoder closed
        return ICERR_ABORT;
      // OutputDebugString("got video...");
      fast_memcpy(icinfo->lpOutput, ((LPBYTE)vars) + vars->videooffset, vars->videoBytesRead);
    }
    ReleaseSemaphore(videoEncSem, 1, NULL);
  }
#ifndef _DEBUG
  __except(exrec = *((EXCEPTION_POINTERS*)_exception_info())->ExceptionRecord, 1) {
    TCHAR str[128];

    _stprintf_s(str, _countof(str), _T("Exception %X at %p, %p"), exrec.ExceptionCode, exrec.ExceptionAddress, hmoduleDfsc);
    MessageBox(NULL, str, _T("DebugMode FrameServer"), MB_OK);
  }
#endif
  return ICERR_OK;
}

DWORD DfscInstance::DecompressGetPalette(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut) {
  return ICERR_BADFORMAT;
}

DWORD DfscInstance::DecompressEnd() {
  decompressing = false;
  // return ICERR_OK;
  if (varFile) {
    vars->decStatus = 1;        // decoder closing.

    UnmapViewOfFile(vars);
    CloseHandle(varFile);
    vars = NULL;
    varFile = NULL;
    CloseHandle(videoEncSem);
    CloseHandle(videoEncEvent);
    CloseHandle(videoDecEvent);
  }

  return ICERR_OK;
}
