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

#ifndef FSCOMMON_IMAGESEQUENCE_H
#define FSCOMMON_IMAGESEQUENCE_H

#include "../dfsc/dfsc.h"

#ifdef IMAGESEQUENCE_EXPORTS
#define IMAGESEQUENCE_API __declspec(dllexport)
#else
#define IMAGESEQUENCE_API __declspec(dllimport)
#endif

enum {
  ImageSequenceFormatJPEG,
  ImageSequenceFormatPNG,
  ImageSequenceFormatTIFF,
  ImageSequenceFormatBMP,
};

extern "C" {
IMAGESEQUENCE_API void SaveImageSequence(DfscData* vars, const TCHAR* pathFormat, int imageFormat);
typedef void (*fxnSaveImageSequence)(DfscData* vars, const TCHAR* pathFormat, int imageFormat);

IMAGESEQUENCE_API bool CheckAndPreparePath(const TCHAR* pathFormat);
typedef bool (*fxnCheckAndPreparePath)(const TCHAR* pathFormat);
};

#endif  // FSCOMMON_IMAGESEQUENCE_H
