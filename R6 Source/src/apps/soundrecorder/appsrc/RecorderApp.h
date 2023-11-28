#ifndef RECORDER_APP_H
#define RECORDER_APP_H

#include <Application.h>

class RecorderWin;

class RecorderApp : public BApplication
{
	public:
		RecorderApp( void );
		virtual ~RecorderApp( void );
		
		virtual	void ReadyToRun( void );
		virtual	void RefsReceived( BMessage *msg );
	
	protected:
		RecorderWin		*recorderWindow;
};

#endif
