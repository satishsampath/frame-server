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

#ifndef VEGASV2_VEGASFSRENDER_H
#define VEGASV2_VEGASFSRENDER_H

#include <string>
#include <vector>
#include "fscommon.h"
#include "SfRenderFile.h"

class SfReadStreams;

class VegasFSRender : public ISfRenderFile, public FrameServerImpl {
public:
  VegasFSRender(ISfRenderFileClass* pRenderFileClass,
                PCSFTEMPLATEx pTemplate);
  ~VegasFSRender();

  STDMETHODIMP Init();

  // FrameServerImpl
  void OnVideoRequest();
  void OnAudioRequest();
  bool OnAudioRequestOneSecond(DWORD second, LPBYTE* data, DWORD* datalen);

  // IUnknown
  STDMETHOD(QueryInterface) (REFIID riid,
      LPVOID * ppvObj);
  STDMETHOD_(ULONG, AddRef) ();
  STDMETHOD_(ULONG, Release) ();

  // ISfRenderFile
  STDMETHOD(GetRenderFileClass) (ISfRenderFileClass * *ppClass);
  STDMETHOD(CloseFile) ();
  STDMETHOD(IsFileOpen) ();
  STDMETHOD(OpenFile) (DWORD fdwCreateFlags, DWORD fdwShareFlags);
  STDMETHOD(SetFile) (LPCWSTR pszFilename);
  STDMETHOD(GetFileName) (LPWSTR pszFilename, UINT cchFileName);
  STDMETHOD(GetStreamCount) (LONG * pcStreams);
  STDMETHOD(GetStreamFormatInfo) (SFFILESTREAMFORMATINFO aStreams[],
      LONG ixFirstStream, LONG cStreams);
  STDMETHOD(SetStreamFormatInfo) (SFFILESTREAMFORMATINFO aStreamsToUse[], LONG c);
  STDMETHOD(SetStreamReader) (ISfReadStream * pSfReadStream, LONG ixStream);
  STDMETHOD(SetMetaReader) (ISfReadMeta * pSfReadMeta);
  STDMETHOD(GetTemplate) (PSFTEMPLATEx * pptpl, DWORD dwReserved);
  STDMETHOD(EstimateFileSize) (LONGLONG * pcbFileSize);
  STDMETHOD(Render) (ISfProgress * pProgress);
  STDMETHOD(UpdateMetaData) (DWORD dwSampleRate, SFFIO_TEXT_ENCODE_TYPE eEncode);

private:
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

  DWORD m_dwRef;
  ISfRenderFileClass* m_pRenderFileClass;

  std::wstring m_sFilename;
  SfReadStreams* m_pReadStreams;
  ISfReadMeta* m_pReadMeta;
  PCSFTEMPLATEx m_pTemplate;

  bool m_isFileOpen;
  VegasFSMetaData FfpHeader;
  std::vector<unsigned char> m_bVideoFrame;
  std::vector<unsigned char> m_bAudioFrame;
};

#endif  // VEGASV2_VEGASFSRENDER_H
