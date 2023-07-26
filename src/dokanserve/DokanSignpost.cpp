#include <windows.h>
#include <stdlib.h>
#define AVI20_API
#include <AVI20/Write/Writer.h>
#include <AVI20/Write/StreamW.h>
#include <AVI20/Write/MediaStreamWriter.h>
#include <AVI20/WaveFormatEx.h>
#include "AviFileWrapperImpl.h"
#include "DokanSignpost.h"
#include "../dfsc/dfsc.h"

#define AUDIO_CACHE_FILE_SUFFIX L".audiocache.avi"

extern DfscData* vars;
extern HANDLE audioEncSem, audioEncEvent, audioDecEvent;

bool CreateSignpostAvis(unsigned long numframes, int frrate, int frratescale,
                        DWORD videoFcc, int width, int height, int videobpp, int framesize,
                        uint64_t numsamples, WAVEFORMATEX* wfx, fxnReadAudioSamples readSamples,
                        void* readSamplesHandle, AviFileWrapper* videoWrapper, AviFileWrapper* audioCacheWrapper) {
  // In memory AVI
  AVI20::Write::Stream imStream(videoWrapper);
  AVI20::Write::Writer imWriter(imStream);
  AVI20::Write::MediaStreamWriter imVideoStreamWriter = imWriter.AddMediaStream(
    width, height, videobpp, videoFcc, framesize, frrate, frratescale);
  AVI20::Write::MediaStreamWriter imAudioStreamWriter = imWriter.AddMediaStream(
    AVI20::WaveFormatEx(*(const AVI20::WAVEFORMATEX*)wfx));
  imWriter.Start();

  //---- audio cache AVI
  AVI20::Write::Stream acStream(audioCacheWrapper);
  AVI20::Write::Writer acWriter(acStream);
  AVI20::Write::MediaStreamWriter acAudioStreamWriter = acWriter.AddMediaStream(
    AVI20::WaveFormatEx(*(const AVI20::WAVEFORMATEX*)wfx));
  acWriter.Start();

  uint8_t* videoframe = new uint8_t[framesize];
  uint8_t* audioframe = new uint8_t[wfx->nBlockAlign * wfx->nSamplesPerSec];
  memset(videoframe, 0, framesize);
  int64_t totalAudioBytes = numsamples * wfx->nBlockAlign;
  int second = 0;
  unsigned long videoFrameIndex = 0;
  while (totalAudioBytes > 0) {
    // Write frames for about a second worth (with frame markers) to in memory AVI
    for (int i = 0; i < (int)(frrate / frratescale); ++i) {
      if (videoFrameIndex < numframes) {
        *(unsigned long*)videoframe = videoFrameIndex++;
        imVideoStreamWriter.WriteFrame(videoframe, framesize, true);
      }
    }

    // Write audio worth one second to the audio cache AVI
    unsigned long datalen = 0;
    readSamples(readSamplesHandle, second, audioframe, &datalen);
    acAudioStreamWriter.WriteFrame(audioframe, datalen, true);

    // .. and one second of blank audio (with frame marker) to in memory AVI
    memset(audioframe, 0, datalen);
    *(unsigned long*)audioframe = second;
    imAudioStreamWriter.WriteFrame(audioframe, datalen, true);

    second++;
    totalAudioBytes -= datalen;
    if (datalen == 0 && videoFrameIndex >= numframes)  // no audio received & all video written? EOF
      totalAudioBytes = 0;
  }

  // Write any remaining video frames (with frame markers) to in memory AVI
  while (videoFrameIndex < numframes) {
    *(unsigned long*)videoframe = videoFrameIndex++;
    imVideoStreamWriter.WriteFrame(videoframe, framesize, true);
  }

  delete[] videoframe;
  delete[] audioframe;
  imWriter.Finalize();
  acWriter.Finalize();

  return true;
}
