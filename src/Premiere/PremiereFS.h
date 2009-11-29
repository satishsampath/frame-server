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

#ifndef PREMIERE_PREMIEREFS_H
#define PREMIERE_PREMIEREFS_H

#include "prSDKTypes.h"
#include "prSDKPlugSuites.h"
#include "prSDKCompile.h"
#include "prSDKEntry.h"
#include "prSDKStructs.h"
#include "PrSDKCompilerRenderSuite.h"
#include "PrSDKCompilerAudioSuite.h"
#include "fscommon.h"

// ----------------------------------------------------------
// Prototypes

extern "C" DllExport PREMPLUGENTRY xCompileEntry(int selector,
                                                 compStdParms* stdParms,
                                                 long param1,
                                                 long param2);

int compSDKStartup(compStdParms* stdParms,
                   compInfoRec* infoRec);

int compSDKGetIndFormat(compStdParms* stdParms,
                        compFileInfoRec* fileInfoRec,
                        int idx);

int compSDKGetAudioIndFormat(compStdParms* stdParms,
                             compAudioInfoRec* audioInfoRec,
                             int idx);

int compSDKDoCompile(compStdParms* stdParms,
                     compDoCompileInfo* SDKdoCompileInfoRec);

int compSDKGetAudioIndFormat7(compStdParms* stdParms,
                              compAudioInfoRec7* audioInfoRec,
                              int idx);

int compSDKDoCompile7(compStdParms* stdParms,
                      compDoCompileInfo7* SDKdoCompileInfoRec);

class PremiereFSImpl : public FrameServerImpl {
public:
  compStdParms* sp;
  LPVOID pFrame;

  // for Premiere <= 6.5
  compDoCompileInfo* irec;
  char** audioBuffer;

  // for PremierePro >= 1.0
  compDoCompileInfo7* irec7;
  float* audioFloatData[2];
  DWORD audioFloatDataPos;
  PrSDKCompilerRenderSuite* CompRenderSuite;
  PrSDKCompilerAudioSuite* CompAudioSuite;

public:
  void OnVideoRequest();
  void OnAudioRequest();
  bool OnAudioRequestOneSecond(DWORD second, LPBYTE* data, DWORD* datalen);
};

#endif  // PREMIERE_PREMIEREFS_H
