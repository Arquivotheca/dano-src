#ifndef WORD_INDEX_H
#define WORD_INDEX_H

#include <DataIO.h>
#include <String.h>

struct WIndexHead
{
	int32 entries;
	int32 entrySize;
	int32 offset;	
};

struct WIndexEntry
{
	int32 key;
	int32 offset;
	int32 size;
};

class FileEntry : public BString
{
	public:
		FileEntry( void );
		FileEntry( const char *entryStr );
		virtual ~FileEntry( void );
};

class WIndex
{
	public:
		WIndex( BPositionIO *dataFile, int32 count = 100 );
		WIndex( int32 count = 100 );
		virtual ~WIndex( void );
		
		status_t InitIndex( void );
		status_t UnflattenIndex( BPositionIO *io );
		status_t FlattenIndex( BPositionIO *io );
		
		int32 Lookup( int32 key );
		
		inline WIndexEntry *ItemAt( int32 index ) { return entryList + index; };
		status_t AddItem( WIndexEntry *entry );
		inline int32 CountItems( void ) { return entries; };
		void SortItems( void );
		
		virtual int32 HashWord( const char *s );
		virtual char *NormalizeWord( const char *word, char *dest );
		
		status_t SetTo( BPositionIO *dataFile );
		void Unset( void );
		
		virtual status_t BuildIndex( void ) = 0;
		
		virtual int32 FindFirst( const char *word );
		virtual FileEntry *GetEntry( int32 index );
		FileEntry *GetEntry( const char *word );
		
	protected:
		status_t BlockCheck( void );
		
	protected:
		int32		entries;
		int32		maxEntries;
		int32		ePerB;
		int32		blockSize;
		int32		blocks;
		bool		isSorted;
		WIndexEntry	*entryList;
		BPositionIO	*dataFile;
};


#endif