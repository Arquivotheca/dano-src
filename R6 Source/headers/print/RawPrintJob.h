/*******************************************************************************
/
/	File:			RawPrintJob.h
/
/   Description:    BPrintJob runs a printing session.
/
/	Copyright 1996-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef	_RAW_PRINT_JOB_H
#define	_RAW_PRINT_JOB_H

#include <OS.h>
#include <PrintJob.h>

namespace BPrivate
{
	struct raw_printjob_private;
}

class BRawPrintJob
{
public:
			BRawPrintJob(const char *printer = NULL, const char *job_name = NULL);
	virtual	~BRawPrintJob();
	status_t InitCheck() const;

	// //////////////////////////////////////////////////////
	// Job creation / Spooling
	status_t	BeginJob();	
	ssize_t		Print(const void *buffer, size_t length);
	status_t	CommitJob();	
	void		CancelJob();
	bool		CanContinue();

//----- Private or reserved -----------------------------------------

private:
	virtual void _ReservedRawPrintJob1();
	virtual void _ReservedRawPrintJob2();
	virtual void _ReservedRawPrintJob3();
	virtual void _ReservedRawPrintJob4();

				BRawPrintJob(const BPrintJob &);
	BRawPrintJob&	operator = (const BRawPrintJob &);

private:
	BPrivate::raw_printjob_private *_m_private;
	BPrivate::raw_printjob_private& _m_rprivate;
	uint32 _reserved0[3];
};


#endif

