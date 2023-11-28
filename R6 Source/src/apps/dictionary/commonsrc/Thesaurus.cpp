#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "Thesaurus.h"

Thesaurus::Thesaurus( void )
{
	
}

Thesaurus::Thesaurus( BPositionIO *thes )
	: WIndex( thes )
{
	
}

Thesaurus::~Thesaurus( void )
{
	
}

		
// Parse the Thesaurus file...
status_t Thesaurus::BuildIndex( void )
{
	// Buffer Stuff
	char			buffer[16384];
	char			*nptr, *eptr;
	int64			blockOffset;
	int32			blockSize;
	
	// The Thesaurus Entry
	WIndexEntry		entry;
	char			entryName[256], *namePtr = entryName;
	
	// State Info
	bool			getWord = true; // reading word
	
	// Make sure we are at start of file
	dataFile->Seek( 0, SEEK_SET );
	entry.size = 0;
	
	// Read blocks from thes until eof
	while( true )
	{
		// Get next block
		blockOffset = dataFile->Position();
		if( (blockSize = dataFile->Read( buffer, 16384 )) == 0 )
			break;
		
		// parse block
		for( nptr = buffer, eptr = buffer + blockSize; nptr < eptr; nptr++, entry.size++ )
		{
			if( *nptr == '\n' )
			{
				// Add previous entry to word index
				NormalizeWord( entryName, entryName );
				entry.key = HashWord( entryName );
				AddItem( &entry );
				
				// Init new entry
				entry.offset = blockOffset + (nptr - buffer);
				entry.size = 0;
				getWord = true;
				namePtr = entryName;
			}
			else if( getWord ) // Are we looking for a word?
			{
				// end of word?
				if( *nptr == ':' )
				{
					*namePtr = 0; // terminate word
					//printf( "Found word: %s\n", entryName );
					
					// Reset state
					namePtr = entryName;
					getWord = false;
				}
				else
					*namePtr++ = *nptr; // copy word
			}
		} // End for( nptr = buffer, eptr = buffer + blockSize; nptr < eptr; nptr++, entry.size++ )
	} // End while( true )
	// Add last entry
	NormalizeWord( entryName, entryName );
	entry.key = HashWord( entryName );
	AddItem( &entry );
	
	SortItems();
	return B_OK;
}
