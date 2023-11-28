#ifndef THESAURUS_H
#define THESAURUS_H

#include <String.h>
#include "WIndex.h"

class Thesaurus : public WIndex
{
	public:
		Thesaurus( void );
		Thesaurus( BPositionIO *thes );
		virtual ~Thesaurus( void );
		
		virtual status_t BuildIndex( void );
};


#endif