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

#ifndef VEGAS_VEGASFS_H
#define VEGAS_VEGASFS_H

#define APPNAME "DebugMode FrameServer"
#define LABOUTSTR L"DebugMode FrameServer"

class VegasFS : public ISfRenderFileClass,
  public ISfFIO_TemplateInfo_Renderer,
  public ISfFIO_PrivateCopyPreset,
  public ISfRenderMetaSupport2 {
public:
  VegasFS();
  virtual ~VegasFS() ;

  STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppvObj);
  STDMETHODIMP_(ULONG) AddRef();
  STDMETHODIMP_(ULONG) Release();

  STDMETHODIMP SetIOManager(ISfFileIOManager* pBack);
  STDMETHODIMP GetIOManager(ISfFileIOManager** ppBack);

  virtual STDMETHODIMP SetTypeIndex(LONG ixType);
  virtual STDMETHODIMP_(LONG) GetTypeIndex();
  virtual STDMETHODIMP GetTypeID(GUID* puuid);
  virtual STDMETHODIMP GetAllowedStreamTypes(DWORD* fdwStreamTypes);
  virtual STDMETHODIMP GetCaps(PSFFILERENDERCAPS pCaps);
  virtual STDMETHODIMP GetStrings(LPWSTR pszFileSaveDialogString,
                                  UINT cchFileSaveDialogString,
                                  LPWSTR pszFileExtensions,
                                  UINT cchFileExtensions,
                                  LPWSTR pszFileTypeName,
                                  UINT cchFileTypeName);
  virtual STDMETHODIMP GetReadClass(ISfReadFileClass** ppReadClass)
  { return E_NOINTERFACE; }
  virtual STDMETHODIMP Open(ISfRenderFile* pSfFile, HANDLE hfile, LPCWSTR pszFilename)
  { return E_NOTIMPL; }
  virtual STDMETHODIMP Create(ISfRenderFile** pSfFile, LPCWSTR pszFilename,
                              LONG cStreams, ISfReadStream** paStreams);

  virtual STDMETHODIMP GetPresetCount(long* pcPresets);
  virtual STDMETHODIMP GetPresetName(int ixPreset, LPWSTR pszName, UINT cchName);
  virtual STDMETHODIMP GetPresetDescription(int ixPreset, LPWSTR pszDesc, UINT cchDesc);
  virtual STDMETHODIMP UsePreset(long ixPreset);
  virtual STDMETHODIMP GetPresetVersion(long ixPreset, LPDWORD pdwVersion);

  STDMETHODIMP SaveSettings(LPSTREAM pStrm, BOOL fClearDirty);
  STDMETHODIMP LoadSettings(LPSTREAM pStrm);
  STDMETHODIMP EditSettings(HWND hwndParent, LPDWORD pdwTemplateIndex)
  { return E_NOTIMPL; }
  STDMETHODIMP IsActiveTemplateDirty()
  { return (m_fActiveTemplateDirty ? S_OK : S_FALSE); }

  STDMETHODIMP InitDefaultTemplate(PSFTEMPLATEx pDefaultTemplate);
  STDMETHODIMP GetActiveTemplateInfo(PSFTEMPLATEx* ppTemplate);
  STDMETHODIMP SetActiveTemplateInfo(PSFTEMPLATEx pTemplate);
  STDMETHODIMP FreeActiveTemplateInfo(PSFTEMPLATEx pTemplate);

  STDMETHODIMP_(LONG) GetPropertyPageCount() { return 0; }
  STDMETHODIMP GetPropertyPage(LONG lIndex, void** ppPropPage) { return E_NOINTERFACE; }

  STDMETHODIMP GetCopyright(LPCWSTR pszwCopyright, LPDWORD pcchCopyright)
  { return E_NOTIMPL; }

  STDMETHODIMP HasAboutBox(void) { return S_OK; }
  STDMETHODIMP ShowAboutBox(HWND hwndParent);

  // ISfFIO_PrivateCopyPreset
  STDMETHODIMP CopyPreset(PSFTEMPLATEx pTemplate, long ixPreset);

  // ISfFIO_TemplateInfo_Renderer
  STDMETHODIMP GetTemplateInfoText(PSFTEMPLATEx pTemplate, LPWSTR pszInfo, UINT cchInfo,
                                   SFFIO_TEMPLATE_INFOTEXT eInfoType);
  STDMETHODIMP GetTemplateInfo(PSFTEMPLATEx pTemplate, LPVOID pvInfo, UINT cbInfo,
                               SFFIO_TEMPLATE_INFO eInfoType);

  // ISfRenderMetaSupport
  STDMETHODIMP Reset() { return NOERROR; }
  STDMETHODIMP GetNextSumTag(FOURCC* pfcc, DWORD* pcchLen) { return S_FALSE; }

  // ISfRenderMetaSupport2
  STDMETHODIMP GetSupportedTypes(SFFIO_TYPESOFMETADATA* peEmbedded,
                                 SFFIO_TYPESOFMETADATA* peExternal);

  // start of public methods that are not part of the COM interface
  STDMETHODIMP initObj(void);
  HRESULT copyTemplate(PSFTEMPLATEx pDestTemp, PSFTEMPLATEx pSrcTemp);
  PSFTEMPLATEx allocTemplate(void);
  void freeTemplate(PSFTEMPLATEx pTemplate);

private:
  DWORD m_dwRef;
  ISfFileIOManager* m_pBack;
  LONG m_ixType;
  PSFTEMPLATEx m_pActiveTemplate;
  BOOL m_fActiveTemplateDirty;
  PSFTEMPLATEx m_pDefaultTemplate;

  static GUID ms_TypeGUID;
  static DWORD ms_dwStreamTypes;
  static SFFILERENDERCAPS ms_TypeCapabilities;
  static DWORD ms_dwTemplateVer;
  static SFTEMPLATEx ms_ClassDefaultTemplate;
  static SFAUDIOTEMPLATEx ms_ClassDefaultAudioTemplate;
  static SFVIDEOTEMPLATEx ms_ClassDefaultVideoTemplate;
};

#endif  // VEGAS_VEGASFS_H
