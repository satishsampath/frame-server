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

#ifndef UTILS_UTILS_H
#define UTILS_UTILS_H

extern void CpuDetect();
extern int FPU_enabled, MMX_enabled;
extern void mmx_ConvertRGB32toYUY2(unsigned char *src,unsigned char *dst,int src_pitch, int dst_pitch,int w, int h);
extern "C" void fast_memcpy(void *_dst, const void *_src, size_t _size);

#if defined(_M_X64)
extern "C"
{
  unsigned long mmx_BGRx_to_RGB24(const void *pSrc, void *pDst, long height, long width, long cbRow);
  unsigned long mmx_VUYx_to_YUY2(const void *pSrc, void *pDst, long height, long width, long cbRow);
  unsigned long mmx_CopyVideoFrame(const void *pSrc, void *pDst, long height, long dstRowBytes, long srcRowBytes);
}
#endif

#endif // UTILS_UTILS_H
