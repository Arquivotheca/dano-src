#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "Words.h"

/* 
**  File METAPHON.C 
*/ 

/* 
**  MAXMETAPH is the length of the Metaphone code. 
** 
**  Four is a good compromise value for English names. For comparing words 
**  which are not names or for some non-English names, use a longer code 
**  length for more precise matches. 
** 
**  The default here is 5. 
*/ 

#define MAXMETAPH 6 

typedef enum {COMPARE, GENERATE} metaphlag; 

bool metaphone(const char *Word, char *Metaph, metaphlag Flag);

Words::Words( void )
{
	
}

Words::Words( BPositionIO *thes )
	: WIndex( thes )
{
	
}

Words::~Words( void )
{
	
}

		
// Parse the Thesaurus file...
status_t Words::BuildIndex( void )
{
	// Buffer Stuff
	char			buffer[16384];
	char			*nptr, *eptr;
	int64			blockOffset;
	int32			blockSize;
	
	// The Word Entry
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
				*namePtr = 0; // terminate word
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
				if( *nptr == '/' )
				{
					*namePtr = 0; // terminate word
					//printf( "Found word: %s\n", entryName );
					
					// Reset state
					//namePtr = entryName;
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

/*
**  Character coding array
*/

static char vsvfn[26] = {
      1,16,4,16,9,2,4,16,9,2,0,2,2,2,1,4,0,2,4,4,1,0,0,0,8,0};
/*    A  B C  D E F G  H I J K L M N O P Q R S T U V W X Y Z      */

int32 Words::HashWord( const char *s )
{
	char		Metaph[12];
	const char	*sPtr;
	int32		hash = 0;
	int32		offset;
	char		c;
	
	metaphone( s, Metaph, GENERATE );
	// Compact Metaphone from 6 bytes to 4
	
	// printf( "%s -> %s: ", s, Metaph );
	
	for( sPtr = Metaph, offset = 25; *sPtr; sPtr++, offset -= 5 )
	{
		c = *sPtr - 'A';
		// printf( "%d,", int16(c) );
		hash |= int32(c) << offset;
	}
	for( ; offset >= 0; offset -= 5 )
		hash |= int32(31) << offset;
	// printf( ": %ld\n", hash );
	return hash;
}

/*
**  Macros to access the character coding array
*/

#define vowel(x)  (vsvfn[(x) - 'A'] & 1)  /* AEIOU    */
#define same(x)   (vsvfn[(x) - 'A'] & 2)  /* FJLMNR   */
#define varson(x) (vsvfn[(x) - 'A'] & 4)  /* CGPST    */
#define frontv(x) (vsvfn[(x) - 'A'] & 8)  /* EIY      */
#define noghf(x)  (vsvfn[(x) - 'A'] & 16) /* BDH      */
#define NUL '\0'
/*
**  metaphone()
**
**  Arguments: 1 - The word to be converted to a metaphone code.
**             2 - A MAXMETAPH+1 char field for the result.
**             3 - Function flag:
**                 If 0: Compute the Metaphone code for the first argument,
**                       then compare it to the Metaphone code passed in
**                       the second argument.
**                 If 1: Compute the Metaphone code for the first argument,
**                       then store the result in the area pointed to by the
**                       second argument.
**
**  Returns: If function code is 0, returns Success_ for a match, else Error_.
**           If function code is 1, returns Success_.
*/

bool metaphone(const char *Word, char *Metaph, metaphlag Flag)
{
      char *n, *n_start, *n_end;    /* Pointers to string               */
      char *metaph = NULL, *metaph_end;    /* Pointers to metaph               */
      char ntrans[512];             /* Word with uppercase letters      */
      char newm[MAXMETAPH + 4];     /* New metaph for comparison        */
      int KSflag;                   /* State flag for X translation     */

      /*
      ** Copy word to internal buffer, dropping non-alphabetic characters
      ** and converting to upper case.
      */

      for (n = ntrans + 1, n_end = ntrans + sizeof(ntrans) - 2;
            *Word && n < n_end; ++Word)
      {
            if (isalpha(*Word))
                  *n++ = toupper(*Word);
      }

      if (n == ntrans + 1)
            return false;           /* Return if zero characters        */
      else  n_end = n;              /* Set end of string pointer        */

      /*
      ** Pad with NULs, front and rear
      */

      *n++ = NUL;
      *n   = NUL;
      n    = ntrans;
      *n++ = NUL;

      /*
      ** If doing comparison, redirect pointers
      */

      if (COMPARE == Flag)
      {
            metaph = Metaph;
            Metaph = newm;
      }

      /*
      ** Check for PN, KN, GN, WR, WH, and X at start
      */

      switch (*n)
      {
      case 'P':
      case 'K':
      case 'G':
            if ('N' == *(n + 1))
                  *n++ = NUL;
            break;

      case 'A':
            if ('E' == *(n + 1))
                  *n++ = NUL;
            break;

      case 'W':
            if ('R' == *(n + 1))
                  *n++ = NUL;
            else if ('H' == *(n + 1))
            {
                  *(n + 1) = *n;
                  *n++ = NUL;
            }
            break;

      case 'X':
            *n = 'S';
            break;
      }

      /*
      ** Now loop through the string, stopping at the end of the string
      ** or when the computed Metaphone code is MAXMETAPH characters long.
      */

      KSflag = false;              /* State flag for KStranslation     */
      for (metaph_end = Metaph + MAXMETAPH, n_start = n;
            n <= n_end && Metaph < metaph_end; ++n)
      {
            if (KSflag)
            {
                  KSflag = false;
                  *Metaph++ = *n;
            }
            else
            {
                  /* Drop duplicates except for CC    */

                  if (*(n - 1) == *n && *n != 'C')
                        continue;

                  /* Check for F J L M N R  or first letter vowel */

                  if (same(*n) || (n == n_start && vowel(*n)))
                        *Metaph++ = *n;
                  else switch (*n)
                  {
                  case 'B':
                        if (n < n_end || *(n - 1) != 'M')
                              *Metaph++ = *n;
                        break;

                  case 'C':
                        if (*(n - 1) != 'S' || !frontv(*(n + 1)))
                        {
                              if ('I' == *(n + 1) && 'A' == *(n + 2))
                                    *Metaph++ = 'X';
                              else if (frontv(*(n + 1)))
                                    *Metaph++ = 'S';
                              else if ('H' == *(n + 1))
                                    *Metaph++ = ((n == n_start &&
                                          !vowel(*(n + 2))) ||
                                          'S' == *(n - 1)) ? 'K' : 'X';
                              else  *Metaph++ = 'K';
                        }
                        break;

                  case 'D':
                        *Metaph++ = ('G' == *(n + 1) && frontv(*(n + 2))) ?
                              'J' : 'T';
                        break;

                  case 'G':
                        if ((*(n + 1) != 'H' || vowel(*(n + 2))) &&
                              (*(n + 1) != 'N' || ((n + 1) < n_end &&
                              (*(n + 2) != 'E' || *(n + 3) != 'D'))) &&
                              (*(n - 1) != 'D' || !frontv(*(n + 1))))
                        {
                              *Metaph++ = (frontv(*(n + 1)) &&
                                    *(n + 2) != 'G') ? 'J' : 'K';
                        }
                        else if ('H' == *(n + 1) && !noghf(*(n - 3)) &&
                              *(n - 4) != 'H')
                        {
                              *Metaph++ = 'F';
                        }
                        break;

                  case 'H':
                        if (!varson(*(n - 1)) && (!vowel(*(n - 1)) ||
                              vowel(*(n + 1))))
                        {
                              *Metaph++ = 'H';
                        }
                        break;

                  case 'K':
                        if (*(n - 1) != 'C')
                              *Metaph++ = 'K';
                        break;

                  case 'P':
                        *Metaph++ = ('H' == *(n + 1)) ? 'F' : 'P';
                        break;

                  case 'Q':
                        *Metaph++ = 'K';
                        break;

                  case 'S':
                        *Metaph++ = ('H' == *(n + 1) || ('I' == *(n + 1) &&
                              ('O' == *(n + 2) || 'A' == *(n + 2)))) ?
                              'X' : 'S';
                        break;

                  case 'T':
                        if ('I' == *(n + 1) && ('O' == *(n + 2) ||
                              'A' == *(n + 2)))
                        {
                              *Metaph++ = 'X';
                        }
                        else if ('H' == *(n + 1))
                              *Metaph++ = 'O';
                        else if (*(n + 1) != 'C' || *(n + 2) != 'H')
                              *Metaph++ = 'T';
                        break;

                  case 'V':
                        *Metaph++ = 'F';
                        break;

                  case 'W':
                  case 'Y':
                        if (vowel(*(n + 1)))
                              *Metaph++ = *n;
                        break;

                  case 'X':
                        if (n == n_start)
                              *Metaph++ = 'S';
                        else
                        {
                              *Metaph++ = 'K';
                              KSflag = true;
                        }
                        break;

                  case 'Z':
                        *Metaph++ = 'S';
                        break;
                  }
            }

            /*
            ** Compare new Metaphone code with old
            */

            if (COMPARE == Flag &&
                  *(Metaph - 1) != metaph[(Metaph - newm) - 1])
            {
                  return false;
            }
      }

      /*
      ** If comparing, check if Metaphone codes were equal in length
      */

      if (COMPARE == Flag && metaph[Metaph - newm])
            return false;

      *Metaph = NUL;
      return true;
}
