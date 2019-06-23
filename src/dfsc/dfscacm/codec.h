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
#ifndef DFSC_DFSCACM_CODEC_H
#define DFSC_DFSCACM_CODEC_H

#include "../dfsc.h"

#pragma pack(1) // assume byte packing throughout

#ifdef __cplusplus
    #define EXTERN_C extern "C"
#else
    #define EXTERN_C extern
#endif

#ifdef __cplusplus
extern "C"                          // assume C declarations for C++
{
#endif

#if (WINVER >= 0x0400)
 #define VERSION_ACM_DRIVER  MAKE_ACM_VERSION(4, 0, 0)
#else
 #define VERSION_ACM_DRIVER  MAKE_ACM_VERSION(3, 51, 0)
#endif
#define VERSION_MSACM       MAKE_ACM_VERSION(3, 50, 0)

#ifndef FNLOCAL
    #define FNLOCAL     _stdcall
    #define FNCLOCAL    _stdcall
    #define FNGLOBAL    _stdcall
    #define FNCGLOBAL   _stdcall
    #define FNWCALLBACK CALLBACK
    #define FNEXPORT    CALLBACK
#endif

#define SIZEOF_ARRAY(ar)            (sizeof(ar) / sizeof((ar)[0]))
#define SIZEOFACMSTR(x) (sizeof(x) / sizeof(WCHAR))

//
// macros to compute block alignment and convert between samples and bytes
// of PCM data. note that these macros assume:
// wBitsPerSample  =  8 or 16
// nChannels       =  1 or 2
// the pwfx argument is a pointer to a WAVEFORMATEX structure.
//
#define PCM_BLOCKALIGNMENT(pwfx)        (UINT)(((pwfx)->wBitsPerSample >> 3) << ((pwfx)->nChannels >> 1))
#define PCM_AVGBYTESPERSEC(pwfx)        (DWORD)((pwfx)->nSamplesPerSec * (pwfx)->nBlockAlign)
#define PCM_BYTESTOSAMPLES(pwfx, cb)    (DWORD)(cb / PCM_BLOCKALIGNMENT(pwfx))
#define PCM_SAMPLESTOBYTES(pwfx, dw)    (DWORD)(dw * PCM_BLOCKALIGNMENT(pwfx))

typedef struct tDRIVERINSTANCE {
  //
  // although not required, it is suggested that the first two members
  // of this structure remain as fccType and DriverProc _in this order_.
  // the reason for this is that the driver will be easier to combine
  // with other types of drivers (defined by AVI) in the future.
  //
  FOURCC fccType;                   // type of driver: 'audc'
  DRIVERPROC fnDriverProc;          // driver proc for the instance

  //
  // the remaining members of this structure are entirely open to what
  // your driver requires.
  //
  HDRVR hdrvr;                      // driver handle we were opened with
  HINSTANCE hinst;                  // DLL module handle.
  DWORD vdwACM;                     // current version of ACM opening you
  DWORD fdwOpen;                    // flags from open description
} DRIVERINSTANCE, * PDRIVERINSTANCE, FAR* LPDRIVERINSTANCE;

typedef struct {
  DfscData* vars;
  HANDLE varFile;
  HANDLE audioEncSem, audioEncEvent, audioDecEvent;
  int curFrameIndex;
} DfscAdec;

typedef struct tSTREAMINSTANCE {
  DfscAdec adec;
  int audioEncFrame;
} STREAMINSTANCE, * PSTREAMINSTANCE, FAR* LPSTREAMINSTANCE;

#define DFSCACM_MAX_CHANNELS       2
#define DFSCACM_BITS_PER_SAMPLE    0
#define DFSCACM_WFX_EXTRA_BYTES    (sizeof(IMAADPCMWAVEFORMAT) - sizeof(WAVEFORMATEX))
#define DFSCACM_HEADER_LENGTH      4    // In bytes, per channel.

#pragma pack() // revert to default packing

#ifdef __cplusplus
}                                   // end of extern "C" {
#endif

#endif // DFSC_DFSCACM_CODEC_H
