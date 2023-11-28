#ifndef _READER_H
#define _READER_H

#include <image.h>
#include <SmartCardDefs.h>
#include <ResourceManager.h>

class READER
{
friend class SCARDCOMM;
public:
				READER(const STR& ReaderName, const DWORD Flags);
	virtual 	~READER();
	virtual	RESPONSECODE InitCheck();

public:
	RESPONSECODE (*IO_create_channel)			(DWORD channelId);
	RESPONSECODE (*IO_close_channel)			();
	RESPONSECODE (*IFD_get_capabilities)		(DWORD Tag, BYTE Value[]);
	RESPONSECODE (*IFD_set_capabilities)		(DWORD Tag, const BYTE Value[]);
	RESPONSECODE (*IFD_set_protocol_parameters)	(DWORD ProtocolType, BYTE SelectionFlags, BYTE PTS1, BYTE PTS2, BYTE PTS3);
	RESPONSECODE (*IFD_power_icc)				(DWORD ActionRequested);
	RESPONSECODE (*IFD_swallow_icc)				();
	RESPONSECODE (*IFD_eject_icc)				();
	RESPONSECODE (*IFD_confiscate_icc)			();
	RESPONSECODE (*IFD_transmit_to_icc)			(const struct SCARD_IO_HEADER SendPci, 
												const BYTE CommandData[], DWORD CommandSize,
												BYTE ResponseData[], DWORD *ResponseSize,
												struct SCARD_IO_HEADER *RecvPci );
	RESPONSECODE (*IFD_is_icc_present)			();
	RESPONSECODE (*IFD_is_icc_absent)			();

private:
	STR fReaderName;
	RESPONSECODE fError;
	image_id fImageId;
};

#endif
