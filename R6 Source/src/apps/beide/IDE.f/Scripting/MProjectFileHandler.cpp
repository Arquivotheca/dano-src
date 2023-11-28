//========================================================================
//	MProjectFileHandler.cpp
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include "MProjectFileHandler.h"
#include "MSourceFileLine.h"
#include "MProjectView.h"
#include "MEntryRefHandler.h"
#include "MScripting.h"


// ---------------------------------------------------------------------------
//		MProjectFileHandler
// ---------------------------------------------------------------------------

MProjectFileHandler::MProjectFileHandler(
	MSourceFileLine*	inProjectLine,
	MProjectView&		inProjectView,
	int32				inIndex)
	: LazyScriptHandler(-1, "filehandler"),
	fIndex(inIndex),
	fSourceFileLine(inProjectLine),
	fProjectView(inProjectView)
{
}

// ---------------------------------------------------------------------------
//		~MProjectFileHandler
// ---------------------------------------------------------------------------

MProjectFileHandler::~MProjectFileHandler()
{
}

// ---------------------------------------------------------------------------
//		GetSubHandler
// ---------------------------------------------------------------------------

ScriptHandler *
MProjectFileHandler::GetSubHandler(
	const char *			propertyName,
	EForm					form,
	const SData &			data)
{
	int propCount = -1;

	if (!strcmp(propertyName, "property") && (form == formIndex)) {
		propCount = data.index;
	}

	if (0 == strcmp(propertyName, kDependenciesProp) && (form == formDirect)) 
	{
		BList*				list = new BList(50);
		
		if (fSourceFileLine != nil)
		{
			fSourceFileLine->FillDependencyList(*list);
	
			return new MEntryRefHandler("", list);
		}
		else
			return Reference();
	}
	else
	if (propCount > 0) 
	{
		SData newData;
		newData.index = propCount;
	
		return LazyScriptHandler::GetSubHandler(propertyName, formIndex, newData);
	}
	else
		return LazyScriptHandler::GetSubHandler(propertyName, form, data);
}

// ---------------------------------------------------------------------------
//		PerformScriptAction
// ---------------------------------------------------------------------------
//	Perform actions from scripts or remote apps.

status_t
MProjectFileHandler::PerformScriptAction(
	BMessage *			message,
	BMessage * &		reply,
	bool&				wasDeferred)
{
	status_t		result;

	switch (message->what) 
	{
		// These are handled by the project view
		case kCreateVerb:		// Add file
			result = fProjectView.AddFilesFromScript(message, reply, fIndex);
			break;

		case kDeleteVerb:		// Remove file
			result = fProjectView.RemoveFileFromScript(message, reply, fSourceFileLine);
			break;

		case kGetVerb:			// Get dependencies
			result = fProjectView.SourceFileLineFromScript(message, fSourceFileLine);
			if (result == B_NO_ERROR)
			{
				BList*				list = new BList(50);
			
				fSourceFileLine->FillDependencyList(*list);
				MEntryRefHandler*		temp = new MEntryRefHandler("", list);
				temp->PerformScriptAction(message, reply, wasDeferred);
				temp->Done();
			}
			break;

		default:
			result = SCRIPT_BAD_VERB;
			break;
	}

	return result;
}

