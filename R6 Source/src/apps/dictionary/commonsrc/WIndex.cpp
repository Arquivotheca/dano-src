#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "WIndex.h"

int32 cmp_i_entries( const WIndexEntry *e1, const WIndexEntry *e2 );

FileEntry::FileEntry( void )
{
	
}

FileEntry::FileEntry( const char *entryStr )
	: BString( entryStr )
{
	
}

FileEntry::~FileEntry( void )
{
	
}

WIndex::WIndex( int32 count )
{
	entryList = NULL;
	dataFile = NULL;
	ePerB = count;
}

WIndex::WIndex( BPositionIO *dataFile, int32 count )
{
	entryList = NULL;
	this->dataFile = dataFile;
	ePerB = count;
}

WIndex::~WIndex( void )
{
	if( entryList )
		free( entryList );
}

status_t WIndex::UnflattenIndex( BPositionIO *io )
{
	if( entryList )
		free( entryList );
	WIndexHead		head;
	
	io->Seek( 0, SEEK_SET );
	io->Read( &head, sizeof( head ) );
	io->Seek( head.offset, SEEK_SET );
	
	int32		size = head.entries * head.entrySize;
	
	if( !(entryList = (WIndexEntry *)malloc( size )) )
		return B_ERROR;
	entries = head.entries;
	maxEntries = ePerB;
	blockSize = ePerB * sizeof(WIndexEntry);
	blocks = entries/ePerB+1;;
	isSorted = true;
	
	io->Read( entryList, size );
	
	return B_OK;
}

status_t WIndex::FlattenIndex( BPositionIO *io )
{
	if( !isSorted )
		SortItems();
	WIndexHead		head;
	
	head.entries = entries;
	head.entrySize = sizeof( WIndexEntry );
	head.offset = sizeof( WIndexHead );
	io->Seek( 0, SEEK_SET );
	io->Write( &head, sizeof( head ) );
	io->Write( entryList, head.entries * head.entrySize );
	
	return B_OK;
}

int32 WIndex::Lookup( int32 key )
{
	if( !isSorted )
		SortItems();
	
	// Binary Search
	int32	M, Lb, Ub;
	Lb = 0;
	Ub = entries-1;
	while( true )
	{
		M = (Lb + Ub)/2;
		if( key < entryList[M].key )
			Ub = M - 1;
		else if(key > entryList[M].key )
			Lb = M + 1;
		else
			return M;
		if( Lb > Ub )
     		return -1;
	}
}

status_t WIndex::AddItem( WIndexEntry *entry )
{
	if( BlockCheck() == B_ERROR )
		return B_ERROR;
	entryList[entries] = *entry;
	entries++;
	isSorted = false;
	return B_OK;	
}

void WIndex::SortItems( void )
{
	qsort( entryList, entries, sizeof(WIndexEntry), (int (*)(const void *, const void *))cmp_i_entries );
	isSorted = true;
	//for( int32 i = 0; i < entries; i++ )
	//	printf( "Hash = %ld\n", entryList[i].key );
}

status_t WIndex::BlockCheck( void )
{
	if( entries < maxEntries )
		return B_OK;
	blocks = entries/ePerB+1;
	entryList = (WIndexEntry *)realloc( entryList, blockSize*blocks );
	if( !entryList )
		return B_ERROR;
	return B_OK;
}

status_t WIndex::InitIndex( void )
{
	if( entryList )
		free( entryList );
	isSorted = 0;
	entries = 0;
	maxEntries = ePerB;
	blockSize = ePerB * sizeof(WIndexEntry);
	blocks = 1;
	entryList = (WIndexEntry *)malloc( blockSize );
	if( !entryList )
		return B_ERROR;
	return B_OK;
}

int32 WIndex::HashWord( const char *s )
{
	int32	hash = 0;
	int32	x;
	int32	a = 84589;
	int32	b = 45989;
	int32	m = 217728;
	while( *s )
	{
		x = *s++ - 'a';
		
		hash ^= (a*x + b) % m;
		hash <<= 1;
	}
	if( hash < 0 ) // No negavite values!
		hash = ~hash;
	
	return hash;
}

int32 cmp_i_entries( const WIndexEntry *e1, const WIndexEntry *e2 )
{
	return e1->key - e2->key;
}

status_t WIndex::SetTo( BPositionIO *dataFile )
{
	this->dataFile = dataFile;
	return B_OK;
}

void WIndex::Unset( void )
{
	dataFile = NULL;
}

int32 WIndex::FindFirst( const char *word )
{
	int32			index;
	char			nword[256];
	int32			key;
	
	NormalizeWord( word, nword );
	key = HashWord( nword );
	if( (index = Lookup( key )) < 0 )
		return -1;
	// Find first instance of key
	while( (ItemAt( index-1 ))->key == key )
		index--;
	return index;
}

FileEntry *WIndex::GetEntry( int32 index )
{
	if( (index >= entries)||(index < 0) )
		return NULL;
	WIndexEntry		*ientry;
	FileEntry		*dentry;
	char			*buffer;
	
	dentry = new FileEntry();
	
	ientry = ItemAt( index );
	
	dataFile->Seek( ientry->offset, SEEK_SET );
	buffer = dentry->LockBuffer( ientry->size );
	dataFile->Read( buffer, ientry->size );
	dentry->UnlockBuffer( ientry->size );
	
	return dentry;
}

FileEntry *WIndex::GetEntry( const char *word )
{
	return GetEntry( FindFirst( word ) );
}

char *WIndex::NormalizeWord( const char *word, char *dest )
{
	const char 	*src;
	char		*dst;
	
	// remove dots and copy
	src = word;
	dst = dest;
	while( *src )
	{
		if( *src != '.' )
			*dst++ = *src;
		src++;
	}
	*dst = 0;
	
	// convert to lower-case
	dst = dest;
	while( *dst )
		*dst++ = tolower( *dst );
	return dest;
}