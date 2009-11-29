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
#include "MediaStudioProFS.h"
#include "blankavi.h"
#include "../dfsc/dfsc.h"
#include <stdio.h>

#define MID MID_SAVEFILE    // for error handling

extern DWORD WINAPI dib_GetTotalSize(PBIH pbih);
extern void WINAPI dib_Prepare(PBIH pbih, int w, int h, int nBits);
extern void WINAPI wfx_Prepare(PWFX pWfx, WORD wChannels, WORD wBitsPerSample, DWORD dwSamplesPerSec);

void CMediaStudioProFS::OnVideoRequest() {
  PDIB pDib = NULL;

  CallbackGetVideoFrame(vars->videoFrameIndex, &pDib);
  // NotifyEndOfVideoFrame();
  vars->videoBytesRead = 0;
  if (pDib)
    ConvertVideoFrame(uv_BihToData(pDib), uv_BihToSize(pDib) / pDib->biHeight, vars);
}

void CMediaStudioProFS::OnAudioRequest() {
  __int64 curpos, nextpos;

  curpos = ((__int64)vars->audioFrameIndex * m_SrcWfx.nSamplesPerSec) / 100;
  nextpos = ((__int64)(vars->audioFrameIndex + 1) * m_SrcWfx.nSamplesPerSec) / 100;
  ULONG ccToRead = (ULONG)(nextpos - curpos);

  PBYTE pData = NULL;
  CallbackGetAudioSamples((ULONG)curpos, ccToRead, &pData);
  // NotifyEndOfAudioSamples(ccToRead);
  memcpy(((LPBYTE)vars) + vars->audiooffset, pData, ccToRead * m_SrcWfx.nBlockAlign);
  vars->audioBytesRead = ccToRead * m_SrcWfx.nBlockAlign;
}

bool CMediaStudioProFS::OnAudioRequestOneSecond(DWORD second, LPBYTE* data, DWORD* datalen) {
  ULONG curpos, nextpos;

  curpos = (second * m_SrcWfx.nSamplesPerSec);
  nextpos = ((second + 1) * m_SrcWfx.nSamplesPerSec);
  ULONG ccToRead = nextpos - curpos;
  *datalen = ccToRead * m_SrcWfx.nBlockAlign;
  if (!*data)
    *data = (unsigned char*)malloc(*datalen);

  PBYTE pData = NULL;
  CallbackGetAudioSamples(curpos, ccToRead, &pData);
  // NotifyEndOfAudioSamples(ccToRead);
  memcpy(*data, pData, *datalen);
  return true;
}

int CMediaStudioProFS::Progress(HWND hParent, PSAVEDATA pSData, UVPFN_SVCB pfnCallBack) {
  m_pSData = pSData;
  m_pfnCallBack = pfnCallBack;
  m_bSaveAudio = (m_pSData->wSaveDataTrack & UVDT_AUDIO) && m_pSData->wAudio;
  m_bSaveVideo = (m_pSData->wSaveDataTrack & UVDT_VIDEO) && m_pSData->wVideo;
  if (!m_bSaveVideo) {
    MessageBox(hParent, "No video to frameserve!", "DebugMode FrameServer", MB_OK | MB_ICONSTOP);
    return 0;
  }

  if (NotifyStartSave() <= 0)
    return 0;
  if (NotifyStartSaveVideo() <= 0)
    return 0;
  if (m_bSaveAudio)
    if (NotifyStartSaveAudio() <= 0)
      return 0;
  int nRet = 1;

  Init(!!m_bSaveAudio, m_pSData->dwSaveSamplesPerSec, m_pSData->wSaveBitsPerSample, m_pSData->wSaveChannels,
      m_pSData->dwSaveFrames, (double)m_pSData->dwSaveFrameRate / FPS_PREC, m_SrcBih.biWidth, m_SrcBih.biHeight,
      hParent, m_pSData->szSaveName);
  nRet = (Run() == true) ? 1 : 0;

  if (m_bSaveAudio)
    NotifyEndSaveAudio();
  NotifyEndSaveVideo();
  NotifyEndSave();
  return nRet;
}

int CMediaStudioProFS::NotifyStartSave() {
  // callback AP to initialize saving
  dib_Prepare(&m_SrcBih, m_pSData->wSaveWidth, m_pSData->wSaveHeight, m_pSData->wSaveBitCount);
  m_pSData->dwAudioCalls = m_bSaveAudio ? m_pSData->dwSaveFrames : 0;
  m_pSData->dwVideoCalls = m_pSData->dwSaveFrames;
  return m_pfnCallBack(m_pSData, UVDATA_START, 0, 0, &m_SrcBih, NULL);
}

int CMediaStudioProFS::NotifyStartSaveVideo() {
  // callback AP to prepare video data
  return m_pfnCallBack(m_pSData, UVDATA_VIDEO_BEGIN, 0, 0, &m_SrcBih, NULL);
}

int CMediaStudioProFS::NotifyStartSaveAudio() {
  // callback AP to prepare audio data
  wfx_Prepare(&m_SrcWfx, m_pSData->wSaveChannels, m_pSData->wSaveBitsPerSample, m_pSData->dwSaveSamplesPerSec);
  return m_pfnCallBack(m_pSData, UVDATA_AUDIO_BEGIN, 0, 0, &m_SrcWfx, NULL);
}

int CMediaStudioProFS::NotifyEndSave() {
  return m_pfnCallBack(m_pSData, UVDATA_END, 0, 0, NULL, NULL);
}

int CMediaStudioProFS::NotifyEndSaveVideo() {
  return m_pfnCallBack(m_pSData, UVDATA_VIDEO_END, 0, 0, NULL, NULL);
}

int CMediaStudioProFS::NotifyEndSaveAudio() {
  return m_pfnCallBack(m_pSData, UVDATA_AUDIO_END, 0, 0, NULL, NULL);
}

int CMediaStudioProFS::CallbackGetVideoFrame(DWORD dwFrame, PDIB* ppDib) {
  return m_pfnCallBack(m_pSData, UVDATA_VIDEO, dwFrame, 1, &m_SrcBih, (PVOID*)ppDib);
}

int CMediaStudioProFS::NotifyEndOfVideoFrame() {
  return m_pfnCallBack(m_pSData, UVDATA_FRAME_END, 0, 0, NULL, NULL);
}

int CMediaStudioProFS::CallbackGetAudioSamples(DWORD dwStart, DWORD dwSamples, PBYTE* ppData) {
  return m_pfnCallBack(m_pSData, UVDATA_AUDIO, dwStart, dwSamples, &m_SrcWfx, (PVOID*)ppData);
}

int CMediaStudioProFS::NotifyEndOfAudioSamples(DWORD dwSamples) {
  return m_pfnCallBack(m_pSData, UVDATA_SAMPLE_END, 0, dwSamples, NULL, NULL);
}
