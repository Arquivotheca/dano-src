/******************************************************************************
/
/	File:			SmartCardComm.h
/
/	Description:	PC/SC Defined types.
/
/	Copyright 1993-2000, Be Incorporated
/
******************************************************************************/

#ifndef SCARDCOMM_H
#define SCARDCOMM_H

#include <OS.h>
#include <Locker.h>
#include <Messenger.h>
#include <image.h>

#include <SmartCardDefs.h>
#include <SmartCardAddOn.h>
#include <ResourceManager.h>

class ReaderLocker;
class READER;

class SCARDCOMM
{
public:

				SCARDCOMM(RESOURCEMANAGER *resmgr);
				~SCARDCOMM();

 	RESPONSECODE	Connect(	const STR& ReaderName,
								const DWORD Flags,
								const DWORD PreferredProtocols,
								DWORD *ActiveProtocol);

	RESPONSECODE	Reconnect(	const DWORD Flags,
								const DWORD PreferredProtocols,
								const DWORD Initialization,
								DWORD *ActiveProtocol);

	RESPONSECODE	Disconnect(	const DWORD Disposition);

	RESPONSECODE	Status(		STR *Reader = NULL,
								DWORD *State = NULL,
								DWORD *ActiveProtocol = NULL,
								BYTE *Atr = NULL);

	RESPONSECODE	BeginTransaction();

	RESPONSECODE	EndTransaction(const DWORD Disposition);

	RESPONSECODE	Cancel();

	RESPONSECODE	Transmit(	const SCARD_IO_HEADER& SendPci,
								const BYTE CommandData[],
								const DWORD CommandLength,
								SCARD_IO_HEADER *RecvPci,
								BYTE ResponseData[],
								DWORD *ResponseSize);

	RESPONSECODE	Control(	const DWORD ControlCode,
								const BYTE *InBuffer,
								BYTE *OutBuffer,
								DWORD *OutBufferLength);

	RESPONSECODE	GetReaderCapabilities(	const DWORD Tag,
											BYTE *Buffer,
											DWORD length);
											
	RESPONSECODE	SetReaderCapabilities(	const DWORD Tag,
											const BYTE *Buffer,
											DWORD length);


private:
	SCARDCOMM();
	SCARDCOMM(const SCARDCOMM&);
	SCARDCOMM& operator=(const SCARDCOMM&);
 
private:
	RESOURCEMANAGER *resmgr_;
	READER *hCard;
	ReaderLocker *fReaderLock;
	DWORD fActiveProtocol;
	BLocker fLock;
	STR fReaderName;
	uint32 _reserved_SCARDCOMM_[16];
};

#endif

