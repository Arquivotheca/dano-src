/******************************************************************************
/
/	File:			SmartCardAddOn.h
/
/	Description:	IFD Handler definition.
/
/	Copyright 1993-2000, Be Incorporated
/
******************************************************************************/

#ifndef _IFD_HANDLER_H
#define _IFD_HANDLER_H

#include <SmartCardTypes.h>


struct SCARD_IO_HEADER
{
	DWORD Protocol;
	DWORD Length;
};


// List of tags

enum
{
	TAG_IFD_ICC_PRESENCE			= 0x0300,
	TAG_IFD_ICC_INTERFACE_STATUS	= 0x0301,
	TAG_IFD_ATR						= 0x0303,
	TAG_IFD_ICC_TYPE				= 0x0304
};


// Defined constants
	
enum
{
	IFD_POWER_UP			= 500,
	IFD_POWER_DOWN			= 501,
	IFD_RESET				= 502
};

enum
{
	IFD_SUCCESS					= 0,
	IFD_ERROR_TAG				= 600,
	IFD_ERROR_SET_FAILURE		= 601,
	IFD_ERROR_VALUE_READ_ONLY	= 602,
	IFD_NEGOTIATE_PTS11			= 603,
	IFD_NEGOTIATE_PTS22			= 604,
	IFD_ERROR_PTS_FAILURE		= 605,
	IFD_ERROR_NOT_SUPPORTED		= 606,
	IFD_PROTOCOL_NOT_SUPPORTED	= 607,
	IFD_ERROR_POWER_ACTION		= 608,
	IFD_ERROR_SWALLOW			= 609,
	IFD_ERROR_EJECT				= 610,
	IFD_ERROR_CONFISCATE		= 611,
	IFD_COMMUNICATION_ERROR		= 612,
	IFD_RESPONSE_TIMEOUT		= 613,
	IFD_NOT_SUPPORTED			= 614,
	IFD_ICC_PRESENT				= 615,
	IFD_ICC_NOT_PRESENT			= 616
};


// Be functions

#ifdef __cplusplus
extern "C" {
#endif

_EXPORT status_t IFD_init_hardware	(const char **type);

// Functions provided by the handler

_EXPORT RESPONSECODE IO_Create_Channel			(DWORD channelId);
_EXPORT RESPONSECODE IO_Close_Channel			();

_EXPORT RESPONSECODE IFD_Get_Capabilities		(DWORD Tag, BYTE Value[]);
_EXPORT RESPONSECODE IFD_Set_Capabilities		(DWORD Tag, BYTE Value[]);
_EXPORT RESPONSECODE IFD_Set_Protocol_Parameters(DWORD ProtocolType, BYTE SelectionFlags, BYTE PTS1, BYTE PTS2, BYTE PTS3);
_EXPORT RESPONSECODE IFD_Power_ICC 				(DWORD ActionRequested);
_EXPORT RESPONSECODE IFD_Swallow_ICC			();
_EXPORT RESPONSECODE IFD_Eject_ICC				();
_EXPORT RESPONSECODE IFD_Confiscate_ICC			();
_EXPORT RESPONSECODE IFD_Transmit_to_ICC		(const struct SCARD_IO_HEADER SendPci, 
												const BYTE CommandData[], DWORD CommandSize,
												BYTE ResponseData[], DWORD *ResponseSize,
												struct SCARD_IO_HEADER *RecvPci);
_EXPORT RESPONSECODE IFD_Is_ICC_Present			();
_EXPORT RESPONSECODE IFD_Is_ICC_Absent			();

#ifdef __cplusplus
}
#endif


#endif

