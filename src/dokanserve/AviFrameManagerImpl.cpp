#include "AviFrameManagerImpl.h"
#include <AVI20/Read/MediaStreamReader.h>
#include "DokanSignpost.h"

struct AviFrameManagerItem {
  bool isAudio;
  unsigned long frameIndex;
  int dataLength;
};

bool AviFrameManagerImpl::SignpostAviGetOneSecondAudio(void* handle, unsigned long second, unsigned char* data, unsigned long* datalen) {
  AviFrameManagerImpl* me = (AviFrameManagerImpl*) handle;
  DfscData* vars = me->vars;
  WaitForSingleObject(me->audioEncSem, INFINITE);
  if (vars->encStatus != 1) {  // encoder still running?
    vars->audioFrameIndex = second;
    vars->audioRequestFullSecond = TRUE;
    SetEvent(me->audioEncEvent);
    *datalen = 0;
    if (WaitForSingleObject(me->audioDecEvent, INFINITE) == 0) {
      memcpy(data, ((LPBYTE)vars) + vars->audiooffset, vars->audioBytesRead);
      *datalen = vars->audioBytesRead;
    }
  }
  ReleaseSemaphore(me->audioEncSem, 1, NULL);
  return *datalen > 0;
}

AviFrameManagerImpl::AviFrameManagerImpl() :
    audiobuf(NULL), audioCacheReader(NULL),
    audioCacheAvi(NULL), audioCacheStream(NULL),
    prevVideoFrameGetIndex(-1), prevAudioFrameGetIndex(-1),
    vars(NULL), varFile(NULL), audioEncSem(NULL), videoEncSem(NULL),
    audioEncEvent(NULL), audioDecEvent(NULL), videoEncEvent(NULL), videoDecEvent(NULL) {
}

AviFrameManagerImpl::~AviFrameManagerImpl() {
  if (audiobuf) delete[] audiobuf;
  if (audioCacheReader) delete audioCacheReader;
  if (audioCacheStream) delete audioCacheStream;
  if (audioCacheAvi) delete audioCacheAvi;
  for (int i = 0; i < frameCache.size(); ++i)
    delete[] frameCache[i].bytes;
  if (vars)
    UnmapViewOfFile(vars);
  CloseHandle(varFile);
  CloseHandle(audioEncEvent);
  CloseHandle(audioDecEvent);
  CloseHandle(audioEncSem);
  CloseHandle(videoEncEvent);
  CloseHandle(videoDecEvent);
  CloseHandle(videoEncSem);
}

bool AviFrameManagerImpl::Init(wchar_t* varsName) {
  varFile = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(DfscData), varsName);
  if (GetLastError() != ERROR_ALREADY_EXISTS)
    return false;
  if (!varFile)
    return false;
  vars = (DfscData*)MapViewOfFile(varFile, FILE_MAP_WRITE, 0, 0, 0);
  if (!vars)
    return false;
  audioEncSem = CreateSemaphoreA(NULL, 1, 1, vars->audioEncSemName);
  videoEncSem = CreateSemaphoreA(NULL, 1, 1, vars->videoEncSemName);
  audioEncEvent = CreateEventA(NULL, FALSE, FALSE, vars->audioEncEventName);
  audioDecEvent = CreateEventA(NULL, FALSE, FALSE, vars->audioDecEventName);
  videoEncEvent = CreateEventA(NULL, FALSE, FALSE, vars->videoEncEventName);
  videoDecEvent = CreateEventA(NULL, FALSE, FALSE, vars->videoDecEventName);
  return true;
}

AviFrameManagerHandle AviFrameManagerImpl::AviFrameManagerPut(bool isAudio, const char* data, int datalen) {
  AviFrameManagerItem* item = new AviFrameManagerItem;
  item->isAudio = isAudio;
  item->frameIndex = *(unsigned long*)data;
  item->dataLength = datalen;
  if (isAudio) {
    if (!audiobuf) {
      audiobuf = new char[datalen];
      memset(audiobuf, 0, datalen);
    }
  }

  if (!isAudio && frameCache.size() == 0) {
    for (int i = 0; i < _countof(cachedFrameBytes); ++i) {
      cachedFrameBytes[i] = new char[datalen];
      FrameCacheItem fci = { -1, cachedFrameBytes[i] };
      frameCache.push_back(fci);
    }
  }

  return (AviFrameManagerHandle)item;
}

bool AviFrameManagerImpl::AviFrameManagerGet(AviFrameManagerHandle handle, char** data, int* datalen) {
  AviFrameManagerItem* item = (AviFrameManagerItem*)handle;
  *datalen = item->dataLength;
  if (item->isAudio) {
    *data = audiobuf;
    if (item->frameIndex != prevAudioFrameGetIndex) {
      if (audioCacheReader && audioCacheReader->FirstAudioStreamReader().IsValid()) {
        audioCacheReader->FirstAudioStreamReader().ReadFrameData(item->frameIndex, (uint8_t*)audiobuf);
      } else {
        memset(*data, 0, *datalen);
      }
      prevAudioFrameGetIndex = item->frameIndex;
    }
  } else {
    for (std::vector<FrameCacheItem>::iterator it = frameCache.begin(); it != frameCache.end(); ++it) {
      FrameCacheItem fci = *it;
      if (fci.index == item->frameIndex) {
        *data = fci.bytes;
        frameCache.erase(it);
        frameCache.push_back(fci);
        prevVideoFrameGetIndex = item->frameIndex;
        return true;
      }
    }
    FrameCacheItem fci = frameCache[0];
    WaitForSingleObject(videoEncSem, INFINITE);
    do {
      if (vars->encStatus == 1)         // encoder closed
        break;
      vars->videoFrameIndex = item->frameIndex;
      SetEvent(videoEncEvent);
      WaitForSingleObject(videoDecEvent, INFINITE);
      if (vars->encStatus == 1)         // encoder closed
        break;

      frameCache.erase(frameCache.begin());
      memcpy(fci.bytes, (char*)vars + vars->videooffset, vars->videoBytesRead);
    } while (0);
    ReleaseSemaphore(videoEncSem, 1, NULL);

    if (vars->encStatus == 1) {        // encoder closed
      memset(fci.bytes, 0, item->dataLength);
    }
    fci.index = item->frameIndex;
    frameCache.push_back(fci);
    *data = fci.bytes;
    prevVideoFrameGetIndex = item->frameIndex;
  }
  return true;
}

void AviFrameManagerImpl::AviFrameManagerCloseHandle(AviFrameManagerHandle handle) {
  AviFrameManagerItem* item = (AviFrameManagerItem*)handle;
  if (item) delete item;
}

bool AviFrameManagerImpl::CreateSignpostAndAudioCacheAVIs(AviFileWrapper* signpostAvi, const wchar_t* audioCacheFilename) {
  audioCacheAvi = new AviFileDiskWrapperImpl(audioCacheFilename, true);
  if (!audioCacheAvi->is_open()) {
    delete audioCacheAvi;
    return false;
  }

  DWORD fourccs[] = {
    /* sfRGB24 */ BI_RGB,
    /* sfRGB32 */ BI_RGB,
    /* sfYUY2 */ mmioFOURCC('Y', 'U', 'Y', '2'),
    /* sfV210 */ mmioFOURCC('v', '2', '1', '0')
  };
  DWORD framesizes[] = {
    vars->encBi.biHeight * ((vars->encBi.biWidth * 3 + 3) & (~3)),
    vars->encBi.biHeight * (vars->encBi.biWidth * 4),
    vars->encBi.biHeight * ((vars->encBi.biWidth * 2 + 1) & (~1)),
    vars->encBi.biHeight * ((((vars->encBi.biWidth + 5) / 6 * 16) + 127) & (~127))
  };

  CreateSignpostAvis(vars->numVideoFrames, vars->fpsNumerator, vars->fpsDenominator, fourccs[vars->serveFormat],
    vars->encBi.biWidth, vars->encBi.biHeight,
    vars->encBi.biBitCount, framesizes[vars->serveFormat],
    (uint64_t)(((double)vars->numVideoFrames * vars->wfx.nSamplesPerSec * vars->fpsDenominator) / vars->fpsNumerator),
    &vars->wfx, SignpostAviGetOneSecondAudio, this, signpostAvi, audioCacheAvi);
  if (!audioCacheAvi->good()) {
    delete audioCacheAvi;
    return false;
  }

  audioCacheAvi->seekg(0);
  audioCacheStream = new AVI20::Read::Stream(audioCacheAvi);
  audioCacheReader = new AVI20::Read::Reader(*audioCacheStream);
  return true;
}
