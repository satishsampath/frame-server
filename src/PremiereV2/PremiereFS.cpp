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

#include <PrSDKEntry.h>
#include <PrSDKExport.h>
#include <PrSDKMalErrors.h>
#include <PrSDKExportParamSuite.h>
#include <PrSDKExportInfoSuite.h>
#include <PrSDKExportFileSuite.h>
#include <PrSDKMemoryManagerSuite.h>
#include <PrSDKExportProgressSuite.h>
#include <PrSDKSequenceRenderSuite.h>
#include <PrSDKSequenceAudioSuite.h>
#include <PrSDKTimeSuite.h>
#include <PrSDKAudioSuite.h>
#include <PrSDKPPixSuite.h>
#include "fscommon.h"

HINSTANCE ghInst;

#define APPNAME L"DebugMode FrameServer"

// Define a placement new operator to use c++ objects with premiere's NewPtr allocator.
void* operator new(size_t cbSize, void* pv) {
  return pv;
}
void operator delete(void*, void*) {  // dummy method to turn off warnings.
}

void debug(wchar_t* prefix, int value) {
  wchar_t msg[512];

  lstrcpy(msg, prefix);
  _itow_s(value, msg + lstrlen(prefix), 10, 10);
  OutputDebugString(msg);
}

class PremiereFSImpl : public FrameServerImpl {
public:
  PremiereFSImpl(SPBasicSuite* basicSuite);
  ~PremiereFSImpl();

  prMALError generateDefaultParams(exGenerateDefaultParamRec* rec);
  prMALError postProcessParams(exPostProcessParamsRec* rec);
  prMALError queryOutputSettings(exQueryOutputSettingsRec* rec);
  prMALError serve(exDoExportRec* rec);

  // Overrides
  void OnVideoRequest();
  void OnAudioRequest();
  bool OnAudioRequestOneSecond(DWORD second, LPBYTE* data, DWORD* datalen);

private:
  // Reads next N samples into audioFloatData and returns the number of samples read.
  int ReadAudioSamples(int numSamples, short* audioShortData);

private:
  SPBasicSuite* basicSuite;
  PrSDKTimeSuite* timeSuite;
  PrSDKExportFileSuite* exportFile;
  PrSDKExportParamSuite* exportParam;
  PrSDKExportInfoSuite* exportInfo;
  PrSDKSequenceRenderSuite* renderSuite;
  PrSDKSequenceAudioSuite* audioSuite;
  PrSDKPPixSuite* ppixSuite;
  PrSDKMemoryManagerSuite* memorySuite;
  long videoRenderId;
  long audioRenderId;
  exDoExportRec* exportRec;
  PrTime ticksPerFrame;
  PrPixelFormat pixelFormat[2];
  SequenceRender_ParamsRec renderParams;
  float* audioFloatData[2];
  unsigned long audioFloatDataPos;
};

PremiereFSImpl::PremiereFSImpl(SPBasicSuite* bs) {
  basicSuite = bs;
  basicSuite->AcquireSuite(kPrSDKTimeSuite, kPrSDKTimeSuiteVersion,
      const_cast<const void**>(reinterpret_cast<void**>(&timeSuite)));
  basicSuite->AcquireSuite(kPrSDKExportFileSuite, kPrSDKExportFileSuiteVersion,
      const_cast<const void**>(reinterpret_cast<void**>(&exportFile)));
  basicSuite->AcquireSuite(kPrSDKExportParamSuite, kPrSDKExportParamSuiteVersion,
      const_cast<const void**>(reinterpret_cast<void**>(&exportParam)));
  basicSuite->AcquireSuite(kPrSDKExportInfoSuite, kPrSDKExportInfoSuiteVersion,
      const_cast<const void**>(reinterpret_cast<void**>(&exportInfo)));
  basicSuite->AcquireSuite(kPrSDKSequenceRenderSuite, kPrSDKSequenceRenderSuiteVersion,
      const_cast<const void**>(reinterpret_cast<void**>(&renderSuite)));
  basicSuite->AcquireSuite(kPrSDKSequenceAudioSuite, kPrSDKSequenceAudioSuiteVersion,
      const_cast<const void**>(reinterpret_cast<void**>(&audioSuite)));
  basicSuite->AcquireSuite(kPrSDKPPixSuite, kPrSDKPPixSuiteVersion,
      const_cast<const void**>(reinterpret_cast<void**>(&ppixSuite)));
  basicSuite->AcquireSuite(kPrSDKMemoryManagerSuite, kPrSDKMemoryManagerSuiteVersion,
      const_cast<const void**>(reinterpret_cast<void**>(&memorySuite)));
  pixelFormat[0] = PrPixelFormat_VUYA_4444_8u;
  pixelFormat[1] = PrPixelFormat_BGRA_4444_8u;
}

PremiereFSImpl::~PremiereFSImpl() {
  if (!basicSuite)
    return;
  basicSuite->ReleaseSuite(kPrSDKTimeSuite, kPrSDKTimeSuiteVersion);
  basicSuite->ReleaseSuite(kPrSDKExportFileSuite, kPrSDKExportFileSuiteVersion);
  basicSuite->ReleaseSuite(kPrSDKExportParamSuite, kPrSDKExportParamSuiteVersion);
  basicSuite->ReleaseSuite(kPrSDKExportInfoSuite, kPrSDKExportInfoSuiteVersion);
  basicSuite->ReleaseSuite(kPrSDKSequenceRenderSuite, kPrSDKSequenceRenderSuiteVersion);
  basicSuite->ReleaseSuite(kPrSDKPPixSuite, kPrSDKPPixSuiteVersion);
  basicSuite->ReleaseSuite(kPrSDKMemoryManagerSuite, kPrSDKMemoryManagerSuiteVersion);
  basicSuite->ReleaseSuite(kPrSDKSequenceAudioSuite, kPrSDKSequenceAudioSuiteVersion);
  basicSuite = NULL;
}

prMALError PremiereFSImpl::generateDefaultParams(exGenerateDefaultParamRec* rec) {
  long pluginId = rec->exporterPluginID;
  PrParam param;

  exportInfo->GetExportSourceInfo(pluginId, kExportInfo_AudioSampleRate, &param);
  int rate = (int)(param.mFloat64 + 0.5);
  if (rate != 16000 && rate != 32000 && rate != 48000 && rate != 44100)
    param.mFloat64 = 44100.0;  // only the above list of sampling rates are supported.

  int groupIndex = 0;
  exportParam->AddMultiGroup(pluginId, &groupIndex);
  exportParam->AddParamGroup(pluginId, groupIndex, ADBETopParamGroup, ADBEAudioTabGroup,
      L"Audio", kPrFalse, kPrFalse, kPrFalse);

  exNewParamInfo sampleRateParam;
  sampleRateParam.structVersion = 1;
  strcpy_s(sampleRateParam.identifier, 256, ADBEAudioRatePerSecond);
  sampleRateParam.paramType = exParamType_float;
  sampleRateParam.flags = exParamFlag_none;
  exParamValues sampleRateValues;
  sampleRateValues.structVersion = 1;
  sampleRateValues.value.floatValue = param.mFloat64;
  sampleRateValues.disabled = kPrFalse;
  sampleRateValues.hidden = kPrFalse;
  sampleRateParam.paramValues = sampleRateValues;
  exportParam->AddParam(pluginId, groupIndex, ADBEAudioTabGroup, &sampleRateParam);

  exportParam->SetParamsVersion(pluginId, 1);

  return malNoError;
}

prMALError PremiereFSImpl::postProcessParams(exPostProcessParamsRec* rec) {
  long pluginId = rec->exporterPluginID;

  exportParam->SetParamName(pluginId, 0, ADBEAudioRatePerSecond, L"Sample Rate");
  exportParam->ClearConstrainedValues(pluginId, 0, ADBEAudioRatePerSecond);
  float sampleRates[] = { 16000.0f, 32000.0f, 44100.0f, 48000.0f };
  wchar_t* sampleRateStrings[] = { L"16000 Hz", L"32000 Hz", L"44100 Hz", L"48000 Hz" };
  for (int i = 0; i < sizeof(sampleRates) / sizeof(float); i++) {
    exOneParamValueRec value;
    value.floatValue = sampleRates[i];
    exportParam->AddConstrainedValuePair(pluginId, 0, ADBEAudioRatePerSecond, &value, sampleRateStrings[i]);
  }

  return malNoError;
}

prMALError PremiereFSImpl::queryOutputSettings(exQueryOutputSettingsRec* rec) {
  long pluginId = rec->exporterPluginID;

  if (rec->inExportVideo) {
    PrParam param;
    exportInfo->GetExportSourceInfo(pluginId, kExportInfo_VideoWidth, &param);
    rec->outVideoWidth = param.mInt32;
    exportInfo->GetExportSourceInfo(pluginId, kExportInfo_VideoHeight, &param);
    rec->outVideoHeight = param.mInt32;
    exportInfo->GetExportSourceInfo(pluginId, kExportInfo_VideoFrameRate, &param);
    rec->outVideoFrameRate = param.mInt64;
    exportInfo->GetExportSourceInfo(pluginId, kExportInfo_PixelAspectNumerator, &param);
    rec->outVideoAspectNum = param.mInt32;
    exportInfo->GetExportSourceInfo(pluginId, kExportInfo_PixelAspectDenominator, &param);
    rec->outVideoAspectDen = param.mInt32;
    rec->outVideoFieldType = prFieldsNone;
  }
  if (rec->inExportAudio) {
    exParamValues sampleRate;
    exportParam->GetParamValue(pluginId, 0, ADBEAudioRatePerSecond, &sampleRate);
    rec->outAudioSampleRate = sampleRate.value.floatValue;
    rec->outAudioSampleType = kPrAudioSampleType_32BitFloat;
    rec->outAudioChannelType = kPrAudioChannelType_Stereo;
  }

  return malNoError;
}

prMALError PremiereFSImpl::serve(exDoExportRec* rec) {
  if (rec->exportVideo == 0) {
    MessageBox(NULL, L"Frameserver cannot serve audio-only sequences.", APPNAME,
        MB_OK | MB_ICONEXCLAMATION);
    return malUnknownError;
  }

#define ReturnIfError(x) result = (x); if (result != 0) return malUnknownError

  prMALError returnCode = malUnknownError;
  prSuiteError result;

  TCHAR filename[MAX_PATH] = L"";
  long filenameLength = MAX_PATH;
  ReturnIfError(exportFile->GetPlatformPath(rec->fileObject, &filenameLength, filename));
  if (filename[0] == 0)
    return malUnknownError;
  if (filename[lstrlen(filename) - 1] == '.')
    filename[lstrlen(filename) - 1] = 0;
  if (lstrlen(filename) < 4 || lstrcmpi(filename + lstrlen(filename) - 4, L".avi") != 0)
    lstrcat(filename, L".avi");

  PrTime ticksPerSecond;
  timeSuite->GetTicksPerSecond(&ticksPerSecond);
  int pluginId = rec->exporterPluginID;
  PrParam param;
  exParamValues paramValue;

  ReturnIfError(exportInfo->GetExportSourceInfo(pluginId, kExportInfo_VideoWidth, &param));
  renderParams.inWidth = param.mInt32;
  ReturnIfError(exportInfo->GetExportSourceInfo(pluginId, kExportInfo_VideoHeight, &param));
  renderParams.inHeight = param.mInt32;
  ReturnIfError(exportInfo->GetExportSourceInfo(pluginId, kExportInfo_PixelAspectNumerator, &param));
  renderParams.inPixelAspectRatioNumerator = param.mInt32;
  ReturnIfError(exportInfo->GetExportSourceInfo(pluginId, kExportInfo_PixelAspectDenominator, &param));
  renderParams.inPixelAspectRatioDenominator = param.mInt32;
  ReturnIfError(exportInfo->GetExportSourceInfo(pluginId, kExportInfo_VideoFrameRate, &param));
  ticksPerFrame = param.mInt64;
  double fps = (double)ticksPerSecond / (double)ticksPerFrame;

  exportParam->GetParamValue(pluginId, 0, ADBEAudioRatePerSecond, &paramValue);
  int samplingRate = (int)(paramValue.value.floatValue + 0.5);

  renderParams.inRenderQuality = kPrRenderQuality_Max;
  renderParams.inFieldType = prFieldsNone;
  renderParams.inDeinterlace = 0;
  renderParams.inDeinterlaceQuality = kPrRenderQuality_Max;
  renderParams.inCompositeOnBlack = 0;

  audioFloatData[0] = reinterpret_cast<float*>(memorySuite->NewPtr(sizeof(float) * samplingRate));
  audioFloatData[1] = reinterpret_cast<float*>(memorySuite->NewPtr(sizeof(float) * samplingRate));

  ReturnIfError(renderSuite->MakeVideoRenderer(pluginId, &videoRenderId, ticksPerFrame));
  ReturnIfError(audioSuite->MakeAudioRenderer(pluginId, rec->startTime, kPrAudioChannelType_Stereo,
          kPrAudioSampleType_32BitFloat, static_cast<float>(samplingRate), &audioRenderId));

  bool doAudio = (rec->exportAudio != 0);
  DWORD numFrames = static_cast<DWORD>((rec->endTime - rec->startTime) / ticksPerFrame);
  exportRec = rec;
  Init(doAudio, samplingRate, 16, 2, numFrames, fps,
      renderParams.inWidth, renderParams.inHeight, GetActiveWindow(), filename);
  bool rval = Run();

  renderSuite->ReleaseVideoRenderer(pluginId, videoRenderId);
  audioSuite->ReleaseAudioRenderer(pluginId, audioRenderId);
  memorySuite->PrDisposePtr(reinterpret_cast<PrMemoryPtr>(audioFloatData[0]));
  memorySuite->PrDisposePtr(reinterpret_cast<PrMemoryPtr>(audioFloatData[1]));

  return (rval ? malNoError : exportReturn_Abort);
}

void PremiereFSImpl::OnVideoRequest() {
  // This is done here since the selection of YUY2 or RGBA happens in the frameserver dialog
  // after Run() is called above.
  renderParams.inRequestedPixelFormatArrayCount = (serveFormat == sfYUY2) ? 2 : 1;
  renderParams.inRequestedPixelFormatArray = pixelFormat + ((serveFormat == sfYUY2) ? 0 : 1);

  SequenceRender_GetFrameReturnRec renderResult;
  int result = renderSuite->RenderVideoFrame(videoRenderId,
      exportRec->startTime + ticksPerFrame * vars->videoFrameIndex,
      &renderParams, kRenderCacheType_None, &renderResult);
  if (result == suiteError_NoError) {
    char* pixels;
    long rowBytes;
    PrPixelFormat format;
    ppixSuite->GetPixelFormat(renderResult.outFrame, &format);
    ppixSuite->GetRowBytes(renderResult.outFrame, &rowBytes);
    ppixSuite->GetPixels(renderResult.outFrame, PrPPixBufferAccess_ReadOnly, &pixels);
    ConvertVideoFrame(pixels, rowBytes, vars, (format == PrPixelFormat_VUYA_4444_8u) ? idfAYUV : idfRGB32);
    ppixSuite->Dispose(renderResult.outFrame);
  } else {
    vars->videoBytesRead = vars->encBi.biWidth * vars->encBi.biHeight * vars->encBi.biBitCount / 8;
    memset(((LPBYTE)vars) + vars->videooffset, 0, vars->videoBytesRead);
  }
}

int PremiereFSImpl::ReadAudioSamples(int numSamples, short* audioShortData) {
  int samplesRead = 0;
  int result;

  // Keep reading until we get all required samples or an error happens.
  do {
    long maxBlip = 0;
    audioSuite->GetMaxBlip(audioRenderId, ticksPerFrame, &maxBlip);
    int samplesRemaining = numSamples - samplesRead;
    int samplesToRead = (maxBlip < samplesRemaining) ? maxBlip : samplesRemaining;
    float* buffers[2] = {
      audioFloatData[0] + samplesRead,
      audioFloatData[1] + samplesRead
    };
    result = audioSuite->GetAudio(audioRenderId, samplesToRead, buffers, kPrFalse);
    if (result == malNoError) {
      samplesRead += samplesToRead;
    }
  } while (result == malNoError && samplesRead < numSamples);

  // Convert to 16 bit samples.
  if (audioShortData) {
    for (int i = 0; i < samplesRead; i++) {
      audioShortData[i * 2 + 0] = static_cast<short>(audioFloatData[0][i] * 32767);
      audioShortData[i * 2 + 1] = static_cast<short>(audioFloatData[1][i] * 32767);
    }
    for (int i = samplesRead; i < numSamples; i++) {
      audioShortData[i * 2 + 0] = 0;
      audioShortData[i * 2 + 1] = 0;
    }
  }

  return samplesRead;
}

void PremiereFSImpl::OnAudioRequest() {
  int samplesToRead = audioSamplingRate / 100;
  unsigned long curpos = vars->audioFrameIndex * samplesToRead;

  if (hasAudio && audioFloatData[0]) {
    // If the caller wants to read audio before the current position, we have to restart
    // from the beginning since the API only allows sequential access.
    if (curpos < audioFloatDataPos) {
      audioFloatDataPos = 0;
      audioSuite->ResetAudioToBeginning(audioRenderId);
    }
    while (audioFloatDataPos < curpos) {
      int samplesRead = ReadAudioSamples(samplesToRead, NULL);  // read and ignore.
      if (!samplesRead) break;
      audioFloatDataPos += samplesRead;
    }

    // Read audio and convert to 16 bit samples.
    short* audioShortData = reinterpret_cast<short*>(((LPBYTE)vars) + vars->audiooffset);
    audioFloatDataPos += ReadAudioSamples(samplesToRead, audioShortData);
  } else {
    short* audioShortData = reinterpret_cast<short*>(((LPBYTE)vars) + vars->audiooffset);
    for (int i = 0; i < samplesToRead; i++) {
      audioShortData[i * 2 + 0] = 0;
      audioShortData[i * 2 + 1] = 0;
    }
  }
  int blockalign = (audioBitsPerSample * audioChannels) / 8;
  vars->audioBytesRead = samplesToRead * blockalign;
}

bool PremiereFSImpl::OnAudioRequestOneSecond(DWORD second, LPBYTE* data, DWORD* datalen) {
  int samplesToRead = audioSamplingRate;
  unsigned long curpos = second * samplesToRead;

  // If the caller wants to read audio before the current position, we have to restart
  // from the beginning since the API only allows sequential access.
  if (curpos < audioFloatDataPos) {
    audioFloatDataPos = 0;
    audioSuite->ResetAudioToBeginning(audioRenderId);
  }
  while (audioFloatDataPos < curpos) {
    int samplesRead = ReadAudioSamples(samplesToRead, NULL);  // read and ignore.
    if (!samplesRead) break;
    audioFloatDataPos += samplesRead;
  }

  // Read audio and convert to 16 bit samples.
  short* audioShortData = reinterpret_cast<short*>(*data);
  if (!audioShortData)
    audioShortData = reinterpret_cast<short*>(calloc(1, samplesToRead * 2 * sizeof(short)));
  int samplesRead = ReadAudioSamples(samplesToRead, audioShortData);
  audioFloatDataPos += samplesRead;

  *data = reinterpret_cast<LPBYTE>(audioShortData);
  *datalen = samplesRead * audioChannels * sizeof(short);

  return true;
}

prMALError doStartup(exportStdParms* stdParams, exExporterInfoRec* rec) {
  rec->fileType = 'DFSC';          // The filetype FCC (four char code)
  lstrcpy(rec->fileTypeName, APPNAME);
  lstrcpy(rec->fileTypeDefaultExtension, L"avi");
  rec->classID = 'DTEK';   // Class ID of the MAL (media abstraction layer)
  rec->exportReqIndex = 0;              // 0 for single file type
  rec->wantsNoProgressBar = kPrFalse;
  rec->hideInUI = kPrFalse;
  rec->doesNotSupportAudioOnly = kPrFalse;
  rec->canExportVideo = kPrTrue;
  rec->canExportAudio = kPrTrue;
  rec->singleFrameOnly = kPrFalse;
  rec->interfaceVersion = EXPORTMOD_VERSION;
  return malNoError;
}

prMALError doBeginInstance(exportStdParms* stdParams, exExporterInstanceRec* rec) {
  SPBasicSuite* basicSuite = stdParams->getSPBasicSuite();

  if (basicSuite == NULL)
    return malUnknownError;

  PrSDKMemoryManagerSuite* memorySuite;
  basicSuite->AcquireSuite(kPrSDKMemoryManagerSuite, kPrSDKMemoryManagerSuiteVersion,
      const_cast<const void**>(reinterpret_cast<void**>(&memorySuite)));
  PremiereFSImpl* fs = reinterpret_cast<PremiereFSImpl*>(memorySuite->NewPtr(sizeof(PremiereFSImpl)));
  if (!fs)
    return malUnknownError;

  // Initialize the object per c++
  fs = new (fs)PremiereFSImpl(basicSuite);

  rec->privateData = reinterpret_cast<long>(fs);

  return malNoError;
}

prMALError doEndInstance(exportStdParms* stdParams, exExporterInstanceRec* rec) {
  SPBasicSuite* basicSuite = stdParams->getSPBasicSuite();

  if (basicSuite == NULL)
    return malUnknownError;

  PremiereFSImpl* fs = reinterpret_cast<PremiereFSImpl*>(rec->privateData);
  if (fs) {
    // Destroy the object per c++ before releasing the memory.
    fs->~PremiereFSImpl();
    rec->privateData = NULL;

    PrSDKMemoryManagerSuite* memorySuite;
    basicSuite->AcquireSuite(kPrSDKMemoryManagerSuite, kPrSDKMemoryManagerSuiteVersion,
        const_cast<const void**>(reinterpret_cast<void**>(&memorySuite)));
    if (memorySuite) {
      memorySuite->PrDisposePtr(reinterpret_cast<PrMemoryPtr>(fs));
      basicSuite->ReleaseSuite(kPrSDKMemoryManagerSuite, kPrSDKMemoryManagerSuiteVersion);
    }
  }
  return malNoError;
}

extern "C" DllExport PREMPLUGENTRY xSDKExport(int selector, exportStdParms* stdParams, long param1, long param2) {
  prMALError result = exportReturn_Unsupported;

  debug(L"sel=", selector);

  switch (selector) {
  case exSelStartup:
    result = doStartup(stdParams, reinterpret_cast<exExporterInfoRec*>(param1));
    break;
  case exSelBeginInstance:
    result = doBeginInstance(stdParams, reinterpret_cast<exExporterInstanceRec*>(param1));
    break;
  case exSelEndInstance:
    result = doEndInstance(stdParams, reinterpret_cast<exExporterInstanceRec*>(param1));
    break;
  case exSelGenerateDefaultParams: {
    exGenerateDefaultParamRec* rec = reinterpret_cast<exGenerateDefaultParamRec*>(param1);
    PremiereFSImpl* fs = reinterpret_cast<PremiereFSImpl*>(rec->privateData);
    result = (fs ? fs->generateDefaultParams(rec) : malUnknownError);
  }
  break;
  case exSelPostProcessParams: {
    exPostProcessParamsRec* rec = reinterpret_cast<exPostProcessParamsRec*>(param1);
    PremiereFSImpl* fs = reinterpret_cast<PremiereFSImpl*>(rec->privateData);
    result = (fs ? fs->postProcessParams(rec) : malUnknownError);
  }
  break;
  case exSelQueryOutputSettings: {
    exQueryOutputSettingsRec* rec = reinterpret_cast<exQueryOutputSettingsRec*>(param1);
    PremiereFSImpl* fs = reinterpret_cast<PremiereFSImpl*>(rec->privateData);
    result = (fs ? fs->queryOutputSettings(rec) : malUnknownError);
  }
  break;
  case exSelExport: {
    exDoExportRec* rec = reinterpret_cast<exDoExportRec*>(param1);
    PremiereFSImpl* fs = reinterpret_cast<PremiereFSImpl*>(rec->privateData);
    result = (fs ? fs->serve(rec) : malUnknownError);
  }
  break;
  }
  return result;
}

BOOL APIENTRY DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved) {
  if (reason == DLL_PROCESS_ATTACH)
    ghInst = hinst;
  return TRUE;
}
