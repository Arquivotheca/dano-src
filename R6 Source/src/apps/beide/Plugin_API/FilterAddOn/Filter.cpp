// ---------------------------------------------------------------------------
/*
	Filter.cpp
	
	Copyright (c) 2001 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			17 May 2001

	Create a FilterHandler and allow it to do all the work

*/
// ---------------------------------------------------------------------------

#include "FilterHandler.h"
#include "MTextAddOn.h"

#include <Alert.h>

class MTextAddOn;

extern "C" {
	status_t perform_edit(MTextAddOn* addon);
	status_t edit_addon_startup(MTextAddOnStorage* storage);
	status_t edit_addon_shutdown(MTextAddOnStorage* storage);
}

FilterHandler* gFilterHandler = NULL;

// ---------------------------------------------------------------------------

status_t
perform_edit(MTextAddOn* addon)
{
	// perform_edit is the function called when the menu is selected
	// Check if we can do the filter, otherwise just return
	if (addon->IsEditable() == false) {
		BAlert* alert = new BAlert("", "File is not ediable. Filters can only be applied to editable files.", "OK");
		alert->Go();
		return B_ERROR;
	}
	
	gFilterHandler->QueryUser();
	return gFilterHandler->ApplyFilter(addon);
}

// ---------------------------------------------------------------------------

status_t 
edit_addon_startup(MTextAddOnStorage* storage)
{	
	if (gFilterHandler == NULL) {
		gFilterHandler = new FilterHandler();
	}

	return gFilterHandler->GetData(storage);
}

// ---------------------------------------------------------------------------

status_t 
edit_addon_shutdown(MTextAddOnStorage* storage)
{
	status_t status = B_OK;
	
	if (gFilterHandler) {
		status = gFilterHandler->StoreData(storage);
		delete gFilterHandler;
		gFilterHandler = NULL;	
	}
	
	return status;
}
