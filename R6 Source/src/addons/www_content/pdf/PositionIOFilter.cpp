#include "PositionIOFilter.h"
#include <stdio.h>
//
//  Input
//
BPositionIOInputFilter::BPositionIOInputFilter(BPositionIO *source, ssize_t limit)
	: BInputFilter(0), m_io(source), m_limit(limit), m_totalRead(0)
{
	m_resetPosition = m_io ? m_io->Position() : 0;
}


BPositionIOInputFilter::~BPositionIOInputFilter()
{
}

ssize_t 
BPositionIOInputFilter::Read(void *buffer, size_t size)
{
	//printf("BPositionIOInputFilter::Read(%ld)\n", size);
	size_t bytesToRead = m_limit - m_totalRead;
	//printf("m_limit: %ld m_totalRead: %ld bytesToRead: %ld\n", m_limit, m_totalRead, bytesToRead);
	
	if (bytesToRead == 0) {
		printf("no more to read!\n");
		return BInputFilter::END_OF_INPUT;
	}
	
	if (size < bytesToRead) bytesToRead = size;
	ssize_t bytesRead = m_io ? m_io->Read(buffer, bytesToRead) : BInputFilter::END_OF_INPUT;
	if (bytesRead > 0) m_totalRead += bytesRead;
	//printf("read: %ld  totalRead: %ld\n", bytesRead, m_totalRead);
	if (bytesRead == 0) bytesRead = BInputFilter::END_OF_INPUT;
	return bytesRead;
}

void 
BPositionIOInputFilter::Reset(void)
{
	if (m_io)
	{
		m_io->Seek(m_resetPosition, 0);
		m_totalRead = 0;
	}
}

off_t 
BPositionIOInputFilter::Seek(off_t position, uint32 seek_mode)
{
	// FIXME: is this the behaviour we want?
	m_resetPosition = position;
	// this is for sure
	return m_io ? m_io->Seek(position, seek_mode) : 0;
}

ssize_t 
BPositionIOInputFilter::GetLimit(void)
{
	return m_limit;
}

ssize_t 
BPositionIOInputFilter::SetLimit(ssize_t limit)
{
	ssize_t old_limit = m_limit;
	m_limit = limit;
	return old_limit;
}

BPositionIO *
BPositionIOInputFilter::GetIO(void)
{
	return m_io;
}

BPositionIO *
BPositionIOInputFilter::SetIO(BPositionIO *new_source)
{
	BPositionIO *old_source = m_io;
	m_io = new_source;
	return old_source;
}


//
//  Output
//
BPositionIOOutputFilter::BPositionIOOutputFilter(BPositionIO *sink)
	: BOutputFilter(0), m_sink(sink)
{
	m_resetPosition = m_sink ? m_sink->Position() : 0;
}


BPositionIOOutputFilter::~BPositionIOOutputFilter()
{
}

ssize_t 
BPositionIOOutputFilter::Write(void *buffer, size_t size)
{
	return m_sink ? m_sink->Write(buffer, size) : BOutputFilter::END_OF_OUTPUT;
}

void 
BPositionIOOutputFilter::Reset(void)
{
}

