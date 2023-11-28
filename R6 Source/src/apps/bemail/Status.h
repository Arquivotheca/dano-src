//--------------------------------------------------------------------
//	
//	Status.h
//
//	Written by: Robert Polic
//	
//	Copyright 1997 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef STATUS_H
#define STATUS_H

#include <Beep.h>
#include <Box.h>
#include <Button.h>
#include <fs_index.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Query.h>
#include <TextControl.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <Window.h>

#define	STATUS_WIDTH		220
#define STATUS_HEIGHT		80

#define STATUS_TEXT			"Status:"
#define STATUS_FIELD_H		 10
#define STATUS_FIELD_V		  8
#define STATUS_FIELD_WIDTH	(STATUS_WIDTH - STATUS_FIELD_H)
#define STATUS_FIELD_HEIGHT	 16

#define BUTTON_WIDTH		 70
#define BUTTON_HEIGHT		 20

#define S_OK_BUTTON_X1		(STATUS_WIDTH - BUTTON_WIDTH - 6)
#define S_OK_BUTTON_Y1		(STATUS_HEIGHT - (BUTTON_HEIGHT + 10))
#define S_OK_BUTTON_X2		(S_OK_BUTTON_X1 + BUTTON_WIDTH)
#define S_OK_BUTTON_Y2		(S_OK_BUTTON_Y1 + BUTTON_HEIGHT)
#define S_OK_BUTTON_TEXT		"OK"

#define S_CANCEL_BUTTON_X1	(S_OK_BUTTON_X1 - (BUTTON_WIDTH + 10))
#define S_CANCEL_BUTTON_Y1	S_OK_BUTTON_Y1
#define S_CANCEL_BUTTON_X2	(S_CANCEL_BUTTON_X1 + BUTTON_WIDTH)
#define S_CANCEL_BUTTON_Y2	S_OK_BUTTON_Y2
#define S_CANCEL_BUTTON_TEXT	"Cancel"

#define INDEX_STATUS		"_status"

enum	status_messages		{STATUS = 128, OK, CANCEL};

class	TStatusView;


//====================================================================

class TStatusWindow : public BWindow {
public:

				TStatusWindow(BRect, BWindow*, char*);

private:

	TStatusView		*fView;

};

//--------------------------------------------------------------------

class TStatusView : public BBox {
public:

					TStatusView(BRect, BWindow*, char*); 
	virtual	void	AttachedToWindow();
	virtual void	MessageReceived(BMessage*);
	bool			Exists(const char*);

private:

	char			*fString;
	BTextControl	*fStatus;
	BWindow			*fWindow;
};
#endif
