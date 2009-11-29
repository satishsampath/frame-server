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
#include "SfFioGuids.h"
#include "SfHelpers.h"
#include "ifileio.h"
#include "ifilemng.h"
#include "VegasFS.h"
#include "VegasFSRender.h"

STDMETHODIMP VegasFSRender::QueryInterface(REFIID riid, LPVOID* ppvObj) {
  if (IID_ISfRenderFile == riid || IID_IUnknown == riid) {
    AddRef();
    *ppvObj = static_cast<ISfRenderFile*>(this);
    return NOERROR;
  }

  return E_NOINTERFACE;
};

STDMETHODIMP_(ULONG) VegasFSRender::AddRef() {
  m_dwRef++;
  return m_dwRef;
};

STDMETHODIMP_(ULONG) VegasFSRender::Release() {
  m_dwRef--;
  if (m_dwRef == 0) {
    delete this;
    return 0;
  }
  return m_dwRef;
};

STDMETHODIMP VegasFSRender::GetFileClass(ISfRenderFileClass** fileClass) {
  return m_pClassBackPointer->QueryInterface(IID_ISfRenderFileClass, (void**)fileClass);
};

STDMETHODIMP VegasFSRender::GetRenderFileClass(ISfRenderFileClass** fileClass) {
  if (NULL == m_pClassBackPointer)
    return E_NOINTERFACE;

  *fileClass = m_pClassBackPointer;
  m_pClassBackPointer->AddRef();

  return NOERROR;
}

VegasFSRender::VegasFSRender(VegasFS* pClass, PSFTEMPLATEx pTemplate)
  : m_pClassBackPointer(pClass), m_dwRef(0) {
  m_szFileName[0] = '\0';
  m_pTemplate = pTemplate;
  m_pIVideoStream = NULL;
  m_pIAudioStream = NULL;
  m_pISfReadMeta = NULL;
  m_fOpen = FALSE;
}

STDMETHODIMP VegasFSRender::initObj(void) {
  return S_OK;
}

VegasFSRender::~VegasFSRender() {
  CloseFile();
  freeStreams();
  if (m_pTemplate) {
    m_pClassBackPointer->freeTemplate(m_pTemplate);
  }
  return;
}

#define FSALIGN(x, y) (((x) + ((y) - 1)) & ~((y) - 1))

long FSDibScanBytes(BITMAPINFOHEADER* pbi, LONG lPitch = 0) {
  return (0 == lPitch) ? (FSALIGN((pbi)->biBitCount * (pbi)->biWidth, 32) / 8) : lPitch;
}

long FSDibImageBytes(BITMAPINFOHEADER* pbi) {
  return FSDibScanBytes(pbi, 0) * (pbi)->biHeight;
}

STDMETHODIMP VegasFSRender::Render(ISfProgress* progress) {
  HRESULT hr = S_OK;

  if (NULL != m_pIVideoStream || NULL != m_pIAudioStream)
    hr = writeData(progress);

  return hr;
}

STDMETHODIMP VegasFSRender::UpdateMetaData(DWORD sampleRate) {
  return S_OK;
}

STDMETHODIMP VegasFSRender::SetStreamReader(ISfReadStream* readStream, LONG streamIndex) {
  HRESULT hr = E_OUTOFMEMORY;

  if (readStream) {
    DWORD dwStreamType;
    readStream->GetStreamType(&dwStreamType, NULL, NULL);
    if (SFFIO_STREAM_TYPE_AUDIO == dwStreamType) {
      if (m_pIAudioStream)
        m_pIAudioStream->Release();
      m_pIAudioStream = NULL;
      hr = readStream->QueryInterface(IID_ISfReadAudioStream,
          (void**)&m_pIAudioStream);
    } else if (SFFIO_STREAM_TYPE_VIDEO == dwStreamType) {
      if (m_pIVideoStream)
        m_pIVideoStream->Release();
      m_pIVideoStream = NULL;
      hr = readStream->QueryInterface(IID_ISfReadVideoStream,
          (void**)&m_pIVideoStream);
    }
  }
  return hr;
}

STDMETHODIMP VegasFSRender::SetMetaReader(ISfReadMeta* readMeta) {
  if (m_pISfReadMeta)
    m_pISfReadMeta->Release();
  m_pISfReadMeta = readMeta;
  if (m_pISfReadMeta)
    m_pISfReadMeta->AddRef();
  return S_OK;
}

void VegasFSRender::freeStreams() {
  if (m_pIVideoStream)
    m_pIVideoStream->Release();
  if (m_pIAudioStream)
    m_pIAudioStream->Release();
  if (m_pISfReadMeta)
    m_pISfReadMeta->Release();
  return;
}

STDMETHODIMP VegasFSRender::CloseFile() {
  m_fOpen = FALSE;
  return S_OK;
}

STDMETHODIMP VegasFSRender::IsFileOpen() {
  return (m_fOpen ? S_OK : S_FALSE);
}

STDMETHODIMP VegasFSRender::OpenFile(DWORD createFlags, DWORD shareFlags) {
  m_fOpen = TRUE;
  return NOERROR;
}

STDMETHODIMP VegasFSRender::SetFile(LPCWSTR filename) {
  if (filename)
    wcsncpy(m_szFileName, filename, MAX_PATH);
  return S_OK;
}

STDMETHODIMP VegasFSRender::GetFileName(LPWSTR filename, UINT nameCount) {
  if (filename)
    wcsncpy(filename, m_szFileName, nameCount);
  return S_OK;
}

STDMETHODIMP VegasFSRender::GetStreamCount(LONG* count) {
  LONG cStreams = 0;

  if (m_pTemplate) {
    if (m_pTemplate->pAudioTemplate)
      cStreams += m_pTemplate->pAudioTemplate->cNumAStreams;
    if (m_pTemplate->pVideoTemplate)
      cStreams += m_pTemplate->pVideoTemplate->cNumVStreams;
  }
  if (count)
    *count = cStreams;

  return S_OK;
}

STDMETHODIMP VegasFSRender::GetTemplateInfo(SFTEMPLATEx** info) {
  // return a copy.
  *info = m_pClassBackPointer->allocTemplate();
  return m_pClassBackPointer->copyTemplate(*info, m_pTemplate);
}

STDMETHODIMP VegasFSRender::FreeTemplateInfo(SFTEMPLATEx* info) {
  m_pClassBackPointer->freeTemplate(info);
  return S_OK;
}

HRESULT VegasFSRender::writeData(ISfProgress* progress) {
  BOOL useAudio = (m_pIAudioStream &&
                   m_pTemplate->pAudioTemplate &&
                   m_pTemplate->pAudioTemplate->cNumAStreams);
  BOOL useVideo = (m_pIVideoStream &&
                   m_pTemplate->pVideoTemplate &&
                   m_pTemplate->pVideoTemplate->cNumVStreams);

  if (!useVideo)
    return SF_E_NOVIDEO;

  HWND activewnd = GetForegroundWindow();
  DWORD threadid = GetWindowThreadProcessId(activewnd, NULL);
  AttachThreadInput(GetCurrentThreadId(), threadid, TRUE);

  ZeroMemory(&FfpHeader, NUMBYTES(FfpHeader));

  LPTSTR pszFileName;
#ifdef UNICODE
  pszFileName = m_szFileName;
#else
  TCHAR szFileName[MAX_PATH];
  SfMBFromWC(szFileName, m_szFileName, MAX_PATH);
  pszFileName = szFileName;
#endif
  HRESULT hr = S_OK;

  hr = m_pIVideoStream->GetFrameCount(&FfpHeader.Video.cfTotal);
  hr = m_pIVideoStream->GetFrameRate(&FfpHeader.Video.dFPS);
  FfpHeader.Video.ntLength = SfVideo_FramesToTime(
      FfpHeader.Video.cfTotal, FfpHeader.Video.dFPS);
  FfpHeader.Video.bih = *m_pTemplate->pVideoTemplate->pbihCodec;
  FfpHeader.Video.cbFrameSize = FSDibImageBytes(&FfpHeader.Video.bih);
  if (useAudio) {
    hr = m_pIAudioStream->GetSampleCount(&FfpHeader.Audio.ccTotal);
    FfpHeader.Audio.wfx = m_pTemplate->pAudioTemplate->wfx;
    hr = m_pIAudioStream->GetStreamLength(&FfpHeader.Audio.ntLength);
    FfpHeader.Audio.ntLength = SfAudio_CellsToTime(FfpHeader.Audio.ccTotal,
        FfpHeader.Audio.wfx.nSamplesPerSec);
  }

  Init(!!useAudio, FfpHeader.Audio.wfx.nSamplesPerSec,
      FfpHeader.Audio.wfx.wBitsPerSample, FfpHeader.Audio.wfx.nChannels,
      (DWORD)FfpHeader.Video.cfTotal, FfpHeader.Video.dFPS,
      FfpHeader.Video.bih.biWidth, FfpHeader.Video.bih.biHeight, activewnd, pszFileName);
  hr = S_OK;
  if (!Run())
    hr = SF_E_CANCEL;

  EnableWindow(activewnd, TRUE);
  SetForegroundWindow(activewnd);
  AttachThreadInput(GetCurrentThreadId(), threadid, FALSE);
  return hr;
}

bool VegasFSRender::OnAudioRequestOneSecond(DWORD second, LPBYTE* data, DWORD* datalen) {
  LONGLONG curpos, nextpos;

  curpos = (second * (LONGLONG)FfpHeader.Audio.wfx.nSamplesPerSec);
  nextpos = ((second + 1) * (LONGLONG)FfpHeader.Audio.wfx.nSamplesPerSec);
  ULONG ccToRead = (ULONG)(nextpos - curpos), ccActuallyRead = 0;
  *datalen = ccToRead * FfpHeader.Audio.wfx.nBlockAlign;
  if (*data == NULL)
    *data = (unsigned char*)malloc(*datalen);

  HRESULT hr = m_pIAudioStream->Read(curpos, *data, ccToRead, &ccActuallyRead);
  memset(*data + ccActuallyRead * FfpHeader.Audio.wfx.nBlockAlign, 0,
      (ccToRead - ccActuallyRead) * FfpHeader.Audio.wfx.nBlockAlign);
  return true;
}

void VegasFSRender::OnVideoRequest() {
  LPCVOID pFrame;
  HRESULT hr = m_pIVideoStream->GetFrame(vars->videoFrameIndex, &pFrame,
      NULL, NULL, NULL, NULL, 0, NULL);

  if (hr == S_OK) {
    int rowBytes = FfpHeader.Video.cbFrameSize / vars->encBi.biHeight;
    ConvertVideoFrame(pFrame, rowBytes, vars);
    m_pIVideoStream->ReleaseFrame(pFrame);
  } else
    memset(((LPBYTE)vars) + vars->videooffset, 0, vars->videoBytesRead);
}

void VegasFSRender::OnAudioRequest() {
  LONGLONG curpos, nextpos;

  curpos = (vars->audioFrameIndex * (LONGLONG)FfpHeader.Audio.wfx.nSamplesPerSec) / 100;
  nextpos = ((vars->audioFrameIndex + 1) * (LONGLONG)FfpHeader.Audio.wfx.nSamplesPerSec) / 100;
  ULONG ccToRead = (ULONG)(nextpos - curpos);
  ULONG ccActuallyRead = 0;
  HRESULT hr = m_pIAudioStream->Read(curpos,
      ((LPBYTE)vars) + vars->audiooffset, ccToRead, &ccActuallyRead);
  vars->audioBytesRead = ccActuallyRead * FfpHeader.Audio.wfx.nBlockAlign;
}
