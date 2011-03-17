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
#include <vfw.h>
#include "ImageSequence.h"

#if !defined(WIN64)
typedef unsigned long ULONG_PTR;
#endif

#include <gdiplus.h>
using namespace Gdiplus;

#define APPNAME "Debugmode FrameServer"

BOOL APIENTRY DllMain(HANDLE, DWORD, LPVOID) {
  return TRUE;
}

void FormatImagePath(const char* format, char* path, int num) {
  strcpy(path, format);
  int numpos = strchr(path, '#') - path;
  if (numpos >= 0) {
    int numsize = 0;
    while (path[numpos + numsize] == '#')
      numsize++;
    char strnum[12];
    itoa(num, strnum, 10);
    int numzeroes = max(numsize - lstrlen(strnum), 0);
    path[numpos] = 0;
    while (numzeroes-- > 0)
      strcat(path, "0");
    strcat(path, strnum);
    strcat(path, format + numpos + numsize);
  }
}

extern "C"
IMAGESEQUENCE_API bool CheckAndPreparePath(const char* pathFormat) {
  if (strlen(pathFormat) < 4 ||
      (!(pathFormat[0] == '\\' && pathFormat[1] == '\\') && !(pathFormat[1] == ':' && pathFormat[2] == '\\'))) {
    MessageBox(NULL, "Please give the full path where images should be stored\n(relative paths are not supported).",
        APPNAME, MB_OK | MB_ICONERROR);
    return false;
  }

  int dirend = strrchr(pathFormat, '\\') - pathFormat;
  int numstart = strchr(pathFormat, '#') - pathFormat;
  if (numstart > 0 && numstart < dirend) {
    MessageBox(NULL, "Number placeholders ('#') are only allowed in the filename, not in the path",
        APPNAME, MB_OK | MB_ICONERROR);
    return false;
  }

  char dir[MAX_PATH * 2], curdir[MAX_PATH * 2];
  strcpy(dir, pathFormat);
  dir[dirend] = 0;

  int i = 2;
  while ((i = strchr(pathFormat + i, '\\') - pathFormat) > 0) {
    strcpy(curdir, pathFormat);
    curdir[i] = 0;
    WIN32_FILE_ATTRIBUTE_DATA attrs;
    if (!GetFileAttributesEx(curdir, GetFileExInfoStandard, &attrs)) {
      if (!CreateDirectory(curdir, NULL))
        break;          // error.
    } else if (!(attrs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
      break;        // some file with the same name exists.
    }
    ++i;
  }
  if (i > 0) {
    // aborted in the middle
    char msg[MAX_PATH * 2] = "Unable to create directory ";
    strcat(msg, dir);
    strcat(msg, "\nPlease check the permissions and try again.");
    MessageBox(NULL, msg, APPNAME, MB_OK | MB_ICONERROR);
    return false;
  }

  FormatImagePath(pathFormat, dir, 1);
  WIN32_FILE_ATTRIBUTE_DATA attrs;
  if (GetFileAttributesEx(dir, GetFileExInfoStandard, &attrs)) {
    if (MessageBox(NULL, "Some files exist in the target directory, continue and overwrite them?",
            APPNAME, MB_YESNO | MB_ICONQUESTION) != IDYES)
      return false;
  }

  return true;
}

extern "C"
IMAGESEQUENCE_API void SaveImageSequence(DfscData* vars, const char* pathFormat, int imageFormat) {
  PAVIFILE aviFile = NULL;
  PAVISTREAM videoStream = NULL;
  PGETFRAME getFrame = NULL;
  const char* errMsg = NULL;

  GdiplusStartupInput input;   // constructor sets default members
  GdiplusStartupOutput output;
  ULONG_PTR gdiplusToken = 0;

  GdiplusStartup(&gdiplusToken, &input, &output);

  do {
    CLSID encoderClsid;
    CLSID bmpEncoderClsid;
    CLSIDFromString(L"{557CF400-1A04-11D3-9A73-0000F81EF32E}", &bmpEncoderClsid);
    CLSID pngEncoderClsid;
    CLSIDFromString(L"{557CF406-1A04-11D3-9A73-0000F81EF32E}", &pngEncoderClsid);
    CLSID jpgEncoderClsid;
    CLSIDFromString(L"{557CF401-1A04-11D3-9A73-0000F81EF32E}", &jpgEncoderClsid);
    CLSID tiffEncoderClsid;
    CLSIDFromString(L"{557cf405-1a04-11d3-9a73-0000f81ef32e}", &tiffEncoderClsid);
    CLSID clsidEncoderQuality;
    CLSIDFromString(L"{1d5be4b5-fa4a-452d-9cdd-5db35105e7eb}", &clsidEncoderQuality);

    encoderClsid = jpgEncoderClsid;
    if (imageFormat == ImageSequenceFormatPNG)
      encoderClsid = pngEncoderClsid;
    if (imageFormat == ImageSequenceFormatTIFF)
      encoderClsid = tiffEncoderClsid;
    if (imageFormat == ImageSequenceFormatBMP)
      encoderClsid = bmpEncoderClsid;

    HRESULT hr = AVIFileOpen(&aviFile, vars->signpostPath, OF_READ, NULL);
    if (hr == CO_E_NOTINITIALIZED) {
      // COM was not initialized by the caller. We'll try to do it for him.
      if (CoInitialize(NULL) == S_OK)
        hr = AVIFileOpen(&aviFile, vars->signpostPath, OF_READ, NULL);
    }
    if (hr != AVIERR_OK) {
      errMsg = "Unable to open signpost AVI file!";
      break;
    }

    hr = AVIFileGetStream(aviFile, &videoStream, streamtypeVIDEO, 0);
    if (FAILED(hr)) {
      errMsg = "Signpost AVI has no video?";
      break;
    }

    getFrame = AVIStreamGetFrameOpen(videoStream, NULL);
    if (!getFrame) {
      errMsg = "Unable to read video from signpost AVI";
      break;
    }

    EncoderParameters saveParams;
    ULONG quality = 100;
    saveParams.Count = 1;
    saveParams.Parameter[0].Guid = clsidEncoderQuality;
    saveParams.Parameter[0].NumberOfValues = 1;
    saveParams.Parameter[0].Type = EncoderParameterValueTypeLong;
    saveParams.Parameter[0].Value = &quality;

    LPBITMAPINFOHEADER lpbi = NULL;
    int frame = 1;
    do {
      char filepath[MAX_PATH * 2];
      FormatImagePath(pathFormat, filepath, frame);
      WCHAR filepathw[MAX_PATH * 2];
      for (int j = 0; j < 1 || filepath[j - 1] != 0; ++j)
        filepathw[j] = filepath[j];

      lpbi = (LPBITMAPINFOHEADER)AVIStreamGetFrame(getFrame, frame - 1);
      if (lpbi != NULL) {
        if (lpbi->biBitCount == 24) {
          int bytesPerLine = (lpbi->biWidth * 3 + 3) & (~3);
          BYTE* scan0 = (BYTE*)(lpbi + 1);
          if (lpbi->biHeight > 0) {
            scan0 += bytesPerLine * (lpbi->biHeight - 1);
            bytesPerLine = -bytesPerLine;
          }
          Bitmap bmp(lpbi->biWidth, lpbi->biHeight, bytesPerLine,
                     PixelFormat24bppRGB, scan0);
          if (bmp.Save(filepathw, &encoderClsid, &saveParams) != Ok) {
            errMsg = "Unable to save image file. Please check permissions in the destination path.";
            break;
          }
        } else {
          errMsg = "Data from signpost AVI is not 24 bit.";
          break;
        }
      }
      ++frame;
    } while (lpbi != NULL);
  } while (0);

  if (getFrame) AVIStreamGetFrameClose(getFrame);
  if (videoStream) AVIStreamClose(videoStream);
  if (aviFile) AVIFileClose(aviFile);
  Gdiplus::GdiplusShutdown(gdiplusToken);
  if (errMsg)
    MessageBox(NULL, errMsg, APPNAME, MB_OK | MB_ICONERROR);
}
