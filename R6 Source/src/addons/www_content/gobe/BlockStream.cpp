//**********************************************************************************
// BlockStream.cpp
//**********************************************************************************

#include <ByteOrder.h>
#include <string.h>
#include <unistd.h>
#include <algobase.h>
#include <BitmapStream.h>

#include "BlockStream.h"
#include "TranslationConsts.h"

enum
{
	READ_PAST_BLOCK_ERROR 		= B_ERROR,
	MISSING_START_CHUNK_ERROR 	= B_ERROR,
	MISSING_END_CHUNK_ERROR		= B_ERROR,
	NOT_BLOCK_STREAM_ERROR		= B_ERROR,
	CANT_SEEK_BLOCK_ERROR		= B_ERROR
};

char Signature[12] = {'B','l','o','c','k',' ','S','t','r','e','a','m'};		// SHOULD be 4 bytes aligned.

TBlockStreamWriter::TBlockStreamWriter(BPositionIO* srcStream)
{
	mSrcStream = srcStream;
	mMajorKind = mMinorKind = 0;
	mErr = B_NO_ERROR;
}

TBlockStreamWriter::~TBlockStreamWriter()
{
}

status_t TBlockStreamWriter::CreateStream(uint32 majorKind, uint32 minorKind)
{
	int32 version = 0x0100;
	
	Write(&Signature[0], sizeof(Signature));
	write_int32(B_HOST_IS_LENDIAN);
	write_int32(version);
	write_uint32(majorKind);
	write_uint32(minorKind);
	
	mMajorKind = majorKind;
	mMinorKind = minorKind;
	return mErr;
}

status_t TBlockStreamWriter::Error(void) const
{
	return mErr;
}

uint32 TBlockStreamWriter::MajorKind(void) const
{
	return mMajorKind;
}

uint32 TBlockStreamWriter::MinorKind(void) const
{
	return mMinorKind;
}

status_t TBlockStreamWriter::write_int32(int32 value)
{
	return Write(&value, sizeof(int32));
}

status_t TBlockStreamWriter::write_uint32(uint32 value)
{
	return Write(&value, sizeof(uint32));
}

status_t TBlockStreamWriter::Write(const void* bytes, ssize_t numBytes)
{
	if (!mErr)
	{
		const char* ptr = (const char*) bytes;
		ssize_t writeBytes = numBytes;
		while (!mErr && writeBytes)
		{
			ssize_t writeSize = min( writeBytes, 64*1024L );			
			ssize_t bytesWritten = mSrcStream->Write(ptr, writeSize);
			if (bytesWritten < 0)
				mErr = bytesWritten;
			else if (!bytesWritten)
				mErr = B_ERROR;
			else
			{
				ptr += bytesWritten;
				writeBytes -= bytesWritten;
			}
		}
	}
	return mErr;
}

status_t TBlockStreamWriter::WriteBlockHeader(block_id id, type_code type)
{
	Write(&id, sizeof(block_id));
	Write(&type, sizeof(type_code));
	return mErr;
}

status_t TBlockStreamWriter::BeginChunk(block_id id)
{
	mChunkStack.AddItem( (void*) id );
	return WriteBlockHeader(id, START_CHUNK_TYPE);
}

status_t TBlockStreamWriter::EndChunk(block_id id)
{
	int32 count = mChunkStack.CountItems();
	if (count)
	{
		block_id startID = (block_id) mChunkStack.RemoveItem(count - 1);
		if (startID != id)
			mErr = B_ERROR;
	}
	else
		mErr = B_ERROR;
	return WriteBlockHeader(id, END_CHUNK_TYPE);
}

status_t TBlockStreamWriter::WriteBytes(block_id id, const void* bytes, ssize_t numBytes)
{
	WriteBlockHeader(id, B_RAW_TYPE);

	Write(&numBytes, sizeof(ssize_t));
	Write(bytes, numBytes);

	// Pad block to 4 byte boundry
	if (numBytes % 4)
	{
		int32 padCount = 4 - (numBytes % 4);
		int32 zero = 0;
		Write(&zero, padCount);
	}
	return mErr;
}

status_t TBlockStreamWriter::WriteBool(block_id id, bool value)
{
	WriteBlockHeader(id, B_BOOL_TYPE);
	int32 tmp = value;
	Write(&tmp, sizeof(int32));
	return mErr;
}

status_t TBlockStreamWriter::WriteInt8(block_id id, int8 value)
{
	WriteBlockHeader(id, B_INT8_TYPE);
	int32 tmp = value;
	Write(&tmp, sizeof(int32));
	return mErr;
}

status_t TBlockStreamWriter::WriteInt16(block_id id, int16 value)
{
	WriteBlockHeader(id, B_INT16_TYPE);
	int32 tmp = value;
	Write(&tmp, sizeof(int32));
	return mErr;
}

status_t TBlockStreamWriter::WriteInt32(block_id id, int32 value)
{
	WriteBlockHeader(id, B_INT32_TYPE);
	Write(&value, sizeof(int32));
	return mErr;
}

status_t TBlockStreamWriter::WriteFloat(block_id id, float value)
{
	WriteBlockHeader(id, B_FLOAT_TYPE);
	Write(&value, sizeof(float));
	return mErr;
}

status_t TBlockStreamWriter::WriteInt64(block_id id, int64 value)
{
	WriteBlockHeader(id, B_INT64_TYPE);
	Write(&value, sizeof(int64));
	return mErr;
}

status_t TBlockStreamWriter::WriteDouble(block_id id, double value)
{
	WriteBlockHeader(id, B_DOUBLE_TYPE);
	Write(&value, sizeof(double));
	return mErr;
}

status_t TBlockStreamWriter::WritePoint(block_id id, BPoint value)
{
	WriteBlockHeader(id, B_POINT_TYPE);
	Write(&value, sizeof(BPoint));
	return mErr;
}

status_t TBlockStreamWriter::WriteRect(block_id id, BRect value)
{
	WriteBlockHeader(id, B_RECT_TYPE);
	Write(&value, sizeof(BRect));
	return mErr;
}

status_t TBlockStreamWriter::WriteString(block_id id, const char* str)
{
	uint32 len = str ? strlen(str) + 1 : 0;
	return WriteString(id, str, len);
}

status_t TBlockStreamWriter::WriteString(block_id id, const char* str, uint32 len)
{
	int32 zero = 0;
	WriteBlockHeader(id, B_STRING_TYPE);

	// If string is not null terminated then make sure we null terminate it
	// when we go to save it.
	if (str && len && str[len] != 0)
	{
		len++;
		Write(&len, sizeof(uint32));
		Write(str, len-1);
		Write(&zero, 1);
	}
	else
	{
		Write(&len, sizeof(uint32));
		if (str && len)
			Write(str, len);
	}
		
	// Pad block to 4 byte boundry
	if (len % 4)
	{
		int32 padCount = 4 - (len % 4);
		Write(&zero, padCount);
	}
	return mErr;
}

status_t TBlockStreamWriter::WriteBitmap(block_id id, BBitmap* bitmap)
{
//	char buffer[2048];
	BBitmapStream stream( bitmap );
//	stream.Seek(0L, SEEK_SET);
	off_t size = stream.Size();
	int32 streamSize = size;
	off_t offset = 0;
	char* buffer = new char[size];

	while (!mErr && size)
	{
		stream.Seek(offset, SEEK_SET);
		char* ptr = buffer + offset;
		ssize_t bytesRead = stream.ReadAt(offset, ptr, size);
		if (bytesRead < 0)
			mErr = bytesRead;
		else
		{
			size -= bytesRead;
			offset += bytesRead;
		}
	}
	WriteBytes(id, buffer, streamSize);
	delete [] buffer;
	stream.DetachBitmap(&bitmap);
	return mErr;
}

#pragma mark -

TBlockStreamReader::TBlockStreamReader(BPositionIO* srcStream)
{
	mSrcStream = srcStream;
	mValidStream = false;
	mMajorKind = mMinorKind = 0;

	mBlockID = 0;
	mBlockType = 0;
	mBlockDataSize = mBlockBytesRead = mBlockActualSize = 0;

	mEOF = false;
	mErr = B_NO_ERROR;
	mStackDepth = 0;
}

TBlockStreamReader::~TBlockStreamReader()
{
}

status_t TBlockStreamReader::OpenStream(void)
{
	// Check for block stream signature.
	for (size_t x = 0; x < sizeof(Signature); x++)
	{
		char buffer;
		status_t err = read(&buffer, 1);
		if (B_OK != err || buffer != Signature[x])
		{
			mErr = NOT_BLOCK_STREAM_ERROR;
			if (err == B_OK)
				err = NOT_BLOCK_STREAM_ERROR;
			
			
			mSrcStream->Seek(0, SEEK_SET);
			return err;
		}
	}
	
	// Read block stream information.
	read(&mLendian, sizeof(int32));
	read_int32(&mVersion);
	read_int32((int32*) &mMajorKind);
	read_int32((int32*) &mMinorKind);
	return mErr;
}

status_t TBlockStreamReader::Error(void) const
{
	return mErr;
}

bool TBlockStreamReader::NextBlock(block_id* id, type_code* type, bool stopOnEndChunk)
{
	if (mErr)
		return false;

	// Skip past any remaining data in the current block. And get next block header.
	next_block();
	read_int32(&mBlockID);
	if (mEOF)
	{
		mErr = B_NO_ERROR;
		return false;
	}		
	read_int32((int32*) &mBlockType);
	if (mErr)
		return false;

	// Set up return values for id & type.
	if (id)
		*id = mBlockID;
	if (type)
		*type = mBlockType;

	// Determine size of the block data that follows.		
	mBlockBytesRead = 0;
	mBlockActualSize = mBlockDataSize = 0;
	switch (mBlockType)
	{
		case START_CHUNK_TYPE:
			mChunkStack[mStackDepth++] = mBlockID;
			break;
		case END_CHUNK_TYPE:
			if (!mStackDepth)
				mErr = MISSING_START_CHUNK_ERROR;
			else if (mChunkStack[--mStackDepth] != mBlockID)
				mErr = MISSING_END_CHUNK_ERROR;
			if (mErr || stopOnEndChunk)
				return false;
			break;
		case B_BOOL_TYPE:
		case B_INT8_TYPE:
		case B_INT16_TYPE:
		case B_INT32_TYPE:
			mBlockDataSize = sizeof(int32);
			break;
		case B_FLOAT_TYPE:
			mBlockDataSize = sizeof(float);
			break;
		case B_INT64_TYPE:
			mBlockDataSize = sizeof(int64);
			break;
		case B_DOUBLE_TYPE:
			mBlockDataSize = sizeof(double);
			break;
		case B_POINT_TYPE:
			mBlockDataSize = sizeof(BPoint);
			break;
		case B_RECT_TYPE:
			mBlockDataSize = sizeof(BRect);
			break;
		default:
			read_int32((int32*) &mBlockDataSize);
			break;
	}
	
	// Determine actual size written to disk
	mBlockActualSize = mBlockDataSize;
	if (mBlockDataSize % 4)
		mBlockActualSize += 4 - (mBlockDataSize % 4);
	return !mErr;
}

// Does nothing if block is not a start chunk. If start chunk skips past end chunk.
status_t TBlockStreamReader::SkipBlock(void)
{
	block_id id;

	if (mBlockType != START_CHUNK_TYPE)
		next_block();
	else
	{
		while (!mErr && NextBlock(&id))
			SkipBlock();		
	}
	return mErr;
}

#pragma mark -
int32 TBlockStreamReader::Version(void) const
{
	return mVersion;
}

uint32 TBlockStreamReader::MajorKind(void) const
{
	return mMajorKind;
}

uint32 TBlockStreamReader::MinorKind(void) const
{
	return mMinorKind;
}

block_id TBlockStreamReader::BlockID(void) const
{
	return mBlockID;
}

type_code TBlockStreamReader::BlockKind(void) const
{
	return mBlockType;
}

uint32 TBlockStreamReader::BlockDataSize(void) const
{
	return mBlockDataSize;
}

const void* TBlockStreamReader::BytesPtr(void) const
{
	BMallocIO* src = dynamic_cast<BMallocIO*>(mSrcStream);
	if (src)
		return ((const char*) src->Buffer()) + src->Position() + mBlockBytesRead;
	return NULL;
}

#pragma mark -
status_t TBlockStreamReader::ReadBytes(void* bytes, ssize_t numBytes)
{
	if (!mErr)
	{
		if (numBytes > mBlockActualSize - mBlockBytesRead)
			mErr = READ_PAST_BLOCK_ERROR;
		else
		{
			char* ptr = (char*) bytes;
			ssize_t bytesLeft = numBytes;
			while (!mErr && bytesLeft)
			{
				ssize_t readSize = 64*1024L;
				if (bytesLeft < readSize)
					readSize = bytesLeft;
				ssize_t bytesRead = mSrcStream->Read(ptr, readSize);
				if (bytesRead < 0)
					mErr = bytesRead;
				else if (bytesRead != readSize)
					mErr = B_ERROR;
				else
					mBlockBytesRead += readSize;
				ptr += readSize;
				bytesLeft -= readSize;
			}
		}
	}
	return mErr;
}

status_t TBlockStreamReader::ReadString(void* bytes, ssize_t maxBytes)
{
	ssize_t numBytes = mBlockDataSize;
	if (numBytes > maxBytes)
		numBytes = maxBytes;
		
	if (!mErr)
	{
		if (numBytes > mBlockActualSize - mBlockBytesRead)
			mErr = READ_PAST_BLOCK_ERROR;
		else
		{
			ssize_t bytesRead = mSrcStream->Read(bytes, numBytes);
			if (bytesRead < 0)
				mErr = bytesRead;
			else if (bytesRead != numBytes)
				mErr = B_ERROR;
			else
				mBlockBytesRead += bytesRead;
		}
	}
	return mErr;
}

char* TBlockStreamReader::ReadString(void)
{
	int32 size = BlockDataSize();
	char* strPtr = new char[size+1];
	strPtr[size] = 0;
	if (B_NO_ERROR != ReadString(strPtr, size))
	{
		delete [] strPtr;
		strPtr = NULL;
	}
	return strPtr;
}

status_t TBlockStreamReader::ReadBool(bool* value)
{
	int32 tmp;
	status_t err = ReadInt32(&tmp);
	*value = tmp;
	return err;
}

status_t TBlockStreamReader::ReadInt8(int8* value)
{
	int32 tmp;
	status_t err = ReadInt32(&tmp);
	*value = tmp;
	return err;
}

status_t TBlockStreamReader::ReadInt16(int16* value)
{
	int32 tmp;
	status_t err = ReadInt32(&tmp);
	*value = tmp;
	return err;
}

status_t TBlockStreamReader::ReadFloat(float* value)
{
	mErr = ReadBytes(value, sizeof(float));
	if (mLendian)
		*value = B_LENDIAN_TO_HOST_FLOAT(*value);
	return mErr;
}

status_t TBlockStreamReader::ReadInt32(int32* value)
{
	mErr = ReadBytes(value, sizeof(int32));
	if (mLendian)
		*value = B_LENDIAN_TO_HOST_INT32(*value);
	return mErr;
}

status_t TBlockStreamReader::ReadInt64(int64* value)
{
	mErr = ReadBytes(value, sizeof(int64));
	if (mLendian)
		*value = B_LENDIAN_TO_HOST_INT64(*value);
	return mErr;
}

status_t TBlockStreamReader::ReadDouble(double* value)
{
	mErr = ReadBytes(value, sizeof(double));
	if (mLendian)
		*value = B_LENDIAN_TO_HOST_DOUBLE(*value);
	return mErr;
}

status_t TBlockStreamReader::ReadBitmap(BBitmap** bitmapPtr)
{
	size_t size = BlockDataSize();
	char* buffer = new char[size];
	mErr = ReadBytes(buffer, size);
	off_t offset = 0;
	
	BBitmapStream stream;
	while (!mErr && size)
	{
		size_t bytesWritten = stream.Write(buffer + offset, size);
		if (bytesWritten < 0)
			mErr = bytesWritten;
		else
		{
			size -= bytesWritten;
			offset += bytesWritten;
		}
	}
	delete [] buffer;
	if (mErr || B_NO_ERROR != stream.DetachBitmap(bitmapPtr))
		*bitmapPtr = NULL;
	return mErr;
}

status_t TBlockStreamReader::ReadPoint(BPoint* value)
{
	mErr = ReadBytes(&value->x, sizeof(float));
	if (!mErr)
		mErr = ReadBytes(&value->y, sizeof(float));
	if (mLendian)
	{
		value->x = B_LENDIAN_TO_HOST_FLOAT(value->x);
		value->y = B_LENDIAN_TO_HOST_FLOAT(value->y);
	}
	return mErr;
}

status_t TBlockStreamReader::ReadRect(BRect* value)
{
	BPoint pt;
//	mErr = ReadPoint(&value->LeftTop());
	mErr = ReadPoint(&pt);
	value->left = pt.x;
	value->top = pt.y;
	if (!mErr)
	{
//		mErr = ReadPoint(&value->RightBottom());
		mErr = ReadPoint(&pt);
		value->right = pt.x;
		value->bottom = pt.y;
	}
	return mErr;
}

#pragma mark -
status_t TBlockStreamReader::read_int32(int32* value)
{
	mErr = read(value, sizeof(int32));
	if (mLendian)
		*value = B_LENDIAN_TO_HOST_INT32(*value);
	return mErr;
}

status_t TBlockStreamReader::next_block(void)
{
	int32 skipSize = mBlockActualSize - mBlockBytesRead;
	if (!mErr && skipSize)
	{
		off_t pos = mSrcStream->Position();
		off_t newPos = mSrcStream->Seek(skipSize, SEEK_CUR);
		if (newPos != pos + skipSize)
			mErr = CANT_SEEK_BLOCK_ERROR;
		else
			mBlockBytesRead = mBlockActualSize;
	}
	return mErr;
}

status_t TBlockStreamReader::read(void* buffer, ssize_t numBytes)
{
	if (!mErr)
	{
		int32 bytesRead = mSrcStream->Read(buffer, numBytes);
		if (bytesRead < 0)
			mErr = bytesRead;
		else if (bytesRead != numBytes)
			mErr = B_ERROR;
		if (!bytesRead)
			mEOF = true;
	}
	return mErr;
}

#pragma mark -

//-------------------------------------------------------------------
// TBlockData - data passed in will be deleted.
//-------------------------------------------------------------------
TBlockData::TBlockData(block_id id, type_code type, void* data, ssize_t size)
{
	mID = id;
	mType = type;
	mSize = size;
	mData = data;
	ASSERTC(type != START_CHUNK_TYPE || mData);
}

TBlockData::TBlockData(const TBlockData& src)
{
	mID = src.mID;
	mType = src.mType;
	mSize = src.mSize;
	mData = NULL;
	
	if (mType == START_CHUNK_TYPE)
	{
		ASSERTC( src.mData );
		TBlockDataTable* dataPtr = static_cast<TBlockDataTable*>(src.mData);
		mData = new TBlockDataTable( *dataPtr );
	}
	else if (mSize && src.mData)
	{
		mData = new char[mSize];
		memcpy(mData, src.mData, mSize);
	}
}

TBlockData::~TBlockData()
{
	if (mType == START_CHUNK_TYPE)
		delete static_cast<TBlockDataTable*>(mData);
	else
		delete [] mData;
}

bool TBlockData::operator==(const TBlockData& data) const
{
	if (mID != data.mID || mType != data.mType || mSize != data.mSize)
		return false;
	if (mType == START_CHUNK_TYPE)
	{
		TBlockDataTable* table1 = (TBlockDataTable*) mData;
		TBlockDataTable* table2 = (TBlockDataTable*) data.mData;
		return (*table1 == *table2);
	}
	if (mSize)
		return !memcmp(mData, data.mData, mSize);
	return true;
}

#pragma mark -
//-------------------------------------------------------------------
// TBlockDataTable
//-------------------------------------------------------------------
TBlockDataTable::TBlockDataTable()
{
}

TBlockDataTable::TBlockDataTable(const TBlockDataTable& src)
{
	for (int32 x = 0; x < src.CountItems(); x++)
	{
		const TBlockData* srcBlockPtr = src[x];
		TBlockData* blockPtr = new TBlockData(*srcBlockPtr);
		mBlockData.AddItem(blockPtr);
	}
}

TBlockDataTable::~TBlockDataTable()
{
	while (mBlockData.CountItems())
		delete static_cast<TBlockData*>(mBlockData.RemoveItem(0L));
}

int32 TBlockDataTable::CountItems(void) const
{
	return mBlockData.CountItems();
}

TBlockData* TBlockDataTable::operator[](int32 index) const
{
	if (index < 0 || index >= mBlockData.CountItems())
		return NULL;
	return reinterpret_cast<TBlockData*>(mBlockData.ItemAt(index));
}

bool TBlockDataTable::operator==(const TBlockDataTable& table) const
{
	if (CountItems() != table.CountItems())
		return false;
	for (long x = 0; x < CountItems(); x++)
	{
		TBlockData* data1 = (*this)[x];
		TBlockData* data2 = table[x];
		if (!(*data1 == *data2))
			return false;
	}
	return true;
}

#pragma mark -
void TBlockDataTable::AddBool(block_id id, bool value)
{
	AddBlock( id, B_BOOL_TYPE, &value, sizeof(bool) );
}

void TBlockDataTable::AddInt16(block_id id, int16 value)
{
	AddBlock( id, B_INT16_TYPE, &value, sizeof(int16) );
}

void TBlockDataTable::AddInt32(block_id id, int32 value)
{
	AddBlock( id, B_INT32_TYPE, &value, sizeof(int32) );
}

void TBlockDataTable::AddInt64(block_id id, int64 value)
{
	AddBlock( id, B_INT64_TYPE, &value, sizeof(int64) );
}

void TBlockDataTable::AddFloat(block_id id, float value)
{
	AddBlock( id, B_FLOAT_TYPE, &value, sizeof(float) );
}

void TBlockDataTable::AddDouble(block_id id, double value)
{
	AddBlock( id, B_DOUBLE_TYPE, &value, sizeof(double) );
}

void TBlockDataTable::AddString(block_id id, const char* value)
{
	AddBlock( id, B_STRING_TYPE, value, strlen(value)+1 );
}

void TBlockDataTable::AddBytes(block_id id, const void* bytes, ssize_t numBytes)
{
	AddBlock( id, B_RAW_TYPE, bytes, numBytes );
}

void TBlockDataTable::AddChunk(block_id id, TBlockDataTable* chunk)
{
	TBlockData* data = new TBlockData(id, START_CHUNK_TYPE, chunk, 0);
	mBlockData.AddItem( data );
}

void TBlockDataTable::AddBlock(block_id id, type_code type, const void* bytes, ssize_t numBytes)
{
	// Make our own copy of the data.
	void* copyBytes = NULL;
	if (bytes && numBytes)
		copyBytes = new char[numBytes];
	if (copyBytes)
		memcpy( copyBytes, bytes, numBytes );
		
	TBlockData* data = new TBlockData(id, type, copyBytes, numBytes);
	mBlockData.AddItem( data );
}

status_t TBlockDataTable::AddBlockFromStream(TBlockStreamReader* reader)
{
	block_id  	id = reader->BlockID();
	type_code 	type = reader->BlockKind();
	uint32 		size = reader->BlockDataSize();
	status_t	err = B_NO_ERROR;

	switch (type)
	{
		case START_CHUNK_TYPE:
		{
			block_id 			chunkID = id;
			TBlockDataTable* 	chunk = new TBlockDataTable();

			while (reader->NextBlock(&id, &type))
				chunk->AddBlockFromStream(reader);
			err = reader->Error();

			if (err)
				delete chunk;
			else
				AddChunk( chunkID, chunk );
			break;
		}
		
		case B_RAW_TYPE:
		case B_STRING_TYPE:
		{
			char* bytes = new char[ size ];
			err = reader->ReadBytes( bytes, size );
			if (err)
			{
				delete [] bytes;
				break;
			}		
			TBlockData* data = new TBlockData(id, type, bytes, size);
			mBlockData.AddItem( data );
			break;
		}

		case B_BOOL_TYPE:
		{
			bool value;
			err = reader->ReadBool(&value);
			if (!err)
				AddBool(id, value);
			break;
		}
		
		case B_INT16_TYPE:
		{
			int16 value;
			err = reader->ReadInt16(&value);
			if (!err)
				AddInt16(id, value);
			break;
		}		

		case B_INT32_TYPE:
		{
			int32 value;
			err = reader->ReadInt32(&value);
			if (!err)
				AddInt32(id, value);
			break;
		}
		
		case B_FLOAT_TYPE:
		{
			float value;
			err = reader->ReadFloat(&value);
			if (!err)
				AddFloat(id, value);
			break;
		}

		case B_INT64_TYPE:
		{
			int64 value;
			err = reader->ReadInt64(&value);
			if (!err)
				AddInt64(id, value);
			break;
		}

		case B_DOUBLE_TYPE:
		{
			double value;
			err = reader->ReadDouble(&value);
			if (!err)
				AddDouble(id, value);
			break;
		}		
	}
	return err;
}

status_t TBlockDataTable::Write(TBlockStreamWriter* writer) const
{
	status_t err = B_NO_ERROR;
	
	for (long x = 0; x < CountItems(); x++)
	{
		TBlockData* data = (*this)[x];
		block_id 	id = data->BlockID();
		type_code 	type = data->BlockKind();
		const void*	blockData = data->BlockData();
		
		switch (type)
		{
			case START_CHUNK_TYPE:
			{
				err = writer->BeginChunk(id);
				const TBlockDataTable* table = reinterpret_cast<const TBlockDataTable*>(blockData);
				if (table)
					err = table->Write(writer);
				err = writer->EndChunk(id);
				break;
			}
			
			case B_BOOL_TYPE:
				err = writer->WriteBool(id, *(bool*) blockData);
				break;
			case B_INT8_TYPE:
				err = writer->WriteInt8(id, *(int8*) blockData);
				break;
			case B_INT16_TYPE:
				err = writer->WriteInt16(id, *(int16*) blockData);
				break;
			case B_INT32_TYPE:
				err = writer->WriteInt32(id, *(int32*) blockData);
				break;
			case B_INT64_TYPE:
				err = writer->WriteInt64(id, *(int64*) blockData);
				break;
			case B_FLOAT_TYPE:
				err = writer->WriteFloat(id, *(float*) blockData);
				break;
			case B_DOUBLE_TYPE:
				err = writer->WriteDouble(id, *(double*) blockData);
				break;
			case B_STRING_TYPE:
				err = writer->WriteString(id, (const char*) blockData);
				break;
			case B_RAW_TYPE:
				err = writer->WriteBytes(id, blockData, data->BlockDataSize());
				break;
		}
		if (err != B_NO_ERROR)
			break;
	}
	return err;
}

status_t TBlockDataTable::Read( TBlockStreamReader* reader )
{
	if (reader->BlockKind() != START_CHUNK_TYPE)
		return B_ERROR;
	
	block_id id;	
	while (reader->NextBlock(&id))
		AddBlockFromStream(reader);
	return reader->Error();
}

bool TBlockDataTable::FindBool( block_id id, bool& value ) const
{
	const TBlockData* blockPtr = FindBlock( id );
	if (!blockPtr || blockPtr->BlockKind() != B_BOOL_TYPE)
		return false;
	value = *(bool*) blockPtr->BlockData();	
	return true;
}

bool TBlockDataTable::FindInt16( block_id id, int16& value ) const
{
	const TBlockData* blockPtr = FindBlock( id );
	if (!blockPtr || blockPtr->BlockKind() != B_INT16_TYPE)
		return false;
	value = *(int16*) blockPtr->BlockData();	
	return true;

//	int32 temp;
//	bool result = FindInt32( id, temp );
//	value = temp;
//	return result;
}

bool TBlockDataTable::FindInt32( block_id id, int32& value ) const
{
	const TBlockData* blockPtr = FindBlock( id );
	if (!blockPtr || blockPtr->BlockKind() != B_INT32_TYPE)
		return false;
	value = *(int32*) blockPtr->BlockData();	
	return true;
}

bool TBlockDataTable::FindInt64( block_id id, int64& value ) const
{
	const TBlockData* blockPtr = FindBlock( id );
	if (!blockPtr || blockPtr->BlockKind() != B_INT64_TYPE)
		return false;
	value = *(int64*) blockPtr->BlockData();	
	return true;
}

bool TBlockDataTable::FindFloat( block_id id, float& value ) const
{
	const TBlockData* blockPtr = FindBlock( id );
	if (!blockPtr || blockPtr->BlockKind() != B_FLOAT_TYPE)
		return false;
	value = *(float*) blockPtr->BlockData();	
	return true;
}

bool TBlockDataTable::FindDouble( block_id id, double& value ) const
{
	const TBlockData* blockPtr = FindBlock( id );
	if (!blockPtr || blockPtr->BlockKind() != B_DOUBLE_TYPE)
		return false;
	value = *(double*) blockPtr->BlockData();	
	return true;
}

bool TBlockDataTable::FindString( block_id id, const char** strPtr) const
{
	const TBlockData* blockPtr = FindBlock( id );
	if (!blockPtr || blockPtr->BlockKind() != B_STRING_TYPE)
		return false;
	*strPtr = (const char*) blockPtr->BlockData();
	return true;
}

bool TBlockDataTable::FindBytes( block_id id, const void** ptr, ssize_t* size ) const
{
	const TBlockData* blockPtr = FindBlock( id );
	if (!blockPtr || blockPtr->BlockKind() != B_RAW_TYPE)
		return false;
	*ptr = blockPtr->BlockData();
	*size = blockPtr->BlockDataSize();
	return true;
}

const TBlockData* TBlockDataTable::FindBlock( block_id id ) const
{
	for (long x = 0; x < CountItems(); x++)
	{
		TBlockData* blockPtr = (*this)[x];
		if (blockPtr->BlockID() == id)
			return blockPtr;
	}
	return NULL;
}

const TBlockDataTable* TBlockDataTable::FindChunk( block_id id ) const
{
	const TBlockData* blockPtr = FindBlock( id );
	if (!blockPtr || blockPtr->BlockKind() != START_CHUNK_TYPE)
		return NULL;
	return reinterpret_cast<const TBlockDataTable*>(blockPtr->BlockData());
}

// $Header: /usr/local/cvsroot/8ball/Datatypes/BlockStream.cpp,v 1.26 1999/11/04 00:11:34 tom Exp $



