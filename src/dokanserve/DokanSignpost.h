#ifndef DOKAN_SIGNPOST_H
#define DOKAN_SIGNPOST_H

#include <AVI20/AviFileWrapper.h>

typedef bool (*fxnReadAudioSamples)(void* handle, unsigned long second, unsigned char* data, unsigned long* datalen);

extern bool CreateSignpostAvis(unsigned long numframes, int frrate, int frratescale,
                               DWORD videoFcc, int width, int height, int videobpp, int framesize,
                               uint64_t numsamples, WAVEFORMATEX* wfx, fxnReadAudioSamples readSamples,
                               void* readSamplesHandle, AviFileWrapper* videoWrapper, AviFileWrapper* audioCacheWrapper);

#endif // DOKAN_SIGNPOST_H
