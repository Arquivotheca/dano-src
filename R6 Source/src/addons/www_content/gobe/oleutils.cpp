//-------------------------------------------------------------------------------------
// oleutils.cpp
//-------------------------------------------------------------------------------------

#include <SupportKit.h>
#include "oleutils.h"
		
#define DUMP_DEPOT_BLOCKS	0
#define DUMP_INDEX_BLOCKS	0

TOLEReader::TOLEReader(BPositionIO* fileSpec)
{
	long	x, count;
	char 	buffer[BLOCKSIZE];

	mFile = fileSpec;
	mErr = B_NO_ERROR;
	mEntriesList = new BList();
	mSmallBlocksFile = NULL;
	
	// Verify that it is an OLE document.
	unsigned char identifier[8] = {0xd0, 0xcf, 0x11, 0xe0, 0xa1, 0xb1, 0x1a, 0xe1};
	mFile->Seek(0, SEEK_SET);
	mErr = read_bytes(buffer, 8);
	if (mErr)
	{
		IFDEBUG(fprintf(stderr, "Error reading from file %#6.4hx\n", mErr));
		return;
	}
	for (x = 0; x < 8; x++)
	{
		if ((uchar) buffer[x] != identifier[x])
		{
			IFDEBUG(fprintf(stderr, "%#4.2hx != %#4.2hx\n", buffer[x], identifier[x]));
			return;
		}
	}

	// Allocate array of all big block indexes blocks
	mFile->Seek(0x2c, SEEK_SET);
	long num_blocks = read_long();
	mFile->Seek(0x30, SEEK_SET);
	mRootStartBlock = read_long();
	mFile->Seek(0x3C, SEEK_SET);
	long sbdStartBlock = read_long();
	mFile->Seek(0x44, SEEK_SET);
	long bbiNext = read_long();
	mFile->Seek(0x4c, SEEK_SET);
	
	// Get table with chain of blocks that make up the big blocks chain.
	// Read big block chain blocks in zero block.
	int32 readBlocks = num_blocks < 110 ? num_blocks : 109;
	for (x = 0; x < readBlocks; x++)
		mBBIBlocks.AddValue( read_long() );

	// If number of blocks in the big blocks chain is larger than what fits
	// in block zero then look in bbiNext for the rest.
	readBlocks = num_blocks - readBlocks;
	if (readBlocks && bbiNext > 0)
	{
		ASSERTC(readBlocks < 129);
		seek_block(bbiNext);
		for (x = 0; x < readBlocks; x++)
			mBBIBlocks.AddValue( read_long() );
	}	
	
	// Read in the big block index lookup table.		
	for (x = 0; x < num_blocks; x++)
	{
		seek_block(mBBIBlocks[x]);
		for (int32 i = 0; i < BLOCKSIZE / 4; i++)
			mBigBlockIndexes.AddValue( read_long() );
	}
	
	// Read in big block chain for small block depot.
	get_block_list(sbdStartBlock, mSmallBlockDepot);
	long small_blocks = mSmallBlockDepot.CountItems();

	for (x = 0; x < small_blocks; x++)
	{
		seek_block( mSmallBlockDepot[x] );
		for (long i = 0; i < BLOCKSIZE / 4; i++)
			mSmallBlockIndexes.AddValue( read_long() );
	}

	#if DUMP_DEPOT_BLOCKS
	DumpBlocks();
	#endif	
	#if DUMP_INDEX_BLOCKS
	DumpIndexes();
	#endif
	
	// OLE storage entries are stored in the root blocks. 
	int32 block = mRootStartBlock;
	while (block >= 0)
	{
		// Get block data into buffer.
		seek_block(block);
		read_bytes(buffer, BLOCKSIZE);

		// 4 entries per block check each.
		for (long entryNum = 0; entryNum < 4; entryNum++)
		{
			char 	type;
			int32 	startBlock, dataSize;
			short	nameSize;
			BList*	blockList = NULL;			
			char*	name = NULL;
			char* 	ptr = buffer + (entryNum * 0x80);
			int32	prevEntry, nextEntry, childEntry;

			memcpy(&nameSize, ptr + 0x40, sizeof(short));
			nameSize = B_LENDIAN_TO_HOST_INT16(nameSize);
			if (!nameSize)
				continue;			
				
			name = (char*) malloc(nameSize+1);
			for (count = 0, x = 0; x < nameSize/2; x++)
			{
				char theChar = *(ptr + x*2);
				if ( theChar >= 0x20)
					name[count++] = theChar;
			}
			name[count] = 0;

			memcpy(&prevEntry, ptr + 0x44, sizeof(int32));
			prevEntry = B_LENDIAN_TO_HOST_INT32(prevEntry);
			memcpy(&nextEntry, ptr + 0x48, sizeof(int32));
			nextEntry = B_LENDIAN_TO_HOST_INT32(nextEntry);
			memcpy(&childEntry, ptr + 0x4C, sizeof(int32));
			childEntry = B_LENDIAN_TO_HOST_INT32(childEntry);

			type = *(ptr + 0x42);
			memcpy(&startBlock, ptr + 0x74, sizeof(int32));
			memcpy(&dataSize, ptr + 0x78, sizeof(int32));
			startBlock = B_LENDIAN_TO_HOST_INT32(startBlock);
			dataSize = B_LENDIAN_TO_HOST_INT32(dataSize);
			
			TOLEEntry* entry = new TOLEEntry(type, name);
			
			if (type == 2 && dataSize < 0x1000)
				get_small_block_list(startBlock, entry->BlockList());
			else if (type != 1)
				get_block_list(startBlock, entry->BlockList());
			
			entry->SetDataSize( dataSize );
			entry->SetDirInfo( prevEntry, nextEntry, childEntry );
			mEntriesList->AddItem( entry );				
		}

		// Advance to next block		
		block = mBigBlockIndexes[block];
	}

	// Get small blocks data reader
	mSmallBlocksFile = GetEntryReader(0);
}

TOLEReader::~TOLEReader()
{
	while (mEntriesList->CountItems())
		delete mEntriesList->RemoveItem(0L);
	delete mEntriesList;

	delete mSmallBlocksFile;
}

status_t TOLEReader::Error(void) const
{
	return mErr;
}

int32 TOLEReader::Entries(void) const
{
	return mEntriesList->CountItems();
}

const TOLEEntry* TOLEReader::Entry(int32 index) const
{
	return (const TOLEEntry*) mEntriesList->ItemAt(index);
}

TOLEEntryReader* TOLEReader::GetEntryReader(int32 index)
{
	return new TOLEEntryReader(Entry(index), this);
}

void TOLEReader::DumpBlocks(void) const
{
	IFDEBUG(fprintf(stderr, "Big block depot blocks: [root:%#6.4hx]", mRootStartBlock));
	mBBIBlocks.DumpValues();
	IFDEBUG(fprintf(stderr, "Small block depot blocks:"));
	mSmallBlockDepot.DumpValues();
}

void TOLEReader::DumpIndexes(void) const
{
	IFDEBUG(fprintf(stderr, "mBigBlockIndexes:%d", mBigBlockIndexes.CountItems()));
	mBigBlockIndexes.DumpValues();
	IFDEBUG(fprintf(stderr, "mSmallBlockIndexes:%d", mSmallBlockIndexes.CountItems()));
	mSmallBlockIndexes.DumpValues();
}

#pragma mark -
void TOLEReader::get_block_list(int32 startBlock, TInt32Table& blockList)
{
	if (startBlock < 0)
		return;
		
	int32 block = startBlock;
	do
	{
		blockList.AddValue( block );
		block = mBigBlockIndexes[block];		
	} while (block >= 0);
}

void TOLEReader::get_small_block_list(int32 startBlock, TInt32Table& blockList)
{
	if (startBlock < 0)
		return;
		
	int32 	block = startBlock;
	do
	{
		blockList.AddValue( block );
		block = mSmallBlockIndexes[block];		
	} while (block >= 0);
}

void TOLEReader::seek_block(long blockNum)
{
	mFile->Seek(BLOCKSIZE * (blockNum + 1), SEEK_SET);
}

short TOLEReader::read_short(void)
{
	short x;
	read_bytes(&x, sizeof(short));
	x = B_LENDIAN_TO_HOST_INT16(x);
	return x;
}

long TOLEReader::read_long(void)
{
	long x;
	read_bytes(&x, sizeof(long));
	x = B_LENDIAN_TO_HOST_INT32(x);
	return x;
}

status_t TOLEReader::read_bytes(void* buffer, uint32 numBytes)
{
	if (!mErr)
	{
		int32 bytesRead = mFile->Read(buffer, numBytes);
		if (bytesRead < 0)
			mErr = bytesRead;
		else if ((uint32) bytesRead != numBytes)
			mErr = B_ERROR;
	}
	return mErr;
}

long TOLEReader::seek(long pos)
{
	mFilePos = pos;
	return mFile->Seek(pos, SEEK_SET);
}

TOLEEntryReader* TOLEReader::small_blocks_reader(void)
{
	return mSmallBlocksFile;
}

#pragma mark -
// When TOLEWriter is created is creates a valid OLE document with a single entry
// for the root.
TOLEWriter::TOLEWriter(BPositionIO* fileSpec)
{
	mFileSize = 0;
	mFile = fileSpec;
	mErr = B_NO_ERROR;

	IFDEBUG(fprintf(stderr, "TOLEWriter: create mBigBlockMgr\n"));
	mBigBlockMgr = new TBigBlockManager();

	IFDEBUG(fprintf(stderr, "TOLEWriter: create mSmallBlockMgr\n"));
	mSmallBlockMgr = new TSmallBlockManager(mBigBlockMgr);

	int32 rootIndex = NewEntry(5, "Root Entry");
	mRootEntry = Entry(rootIndex);

	IFDEBUG(fprintf(stderr, "TOLEWriter: constructed\n"));
}

TOLEWriter::~TOLEWriter()
{
	while (mEntryList.CountItems())
		delete mEntryList.RemoveItem(0L);
	while (mEntryWriters.CountItems())
		delete mEntryWriters.RemoveItem(0L);

	IFDEBUG(fprintf(stderr, "Delete mSmallBlockMgr...\n"));
	delete mSmallBlockMgr;
	IFDEBUG(fprintf(stderr, "Delete mBigBlockMgr...\n"));
	delete mBigBlockMgr;
	IFDEBUG(fprintf(stderr, "~TOLEWriter complete.\n"));
}

status_t TOLEWriter::Error(void) const
{
	return mErr;
}

void TOLEWriter::DocumentComplete(void)
{
	FlushEntries();
	mSmallBlockMgr->FlushSmallBlocksIndex(this);
	mBigBlockMgr->FlushBigBlocksIndex(this);
	write_header_block();
}

int32 TOLEWriter::NewEntry(char type, char* name)
{
	int32 entryIndex = mEntryList.CountItems();
	TOLEEntry* entryPtr = new TOLEEntry(type, name);
	mEntryList.AddItem( entryPtr );
	TOLEEntryWriter* writer = new TOLEEntryWriter( entryPtr, this );
	mEntryWriters.AddItem( writer );
	return entryIndex;
}

int32 TOLEWriter::NewNextEntry(char type, char*name, int32 destEntry)
{
	int32 newIndex = NewEntry(type, name);
	TOLEEntry* entryPtr = Entry(destEntry);
	IFDEBUG(if (entryPtr->NextDirEntry() != -1) fprintf(stderr, "Already has next entry!!\n"));
	entryPtr->SetNextDirEntry(newIndex);
	return newIndex;
}

int32 TOLEWriter::NewPrevEntry(char type, char*name, int32 destEntry)
{
	int32 newIndex = NewEntry(type, name);
	TOLEEntry* entryPtr = Entry(destEntry);
	IFDEBUG(if (entryPtr->PrevDirEntry() != -1) fprintf(stderr, "Already has prev entry!!\n"));
	entryPtr->SetPrevDirEntry(newIndex);
	return newIndex;
}

int32 TOLEWriter::NewChildEntry(char type, char*name, int32 destEntry)
{
	int32 newIndex = NewEntry(type, name);
	TOLEEntry* entryPtr = Entry(destEntry);
	IFDEBUG(if (entryPtr->ChildDirEntry() != -1) fprintf(stderr, "Already has child entry!!\n"));
	entryPtr->SetChildDirEntry(newIndex);
	return newIndex;
}

TOLEEntry* TOLEWriter::Entry(int32 index)
{
	return (TOLEEntry*) mEntryList.ItemAt(index);
}

TOLEEntryWriter* TOLEWriter::EntryWriter(int32 entryIndex)
{
	return (TOLEEntryWriter*) mEntryWriters.ItemAt(entryIndex);
}

void TOLEWriter::FlushEntries(void)
{
	IFDEBUG(fprintf(stderr, "Flushing entries...\n"));
	// Get enough blocks to hold all the entries.
	int32 count = mEntryList.CountItems();
	int32 numBlocks = (count / ENTRIES_PER_BLOCK) + 1;
	int32 blockNo = mBigBlockMgr->NewBigBlock();
	mEntryBlockList.AddValue( blockNo );
	for (long x = 1; x < numBlocks; x++)
	{
		blockNo = mBigBlockMgr->AppendBigBlock( blockNo );
		mEntryBlockList.AddValue( blockNo );
		IFDEBUG(fprintf(stderr, "... added big block %d to mEntryBlockList.\n", blockNo ));
	}
	
	// Flush the non-root entries.
	for (long entry = 1; entry < count; entry++)
	{
		TOLEEntryWriter* entryWriter = EntryWriter(entry);
		entryWriter->FlushEntry();
	}
	// Flush root last since it has all the short block data in it.
	EntryWriter(0)->FlushEntry();

	IFDEBUG(fprintf(stderr, "... write entry information to disk\n"));	

	// Write entry info for each entry.
	for (long x = 0; x < count; x++)
	{
		if (! (x % ENTRIES_PER_BLOCK) )
			seek_block( mEntryBlockList[ x / ENTRIES_PER_BLOCK ] );

		TOLEEntry* entryPtr = Entry(x);
		const char* name = entryPtr->Name();
		int32 nameLen = strlen(name);
		for (long x = 0; x < 0x20; x++)
		{
			short aChar = (x < nameLen) ? name[x] : 0;
			write_short(aChar);
		}
		write_short((nameLen+1)*2);
		write_short(entryPtr->Type());
		write_long(entryPtr->PrevDirEntry());
		write_long(entryPtr->NextDirEntry());
		write_long(entryPtr->ChildDirEntry());
		for (long x = 0x50; x < 0x74; x++)
			write_char(0);
		
		if (entryPtr->BlockList().CountItems())
			write_long(entryPtr->BlockList()[0]);
		else
			write_long(UNUSED_BLOCK);			
		write_long(entryPtr->DataSize());
		write_long(0);
	}
	
	// Write out blank entries to fill the block.
	int32 padCount = ENTRIES_PER_BLOCK - (count % ENTRIES_PER_BLOCK);
	char padBytes[ENTRY_SIZE];
	memset(padBytes, 0x0000, ENTRY_SIZE);
	for (long i = 0; i < padCount; i++)
		write_bytes(padBytes, ENTRY_SIZE);
}

#pragma mark -
status_t TOLEWriter::write_char(char value)
{
	return write_bytes(&value, sizeof(char));
}

status_t TOLEWriter::write_short(short value)
{
	value = B_HOST_TO_LENDIAN_INT16(value);
	return write_bytes(&value, sizeof(int16));
}

status_t TOLEWriter::write_long(long value)
{
	value = B_HOST_TO_LENDIAN_INT32(value);
	return write_bytes(&value, sizeof(int32));
}

status_t TOLEWriter::write_bytes(void* buffer, uint32 numBytes)
{
	#if 0
	long pos = mFile->Position();
	IFDEBUG(fprintf(stderr, "   <write_bytes %d bytes to %#4hx in file.>\n", numBytes, pos));
	#endif	// DEBUG

	if (!mErr)
	{
		const char* ptr = (const char*) buffer;
		ssize_t writeBytes = numBytes;
		while (!mErr && writeBytes)
		{
			ssize_t writeSize = writeBytes;			
			ssize_t bytesWritten = mFile->Write(ptr, writeSize);
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

status_t TOLEWriter::write_header_block(void)
{
	IFDEBUG(fprintf(stderr, "write_header_block\n"));
	char buffer[ BLOCKSIZE ];
	
	// Write OLE document header.
	memset(buffer, 0xFFFF, BLOCKSIZE);
	memset(buffer, 0, 0x4c);

	unsigned char identifier[8] = {0xd0, 0xcf, 0x11, 0xe0, 0xa1, 0xb1, 0x1a, 0xe1};
	memcpy(buffer, identifier, 8);
	
	int16 value = B_HOST_TO_LENDIAN_INT16( 0x3b );
	memcpy(buffer + 0x18, &value, sizeof(int16));
	value = B_HOST_TO_LENDIAN_INT16( 0x03 );
	memcpy(buffer + 0x1a, &value, sizeof(int16));
	value = B_HOST_TO_LENDIAN_INT16( -2 );
	memcpy(buffer + 0x1c, &value, sizeof(int16));
	buffer[0x1e] = 0x09;

	int32 longValue = B_HOST_TO_LENDIAN_INT32( 0x0006 );
	memcpy(buffer + 0x20, &longValue, sizeof(int32));
	longValue = B_HOST_TO_LENDIAN_INT32( 0x1000 );
	memcpy(buffer + 0x38, &longValue, sizeof(int32));
	longValue = B_HOST_TO_LENDIAN_INT32( 0x0001 );
	memcpy(buffer + 0x40, &longValue, sizeof(int32));
	longValue = B_HOST_TO_LENDIAN_INT32( -2 );
	memcpy(buffer + 0x44, &longValue, sizeof(int32));
	
	const TInt32Table& bigBlocksTab = mBigBlockMgr->BlockList();
	int32 numBBIBlocks = bigBlocksTab.CountItems();	
	longValue = B_HOST_TO_LENDIAN_INT32( numBBIBlocks );
	memcpy(buffer + 0x2c, &longValue, sizeof(int32));
	
	int32 rootStartBlock = mEntryBlockList[0];
	longValue = B_HOST_TO_LENDIAN_INT32( rootStartBlock );
	memcpy(buffer + 0x30, &longValue, sizeof(int32));
	
	const TInt32Table& smallBlocksTab = mSmallBlockMgr->BlockList();
	longValue = B_HOST_TO_LENDIAN_INT32( smallBlocksTab[0] );
	memcpy(buffer + 0x3C, &longValue, sizeof(int32));

	long* ptr = (long*) (buffer + 0x4c);
	for (long x = 0; x < numBBIBlocks; x++)
	{
		longValue = B_HOST_TO_LENDIAN_INT32( bigBlocksTab[x] );
		memcpy(ptr, &longValue, sizeof(int32));
		ptr++;
	}
	
	// write out the header block.
	long oldPos = get_file_pos();
	mFile->Seek(0, SEEK_SET);
	status_t result = write_bytes(buffer, BLOCKSIZE);
	seek(oldPos);
	return result;
}

long TOLEWriter::seek(long pos)
{
	IFDEBUG(fprintf(stderr, "TOLEWriter::seek(%#6.4hx) {FileSize:%#6.4hx}\n", pos, mFileSize));
	// Grow file as needed to allow the seek to succeed.
	// Presumably we'll go back and fill in the missing blocks.
	if (pos > mFileSize)
	{
		status_t result = mFile->SetSize(pos);
		IFDEBUG(if (result != B_NO_ERROR) fprintf(stderr, "TOLEWriter::seek failed to set size!\n"));
		mFileSize = pos;
	}
		
	return mFile->Seek(pos, SEEK_SET);
}

void TOLEWriter::seek_block(long blockNum)
{
	seek(BLOCKSIZE * (blockNum + 1));
}

long TOLEWriter::get_file_pos(void) const
{
	return mFile->Position();
}

#pragma mark -
//================================================================================
TOLEEntry::TOLEEntry(char type, char* name)
{
	mType = type;
	mName = name;
	mDataSize = 0;
	mPrev = -1;
	mNext = -1;
	mChild = -1;
}

TOLEEntry::~TOLEEntry()
{
	delete mName;
}

void TOLEEntry::SetDirInfo(int32 prev, int32 next, int32 child)
{
	mPrev = prev;
	mNext = next;
	mChild = child;
}

void TOLEEntry::DumpEntry(void) const
{
	fprintf(stderr, "Name: %s\n", mName);
	fprintf(stderr, "   Type: %d ", mType);
	fprintf(stderr, "Size: %#6.4hx ", mDataSize);
	fprintf(stderr, "Prev: %d  Next: %d  Dir: %d\n", mPrev, mNext, mChild);

	if (mType == 1)
		return;
	if (mType == 5 || mDataSize >= 0x1000)
		fprintf(stderr, "   Big Blocks:");
	else
		fprintf(stderr, "   Small Blocks:");		
	mBlockList.DumpValues();		
}

#pragma mark -
//================================================================================
TOLEEntryReader::TOLEEntryReader(const TOLEEntry* entry, TOLEReader* reader)
{
	mEntry = entry;
	mReader = reader;
	mSmallBlocksReader = NULL;
	if (mEntry->Type() == 2 && mEntry->DataSize() < 0x1000)
		mSmallBlocksReader = reader->small_blocks_reader();

	mBytesLeft = 0;
	mBlockIndex = 0;
	mStreamPos = 0;
	mFilePos = 0;
}

TOLEEntryReader::~TOLEEntryReader()
{
}

status_t TOLEEntryReader::Error(void) const
{
	return mReader->Error();
}

char TOLEEntryReader::ReadChar(void)
{
	char temp;
	ReadBytes((char*) &temp, sizeof(char));
	return temp;
}

short TOLEEntryReader::ReadShort(void)
{
	short temp;	
	ReadBytes((char*) &temp, sizeof(short));
	temp = B_LENDIAN_TO_HOST_INT16(temp);
	return temp;
}

long TOLEEntryReader::ReadLong(void)
{
	long temp;
	status_t result = ReadBytes((char*) &temp, sizeof(long));
	temp = B_LENDIAN_TO_HOST_INT32(temp);
	return temp;
}

double TOLEEntryReader::ReadDouble(void)
{
	double temp;
	ReadBytes((char*) &temp, sizeof(double));
	temp = B_LENDIAN_TO_HOST_DOUBLE(temp);
	return temp;
}

status_t TOLEEntryReader::ReadBytes(char* buffer, long numBytes)
{
	status_t err = mReader->Error();
	ASSERT(numBytes >= 0, "OLEEntryReader: asked to read negative number of bytes!");
	
	// Make sure file position is where it should be.
	if (!mFilePos)
	{
		IFDEBUG(fprintf(stderr, "Starting stream for %s\n", mEntry->Name()));
		SetStreamPos(0);
	}
	else if (mFilePos != mReader->get_file_pos())
	{
		IFDEBUG(fprintf(stderr, "Active entry switch to %s (%#6.4hx to %#6.4hx)\n", mEntry->Name(), mReader->get_file_pos(), mFilePos));
		mReader->seek(mFilePos);
	}
		
	#ifdef DEBUG
//	if (mSmallBlocksReader || mEntry->Name() == mReader->Entry(0)->Name())
//		fprintf(stderr, "   ** %s ** ReadBytes(%d) [mStreamPos: %#6.4hx  mBytesLeft: %d]\n", mEntry->Name(), numBytes, mStreamPos, mBytesLeft);
	#endif

	// Loop until we have read in all the requested bytes.
	while (!err && numBytes)
	{
		// Simple case block still has enough bytes left just read them in.
		if (numBytes <= mBytesLeft)
		{
			if (mSmallBlocksReader)
				err = mSmallBlocksReader->ReadBytes(buffer, numBytes);
			else
				err = mReader->read_bytes(buffer, numBytes);
					
			if (!err)
			{			
				mBytesLeft -= numBytes;
				if (mBytesLeft)
					mStreamPos += numBytes;
				else
					SetStreamPos(mStreamPos + numBytes);
			}
			break;
		}
		
		// Read all the bytes that we can first then the rest.
		if (mBytesLeft)
		{
			if (mSmallBlocksReader)
				err = mSmallBlocksReader->ReadBytes(buffer, mBytesLeft);
			else
				err = mReader->read_bytes(buffer, mBytesLeft);
			
			if (!err)
			{	
				buffer += mBytesLeft;
				numBytes -= mBytesLeft;
			}
		}
		
		// Go to the next block in this stream
		SetStreamPos(mStreamPos + mBytesLeft);
	}
	mFilePos = mReader->get_file_pos();
	return err;
}

long TOLEEntryReader::GetStreamPos(void) const
{
	return mStreamPos;
}

bool TOLEEntryReader::SetStreamPos(long pos)
{
	#ifdef DEBUG
//	if (mSmallBlocksReader || mEntry->Name() == mReader->Entry(0)->Name())
//		fprintf(stderr, "   ** %s ** SetStreamPos(%#6.4hx)\n", mEntry->Name(), pos);
	#endif

	if (pos < 0 || pos > StreamDataSize())
	{
		IFDEBUG(fprintf(stderr, "SetStreamPos: bad pos - %d, StreamDataSize - %d\n", pos, StreamDataSize()));
		return false;
	}
	
	long blockSize = (mSmallBlocksReader) ? SMALLBLOCKSIZE : BLOCKSIZE;
	mBlockIndex = pos / blockSize;
	long block = mEntry->BlockNumber( mBlockIndex );
	
	if (mSmallBlocksReader)
	{
		long smallStreamPos = block * SMALLBLOCKSIZE + (pos % SMALLBLOCKSIZE);
		mSmallBlocksReader->SetStreamPos(smallStreamPos);
	}
	else
	{
		long actual = (block+1) * BLOCKSIZE + (pos % BLOCKSIZE);
		long result = mReader->seek(actual);
		ASSERTC(result == actual);
	}

	mFilePos = mReader->get_file_pos();
	mStreamPos = pos;
	mBytesLeft = blockSize - (pos % blockSize);
	return true;
}

long TOLEEntryReader::StreamDataSize(void) const
{
	return mEntry->DataSize();
}

long TOLEEntryReader::GetFilePos(void) const
{
	// If small blocks the small block stream position should be used to get current file position
	if (mSmallBlocksReader)
		return mSmallBlocksReader->GetFilePos();
		
	long index = mStreamPos / BLOCKSIZE;
	long block = mEntry->BlockNumber( index );
	long filePos = (block+1) * BLOCKSIZE + ( mStreamPos % BLOCKSIZE );
	return filePos;
}

bool TOLEEntryReader::SetFilePos(long pos)
{
	// If small blocks reader let it set the file position for its blocks.
	if (mSmallBlocksReader)
	{
		long streamPos;
		if (!mSmallBlocksReader->MapToStreamPos(pos, streamPos))
			return false;
		if (!MapToStreamPos(streamPos, mStreamPos))
			return false;
		if (!mSmallBlocksReader->SetFilePos(pos))
			return false;
		mBlockIndex = mStreamPos / SMALLBLOCKSIZE;
		mBytesLeft = SMALLBLOCKSIZE - (mStreamPos % SMALLBLOCKSIZE);
		return true;
	}

	// Make sure file position is in stream.
	if (!MapToStreamPos(pos, mStreamPos))
		return false;
	mReader->seek(pos);
	mBlockIndex = mStreamPos / BLOCKSIZE;
	mBytesLeft = BLOCKSIZE - (mStreamPos % BLOCKSIZE);
	return true;
}

bool TOLEEntryReader::MapToStreamPos(long pos, long& streamPos) const
{
	long 	blockStart = 0, blockIndex;
	bool 	found = false;
	long 	blockSize = (mSmallBlocksReader) ? SMALLBLOCKSIZE : BLOCKSIZE;	

	// Find the block the file/smallstream position will be in.
	blockIndex = 0;
	while (!found && blockIndex < mEntry->NumBlocks())
	{
		long block = mEntry->BlockNumber( blockIndex );
		if (mSmallBlocksReader)
			blockStart = block * SMALLBLOCKSIZE;
		else		
			blockStart = (block+1) * BLOCKSIZE;

		found = (pos >= blockStart && pos < blockStart + blockSize);
		if (!found)
			blockIndex++;
	}
	if (found)
		streamPos = blockIndex * blockSize + ((pos - blockStart) % blockSize);
	return found;
}

#pragma mark -
TOLEEntryWriter::TOLEEntryWriter(TOLEEntry* entry, TOLEWriter* writer)
{
	mWriter = writer;
	mEntry = entry;
	mStreamPos = 0;
	mFilePos = 0;
	mBytesLeft = 0;
	mBlockIndex = 0;
	mEntry->SetDataSize(0);
	mCache = (char*) malloc( 0x1000 );
	memset(mCache, 0xFFFF, 0x1000);
	mUseCache = (mEntry->Type() == 2);
}

TOLEEntryWriter::~TOLEEntryWriter()
{
	delete mCache;
}

status_t TOLEEntryWriter::Error(void) const
{
	return mWriter->Error();
}

status_t TOLEEntryWriter::WriteChar(char value)
{
	return WriteBytes(&value, sizeof(char));
}

status_t TOLEEntryWriter::WriteShort(short value)
{
	short temp = B_HOST_TO_LENDIAN_INT16(value);
	return WriteBytes((char*) &temp, sizeof(short));
}

status_t TOLEEntryWriter::WriteLong(long value)
{
	long temp = B_HOST_TO_LENDIAN_INT32(value);
	return WriteBytes((char*) &temp, sizeof(long));
}

status_t TOLEEntryWriter::WriteDouble(double value)
{
	double temp = B_HOST_TO_LENDIAN_DOUBLE(value);
	return WriteBytes((char*) &temp, sizeof(double));
}

status_t TOLEEntryWriter::WriteBytes(char* buffer, long numBytes)
{
	status_t err = mWriter->Error();
	
	// If data fits in the cache just add it to the cache and inc size.
	if (mUseCache && mStreamPos + numBytes < 0x1000)
	{
//		IFDEBUG(fprintf(stderr, "WriteBytes %#6.4hx to cache [%s %#6.4hx]: \n", numBytes, mEntry->Name(), mStreamPos));
		memcpy(mCache + mStreamPos, buffer, numBytes);
		mStreamPos += numBytes;
		if (mStreamPos > StreamDataSize())
			mEntry->SetDataSize(mStreamPos);
		return err;
	}

	// If we were using the cache clear it out.
	if (mUseCache)
	{
		IFDEBUG(fprintf(stderr, "WriteBytes: clear the cache\n"));
		long size = mStreamPos;
		mUseCache = false;
		mStreamPos = 0;
		err = WriteBytes(mCache, size);
		delete mCache;
		mCache = NULL;
	}
	
//	IFDEBUG(fprintf(stderr, "WriteBytes %#6.4hx to [%s %#6.4hx]: \n", numBytes, mEntry->Name(), mStreamPos));

	// If the file position has been changed by a different writer set it back for us.
	if (mFilePos && mFilePos != mWriter->get_file_pos())
	{
		IFDEBUG(fprintf(stderr, "Active entry switch to %s (%#6.4hx to %#6.4hx)\n", mEntry->Name(), mWriter->get_file_pos(), mFilePos));
		mWriter->seek(mFilePos);
	}
	
	// Loop writing bytes until they have all be written out.
	TInt32Table& blockList = mEntry->BlockList();
	while (!err && numBytes)
	{
		if (numBytes <= mBytesLeft)
		{
			err = mWriter->write_bytes(buffer, numBytes);
			mBytesLeft -= numBytes;
			mStreamPos += numBytes;
			if (mStreamPos > StreamDataSize())
				mEntry->SetDataSize(mStreamPos);
			break;
		}
		
		// Write what we can to fill the block.
		if (mBytesLeft)
		{
			err = mWriter->write_bytes(buffer, mBytesLeft);
			buffer += mBytesLeft;
			numBytes -= mBytesLeft;
			mStreamPos += mBytesLeft;
			if (mStreamPos > StreamDataSize())
				mEntry->SetDataSize(mStreamPos);
		}
		
		// Out of bytes in the block get the next block if it exists.
		int32 nextIndex = mStreamPos / BLOCKSIZE;
		int32 numBlocks = blockList.CountItems();
		if (nextIndex < numBlocks)
		{
			mBlockIndex = nextIndex;
			mWriter->seek_block( blockList[mBlockIndex] );
			mBytesLeft = BLOCKSIZE;
			continue;
		}
		
		// Allocate a new block.
		if (!numBlocks)
		{
			blockList.AddValue( mWriter->BigBlockMgr()->NewBigBlock() );
			mBlockIndex = 0;
		}
		else
		{
			int32 blockCount = blockList.CountItems();
			blockList.AddValue( mWriter->BigBlockMgr()->AppendBigBlock(blockList[blockCount-1]) );
			mBlockIndex = blockCount;
		}
		IFDEBUG(fprintf(stderr, "[%s] WriteBytes add big block blockList[%d] = %d\n", mEntry->Name(), mBlockIndex, blockList[mBlockIndex]) );
		mWriter->seek_block( blockList[mBlockIndex] );
		mBytesLeft = BLOCKSIZE;
	}

	mFilePos = mWriter->get_file_pos();
	return Error();
}

long TOLEEntryWriter::GetStreamPos(void) const
{
	return mStreamPos;
}

long TOLEEntryWriter::StreamDataSize(void) const
{
	return mEntry->DataSize();
}

bool TOLEEntryWriter::SetStreamPos(long pos)
{
	// If position would be outside stream so far written return error.
	if (pos > StreamDataSize())
	{
		IFDEBUG(fprintf(stderr, "[%s] TOLEEntryWriter::SetStreamPos(%#6.4hx) trying to write past EOS:%#6.4hx\n", mEntry->Name(), pos, StreamDataSize()));
		return false;
	}
	
	// If still in cache just set it.
	if (mUseCache)
	{
		mStreamPos = pos;
		return true;
	}
	
	// Find new current block Index and	
	TInt32Table& blockList = mEntry->BlockList();
	int32 index = pos / BLOCKSIZE;
	while (index >= blockList.CountItems())
	{
		if (!blockList.CountItems())
		{
			blockList.AddValue( mWriter->BigBlockMgr()->NewBigBlock() );
			mBlockIndex = 0;
		}
		else
		{
			int32 blockCount = blockList.CountItems();
			blockList.AddValue( mWriter->BigBlockMgr()->AppendBigBlock(blockList[blockCount-1]) );
			mBlockIndex = blockCount;
		}
		IFDEBUG(fprintf(stderr, "[%s] SetStreamPos(%#6.4hx) add big block blockList[%d] = %d.\n", mEntry->Name(), pos, mBlockIndex, blockList[mBlockIndex]) );
	}

	mBlockIndex = index;
	mStreamPos = pos;
	mBytesLeft = BLOCKSIZE - (pos % BLOCKSIZE);
	mFilePos = (blockList[mBlockIndex]+1) * BLOCKSIZE + (pos % BLOCKSIZE);
	mWriter->seek( mFilePos );
	return true;
}

status_t TOLEEntryWriter::FlushEntry(void)
{
	status_t err = mWriter->Error();
//	IFDEBUG(fprintf(stderr, "[%s] FlushEntry {size:%#6.4hx}.\n", mEntry->Name(), mEntry->DataSize()));
	
	// If not using cache then data has already been written out.
	if (!mUseCache)
		return err;
	
	TOLEEntryWriter* rootWriter = mWriter->EntryWriter(0);

	// Loop writing all the cached bytes to small blocks.
	int32 	numBytes = StreamDataSize();
	char* 	buffer = mCache;
	TInt32Table& blockList = mEntry->BlockList();
	
	while (!err && numBytes)
	{
		if (numBytes <= mBytesLeft)
		{
			// All the remaining bytes fit in a block.
//			rootWriter->WriteBytes(buffer, numBytes);
			err = rootWriter->WriteBytes(buffer, mBytesLeft);
			return err;
		}
		
		// Write what we can to fill the block.
		if (mBytesLeft)
		{
			err = rootWriter->WriteBytes(buffer, mBytesLeft);
			buffer += mBytesLeft;
			numBytes -= mBytesLeft;
		}
		
		// Allocate a new block.
		int32 numBlocks = blockList.CountItems();
		if (!numBlocks)
		{
			blockList.AddValue( mWriter->SmallBlockMgr()->NewSmallBlock() );
			mBlockIndex = 0;
		}
		else
		{
			int32 blockCount = blockList.CountItems();
			blockList.AddValue( mWriter->SmallBlockMgr()->AppendSmallBlock(blockList[blockCount-1]) );
			mBlockIndex = blockCount;
		}
//		IFDEBUG(fprintf(stderr, "[%s] FlushEntry add small block blockList[%d] == %d\n", mEntry->Name(), mBlockIndex, blockList[mBlockIndex]) );
		long pos = blockList[mBlockIndex] * SMALLBLOCKSIZE;
		bool result = rootWriter->SetStreamPos( pos );
//		IFDEBUG( if (!result) fprintf(stderr, "FlushEntry SetStreamPos(%#6.4hx) failed. blockList[%d] == %d\n", pos, mBlockIndex, blockList[mBlockIndex]) );
		mBytesLeft = SMALLBLOCKSIZE;
	}
	return Error();
}
#pragma mark -
TInt32Table::~TInt32Table()
{
	while (CountItems())
		RemoveItem(0L);
}

int32 TInt32Table::operator[](int32 index) const
{
	return reinterpret_cast<int32>(ItemAt(index));
}

int32& TInt32Table::operator[](int32 index)
{
	int32* items = reinterpret_cast<int32*>(Items());
	return items[index];
}

void TInt32Table::AddValue(int32 value)
{
	AddItem((void*) value);
}

void TInt32Table::DumpValues(void) const
{
	for (long x = 0; x < CountItems(); x++)
	{
		if (!(x%8))
			fprintf(stderr, "\n   ");
		fprintf( stderr, " %#6.4hx", reinterpret_cast<int32>(ItemAt(x)) );
	}
	fprintf(stderr, "\n");	
}

#pragma mark -
TBlockIndexTable::TBlockIndexTable()
{
}

TBlockIndexTable::~TBlockIndexTable()
{
	while (mBlockData.CountItems())
		delete mBlockData.RemoveItem(0L);	
}

void TBlockIndexTable::AddBlock(int32 blockNum)
{
	mBlockList.AddValue( blockNum );
	int32* blockData = AllocateIndexBlock();
	mBlockData.AddItem( blockData );
}

void TBlockIndexTable::FlushBlocks(TOLEWriter* writer)
{
	for (long x = 0; x < mBlockList.CountItems(); x++)
	{
		writer->seek_block(mBlockList[x]);
		int32* ptr = (int32*) mBlockData.ItemAt(x);
		for (long y = 0; y < INDEXES_PER_BLOCK; y++)
			writer->write_long(*ptr++);
	}
}

int32* TBlockIndexTable::FindIndexEntry(int32 index)
{
	int32 blockIndex = index / INDEXES_PER_BLOCK;	
	if (blockIndex < 0 || blockIndex >= mBlockList.CountItems())
	{
		IFDEBUG(fprintf(stderr, "TBlockIndexTable::FindIndexEntry(%d) not found.\n", index));
		return NULL;
	}
		
	int32* blockData = (int32*) mBlockData.ItemAt(blockIndex);
	blockData += index % 128;
	return blockData;
}

int32 TBlockIndexTable::FindUnusedIndex(void) const
{
	for (long x = 0; x < mBlockData.CountItems(); x++)
	{
		int32* blockData = (int32*) mBlockData.ItemAt(x);
		for (long y = 0; y < INDEXES_PER_BLOCK; y++)
		{
			if (blockData[y] == (int32) UNUSED_BLOCK)
				return x * INDEXES_PER_BLOCK + y;
		}
	}
	return UNUSED_BLOCK;
}

int32* TBlockIndexTable::AllocateIndexBlock(void)
{
	// When we allocate blocks we prefill them with UNUSED_BLOCK value
	int32* blockData = (int32*) malloc( BLOCKSIZE );
	for (long x = 0; x < INDEXES_PER_BLOCK; x++)
		blockData[x] = UNUSED_BLOCK;
	return blockData;
}

#pragma mark -

TBigBlockManager::TBigBlockManager()
{
	mIndexTable.AddBlock(0);
	int32* indexPtr = mIndexTable.FindIndexEntry(0);
	*indexPtr = SPECIAL_BLOCK;
}

TBigBlockManager::~TBigBlockManager()
{
}

int32 TBigBlockManager::NewBigBlock(void)
{
	int32 blockNo = mIndexTable.FindUnusedIndex();
//	IFDEBUG(fprintf(stderr, "NewBigBlock index found %#6.4hx\n", blockNo));

	// If we are out of index blocks then add a new block and try again.	
	if (blockNo == (int32) UNUSED_BLOCK)
	{
		int32 count = mIndexTable.BlockList().CountItems();
		int32 newBlock = count * INDEXES_PER_BLOCK;
		mIndexTable.AddBlock(newBlock);		
		int32* newEntry = mIndexTable.FindIndexEntry( newBlock );
		*newEntry = SPECIAL_BLOCK;

		IFDEBUG(fprintf(stderr, "NewBigBlock allocated new index block %#6.4hx\n", newBlock));
		
		blockNo = mIndexTable.FindUnusedIndex();
	}

//	IFDEBUG(fprintf(stderr, "NewBigBlock returning block %#6.4hx\n", blockNo));

	// Mark this block as end of chain.
	int32* indexPtr = mIndexTable.FindIndexEntry( blockNo );
	*indexPtr = END_OF_CHAIN;
	return blockNo;
}

int32 TBigBlockManager::AppendBigBlock(int32 lastBlock)
{
	int32 	newBlock = NewBigBlock();
	int32*	blockEntry = mIndexTable.FindIndexEntry( lastBlock );

	if (blockEntry)	
		*blockEntry = newBlock;
	return newBlock;
}

void TBigBlockManager::FlushBigBlocksIndex(TOLEWriter* writer)
{
	IFDEBUG(fprintf(stderr, "TBigBlockManager flushing blocks...\n"));
	mIndexTable.FlushBlocks(writer);
}

const TInt32Table& TBigBlockManager::BlockList(void) const
{
	return mIndexTable.BlockList();
}

#pragma mark -
TSmallBlockManager::TSmallBlockManager(TBigBlockManager* blockMgr)
{
	mBlockMgr = blockMgr;
}

TSmallBlockManager::~TSmallBlockManager()
{
}

int32 TSmallBlockManager::NewSmallBlock(void)
{
	int32 blockNo = mIndexTable.FindUnusedIndex();

	// If we are out of index blocks then add a new block and try again.	
	if (blockNo == (int32) UNUSED_BLOCK)
	{
		int32 newBlock = mBlockMgr->NewBigBlock();
		mIndexTable.AddBlock(newBlock);
		blockNo = mIndexTable.FindUnusedIndex();
	}

	// Mark this block as end of chain.
	int32* indexPtr = mIndexTable.FindIndexEntry( blockNo );
	*indexPtr = END_OF_CHAIN;
	return blockNo;
}

int32 TSmallBlockManager::AppendSmallBlock(int32 lastBlock)
{
	int32 	newBlock = NewSmallBlock();
	int32*	blockEntry = mIndexTable.FindIndexEntry( lastBlock );

	if (blockEntry)	
		*blockEntry = newBlock;
	return newBlock;
}

void TSmallBlockManager::FlushSmallBlocksIndex(TOLEWriter* writer)
{
	IFDEBUG(fprintf(stderr, "TSmallBlockManager flushing blocks...\n"));
	mIndexTable.FlushBlocks(writer);
}

const TInt32Table& TSmallBlockManager::BlockList(void) const
{
	return mIndexTable.BlockList();
}
