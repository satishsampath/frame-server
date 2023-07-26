#ifndef AVI_FRAME_MANAGER_IMPL_H
#define AVI_FRAME_MANAGER_IMPL_H

#define WIN32_NO_STATUS 
#include <windows.h>
#define AVI20_API
#include <AVI20/Read/Reader.h>
#include <AVI20/Read/Stream.h>
#include "AviFileWrapperImpl.h"
#include "..\dfsc\dfsc.h"
#include <vector>

class AviFrameManagerImpl : public AviFrameManager {
public:
  AviFrameManagerImpl();
  virtual ~AviFrameManagerImpl();

  virtual AviFrameManagerHandle AviFrameManagerPut(bool isAudio, const char* data, int datalen);
  virtual bool AviFrameManagerGet(AviFrameManagerHandle handle, char** data, int* datalen);
  virtual void AviFrameManagerCloseHandle(AviFrameManagerHandle handle);

  bool Init(wchar_t* varsName);
  bool CreateSignpostAndAudioCacheAVIs(AviFileWrapper* videoSignpostAvi, const wchar_t* audioCacheFilename);
  DfscData* getDfscData() { return vars; }

private:
  static bool SignpostAviGetOneSecondAudio(void* handle, unsigned long second, unsigned char* data, unsigned long* datalen);

  typedef struct {
    int index;
    char* bytes;
  } FrameCacheItem;

  char* audiobuf;
  unsigned long prevVideoFrameGetIndex;
  unsigned long prevAudioFrameGetIndex;

  AviFileDiskWrapperImpl* audioCacheAvi;
  AVI20::Read::Reader* audioCacheReader;
  AVI20::Read::Stream* audioCacheStream;

  char *cachedFrameBytes[20];
  std::vector<FrameCacheItem> frameCache;

  DfscData* vars;
  HANDLE varFile;
  HANDLE audioEncSem, audioEncEvent, audioDecEvent;
  HANDLE videoEncSem, videoEncEvent, videoDecEvent;
};

#endif  // AVI_FRAME_MANAGER_IMPL_H
