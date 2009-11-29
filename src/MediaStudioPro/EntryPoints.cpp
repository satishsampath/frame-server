// EntryPoints.cpp : Defines the entry point for the DLL application.
//

#include <windows.h>
#include "uvideo.h"
#include "MediaStudioProFS.h"

// temp use the last file format signature defined in uvideo.h
// should request a standard signature from Ulead before formal release
#define UVSIG_SAMPLE    (UVSIG_END<<3)

#define MID MID_MAIN    // for error handling

HINSTANCE ghInst;

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        ghInst = (HINSTANCE)hModule;
        break;

    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

int WINAPI uvInit2(PVIDAPP pVidApp)
{
    pVidApp->dwFlags = UVAP_SUPPORT_DIB_32BIT;
    return 1;
}

int WINAPI uvGetFileFmtNum()
/***************************************************************************
 *   Purpose :  get total number of file formats
 *    Return :  number of formats
 ***************************************************************************/
{
    return  1;
}

int WINAPI uvGetFormatInfo(PVIDFMT pVidFmt)
/***************************************************************************
 *   Purpose :  get file format info
 *     Input :  pVidFmt     -- points to store format info
 *    Return :  >0 for success, <=0 for error
 ***************************************************************************/
{
    ZeroMemory(pVidFmt, sizeof(VIDFMT));
    pVidFmt->dwVersion	= UVIDEO_VERSION;       // current VIO version, defined in uvideo.h
    pVidFmt->dwFmtInfo	= UVFMT_NORMAL;         // normal file format
    pVidFmt->dwOpenCap	= 0;                    // not support open functions
    pVidFmt->dwSaveCap	= UVCAP_RGB32|          // only support 24-bit DIB
                          UVCAP_AUDIO|          // support audio
                          0;
    pVidFmt->dwFmtSig	= UVSIG_SAMPLE;            // file format signature, registered in uvideo.h
    pVidFmt->wExtraPSPG	= 0;                       // number of save option page
    strcpy(pVidFmt->szDefFileExt, "avi");
    strcpy(pVidFmt->szFormatName, "DebugMode FrameServer Files");
    return 1;
}

int WINAPI uvSaveVideo(HWND hParent, PSAVEDATA pSData, UVPFN_SVCB pfnCallBack)
/***************************************************************************
 *   Purpose :	save video file
 *     Input :	hParent		-- parent window handle
 *	        	pSData		-- points to save data
 *      		pfnCallBack	-- callback function for saving video
 *    Return :  >0 for success, <=0 for error
 ***************************************************************************/
{
    CMediaStudioProFS fs;
    return fs.Progress(hParent, pSData, pfnCallBack);
}

int WINAPI uvGetSaveOptionInfo(PSAVEDATA pSData)
/***************************************************************************
 *   Purpose :	get save options
 *     Input :	pSData		-- points to save data
 *    Return :  >0 for success, <=0 for error
 ***************************************************************************/
{
    pSData->pVIOBuf = (void*)1;
    pSData->wSaveBitCount = 32;
    return 1;
}

int WINAPI uvFreeSaveOptionInfo(PSAVEDATA pSData)
/***************************************************************************
 *   Purpose :	free save options data
 *     Input :	pSData		-- points to save data
 *    Return :  >0 for success, <=0 for error
 ***************************************************************************/
{
    return 1;
}

int WINAPI uvGetExtraSavePSPG(PSAVEDATA pSData, PROPSHEETPAGE *psp, int nIndex)
/***************************************************************************
 *   Purpose :	get extra save option property sheet pages
 *     Input :	pSData		-- save data
 *          		psp		-- points to property sheet page data
 *      		nIndex		-- page index
 *    Return :  >0 for success, <=0 for error
 ***************************************************************************/
{
    return 0;   // no prop pages
}

int WINAPI uvPackSaveData(PSAVEDATA pSData, PBYTE pBuf, int * pnSize)
/***************************************************************************
 *   Purpose :  pack vio buffer ofsave data into packedbuffer
 *     Input :  pSData  -- points to save data
 *              pBuf    -- points to packed buffer
 *              pnSize  -- points to store packed size
 *    Return :  >0 for success, <=0 for error
 ***************************************************************************/
{
    int nSize = 1;
    if (!pBuf) { // query needed size of buffer
        *pnSize = nSize;
        return 1;
    }
    return 1;
}

int WINAPI uvUnPackSaveData(PSAVEDATA pSData, PBYTE pBuf, int nSize)
/***************************************************************************
 *   Purpose :  unpack packed buffer into vio buffer of save data
 *     Input :  pSData      -- points to save data
 *              pBuf        -- points to packed buffer
 *              nSize       -- packed size
 *    Return :  >0 for success, <=0 for error
 ***************************************************************************/
{
    pSData->pVIOBuf = (void*)1;
    return 1;
}
