/*****************************************************************************

     File: MetaPositionIO.cpp

	 Written By: Dianne Hackborn

     Copyright (c) 2001 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/

#include <MetaPositionIO.h>
#include <Message.h>
#include <String.h>

#include <Debug.h>

#include <string.h>
#include <unistd.h>

#include <new>

BMetaPositionIO::BMetaPositionIO(BPositionIO* target, bool ownsTarget)
{
	fTarget = target;
	fOwnsTarget = ownsTarget;
	fMetaData = NULL;
}

// ----------------------------------------------------------------- //

BMetaPositionIO::BMetaPositionIO(const BMetaPositionIO &o)
	:	BPositionIO()
{
	fTarget = o.fTarget;
	fOwnsTarget = false;
	if (o.fMetaData)
		fMetaData = new(std::nothrow) BMessage(*o.fMetaData);
	else
		fMetaData = NULL;
}
	
// ----------------------------------------------------------------- //

BMetaPositionIO::~BMetaPositionIO()
{
	if (fOwnsTarget)
		delete fTarget;
	delete fMetaData;
}

/*-------------------------------------------------------------*/

BMetaPositionIO &BMetaPositionIO::operator=(const BMetaPositionIO &o)
{
	if (this != &o) {
		if (fOwnsTarget)
			delete fTarget;
		fTarget = o.fTarget;
		fOwnsTarget = false;
		if (o.fMetaData) {
			if (!fMetaData)
				fMetaData = new(std::nothrow) BMessage(*o.fMetaData);
			else
				*fMetaData = *o.fMetaData;
		} else {
			delete fMetaData;
			fMetaData = NULL;
		}
	}
	return *this;
}

// ----------------------------------------------------------------- //

ssize_t BMetaPositionIO::Read(void *buffer, size_t size)
{
	if (fTarget) return fTarget->Read(buffer, size);
	else return 0;
}

// ----------------------------------------------------------------- //

ssize_t BMetaPositionIO::Write(const void *buffer, size_t size)
{
	if (fTarget) return fTarget->Write(buffer, size);
	else return 0;
}

// ----------------------------------------------------------------- //

ssize_t BMetaPositionIO::ReadAt(off_t pos, void *buffer, size_t size)
{
	if (fTarget) return fTarget->ReadAt(pos, buffer, size);
	else return 0;
}

// ----------------------------------------------------------------- //

ssize_t BMetaPositionIO::WriteAt(off_t pos, const void *buffer, size_t size)
{
	if (fTarget) return fTarget->WriteAt(pos, buffer, size);
	else return 0;
}

// ----------------------------------------------------------------- //

off_t BMetaPositionIO::Seek(off_t pos, uint32 seek_mode)
{
	if (fTarget) return fTarget->Seek(pos, seek_mode);
	else return 0;
}

// ----------------------------------------------------------------- //

off_t	BMetaPositionIO::Position() const
{
	if (fTarget) return fTarget->Position();
	else return 0;
}

/*-------------------------------------------------------------*/

status_t BMetaPositionIO::SetSize(off_t size)
{
	if (fTarget) return fTarget->SetSize(size);
	else return B_UNSUPPORTED;
}

// ----------------------------------------------------------------- //

ssize_t BMetaPositionIO::MetaWrite(	const char *in_name, type_code in_type,
									int32 in_index, off_t in_offset,
									const void *in_buf, size_t in_size)
{
	if (in_offset != 0) return B_UNSUPPORTED;
	
	if (!fMetaData) {
		fMetaData = new(std::nothrow) BMessage;
		if (!fMetaData) return B_NO_MEMORY;
	}
	
	ssize_t result = fMetaData->ReplaceData(in_name, in_type, in_index, in_buf, in_size);
	if (result < B_OK) {
		type_code type;
		int32 count;
		result = fMetaData->GetInfo(in_name, &type, &count);
		if (result < B_OK) count = 0;
		if (in_index != count)
			result = B_BAD_INDEX;
		else
			result = fMetaData->AddData(in_name, in_type, in_buf, in_size);
	}
	
	return result;
}

ssize_t BMetaPositionIO::MetaRead(	const char *in_name, type_code in_type,
									int32 in_index, off_t in_offset,
									void *out_buf, size_t in_size) const
{
	if (!fMetaData)
		return B_NAME_NOT_FOUND;
	
	const void* data;
	ssize_t size;
	ssize_t result = fMetaData->FindData(in_name, in_type, in_index, &data, &size);
	if (result >= B_OK) {
		if (size > in_offset) {
			result = (ssize_t)(size-in_offset);
			if (result > (ssize_t)in_size) result = in_size;
			memcpy(out_buf, ((uint8*)data)+in_offset, result);
		} else {
			result = 0;
		}
	}
	
	return result;
}

status_t BMetaPositionIO::MetaRemove(	const char *in_name, int32 in_index)
{
	status_t result = B_OK;
	if (fMetaData) {
		if (in_name) {
			if (in_index >= 0)
				result = fMetaData->RemoveData(in_name, in_index);
			else
				result = fMetaData->RemoveName(in_name);
		} else {
			delete fMetaData;
			fMetaData = NULL;
		}
	}
	return result;
}

status_t BMetaPositionIO::MetaGetInfo(	const char *in_name, int32 in_index,
										meta_info *out_info, BString *out_name,
										void **inout_cookie) const
{
	status_t result = B_OK;
	
	type_code type = B_ANY_TYPE;
	int32 count = 0;
	const void* data;
	ssize_t size = 0;
	
	if (in_name) {
		if (!fMetaData)
			return B_NAME_NOT_FOUND;
		
		result = fMetaData->GetInfo(in_name, &type, &count);
		if (result >= B_OK)
			result = fMetaData->FindData(in_name, type,
										(in_index>=0) ? in_index:0, &data, &size);
		if (out_name)
			*out_name = in_name;
	
	} else {
		if (!fMetaData)
			return B_BAD_VALUE;
		if (!inout_cookie)
			return B_BAD_VALUE;
		
		const char* name;
		type_code type;
		int32 count;
		result = fMetaData->GetNextName(inout_cookie, &name, &type, &count);
		if (result >= B_OK) {
			if (out_name)
				*out_name = name;
			if (out_info) {
				const void* data;
				ssize_t size;
				result = fMetaData->FindData(name, type, 0, &data, &size);
			}
		}
	}
	
	if (out_info) {
		out_info->type = type;
		out_info->count = count;
		out_info->size = size;
	}
	
	return result;
}

BMessage* BMetaPositionIO::MetaData()
{
	if (!fMetaData) fMetaData = new(std::nothrow) BMessage;
	return fMetaData;
}

/* ---------------------------------------------------------------- */

void BMetaPositionIO::_ReservedMetaPositionIO1() {}
void BMetaPositionIO::_ReservedMetaPositionIO2() {}

// ----------------------------------------------------------------- //
// ----------------------------------------------------------------- //
// ----------------------------------------------------------------- //
