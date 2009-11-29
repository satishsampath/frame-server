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

#ifndef DMFILTER_H
#define DMFILTER_H

#include "dmbase.h"

//---- Flags to indicate which elements are there in the given file.
#define DM_ELEMENT_INVALID 0
#define DM_ELEMENT_IMAGE   1
#define DM_ELEMENT_AUDIO   2
#define DM_ELEMENT_VIDEO   4

//---- Structure which is passed to the Audio Import/Export functions.
typedef struct
{
	DWORD dwSamplingRate;
	DWORD dwNumSamples;
	AudioSample *psSamples;
}AudioFilterParams;

//---- Structure which is passed to the Video Import/Export functions.
typedef struct
{
	DWORD dwNumFrames;
	DWORD dwFrameRate;
	DWORD dwWidth;
	DWORD dwHeight;
	ImageSample *psVideoData;
    BOOL  bFields;
    BOOL  bOddFieldFirst;
}VideoFilterParams;

//---- Structure which is passed to the Image Import/Export functions.
typedef struct
{
	DWORD dwWidth;
	DWORD dwHeight;
	ImageSample *psImageData;
}ImageFilterParams;

// structure which holds the utility function pointers.
typedef struct
{
	DmFxnMalloc	    Malloc;
	DmFxnFree	    Free;
	HWND			hMainWnd;
}FilterTools;

//---- Prototypes for the import functions.
typedef HANDLE  (*DmFxnFilterBeginImport)(char *pcFileName,
                                          int *nStreams);
typedef int     (*DmFxnFilterImport)(HANDLE hImport,
                                     ImageFilterParams *psImage,
                                     AudioFilterParams *psAudio,
                                     VideoFilterParams *psVideo);
typedef int     (*DmFxnFilterImportSeek)(HANDLE hImport,
                                         ImageFilterParams *psImage,
                                         AudioFilterParams *psAudio,
                                         VideoFilterParams *psVideo);
typedef int     (*DmFxnFilterEndImport)(HANDLE hImport);
typedef int     (*DmFxnFilterImportInfo)(char *pcFileName,
                                         ImageFilterParams *psImage,
                                         AudioFilterParams *psAudio,
                                         VideoFilterParams *psVideo);

//---- Prototypes for the export functions.

// returns 0 if success, 1 if failure, <1 if other errors.
// if <1, return the same error code back to host.
// Alloc memory for 'image' using data given in Image/VideoFilterParams in DmFxnFilterExport
typedef int     (*DmFxnHostRenderGetVideo)(void *opaqueData, int frame, ImageSample *image);

// same as above,alloc memory for 'numFramesToRead' frames worth of 'audio' before calling
typedef int     (*DmFxnHostRenderGetAudio)(void *opaqueData, int frame, int numFramesToRead,
                                          AudioSample *audio, int *numAudioSamplesReturned);

typedef HANDLE  (*DmFxnFilterExportBegin)(char *pcFileName,
									      BYTE *pbVideoOptions,
									      int nVideoOptionsSize,
									      BYTE *pbAudioOptions,
									      int nAudioOptionsSize,
									      ImageFilterParams *psImage,
									      AudioFilterParams *psAudio,
									      VideoFilterParams *psVideo);
typedef int	    (*DmFxnFilterExportFrame)(HANDLE hExport,
								          ImageFilterParams *psImage,
								          AudioFilterParams *psAudio,
								          VideoFilterParams *psVideo);
typedef int	    (*DmFxnFilterExportEnd)(HANDLE hExport);
// return 0 if success, other error codes if failure
typedef int     (*DmFxnFilterExport)(DmFxnHostRenderGetVideo fxnHostRenderGetVideo,
                                     DmFxnHostRenderGetAudio fxnHostRenderGetAudio,
                                     void *opaqueData,
                                     char *pcFileName,
									 BYTE *pbVideoOptions,
									 int nVideoOptionsSize,
									 BYTE *pbAudioOptions,
									 int nAudioOptionsSize,
									 ImageFilterParams *psImage,
									 AudioFilterParams *psAudio,
									 VideoFilterParams *psVideo);
typedef void	(*DmFxnFilterOptions)(HWND hwnd,
								      BYTE **ppbOptions,
								      int *pnSize,
								      BOOL bVideo);
typedef int	    (*DmFxnFilterSaveOptions)(HANDLE hExport,
									      DmFxnWriteToFile fxnWriteFile,
									      BYTE *pbOptions,
									      int nSize,
									     BOOL bVideo);
typedef int	    (*DmFxnFilterLoadOptions)(HANDLE hImport,
									      DmFxnReadFromFile fxnReadFile,
									      BYTE **ppbOptions,
									      int *pnSize,
									      BOOL bVideo);
typedef BOOL    (*DmFxnFilterNeedAlpha)(BYTE *ppbOptions,
                                        int nSize);

//---- The capabilities of a given Import/Export Filter.
typedef struct
{
	char *pcFileExt;					            // The extension of the supported file type.
	char *pcFilter;					                // The filter string to be displayed in the File Open/Save dialog.
	int   nImportCaps;				                // A combination of the first-declared Flags to indicate which
										            //  elements (image/audio/video) can this import filter handle.
	int   nExportCaps;				                // A combination of the first-declared Flags to indicate which
										            //  elements (image/audio/video) can this export filter handle.
	DmFxnFilterBeginImport  fxnFilterBeginImport;   // The function to open the file & begin reading.
	DmFxnFilterImport	    fxnFilterImport;	    // The function which reads the file.
	DmFxnFilterImportSeek   fxnFilterSeek;		    // The function to seek the read file.
	DmFxnFilterEndImport    fxnFilterEndImport;	    // The function to end reading & close the file.

	DmFxnFilterImportInfo   fxnFilterImportInfo;    // The function which gives info about the file.

	DmFxnFilterExportBegin  fxnFilterBeginExport;   // The function to create the file & begin saving.
	DmFxnFilterExportFrame  fxnFilterExportFrame;   // The function to write a block fo data to the file.
	DmFxnFilterExportEnd    fxnFilterEndExport;	    // The function to end saving & close the file.

	DmFxnFilterOptions	    fxnFilterOptions;	    // The function to prompt the user for save options.
	DmFxnFilterSaveOptions  fxnFilterSaveOptions;   // The function to save the options into a file.
	DmFxnFilterLoadOptions  fxnFilterLoadOptions;   // The function to load the options from a file.
	DmFxnFree			    fxnFilterFreeOptions;   // The function to free the allocated options struct.
    DmFxnFilterNeedAlpha    fxnFilterNeedAlpha;     // If TRUE is returned, pass alpha as it is, else normalise.
}FilterCapsOld;

// This is the latest filter caps struct that plugins should use. The above isjust here for compatibility and
// should not be used. 
// **** Any additions to this struct should also result in additions in FilterManager.cpp ****
typedef struct
{
    int   cbSize;                                   // size of this struct, for extensibility
    char *pcFilterUniqueId;                         // Unique and should not change across versions
	char *pcFileExt;					            // The extension of the supported file type.
	char *pcFilter;					                // The filter string to be displayed in the File Open/Save dialog.
	int   nImportCaps;				                // A combination of the first-declared Flags to indicate which
										            //  elements (image/audio/video) can this import filter handle.
	int   nExportCaps;				                // A combination of the first-declared Flags to indicate which
										            //  elements (image/audio/video) can this export filter handle.
	DmFxnFilterBeginImport  fxnFilterBeginImport;   // The function to open the file & begin reading.
	DmFxnFilterImport	    fxnFilterImport;	    // The function which reads the file.
	DmFxnFilterImportSeek   fxnFilterSeek;		    // The function to seek the read file.
	DmFxnFilterEndImport    fxnFilterEndImport;	    // The function to end reading & close the file.

	DmFxnFilterImportInfo   fxnFilterImportInfo;    // The function which gives info about the file.

	DmFxnFilterExportBegin  fxnFilterBeginExport;   // The function to create the file & begin saving.
	DmFxnFilterExportFrame  fxnFilterExportFrame;   // The function to write a block fo data to the file.
	DmFxnFilterExportEnd    fxnFilterEndExport;	    // The function to end saving & close the file.
    DmFxnFilterExport       fxnFilterExport;        // Writes the full file synchronously

	DmFxnFilterOptions	    fxnFilterOptions;	    // The function to prompt the user for save options.
	DmFxnFilterSaveOptions  fxnFilterSaveOptions;   // The function to save the options into a file.
	DmFxnFilterLoadOptions  fxnFilterLoadOptions;   // The function to load the options from a file.
	DmFxnFree			    fxnFilterFreeOptions;   // The function to free the allocated options struct.
    DmFxnFilterNeedAlpha    fxnFilterNeedAlpha;     // If TRUE is returned, pass alpha as it is, else normalise.
}FilterCaps;

//---- General functions.
typedef int  (*DmFxnFilterGetCapsOld)(FilterCapsOld **ppsFilterCaps);
typedef int  (*DmFxnFilterGetCaps)(FilterCaps ***pppsFilterCaps);   // pointer to array of pointers
typedef void (*DmFxnFilterSetTools)(FilterTools *psTools);

#endif
