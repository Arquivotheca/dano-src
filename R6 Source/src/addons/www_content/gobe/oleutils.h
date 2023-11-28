//-------------------------------------------------------------------------------------
// oleutils.h
//
//	This file defines the following classes that can be used to read and write OLE documents.
//
//	TOLEReader
//		- Provides top level information about the file you give it. Once created
//		- you can query the class for information about the OLE streams ("Entries")
//		- that are in the document. To actually read an OLE stream call the
//		- GetEntryReader method. This will give you a TOLEEntryReader* for your use.
//		- Remember to delete it when you are done.
//
//	TOLEEntry
//		- Provides methods to acces information about a specific entry (name, size, etc.)
//
//	TOLEEntryReader
//		- Provides methods for reading from the stream of data associated with a
//		- particular entry. You can intermix calls to different entry readers in
//		- the same document safely (but note that you are seeking around in the file
//		- when you do so.)
//
//	TOLEWriter
//		- Will create an OLE document directory in the given file. Initially the document
// 		- will only have the "Root Entry". To create new streams in the document you must
//		- first create a TOLEEntry for the stream. Use the NewChildEntry method to create
//		- the first entry with destIndex of 0 (the Root). Other streams can be created
//		- using the NewPrevEntry, NewNextEntry, and NewChildEntry methods as required by
//		- the type of document you are creating. Note: The order of entries does matter!
//		- Once you have created an entry you can request the TOLEEntryWriter* for it. 
//		- There exists only one writer per entry (ie, it's not yours to dispose). 
//
//		- Entry information is cached until it's known whether the entry will end up using
//		- big blocks or small blocks for its stream. (4K or larger streams always use big
//		- blocks). You MUST call the DocumentComplete method of TOLEWriter when you are
//		- done with all writing to all entries.
//
//	TOLEEntryWriter
//		- Provides methods for writing to the stream of data associated with a particular
//		- entry. You can intermix calls to different entry writers in the same document
//		- safely (but note that you are seeking around in the file when you do so.)
//		- NOTE: you are not allowed to set the stream position beyond what's been written.
//
//-------------------------------------------------------------------------------------
#ifndef __OLEUTILS_H__
#define __OLEUTILS_H__

#include <List.h>
#include "TranslationConsts.h"

#define BLOCKSIZE			512
#define INDEXES_PER_BLOCK	128
#define SMALLBLOCKSIZE		64
#define ENTRIES_PER_BLOCK	4
#define ENTRY_SIZE			(BLOCKSIZE/ENTRIES_PER_BLOCK)

#define UNUSED_BLOCK		0xFFFFFFFF
#define END_OF_CHAIN		0xFFFFFFFE
#define SPECIAL_BLOCK		0xFFFFFFFD


class TOLEReader;
class TOLEWriter;
class TOLEEntryReader;
class TOLEEntryWriter;

//-------------------------------------------------------------------------------------
// TInt32Table - Handy wrapper for table of int32 values.
//-------------------------------------------------------------------------------------
class TInt32Table : public BList
{
	public:
		TInt32Table()		{}
		~TInt32Table();
		
		int32 	operator[](int32 index) const;
		int32&	operator[](int32 index);
		void	AddValue(int32 value);
		void	DumpValues(void) const;
};

//-------------------------------------------------------------------------------------
// TOLEEntry
//
//		This simple class stores information about a directory entry in an OLE document.
//-------------------------------------------------------------------------------------
class TOLEEntry
{
	friend class TOLEReader;
	friend class TOLEWriter;
	friend class TOLEEntryWriter;
	friend class TOLEEntryReader;
	
	public:
		TOLEEntry(char type, char* name);
		~TOLEEntry();
		
		char				Type(void) const				{ return mType; }
		const char*			Name(void) const				{ return mName; }
		int32				DataSize(void) const			{ return mDataSize; }

		int32				NumBlocks(void) const			{ return mBlockList.CountItems(); }
		int32				BlockNumber(int32 index) const	{ return mBlockList[index]; }

		int32				PrevDirEntry(void) const		{ return mPrev; }
		int32				NextDirEntry(void) const		{ return mNext; }
		int32				ChildDirEntry(void) const		{ return mChild; }
		void				SetPrevDirEntry(int32 prev)		{ mPrev = prev; }
		void				SetNextDirEntry(int32 next)		{ mNext = next; }
		void				SetChildDirEntry(int32 child)	{ mChild = child; }
		
		void				DumpEntry(void) const;
				
	protected:
		void				SetDirInfo(int32 prev, int32 next, int32 child);
		void				SetDataSize(int32 dataSize)	{ mDataSize = dataSize; }
		TInt32Table&		BlockList(void)				{ return mBlockList; }
	
		char				mType;
		char*				mName;
		int32				mDataSize;
		int32				mPrev;
		int32				mNext;
		int32				mChild;
		TInt32Table			mBlockList;
};

//-------------------------------------------------------------------------------------
// TOLEEntryReader
//
//		This class is used to read the data from an OLE entry in as a contiguous
// stream of bytes even though the actual data blocks may be scattered around in the
// OLE document.
//-------------------------------------------------------------------------------------
class TOLEEntryReader
{
	public:
		TOLEEntryReader(const TOLEEntry* entry, TOLEReader* reader);
		~TOLEEntryReader();
		
		// Use these methods to read data from the stream.
		status_t			Error(void) const;
		char				ReadChar(void);
		short				ReadShort(void);
		long				ReadLong(void);
		double				ReadDouble(void);
		status_t			ReadBytes(char* buffer, int32 numBytes);
		
		// These methods deal with the relative position in the "contiguous" 
		// mapped stream of blocks.
		long				GetStreamPos(void) const;
		bool				SetStreamPos(long pos);
		long				StreamDataSize(void) const;
		
		// These methods deal with the position in the original fileSpec.
		// SetFilePos will return false if the position would be outside the stream.
		long				GetFilePos(void) const;
		bool				SetFilePos(long pos);
		
		// Returns stream position of the given file position. (FALSE if not in stream)
		bool				MapToStreamPos(long pos, long& streamPos) const;
	
	private:
		TOLEReader*			mReader;
		const TOLEEntry*	mEntry;

		long				mStreamPos;
		int32				mBlockIndex;
		int32				mBytesLeft;
		TOLEEntryReader*	mSmallBlocksReader;
		long				mFilePos;
};

//-------------------------------------------------------------------------------------
// TOLEReader
//
//		This is the main class used for top level access to an OLE document. It will
// open the given file stream and extract information about the OLE entries that exist
// in the document. If the document is not an OLE document it will have 0 entries.
//-------------------------------------------------------------------------------------
class TOLEReader
{
	friend class TOLEEntryReader;
	
	public:
		TOLEReader(BPositionIO* fileSpec);
		~TOLEReader();
		
		status_t			Error(void) const;
		int32				Entries(void) const;
		const TOLEEntry*	Entry(int32 index) const;
		TOLEEntryReader*	GetEntryReader(int32 index);
		
		void				DumpBlocks(void) const;
		void				DumpIndexes(void) const;
		
	protected:
		long				read_long(void);
		short				read_short(void);
		status_t			read_bytes(void* buffer, uint32 numBytes);

		long				seek(long pos);
		long				get_file_pos(void) const	{ return mFilePos; }
		
		void				seek_block(int32 blockNo);
		void				get_block_list(int32 startBlock, TInt32Table& blockList);
		void				get_small_block_list(int32 startBlock, TInt32Table& blockList);
		
		TOLEEntryReader*	small_blocks_reader(void);

	private:
		status_t			mErr;
		BPositionIO*		mFile;
		long				mFilePos;
		BList*				mEntriesList;
		int32				mRootStartBlock;
		TInt32Table			mBBIBlocks;
		TInt32Table			mBigBlockIndexes;
		TInt32Table			mSmallBlockIndexes;
		TInt32Table			mSmallBlockDepot;
		TOLEEntryReader*	mSmallBlocksFile;
};

//-------------------------------------------------------------------------------------
// TBlockIndexTable - This class is used to represent/manage an index table for blocks.
//-------------------------------------------------------------------------------------
class TBlockIndexTable
{
	public:
		TBlockIndexTable();
		~TBlockIndexTable();
		
		void				AddBlock(int32 blockNum);
		void				FlushBlocks(TOLEWriter* writer);
		
		int32*				FindIndexEntry(int32 index);
		int32				FindUnusedIndex(void) const;

		const TInt32Table&	BlockList(void) const				{ return mBlockList; }
		
	private:
		int32*				AllocateIndexBlock(void);

		TInt32Table			mBlockList;
		BList				mBlockData;	
};

//-------------------------------------------------------------------------------------
// TBigBlockManager
//
//		This class manages the allocation of all blocks used in an OLE document. It
// keeps the big block depot blocks and is responsible for keeping the indexes in those
// blocks correct. It can be called to either allocate a single new block in the file
// or add a block to an existing chain of blocks.
//-------------------------------------------------------------------------------------
class TBigBlockManager
{
	public:
		TBigBlockManager();
		~TBigBlockManager();	
		
		int32				NewBigBlock(void);		
		int32				AppendBigBlock(int32 lastBlock);
		void				FlushBigBlocksIndex(TOLEWriter* writer);		

		const TInt32Table&	BlockList(void) const;

	private:
		TBlockIndexTable	mIndexTable;
};

//-------------------------------------------------------------------------------------
// TSmallBlockManager
//
//		This class manages the allocation of small blocks (64 bytes) used in an OLE 
// document. It keeps the small block depot blocks and is responsible for keeping the
// indexes in those blocks correct. It can be called to either allocate a single new 
// small block or add a small block to an existing chain of small blocks.
//-------------------------------------------------------------------------------------
class TSmallBlockManager
{
	public:
		TSmallBlockManager(TBigBlockManager* blockMgr);
		~TSmallBlockManager();
		
		int32				NewSmallBlock(void);
		int32				AppendSmallBlock(int32 lastBlock);
		void				FlushSmallBlocksIndex(TOLEWriter* writer);
		
		const TInt32Table&	BlockList(void) const;

	private:	
		TBigBlockManager*	mBlockMgr;
		TBlockIndexTable	mIndexTable;
};

//-------------------------------------------------------------------------------------
// TOLEEntryWriter
//
//		This class is used to write data to an OLE entry as a contiguous stream of 
// bytes even though the actual data blocks may be scattered around in the OLE document.
//-------------------------------------------------------------------------------------
class TOLEEntryWriter
{
	friend class TOLEWriter;
	
	public:
		TOLEEntryWriter(TOLEEntry* entry, TOLEWriter* writer);
		
		// Use these methods to read data from the stream.
		status_t			Error(void) const;
		status_t			WriteChar(char value);
		status_t			WriteShort(short value);
		status_t			WriteLong(long value);
		status_t			WriteDouble(double value);
		status_t			WriteBytes(char* buffer, int32 numBytes);
		
		// These methods deal with the relative position in the "contiguous" 
		// mapped stream of blocks.
		long				StreamDataSize(void) const;
		long				GetStreamPos(void) const;
		bool				SetStreamPos(long pos);
		
	protected:
		~TOLEEntryWriter();
		status_t			FlushEntry(void);
	
	private:
		TOLEWriter*			mWriter;
		TOLEEntry*			mEntry;

		long				mStreamPos;
		long				mFilePos;
		int32				mBlockIndex;
		int32				mBytesLeft;
		char*				mCache;
		bool				mUseCache;
};

//-------------------------------------------------------------------------------------
// TOLEWriter
//
//		This is the main class used for top level writer access to an OLE document.
// An OLE structure will be written to the given fileSpec stream. The root entry will
// be created. The caller can then make calls to create ole entries and write data to
// them.
//-------------------------------------------------------------------------------------
class TOLEWriter
{
	friend class TOLEEntryWriter;
	friend class TBlockIndexTable;
	
	public:
		TOLEWriter(BPositionIO* fileSpec);
		~TOLEWriter();
		
		status_t			Error(void) const;
		int32				NewNextEntry(char type, char*name, int32 destEntry);
		int32				NewPrevEntry(char type, char*name, int32 destEntry);
		int32				NewChildEntry(char type, char*name, int32 destEntry);

		TOLEEntry*			Entry(int32 index);
		TOLEEntryWriter*	EntryWriter(int32 index);
		
		void				DocumentComplete(void);
		
	protected:
		void				FlushEntries(void);
		int32				NewEntry(char type, char* name);
		TBigBlockManager*	BigBlockMgr(void) const		{ return mBigBlockMgr; }
		TSmallBlockManager*	SmallBlockMgr(void) const	{ return mSmallBlockMgr; }
	
		status_t			write_char(char value);
		status_t			write_short(short value);
		status_t			write_long(long value);
		status_t			write_bytes(void* buffer, uint32 numBytes);
		status_t			write_header_block(void);

		long				get_file_pos(void) const;	

		long				seek(long pos);
		void				seek_block(int32 blockNo);
				
	private:
		status_t			mErr;	
		BPositionIO*		mFile;
		long				mFileSize;
		
		TBigBlockManager*	mBigBlockMgr;
		TSmallBlockManager*	mSmallBlockMgr;
		TOLEEntry*			mRootEntry;

		TInt32Table			mEntryBlockList;
		BList				mEntryList;
		BList				mEntryWriters;
};

#endif	// __OLEUTILS_H__

