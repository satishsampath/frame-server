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
#include "SfEnum.h"
#include "SfMem.h"
#include "SfFileIOManager.h"
#include "SfRenderFile.h"
#include "fscommon.h"
#include "fscommon_resource.h"
#include "VegasFS.h"
#include "VegasFSRender.h"

// {B4B9862D-A3C2-4f1a-C9D6-A8C235F4C9EE}
const GUID CLSID_VegasFS = {
  0xb4b9862d, 0xa3c2, 0x4f1a, { 0xc9, 0xd6, 0xa8, 0xc2, 0x35, 0xf4, 0xc9, 0xee }
};

// The first preset doesn't show up in the Vegas UI for whatever reason, so we define
// it twice with the hope of showing the remaining 4.
#define NUM_PRESETS 6
const GUID GUID_FrameServerPresets[NUM_PRESETS] = {
  // {7EE93CF0-6FDE-4AC7-BDB8-EE6D811392DB}
  { 0x7ee93cf0, 0x6fde, 0x4ac7, { 0xbd, 0xb8, 0xee, 0x6d, 0x81, 0x13, 0x92, 0xdb } },
  // {8A034144-D787-4AA7-9F43-E2DB07FED15A}
  { 0x8a034144, 0xd787, 0x4aa7, { 0x9f, 0x43, 0xe2, 0xdb, 0x07, 0xfe, 0xd1, 0x5a } },
  // {34C844B1-BDD7-eab8-8CF3-B6828FDD1FB3}
  { 0x34c844b1, 0xbdd7, 0xeab8, { 0x8c, 0xf3, 0xb6, 0x82, 0x8f, 0xdd, 0x1f, 0xb3 } },
  // {34C844B1-BDD7-eab8-8CF3-B6828FDD1FB4}
  { 0x34c844b1, 0xbdd7, 0xeab8, { 0x8c, 0xf3, 0xb6, 0x82, 0x8f, 0xdd, 0x1f, 0xb4 } },
  // {DA8BC590-420F-44BB-8D83-BFD38B075A74}
  { 0xda8bc590, 0x420f, 0x44bb, { 0x8d, 0x83, 0xbf, 0xd3, 0x8b, 0x07, 0x5a, 0x74 } },
  // {E7987AB0-062A-466C-97ED-E5AF181956C1}
  { 0xe7987ab0, 0x062a, 0x466c, { 0x97, 0xed, 0xe5, 0xaf, 0x18, 0x19, 0x56, 0xc1 } }
};
static const int sAudioSampleRates[NUM_PRESETS] = { 16000, 16000, 32000, 44100, 48000, 96000 };

#define TEMPLATE_MINOR_VERSION   (2 << 16)
#define TEMPLATE_SUB_VERSION     (0 << 8)
#define TEMPLATE_LETTER_VERSION  0

struct ExtraEncoderParameter
///////////////////////////////////////////////////////////////////////////////////////////////////
{
   DWORD ccPreferedAudioInterleaveSize;
   DWORD dwPaddingGranularity;
   DWORD dwCompressedVideoQuality;     // only valid for fccVideoCompressor = BI_MJEPG
   DWORD bMotionJPEG_B;                // only valid for fccVideoCompressor = BI_MJEPG
};

ExtraEncoderParameter eeParam = {0};

const GUID& VegasFS::ms_uidRenderer(CLSID_VegasFS);
DWORD VegasFS::ms_dwStreamTypes = SFFIO_STREAM_TYPE_AUDIO | SFFIO_STREAM_TYPE_VIDEO;

VegasFS::VegasFS() : m_dwRef(0), m_ixType(-1), m_pFileIOManager(NULL) {
}

VegasFS::~VegasFS() {
  SetIOManager(NULL);
}

STDMETHODIMP VegasFS::QueryInterface(REFIID riid, LPVOID* ppvObj) {
  if (__uuidof(ISfRenderFileClass) == riid || IID_IUnknown == riid) {
    AddRef();
    *ppvObj = static_cast<ISfRenderFileClass*>(this);
    return S_OK;
  }
  else 
   if (__uuidof(ISfRenderMetaSupport) == riid)
   {
      AddRef();
      *ppvObj = static_cast<ISfRenderMetaSupport*>(this);
      return S_OK;
   }
   else
   if (__uuidof(ISfRenderMetaSupport2) == riid)
   {
      AddRef();
      *ppvObj = static_cast<ISfRenderMetaSupport2*>(this);
      return S_OK;
   }
  return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) VegasFS::AddRef() {
  return InterlockedIncrement((long*)&m_dwRef);
}

STDMETHODIMP_(ULONG) VegasFS::Release() {
  LONG cRef = InterlockedDecrement((long*)&m_dwRef);
  if(cRef == 0)
  {
    SfDelete this;
  }
  return cRef;
}

STDMETHODIMP VegasFS::SetTypeIndex(LONG ixType) {
  m_ixType = ixType;
  return S_OK;
}

STDMETHODIMP VegasFS::GetTypeIndex(LONG* pixType) {
  HRESULT hr = S_OK;

  if (pixType) {
    *pixType = m_ixType;
    hr = S_OK;
  } else {
    hr = E_INVALIDARG;
  }

  return hr;
}

STDMETHODIMP VegasFS::SetIOManager(ISfFileIOManager* pBack) {
  if (m_pFileIOManager != pBack) {
    if (m_pFileIOManager) {
      m_pFileIOManager->Release();
      m_pFileIOManager = NULL;
    }

    m_pFileIOManager = pBack;

    if (m_pFileIOManager) {
      m_pFileIOManager->AddRef();
    }
  }

  return S_OK;
}

STDMETHODIMP VegasFS::GetIOManager(ISfFileIOManager** ppBack) {
  HRESULT hr = S_OK;

  if (ppBack && m_pFileIOManager) {
    hr = m_pFileIOManager->QueryInterface(__uuidof(ISfFileIOManager), (void**)ppBack);
  } else {
    hr = E_INVALIDARG;
  }

  return hr;
}

STDMETHODIMP VegasFS::GetTypeID(GUID* puuid) {
  HRESULT hr = S_OK;

  if (puuid) {
    *puuid = ms_uidRenderer;
    hr = S_OK;
  } else {
    hr = E_INVALIDARG;
  }

  return hr;
}

STDMETHODIMP VegasFS::GetAllowedStreamTypes(DWORD* fdwStreamTypes) {
  HRESULT hr = S_OK;

  if (fdwStreamTypes) {
    *fdwStreamTypes = ms_dwStreamTypes;
    hr = S_OK;
  } else {
    hr = E_INVALIDARG;
  }

  return hr;
}

STDMETHODIMP VegasFS::GetCaps(SFFILERENDERCAPS* pCaps) {
  HRESULT hr = S_OK;

  if (pCaps) {
    SFFILERENDERCAPS caps = {0};
    *pCaps = caps;
    hr = S_OK;
  } else {
    hr = E_INVALIDARG;
  }

  return hr;
}

STDMETHODIMP VegasFS::GetStrings(const SFFIO_FILECLASS_INFO_TEXT* paStringIds,
    ULONG cStrings, BSTR* abstrStrings) {
  struct {
    int key;
    wchar_t* value;
  } strings[] = {
    // non-localized Type Name (short version)
    { SFFIO_FILECLASS_TEXT_BriefTypeNameEng, L"FrameServer" },
    // non-localized FileTypeName (long version)
    { SFFIO_FILECLASS_TEXT_FileTypeNameEng, L"DebugMode FrameServer" },
    // list of possible file extensions
    { SFFIO_FILECLASS_TEXT_FileExtensions, L"*.avi" },
    // build type tag (FINAL builds have "" as the tag)
    { SFFIO_FILECLASS_TEXT_BuildTypeTag, L"" },
    // Localized Type Name (long version)
    { SFFIO_FILECLASS_TEXT_FileTypeNameLocalized, L"DebugMode FrameServer"},
    { SFFIO_FILECLASS_TEXT_DialogTitleLocalized, L"DebugMode FrameServer"},
    { SFFIO_FILECLASS_TEXT_CopyrightLocalized, L"Copyright (C) 2000-2019 Satish Kumar. S. All Rights Reserved." },
    { SFFIO_FILECLASS_TEXT_SupportURL, L"http://www.debugmode.com/frameserver/" },
    { SFFIO_FILECLASS_TEXT_SupportURLExplanationLocalized, L"Homepage"}
  };

  for (ULONG i = 0; i < cStrings; ++i) {
    for (int j = 0; j < NUMELMS(strings); ++j) {
      if (strings[j].key == paStringIds[i]) {
        BSTR bstr = SysAllocStringLen(NULL, (UINT)wcslen(strings[j].value));
        wcscpy(bstr, strings[j].value);
        abstrStrings[i] = bstr;
        break;
      }
    }
  }

  return S_OK;
}

STDMETHODIMP VegasFS::Open(ISfRenderFile* pSfFile,
    HANDLE hfile,
    LPCWSTR pszFilename) {
  return S_OK;
}

STDMETHODIMP VegasFS::Create(ISfRenderFile** pSfFile,
    LPCWSTR pszFilename,
    LONG cStreams,
    ISfReadStream** paStreams,
    PCSFTEMPLATEx pTemplate) {
  HRESULT hr = S_OK;
  VegasFSRender* fsr = SfNew VegasFSRender(this, pTemplate);

  fsr->Init();
  hr = fsr->SetFile(pszFilename);

  if (SUCCEEDED(hr)) {
    for (LONG i = 0; i < cStreams; ++i) {
      hr = fsr->SetStreamReader(paStreams[i], i);
      if (FAILED(hr)) {
        break;
      }
    }
  }

  if (SUCCEEDED(hr)) {
    hr = fsr->QueryInterface(__uuidof(ISfRenderFile), (void**)pSfFile);
  }
  if (FAILED(hr)) {
    delete fsr;
  }

  return hr;
}

STDMETHODIMP VegasFS::GetPresetCount(LONG* pcPresets) {
  HRESULT hr = S_OK;

  if (pcPresets) {
    *pcPresets = NUM_PRESETS;
    hr = S_OK;
  } else {
    hr = E_INVALIDARG;
  }
  return hr;
}

STDMETHODIMP VegasFS::AllocateTemplate(PSFTEMPLATEx* ppTemplate,
    DWORD cbSizeExtra,
    LPCTSTR pszName,
    LPCTSTR pszNotes) {
  HRESULT hr = S_OK;

  if (ppTemplate) {
    hr = S_OK;
  } else {
    hr = E_INVALIDARG;
  }

  SFTEMPLATEx* pTemplate = NULL;
  DWORD cbTotalSize = 0;
  if (SUCCEEDED(hr)) {
    cbTotalSize = sizeof(SFTEMPLATEx);
    cbTotalSize += cbSizeExtra;

    if (pszName)
      cbTotalSize += (DWORD)((wcslen(pszName) + 1) * sizeof(wchar_t));
    if (pszNotes)
      cbTotalSize += (DWORD)((wcslen(pszNotes) + 1) * sizeof(wchar_t));

    pTemplate = (SFTEMPLATEx*)SfMalloc(cbTotalSize);

    if (pTemplate) {
      memset(pTemplate, 0, cbTotalSize);
      hr = S_OK;
    } else {
      hr = E_OUTOFMEMORY;
    }
  }

  if (SUCCEEDED(hr)) {
    pTemplate->cbStruct = cbTotalSize;
    pTemplate->cbExtra = cbSizeExtra;

    pTemplate->cchAllocText = 0;

    if (pszName)
      pTemplate->cchAllocText += DWORD(wcslen(pszName) + 1);

    if (pszNotes)
      pTemplate->cchAllocText += DWORD(wcslen(pszNotes) + 1);

    pTemplate->dwVer = SFFILEIO_TEMPLATE_MAJOR_VERSION |
                       TEMPLATE_MINOR_VERSION |
                       TEMPLATE_SUB_VERSION |
                       TEMPLATE_LETTER_VERSION;

    *ppTemplate = pTemplate;
  }

  return hr;
}

STDMETHODIMP VegasFS::GetReadClass(ISfReadFileClass** ppReadClass) {
  return E_NOTIMPL;
}

STDMETHODIMP VegasFS::GetPreset(PSFTEMPLATEx* ppTemplate,
    LONG ixPreset) {
  HRESULT hr = S_OK;

  if (ppTemplate && ixPreset < NUM_PRESETS) {
    TCHAR sName[MAX_PATH];
    TCHAR sDesc[] = _T("");
    _stprintf(sName, _T("Audio: %.1fKhz, Video: project default"), sAudioSampleRates[ixPreset] / 1000.0f);
    if (SUCCEEDED(hr)) {
      hr = AllocateTemplate(ppTemplate,
          sizeof(ExtraEncoderParameter),
          sName,
          sDesc);

      if (SUCCEEDED(hr)) {
        PSFTEMPLATEx pTemplate = *ppTemplate;

        pTemplate->id = ixPreset;
        pTemplate->iidOwner = CLSID_VegasFS;
        pTemplate->guid = GUID_FrameServerPresets[ixPreset];

        // audio
        pTemplate->Audio.cbStruct = NUMBYTES(pTemplate->Audio);
        pTemplate->Audio.cStreams = 1;

        SfAudio_InitWfxExt(&pTemplate->Audio.Render.wfx,
            WAVE_FORMAT_PCM, sAudioSampleRates[ixPreset], 16, 2, 0);
        SfAudio_InitWfxExt(&pTemplate->Audio.Codec.wfx,
            WAVE_FORMAT_PCM, sAudioSampleRates[ixPreset], 16, 2, 0);

        // video
        pTemplate->Video.cbStruct = NUMBYTES(pTemplate->Video);
        pTemplate->Video.cStreams = 1;

        pTemplate->Video.Render.bih.biSize = sizeof(pTemplate->Video.Render.bih);
        pTemplate->Video.Render.bih.biWidth = 0;
        pTemplate->Video.Render.bih.biHeight = 0;
        pTemplate->Video.Render.bih.biPlanes = 1;
        pTemplate->Video.Render.bih.biBitCount = 32;
        pTemplate->Video.Render.bih.biCompression = BI_RGB;
        pTemplate->Video.Render.bih.biSizeImage = 0;
        pTemplate->Video.Render.bih.biXPelsPerMeter = 0;
        pTemplate->Video.Render.bih.biYPelsPerMeter = 0;
        pTemplate->Video.Render.bih.biClrUsed = 0;
        pTemplate->Video.Render.bih.biClrImportant = 0;

        pTemplate->Video.Render.vex.Init(0,
            SFINTERLACETYPE_PROGRESSIVE,
            1.0);

        pTemplate->Video.Codec.bih = pTemplate->Video.Render.bih;
        pTemplate->Video.Codec.bih.biCompression = BI_RGB;
        pTemplate->Video.Codec.cKeyFrameEvery = 1;

        CopyMemory( pTemplate->abExtra, &eeParam, pTemplate->cbExtra);

        (static_cast<SfTemplatex*>(pTemplate))->SetText(sName, sDesc);
      } else {
        hr = E_INVALIDARG;
      }
    }
  } else {
    hr = E_INVALIDARG;
  }

  return hr;
}

STDMETHODIMP VegasFS::GetPresetIndexFromGuid(const GUID* pGuid,
    LONG* pixPreset) {
  HRESULT hr = E_INVALIDARG;
  if (pGuid && pixPreset) {
    *pixPreset = 0;
    for (int i = 0; i < NUM_PRESETS; ++i) {
      if (*pGuid == GUID_FrameServerPresets[i]) {
        *pixPreset = i;
        hr = S_OK;
        break;
      }
    }
  }

  return hr;
}

STDMETHODIMP VegasFS::ChooseBestPreset(PCSFTEMPLATEx ptplToMatch,
    LONG eMatchRules,
    LONG* pixPreset) {
  return S_OK;
}

STDMETHODIMP VegasFS::ConformTemplate(PSFTEMPLATEx* ppOutTemplate,
    PCSFTEMPLATEx pSourceTemplate,
    SFFIO_CONFORM_TEMPLATE_REASON eConform) {
  HRESULT hr = S_OK;

  LPCTSTR pszName = L"Project Settings";
  LPCTSTR pszNotes = L"Render Audio and Video";

  if (ppOutTemplate) {
    hr = S_OK;
  } else {
    hr = E_POINTER;
  }

  if (SUCCEEDED(hr)) {
    *ppOutTemplate = NULL;

    if (SFFIO_CONFORM_TPL_PROJECT == eConform) {
      hr = AllocateTemplate(ppOutTemplate,
          sizeof(ExtraEncoderParameter),
          pszName,
          pszNotes);
    } else {
      hr = E_INVALIDARG;
    }
  }

  if (SUCCEEDED(hr)) {
    (*ppOutTemplate)->id = pSourceTemplate->id;
    (*ppOutTemplate)->iidOwner = CLSID_VegasFS;

    (*ppOutTemplate)->Audio.cbStruct = sizeof(pSourceTemplate->Audio);
    (*ppOutTemplate)->Audio.cStreams = 1;

    (*ppOutTemplate)->Audio.Render.wfx = pSourceTemplate->Audio.Render.wfx;
    (*ppOutTemplate)->Audio.Codec.wfx = pSourceTemplate->Audio.Codec.wfx;

    if ((*ppOutTemplate)->Audio.Codec.wfx.IsEmpty()) {
      (*ppOutTemplate)->Audio.Codec.wfx = (*ppOutTemplate)->Audio.Render.wfx;
    }

    (*ppOutTemplate)->Video.cbStruct = sizeof(pSourceTemplate->Video);
    (*ppOutTemplate)->Video.cStreams = 1;

    (*ppOutTemplate)->Video.Render.bih = pSourceTemplate->Video.Render.bih;
    (*ppOutTemplate)->Video.Render.vex = pSourceTemplate->Video.Render.vex;

    (*ppOutTemplate)->Video.Codec.bih = pSourceTemplate->Video.Codec.bih;

    if ((*ppOutTemplate)->Video.Codec.bih.biHeight == 0) {
      (*ppOutTemplate)->Video.Codec.bih = (*ppOutTemplate)->Video.Render.bih;
    }

    if(pSourceTemplate->cbExtra == sizeof(ExtraEncoderParameter)) {
      CopyMemory( (*ppOutTemplate)->abExtra,
        pSourceTemplate->abExtra,
        sizeof(ExtraEncoderParameter));
    }

    (static_cast<SfTemplatex*>(*ppOutTemplate))->SetText(pszName, pszNotes);
  }

  return hr;
}

STDMETHODIMP VegasFS::ConformTemplateToSource(PSFTEMPLATEx* pptpl,
    PCSFTEMPLATEx ptplBase,
    PCSFTEMPLATEx ptplSource,
    SFFIO_CONFORM_TEMPLATE_REASON eConform) {
  HRESULT hr = S_OK;

  if (SFFIO_CONFORM_TPL_PROJECT == eConform) {
    return S_FALSE;     // no need to conform
  }

  if (pptpl && ptplBase && ptplSource) {
    hr = S_OK;
  } else {
    hr = E_INVALIDARG;
  }

  if (SUCCEEDED(hr)) {
    *pptpl = (PSFTEMPLATEx)SfMalloc(ptplBase->cbStruct);

    if (*pptpl) {
      memset(*pptpl, 0, ptplBase->cbStruct);
      CopyMemory(*pptpl, ptplBase, ptplBase->cbStruct);
      if ((*pptpl)->Video.Render.bih.biWidth == 0)
      {
        (*pptpl)->Video.Render.bih = ptplSource->Video.Render.bih;
        (*pptpl)->Video.Render.bih.biCompression = BI_RGB;
        (*pptpl)->Video.Render.bih.biPlanes = 1;
        (*pptpl)->Video.Render.bih.biBitCount = 32;
       }
      if (ptplBase->Video.Render.vex.dFPS == 0.0)
        (*pptpl)->Video.Render.vex = ptplSource->Video.Render.vex;
    } else {
      hr = E_OUTOFMEMORY;
    }
  }

  if (SUCCEEDED(hr)) {
    if ((*pptpl)->Video.Render.bih.biWidth == 0) {
      (*pptpl)->Video.Render.bih.biWidth = ptplSource->Video.Render.bih.biWidth;
      (*pptpl)->Video.Render.bih.biHeight = ptplSource->Video.Render.bih.biHeight;
    }

    if ((*pptpl)->Video.Render.bih.biBitCount == 0) {
      (*pptpl)->Video.Render.bih.biBitCount = ptplSource->Video.Render.bih.biBitCount;
      (*pptpl)->Video.Render.bih.biCompression = BI_RGB;
    }

    if ((*pptpl)->Video.Codec.bih.biWidth == 0) {
      (*pptpl)->Video.Codec.bih = (*pptpl)->Video.Render.bih;
    }

    if ((*pptpl)->Audio.Codec.wfx.IsEmpty()) {
      (*pptpl)->Audio.Codec.wfx = (*pptpl)->Audio.Render.wfx;
    }
  }

  return hr;
}

STDMETHODIMP VegasFS::GetTemplateInfo(PCSFTEMPLATEx pTemplate,
    LPVOID pvInfo,
    UINT cbInfo,
    SFFIO_TEMPLATE_INFO eInfoType) {
  return E_NOTIMPL;
}

#if defined(VEGAS_SDK_FROM_V3)
STDMETHODIMP VegasFS::GetTemplateInfoText(PCSFTEMPLATEx pTemplate,
    LPWSTR pszwInfo, 
    UINT cchInfo,
    SFFIO_TEMPLATE_INFOTEXT eInfoType) {
  return E_NOTIMPL;
}
#endif

STDMETHODIMP VegasFS::GetPropertyPageCount(LONG* pcFreePages,
    LONG* pcPagesForSale,
    SFFILERENDERPAGEOPTIONS* prPageOptions) {
  return E_NOTIMPL;
}

STDMETHODIMP VegasFS::GetPropertyPage(LONG ixIndex,
    IPropertyPage** ppPropPage) {
  return E_NOTIMPL;
}

STDMETHODIMP VegasFS::BuyPropertyPages(HWND hwndParent) {
  return E_NOTIMPL;
}

STDMETHODIMP VegasFS::HasAboutBox(void) {
  return S_OK;
}

STDMETHODIMP VegasFS::ShowAboutBox(HWND hwndParent) {
  LoadCommonResource();
  DialogBox(ghResInst, MAKEINTRESOURCE(IDD_DFABOUTDLG), hwndParent, AboutDlgProc);
  return S_OK;
}

STDMETHODIMP VegasFS::GetStatus(HRESULT* phrStatus,
    BSTR* bstrStatus) {
  return E_NOTIMPL;
}

// START: ISfRenderMetaSupport

STDMETHODIMP VegasFS::Reset() {
  HRESULT hr = S_OK;
  // TODO: implement
  return hr;
}

STDMETHODIMP VegasFS::GetNextSumTag(FOURCC* fcc, 
    DWORD* pcchLen) {
  HRESULT hr = S_FALSE;
  // TODO: implement
  return hr;
}

STDMETHODIMP VegasFS::GetSupportedTypes(SFFIO_TYPESOFMETADATA* peEmbedded,
    SFFIO_TYPESOFMETADATA* peExternal) {
  HRESULT hr = S_OK;

  if (peEmbedded)
    *peEmbedded = SFFIO_METADATA_NONE; //(SFFIO_TYPESOFMETADATA) (SFFIO_METADATA_SUMMARYINFO | SFFIO_METADATA_COMMANDS | SFFIO_METADATA_MARKERS);

  if (peExternal)
    *peExternal = SFFIO_METADATA_NONE;
  return hr;
}
