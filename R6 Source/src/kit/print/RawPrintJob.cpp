//******************************************************************************
//
//	File:		RawPrintJob.cpp
//
//	Written by:	Mathias Agopian
//
//	Copyright 2001, Be Incorporated
//
//******************************************************************************


#ifndef _DEBUG_H
#	include <Debug.h>
#endif

#include <BeBuild.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include <FindDirectory.h>
#include <File.h>
#include <Directory.h>
#include <Path.h>
#include <String.h>
#include <Errors.h>

// Our own include file
#include <print/RawPrintJob.h>

//------------------------------------------------------------------
// Usefull defines
//------------------------------------------------------------------

#define m _m_rprivate

#define DEBUG 1

#if DEBUG
#	define D(_x)	_x
#else
#	define D(_x)
#endif

#define E(_x)	_x
#define bug		printf


//------------------------------------------------------------------
// Defined types
//------------------------------------------------------------------

namespace BPrivate
{	
	struct raw_printjob_private
	{
		BPrintJob *job;
	};
} using namespace BPrivate;

//------------------------------------------------------------------

BRawPrintJob::BRawPrintJob(const char *printer = NULL, const char *job_name = NULL)
	:	_m_private(new raw_printjob_private),
		m(*_m_private)
{
	m.job = new BPrintJob(printer, job_name);
}

BRawPrintJob::~BRawPrintJob()
{
	delete m.job;
	delete _m_private;
}

status_t BRawPrintJob::InitCheck() const
{
	return m.job->InitCheck();
}

//------------------------------------------------------------------
// #pragma mark -

status_t BRawPrintJob::BeginJob()
{
	return m.job->BeginJob();
}

//------------------------------------------------------------------

ssize_t BRawPrintJob::Print(const void *buffer, size_t length)
{
	status_t status;
	if ((status = InitCheck()) != B_OK)
		return status;
	if (m.job->spool_file() == NULL)
		return B_UNSUPPORTED;
	return m.job->spool_file()->Write(buffer, length);
}

//------------------------------------------------------------------

status_t BRawPrintJob::CommitJob()
{
	return m.job->CommitJob();
}

//------------------------------------------------------------------

bool BRawPrintJob::CanContinue()
{
	return m.job->CanContinue();
}

//------------------------------------------------------------------

void BRawPrintJob::CancelJob()
{
	m.job->CancelJob();
}

//------------------------------------------------------------------
// #pragma mark -

void BRawPrintJob::_ReservedRawPrintJob1() {}
void BRawPrintJob::_ReservedRawPrintJob2() {}
void BRawPrintJob::_ReservedRawPrintJob3() {}
void BRawPrintJob::_ReservedRawPrintJob4() {}
