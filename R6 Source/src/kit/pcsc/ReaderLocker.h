/******************************************************************************
/
/	File:			ReaderLocker.h
/
/	Description:	Lock/unlock/cancel a reader.
/
/	Copyright 1993-99, Be Incorporated
/
******************************************************************************/


#ifndef _READER_LOCKER_H
#define _READER_LOCKER_H

#include <SupportDefs.h>

class ReaderLocker
{
public:
				ReaderLocker(const char *reader);
	virtual 	~ReaderLocker();
	
	status_t Lock(void);
	status_t Unlock(void);
	status_t Cancel(void);

private:
	const char *fReaderName;
};


#endif
