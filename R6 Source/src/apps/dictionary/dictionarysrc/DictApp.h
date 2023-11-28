#ifndef DICTAPP_H
#define DICTAPP_H

#include <Application.h>

class DictApp : public BApplication
{
	public:
		DictApp( void );
		virtual ~DictApp( void );
		
		virtual void MessageReceived( BMessage *msg );
		virtual void ReadyToRun( void );
	
	protected:
		void FileAlert( void );
};

#endif