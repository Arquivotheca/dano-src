#ifndef WORDS_H
#define WORDS_H

#include <String.h>
#include "WIndex.h"

class Words : public WIndex
{
	public:
		Words( void );
		Words( BPositionIO *thes );
		virtual ~Words( void );
		
		virtual status_t BuildIndex( void );
		virtual int32 HashWord( const char *s );
};


#endif