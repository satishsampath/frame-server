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
#include "EditStudioFS.h"

HINSTANCE ghInst;

BOOL APIENTRY DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID pReserved) {
  switch (dwReason) {
  case DLL_PROCESS_ATTACH:
    ghInst = hinst;
    break;
  }
  return TRUE;
}

BOOL CallbackGetFuncs(ES_CallbackGetCallbackFunc pcES, TES_CallbackFuncs* pc) {
  pc->m_pES_CallbackGetCallback = pcES;
  pc->m_pES_CallbackGetLayerNameFromGuid =
    (ES_CallbackGetLayerNameFromGuidFunc)pcES("ES_CallbackGetLayerNameFromGuid");
  pc->m_pES_CallbackGetLayerFromStartEndLayerItem =
    (ES_CallbackGetLayerFromStartEndLayerItemFunc)pcES("ES_CallbackGetLayerFromStartEndLayerItem");
  pc->m_pES_CallbackGetIsStoryboardTransition =
    (ES_CallbackGetIsStoryboardTransitionFunc)pcES("ES_CallbackGetIsStoryboardTransition");
  pc->m_pES_CallbackGetPluginEffectProperties =
    (ES_CallbackGetPluginEffectPropertiesFunc)pcES("ES_CallbackGetPluginEffectProperties");
  pc->m_pES_CallbackPluginPropvalsChanged =
    (ES_CallbackPluginPropvalsChangedFunc)pcES("ES_CallbackPluginPropvalsChanged");
  pc->m_pES_CallbackGetAvailableSourceLayers =
    (ES_CallbackGetAvailableSourceLayersFunc)pcES("ES_CallbackGetAvailableSourceLayers");
  pc->m_pES_CallbackFreeAvailableSourceLayers =
    (ES_CallbackFreeAvailableSourceLayersFunc)pcES("ES_CallbackFreeAvailableSourceLayers");
  pc->m_pES_CallbackGetProgInfo =
    (ES_CallbackGetProgInfoFunc)pcES("ES_CallbackGetProgInfo");
  pc->m_pES_CallbackSetPropvalValue =
    (ES_CallbackSetPropvalValueFunc)pcES("ES_CallbackSetPropvalValue");
  pc->m_pES_CallbackRenderFrameUptoLayer =
    (ES_CallbackRenderFrameUptoLayerFunc)pcES("ES_CallbackRenderFrameUptoLayer");
  pc->m_pES_CallbackGetPropvalValue =
    (ES_CallbackGetPropvalValueFunc)pcES("ES_CallbackGetPropvalValue");
  pc->m_pES_CallbackSetPropvalStartEnd =
    (ES_CallbackSetPropvalStartEndFunc)pcES("ES_CallbackSetPropvalStartEnd");
  pc->m_pES_CallbackCopybitsWithPlacement =
    (ES_CallbackCopybitsWithPlacementFunc)pcES("ES_CallbackCopybitsWithPlacement");
  pc->m_pES_CallbackRenderFrame =
    (ES_CallbackRenderFrameFunc)pcES("ES_CallbackRenderFrame");
  pc->m_pES_CallbackRenderAudio =
    (ES_CallbackRenderAudioFunc)pcES("ES_CallbackRenderAudio");
  pc->m_pES_CallbackGetSelectedItemInfo =
    (ES_CallbackGetSelectedItemInfoFunc)pcES("ES_CallbackGetSelectedItemInfo");
  pc->m_pES_CallbackGetPluginVersions =
    (ES_CallbackGetPluginVersionsFunc)pcES("ES_CallbackGetPluginVersions");
  pc->m_pES_CallbackRenderAudioUptoLayer =
    (ES_CallbackRenderAudioUptoLayerFunc)pcES("ES_CallbackRenderAudioUptoLayer");

  if (!pc->m_pES_CallbackGetLayerNameFromGuid || !pc->m_pES_CallbackGetLayerFromStartEndLayerItem ||
      !pc->m_pES_CallbackGetIsStoryboardTransition || !pc->m_pES_CallbackGetPluginEffectProperties ||
      !pc->m_pES_CallbackPluginPropvalsChanged || !pc->m_pES_CallbackGetAvailableSourceLayers ||
      !pc->m_pES_CallbackFreeAvailableSourceLayers || !pc->m_pES_CallbackGetProgInfo ||
      !pc->m_pES_CallbackSetPropvalValue || !pc->m_pES_CallbackRenderFrameUptoLayer ||
      !pc->m_pES_CallbackGetPropvalValue || !pc->m_pES_CallbackSetPropvalStartEnd ||
      !pc->m_pES_CallbackCopybitsWithPlacement || !pc->m_pES_CallbackRenderFrame ||
      !pc->m_pES_CallbackRenderAudio || !pc->m_pES_CallbackGetSelectedItemInfo ||
      !pc->m_pES_CallbackGetPluginVersions || !pc->m_pES_CallbackRenderAudioUptoLayer)
    return FALSE;
  return TRUE;
}

BOOL __cdecl ES_PluginStartup(void** ppPluginData, ES_CallbackGetCallbackFunc pES_CallbackGetCallback) {
  *ppPluginData = NULL;
  TPluginData* pPluginData = new TPluginData;
  if (pPluginData) {
    memset(pPluginData, 0, sizeof(TPluginData));
    CallbackGetFuncs(pES_CallbackGetCallback, &pPluginData->m_tCallbackFuncs);
    *ppPluginData = pPluginData;
    return TRUE;
  }
  return FALSE;
}

void __cdecl ES_PluginClosedown(void* pPluginDataIn) {
  delete (TPluginData*)pPluginDataIn;
}

BOOL __cdecl ES_PluginAttribute(void* pPluginData,
    INT32 nAttribute, INT32 nParam1, INT32 nParam2) {
  BOOL bRecognised = TRUE;

  switch (nAttribute) {
  case ES_PLUGIN_ATTRIBUTE_PLUGINTYPE:
    *(INT32*)nParam1 = ES_PLUGIN_ATTRIBUTE_PLUGINTYPE_VIDEO;
    break;
  case ES_PLUGINVIDEO_ATTRIBUTE_QUERY:
  {
    TES_PluginVideoAttributeQuery1* pVideoQuery = (TES_PluginVideoAttributeQuery1*)nParam1;
    GUID myGuid = { 0x523BDE95, 0x2A2C, 0x4eb2, 0x8F, 0x36, 0xF9, 0x37, 0xA2, 0xA4, 0xFF, 0xF8 };
    pVideoQuery->m_nFlags = ES_PLUGINVIDEO_QUERY_CANEXPORTVIDEO | ES_PLUGINVIDEO_QUERY_CANEXPORTAUDIO;
    strcpy(pVideoQuery->m_pszShortName, "Debugmode FrameServer");
    strcpy(pVideoQuery->m_pszCopyright, "(c) 2000-2004 Satish Kumar. S");
    strcpy(pVideoQuery->m_pszVersion, "");
    strcpy(pVideoQuery->m_pszVideoFileDialogExport, "AVI files (*.avi)|*.avi|");
    pVideoQuery->m_nMediaFileHandleMemSize = 0;
    pVideoQuery->m_tGuid = myGuid;
  }
  break;
  case ES_PLUGINVIDEO_ATTRIBUTE_EXPORTDESC:
    strcpy((char*)nParam1, "FrameServer plugin from <http://www.debugmode.com>, creates a 'dummy' AVI file "
        "that you can open in other video apps without the need to fully render the project.");
    break;
  case ES_PLUGINVIDEO_ATTRIBUTE_GETPRESETCOUNT:
    *((INT32*)nParam1) = EDSTPLUGIN_NUM_PRESETS;
    break;
  case ES_PLUGINVIDEO_ATTRIBUTE_GETPRESETINFO:
  {
    TES_PluginVideoPresetInfo1* pPresetInfo = (TES_PluginVideoPresetInfo1*)nParam2;
    pPresetInfo->m_nFlags = ES_PLUGINVIDEO_PRESETINFO_SETEXPORTAUDIO | ES_PLUGINVIDEO_PRESETINFO_SETEXPORTVIDEO;
    pPresetInfo->m_bExportAudio = pPresetInfo->m_bExportVideo = TRUE;
    strcpy(pPresetInfo->m_pszShortName, "Debugmode FrameServer (Project settings)");
    pPresetInfo->m_pszAdditionalDesc[0] = 0;
  }
  break;
  case ES_PLUGINVIDEO_ATTRIBUTE_SETPRESETID:
    break;
  case ES_PLUGINVIDEO_ATTRIBUTE_GETEXPORTFILENAME:
  {
    char* str = (char*)nParam1;
    if (strlen(str) > 4 && stricmp(str + strlen(str) - 4, ".avi") != 0)
      strcat(str, ".avi");
  }
  break;
  case ES_PLUGINVIDEO_ATTRIBUTE_GETIMPORTPRIORITYVIDEO:
    *((INT32*)nParam1) = -100;             // Lower than VideoDirectShow, which should be used by default
    break;
  default:
    bRecognised = FALSE;
    break;
  }

  return bRecognised;
}

void __cdecl ES_VideoExport(void* pPluginDataIn, TES_PluginVideoExport1* pExport,
    LPCSTR pFilename, char* pErrorMsg) {
  CEditStudioFS fs;
  char sFilename[MAX_PATH];

  strcpy(sFilename, pFilename);
  if (!pExport->m_bExportVideo) {
    strcpy(pErrorMsg, "Video must be exported!");
    return;
  }

  if (strlen(sFilename) > 4 && stricmp(sFilename + strlen(sFilename) - 4, ".avi") != 0)
    strcat(sFilename, ".avi");

  fs.markInMs = pExport->m_nMarkInThouSecs;
  fs.pPluginData = (TPluginData*)pPluginDataIn;
  fs.pExport = pExport;
  fs.Init(!!pExport->m_bExportAudio, pExport->m_nSamplesPerSec, 16, 2,
      (DWORD)(((double)pExport->m_nMarkOutThouSecs - (double)pExport->m_nMarkInThouSecs) / 1000.0 * pExport->m_fFps),
      pExport->m_fFps, pExport->m_pBitmapInfo->bmiHeader.biWidth, pExport->m_pBitmapInfo->bmiHeader.biHeight,
      NULL,  // pExport->m_pParentWnd->GetSafeHwnd(),
      sFilename);
  fs.Run();
  strcpy(pErrorMsg, "stopped");
}

void CEditStudioFS::OnVideoRequest() {
  INT32 srcFrameMs = markInMs + (DWORD)((vars->videoFrameIndex * 1000) / fps) + 1;

  pPluginData->m_tCallbackFuncs.m_pES_CallbackRenderFrame(pExport, srcFrameMs);
  ConvertVideoFrame((LPBYTE)pExport->m_pBitmapInfo + pExport->m_pBitmapInfo->bmiHeader.biSize,
      (pExport->m_pBitmapInfo->bmiHeader.biSizeImage / pExport->m_pBitmapInfo->bmiHeader.biHeight), vars);
}

void CEditStudioFS::OnAudioRequest() {
  INT32 srcFrameMs = markInMs + vars->audioFrameIndex * 10; // each audio frame = 10ms of audio (1s = 100 audio frames)
  TPM_AudioSample16S* prevAudioSamples = pExport->m_pAudioSamples;
  INT32 prevNumSamples = pExport->m_nNumSamples;

  pExport->m_pAudioSamples = (TPM_AudioSample16S*)(((LPBYTE)vars) + vars->audiooffset);
  pExport->m_nNumSamples = audioSamplingRate / 100;
  pPluginData->m_tCallbackFuncs.m_pES_CallbackRenderAudio(pExport, srcFrameMs);
  pExport->m_pAudioSamples = prevAudioSamples;
  pExport->m_nNumSamples = prevNumSamples;
  vars->audioBytesRead = audioSamplingRate / 100 * 4;
}

bool CEditStudioFS::OnAudioRequestOneSecond(DWORD second, LPBYTE* data, DWORD* datalen) {
  INT32 srcFrameMs = markInMs + second * 1000;
  TPM_AudioSample16S* prevAudioSamples = pExport->m_pAudioSamples;
  INT32 prevNumSamples = pExport->m_nNumSamples;

  *datalen = audioSamplingRate * 4;
  if (!*data)
    *data = (unsigned char*)malloc(*datalen);

  pExport->m_pAudioSamples = (TPM_AudioSample16S*)*data;
  pExport->m_nNumSamples = audioSamplingRate;
  pPluginData->m_tCallbackFuncs.m_pES_CallbackRenderAudio(pExport, srcFrameMs);
  pExport->m_pAudioSamples = prevAudioSamples;
  pExport->m_nNumSamples = prevNumSamples;
  return true;
}

BOOL __cdecl ES_VideoImportVideoRecognise(void* pPluginDataIn, UINT8* pMagicData,
    INT32 nSizeofMagicData, LPCSTR pFilename) {
  return FALSE;
}

BOOL __cdecl ES_VideoImportAudioRecognise(void* pPluginDataIn, UINT8* pMagicData,
    INT32 nSizeofMagicData, LPCSTR pFilename) {
  return FALSE;
}

BOOL __cdecl ES_VideoImportGetFrameOpen(void* pPluginDataIn, LPCSTR pFilename,
    void* pMediaFileHandleIn) {
  return FALSE;
}

void __cdecl ES_VideoImportGetFrameClose(void* pPluginData, void* pMediaFileHandleIn) {
}

BOOL __cdecl ES_VideoImportVideoDetails(void* pPluginData, void* pMediaFileHandleIn,
    TES_PluginVideoMediaDetails1* pMediaDetails) {
  return FALSE;
}

BOOL __cdecl ES_VideoImportAudioDetails(void* pPluginDataIn, void* pMediaFileHandleIn,
    TES_PluginAudioMediaDetails1* pMediaDetails) {
  return FALSE;
}

BOOL __cdecl ES_VideoImportGetFrame(void* pPluginDataIn, void* pMediaFileHandleIn,
    TES_PluginVideoImportGetFrame1* pGetFrame) {
  return FALSE;
}

void __cdecl ES_VideoShowVideoImportPropertiesDlg(void* pPluginDataIn,
    TES_PluginVideoShowImportPropertiesDlg1* pPluginVideoShowImportProperties) {
}

void __cdecl ES_VideoShowAudioImportPropertiesDlg(void* pPluginDataIn,
    TES_PluginVideoShowImportPropertiesDlg1* pPluginVideoShowImportProperties) {
}

void __cdecl ES_VideoShowVideoExportPropertiesDlg(void* pPluginDataIn,
    TES_PluginVideoShowExportPropertiesDlg1* pShowExportProperties) {
}

void __cdecl ES_VideoShowAudioExportPropertiesDlg(void* pPluginDataIn,
    TES_PluginVideoShowExportPropertiesDlg1* pShowExportProperties) {
}

BOOL __cdecl ES_VideoImportGetSampleDataOpen(void* pPluginData, LPCSTR pFilename,
    void* pMediaFileHandleIn) {
  return FALSE;
}

void __cdecl ES_VideoImportGetSampleDataClose(void* pPluginData, void* pMediaFileHandleIn) {
}

BOOL __cdecl ES_VideoImportGetSampleDataSeek(void* pPluginDataIn, void* pMediaFileHandleIn,
    INT32 nSampleNum) {
  return FALSE;
}

BOOL __cdecl ES_VideoImportGetSampleData(void* pPluginDataIn, void* pMediaFileHandleIn,
    TPM_AudioSample16S* pAudioSamples, INT32* pStartSampleNum, INT32* pNumSamples) {
  return FALSE;
}

BOOL __cdecl ES_VideoXMLPropertiesRead(void* pPluginDataIn, void* pFile, INT32 nUngetc,
    TES_PluginVideoXMLPropertiesRead1* pPluginVideoXMLPropertiesRead) {
  return FALSE;
}
