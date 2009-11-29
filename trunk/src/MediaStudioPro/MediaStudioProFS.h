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

#ifndef MEDIASTUDIOPRO_MEDIASTUDIOPROFS_H
#define MEDIASTUDIOPRO_MEDIASTUDIOPROFS_H

#include "uvideo.h"
#include "fscommon.h"

class CMediaStudioProFS : public FrameServerImpl {
public:
  virtual ~CMediaStudioProFS() { }

  int Progress(HWND hParent, PSAVEDATA pSData, UVPFN_SVCB pfnCallBack);

  int NotifyStartSave();
  int NotifyStartSaveVideo();
  int NotifyStartSaveAudio();
  int NotifyEndSave();
  int NotifyEndSaveVideo();
  int NotifyEndSaveAudio();
  int CallbackGetVideoFrame(DWORD dwFrame, PDIB* ppDib);
  int NotifyEndOfVideoFrame();
  int CallbackGetAudioSamples(DWORD dwStart, DWORD dwSamples, PBYTE* ppData);
  int NotifyEndOfAudioSamples(DWORD dwSamples);

  void OnVideoRequest();
  void OnAudioRequest();
  bool OnAudioRequestOneSecond(DWORD second, LPBYTE* data, DWORD* datalen);

  PSAVEDATA m_pSData;
  UVPFN_SVCB m_pfnCallBack;
  BOOL m_bSaveAudio;
  BOOL m_bSaveVideo;
  BIH m_SrcBih;
  WFX m_SrcWfx;
};

#endif  // MEDIASTUDIOPRO_MEDIASTUDIOPROFS_H
