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

#include <float.h>
#include <math.h>
#include <winsock.h>
#include "SfFioGuids.h"
#include "SfHelpers.h"
#include "ifileio.h"
#include "ifilemng.h"
#include "blankavi.h"
#include "dfsc.h"
#include "resource.h"
#include "utils.h"
#include "VegasFS.h"
#include "VegasFSRender.h"

EXTERN_C GUID CLSID_VegasFS;
GUID VegasFS::ms_TypeGUID = CLSID_VegasFS;
DWORD VegasFS::ms_dwTemplateVer = SFFILEIO_TEMPLATE_MAJOR_VERSION;
DWORD VegasFS::ms_dwStreamTypes =
  SFFIO_STREAM_TYPE_AUDIO | SFFIO_STREAM_TYPE_VIDEO;
SFFILERENDERCAPS VegasFS::ms_TypeCapabilities = { 0 };

// The default rendering template declarations.

struct {
  WORD dummy;
} gs_VegasFSExtraData = { 0 };

SFAUDIOTEMPLATEx VegasFS::ms_ClassDefaultAudioTemplate = {
  { WAVE_FORMAT_PCM, 2, 44100, 44100 * (2 * 16 / 8), 2 * 16 / 8, 16, 0 },
  1,
  NULL,
  { 0, 0, { 0, { 0 }, 0.0f, 0, { 0 }, 120.0f } },
  NUMBYTES(gs_VegasFSExtraData),
  &gs_VegasFSExtraData,
};

SFVIDEOTEMPLATEx VegasFS::ms_ClassDefaultVideoTemplate = {
  NULL,
  1,
  {
    NUMBYTES(SFVIDEOEXTENSION),
    SFCOLORSPACE_UNKNOWN,
    0,
    SFINTERLACETYPE_PROGRESSIVE,
    0.0, 0.0, 0.0,
    { SFALPHATYPE_UNDEFINED, FALSE, {0.0, 0.0, 0.0, 0.0} }
  },
  NULL,
  1,
  {
    NUMBYTES(SFVIDEOEXTENSION),
    SFCOLORSPACE_UNKNOWN,
    0,
    SFINTERLACETYPE_PROGRESSIVE,
    0.0, 0.0, 0.0,
    { SFALPHATYPE_UNDEFINED, FALSE, {0.0, 0.0, 0.0, 0.0} }
  },
  NUMBYTES(gs_VegasFSExtraData),
  &gs_VegasFSExtraData,
};

SFTEMPLATEx VegasFS::ms_ClassDefaultTemplate = {
  NUMBYTES(SFTEMPLATEx),
  VegasFS::ms_dwTemplateVer,
  0,
  IDS_DEFAULT,
  IDS_DEFAULT,
  {L"Default"},
  {L"Default"},
  &VegasFS::ms_ClassDefaultAudioTemplate,
  &VegasFS::ms_ClassDefaultVideoTemplate,
};

// Functions.

VegasFS::VegasFS() : m_dwRef(0), m_ixType(0) {
  m_pBack = NULL;
  m_pActiveTemplate = NULL;
  m_pDefaultTemplate = NULL;
  ms_TypeCapabilities.fMultiStream = TRUE;
  ms_TypeCapabilities.fHasMarkers = TRUE;
  ms_TypeCapabilities.fHasCommands = FALSE;
  ms_TypeCapabilities.fHasMetaData = TRUE;
  ms_TypeCapabilities.fHasSettings = FALSE;
  ms_TypeCapabilities.fCanCopyMedia = TRUE;
  ms_TypeCapabilities.fAllowAppOverrides = TRUE;
};

VegasFS::~VegasFS() {
  freeTemplate(m_pActiveTemplate);
  freeTemplate(m_pDefaultTemplate);
  if (m_pBack)
    m_pBack->Release();
}

STDMETHODIMP VegasFS::QueryInterface(REFIID riid, LPVOID* ppvObj) {
  if (IID_ISfRenderFileClass == riid || IID_IUnknown == riid) {
    AddRef();
    *ppvObj = static_cast<ISfRenderFileClass*>(this);
    return NOERROR;
  } else if (IID_ISfFIO_TemplateInfo_Renderer == riid) {
    AddRef();
    *ppvObj = static_cast<ISfFIO_TemplateInfo_Renderer*>(this);
    return NOERROR;
  } else if (IID_ISfFIO_PrivateCopyPreset == riid) {
    AddRef();
    *ppvObj = static_cast<ISfFIO_PrivateCopyPreset*>(this);
    return NOERROR;
  } else if (IID_ISfRenderMetaSupport == riid ||
             IID_ISfRenderMetaSupport2 == riid) {
    AddRef();
    *ppvObj = static_cast<ISfRenderMetaSupport2*>(this);
    return NOERROR;
  }

  return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) VegasFS::AddRef() {
  m_dwRef++;
  return m_dwRef;
}

STDMETHODIMP_(ULONG) VegasFS::Release() {
  m_dwRef--;
  if (m_dwRef == 0) {
    delete this;
    return 0;
  } else
    return m_dwRef;
}

STDMETHODIMP VegasFS::GetSupportedTypes(SFFIO_TYPESOFMETADATA* peEmbedded, SFFIO_TYPESOFMETADATA* peExternal) {
  if (peEmbedded)
    *peEmbedded = SFFIO_METADATA_NONE;

  if (peExternal)
    *peExternal = SFFIO_METADATA_NONE;

  return S_OK;
}

STDMETHODIMP VegasFS::SetTypeIndex(LONG ixType) {
  m_ixType = ixType;
  return S_OK;
}

STDMETHODIMP_(LONG) VegasFS::GetTypeIndex() {
  return m_ixType;
}

STDMETHODIMP VegasFS::GetTypeID(GUID* puuid) {
  *puuid = ms_TypeGUID;
  return S_OK;
}

STDMETHODIMP VegasFS::GetAllowedStreamTypes(DWORD* fdwStreamTypes) {
  *fdwStreamTypes = ms_dwStreamTypes;
  return S_OK;
}

STDMETHODIMP VegasFS::GetCaps(PSFFILERENDERCAPS pCaps) {
  memcpy(pCaps, &ms_TypeCapabilities, sizeof(ms_TypeCapabilities));
  return S_OK;
}

STDMETHODIMP VegasFS::ShowAboutBox(HWND hwndParent) {
  LoadCommonResource();
  DialogBox(ghResInst, MAKEINTRESOURCE(IDD_DFABOUTDLG), hwndParent, AboutDlgProc);
  return S_OK;
}

STDMETHODIMP VegasFS::initObj(void) {
  HRESULT hr = E_OUTOFMEMORY;

  m_pDefaultTemplate = allocTemplate();
  if (m_pDefaultTemplate) {
    hr = copyTemplate(m_pDefaultTemplate, &ms_ClassDefaultTemplate);
    if (SUCCEEDED(hr) && m_pBack) {
      ISfFileIOTemplate* pITemplate = NULL;
      HRESULT hr = m_pBack->QueryInterface(IID_ISfFileIOTemplate, (void**)&pITemplate);
      if (SUCCEEDED(hr)) {
        LPWAVEFORMATEX pwfx = (LPWAVEFORMATEX)pITemplate->AllocTemplateBuffer(
            GHND, sizeof(WAVEFORMATEX));
        if (pwfx) {
          *pwfx = m_pDefaultTemplate->pAudioTemplate->wfx;
        } else {
          hr = E_OUTOFMEMORY;
        }
        pITemplate->Release();
        m_pDefaultTemplate->pAudioTemplate->pwfxCodec = pwfx;
      }
    }
  }
  return hr;
}

STDMETHODIMP VegasFS::SetIOManager(ISfFileIOManager* pBack) {
  if (m_pBack)
    m_pBack->Release();
  m_pBack = pBack;
  m_pBack->AddRef();
  return NOERROR;
}

STDMETHODIMP VegasFS::GetIOManager(ISfFileIOManager** ppBack) {
  if (NULL == m_pBack)
    return E_NOINTERFACE;
  *ppBack = m_pBack;
  m_pBack->AddRef();
  return NOERROR;
}

STDMETHODIMP VegasFS::InitDefaultTemplate(PSFTEMPLATEx pDefaultTemplate) {
  HRESULT hr = E_OUTOFMEMORY;

  if (NULL == m_pDefaultTemplate) {
    m_pDefaultTemplate = allocTemplate();
    if (m_pDefaultTemplate)
      hr = copyTemplate(m_pDefaultTemplate, &ms_ClassDefaultTemplate);
  }

  if (m_pDefaultTemplate && pDefaultTemplate) {
    hr = copyTemplate(m_pDefaultTemplate, pDefaultTemplate);
  }

  if (SUCCEEDED(hr)) {
    wcsncpy(m_pDefaultTemplate->szName, L"Default", NUMCHARS(m_pDefaultTemplate->szName));
    wcsncpy(m_pDefaultTemplate->szDesc, L"Default", NUMCHARS(m_pDefaultTemplate->szDesc));
  }

  return hr;
}

STDMETHODIMP VegasFS::Create(ISfRenderFile** pSfFile,
    LPCWSTR pszFilename,
    LONG cStreams,
    ISfReadStream** paStreams) {
  HRESULT hr = E_OUTOFMEMORY;
  PSFTEMPLATEx pTemplate = NULL;

  pTemplate = allocTemplate();
  if (pTemplate) {
    if (m_pActiveTemplate) {
      hr = copyTemplate(pTemplate, m_pActiveTemplate);
    } else if (m_pDefaultTemplate) {
      hr = copyTemplate(pTemplate, m_pDefaultTemplate);
    } else {
      freeTemplate(pTemplate);
      hr = S_OK;
      pTemplate = NULL;
    }

    if (SUCCEEDED(hr)) {
      VegasFSRender* pFileObj = new VegasFSRender(this, pTemplate);
      if (pFileObj) {
        hr = pFileObj->initObj();
        if (SUCCEEDED(hr)) {
          hr = pFileObj->SetFile(pszFilename);
          if (SUCCEEDED(hr)) {
            for (LONG ii = 0; SUCCEEDED(hr) && ii < cStreams; ii++)
              hr = pFileObj->SetStreamReader(paStreams[ii], ii);
            if (SUCCEEDED(hr))
              hr = pFileObj->QueryInterface(IID_ISfRenderFile, (void**)pSfFile);
          }
        }
      }
    }
  }

  return hr;
}

STDMETHODIMP VegasFS::GetPresetCount(long* pcPresets) {
  if (pcPresets)
    *pcPresets = 1;
  return S_OK;
}

STDMETHODIMP VegasFS::GetPresetName(int ixPreset, LPWSTR pszName, UINT cchName) {
  wcsncpy(pszName, L"Default", max(8, cchName));
  return S_OK;
}

STDMETHODIMP VegasFS::GetPresetDescription(int ixPreset, LPWSTR pszDesc, UINT cchDesc) {
  HRESULT hr = E_FAIL;

  if (0 == ixPreset && m_pDefaultTemplate) {
    wcsncpy(pszDesc, m_pDefaultTemplate->szDesc, cchDesc);
    hr = S_OK;
  }

  return hr;
}

STDMETHODIMP VegasFS::GetPresetVersion(long ixPreset, LPDWORD pdwVersion) {
  HRESULT hr = E_FAIL;

  if (0 == ixPreset) {
    *pdwVersion = VegasFS::ms_dwTemplateVer;
    hr = S_OK;
  }
  return hr;
}

STDMETHODIMP VegasFS::UsePreset(long ixPreset) {
  HRESULT hr;

  if (NULL == m_pActiveTemplate)
    m_pActiveTemplate = allocTemplate();

  if (NULL == m_pActiveTemplate)
    return E_OUTOFMEMORY;

  hr = CopyPreset(m_pActiveTemplate, ixPreset);

  if (SUCCEEDED(hr))
    m_fActiveTemplateDirty = FALSE;

  return hr;
}

STDMETHODIMP VegasFS::CopyPreset(PSFTEMPLATEx pTemplate, long ixPreset) {
  HRESULT hr = E_FAIL;

  if (NULL == pTemplate)
    return E_OUTOFMEMORY;

  if (0 == ixPreset) {
    hr = NOERROR;
    if (NULL == m_pDefaultTemplate) {
      hr = InitDefaultTemplate(NULL);
    }
    if (SUCCEEDED(hr)) {
      hr = copyTemplate(pTemplate, m_pDefaultTemplate);
    }
  }

  return hr;
}

STDMETHODIMP VegasFS::GetTemplateInfoText(PSFTEMPLATEx pTemplate,
    LPWSTR pszwInfo,
    UINT cchInfo,
    SFFIO_TEMPLATE_INFOTEXT eInfoType) {
  pszwInfo[0] = '\0';
  return S_OK;
}

STDMETHODIMP VegasFS::GetTemplateInfo(PSFTEMPLATEx pTemplate,
    LPVOID pvInfo,
    UINT cbInfo,
    SFFIO_TEMPLATE_INFO eInfoType) {
  return S_FALSE;
}

STDMETHODIMP VegasFS::SaveSettings(LPSTREAM pStrm, BOOL fClearDirty) {
  if (NULL == m_pBack)
    return E_FAIL;

  ISfFileIOTemplate* pITemplate = NULL;
  HRESULT hr = m_pBack->QueryInterface(IID_ISfFileIOTemplate, (void**)&pITemplate);
  if (SUCCEEDED(hr)) {
    m_pActiveTemplate->dwVer = ms_dwTemplateVer;
    hr = pITemplate->SaveTemplateData(m_pActiveTemplate, pStrm);
    pITemplate->Release();

    if (SUCCEEDED(hr) && fClearDirty)
      m_fActiveTemplateDirty = FALSE;
  }
  return hr;
}

STDMETHODIMP VegasFS::LoadSettings(LPSTREAM pStrm) {
  if (NULL == m_pBack)
    return E_FAIL;

  HRESULT hr = E_OUTOFMEMORY;
  PSFTEMPLATEx pTemplate = allocTemplate();
  if (pTemplate) {
    ISfFileIOTemplate* pITemplate = NULL;
    hr = m_pBack->QueryInterface(IID_ISfFileIOTemplate, (void**)&pITemplate);
    if (SUCCEEDED(hr)) {
      hr = pITemplate->LoadTemplateData(pTemplate, pStrm, ms_dwTemplateVer);
      pITemplate->Release();

      if (SUCCEEDED(hr)) {
        m_fActiveTemplateDirty = FALSE;
        freeTemplate(m_pActiveTemplate);
        m_pActiveTemplate = pTemplate;

        pTemplate = NULL;
      }
    }
    if (pTemplate)
      freeTemplate(pTemplate);
  }

  return hr;
}

STDMETHODIMP VegasFS::GetStrings(LPWSTR pszFileSaveDialogString,
    UINT cchFileSaveDialogString,
    LPWSTR pszFileExtensions,
    UINT cchFileExtensions,
    LPWSTR pszFileTypeName,
    UINT cchFileTypeName) {
  HRESULT hr = S_OK;

  if (cchFileSaveDialogString && pszFileSaveDialogString) {
    wcsncpy(pszFileSaveDialogString, LABOUTSTR,
        cchFileSaveDialogString);
  }

  if (pszFileExtensions && cchFileExtensions) {
    wcsncpy(pszFileExtensions, L"*.avi", cchFileExtensions);
  }

  if (pszFileTypeName && cchFileTypeName) {
    wcsncpy(pszFileTypeName, LABOUTSTR, cchFileTypeName);
  }

  return hr;
}

STDMETHODIMP VegasFS::GetActiveTemplateInfo(PSFTEMPLATEx* ppTemplate) {
  *ppTemplate = allocTemplate();
  return copyTemplate(*ppTemplate, m_pActiveTemplate);
}

STDMETHODIMP VegasFS::FreeActiveTemplateInfo(PSFTEMPLATEx pTemplate) {
  freeTemplate(pTemplate);
  return S_OK;
}

HRESULT VegasFS::copyTemplate(PSFTEMPLATEx pDestTemp, PSFTEMPLATEx pSrcTemp) {
  if (NULL == m_pBack)
    return E_FAIL;

  ISfFileIOTemplate* pITemplate = NULL;
  HRESULT hr = m_pBack->QueryInterface(IID_ISfFileIOTemplate, (void**)&pITemplate);
  if (SUCCEEDED(hr)) {
    hr = pITemplate->CopyTemplate(pDestTemp, pSrcTemp);
    pITemplate->Release();
  }
  return hr;
}

STDMETHODIMP VegasFS::SetActiveTemplateInfo(PSFTEMPLATEx pTemplate) {
  HRESULT hr = copyTemplate(m_pActiveTemplate, pTemplate);

  if (SUCCEEDED(hr))
    m_fActiveTemplateDirty = TRUE;

  return hr;
}

PSFTEMPLATEx VegasFS::allocTemplate() {
  if (NULL == m_pBack)
    return NULL;

  PSFTEMPLATEx pTemplate = NULL;

  ISfFileIOTemplate* pITemplate = NULL;
  HRESULT hr = m_pBack->QueryInterface(IID_ISfFileIOTemplate, (void**)&pITemplate);
  if (SUCCEEDED(hr)) {
    hr = pITemplate->AllocTemplate(&pTemplate);
    pITemplate->Release();
  }
  return pTemplate;
}

void VegasFS::freeTemplate(PSFTEMPLATEx pTemplate) {
  if (NULL == m_pBack)
    return;
  if (NULL == pTemplate)
    return;
  ISfFileIOTemplate* pITemplate = NULL;
  HRESULT hr = m_pBack->QueryInterface(IID_ISfFileIOTemplate, (void**)&pITemplate);
  if (SUCCEEDED(hr)) {
    hr = pITemplate->FreeTemplate(pTemplate);
    pITemplate->Release();
  }
}
