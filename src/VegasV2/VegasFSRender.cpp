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

#include <windows.h>
#include <TCHAR.H>
#include "SfBase.h"
#include "SfMem.h"
#include "SfReadStream.h"
#include "SfReadMeta.h"
#include "SfReadStreams.h"
#include "SfErrors.h"
#include "VegasFSRender.h"

VegasFSRender::VegasFSRender(ISfRenderFileClass* renderFileClass, PCSFTEMPLATEx pTemplate) :
  m_dwRef(0), m_pRenderFileClass(renderFileClass),
  m_pTemplate(pTemplate), m_pReadStreams(NULL),
  m_pReadMeta(NULL), m_isFileOpen(false) {
}

VegasFSRender::~VegasFSRender() {
  if (m_pReadStreams)
    delete m_pReadStreams;
  if (m_pReadMeta)
    m_pReadMeta->Release();
}

STDMETHODIMP VegasFSRender::Init() {
  m_pReadStreams = SfNew SfReadStreams;
  return S_OK;
}

STDMETHODIMP VegasFSRender::QueryInterface(REFIID riid, LPVOID* ppvObj) {
  if (__uuidof(ISfRenderFile) == riid || IID_IUnknown == riid) {
    AddRef();
    *ppvObj = static_cast<ISfRenderFile*>(this);
    return S_OK;
  }

  return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) VegasFSRender::AddRef() {
  return InterlockedIncrement((long*)&m_dwRef);
}

STDMETHODIMP_(ULONG) VegasFSRender::Release() {
  LONG cRef = InterlockedDecrement((long*)&m_dwRef);
  if(cRef == 0)
  {
    SfDelete this;
  }
  return cRef;
}

STDMETHODIMP VegasFSRender::GetRenderFileClass(ISfRenderFileClass** ppClass) {
  if (!ppClass)
    return E_INVALIDARG;
  *ppClass = m_pRenderFileClass;
  return S_OK;
}

STDMETHODIMP VegasFSRender::CloseFile() {
  m_isFileOpen = false;
  return S_OK;
}

STDMETHODIMP VegasFSRender::IsFileOpen() {
  return m_isFileOpen ? S_OK : S_FALSE;
}

STDMETHODIMP VegasFSRender::OpenFile(DWORD createFlags, DWORD shareFlags) {
  m_isFileOpen = true;
  return S_OK;
}

STDMETHODIMP VegasFSRender::SetFile(LPCWSTR pszFilename) {
  if (pszFilename) {
    m_sFilename = pszFilename;
  } else {
    m_sFilename = L"";
  }
  return S_OK;
}

STDMETHODIMP VegasFSRender::GetFileName(LPWSTR pszFilename, UINT cchFileName) {
  wcsncpy(pszFilename, m_sFilename.c_str(), cchFileName);
  return S_OK;
}

STDMETHODIMP VegasFSRender::GetStreamCount(LONG* pcStreams) {
  HRESULT hr = E_INVALIDARG;

  if (pcStreams) {
    if (m_pTemplate) {
      *pcStreams = m_pTemplate->Video.cStreams;
      *pcStreams += m_pTemplate->Audio.cStreams;

      hr = S_OK;
    } else {
      hr = E_FAIL;
    }
  }

  return hr;
}

STDMETHODIMP VegasFSRender::GetStreamFormatInfo(SFFILESTREAMFORMATINFO aStreams[],
    LONG ixFirstStream, LONG cStreams) {
  return E_NOTIMPL;
}

STDMETHODIMP VegasFSRender::SetStreamFormatInfo(SFFILESTREAMFORMATINFO aStreamsToUse[], LONG c) {
  return E_NOTIMPL;
}

STDMETHODIMP VegasFSRender::SetStreamReader(ISfReadStream* pSfReadStream, LONG ixStream) {
  if (!m_pReadStreams)
    return E_UNEXPECTED;
  return m_pReadStreams->Set(pSfReadStream, ixStream);
}

STDMETHODIMP VegasFSRender::SetMetaReader(ISfReadMeta* pReadMeta) {
  if (m_pReadMeta) {
    m_pReadMeta->Release();
    m_pReadMeta = NULL;
  }

  m_pReadMeta = pReadMeta;
  if (m_pReadMeta)
    m_pReadMeta->AddRef();

  return S_OK;
}

STDMETHODIMP VegasFSRender::GetTemplate(PSFTEMPLATEx* pptpl, DWORD dwReserved) {
  return E_NOTIMPL;
}

STDMETHODIMP VegasFSRender::EstimateFileSize(LONGLONG* pcbFileSize) {
  return E_NOTIMPL;
}

STDMETHODIMP VegasFSRender::UpdateMetaData(DWORD dwSampleRate, SFFIO_TEXT_ENCODE_TYPE eEncode) {
  return S_OK;
}

inline SFNANOTIME SfVideo_FramesToTime(LONGLONG cFrames, double dFPS) {
  double dd = (double)cFrames;

  dd /= dFPS;
  dd *= SFNANOS_PER_SECOND;
  dd += 0.5;
  return (SFNANOTIME)dd;
}

#define SFALIGN(x, y) (((x) + ((y) - 1)) & ~((y) - 1))

long SfDibScanBytes(PBITMAPINFOHEADER pbi, LONG lPitch = 0) {
  return (0 == lPitch) ? (SFALIGN((pbi)->biBitCount * (pbi)->biWidth, 32) / 8) : lPitch;
}

long SfDibImageBytes(PBITMAPINFOHEADER pbi) {
  return SfDibScanBytes(pbi, 0) * (pbi)->biHeight;
}

inline SFNANOTIME SfAudio_CellsToTime(LONGLONG cCells, DWORD dwRate) {
  if (dwRate <= 0)
    return 0;
  double dd = (double)cCells;
  dd /= dwRate;
  dd *= SFNANOS_PER_SECOND;
  dd += 0.5;
  return (SFNANOTIME)dd;
}

STDMETHODIMP VegasFSRender::Render(ISfProgress* pIProgress) {
  ISfReadAudioStream* pIAudioStream = NULL;
  ISfReadVideoRenderStream* pIVideoStream = NULL;

  m_pReadStreams->GetFirstAudioStream(&pIAudioStream, NULL);
  m_pReadStreams->GetFirstReadVideoRenderStream(&pIVideoStream, NULL);

  BOOL fUseAudio = pIAudioStream && (m_pTemplate->Audio.cStreams > 0);
  BOOL fUseVideo = pIVideoStream && (m_pTemplate->Video.cStreams > 0);
  if (!fUseVideo) {
    pIAudioStream->Release();
    pIVideoStream->Release();
    return SF_E_NOVIDEO;
  }

  HWND activewnd = GetForegroundWindow();
  DWORD threadid = GetWindowThreadProcessId(activewnd, NULL);
  AttachThreadInput(GetCurrentThreadId(), threadid, TRUE);

  ZeroMemory(&FfpHeader, NUMBYTES(FfpHeader));

  HRESULT hr = S_OK;

  hr = pIVideoStream->GetFrameCount(&FfpHeader.Video.cfTotal);
  FfpHeader.Video.dFPS = m_pTemplate->Video.Render.vex.dFPS;
  FfpHeader.Video.ntLength = SfVideo_FramesToTime(
      FfpHeader.Video.cfTotal, FfpHeader.Video.dFPS);
  FfpHeader.Video.bih = m_pTemplate->Video.Render.bih;
  FfpHeader.Video.cbFrameSize = SfDibImageBytes(&FfpHeader.Video.bih);
  if (fUseAudio) {
    hr = pIAudioStream->GetSampleCount(&FfpHeader.Audio.ccTotal);
    FfpHeader.Audio.wfx = m_pTemplate->Audio.Render.wfx;
    hr = pIAudioStream->GetStreamLength(&FfpHeader.Audio.ntLength);
    FfpHeader.Audio.ntLength = SfAudio_CellsToTime(FfpHeader.Audio.ccTotal,
        FfpHeader.Audio.wfx.nSamplesPerSec);
    m_bAudioFrame.resize(
        FfpHeader.Audio.wfx.nSamplesPerSec * FfpHeader.Audio.wfx.nBlockAlign * 2);
  }
  m_bVideoFrame.resize(FfpHeader.Video.bih.biSizeImage);

  FrameServerImpl::Init(!!fUseAudio, FfpHeader.Audio.wfx.nSamplesPerSec,
      FfpHeader.Audio.wfx.wBitsPerSample, FfpHeader.Audio.wfx.nChannels,
      (DWORD)FfpHeader.Video.cfTotal, FfpHeader.Video.dFPS,
      FfpHeader.Video.bih.biWidth, FfpHeader.Video.bih.biHeight, activewnd,
      (TCHAR*)m_sFilename.c_str());
  hr = S_OK;
  if (!Run())
    hr = SF_E_CANCEL;

  EnableWindow(activewnd, TRUE);
  SetForegroundWindow(activewnd);
  AttachThreadInput(GetCurrentThreadId(), threadid, FALSE);

  pIAudioStream->Release();
  pIVideoStream->Release();
  return hr;
}

bool VegasFSRender::OnAudioRequestOneSecond(DWORD second, LPBYTE* data, DWORD* datalen) {
  ISfReadAudioStream* pIAudioStream = NULL;

  m_pReadStreams->GetFirstAudioStream(&pIAudioStream, NULL);

  LONGLONG curpos, nextpos;
  curpos = (second * (LONGLONG)FfpHeader.Audio.wfx.nSamplesPerSec);
  nextpos = ((second + 1) * (LONGLONG)FfpHeader.Audio.wfx.nSamplesPerSec);
  ULONG ccToRead = (ULONG)(nextpos - curpos), ccActuallyRead = 0;
  *datalen = ccToRead * FfpHeader.Audio.wfx.nBlockAlign;
  if (*data == NULL)
    *data = (unsigned char*)malloc(*datalen);

  SFMEMORYTOKEN mt;
  SfInitLocalMemoryToken(mt, &m_bAudioFrame[0], ccToRead * FfpHeader.Audio.wfx.nBlockAlign);
  HRESULT hr = pIAudioStream->Read(curpos, mt, ccToRead, &ccActuallyRead);

  CMappingOfSfMemoryToken mtmap(mt);
  void* sourcePtr = NULL;
  if (hr == S_OK)
    hr = mtmap.GetPointer(&sourcePtr);

  if (hr == S_OK) {
    vars->audioBytesRead = ccActuallyRead * FfpHeader.Audio.wfx.nBlockAlign;
    memcpy(*data, sourcePtr, vars->audioBytesRead);
    memset(*data + ccActuallyRead * FfpHeader.Audio.wfx.nBlockAlign, 0,
        (ccToRead - ccActuallyRead) * FfpHeader.Audio.wfx.nBlockAlign);
  } else {
    vars->audioBytesRead = 0;
  }

  pIAudioStream->Release();
  return true;
}

void VegasFSRender::OnVideoRequest() {
  ISfReadVideoRenderStream* pIVideoStream = NULL;

  m_pReadStreams->GetFirstReadVideoRenderStream(&pIVideoStream, NULL);

  SFMEMORYTOKEN mt;
  SfInitLocalMemoryToken(mt, &m_bVideoFrame[0], (LONG)m_bVideoFrame.size());

  LONG_PTR idFrameLock = 0;
  SFGETFRAMEINFO getFrameInfo;
  LONG cbRead = 0;
  HRESULT hr = pIVideoStream->GetRenderFrame(vars->videoFrameIndex, &mt,
      &idFrameLock, &getFrameInfo, &cbRead);

  CMappingOfSfMemoryToken mtmap(mt);
  void* sourcePtr = NULL;
  if (hr == S_OK)
    hr = mtmap.GetPointer(&sourcePtr);

  if (hr == S_OK) {
    int rowBytes = FfpHeader.Video.cbFrameSize / vars->encBi.biHeight;
    ConvertVideoFrame(sourcePtr, rowBytes, vars);
    pIVideoStream->ReleaseRenderFrame(idFrameLock);
  } else {
    memset(((LPBYTE)vars) + vars->videooffset, 0, vars->videoBytesRead);
  }

  pIVideoStream->Release();
}

void VegasFSRender::OnAudioRequest() {
  ISfReadAudioStream* pIAudioStream = NULL;

  m_pReadStreams->GetFirstAudioStream(&pIAudioStream, NULL);

  LONGLONG curpos, nextpos;
  curpos = (vars->audioFrameIndex * (LONGLONG)FfpHeader.Audio.wfx.nSamplesPerSec) / 100;
  nextpos = ((vars->audioFrameIndex + 1) * (LONGLONG)FfpHeader.Audio.wfx.nSamplesPerSec) / 100;
  ULONG ccToRead = (ULONG)(nextpos - curpos), ccActuallyRead = 0;

  SFMEMORYTOKEN mt;
  SfInitLocalMemoryToken(mt, &m_bAudioFrame[0], ccToRead * FfpHeader.Audio.wfx.nBlockAlign);
  HRESULT hr = pIAudioStream->Read(curpos, mt, ccToRead, &ccActuallyRead);

  CMappingOfSfMemoryToken mtmap(mt);
  void* sourcePtr = NULL;
  if (hr == S_OK)
    hr = mtmap.GetPointer(&sourcePtr);

  if (hr == S_OK) {
    vars->audioBytesRead = ccActuallyRead * FfpHeader.Audio.wfx.nBlockAlign;
    memcpy(((BYTE*)vars) + vars->audiooffset, sourcePtr, vars->audioBytesRead);
  } else {
    vars->audioBytesRead = 0;
  }
  pIAudioStream->Release();
}
