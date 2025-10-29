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

#ifndef VEGASV2_VEGASFS_H
#define VEGASV2_VEGASFS_H

#include "SfRenderFileClass.h"
#if !defined(VEGAS_SDK_FROM_V5)
#include "SfMeta.h"
#endif

#define DEBUG_PRINT_FUNCTION_NAME OutputDebugString(L"DMFS: " __FUNCTIONW__ L"\n");

class VegasFS : public ISfRenderFileClass
#if !defined(VEGAS_SDK_FROM_V5)
  , public ISfRenderMetaSupport2
#endif
{
public:
  VegasFS();
  ~VegasFS();

  STDMETHOD(QueryInterface) (REFIID riid, void** ppvObj);
  STDMETHOD_(ULONG, AddRef) ();
  STDMETHOD_(ULONG, Release) ();

  // ISfRenderFileClass
#if defined(VEGAS_SDK_FROM_V5)
  STDMETHOD(SetHostInterface)(IUnknown* pHost);
  STDMETHOD(GetReadClassGuid)(GUID* pguidReadClass);
#endif

#if !defined(VEGAS_SDK_FROM_V5)
  STDMETHOD(SetTypeIndex) (LONG ixType);
  STDMETHOD(GetTypeIndex) (LONG * pixType);
  STDMETHOD(SetIOManager) (ISfFileIOManager * pBack);
  STDMETHOD(GetIOManager) (ISfFileIOManager * *ppBack);
#endif
  STDMETHOD(GetTypeID) (GUID * puuid);
  STDMETHOD(GetAllowedStreamTypes) (DWORD * fdwStreamTypes);
  STDMETHOD(GetCaps) (SFFILERENDERCAPS * pCaps);
#if !defined(VEGAS_SDK_FROM_V5)
  STDMETHOD(GetReadClass) (ISfReadFileClass * *ppReadClass);
#endif
  STDMETHOD(BuyPropertyPages) (HWND hwndParent);
  STDMETHOD(HasAboutBox) ();
  STDMETHOD(ShowAboutBox) (HWND hwndParent);
  STDMETHOD(GetStatus) (HRESULT * phrStatus, BSTR * bstrStatus);
  STDMETHOD(GetStrings) (const SFFIO_FILECLASS_INFO_TEXT * paStringIds,
      ULONG cStrings, BSTR * abstrStrings);
  STDMETHOD(Open) (ISfRenderFile * pSfFile, HANDLE hfile, LPCWSTR pszFilename);
  STDMETHOD(Create) (ISfRenderFile * *pSfFile, LPCWSTR pszFilename, LONG cStreams,
      ISfReadStream * *paStreams, PCSFTEMPLATEx pTemplate);
  STDMETHOD(GetPresetCount) (LONG * pcPresets);
  STDMETHOD(GetPreset) (PSFTEMPLATEx * pptpl, LONG ixPreset);
  STDMETHOD(GetPresetIndexFromGuid) (const GUID * pGuid, LONG * pixPreset);
  STDMETHOD(ChooseBestPreset) (PCSFTEMPLATEx ptplToMatch, LONG eMatchRules,
      LONG * pixPreset);
  STDMETHOD(ConformTemplate) (PSFTEMPLATEx * pptpl, PCSFTEMPLATEx ptplSrc,
      SFFIO_CONFORM_TEMPLATE_REASON eConform);
  STDMETHOD(ConformTemplateToSource) (PSFTEMPLATEx * pptpl, PCSFTEMPLATEx ptplBase,
      PCSFTEMPLATEx ptplSource, SFFIO_CONFORM_TEMPLATE_REASON eConform);
  STDMETHOD(GetTemplateInfo) (PCSFTEMPLATEx pTemplate, LPVOID pvInfo, UINT cbInfo,
      SFFIO_TEMPLATE_INFO eInfoType);
#if !defined(VEGAS_SDK_FROM_V5)
#if defined(VEGAS_SDK_FROM_V3)
  STDMETHOD(GetTemplateInfoText) (PCSFTEMPLATEx pTemplate, LPWSTR pszwInfo, UINT cchInfo,
      SFFIO_TEMPLATE_INFOTEXT eInfoType);
#endif
#endif
  STDMETHOD(GetPropertyPageCount) (LONG * pcFreePages, LONG * pcPagesForSale,
      SFFILERENDERPAGEOPTIONS * prPageOptions);
  STDMETHOD(GetPropertyPage) (LONG ixIndex, IPropertyPage * *ppPropPage);

#if !defined(VEGAS_SDK_FROM_V5)
  // START: ISfRenderMetaSupport
  STDMETHOD(Reset)();
  STDMETHOD(GetNextSumTag)(FOURCC* fcc, 
                           DWORD* pcchLen);
   // START: ISfRenderMetaSupport2
  STDMETHOD(GetSupportedTypes)(SFFIO_TYPESOFMETADATA* peEmbedded,
                               SFFIO_TYPESOFMETADATA* peExternal);
#endif

public:
  static const GUID& ms_uidRenderer;

private:
  STDMETHOD(AllocateTemplate) (PSFTEMPLATEx * ppTemplate, DWORD cbSizeExtra,
      LPCTSTR pszName, LPCTSTR pszNotes);

  DWORD m_dwRef;
  LONG m_ixType;
#if !defined(VEGAS_SDK_FROM_V5)
  ISfFileIOManager* m_pFileIOManager;
#endif
  static DWORD ms_dwStreamTypes;
  bool m_bAllocatorAcquired = false;
};

#endif  // VEGASV2_VEGASFS_H
