/*******************************************************************************
*                    Copyright (c) 1991-1997 Gemplus developpement
*
* Name        : OR3GBPOP.C
*
* Description : Implement the function which opens a communication channel with
*               an OROS 3.x IFD through Gemplus Block Protocol.
*
* Author      : E. PLET
*               G. PAUZIE
*               T. FABRE
*
* Compiler    : Microsoft C PDS 6.0 / 7.0 /8.0
*               Borland   C++   3.x / 4.0
*               Microsoft C++ 1.5 for Windows 16 bits
*               Microsoft C++ 2.0 for Windows 32 bits
*               Borland   C++ 2.0 pour OS/2
*
* Host        : IBM PC and compatible machines under MS/DOS 3.1 and upper.
*               IBM PC and compatible machines under Windows 3.x.
*               IBM PC and compatible machines under Windows 32 bits (W95 or WNT).
*               IBM PC and compatible machines under OS/2 Warp.
*
* Release     : 4.31.002
*
* Last Modif  : 24/08/98: V4.31.002  (GP)
	*               13/10/97: V4.31.001  (GP)
		*                 - Manage the logical handle and not the physical port number.
*               08/07/97: V4.30.002  (TF)
	*                 - Modify G_Oros3OpenComm function for Oros2.x reader.
*               18/03/97: V4.30.001  (TF)
	*                 - Start of development.
*
********************************************************************************
*
* Warning     :
*
* Remark      :
*
*******************************************************************************/

/*------------------------------------------------------------------------------
	Information section
- G_NAME is set to "Or3GBPOp"
- G_RELEASE is set to "4.31.002"
------------------------------------------------------------------------------*/
#define G_NAME     "Or3GBPOp"
#define G_RELEASE  "4.31.002"

// added by atul
#define G_UNIX

/*------------------------------------------------------------------------------
	Pragma section
- comment is called if _MSC_VER is defined.
------------------------------------------------------------------------------*/
#ifdef _MSC_VER
#pragma comment(exestr,"Gemplus(c) "G_NAME" Ver "G_RELEASE" "__DATE__)
#endif
/*------------------------------------------------------------------------------
	Compatibility section
- __MSC is define for compatibily with MSC under Borland environment.
------------------------------------------------------------------------------*/
#define __MSC
/*------------------------------------------------------------------------------
	Include section
Environment include:
- windows.h gives general Windows 3.1 macros, values and functions.
STRICT keyword is used to verify stricly variable types.
This file is include only if windows version is required.
------------------------------------------------------------------------------*/
#ifdef G_WINDOWS
#define STRICT
#include <windows.h>
#endif
#ifdef G_OS2
#include <os2.h>
#endif
/*------------------------------------------------------------------------------
Compiler include:
- string.h for _fmemcmp function.
------------------------------------------------------------------------------*/
#include <string.h>
/*------------------------------------------------------------------------------
Gemplus includes:
- gemplus.h is used to define general macros and values.
- gemgcr.h holds readers definitions
- gtser.h manages the serial line communication.
- gttimout.h manages timeout.
- gtgbp.h manages Gemplus Block Protocol.
- or3comm.h defines communication parameters.
- gemansi.h is used to redefine functions for an Ansi code
------------------------------------------------------------------------------*/
#include "gemplus.h"
#include "gemgcr.h"
#include "gtser.h"
#include "gttimout.h"
#include "gtgbp.h"
#include "or3comm.h"
#if (defined WIN32) || (defined G_UNIX) || (defined G_OS2)
#include "gemansi.h"
#endif
/*------------------------------------------------------------------------------
	Module public interface.
- or3gll.h
------------------------------------------------------------------------------*/
#include "or3gll.h"

#ifdef G_DEBUG
extern void  G_DECL  trace_debug_open(const INT16 Handle);
#endif

#include "config.h"		// for debug macros


/*------------------------------------------------------------------------------
Function definition section:
------------------------------------------------------------------------------*/
/*******************************************************************************
* INT16 G_DECL G_Oros3OpenComm
* (
	INT16 com_no,
	WORD32 BaudRate
* )
	*
* Description :
* -------------
	* This function opens a communication channel with a GemCore >= 1.x IFD. It runs a
* sequence to determine the currently in use baud rate and to set the IFD in
* an idle mode from the communication protocol point of view.
*
* Remarks     :
* -------------
	* Nothing.
*
*
* In          :
* -------------
	*  - Port, BaudRate and ITNumber of the Param structure must be filled.
*  - Handle holds the logical number to manage.
*
* Out         :
* -------------
	* Nothing.
*
* Responses   :
* -------------
* If everything is Ok:
*  - a handle on the communication channel (>= 0).
* If an error condition is raised:
*  - GE_IFD_MUTE        (- 201) if no communication is possible.
*  - GE_HOST_PORT_ABS   (- 401) if port is not found on host or is locked by
*       another device.
*  - GE_HOST_PORT_OS    (- 410) if a unexpected value has been returned by the
*       operating system.
*  - GE_HOST_PORT_OPEN  (- 411) if the port is already opened.
*  - GE_HOST_MEMORY     (- 420) if a memory allocation fails.
*  - GE_HOST_PARAMETERS (- 450) if one of the given parameters is out of the
*    allowed range or is not supported by hardware.
*  - GE_UNKNOWN_PB      (-1000) if an unexpected problem is encountered.
*
Extern var  :
-------------
	Nothing.

Global var  :
-------------
	Nothing.
*******************************************************************************/
INT16 G_DECL G_Oros3OpenComm
(
	const INT16 port_no,
	WORD32 BaudRate
)
{
	/*------------------------------------------------------------------------------
Local variables:
	- comm is used to open a serial communication channel.
	- r_buff and r_len are used to read IFD responses.
	- tx_size, rx_size and status are used to read port state.
	- timeout detects broken communication.
	- i is a loop index.
	- portcom memorises the opened port.
	- response holds the called function responses.
	------------------------------------------------------------------------------*/
	TGTSER_PORT
	comm;
	BYTE
	r_buff[HOR3GLL_OS_STRING_SIZE];
	WORD16
	r_len;
	WORD32
	end_time;
	INT16
	portcom,
		response;
	
	comm.Port     = port_no; //(WORD16) Param->Comm.Serial.Port;
	comm.BaudRate = 9600;
	//comm.ITNumber = (WORD16) Param->Comm.Serial.ITNumber;
	comm.Mode     = HGTSER_WORD_8 + HGTSER_NO_PARITY + HGTSER_STOP_BIT_1;
	comm.TimeOut  = HOR3COMM_CHAR_TIMEOUT;
	comm.TxSize   = HGTGBP_MAX_BUFFER_SIZE;
	comm.RxSize   = HGTGBP_MAX_BUFFER_SIZE;
	
	response = portcom = G_SerPortOpen(&comm);
	
	D(bug("G_SerPortOpenCalled\n"));
	
	if (response < G_OK)
	{
		return (response);
	}
	
	D(bug("G_SerPortOpenCalled-checked resp\n"));
	/*------------------------------------------------------------------------------
		The Gemplus Block Protocol is initialized.
	------------------------------------------------------------------------------*/
	G_GBPOpen(2,4,portcom);
	
	
	D(bug("Come after G_GBPOpen\n"));
	
	do
	{
		end_time = G_EndTime(HOR3COMM_CHAR_TIME);
		while (G_CurrentTime() < end_time)
		{
		}
		r_len = HOR3GLL_IFD_LEN_VERSION+1;
		response = G_Oros3Exchange
		(
			HOR3GLL_LOW_TIME,
			5,
			(BYTE G_FAR *)("\x22\x05\x3F\xE0\x0D"),
			&r_len,
			r_buff
		);
		D(bug("In Or3 Comm Opn: resp=%d\n", response));
		if (response < G_OK)
		{
			if (comm.BaudRate == 9600lu)
			{
				comm.BaudRate = 19200lu;
			}
			else if (comm.BaudRate == 19200lu)
			{
				comm.BaudRate = 38400lu;
			}
			else
			{
				G_GBPClose();
				
				G_SerPortClose(portcom);
				
				return (GE_IFD_MUTE);
			}
			response = G_SerPortSetState(&comm);
			
			if (response < G_OK)
			{
				G_GBPClose();
				
				G_SerPortClose(portcom);
				
				return (response);
			}
		}
		else
		{
			break;
		}
	} while (r_len != (HOR3GLL_IFD_LEN_VERSION+1));
	
	return (G_OK);
}

INT16 G_DECL G_ChangeIFDBaudRate
(
	const INT16 port_no,
	DWORD baud_rate
)
{
	DWORD br;
	
	TGTSER_PORT
	comm;
	
	WORD16
	rlen;
	WORD8
	rbuff[HOR3GLL_BUFFER_SIZE];
	INT16 response;
	
	comm.Port     = port_no;
	comm.BaudRate = baud_rate;
	comm.Mode     = HGTSER_WORD_8 + HGTSER_NO_PARITY + HGTSER_STOP_BIT_1;
	comm.TimeOut  = HOR3COMM_CHAR_TIMEOUT;
	comm.TxSize   = HGTGBP_MAX_BUFFER_SIZE;
	comm.RxSize   = HGTGBP_MAX_BUFFER_SIZE;
	
	for(br = baud_rate; br >= 9600lu; br = br / 2)
	{
		
		// --------------------------------------------------------------
		// The reader is switched to the selected value (G_Oros3SIOConfigure).
		//  The function status is not tested because, as the IFD has switched
		//  immediatly, it is not possible to read its response.
		// ----------------------------------------------------------------
		
		rlen = HOR3GLL_BUFFER_SIZE;
		G_Oros3SIOConfigure
		(
			HOR3GLL_LOW_TIME,
			0,
			8,
			br,
			&rlen,
			rbuff
		);
		
		// ----------------------------------------------------------------
		//      Host is switched to the selected value (G_SerPortSetState).
		//      If this call is successful,
		//      Then
		//         The last SIO command is re-sent to read the IFD response.
		// response is optionnaly initialized with the translated IFD status.
		// ----------------------------------------------------------------
		
		comm.BaudRate = br;
		response = G_SerPortSetState(&comm);
		if (response == G_OK)
		{
			rlen = HOR3GLL_BUFFER_SIZE;
			response = G_Oros3SIOConfigure
			(
				HOR3GLL_LOW_TIME,
				0,
				8,
				comm.BaudRate,
				&rlen,
				rbuff
			);
			if (response >= G_OK)
			{
				response = GE_Translate(rbuff[0]);
				break;
			}
		}
	}
	
	if ( (br < 9600) || (response != G_OK) )
	{
		return (GE_HI_COMM);
	}
	D(bug("%d Baud Rate set\n", br));
	return (G_OK);
}


