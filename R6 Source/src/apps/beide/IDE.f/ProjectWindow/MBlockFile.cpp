//========================================================================
//	MBlockFile.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
// JW

#include "IDEConstants.h"
#include "MBlockFile.h"
#include <StorageKit.h>
#include <string.h>
#include <Debug.h>

#define SEEK_ABSOLUTE 0

#define Read %error%
#define Write %error%
#define Seek %error%

const size_t	kBufferSize = 1024 * 16;

MBlockFile::MBlockFile()
	: fblockStack()
{
	//	Clear out
	fIsReading = false;
	fIsWriting = false;
	fFile = NULL;
	fBuffer = NULL;
}


MBlockFile::MBlockFile(
	const entry_ref& ref) :
	BEntry(&ref),
	fblockStack()
{
	//	Clear out
	fIsReading = false;
	fIsWriting = false;
	fFile = NULL;
	fBuffer = NULL;
}


MBlockFile::~MBlockFile()
{
	Close();	//	Flush everything
}


status_t
MBlockFile::ResetWrite(
	size_t atPosition,
	BlockType useType)
{
	//	Close pending
	status_t err = B_NO_ERROR;

	err = Reset();

	fBufferStart = atPosition;
	fBufferEnd = fBufferStart + kBufferSize;
	fBufferCurrEnd = fBufferStart;

	if (!err)
		fIsWriting = true;
	//	Start from given position
	if (!err)
		err = fFile->SetSize(atPosition);
	if (!err)
		err = Position(atPosition);
	if (!err)
		fPos = atPosition;

	//	Make the first "file" block
	uint32 type;

	if (B_HOST_IS_BENDIAN)
		type = 'MIDE';
	else
		type = 'EDIM';
	
	if (useType)
		type = useType;
	if (!err)
		err = StartBlock(type);

	return err;
}


status_t
MBlockFile::StartBlock(
	BlockType type)
{
	if (!fIsWriting)
		return INCONSISTENT_MODE;


	//	Remember info on new block
	BlockHeaderInfo		info;
	info.header.type = type;
	info.header.size = (uint32) -1;
	info.pos = GetPos();

	//	Create tag in file
	// The header is written using the host endianness here but
	// it will be overrwritten at done block time with a correct header
	status_t	 err = RawWrite(sizeof(info.header), &info.header);

	if (!err)
		fblockStack.push(info);

	return err;
}


status_t
MBlockFile::PutBytes(
	size_t size,
	const void * data)
{
	if (!fIsWriting)
		return INCONSISTENT_MODE;
	status_t err = RawWrite(size, data);
	return err;
}


status_t
MBlockFile::PutInt32(
	int32 l)
{
	if (!fIsWriting)
		return INCONSISTENT_MODE;

	if (B_HOST_IS_LENDIAN)
		l = B_HOST_TO_BENDIAN_INT32(l);

	status_t err = RawWrite(sizeof(int32), &l);
	return err;
}


status_t
MBlockFile::PutString(
	const char *	inString)
{
	if (!fIsWriting)
		return INCONSISTENT_MODE;
	
	int32		len = strlen(inString) + 1;
	int32		len1 = B_HOST_TO_BENDIAN_INT32(len);
	status_t 	err = RawWrite(sizeof(int32), &len1);
	
	if (!err)
		err = RawWrite(len, inString);

	return err;
}


status_t
MBlockFile::GetString(
	char *	outString,
	size_t	inBufferSize)
{
	if (!fIsReading)
		return INCONSISTENT_MODE;

	if (sizeof(int32) > GetBlockLeft())
		return NO_MORE_DATA;

	status_t 	err = B_NO_ERROR;
	int32		len;

	err = RawRead(sizeof(int32), &len);

	if (err)
		return err;

	if (B_HOST_IS_LENDIAN)
		len = B_BENDIAN_TO_HOST_INT32(len);

	if (len > GetBlockLeft())
		return NO_MORE_DATA;

	if (len > inBufferSize)
		len = inBufferSize - 1;
	err = RawRead(len, outString);
	outString[inBufferSize - 1] = '\0';

	return err;
}


status_t
MBlockFile::EndBlock(
	BlockType type)
{
	if (!fIsWriting)
		return INCONSISTENT_MODE;

	//	Find the block we're ending (could be deeper than one)
	if (type) {
		ASSERT(fblockStack.top().header.type == type);
		// we don't use this feature
		// we always match startblock and endblock
	}

	//	Close down to and including this block
	status_t 	err = B_NO_ERROR;

	while (err == B_NO_ERROR && ! fblockStack.empty()) 
	{
		BlockHeaderInfo&		close = fblockStack.top();
		BlockType 				closetype = close.header.type;

		err = DoEndBlock(close);
		fblockStack.pop();

		if (type == 0 || closetype == type)
			break;
	}

	return err;
}


status_t
MBlockFile::ResetRead(
	int32 atPosition)
{
	//	Close pending
	status_t err = B_NO_ERROR;

	err = Reset();

	fBufferStart = atPosition;
	fBufferEnd = atPosition;

	//	Setup at given position
	if (!err)
		err = Position(atPosition);
	if (!err)
		fPos = atPosition;

	//	Scan the "file" block
	BlockType	type;				//	ignore this
	if (!err)
	{
		fIsReading = true;		// BDS
		err = ScanBlock(type);
	}

	return err;
}


status_t
MBlockFile::ScanBlock(
	BlockType & type)
{
	type = 0;	//	for safety
	if (!fIsReading)
		return INCONSISTENT_MODE;

	//	Make sure there is one
	status_t err = B_NO_ERROR;
	if (GetBlockLeft() < sizeof(BlockHeader)) {
		return NO_MORE_BLOCKS;
	}

	//	Remember info on this block
	BlockHeaderInfo		info;
	info.pos = GetPos();

	//	Get the header
	err = RawRead(sizeof(BlockHeader), &info.header);
	if (!err) 
	{
		info.blocksize = B_BENDIAN_TO_HOST_INT32(info.header.size);
		if (B_HOST_IS_LENDIAN)
		{
			info.header.size = B_BENDIAN_TO_HOST_INT32(info.header.size);
		}

		fblockStack.push(info);
		type = info.header.type;
	}

	return err;
}


size_t
MBlockFile::GetCurBlockSize()
{
	if (!fIsReading || fblockStack.empty())
		return 0;
	return fblockStack.top().blocksize;
}


BlockType
MBlockFile::GetCurBlockType()
{
	if (!fIsReading || fblockStack.empty())
		return 0;
	return fblockStack.top().header.type;
}


int32
MBlockFile::GetBlockLeft()
{
	if (!fIsReading)
		return 0;
	if (fblockStack.empty())
	{
		off_t	size;
		(void) GetSize(&size);
		return size - GetPos();
	}
	else
	{
		BlockHeaderInfo&		info = fblockStack.top();
	
		return info.blocksize - (GetPos() - (info.pos + sizeof(BlockHeader)));
	}
}


status_t
MBlockFile::GetBytes(
	size_t size,
	void * data)
{
	if (!fIsReading)
		return INCONSISTENT_MODE;

	if (size > GetBlockLeft())
		return NO_MORE_DATA;

	return RawRead(size, data);
}


status_t
MBlockFile::GetInt32(
	int32 & l)
{
	if (!fIsReading)
		return INCONSISTENT_MODE;

	if (sizeof(int32) > GetBlockLeft())
		return NO_MORE_DATA;

	if (B_HOST_IS_BENDIAN)
		return RawRead(sizeof(int32), &l);
	else
	{
		status_t	err = RawRead(sizeof(int32), &l);
		if (!err)
			l = B_BENDIAN_TO_HOST_INT32(l);
		return err;
	}
}

status_t
MBlockFile::DoneBlock(
	BlockType type)
{
	if (!fIsReading)
		return INCONSISTENT_MODE;

	//	Find the block we're done with
	if (type) {
		ASSERT(fblockStack.top().header.type == type);
		// we don't use this feature
		// we always match startblock and endblock
	}

	//	Down to and including this block, dispose and seek out
	status_t 	err = B_NO_ERROR;

	while (err == B_NO_ERROR && ! fblockStack.empty()) 
	{
		BlockHeaderInfo&		close = fblockStack.top();
		BlockType 				closetype = close.header.type;

		err = DoDoneBlock(close);
		fblockStack.pop();

		if (type == 0 || closetype == type)
			break;
	}

	return err;
}


status_t
MBlockFile::Open()
{
	status_t err = B_ERROR;

	if (fFile == nil)
	{
		fFile = new BFile(this, B_READ_WRITE);
		
		err = fFile->InitCheck();
		if (err != B_NO_ERROR)
		{
			delete fFile;
			fFile = nil;
		}
	}

	return err;
}

status_t
MBlockFile::Close()
{
	status_t err = B_NO_ERROR;

	if (fFile != nil)
	{
		err = Reset();
		if (!err)
		{
			delete fFile;
			fFile = nil;
		}
	}

	::operator delete(fBuffer);
	fBuffer = nil;

	ASSERT(fblockStack.empty());

	return err;
}


#undef Seek

status_t
MBlockFile::Reset()
{
	status_t err = B_NO_ERROR;
	ASSERT(fFile != nil);

	//	depending on mode, repeatedly close until no more blocks
	if (fIsWriting) {
		while (! fblockStack.empty() && err == B_NO_ERROR) {
			err = EndBlock();	//	use default
		}
		Flush();
	}
	if (fIsReading) {
		while (! fblockStack.empty() && err == B_NO_ERROR) {
			err = DoneBlock();	//	use default
		}
	}
	while (! fblockStack.empty())
		fblockStack.pop();

	//	Clear out state
	if (!err)
	{
		off_t	pos = fFile->Seek(0, SEEK_ABSOLUTE);	// start of file
		if (pos > B_NO_ERROR)
			err = B_NO_ERROR;
	}
	if (!err)
	{
		fIsReading = false;
		fIsWriting = false;
	}
	if (fBuffer == nil)
		fBuffer = ::operator new(kBufferSize);

	return err;
}


#undef Read


status_t
MBlockFile::RawRead(
	size_t bytes,
	void * data)
{
	ASSERT(fFile != nil);
	off_t		position = GetPos();		// start
	off_t		end = position + bytes;		// one past the end
	status_t	err;
	ssize_t	 	bytesread;

	if (position >= fBufferStart)
	{
		if (end <= fBufferEnd)
		{
			// reading data that's in the buffer
			char*		pos = (char*) fBuffer + (position - fBufferStart);
			memcpy(data, pos, bytes);
			bytesread = bytes;
		}
		else
		{
			ssize_t	 	bytestoread = bytes;
			bytesread = 0;

			if (position < fBufferEnd)
			{
				// some of the data is in the buffer
				char*		pos = (char*) fBuffer + (position - fBufferStart);
				bytesread = fBufferEnd - position;
				bytestoread -= bytesread;
				memcpy(data, pos, bytesread);
				data = (char*) data + bytesread;
			}
			
			if (bytestoread > kBufferSize)
			{
				// data doesn't fit in the buffer
				bytesread += fFile->Read(data, bytestoread);
			}
			else
			{
				// new data fits in the buffer so read another
				// chunk into the buffer
				fBufferStart = position + bytesread;
				fFile->Seek(fBufferStart, SEEK_ABSOLUTE);
				ssize_t	 	chunksize = fFile->Read(fBuffer, kBufferSize);
				fBufferEnd = fBufferStart + chunksize;
				if (chunksize >= bytestoread)
				{
					memcpy(data, fBuffer, bytestoread);
					bytesread += bytestoread;
				}
			}
		}
	}
	else
	{
		// reading before start of buffer
		bytesread = fFile->Read(data, bytes);
	}

	if (bytesread == bytes) {
		fPos += bytes;
		err = B_NO_ERROR;
	} else if (bytesread >= 0) {				// BDS
		//	Back out if not all could be read
		Position(fPos);
		err = NO_MORE_DATA;
	}
	else
		err = bytesread;

	return err;
}


#undef Write

status_t
MBlockFile::RawWrite(
	size_t bytes,
	const void * data)
{
	ASSERT(fFile != nil);
	ASSERT(fBuffer != nil);
	off_t		position = GetPos();
	off_t		end = position + bytes;
	status_t	err;
	ssize_t		byteswritten = 0;

	if (end > fBufferStart)
	{
		if (end < fBufferEnd)
		{
			// copy to the buffer
			char*		pos = (char*) fBuffer + (position - fBufferStart);
			ASSERT(pos + bytes < (char*)fBuffer + kBufferSize);
			memcpy(pos, data, bytes);
			byteswritten = bytes;
			fBufferCurrEnd = max(fBufferCurrEnd, end);
		}
		else
		{
			// buffer is full so write out buffer
			size_t		bytestowrite = fBufferCurrEnd - fBufferStart;
			if (bytestowrite > 0)
			{
				off_t		savePos = fPos;
				fFile->Seek(fBufferStart, SEEK_ABSOLUTE);
				byteswritten = fFile->Write(fBuffer, bytestowrite);	
				fFile->Seek(savePos, SEEK_ABSOLUTE);
				if (byteswritten == bytestowrite)
				{
					fBufferStart += byteswritten;
					fBufferEnd = fBufferStart + kBufferSize;
					fBufferCurrEnd = fBufferStart;
				}
			}

			// handle this request
			if (bytes <= kBufferSize)
			{
				// copy to the buffer
				char*		pos = (char*) fBuffer + (position - fBufferStart);
				ASSERT(pos + bytes < (char*) fBuffer + kBufferSize);
				memcpy(pos, data, bytes);
				byteswritten = bytes;
				fBufferCurrEnd = max(fBufferCurrEnd, end);
			}
			else
			{
				// the request is bigger than the buffer
				byteswritten = fFile->Write(data, bytes);	
				if (byteswritten == bytes)
				{
					fBufferStart += byteswritten;
					fBufferEnd = fBufferStart + kBufferSize;
					fBufferCurrEnd = fBufferStart;
				}
			}
		}
	}
	else
	{
		// position is before start of buffer
		byteswritten = fFile->Write(data, bytes);	
	}

	if (byteswritten == bytes) {
		fPos += bytes;
		err = B_NO_ERROR;
	} else if (byteswritten >= 0) {				// BDS
		//	Back out if not all could be written
		Position(fPos);
		err = DISK_FULL;
	}
	else
		err = byteswritten;

	return err;
}


void
MBlockFile::Flush()
{
	if (fFile != nil && fBuffer != nil)
	{
		size_t		bytestowrite = fBufferCurrEnd - fBufferStart;
		if (bytestowrite > 0)
		{
			fFile->Seek(fBufferStart, SEEK_ABSOLUTE);
			fFile->Write(fBuffer, bytestowrite);	
		}
	}
}


#undef Seek

status_t
MBlockFile::Position(
	off_t pos)
{
	ASSERT(fFile != nil);
	//	Either seek, or remain untouched
	off_t 		newpos = fFile->Seek(pos, SEEK_ABSOLUTE);
	status_t	err = newpos >= B_NO_ERROR ? B_NO_ERROR : B_ERROR;

	if (!err)
	{
//		fPos = (off_t)(int32)newpos;	// TEMP ????
		fPos = newpos;	
	}
	else
		fFile->Seek(fPos, SEEK_ABSOLUTE);

	return err;
}


status_t
MBlockFile::DoEndBlock(
	BlockHeaderInfo& info)
{
	//	Update length data
	off_t pos = GetPos();
	info.blocksize = pos - (info.pos + sizeof(BlockHeader));
	info.header.size = B_HOST_TO_BENDIAN_INT32(info.blocksize);

	//	Write to disk
	status_t err = Position(info.pos);
	if (!err)
	{
		err = RawWrite(sizeof(BlockHeader), &info.header);
	}
	if (!err)
		err = Position(pos);

	//	Declare result
	return err;
}

status_t
MBlockFile::DoDoneBlock(
	BlockHeaderInfo& info)
{
	return Position(info.pos + sizeof(BlockHeader) + info.blocksize);
}


BFile*
MBlockFile::File()
{
	return fFile;
}
