#include "RecorderApp.h"
#include "RecorderWin.h"

RecorderApp::RecorderApp( void )
	: BApplication( "application/x-vnd.Be-SoundRecorder" )
{
	recorderWindow = new RecorderWin( BPoint( 100, 100 ) );
}

RecorderApp::~RecorderApp( void )
{
	
}

void RecorderApp::ReadyToRun( void )
{
	// Do setup
	
}

void RecorderApp::RefsReceived( BMessage *msg )
{
	recorderWindow->PostMessage( msg );
}
