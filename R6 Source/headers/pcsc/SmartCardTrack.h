/******************************************************************************
/
/	File:			SmartCardTrack.h
/
/	Description:	PC/SC Defined types.
/
/	Copyright 1993-2000, Be Incorporated
/
******************************************************************************/

#ifndef SCARDTRACK_H
#define SCARDTRACK_H

#include <vector>

#include <SmartCardDefs.h>
#include <ResourceManager.h>
#include <ResourceDBQuery.h>

typedef std::vector<SCARD_READERSTATE> SCARD_READERSTATE_LIST;

class READER;

class SCARDTRACK
{
public:

				SCARDTRACK(RESOURCEMANAGER *resmgr);
				~SCARDTRACK();

	RESPONSECODE	LocateCards		(	const STR_LIST& Cards,
										SCARD_READERSTATE_LIST& ReaderStates);

	RESPONSECODE	GetStatusChange	(	SCARD_READERSTATE_LIST& ReaderStates,
										DWORD Timeout);

	RESPONSECODE	Cancel			();


private:
	SCARDTRACK();
	SCARDTRACK(const SCARDTRACK&);
	SCARDTRACK& operator=(const SCARDTRACK&);

private:
	typedef struct
	{
		READER *reader;
		RESPONSECODE result;
	} active_reader_t;

	RESPONSECODE	LocateCards		(	const STR_LIST& Cards,
										SCARD_READERSTATE_LIST& ReaderStates,
										bool try_atr_match);
	RESOURCEMANAGER *resmgr_;
	sem_id fCancelSem;

	uint32 _reserved_SCARDTRACK_[16];
};

#endif
