/******************************************************************************
/
/	File:			ResourceManager.h
/
/	Description:	PC/SC Defined types.
/
/	Copyright 1993-2000, Be Incorporated
/
******************************************************************************/

#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include <Locker.h>
#include <SmartCardDefs.h>


class READER;
class SCARDCOMM;

class RESOURCEMANAGER
{
public:

				RESOURCEMANAGER();
	virtual		~RESOURCEMANAGER();

	RESPONSECODE	EstablishContext	(	const DWORD Scope,
											const DWORD Reserved1,
											const DWORD Reserved2);

	RESPONSECODE	ReleaseContext		();


private:
	RESOURCEMANAGER(const RESOURCEMANAGER&);
	RESOURCEMANAGER& operator=(const RESOURCEMANAGER&);
  
private:
friend class SCARDCOMM;
friend class SCARDTRACK;
	RESPONSECODE	GetReader(	const STR& ReaderName,
								const DWORD Flags,
								READER **reader);

	RESPONSECODE	ReleaseReader(READER *reader);

private:
	HANDLE hContext_;
	BLocker fLock;
	uint32 _reserved_RESOURCEMANAGER_[16];
};

#endif
