/*****************************************************************************

     File: DataIO.cpp

	 Written By: Peter Potrebic

     Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/

#include <DataIO.h>
#include <Message.h>

#include <Debug.h>

#include <string.h>
#include <unistd.h>

#include <new>

// ----------------------------------------------------------------- //

			BDataIO::BDataIO()
{
}

// ----------------------------------------------------------------- //

			BDataIO::~BDataIO()
{
}

// ----------------------------------------------------------------- //

ssize_t BDataIO::MetaWrite(	const char */*in_name*/, type_code /*in_type*/,
							int32 /*in_index*/, off_t /*in_offset*/,
							const void */*in_buf*/, size_t /*in_size*/)
{
	return B_UNSUPPORTED;
}

ssize_t BDataIO::MetaRead(	const char */*in_name*/, type_code /*in_type*/,
							int32 /*in_index*/, off_t /*in_offset*/,
							void */*out_buf*/, size_t /*in_size*/) const
{
	return B_UNSUPPORTED;
}

status_t BDataIO::MetaRemove(	const char */*in_name*/, int32 /*in_index*/)
{
	return B_UNSUPPORTED;
}

status_t BDataIO::MetaGetInfo(	const char */*in_name*/, int32 /*in_index*/,
								meta_info */*out_info*/, BString */*out_name*/,
								void **/*inout_cookie*/) const
{
	return B_UNSUPPORTED;
}

BDataIO::BDataIO(const BDataIO &) {}
BDataIO &BDataIO::operator=(const BDataIO &) { return *this; }

/* ---------------------------------------------------------------- */

#if !_PR3_COMPATIBLE_
void 
BDataIO::_ReservedDataIO5()
{
}

void 
BDataIO::_ReservedDataIO6()
{
}

void 
BDataIO::_ReservedDataIO7()
{
}

void 
BDataIO::_ReservedDataIO8()
{
}

void 
BDataIO::_ReservedDataIO9()
{
}

void 
BDataIO::_ReservedDataIO10()
{
}

void 
BDataIO::_ReservedDataIO11()
{
}

void 
BDataIO::_ReservedDataIO12()
{
}

#endif

#if _R5_COMPATIBLE_
extern "C" {

	_EXPORT void
	#if __GNUC__
	_ReservedDataIO1__7BDataIO
	#elif __MWERKS__
	_ReservedDataIO1__7BDataIOFv
	#endif
	(BDataIO* This,	const char *in_name, type_code in_type,
					int32 in_index, off_t in_offset,
					const void *in_buf, size_t in_size)
	{
		This->BDataIO::MetaWrite(in_name, in_type, in_index, in_offset, in_buf, in_size);
	}
	
	_EXPORT void
	#if __GNUC__
	_ReservedDataIO2__7BDataIO
	#elif __MWERKS__
	_ReservedDataIO2__7BDataIOFv
	#endif
	(BDataIO* This,	const char *in_name, type_code in_type,
					int32 in_index, off_t in_offset,
					void *out_buf, size_t in_size)
	{
		This->BDataIO::MetaRead(in_name, in_type, in_index, in_offset, out_buf, in_size);
	}
	
	_EXPORT void
	#if __GNUC__
	_ReservedDataIO3__7BDataIO
	#elif __MWERKS__
	_ReservedDataIO3__7BDataIOFv
	#endif
	(BDataIO* This,	const char *in_name, int32 in_index)
	{
		This->BDataIO::MetaRemove(in_name, in_index);
	}
	
	_EXPORT void
	#if __GNUC__
	_ReservedDataIO4__7BDataIO
	#elif __MWERKS__
	_ReservedDataIO4__7BDataIOFv
	#endif
	(BDataIO* This,	const char *in_name, int32 in_index,
					meta_info *out_info, BString *out_name,
					void **inout_cookie)
	{
		This->BDataIO::MetaGetInfo(in_name, in_index, out_info, out_name, inout_cookie);
	}
	
}
#endif

// ----------------------------------------------------------------- //
// ----------------------------------------------------------------- //
// ----------------------------------------------------------------- //


BPositionIO::BPositionIO()
{}

// ----------------------------------------------------------------- //

BPositionIO::~BPositionIO()
{}

// ----------------------------------------------------------------- //

ssize_t		BPositionIO::Write(const void *buf, size_t size)
{
	ssize_t		res;

	res = WriteAt(Position(), buf, size);
	if (res >= 0)
		Seek(Position() + res, SEEK_SET);
	return res;
}

// ----------------------------------------------------------------- //

ssize_t		BPositionIO::Read(void *buf, size_t size)
{
	ssize_t		res;

	res = ReadAt(Position(), buf, size);
	if (res >= 0)
		Seek(Position() + res, SEEK_SET);
	return res;
}

/*-------------------------------------------------------------*/

status_t BPositionIO::SetSize(off_t size)
{
	/* FBC fix to re-route to newly overridden function */
	BMemoryIO * that = dynamic_cast<BMemoryIO*>(this);
	if (that != NULL) {
		return that->BMemoryIO::SetSize(size);
	}
	return B_ERROR;
}

/*-------------------------------------------------------------*/

void 
BPositionIO::_ReservedPositionIO1()
{
}

void 
BPositionIO::_ReservedPositionIO2()
{
}

void 
BPositionIO::_ReservedPositionIO3()
{
}

void 
BPositionIO::_ReservedPositionIO4()
{
}

#if !_PR3_COMPATIBLE_

void 
BPositionIO::_ReservedPositionIO5()
{
}

void 
BPositionIO::_ReservedPositionIO6()
{
}

void 
BPositionIO::_ReservedPositionIO7()
{
}

void 
BPositionIO::_ReservedPositionIO8()
{
}

void 
BPositionIO::_ReservedPositionIO9()
{
}

void 
BPositionIO::_ReservedPositionIO10()
{
}

void 
BPositionIO::_ReservedPositionIO11()
{
}

void 
BPositionIO::_ReservedPositionIO12()
{
}

#endif

// ----------------------------------------------------------------- //
// ----------------------------------------------------------------- //
// ----------------------------------------------------------------- //

BMallocIO::BMallocIO()
{
	fData = NULL;
	fBlockSize = 256;
	fLength = 0;
	fMallocSize = 0;
	fPosition = 0;
}

// ----------------------------------------------------------------- //

BMallocIO::BMallocIO(const BMallocIO &o)
	:	BPositionIO()
{
	Copy(o);
}
	
// ----------------------------------------------------------------- //

BMallocIO::~BMallocIO()
{
	if (fData)
		free(fData);
}

/*-------------------------------------------------------------*/

BMallocIO &BMallocIO::operator=(const BMallocIO &o)
{
	if (this != &o) {
		if (fData)
			free(fData);
		Copy(o);
	}
	return *this;
}

// ----------------------------------------------------------------- //

size_t BMallocIO::BufferLength() const
{
	return fLength;
}

// ----------------------------------------------------------------- //

const void *	BMallocIO::Buffer() const
{
	return fData;
}

// ----------------------------------------------------------------- //

const char *	BMallocIO::AsString() const
{
	if (!fData)
		return "";
	if (fMallocSize > fLength) {
		fData[fLength] = 0;
		return (const char*)fData;
	}
	
	// Need to grow buffer to have room for termination.
	BMallocIO* This = const_cast<BMallocIO*>(this);
	size_t newsize = ((fLength + 1 + (fBlockSize-1)) / fBlockSize) * fBlockSize;
	char *newdata = (char *) realloc(fData, newsize);
	if (!newdata)
		return NULL;
	This->fData = newdata;
	This->fMallocSize = newsize;
	fData[fLength] = 0;
	return (const char*)fData;
}

/*-------------------------------------------------------------*/

int BMallocIO::Compare(const BMallocIO &o) const
{
	if (this == &o || fData == o.fData)
		return 0;
	if (fData == NULL)
		return -1;
	if (o.fData == NULL)
		return 1;
	const int cmp = memcmp(fData, o.fData, fLength < o.fLength ? fLength : o.fLength);
	if (cmp == 0 && fLength != o.fLength)
		return (fLength < o.fLength ? -1 : 1);
	return cmp;
}

// ----------------------------------------------------------------- //

void BMallocIO::SetBlockSize(size_t newsize)
{
  fBlockSize = newsize;
}

// ----------------------------------------------------------------- //

ssize_t BMallocIO::ReadAt(off_t pos, void *buffer, size_t size)
{
	if (pos > fLength)
		return 0;

	if (pos + size > fLength)
		size = fLength - pos;

	memcpy(buffer, fData + pos, size);
	return size;
}

// ----------------------------------------------------------------- //

ssize_t BMallocIO::WriteAt(off_t pos, const void *buffer, size_t size)
{
	char		*newdata;
	size_t		newsize;

	if (pos + size > fLength) {
		if (pos + size > fMallocSize) {
			newsize = ((pos + size + (fBlockSize-1)) / fBlockSize) * fBlockSize;
			newdata = (char *) realloc(fData, newsize);
			if (!newdata)
				return ENOMEM;
			fData = newdata;
			fMallocSize = newsize;
		}
		fLength = pos + size;
	}

	memcpy(fData + pos, buffer, size);
	return size;
}

// ----------------------------------------------------------------- //

off_t BMallocIO::Seek(off_t pos, uint32 seek_mode)
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

off_t	BMallocIO::Position() const
{
	return fPosition;
}

/*-------------------------------------------------------------*/

status_t BMallocIO::SetSize(off_t size)
{

	if (size > fLength) {
		if (size > fMallocSize) {
			char	*newdata;
			size_t	newsize;
			newsize = ((size + (fBlockSize-1)) / fBlockSize) * fBlockSize;
			newdata = (char *) realloc(fData, newsize);
			if (!newdata)
				return ENOMEM;
			fData = newdata;
			fMallocSize = newsize;
		}
		fLength = size;
	} else if (size < fLength) {
		// don't bother reallocing, just shrink the logical size.
		fLength = size;

		// ??? Is this the right thing to do?
		// if (fPosition > fLength)
		//	fPosition = fLength;
		// NO -- hplus
	}
	return B_OK;
}

// ----------------------------------------------------------------- //

void BMallocIO::Copy(const BMallocIO &o)
{
	fBlockSize = o.fBlockSize;
	
	if (o.fData) {
		fData = (char*)malloc(o.fMallocSize);
		if (fData) {
			memcpy(fData, o.fData, o.fLength);
			fMallocSize = o.fMallocSize;
			fLength = o.fLength;
			fPosition = o.fPosition;
		}
	} else {
		fData = NULL;
	}
	
	if (!fData) {
		fMallocSize = 0;
		fLength = 0;
		fPosition = 0;
	}
}
	
/* ---------------------------------------------------------------- */

void BMallocIO::_ReservedMallocIO1() {}
void BMallocIO::_ReservedMallocIO2() {}

// ----------------------------------------------------------------- //
// ----------------------------------------------------------------- //
// ----------------------------------------------------------------- //

BMemoryIO::BMemoryIO(void *p, size_t len)
{
	fReadOnly = false;
	fBuf = (char *) p;
	fLen = len;
	fPos = 0;
	fPhys = len;
}

BMemoryIO::BMemoryIO(const void *p, size_t len)
{
	fReadOnly = true;
	fBuf = (char *) p;
	fLen = len;
	fPos = 0;
	fPhys = len;
}

// ----------------------------------------------------------------- //

BMemoryIO::~BMemoryIO()
{
}

// ----------------------------------------------------------------- //

ssize_t BMemoryIO::ReadAt(off_t pos, void *buffer, size_t size)
{
	if (pos >= fLen)
		return 0;

	if (pos + size > fLen)
		size = fLen - pos;

	memcpy(buffer, fBuf + pos, size);
	return size;
}

// ----------------------------------------------------------------- //

ssize_t BMemoryIO::WriteAt(off_t pos, const void *buffer, size_t size)
{
	if (fReadOnly)
		return EPERM;

	if (pos >= fPhys)
		return 0;

	if (pos + size > fPhys)
		size = fPhys - pos;
	memcpy(fBuf + pos, buffer, size);
	if (pos+size > fLen) {
		fLen = pos+size;
	}
	return size;
}

// ----------------------------------------------------------------- //

off_t BMemoryIO::Seek(off_t pos, uint32 seek_mode)
{
	switch (seek_mode) {
		case SEEK_SET:
			fPos = pos;
			break;
		case SEEK_CUR:
			fPos = fPos + pos;
			break;
		case SEEK_END:
			fPos = fLen + pos;
			break;
	}
	return fPos;
}

// ----------------------------------------------------------------- //

status_t BMemoryIO::SetSize(off_t size)
{
	if (fReadOnly) {
		return EPERM;
	}
	if ((size > fPhys) || (size < 0)) {
		return B_ERROR;
	}
	fLen = size;
	return B_OK;
}

// ----------------------------------------------------------------- //

off_t	BMemoryIO::Position() const
{
	return fPos;
}

/*-------------------------------------------------------------*/

BMemoryIO::BMemoryIO(const BMemoryIO &)
	:	BPositionIO()
	{}
BMemoryIO &BMemoryIO::operator=(const BMemoryIO &) { return *this; }

/* ---------------------------------------------------------------- */

void BMemoryIO::_ReservedMemoryIO1() {}
void BMemoryIO::_ReservedMemoryIO2() {}

// ----------------------------------------------------------------- //
// ----------------------------------------------------------------- //
// ----------------------------------------------------------------- //
