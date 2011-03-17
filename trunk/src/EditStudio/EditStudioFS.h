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
#ifndef EDITSTUDIO_EDITSTUDIOFS_H
#define EDITSTUDIO_EDITSTUDIOFS_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "fscommon.h"

class CWnd {};

#include "PM_Consts.h"
#include "ES_Plugin/ES_Plugin.h"
#include "ES_Plugin/ES_PluginVideo.h"

enum {
  EDSTPLUGIN_DEFAULT_PRESET,
  EDSTPLUGIN_NUM_PRESETS
};

// Function prototypes. Wrapped in "extern "C"" to avoid any name
// mangling and ensure that the functions are correctly exported in
// the DLL

extern "C" {
BOOL __cdecl ES_PluginAttribute(void* pPluginData,
                                INT32 nAttribute, INT32 nParam1, INT32 nParam2);
BOOL __cdecl ES_PluginStartup(void** pPluginData, ES_CallbackGetCallbackFunc pES_CallbackGetCallback);
void __cdecl ES_PluginClosedown(void* pPluginData);
BOOL __cdecl ES_VideoImportVideoRecognise(void* pPluginData, UINT8* pMagicData,
                                          INT32 nSizeofMagicData, LPCSTR pFilename);
BOOL __cdecl ES_VideoImportGetFrameOpen(void* pPluginData, LPCSTR pFilename,
                                        void* pMediaFileHandle);
void __cdecl ES_VideoImportGetFrameClose(void* pPluginData, void* pMediaFileHandle);
BOOL __cdecl ES_VideoImportVideoDetails(void* pPluginData, void* pMediaFileHandle,
                                        TES_PluginVideoMediaDetails1* pMediaDetails);
BOOL __cdecl ES_VideoImportGetFrame(void* pPluginDataIn, void* pMediaFileHandleIn, TES_PluginVideoImportGetFrame2* pGetFrame);
void __cdecl ES_VideoShowVideoImportPropertiesDlg(void* pPluginData,
                                                  TES_PluginVideoShowImportPropertiesDlg1* pPluginVideoShowImportProperties);
void __cdecl ES_VideoShowVideoExportPropertiesDlg(void* pPluginData,
                                                  TES_PluginVideoShowExportPropertiesDlg1* pPluginVideoShowExportProperties);
void __cdecl ES_VideoShowAudioImportPropertiesDlg(void* pPluginData,
                                                  TES_PluginVideoShowImportPropertiesDlg1* pPluginVideoShowImportProperties);
void __cdecl ES_VideoShowAudioExportPropertiesDlg(void* pPluginData,
                                                  TES_PluginVideoShowExportPropertiesDlg1* pPluginVideoShowExportProperties);
BOOL __cdecl ES_VideoXMLPropertiesRead(void* pPluginData, void* pFile, INT32 nUngetc,
                                       TES_PluginVideoXMLPropertiesRead1* pPluginVideoXMLPropertiesRead);
BOOL __cdecl ES_VideoImportGetSampleDataOpen(void* pPluginData, LPCSTR pFilename, void* pMediaFileHandle);
void __cdecl ES_VideoImportGetSampleDataClose(void* pPluginData, void* pMediaFileHandle);
BOOL __cdecl ES_VideoImportGetSampleDataSeek(void* pPluginDataIn, void* pMediaFileHandleIn,
                                             INT32 nSampleNum);
BOOL __cdecl ES_VideoImportGetSampleData(void* pPluginDataIn, void* pMediaFileHandleIn,
                                         TPM_AudioSample16S* pAudioSamples, INT32* pStartSampleNum, INT32* pNumSamples);
void __cdecl ES_VideoExport(void* pPluginData, TES_PluginVideoExport1* pPluginVideoExport,
                            LPCSTR pFilename, char* pErrorMsg);
};

void AttributeQuery(INT32 nParam1, INT32 nParam2);
void AttributeExportDesc(INT32 nParam1, INT32 nParam2);
void AttributeGetPresetInfo(INT32 nParam1, INT32 nParam2);
void AttributeSetPresetID(void* pPluginData, INT32 nParam1, INT32 nParam2);
void AttributeGetExportFilename(INT32 nParam1, INT32 nParam2);

typedef struct {
  TES_CallbackFuncs m_tCallbackFuncs;
} TPluginData;

class CEditStudioFS : public FrameServerImpl {
public:
  INT32 markInMs;
  TES_PluginVideoExport2* pExport;
  TPluginData* pPluginData;

public:
  void OnVideoRequest();
  void OnAudioRequest();
  bool OnAudioRequestOneSecond(DWORD second, LPBYTE* data, DWORD* datalen);
};

// ///////////////////////////////////////////////////////////////////////////

#endif  // EDITSTUDIO_EDITSTUDIO_FS_H
