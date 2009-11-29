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

#ifndef DMBASE_H
#define DMBASE_H

#ifdef NOWINDOWSH
    typedef void *HANDLE;
    typedef unsigned char BYTE;
    typedef unsigned int UINT;
    typedef unsigned long DWORD;
    typedef int BOOL;
    typedef void *HWND;
    typedef void *HICON;
    typedef unsigned long COLORREF;
    typedef struct { long x, y; }POINT;
    typedef BOOL (*DLGPROC)(HWND,unsigned int,DWORD,DWORD);
    typedef void *LPBITMAPINFOHEADER;
    typedef void *HINSTANCE;
#else
    #include <windows.h>
#endif

typedef void* (*DmFxnMalloc)(unsigned int size);
typedef void  (*DmFxnFree)(void *pvPtr);
typedef int   (*DmFxnWriteToFile)(HANDLE hExport, const char *format, ...);
typedef int   (*DmFxnReadFromFile)(HANDLE hExport, const char *format, ...);

#ifndef IMAGESAMPLE_DEFINED
#define IMAGESAMPLE_DEFINED

    typedef struct
    {
	    BYTE b, g, r, a;
    }ImageSample;

    #define CopyImageSample(dstptr, srcptr)		 \
	    *(DWORD *)(dstptr) = *(DWORD *)(srcptr)

    #define ZeroImageSample(dstptr)				 \
	    *(DWORD *)(dstptr) = 0
#endif

#ifndef IMAGESAMPLE3_DEFINED
#define IMAGESAMPLE3_DEFINED
    typedef struct
    {
        unsigned char b, g, r;
    }ImageSample3;
#endif

#ifndef AUDIOSAMPLE_DEFINED
#define AUDIOSAMPLE_DEFINED
    typedef struct
    {
	    signed short left, right;
    }AudioSample;

    #define CopyAudioSample(dstptr, srcptr)		 \
	    *(DWORD *)(dstptr) = *(DWORD *)(srcptr)

    #define ZeroAudioSample(dstptr)				 \
	    *(DWORD *)(dstptr) = 0
#endif

#endif
