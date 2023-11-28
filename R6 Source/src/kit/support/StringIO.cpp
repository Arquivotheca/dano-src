/*****************************************************************************

     File: StringIO.cpp

	 Written By: Dianne Hackborn

     Copyright (c) 2000 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/

#include <StringIO.h>

#include <Debug.h>
#include <String.h>

#include <string.h>
#include <unistd.h>

// ----------------------------------------------------------------- //
// ----------------------------------------------------------------- //
// ----------------------------------------------------------------- //

BStringIO::BStringIO()
{
	fBlockSize = 256;
	fMallocSize = 0;
	fLength = 0;
	fPosition = 0;
	fString = NULL;
	fData = NULL;
	fOwnsString = false;
}

BStringIO::BStringIO(BString* target)
{
	fBlockSize = 256;
	fMallocSize = 0;
	fLength = 0;
	fPosition = 0;
	fString = target;
	fData = NULL;
	fOwnsString = false;
}

// ----------------------------------------------------------------- //

BStringIO::~BStringIO()
{
	Reset();
}

// ----------------------------------------------------------------- //

ssize_t BStringIO::ReadAt(off_t pos, void *buffer, size_t size)
{
	if (pos > fLength)
		return 0;

	if (!fString)
		return 0;
	
	if (!fData)
		fData = fString->LockBuffer(fLength);
	
	if (pos + size > fLength)
		size = fLength - pos;

	memcpy(buffer, fData + pos, size);
	return size;
}

// ----------------------------------------------------------------- //

ssize_t BStringIO::WriteAt(off_t pos, const void *buffer, size_t size)
{
	if (pos + size > fLength) {
		if (pos + size > fMallocSize) {
			const size_t newsize = ((pos + size + (fBlockSize-1)) / fBlockSize)
									* fBlockSize;
			if (fData)
				fString->UnlockBuffer(fMallocSize);
			if (!fString) {
				fString = new BString;
				fOwnsString = true;
			}
			fData = fString->LockBuffer(newsize);
			if (!fData)
				return ENOMEM;
			fMallocSize = newsize;
		}
		fLength = pos + size;
	}

	if (!fString) {
		fString = new BString;
		fOwnsString = true;
	}
	if (!fData)
		fData = fString->LockBuffer(fMallocSize);
	
	memcpy(fData + pos, buffer, size);
	return size;
}

// ----------------------------------------------------------------- //

off_t BStringIO::Seek(off_t pos, uint32 seek_mode)
{
	switch (seek_mode) {
		case SEEK_SET:
			fPosition = pos;
			break;
		case SEEK_CUR:
			fPosition = fPosition + pos;
			break;
		case SEEK_END:
			fPosition = fLength + pos;
			break;
	}
	return fPosition;
}

// ----------------------------------------------------------------- //

off_t	BStringIO::Position() const
{
	return fPosition;
}

/*-------------------------------------------------------------*/

status_t BStringIO::SetSize(off_t size)
{

	if (size > fLength) {
		if (size > fMallocSize) {
			const size_t newsize = ((size + (fBlockSize-1)) / fBlockSize)
									* fBlockSize;
			if (fData)
				fString->UnlockBuffer(fMallocSize);
			if (!fString) {
				fString = new BString;
				fOwnsString = true;
			}
			fData = fString->LockBuffer(newsize);
			if (!fData)
				return ENOMEM;
			fMallocSize = newsize;
		}
		fLength = size;
	} else if (size < fLength) {
		// don't bother reallocing, just shrink the logical size.
		fLength = size;
	}
	return B_OK;
}

// ----------------------------------------------------------------- //

void BStringIO::SetBlockSize(size_t newsize)
{
  fBlockSize = newsize;
}

// ----------------------------------------------------------------- //

const char *	BStringIO::String() const
{
	if (!fString)
		return "";
	
	if (!fData)
		fData = fString->LockBuffer(fLength);
	if (!fData)
		return "";
	
	// Make sure string is \0-terminated.  This is safe because
	// BString always reserves room for the termination.
	fData[fLength] = 0;
	
	return fData;
}

// ----------------------------------------------------------------- //

size_t BStringIO::StringLength() const
{
	return fLength;
}

// ----------------------------------------------------------------- //

void BStringIO::Attach(BString* target)
{
	Reset();
	fString = target;
}

// ----------------------------------------------------------------- //

BString* BStringIO::Detach()
{
	if (fData) {
		fString->UnlockBuffer(fLength);
		fData = NULL;
	}
	
	BString* ret = fString;
	if (fOwnsString)
		delete fString;
	fString = NULL;
	
	fLength = 0;
	fPosition = 0;
	fOwnsString = false;
	
	return ret;
}

// ----------------------------------------------------------------- //

void BStringIO::Reset()
{
	if (fData) {
		fString->UnlockBuffer(fLength);
		fData = NULL;
	}
	
	if (fOwnsString)
		delete fString;
	fString = NULL;
	
	fLength = 0;
	fPosition = 0;
	fOwnsString = false;
}

/*-------------------------------------------------------------*/

BStringIO::BStringIO(const BStringIO &)
	:	BPositionIO()
	{}
BStringIO &BStringIO::operator=(const BStringIO &) { return *this; }

/* ---------------------------------------------------------------- */

void BStringIO::_ReservedStringIO1() {}
void BStringIO::_ReservedStringIO2() {}
void BStringIO::_ReservedStringIO3() {}
void BStringIO::_ReservedStringIO4() {}
void BStringIO::_ReservedStringIO5() {}
void BStringIO::_ReservedStringIO6() {}

// ----------------------------------------------------------------- //
// ----------------------------------------------------------------- //
// ----------------------------------------------------------------- //
