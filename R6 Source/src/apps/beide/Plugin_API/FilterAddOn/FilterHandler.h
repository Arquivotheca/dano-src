// ---------------------------------------------------------------------------
/*
	FilterHandler.h
	
	Copyright (c) 2001 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			18 May 2001

	Given an MTextAddOn - a FilterHandler can apply a filter to the text
	If there is a selection, the filter will be applied to that selection
	otherwise, the filter will be applied to the entire text.
	
	The resulting text is pasted into the document (if the filter succeeds)
	
	The style of use is as follows:
	
	1. Ask the user for a filter and then apply it:
		handler.QueryUser()
		handler.ApplyFilter()
		
	2. Specify your own filter and apply it:
		handler.SetCommand("sort -u");
		handler.ApplyFilter()


*/
// ---------------------------------------------------------------------------

#ifndef FILTERHANDLER_H
#define FILTERHANDLER_H

class MTextAddOn;
class MTextAddOnStorage;
class FilterWindow;

#include <String.h>
#include <OS.h>
#include <List.h>

class FilterHandler {
public:
					FilterHandler();
					~FilterHandler();

	status_t		GetData(MTextAddOnStorage* storage);
	status_t		StoreData(MTextAddOnStorage* storage);
	
	void			QueryUser();
	void			SetCommand(const BString& command);
	void			SetCommand(const char* command);
	void			RemoveCommand(const char* command);
	status_t		ApplyFilter(MTextAddOn* textProxy);

private:
	void			DoFilter(MTextAddOn* textProxy);
	
private:
	FilterWindow*		fWindow;
	BList				fCommandCache;
	BString*			fCurrentCommand;
};

#endif
