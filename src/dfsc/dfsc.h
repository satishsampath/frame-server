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

#ifndef DFSC_DFSC_H
#define DFSC_DFSC_H

#include <windows.h>
#include <vfw.h>

static const DWORD FOURCC_DFSC = mmioFOURCC('D', 'F', 'S', 'C');
#define WAVE_FORMAT_DFSC 0xDFAC

// Format in which the video data is served, based on user selection
enum {
  sfRGB24 = 0,
  sfRGB32,
  sfYUY2,
  sfV210,
};

#pragma pack(4)

typedef struct {
  char videoEncSemName[64],
       videoEncEventName[64],
       videoDecEventName[64],
       audioEncSemName[64],
       audioEncEventName[64],
       audioDecEventName[64];
  TCHAR signpostPath[MAX_PATH * 2];
  int encStatus, decStatus;
  DWORD videoFrameIndex, videoBytesRead;
  DWORD audioFrameIndex, audioBytesRead;
  BOOL audioRequestFullSecond;
  int numNetworkClientsConnected;
  DWORD totalAVDataSize;
  BITMAPINFOHEADER encBi;
  WAVEFORMATEX wfx;
  DWORD videooffset;
  DWORD audiooffset;
  int serveFormat;
  HWND servingDlg;
  DWORD numVideoFrames;
  int fpsNumerator, fpsDenominator;
} DfscData;

#pragma pack()

#endif
