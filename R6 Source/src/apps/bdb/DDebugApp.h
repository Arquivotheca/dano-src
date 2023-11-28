/*	$Id: DDebugApp.h,v 1.3 1999/02/03 08:30:01 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 04/07/98 13:38:57
*/

#ifndef DDEBUGAPP_H
#define DDEBUGAPP_H

#include <Application.h>
#include <FilePanel.h>

class DDebugApp : public BApplication
{
  public:
	DDebugApp();
	virtual ~DDebugApp();

	virtual void ArgvReceived(int32 argc, char **argv);

	virtual void ReadyToRun();
	virtual void MessageReceived(BMessage *msg);
	virtual bool QuitRequested();
	
	virtual void RefsReceived(BMessage *a_message);
	
  private:
	BFilePanel *fPanel;
};

extern BFile gAppFile;

#endif
