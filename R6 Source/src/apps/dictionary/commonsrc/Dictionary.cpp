#include <ctype.h>
#include <stdio.h>
#include "Dictionary.h"

Dictionary::Dictionary( void )
{
	
}

Dictionary::Dictionary( BPositionIO *dict )
	: WIndex( dict )
{
	
}

Dictionary::~Dictionary( void )
{
	
}
		
// Parse the Evil dictionary file!
status_t Dictionary::BuildIndex( void )
{
	// Buffer Stuff
	char			buffer[16384];
	char			*nptr, *eptr;
	int64			blockOffset;
	int32			blockSize;
	BString			*s;
	BString			sarray[8];
	
	
	// The Dictionary Entry
	WIndexEntry		entry;
	char			entryName[256], *namePtr = entryName;
	
	// State Info
	bool			foundEntry = false; // Has an entry been found
	bool			getWord = true; // reading word
	bool			foundWord = false; // Found start of word
	char			prevc = 0, prevc2 = 0; // previous characters read
	int32			scount = 0;
	
	// Make sure we are at start of file
	dataFile->Seek( 0, SEEK_SET );
	entry.size = 0;
	
	// Read blocks from dict until eof
	while( true )
	{
		// Get next block
		blockOffset = dataFile->Position();
		if( (blockSize = dataFile->Read( buffer, 16384 )) == 0 )
			break;
		
		// parse block
		for( nptr = buffer, eptr = buffer + blockSize; nptr < eptr; prevc2 = prevc, prevc = *nptr++, entry.size++ )
		{
			if( *nptr == '@' )
			{
				// Add previous entry to word index
				if( foundEntry )
				{
					// Add each word in entry as seperate item
					for( int32 i=0; i < scount; i++ )
					{
						s = &sarray[i];
						NormalizeWord( s->String(), entryName );
						entry.key = HashWord( entryName );
						AddItem( &entry );
					}
					scount = 0;
				}
				
				// Init new entry
				entry.offset = blockOffset + (nptr - buffer);
				entry.size = 0;
				foundEntry = true;
				foundWord = false;
				getWord = true;
				namePtr = entryName;
			}
			else if( foundEntry ) // Has a dictionary entry been found?
			{
				if( getWord ) // Are we looking for a word?
				{
					if( foundWord ) // Have we found a word?
					{
						// False Start?
						if( *nptr == ' ' && prevc2 == '@'  )
							namePtr = entryName;
						// end of word?
						else if( *nptr == ' ' || *nptr == ',' )
						{
							*namePtr = 0; // terminate word
							// Filter out garbage
							if( !(scount > 0 && 
							((strcmp( "used", entryName ) == 0) || 
							(strcmp( "a", entryName ) == 0) || 
							(strcmp( "often", entryName ) == 0) || 
							(strcmp( "see", entryName ) == 0) || 
							(strcmp( ",", entryName ) == 0) || 
							(strcmp( "usu", entryName ) == 0))) )
							{
								// Add word to word list
								sarray[scount++].SetTo( entryName );
								if( scount >= 8 )
									scount = 7;
							}
							
							// Reset state
							namePtr = entryName;
							getWord = false;
							foundWord = false;
						}
						else
							*namePtr++ = *nptr; // copy word
					}
					else if( !isdigit( *nptr ) && !isspace( *nptr ) ) // Find start of word
					{
						*namePtr++ = *nptr; // copy fist character
						foundWord = true;
					}
				}
				else if( *nptr == '-' && prevc == '-' && prevc2 == ' ' ) // have we found another word?
					getWord = true;
			}
		} // End for( nptr = buffer, eptr = buffer + blockSize; nptr < eptr; nptr++, entry.size++ )
	} // End while( true )
	// Add last entry
	if( foundEntry )
	{
		for( int32 i=0; i < scount; i++ )
		{
			s = &sarray[i];
			NormalizeWord( s->String(), entryName );
			entry.key = HashWord( entryName );
			AddItem( &entry );
			
		}
		scount = 0;
	}
	
	SortItems();
	return B_OK;
}

