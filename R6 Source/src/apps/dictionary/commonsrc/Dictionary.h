#ifndef DICTIONARY_H
#define DICTIONARY_H

#include "WIndex.h"
#include <String.h>

class Dictionary : public WIndex
{
	public:
		Dictionary( void );
		Dictionary( BPositionIO *dict );
		virtual ~Dictionary( void );
		
		virtual status_t BuildIndex( void );
};

#endif