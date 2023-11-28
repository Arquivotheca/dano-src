/******************************************************************************
/
/	File:			ResourceDB.h
/
/	Description:	PC/SC Defined types.
/
/	Copyright 1993-2000, Be Incorporated
/
******************************************************************************/

#ifndef RESOURCEDB_H
#define RESOURCEDB_H

#include <SmartCardDefs.h>
#include <ResourceManager.h>
#include <ResourceDBQuery.h>

class RESOURCEDB : public RESOURCEDBQUERY
{
public:

				RESOURCEDB(RESOURCEMANAGER *resmgr);
	virtual		~RESOURCEDB();

	RESPONSECODE	IntroduceReader			(	const STR& ReaderName,	// Pretty name of the reader
												const STR& DeviceName,	// Leaf() of the add-on for this reader
												DWORD channelId);		// channel id (eg: 0x00010001) for /dev/ports/serial1

	RESPONSECODE	ForgetReader			(const STR& ReaderName);

	RESPONSECODE	IntroduceReaderGroup	(const STR& GroupName);

	RESPONSECODE	ForgetReaderGroup		(const STR& GroupName);

	RESPONSECODE	AddReaderToGroup		(const STR& ReaderName, const STR& GroupName);

	RESPONSECODE	RemoveReaderFromGroup	(const STR& ReaderName, const STR& GroupName);

	RESPONSECODE	IntroduceCardType		(	const STR& CardName,
												const BYTE *ATRRefVal,
												const BYTE *ATRMask,
												const BYTE ATRMaskSize,
												const STR& ProviderId,
												const GUID_LIST& Interfaces );

	RESPONSECODE	ForgetCardType			(const STR& CardName);

private:
	RESOURCEDB();
	RESOURCEDB(const RESOURCEDB&);
	RESOURCEDB& operator=(const RESOURCEDB&);
 
 	RESOURCEMANAGER *resmgr_;
	uint32 _reserved_RESOURCEDB_[16];
};

#endif
