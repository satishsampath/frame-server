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

#ifndef FSCOMMON_FSCOMMON_H
#define FSCOMMON_FSCOMMON_H

#include <tchar.h>
#include "dfsc.h"

// These defines are used to make the same code build on VC6 32-bit, latest VC 32-bit and
// latest VC 64-bit builds.
#if defined(_WIN64)
  #define DLGPROCRET INT_PTR
#else  // defined(__WIN64)
  #define DLGPROCRET BOOL
  #if !defined(GWLP_WNDPROC)
    #define GWLP_WNDPROC GWL_WNDPROC
    #define GetWindowLongPtr GetWindowLong
    #define SetWindowLongPtr SetWindowLong
  #endif  // !defined(GWLP_WNDPROC)
#endif  // defined(__WIN64)

enum {
  sfRGB24 = 0,
  sfRGB32,
  sfYUY2
};

enum {
  idfRGB32 = 0,
  idfAYUV
};

extern BOOL pcmAudioInAvi, networkServing;
extern UINT networkPort, serveFormat;
extern BOOL stopServing;
extern HWND appwnd, servingdlg;
extern TCHAR installDir[MAX_PATH];
extern unsigned char* clip, * clipr, * clipb;
extern HINSTANCE ghResInst;

extern bool LoadCommonResource();
extern void ConvertVideoFrame(LPCVOID pFrame, int rowBytes, DfscData* vars, int inDataFormat = idfRGB32);
extern void InitClip();
extern void SetFsIconForWindow(HWND wnd);
extern DLGPROCRET CALLBACK AboutDlgProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp);
extern DLGPROCRET CALLBACK OptionsDlgProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp);
extern DLGPROCRET CALLBACK WritingSignpostDlgProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp);
extern DLGPROCRET CALLBACK ServingDlgProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp);

inline unsigned char Clip(int x) {
  return clip[320 + ((x + 0x8000) >> 16)];
}

class FrameServerImpl {
public:
  bool hasAudio;
  DWORD audioSamplingRate, audioBitsPerSample, audioChannels;
  DWORD numVideoFrames;     // numAudioFrames will be numVideoFrames*100, 100 audio frames per video frame
  double fps;
  DWORD width, height;      // video width & height in pixels
  HWND parentWnd, servingdlg;
  TCHAR filename[MAX_PATH];

  HANDLE varFile;
  HANDLE videoEncEvent, videoDecEvent;
  HANDLE audioEncEvent, audioDecEvent;
  DfscData* vars;

public:
  void Init(bool _hasAudio, DWORD _audioSamplingRate, DWORD _audioBitsPerSample, DWORD _audioChannels,
            DWORD _numVideoFrames, double _fps, DWORD _width, DWORD _height, HWND _parentWnd, TCHAR* _filename);
  static bool BlankAviReadAudioSamples(unsigned long second, void* readData, unsigned char** data,
                                       unsigned long* datalen);
  bool Run();
  virtual void OnVideoRequest() = 0;
  virtual void OnAudioRequest() = 0;
  virtual bool OnAudioRequestOneSecond(DWORD second, LPBYTE* data, DWORD* datalen) = 0;
};

#endif  // FSCOMMON_FSCOMMON_H

