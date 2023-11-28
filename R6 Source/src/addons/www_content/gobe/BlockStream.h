//**********************************************************************************
// BlockStream.h
//**********************************************************************************

#ifndef __BLOCKSTREAM_H__
#define __BLOCKSTREAM_H__

#include <TranslationKit.h>
#include <DataIO.h>
#include <List.h>
#include <TypeConstants.h>

#define START_CHUNK_TYPE	'CHK+'
#define END_CHUNK_TYPE		'CHK-'
#define MAX_CHUNK_DEPTH		500

typedef int32 block_id;

//-------------------------------------------------------------------------------------
// TBlockStreamWriter
//-------------------------------------------------------------------------------------
class TBlockStreamWriter
{
	public:
		TBlockStreamWriter(BPositionIO* srcStream);
		~TBlockStreamWriter();
		
		status_t		CreateStream(uint32 majorKind, uint32 minorKind);		
		status_t		Error(void) const;

		uint32			MajorKind(void) const;
		uint32			MinorKind(void) const;

		status_t		BeginChunk(block_id id);
		status_t		EndChunk(block_id id);
		
		status_t		WriteBytes(block_id id, const void* bytes, ssize_t numBytes);
		status_t		WriteBool(block_id id, bool value);
		status_t		WriteInt8(block_id id, int8 value);
		status_t		WriteInt16(block_id id, int16 value);
		status_t		WriteInt32(block_id id, int32 value);
		status_t		WriteFloat(block_id id, float value);
		status_t		WriteInt64(block_id id, int64 value);
		status_t		WriteDouble(block_id id, double value);
		status_t		WritePoint(block_id id, BPoint pt);
		status_t		WriteRect(block_id id, BRect rect);
		status_t		WriteString(block_id id, const char* str);
		status_t		WriteString(block_id id, const char* str, uint32 len);
		status_t		WriteBitmap(block_id id, BBitmap* bitmap);

		// If you want to write out the block data yourself you can use these methods.
		// For example instead of having to buffer a big chunk of data you might want
		// to call WriteBlockHeader with the id and B_RAW_TYPE, then write_int32 the
		// data size, then call Write to write out the actual bytes.
		
		status_t		WriteBlockHeader(block_id id, type_code type);
		status_t		Write(const void* bytes, ssize_t numBytes);
		status_t		write_int32(int32 value);
		status_t		write_uint32(uint32 value);
				
	private:
		BPositionIO*	mSrcStream;
		BList			mChunkStack;
		uint32			mMajorKind;
		uint32			mMinorKind;
		status_t		mErr;
};

//-------------------------------------------------------------------------------------
// TBlockStreamReader
//-------------------------------------------------------------------------------------
class TBlockStreamReader
{
	public:
		TBlockStreamReader(BPositionIO* srcStream);
		~TBlockStreamReader();
		
		// Call to try and read srcStream as a block stream.
		status_t		OpenStream(void);
		
		// Use to loop through blocks in the stream.
		status_t		Error(void) const;
		bool			NextBlock(block_id* id, type_code* type = NULL, bool stopOnEndChunk = true);
		status_t		SkipBlock(void);
		
		// Stream information.
		int32			Version(void) const;
		uint32			MajorKind(void) const;
		uint32			MinorKind(void) const;

		// Current block being read.
		block_id		BlockID(void) const;
		type_code		BlockKind(void) const;
		uint32			BlockDataSize(void) const;

		// Used to read contents of current block.
		status_t		ReadBytes(void* bytes, ssize_t numBytes);
		status_t		ReadString(void* bytes, ssize_t maxBytes);
		status_t		ReadBool(bool* value);
		status_t		ReadInt8(int8* value);
		status_t		ReadInt16(int16* value);
		status_t		ReadFloat(float* value);
		status_t		ReadInt32(int32* value);
		status_t		ReadInt64(int64* value);
		status_t		ReadDouble(double* value);
		status_t		ReadBitmap(BBitmap** bitmap);
		status_t		ReadPoint(BPoint* value);
		status_t		ReadRect(BRect* value);
		
		// Convient shortcut - returns string with block data size.
		char*			ReadString(void);
		
		// This method may be used to avoid double buffering. If the source
		// stream has the data in memory this routine will return a pointer
		// to the data bytes of a Bytes block instead of having to read the
		// bytes into a separate buffer.
		const void*		BytesPtr(void) const;
		
				
	private:
		status_t		read(void* bytes, ssize_t numBytes);
		status_t		read_int32(int32* value);
		status_t		next_block(void);
		
		BPositionIO*	mSrcStream;
		bool			mValidStream;
		int32			mLendian;			// Non-zero if stream is LENDIAN
		int32			mVersion;
		uint32			mMajorKind;
		uint32			mMinorKind;
		
		block_id		mBlockID;
		type_code		mBlockType;
		ssize_t			mBlockDataSize;
		ssize_t			mBlockActualSize;
		ssize_t			mBlockBytesRead;
		
		bool			mEOF;
		status_t		mErr;
		int32			mStackDepth;
		block_id		mChunkStack[MAX_CHUNK_DEPTH];
};


//-------------------------------------------------------------------------------------
// TBlockData
//-------------------------------------------------------------------------------------
class TBlockData
{
	public:
		TBlockData(block_id id, type_code type, void* data, ssize_t size);
		TBlockData(const TBlockData& src);
		~TBlockData();
		
		block_id		BlockID(void) const			{ return mID; }
		type_code		BlockKind(void) const		{ return mType; }
		uint32			BlockDataSize(void) const	{ return mSize; }
		const void*		BlockData(void) const		{ return mData; }

	    bool			operator==(const TBlockData& data) const;
		
	private:
		block_id		mID;
		type_code		mType;
		ssize_t			mSize;
		void*			mData;
};
		
//-------------------------------------------------------------------------------------
// TBlockDataTable
//-------------------------------------------------------------------------------------
class TBlockDataTable
{
	public:
		TBlockDataTable();
		TBlockDataTable(const TBlockDataTable& src);
		~TBlockDataTable();
	
		// Accessors.
		int32			CountItems(void) const;
		TBlockData* 	operator[](int32 index) const;
	    bool			operator==(const TBlockDataTable& table) const;

		bool			FindBool( block_id id, bool& value ) const;
		bool			FindInt16( block_id id, int16& value ) const;
		bool			FindInt32( block_id id, int32& value ) const;
		bool			FindInt64( block_id id, int64& value ) const;
		bool			FindFloat( block_id id, float& value ) const;
		bool			FindDouble( block_id id, double& value ) const;
		bool			FindString( block_id id, const char** str) const;
		bool			FindBytes( block_id id, const void** bytes, ssize_t* numBytes ) const;
		
		const TBlockData*		FindBlock( block_id id ) const;
		const TBlockDataTable*	FindChunk( block_id id ) const;
		
		// Shortcut methods for adding blocks to table.
		void			AddBool(block_id id, bool value);		// becomes int32
		void			AddInt16(block_id id, int16 value);		// becomes int32
		void			AddInt32(block_id id, int32 value);
		void			AddInt64(block_id id, int64 value);
		void			AddFloat(block_id id, float value);
		void			AddDouble(block_id id, double value);
		void			AddString(block_id id, const char* str);
		void			AddBytes(block_id id, const void* bytes, ssize_t numBytes);
		void			AddChunk(block_id id, TBlockDataTable* chunk);
		
		// Add the current block in the stream to the table.
		status_t 		AddBlockFromStream(TBlockStreamReader* reader);

		// Output the blocks in the table to the given block stream.
		status_t		Write(TBlockStreamWriter* writer) const;
		status_t		Read(TBlockStreamReader* reader);	

		// Will make a copy of bytes
		void 			AddBlock(block_id id, type_code type, const void* bytes, ssize_t numBytes);
				
	private:
		BList			mBlockData;
};
	
#endif // __BLOCKSTREAM_H__

// $Header: /usr/local/cvsroot/8ball/Datatypes/BlockStream.h,v 1.15 1999/11/02 00:46:35 tom Exp $
