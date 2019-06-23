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
#include "SfMeta.h"

class VegasFS : public ISfRenderFileClass,
                public ISfRenderMetaSupport2 {
public:
  VegasFS();
  ~VegasFS();

  STDMETHOD(QueryInterface) (REFIID riid, void** ppvObj);
  STDMETHOD_(ULONG, AddRef) ();
  STDMETHOD_(ULONG, Release) ();

  // ISfRenderFileClass
  STDMETHOD(SetTypeIndex) (LONG ixType);
  STDMETHOD(GetTypeIndex) (LONG * pixType);
  STDMETHOD(SetIOManager) (ISfFileIOManager * pBack);
  STDMETHOD(GetIOManager) (ISfFileIOManager * *ppBack);
  STDMETHOD(GetTypeID) (GUID * puuid);
  STDMETHOD(GetAllowedStreamTypes) (DWORD * fdwStreamTypes);
  STDMETHOD(GetCaps) (SFFILERENDERCAPS * pCaps);
  STDMETHOD(GetReadClass) (ISfReadFileClass * *ppReadClass);
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
  STDMETHOD(GetPropertyPageCount) (LONG * pcFreePages, LONG * pcPagesForSale,
      SFFILERENDERPAGEOPTIONS * prPageOptions);
  STDMETHOD(GetPropertyPage) (LONG ixIndex, IPropertyPage * *ppPropPage);

  // START: ISfRenderMetaSupport
  STDMETHOD(Reset)();
  STDMETHOD(GetNextSumTag)(FOURCC* fcc, 
                           DWORD* pcchLen);
   // START: ISfRenderMetaSupport2
  STDMETHOD(GetSupportedTypes)(SFFIO_TYPESOFMETADATA* peEmbedded,
                               SFFIO_TYPESOFMETADATA* peExternal);

public:
  static const GUID& ms_uidReader;
  static const GUID& ms_uidRenderer;

private:
  STDMETHOD(AllocateTemplate) (PSFTEMPLATEx * ppTemplate, DWORD cbSizeExtra,
      LPCTSTR pszName, LPCTSTR pszNotes);

  DWORD m_dwRef;
  LONG m_ixType;
  ISfFileIOManager* m_pFileIOManager;
  static DWORD ms_dwStreamTypes;
};

#endif  // VEGASV2_VEGASFS_H
