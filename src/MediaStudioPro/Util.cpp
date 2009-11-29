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
#include "uvideo.h"

DWORD WINAPI dib_GetHeaderSize(PBIH pbih) {
  DWORD dwSize;

  dwSize = pbih->biSize;
  if (pbih->biBitCount == 8)
    dwSize += ((pbih->biClrUsed == 0) ? 256 : pbih->biClrUsed) * sizeof(RGBQUAD);
  if (pbih->biBitCount == 4)
    dwSize += ((pbih->biClrUsed == 0) ? 16 : pbih->biClrUsed) * sizeof(RGBQUAD);
  return dwSize;
}

DWORD WINAPI dib_GetDataSize(PBIH pbih) {
  DWORD dwsize;

  if (pbih->biBitCount == 4)
    dwsize = ((pbih->biWidth * pbih->biBitCount / 8 + 3) & ~3) * pbih->biHeight;
  else
    dwsize = ((pbih->biWidth * (pbih->biBitCount / 8) + 3) & ~3) * pbih->biHeight;
  return dwsize;
}

DWORD WINAPI dib_GetTotalSize(PBIH pbih) {
  return dib_GetHeaderSize(pbih) + dib_GetDataSize(pbih);
}

void WINAPI dib_Prepare(PBIH pbih, int w, int h, int nBits) {
  FillMemory(pbih, sizeof(BIH), 0);
  pbih->biSize = sizeof(BIH);
  pbih->biPlanes = 1;
  pbih->biCompression = BI_RGB;
  pbih->biWidth = w;
  pbih->biHeight = h;
  pbih->biBitCount = nBits;
  if (nBits == 8)
    pbih->biClrUsed = 256;
  if (w && h && nBits)
    pbih->biSizeImage = dib_GetDataSize(pbih);
}

void WINAPI wfx_Prepare(PWFX pWfx, WORD wChannels, WORD wBitsPerSample, DWORD dwSamplesPerSec) {
  FillMemory((LPSTR)pWfx, sizeof(WFX), 0);
  pWfx->wFormatTag = WAVE_FORMAT_PCM;
  pWfx->nChannels = wChannels;
  pWfx->nSamplesPerSec = dwSamplesPerSec;
  pWfx->wBitsPerSample = (wBitsPerSample + 7) & ~7;
  pWfx->nBlockAlign = wChannels * pWfx->wBitsPerSample / 8;
  pWfx->nAvgBytesPerSec = dwSamplesPerSec * pWfx->nBlockAlign;
}
