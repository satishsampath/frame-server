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

#ifndef FSCOMMON_BLANKAVI_H
#define FSCOMMON_BLANKAVI_H

typedef bool (*fxnReadAudioSamples)(unsigned long frame, void* readData,
                                    unsigned char** data, unsigned long* datalen);

bool CreateBlankAviPcmAudio(unsigned long numframes, int frrate, int frratescale,
                            int width, int height, int videobpp, TCHAR* filename, DWORD fcc,
                            unsigned long numsamples, WAVEFORMATEX* wfx,
                            unsigned long stream, fxnReadAudioSamples readSamples, void* readData);

bool CreateBlankAvi(unsigned long numframes, int frrate, int frratescale,
                    int width, int height, int videobpp, TCHAR* filename, DWORD fcc,
                    unsigned long numaudioblocks, WAVEFORMATEX* wfx,
                    unsigned long stream);

bool IsAudioCompatible(int bitsPerSample, int samplingRate);

#endif // FSCOMMON_BLANKAVI_H
