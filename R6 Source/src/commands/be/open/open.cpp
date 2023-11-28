//--------------------------------------------------------------------
//	
//	open - opens a browser window from a shell
//
//	Written by: Scott Bronson (from listres.cpp by Robert Polic)
//	
//	Copyright 1995 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef _FILE_H
#include <File.h>
#endif
#ifndef _MESSAGE_H
#include <Message.h>
#endif
#ifndef _RESOURCE_FILE_H
#include <ResourceFile.h>
#endif

class TOpenApp : public BApplication {

public:
					TOpenApp();
virtual		void	ArgvReceived(int argc, char** argv);
virtual		void	RefsReceived(BMessage *theMessage);
virtual		void	ReadyToRun();

			void	HandleRef(record_ref);

			BMessenger fMessenger;
};

bool	have_a_file = FALSE;

//====================================================================

int main()
{	
	BApplication* myApp = new TOpenApp();
	myApp->Run();

	delete myApp;
	return B_NO_ERROR;
}

//--------------------------------------------------------------------

TOpenApp::TOpenApp()
	:	BApplication("application/x-vnd.Be-cmd-OPEN"),
		fMessenger( 'SHRK' )
{
	// make sure the channel to the DR8 browser was opened.
	long err = fMessenger.Error();
	if( err != B_NO_ERROR ) {
		printf( "open: error %ld, perhaps the DR8 Browser isn't running?\n",
			err );
	}
}

//--------------------------------------------------------------------

void TOpenApp::ArgvReceived(int argc, char** argv)
{
	int			loop;
	record_ref	ref;

	for (loop = 1; loop < argc; loop++)
		if (get_ref_for_path(argv[loop], &ref) == B_NO_ERROR)
			HandleRef(ref);
		else
			printf("%s: cannot open %s\n", argv[0], argv[loop]);
}

//--------------------------------------------------------------------

void TOpenApp::RefsReceived(BMessage *theMessage)
{
	long	count;
	long	loop;
	ulong	type;

	if (theMessage->GetInfo("refs", &type, &count))
		for (loop = 0; loop < count; loop++)
			HandleRef(theMessage->FindRef("refs", loop));
}

//--------------------------------------------------------------------

void TOpenApp::HandleRef(record_ref ref)
{
	long err;

	have_a_file = TRUE;

	if( fMessenger.Error() == B_NO_ERROR ) {
		BMessage *msg = new BMessage( B_REFS_RECEIVED );

		msg->AddRef( "refs", ref );
		if( (err=msg->Error()) == B_NO_ERROR ) {
			fMessenger.SendMessage( msg );
			// if SendMessage() fails, we don't behave very intelligently.
		} else {
			printf( "open: AddRef error %ld.\n", err );
		}
	}
}

//--------------------------------------------------------------------

void TOpenApp::ReadyToRun()
{
	if (!(have_a_file))
		printf("usage: open 'filename'\n");
	be_app->PostMessage(B_QUIT_REQUESTED);
}
