/*****************************************************************************

     File: PositionIO.cpp

     Copyright (c) 2001 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/

#include <support2/PositionIO.h>

#include <support2/Debug.h>

#include <string.h>
#include <unistd.h>

#include <new>

namespace B {
namespace Support2 {

/* ---------------------------------------------------------------- */

BPositionIO::BPositionIO(IStorage *This)
{
	m_store = This;
	m_pos = 0;
}

BPositionIO::BPositionIO(IStorage::arg store)
{
	m_store = store.ptr();
	m_store->Acquire(this);
	m_pos = 0;
}

// ----------------------------------------------------------------- //

BPositionIO::~BPositionIO()
{
	m_store->AttemptRelease(this);
}

// ----------------------------------------------------------------- //

BValue
BPositionIO::Inspect(const BValue &v, uint32 flags)
{
	return LByteInput::Inspect(v, flags)
			.Overlay(LByteOutput::Inspect(v, flags))
			.Overlay(LByteSeekable::Inspect(v, flags));
}

// ----------------------------------------------------------------- //

ssize_t
BPositionIO::WriteV(const struct iovec *vector, ssize_t count)
{
	ssize_t res;
	res = m_store->WriteAtV(m_pos,vector,count);
	if (res >= 0) m_pos += res;
	return res;
}

status_t 
BPositionIO::End()
{
	return m_store->SetSize(m_pos);
}

status_t 
BPositionIO::Sync()
{
	return m_store->Sync();
}

// ----------------------------------------------------------------- //

ssize_t
BPositionIO::ReadV(const struct iovec *vector, ssize_t count)
{
	ssize_t res;
	res = m_store->ReadAtV(m_pos,vector,count);
	if (res >= 0) m_pos += res;
	return res;
}

// ----------------------------------------------------------------- //

off_t 
BPositionIO::Seek(off_t position, uint32 seek_mode)
{
	if (seek_mode == SEEK_SET)
		m_pos = position;
	else if (seek_mode == SEEK_END)
		m_pos = m_store->Size() - position;
	else if (seek_mode == SEEK_CUR)
		m_pos += position;
	return m_pos;
}

off_t 
BPositionIO::Position() const
{
	return m_pos;
}

} }	// namespace B::Support2
