//========================================================================
//	MLinkObj.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include <string.h>

#include <Message.h>

#include "MLinkObj.h"
#include "MProjectView.h"
#include "MMessageWindow.h"
#include "IDEMessages.h"
#include "ErrorMessage.h"

// ---------------------------------------------------------------------------
//		MLinkObj
// ---------------------------------------------------------------------------

MLinkObj::MLinkObj(
	const char * 	compiler,
	BList&			args,
	bool 			IDEAware,
	MProjectView& 	inProjectView) :
	MCompileTool(
		compiler,
		args,
		IDEAware),
	fProjectView(inProjectView)
{
}

// ---------------------------------------------------------------------------

status_t
MLinkObj::DoMessage(
	const ErrorNotificationMessage & message)
{
	// An error or warning message has been received from the linker.  
	// We package it in a BMessage and send it to the project's 
	// error message window.

	fProjectView.GetErrorMessageWindow()->PostErrorMessage(message);

	return B_NO_ERROR;
}

// ---------------------------------------------------------------------------

status_t
MLinkObj::DoMessage(const ErrorNotificationMessageShort& message)
{
	// A fabricated error from linker setup/teardown
	// Send it to the project's error message window
	
	fProjectView.GetErrorMessageWindow()->PostErrorMessage(message);

	return B_NO_ERROR;
}

// ---------------------------------------------------------------------------
//		DoStatus
// ---------------------------------------------------------------------------

void
MLinkObj::DoStatus(
	bool objProduced,
	status_t errCode)
{
	fProjectView.LinkDone();

	MCompileTool::DoStatus(objProduced, errCode);
}

// ---------------------------------------------------------------------------
//		ParseMessageText
// ---------------------------------------------------------------------------
//	PlugIn linkers didn't have any way of signaling errors/warnings
//	Ask the fProjectView (who knows about our linker) to parse the messages
//	(added by John Dance)

status_t
MLinkObj::ParseMessageText(
	const char*	inText)
{
	status_t result = B_OK;
	BList messageList;
	
	fProjectView.ParseLinkerMessageText(inText, messageList);
	
	if (messageList.CountItems() > 0) {
		bool hasErrors = false;
		BMessage bmsg(msgAddErrorsToMessageWindow);

		for (int32 i = 0; i < messageList.CountItems(); i++) {
			ErrorMessage* msg = (ErrorMessage*) messageList.ItemAt(i);
			ASSERT(msg != nil);

			if (msg != nil) {
				hasErrors = hasErrors | ! msg->isWarning;
				msg->filename[MAX_ERROR_PATH_LENGTH-1] = '\0';		// protect against bad strings from the add_ons
				msg->errorMessage[MAX_ERROR_TEXT_LENGTH-1] = '\0';
				bmsg.AddData(kPlugInErrorName, kErrorType, msg, sizeof(ErrorMessage));
				delete msg;
			}
		}
		
		fProjectView.GetErrorMessageWindow()->PostMessage(&bmsg);
		
		if (hasErrors) {
			result = B_ERROR;
		}
	}
	
	return result;	
}
