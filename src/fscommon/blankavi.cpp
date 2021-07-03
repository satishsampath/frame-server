/**
 * Debugmode Frameserver
 Copyright (C) 2002-2009 Satish Kumar, All Rights Reserved
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
#include <avifmt.h>
#include <aviriff.h>
#include "blankavi.h"

bool IsAudioCompatible(int bitsPerSample, int samplingRate) {
  if (bitsPerSample != 8 && bitsPerSample != 16)
    return false;
  if ((samplingRate % 100) != 0)
    return false;
  return true;
}

bool CreateBlankAviPcmAudio(unsigned long numframes, int frrate, int frratescale,
    int width, int height, int videobpp, TCHAR* filename, DWORD containerFcc, DWORD realFcc,
    unsigned long numsamples, WAVEFORMATEX* wfx,
    unsigned long stream, fxnReadAudioSamples readSamples, void* readData) {
  WIN32_FIND_DATA fd;
  HANDLE h = FindFirstFile(filename, &fd);

  if (h != INVALID_HANDLE_VALUE) {
    FindClose(h);
    if (!DeleteFile(filename))
      return false;
  }

  HMMIO file;
  MMCKINFO riffinfo;
  file = mmioOpen(filename, NULL, MMIO_CREATE | MMIO_WRITE);
  if (!file) return false;

  unsigned int numaudioblocks = (numsamples + wfx->nSamplesPerSec - 1) / wfx->nSamplesPerSec;

  riffinfo.cksize = 0;
  riffinfo.fccType = mmioFOURCC('A', 'V', 'I', ' ');
  mmioCreateChunk(file, &riffinfo, MMIO_CREATERIFF);

  MMCKINFO mainlist;
  mainlist.cksize = 0;
  mainlist.fccType = mmioFOURCC('h', 'd', 'r', 'l');
  mmioCreateChunk(file, &mainlist, MMIO_CREATELIST);

  MMCKINFO avihinfo;
  avihinfo.cksize = 0;
  avihinfo.ckid = mmioFOURCC('a', 'v', 'i', 'h');
  mmioCreateChunk(file, &avihinfo, 0);

  AVIMAINHEADER avihdr = { 0, 0, (1000000 * frratescale) / frrate, (frrate / frratescale * 8), 0,
                           AVIF_HASINDEX, numframes, 0, 2, wfx->nSamplesPerSec * wfx->nBlockAlign, width, height, 0, 0, 0, 0 };
  mmioWrite(file, (LPCSTR)&avihdr + 8, sizeof(avihdr) - 8);

  mmioAscend(file, &avihinfo, 0);

  MMCKINFO videostrl;
  videostrl.cksize = 0;
  videostrl.fccType = mmioFOURCC('s', 't', 'r', 'l');
  mmioCreateChunk(file, &videostrl, MMIO_CREATELIST);

  MMCKINFO videostrh;
  videostrh.cksize = 0;
  videostrh.ckid = mmioFOURCC('s', 't', 'r', 'h');
  mmioCreateChunk(file, &videostrh, 0);

  AVISTREAMHEADER videohdr = { 0, 0, streamtypeVIDEO, containerFcc, 0, 0, 0, 0, frratescale, frrate,
                               0, numframes, (width * height * videobpp) / 8, 0, 0 };
  mmioWrite(file, (LPCSTR)&videohdr + 8, sizeof(videohdr) - 8);

  mmioAscend(file, &videostrh, 0);

  MMCKINFO videostrf;
  videostrf.cksize = 0;
  videostrf.ckid = mmioFOURCC('s', 't', 'r', 'f');
  mmioCreateChunk(file, &videostrf, 0);

  // containerFcc is set in the biCompression field so that our frameserver codec gets invoked to read
  // the data.
  // realFcc is the actual fourcc of the bitmap such as BI_RGB, YUY2 or v210. We pass that in to the codec
  // via the biClrImportant field which is otherwise unused in this flow. The decoder, dfsc.cpp, will make
  // use of it.
  BITMAPINFOHEADER videofmt = {
    sizeof(BITMAPINFOHEADER), width, height, 1, videobpp, containerFcc, (width * height * videobpp) / 8,
    0, 0, 0, realFcc
  };
  mmioWrite(file, (LPCSTR)&videofmt, sizeof(videofmt));

  mmioAscend(file, &videostrf, 0);

  mmioAscend(file, &videostrl, 0);

  MMCKINFO audiostrl;
  audiostrl.cksize = 0;
  audiostrl.fccType = mmioFOURCC('s', 't', 'r', 'l');
  mmioCreateChunk(file, &audiostrl, MMIO_CREATELIST);

  MMCKINFO audiostrh;
  audiostrh.cksize = 0;
  audiostrh.ckid = mmioFOURCC('s', 't', 'r', 'h');
  mmioCreateChunk(file, &audiostrh, 0);

  AVISTREAMHEADER audiohdr = { 0, 0, streamtypeAUDIO, 0, 0, 0, 0, 0, wfx->nBlockAlign, wfx->nAvgBytesPerSec,
                               0, numsamples, wfx->nAvgBytesPerSec, 0, wfx->nBlockAlign };
  mmioWrite(file, (LPCSTR)&audiohdr + 8, sizeof(audiohdr) - 8);

  mmioAscend(file, &audiostrh, 0);

  MMCKINFO audiostrf;
  audiostrf.cksize = 0;
  audiostrf.ckid = mmioFOURCC('s', 't', 'r', 'f');
  mmioCreateChunk(file, &audiostrf, 0);

  mmioWrite(file, (LPCSTR)wfx, sizeof(WAVEFORMATEX));

  mmioAscend(file, &audiostrf, 0);

  mmioAscend(file, &audiostrl, 0);

  mmioAscend(file, &mainlist, 0);

  MMCKINFO movilist;
  movilist.cksize = 0;
  movilist.fccType = mmioFOURCC('m', 'o', 'v', 'i');
  mmioCreateChunk(file, &movilist, MMIO_CREATELIST);

  unsigned int i;
  unsigned long *audioBlockByteLen = new unsigned long[numaudioblocks];
  for (i = 0; i < numaudioblocks; i++) {
    MMCKINFO audioframe;
    audioframe.cksize = 0;
    audioframe.ckid = mmioFOURCC('0', '1', 'w', 'b');
    mmioCreateChunk(file, &audioframe, 0);

    unsigned char* data = NULL;
    unsigned long datalen;
    if (!(*readSamples)(i, readData, &data, &datalen)) {
      mmioClose(file, 0);
      delete[] audioBlockByteLen;
      DeleteFile(filename);
      return true;
    }
    audioBlockByteLen[i] = datalen;
    mmioWrite(file, (LPCSTR)data, datalen);
    free(data);

    mmioAscend(file, &audioframe, 0);
  }

  for (i = 0; i < numframes; i++) {
    MMCKINFO videoframe;
    videoframe.cksize = 0;
    videoframe.ckid = mmioFOURCC('0', '0', 'd', 'c');
    mmioCreateChunk(file, &videoframe, 0);

    DWORD videodata = i;
    mmioWrite(file, (LPCSTR)&videodata, sizeof(videodata));
    mmioWrite(file, (LPCSTR)&stream, sizeof(stream));

    mmioAscend(file, &videoframe, 0);
  }

  mmioAscend(file, &movilist, 0);

  MMCKINFO indexchunk;
  indexchunk.cksize = 0;
  indexchunk.ckid = mmioFOURCC('i', 'd', 'x', '1');
  mmioCreateChunk(file, &indexchunk, 0);

  AVIINDEXENTRY indexentry;
  indexentry.ckid = mmioFOURCC('0', '1', 'w', 'b');
  indexentry.dwFlags = AVIIF_KEYFRAME;
  indexentry.dwChunkOffset = 4;
  for (i = 0; i < numaudioblocks; i++) {
    indexentry.dwChunkLength = audioBlockByteLen[i];
    mmioWrite(file, (LPCSTR)&indexentry, sizeof(indexentry));
    indexentry.dwChunkOffset += (8 + indexentry.dwChunkLength);
  }

  indexentry.ckid = mmioFOURCC('0', '0', 'd', 'c');
  indexentry.dwFlags = AVIIF_KEYFRAME;
  indexentry.dwChunkLength = 8;
  for (i = 0; i < numframes; i++) {
    mmioWrite(file, (LPCSTR)&indexentry, sizeof(indexentry));
    indexentry.dwChunkOffset += (8 + 4 + 4);
  }

  mmioAscend(file, &indexchunk, 0);

  mmioAscend(file, &riffinfo, 0);
  mmioClose(file, 0);
  delete[] audioBlockByteLen;
  return true;
}

bool CreateBlankAvi(unsigned long numframes, int frrate, int frratescale,
    int width, int height, int videobpp, TCHAR* filename, DWORD containerFcc, DWORD realFcc,
    unsigned long numaudioblocks, WAVEFORMATEX* wfx,
    unsigned long stream) {
  WIN32_FIND_DATA fd;
  HANDLE h = FindFirstFile(filename, &fd);

  if (h != INVALID_HANDLE_VALUE) {
    FindClose(h);
    if (!DeleteFile(filename))
      return false;
  }

  HMMIO file;
  MMCKINFO riffinfo;
  file = mmioOpen(filename, NULL, MMIO_CREATE | MMIO_WRITE);
  if (!file) return false;

  riffinfo.cksize = 0;
  riffinfo.fccType = mmioFOURCC('A', 'V', 'I', ' ');
  mmioCreateChunk(file, &riffinfo, MMIO_CREATERIFF);

  MMCKINFO mainlist;
  mainlist.cksize = 0;
  mainlist.fccType = mmioFOURCC('h', 'd', 'r', 'l');
  mmioCreateChunk(file, &mainlist, MMIO_CREATELIST);

  MMCKINFO avihinfo;
  avihinfo.cksize = 0;
  avihinfo.ckid = mmioFOURCC('a', 'v', 'i', 'h');
  mmioCreateChunk(file, &avihinfo, 0);

  AVIMAINHEADER avihdr = { 0, 0, (1000000 * frratescale) / frrate, (frrate / frratescale * 8), 0, AVIF_HASINDEX, numframes, 0, 2, 800, width, height, 0, 0, 0, 0 };
  mmioWrite(file, (LPCSTR)&avihdr + 8, sizeof(avihdr) - 8);

  mmioAscend(file, &avihinfo, 0);

  MMCKINFO videostrl;
  videostrl.cksize = 0;
  videostrl.fccType = mmioFOURCC('s', 't', 'r', 'l');
  mmioCreateChunk(file, &videostrl, MMIO_CREATELIST);

  MMCKINFO videostrh;
  videostrh.cksize = 0;
  videostrh.ckid = mmioFOURCC('s', 't', 'r', 'h');
  mmioCreateChunk(file, &videostrh, 0);

  AVISTREAMHEADER videohdr = { 0, 0, streamtypeVIDEO, containerFcc, 0, 0, 0, 0, frratescale, frrate,
                               0, numframes, (width * height * videobpp) / 8, 0, 0 };
  mmioWrite(file, (LPCSTR)&videohdr + 8, sizeof(videohdr) - 8);

  mmioAscend(file, &videostrh, 0);

  MMCKINFO videostrf;
  videostrf.cksize = 0;
  videostrf.ckid = mmioFOURCC('s', 't', 'r', 'f');
  mmioCreateChunk(file, &videostrf, 0);

  // containerFcc is set in the biCompression field so that our frameserver codec gets invoked to read
  // the data.
  // realFcc is the actual fourcc of the bitmap such as BI_RGB, YUY2 or v210. We pass that in to the codec
  // via the biClrImportant field which is otherwise unused in this flow. The decoder, dfsc.cpp, will make
  // use of it.
  BITMAPINFOHEADER videofmt = {
    sizeof(BITMAPINFOHEADER), width, height, 1, videobpp, containerFcc, (width * height * videobpp) / 8,
    0, 0, 0, realFcc
  };
  mmioWrite(file, (LPCSTR)&videofmt, sizeof(videofmt));

  mmioAscend(file, &videostrf, 0);

  mmioAscend(file, &videostrl, 0);

  MMCKINFO audiostrl;
  audiostrl.cksize = 0;
  audiostrl.fccType = mmioFOURCC('s', 't', 'r', 'l');
  mmioCreateChunk(file, &audiostrl, MMIO_CREATELIST);

  MMCKINFO audiostrh;
  audiostrh.cksize = 0;
  audiostrh.ckid = mmioFOURCC('s', 't', 'r', 'h');
  mmioCreateChunk(file, &audiostrh, 0);

  AVISTREAMHEADER audiohdr = { 0, 0, streamtypeAUDIO, 0, 0, 0, 0, 0, 1, wfx->nAvgBytesPerSec,
                               0, numaudioblocks * 8, wfx->nAvgBytesPerSec, 0, 1 };
  mmioWrite(file, (LPCSTR)&audiohdr + 8, sizeof(audiohdr) - 8);

  mmioAscend(file, &audiostrh, 0);

  MMCKINFO audiostrf;
  audiostrf.cksize = 0;
  audiostrf.ckid = mmioFOURCC('s', 't', 'r', 'f');
  mmioCreateChunk(file, &audiostrf, 0);

  mmioWrite(file, (LPCSTR)wfx, sizeof(WAVEFORMATEX));

  mmioAscend(file, &audiostrf, 0);

  mmioAscend(file, &audiostrl, 0);

  mmioAscend(file, &mainlist, 0);

  MMCKINFO movilist;
  movilist.cksize = 0;
  movilist.fccType = mmioFOURCC('m', 'o', 'v', 'i');
  mmioCreateChunk(file, &movilist, MMIO_CREATELIST);

  unsigned int i;
  for (i = 0; i < numaudioblocks; i++) {
    MMCKINFO audioframe;
    audioframe.cksize = 0;
    audioframe.ckid = mmioFOURCC('0', '1', 'w', 'b');
    mmioCreateChunk(file, &audioframe, 0);

    DWORD audiodata = i;
    mmioWrite(file, (LPCSTR)&audiodata, sizeof(audiodata));
    mmioWrite(file, (LPCSTR)&stream, sizeof(stream));

    mmioAscend(file, &audioframe, 0);
  }

  for (i = 0; i < numframes; i++) {
    MMCKINFO videoframe;
    videoframe.cksize = 0;
    videoframe.ckid = mmioFOURCC('0', '0', 'd', 'c');
    mmioCreateChunk(file, &videoframe, 0);

    DWORD videodata = i;
    mmioWrite(file, (LPCSTR)&videodata, sizeof(videodata));
    mmioWrite(file, (LPCSTR)&stream, sizeof(stream));

    mmioAscend(file, &videoframe, 0);
  }

  mmioAscend(file, &movilist, 0);

  MMCKINFO indexchunk;
  indexchunk.cksize = 0;
  indexchunk.ckid = mmioFOURCC('i', 'd', 'x', '1');
  mmioCreateChunk(file, &indexchunk, 0);

  for (i = 0; i < numaudioblocks; i++) {
    AVIINDEXENTRY indexentry;
    indexentry.ckid = mmioFOURCC('0', '1', 'w', 'b');
    indexentry.dwFlags = AVIIF_KEYFRAME;
    indexentry.dwChunkOffset = 4 + i * (8 + 4 + 4);
    indexentry.dwChunkLength = 8;
    mmioWrite(file, (LPCSTR)&indexentry, sizeof(indexentry));
  }

  for (i = 0; i < numframes; i++) {
    AVIINDEXENTRY indexentry;
    indexentry.ckid = mmioFOURCC('0', '0', 'd', 'c');
    indexentry.dwFlags = AVIIF_KEYFRAME;
    indexentry.dwChunkOffset = 4 + (i + numaudioblocks) * (8 + 4 + 4);
    indexentry.dwChunkLength = 8;
    mmioWrite(file, (LPCSTR)&indexentry, sizeof(indexentry));
  }

  mmioAscend(file, &indexchunk, 0);

  mmioAscend(file, &riffinfo, 0);
  mmioClose(file, 0);
  return true;
}
