/* ex:set sw=2 ts=8 wm=0:
 * $Header: u:/rcs/cv/rcs/drvproc.c 2.16 1995/05/09 12:50:58 bog Exp $

 * (C) Copyright 1992-1993 SuperMac Technology, Inc.
 * All rights reserved

 * This source code and any compilation or derivative thereof is the
 * sole property of SuperMac Technology, Inc. and is provided pursuant
 * to a Software License Agreement.  This code is the proprietary
 * information of SuperMac Technology and is confidential in nature.
 * Its use and dissemination by any party other than SuperMac Technology
 * is strictly limited by the confidential information provisions of the
 * Agreement referenced above.

 * $Log: drvproc.c $
 * Revision 2.16  1995/05/09 12:50:58  bog
 * DefDriverProc is declared in .h files.
 * Revision 2.15  1995/05/09  09:23:20  bog
 * Move WINVER back into the makefile.  Sigh.
 * 
 * Revision 2.14  1995/02/25  15:17:13  bog
 * ICDECOMPRESS_NULLFRAME means the frame is empty and we should not try to
 * decompress it.
 * 
 * Revision 2.13  1994/07/18  13:30:47  bog
 * Move WINVER definition from makefile to each .c file.
 * 
 * Revision 2.12  1994/05/24  17:04:23  timr
 * Add prototype for DevDriverProc.
 * 
 * Revision 2.11  1994/03/02  08:22:14  timr
 * Add support for ICM_DECOMPRESS_SET_PALETTE.
 *
 * Revision 2.10  1993/11/04  22:24:15  bog
 * Remove acceleration hook; use VIDS.CVID instead.
 *
 * Revision 2.9  1993/09/23  17:21:32  geoffs
 * Now correctly processing status callback during compression
 *
 * Revision 2.8  1993/09/09  09:20:56  geoffs
 * Add CompressFrames* stuff
 *
 * Revision 2.7  1993/08/05  06:38:52  timr
 * Correct return from DRV_CONFIGURE.  Required for NT drivers applet.
 *
 * Revision 2.6  1993/07/02  17:26:21  geoffs
 * Delete in-line int 3
 *
 * Revision 2.5  93/07/02  16:34:45  geoffs
 * Now compiles,runs under Windows NT
 *
 * Revision 2.4  93/06/29  10:16:56  geoffs
 * Now WIN32 compatible
 *
 * Revision 2.3  93/06/13  11:21:17  bog
 * Add hooks for playback acceleration.
 *
 * Revision 2.2  93/06/08  16:48:11  geoffs
 * Remove calls to DRAW routines
 *
 * Revision 2.1  93/06/03  10:13:18  geoffs
 * First cut at rework for VFW 1.5
 *
 * Revision 2.0  93/06/01  14:14:33  bog
 * Version 1.0 Release 1.3.0.1 of 1 June 1993.
 *
 * Revision 1.6  93/04/21  15:48:22  bog
 * Fix up copyright and disclaimer.
 *
 * Revision 1.5  92/12/22  14:22:24  geoffs
 * Added GetDefaultKeyframeRate function
 *
 * Revision 1.4  92/12/22  14:14:00  geoffs
 * Added ...Quality functions
 *
 * Revision 1.3  92/12/08  14:36:37  geoffs
 * Added DRAW message functionality into codec
 *
 * Revision 1.2  92/11/11  13:00:15  geoffs
 * We now use the version resource to display info in dialog boxes
 *
 * Revision 1.1  92/10/28  13:26:58  geoffs
 * Initial revision
 *
 */

/****************************************************************************
 *
 *   drvproc.c
 *
 *   Copyright (c) 1991 Microsoft Corporation.  All Rights Reserved.
 *
 *    You have a royalty-free right to use, modify, reproduce and
 *    distribute the Sample Files (and/or any modified version) in
 *    any way you find useful, provided that you agree that
 *    Microsoft has no warranty obligations or liability for any
 *    Sample Application Files which are modified.
 *
 ***************************************************************************/

#include <windows.h>
#include <mmsystem.h>
#include <compddk.h>
#include "iccv.h"

#define	lpbiIn		((LPBITMAPINFOHEADER) lParam1)
#define	lpbiOut		((LPBITMAPINFOHEADER) lParam2)
#define	px		((ICDECOMPRESSEX FAR *) lParam1)

#ifndef	WIN32
#define	DRVCNF_CANCEL	DRV_CANCEL
#define	DRVCNF_OK	DRV_OK
#endif

HMODULE ghModule;

//
//  we use this driverID to determine when we where opened as a video
//  device or from the control panel, etc....
//
#define BOGUS_DRIVER_ID     1


/***************************************************************************
 * @doc INTERNAL
 *
 * @api LRESULT | DriverProc | The entry point for an installable driver.
 *
 * @parm DWORD | dwDriverId | For most messages, <p dwDriverId> is the DWORD
 *     value that the driver returns in response to a <m DRV_OPEN> message.
 *     Each time that the driver is opened, through the <f DrvOpen> API,
 *     the driver receives a <m DRV_OPEN> message and can return an
 *     arbitrary, non-zero value. The installable driver interface
 *     saves this value and returns a unique driver handle to the
 *     application. Whenever the application sends a message to the
 *     driver using the driver handle, the interface routes the message
 *     to this entry point and passes the corresponding <p dwDriverId>.
 *     This mechanism allows the driver to use the same or different
 *     identifiers for multiple opens but ensures that driver handles
 *     are unique at the application interface layer.
 *
 *     The following messages are not related to a particular open
 *     instance of the driver. For these messages, the dwDriverId
 *     will always be zero.
 *
 *         DRV_LOAD, DRV_FREE, DRV_ENABLE, DRV_DISABLE, DRV_OPEN
 *
 * @parm HDRVR | hDriver | This is the handle returned to the
 *     application by the driver interface.
 *
 * @parm UINT | uiMessage | The requested action to be performed. Message
 *     values below <m DRV_RESERVED> are used for globally defined messages.
 *     Message values from <m DRV_RESERVED> to <m DRV_USER> are used for
 *     defined driver protocols. Messages above <m DRV_USER> are used
 *     for driver specific messages.
 *
 * @parm LPARAM | lParam1 | Data for this message.  Defined separately for
 *     each message
 *
 * @parm LPARAM | lParam2 | Data for this message.  Defined separately for
 *     each message
 *
 * @rdesc Defined separately for each message.
 ***************************************************************************/

LRESULT FAR PASCAL _loadds DriverProc(DWORD dwDriverID, HDRVR hDriver, UINT uiMessage, LPARAM lParam1, LPARAM lParam2)
{
    PINSTINFO pi = (
        (!dwDriverID || (dwDriverID == BOGUS_DRIVER_ID)) ?
	    NULL
	    :
	    (INSTINFO *)(UINT) dwDriverID
    );

    switch (uiMessage) {

        case DRV_LOAD:
            return (LRESULT) Load();

        case DRV_FREE:
            Free();
            return (LRESULT)1L;

        case DRV_OPEN:
            // if being opened with no open struct, then return a non-zero
            // value without actually opening
	    return (
	    	    (lParam2) ? ((LRESULT)(UINT) Open((ICOPEN FAR *) lParam2))
		    		:
				BOGUS_DRIVER_ID
		   );

        case DRV_CLOSE:
            if (pi) Close(pi);
            return (LRESULT)1L;

        /*********************************************************************

            state messages

        *********************************************************************/

        case DRV_QUERYCONFIGURE:    // configuration from drivers applet
            return ((LRESULT) QueryAbout());

        case DRV_CONFIGURE:
            return (About((HWND) lParam1) == ICERR_OK ?
	        DRVCNF_OK :
		DRVCNF_CANCEL
	    );

        case ICM_CONFIGURE:
            //
            //  return ICERR_OK if you will do a configure box, error otherwise
            //
            if (lParam1 == -1)
                return QueryConfigure(pi) ? ICERR_OK : ICERR_UNSUPPORTED;
            else
                return Configure(pi, (HWND)lParam1);

        case ICM_ABOUT:
            //
            //  return ICERR_OK if you will do a about box, error otherwise
            //
            if (lParam1 == -1)
                return QueryAbout() ? ICERR_OK : ICERR_UNSUPPORTED;
            else
                return About((HWND)lParam1);

        case ICM_GETSTATE:
            return GetState(pi, (LPVOID)lParam1, (DWORD)lParam2);

        case ICM_SETSTATE:
            return SetState(pi, (LPVOID)lParam1, (DWORD)lParam2);

        case ICM_GETINFO:
            return GetInfo(pi, (ICINFO FAR *)lParam1, (DWORD)lParam2);

        case ICM_GET:
            return Get(pi, (FOURCC) lParam1, (DWORD) lParam2);

        case ICM_SET:
            return Set(pi, (DWORD) lParam1, (DWORD) lParam2);

        /*********************************************************************
        *********************************************************************/

        case ICM_GETQUALITY:
	    return GetQuality(pi,(LPDWORD) lParam1);

        case ICM_SETQUALITY:
	    return SetQuality(pi,(DWORD) lParam1);

        case ICM_GETDEFAULTQUALITY:
	    return GetDefaultQuality((LPDWORD) lParam1);

        /*********************************************************************
        *********************************************************************/

        case ICM_GETDEFAULTKEYFRAMERATE:
	    return GetDefaultKeyframeRate((LPDWORD) lParam1);

        /*********************************************************************

            compression messages

        *********************************************************************/

        case ICM_COMPRESS_QUERY:
            return CompressQuery(pi,
                         (LPBITMAPINFOHEADER)lParam1,
                         (LPBITMAPINFOHEADER)lParam2);

        case ICM_COMPRESS_BEGIN:
            return CompressBegin(pi,
                         (LPBITMAPINFOHEADER)lParam1,
                         (LPBITMAPINFOHEADER)lParam2);

        case ICM_COMPRESS_GET_FORMAT:
            return CompressGetFormat(pi,
                         (LPBITMAPINFOHEADER)lParam1,
                         (LPBITMAPINFOHEADER)lParam2);

        case ICM_COMPRESS_GET_SIZE:
            return CompressGetSize(pi,
                         (LPBITMAPINFOHEADER)lParam1,
                         (LPBITMAPINFOHEADER)lParam2);

	case ICM_SET_STATUS_PROC: {
	    return (SetStatusProc(
	    		pi,
			(ICSETSTATUSPROC FAR *) lParam1,
			(DWORD) lParam2
		    )
	    );
	}

	case ICM_COMPRESS_FRAMES_INFO: {
	    return (CompressFramesInfo(
		    	pi,
			(ICCOMPRESSFRAMES FAR *) lParam1,
			(DWORD) lParam2
	    	    )
	    );
	}

	case ICM_COMPRESS_FRAMES: {
	    return (CompressFrames(
		    	pi,
			(ICCOMPRESSFRAMES FAR *) lParam1,
			(DWORD) lParam2
	    	    )
	    );
	}

        case ICM_COMPRESS:
            return Compress(pi,
                            (ICCOMPRESS FAR *)lParam1, (DWORD)lParam2);

        case ICM_COMPRESS_END:
            return CompressEnd(pi);

        /*********************************************************************

            decompress format query messages

        *********************************************************************/

        case ICM_DECOMPRESS_GET_FORMAT:
            return DecompressGetFormat(pi,
                         (LPBITMAPINFOHEADER)lParam1,
                         (LPBITMAPINFOHEADER)lParam2);

        case ICM_DECOMPRESS_GET_PALETTE:
            return DecompressGetPalette(pi,
                         (LPBITMAPINFOHEADER)lParam1,
                         (LPBITMAPINFOHEADER)lParam2);

	case ICM_DECOMPRESS_SET_PALETTE:
	    return DecompressSetPalette(pi,
			(LPBITMAPINFOHEADER)lParam1);

        /*********************************************************************

            decompress (old) messages, map these to the new (ex) messages

        *********************************************************************/

        case ICM_DECOMPRESS_QUERY:
            return DecompressQuery(pi,0,
                    lpbiIn,NULL,
                    0,0,-1,-1,
                    lpbiOut,NULL,
                    0,0,-1,-1);

        case ICM_DECOMPRESS_BEGIN:
            return DecompressBegin(pi,0,
                    lpbiIn,NULL,
                    0,0,-1,-1,
                    lpbiOut,NULL,
                    0,0,-1,-1);

        case ICM_DECOMPRESS:
            return(
	      Decompress(
		pi,
		px->dwFlags,
		px->lpbiSrc,px->lpSrc,
		0, 0, -1, -1,
		px->lpbiDst,px->lpDst,
		0, 0, -1, -1
	      )
	    );

	case ICM_DECOMPRESS_END:
            return DecompressEnd(pi);

        /*********************************************************************

            decompress (ex) messages

        *********************************************************************/

        case ICM_DECOMPRESSEX_QUERY:
            return DecompressQuery(pi,
                    px->dwFlags,
                    px->lpbiSrc,px->lpSrc,
                    px->xSrc,px->ySrc,px->dxSrc,px->dySrc,
                    px->lpbiDst,px->lpDst,
                    px->xDst,px->yDst,px->dxDst,px->dyDst);

        case ICM_DECOMPRESSEX_BEGIN:
            return DecompressBegin(pi,
                    px->dwFlags,
                    px->lpbiSrc,px->lpSrc,
                    px->xSrc,px->ySrc,px->dxSrc,px->dySrc,
                    px->lpbiDst,px->lpDst,
                    px->xDst,px->yDst,px->dxDst,px->dyDst);

        case ICM_DECOMPRESSEX:
            return Decompress(pi,
                    px->dwFlags,
                    px->lpbiSrc,px->lpSrc,
                    px->xSrc,px->ySrc,px->dxSrc,px->dySrc,
                    px->lpbiDst,px->lpDst,
                    px->xDst,px->yDst,px->dxDst,px->dyDst);

        case ICM_DECOMPRESSEX_END:
	    return DecompressEnd(pi);

        /*********************************************************************

            draw messages

        *********************************************************************/

	case ICM_DRAW:
	case ICM_DRAW_BEGIN:
	case ICM_DRAW_BITS:
	case ICM_DRAW_CHANGEPALETTE:
	case ICM_DRAW_END:
	case ICM_DRAW_FLUSH:
	case ICM_DRAW_GETTIME:
	case ICM_DRAW_GET_PALETTE:
	case ICM_DRAW_QUERY:
	case ICM_DRAW_REALIZE:
	case ICM_DRAW_RENDERBUFFER:
	case ICM_DRAW_SETTIME:
	case ICM_DRAW_START:
	case ICM_DRAW_STOP:
	case ICM_DRAW_SUGGESTFORMAT:
	case ICM_DRAW_UPDATE:
	case ICM_DRAW_WINDOW:
	{
	  return(ICERR_UNSUPPORTED);
	}

        /*********************************************************************

            standard driver messages

        *********************************************************************/

        case DRV_DISABLE:
        case DRV_ENABLE:
            return (LRESULT)1L;

        case DRV_INSTALL:
        case DRV_REMOVE:
            return (LRESULT)DRV_OK;
    }

    if (uiMessage < DRV_USER)
        return DefDriverProc(dwDriverID, hDriver, uiMessage,lParam1,lParam2);
    else
        return ICERR_UNSUPPORTED;
}

#if	!defined(WIN32)
/****************************************************************************
 * @doc INTERNAL
 *
 * @api int | LibInit | Library initialization code.
 *
 * @parm HANDLE | hModule | Our module handle.
 *
 * @rdesc Returns 1 if the initialization was successful and 0 otherwise.
 ***************************************************************************/
int NEAR PASCAL LibInit(HMODULE hModule)
{
    ghModule = hModule;

    return (1);
}
#else
/******************************************************************************\
*
*  FUNCTION:    DLLEntryPoint
*
*  INPUTS:      hDLL       - DLL module handle
*               dwReason   - reason being called (e.g. process attaching)
*               lpReserved - reserved
*
*  RETURNS:     TRUE if initialization passed, or
*               FALSE if initialization failed.
*
*  COMMENTS:    DLL initialization serialization is guaranteed within a
*               process (if multiple threads then DLL entry points are
*               serialized), but is not guaranteed across processes.
*
*               If your DLL uses any C runtime functions then you should
*               always call _CRT_INIT so that the C runtime can initialize
*               itself appropriately. Failure to do this may result in
*               indeterminate behavior. When the DLL entry point is called
*               for DLL_PROCESS_ATTACH & DLL_THREAD_ATTACH circumstances,
*               _CRT_INIT should be called before any other initilization
*               is performed. When the DLL entry point is called for
*               DLL_PROCESS_DETACH & DLL_THREAD_DETACH circumstances,
*               _CRT_INIT should be called after all cleanup has been
*               performed, i.e. right before the function returns.
*
\******************************************************************************/

BOOL WINAPI DllInstanceInit(PVOID hModule, ULONG Reason, PCONTEXT pContext)
{
    if (Reason == DLL_PROCESS_ATTACH) {
	ghModule = hModule;
    }

    return (TRUE);
}
#endif
