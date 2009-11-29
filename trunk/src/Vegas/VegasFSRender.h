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

#ifndef VEGAS_VEGASFSRENDER_H
#define VEGAS_VEGASFSRENDER_H

#include "fscommon.h"

class VegasFS;

class VegasFSRender : public ISfRenderFile, public FrameServerImpl {
public:
  VegasFSRender(VegasFS* pClass, PSFTEMPLATEx pTemplate);
  virtual ~VegasFSRender();

  STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppvObj);
  STDMETHODIMP_(ULONG) AddRef();
  STDMETHODIMP_(ULONG) Release();

  STDMETHODIMP GetFileClass(ISfRenderFileClass** pC);
  STDMETHODIMP CloseFile();
  STDMETHODIMP IsFileOpen();
  STDMETHODIMP OpenFile(DWORD fdwCreateFlags, DWORD fdwShareFlags);
  STDMETHODIMP SetFile(LPCWSTR pszFilename);
  STDMETHODIMP GetFileName(LPWSTR pszFilename, UINT cchFileName);
  STDMETHODIMP GetRenderFileClass(ISfRenderFileClass** ppBack);
  STDMETHODIMP GetStreamCount(LONG* pcStreams);
  STDMETHODIMP SetStreamReader(ISfReadStream* pSfReadStream, LONG ixStream);
  STDMETHODIMP SetMetaReader(ISfReadMeta* pSfReadMeta);
  STDMETHODIMP GetTemplateInfo(PSFTEMPLATEx* ppDefaultTemplate);
  STDMETHODIMP FreeTemplateInfo(PSFTEMPLATEx pTemplate);
  STDMETHODIMP Render(ISfProgress* pProgress);
  STDMETHODIMP UpdateMetaData(DWORD dwSampleRate);

  STDMETHODIMP GetStreamFormatInfo(SFFILESTREAMFORMATINFO [], LONG, LONG) { return E_NOTIMPL; }
  STDMETHODIMP SetStreamFormatInfo(SFFILESTREAMFORMATINFO [], LONG) { return E_NOTIMPL; }
  STDMETHODIMP GetCompFrameInfo(ISfRenderCompInfo**) { return E_NOTIMPL; }
  STDMETHODIMP EstimateFileSize(LONGLONG*) { return E_NOTIMPL; }

  STDMETHODIMP initObj(void);

  void OnVideoRequest();
  void OnAudioRequest();
  bool OnAudioRequestOneSecond(DWORD second, LPBYTE* data, DWORD* datalen);

private:
  void freeStreams(void);
  HRESULT writeData(ISfProgress* pIProgress);

  typedef struct tagMetaData {
    struct tagAudio {
      WAVEFORMATEX wfx;
      SFNANOTIME ntLength;
      LONGLONG ccTotal;
    } Audio;

    struct tagVideo {
      BITMAPINFOHEADER bih;
      SFNANOTIME ntLength;
      LONGLONG cfTotal;
      double dFPS;
      DWORD cbFrameSize;
    } Video;
  } VegasFSMetaData;

  VegasFS* m_pClassBackPointer;
  WCHAR m_szFileName[MAX_PATH];
  BOOL m_fOpen;
  DWORD m_dwRef;
  ISfReadVideoStream* m_pIVideoStream;
  ISfReadAudioStream* m_pIAudioStream;
  ISfReadMeta* m_pISfReadMeta;
  ISfProgress* m_pIProgress;

  PSFTEMPLATEx m_pTemplate;

  VegasFSMetaData FfpHeader;
  ISfReadAudioStream* audioStream;
};

#endif  // VEGAS_VEGASFSRENDER_H
