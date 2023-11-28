#ifndef WORDS_H
#define WORDS_H

#include <String.h>
#include "WIndex.h"

typedef enum {COMPARE, GENERATE} metaphlag; 

class Words;

bool metaphone(const char *Word, char *Metaph, metaphlag Flag);
int word_match( const char *reference, const char *test );
int32 suffix_word( char *dst, const char *src, char flag );
void sort_word_list( BList *matches, const char *reference );

class Words : public WIndex
{
	public:
		Words( bool useMetaphone = true );
		Words( BPositionIO *thes, bool useMetaphone = true );
		Words( const char *dataPath, const char *indexPath, bool useMetaphone );
		virtual ~Words( void );
		
		virtual status_t BuildIndex( void );
		virtual int32 GetKey( const char *s );
		
		int32 FindBestMatches( BList *matches, const char *word );
	
	protected:
		bool fUseMetaphone;
};

#endif
