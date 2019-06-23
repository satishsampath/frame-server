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
#include <math.h>
#include "fscommon.h"
#include "DmFilter.h"

HINSTANCE ghInst = NULL;
FilterTools* filterTools = NULL;

#define APPNAME "DebugMode FrameServer"

int WaxFSExport(DmFxnHostRenderGetVideo renderGetVideo,
                DmFxnHostRenderGetAudio renderGetAudio,
                void* opData,
                char* pcFileName,
                BYTE* pbVideoOptions,
                int nVideoOptionsSize,
                BYTE* pbAudioOptions,
                int nAudioOptionsSize,
                ImageFilterParams* psImage,
                AudioFilterParams* psAudio,
                VideoFilterParams* psVideo);

class CWaxFS : public FrameServerImpl {
public:
  DmFxnHostRenderGetVideo getVideo;
  DmFxnHostRenderGetAudio getAudio;
  void* opaqueData;
  AudioFilterParams* audioParams;
  VideoFilterParams* videoParams;
  ImageSample* imageBuffer;
  AudioSample* audioBuffer;

public:
  void OnVideoRequest();
  void OnAudioRequest();
  bool OnAudioRequestOneSecond(DWORD second, LPBYTE* data, DWORD* datalen);
};

CWaxFS* waxfs = NULL;

FilterCaps videoCaps = {
  sizeof(FilterCaps),
  APPNAME,
  "AVI",
  APPNAME " (*.avi)|*.avi",
  0,
  DM_ELEMENT_AUDIO | DM_ELEMENT_VIDEO,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  WaxFSExport,
  NULL, NULL, NULL, NULL, NULL
};

FilterCaps* gasFilterCaps[] = {
  &videoCaps
};

BOOL APIENTRY DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID pReserved) {
  switch (dwReason) {
  case DLL_PROCESS_ATTACH:
    ghInst = hinst;
    break;
  }
  return TRUE;
}

extern "C" __declspec(dllexport) int GetFilterCapsEx(FilterCaps*** ppsFilterCaps) {
  *ppsFilterCaps = gasFilterCaps;
  return sizeof(gasFilterCaps) / sizeof(gasFilterCaps[0]);
}

extern "C" __declspec(dllexport) void SetFilterTools(FilterTools* psTools) {
  filterTools = psTools;
}

int WaxFSExport(DmFxnHostRenderGetVideo renderGetVideo,
    DmFxnHostRenderGetAudio renderGetAudio,
    void* opData,
    char* pcFileName,
    BYTE* pbVideoOptions,
    int nVideoOptionsSize,
    BYTE* pbAudioOptions,
    int nAudioOptionsSize,
    ImageFilterParams* psImage,
    AudioFilterParams* psAudio,
    VideoFilterParams* psVideo) {
  if (!psVideo) {
    MessageBoxA(NULL, "Frameserver cannot work with just audio, please include video in the render",
        APPNAME, MB_OK);
    return 1;
  }

  CWaxFS fs;
  fs.getVideo = renderGetVideo;
  fs.getAudio = renderGetAudio;
  fs.opaqueData = opData;
  fs.audioParams = psAudio;
  fs.videoParams = psVideo;
  fs.imageBuffer = (psVideo) ? new ImageSample[psVideo->dwWidth * psVideo->dwHeight] : NULL;
  fs.audioBuffer = (psAudio) ? new AudioSample[psAudio->dwSamplingRate + 100] : NULL;

  TCHAR tcFileName[_MAX_PATH];
  MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pcFileName, -1, tcFileName, _MAX_PATH);
  fs.Init((psAudio != NULL), (psAudio ? psAudio->dwSamplingRate : 0), 16, 2, (psVideo ? psVideo->dwNumFrames : 0),
      (psVideo ? psVideo->dwFrameRate / 100.0 : 1), (psVideo ? psVideo->dwWidth : 0),
      (psVideo ? psVideo->dwHeight : 0), filterTools->hMainWnd, tcFileName);
  fs.Run();

  delete[] fs.imageBuffer;
  delete[] fs.audioBuffer;

  return 0;
}

void CWaxFS::OnVideoRequest() {
  if (!videoParams)
    return;
  if (getVideo(opaqueData, vars->videoFrameIndex, imageBuffer) != 0)
    memset(imageBuffer, 0, sizeof(ImageSample) * videoParams->dwWidth * videoParams->dwHeight);
  // since wax gives top-down images and we need bottom up ones, flip it
  ConvertVideoFrame(imageBuffer + (videoParams->dwHeight - 1) * videoParams->dwWidth,
      -int(sizeof(ImageSample) * videoParams->dwWidth), vars);
}

void CWaxFS::OnAudioRequest() {
  vars->audioBytesRead = 0;
  if (!audioParams)
    return;

  int ccToRead = audioSamplingRate / 100;
  ULONG curpos = vars->audioFrameIndex * ccToRead;

  double stframe = ((double)vars->audioFrameIndex / 100) * fps;
  double endframe = ((double)(vars->audioFrameIndex + 1) / 100) * fps;
  int stframesample = (int)(audioSamplingRate * floor(stframe) / fps);

  int blockalign = sizeof(AudioSample);
  int offintobuf = curpos - stframesample;
  int audioSamplesRead = 0;
  int audioFrameCount = (int)(ceil(endframe) - floor(stframe));

  if (getAudio(opaqueData, (int)floor(stframe), audioFrameCount, audioBuffer, &audioSamplesRead) == 0)
    ccToRead = min(ccToRead, audioSamplesRead);
  else
    ccToRead = 0;       // error

  memcpy(((LPBYTE)vars) + vars->audiooffset, audioBuffer + offintobuf, ccToRead * blockalign);
  vars->audioBytesRead = ccToRead * blockalign;
}

bool CWaxFS::OnAudioRequestOneSecond(DWORD second, LPBYTE* data, DWORD* datalen) {
  if (!audioParams)
    return false;

  int ccToRead = audioSamplingRate;
  ULONG curpos = second * ccToRead;

  double stframe = ((double)second) * fps;
  double endframe = ((double)(second + 1)) * fps;
  int stframesample = (int)(audioSamplingRate * floor(stframe) / fps);

  int blockalign = sizeof(AudioSample);
  int offintobuf = curpos - stframesample;

  long audioFrameCount = (int)(ceil(endframe) - floor(stframe));

  *datalen = (int)(audioSamplingRate / fps * (audioFrameCount + 1) * 4);
  if (!*data)
    *data = (unsigned char*)calloc(1, *datalen);

  int audioSamplesRead = 0;
  if (getAudio(opaqueData, (int)floor(stframe), audioFrameCount, audioBuffer, &audioSamplesRead) == 0)
    ccToRead = min(ccToRead, audioSamplesRead);
  else
    ccToRead = 0;       // error

  memcpy(*data, audioBuffer + offintobuf, ccToRead * blockalign);
  *datalen = ccToRead * blockalign;
  return true;
}
