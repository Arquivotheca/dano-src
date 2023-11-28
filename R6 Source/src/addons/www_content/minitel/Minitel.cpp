/* -*- Mode: C; tab-width: 4; -*- */
/******************************************************************************
 * Copyright (c) 1996 Netscape Communications. All rights reserved.
 ******************************************************************************/
/*
 * UnixShell.c
 *
 * Netscape Client Plugin API
 * - Function that need to be implemented by plugin developers
 *
 * This file defines a "Template" plugin that plugin developers can use
 * as the basis for a real plugin.  This shell just provides empty
 * implementations of all functions that the plugin can implement
 * that will be called by Netscape (the NPP_xxx methods defined in 
 * npapi.h). 
 *
 * dp Suresh <dp@netscape.com>
 *
 */

// your code here
#include <stdio.h>
#include "npapi.h"

#ifdef __BEOS__
#include "MinitelView.h"
#endif // __BEOS__



//========================================================================

class Minitel : public BView
{
	public:
							Minitel				(BRect);
};


/***********************************************************************
 * Instance state information about the plugin.
 *
 * PLUGIN DEVELOPERS:
 *	Use this struct to hold per-instance information that you'll
 *	need in the various functions in this file.
 ***********************************************************************/

typedef struct _PluginInstance
{
	NPWindow* npWindow;
#ifdef __BEOS__
	BView*    container;
	BView*    view;
#endif // __BEOS__
} PluginInstance;

/***********************************************************************
 *
 * Empty implementations of plugin API functions
 *
 * PLUGIN DEVELOPERS:
 *	You will need to implement these functions as required by your
 *	plugin.
 *
 ***********************************************************************/

NPError
NPP_Initialize(void)
{
    return NPERR_NO_ERROR;
}


jref
NPP_GetJavaClass()
{
    return NULL;
}

void
NPP_Shutdown(void)
{
}


NPError 
NPP_New(NPMIMEType pluginType,
	NPP instance,
	uint16 mode,
	int16 argc,
	char* argn[],
	char* argv[],
	NPSavedData* saved)
{
    PluginInstance* This;

	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;
		
	instance->pdata = NPN_MemAlloc(sizeof(PluginInstance));
	
	This = (PluginInstance*) instance->pdata;

	if (This == NULL)
		return NPERR_OUT_OF_MEMORY_ERROR;

	This->npWindow  = NULL;
	
#ifdef __BEOS__
	This->view = NULL;
	This->container = NULL;
#endif // __BEOS__	

	return NPERR_NO_ERROR;

}


NPError 
NPP_Destroy(NPP instance, NPSavedData** save)
{
	PluginInstance* This;

	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	This = (PluginInstance*) instance->pdata;

#ifdef __BEOS__

	if ((This != NULL) && (This->view != NULL)) {

		if (This->container->LockLooper()) {
			This->container->RemoveChild(This->view);
			delete This->view;
			This->view = NULL;
			This->container->UnlockLooper();
		}
	}
		
#endif // __BEOS__

	/* PLUGIN DEVELOPERS:
	 *	If desired, call NP_MemAlloc to create a
	 *	NPSavedDate structure containing any state information
	 *	that you want restored if this plugin instance is later
	 *	recreated.
	 */

	if (This != NULL) {
		NPN_MemFree(instance->pdata);
		instance->pdata = NULL;
	}

	return NPERR_NO_ERROR;
}

NPError 
NPP_SetWindow(NPP instance, NPWindow* window)
{

	PluginInstance* This;

	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	if (window == NULL)
		return NPERR_NO_ERROR;

	This = (PluginInstance*) instance->pdata;

	/*
	 * PLUGIN DEVELOPERS:
	 *	Before setting window to point to the
	 *	new window, you may wish to compare the new window
	 *	info to the previous window (if any) to note window
	 *	size changes, etc.
	 */

#ifdef __BEOS__

		/* 
		 * Simple Example
		 *
		 */

		BView* parent = (BView*) window->window;

		if (This->container != parent) {
  			This->container = parent;
			if (parent->LockLooper()) {
				parent->AddChild(This->view = new Minitel(parent->Bounds()));
				parent->UnlockLooper();
			}
		}

#endif // __BEOS__

	return NPERR_NO_ERROR;
}

NPError 
NPP_NewStream(NPP instance,
			  NPMIMEType type,
			  NPStream *stream, 
			  NPBool seekable,
			  uint16 *stype)
{
//	NPByteRange range;
	PluginInstance* This;

	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	This = (PluginInstance*) instance->pdata;

	return NPERR_NO_ERROR;
}


/* PLUGIN DEVELOPERS:
 *	These next 2 functions are directly relevant in a plug-in which
 *	handles the data in a streaming manner. If you want zero bytes
 *	because no buffer space is YET available, return 0. As long as
 *	the stream has not been written to the plugin, Navigator will
 *	continue trying to send bytes.  If the plugin doesn't want them,
 *	just return some large number from NPP_WriteReady(), and
 *	ignore them in NPP_Write().  For a NP_ASFILE stream, they are
 *	still called but can safely be ignored using this strategy.
 */

int32 STREAMBUFSIZE = 0X0FFFFFFF; /* If we are reading from a file in NPAsFile
				   * mode so we can take any size stream in our
				   * write call (since we ignore it) */

int32 
NPP_WriteReady(NPP instance, NPStream *stream)
{
	PluginInstance* This;
	if (instance != NULL)
		This = (PluginInstance*) instance->pdata;

	return STREAMBUFSIZE;
}


int32 
NPP_Write(NPP instance, NPStream *stream, int32 offset, int32 len, void *buffer)
{
	if (instance != NULL)
	{
//		PluginInstance* This = (PluginInstance*) instance->pdata;
	}

	return len;		/* The number of bytes accepted */
}


NPError 
NPP_DestroyStream(NPP instance, NPStream *stream, NPError reason)
{
	PluginInstance* This;

	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;
	This = (PluginInstance*) instance->pdata;

	return NPERR_NO_ERROR;
}


void 
NPP_StreamAsFile(NPP instance, NPStream *stream, const char* fname)
{
	PluginInstance* This;
	if (instance != NULL)
		This = (PluginInstance*) instance->pdata;
}


void 
NPP_Print(NPP instance, NPPrint* printInfo)
{
	if(printInfo == NULL)
		return;

	if (instance != NULL) {
//		PluginInstance* This = (PluginInstance*) instance->pdata;
	
		if (printInfo->mode == NP_FULL) {
		    /*
		     * PLUGIN DEVELOPERS:
		     *	If your plugin would like to take over
		     *	printing completely when it is in full-screen mode,
		     *	set printInfo->pluginPrinted to TRUE and print your
		     *	plugin as you see fit.  If your plugin wants Netscape
		     *	to handle printing in this case, set
		     *	printInfo->pluginPrinted to FALSE (the default) and
		     *	do nothing.  If you do want to handle printing
		     *	yourself, printOne is true if the print button
		     *	(as opposed to the print menu) was clicked.
		     *	On the Macintosh, platformPrint is a THPrint; on
		     *	Windows, platformPrint is a structure
		     *	(defined in npapi.h) containing the printer name, port,
		     *	etc.
		     */

//			void* platformPrint =
//				printInfo->print.fullPrint.platformPrint;
//			NPBool printOne =
//				printInfo->print.fullPrint.printOne;
			
			/* Do the default*/
			printInfo->print.fullPrint.pluginPrinted = FALSE;
		}
		else {	/* If not fullscreen, we must be embedded */
		    /*
		     * PLUGIN DEVELOPERS:
		     *	If your plugin is embedded, or is full-screen
		     *	but you returned false in pluginPrinted above, NPP_Print
		     *	will be called with mode == NP_EMBED.  The NPWindow
		     *	in the printInfo gives the location and dimensions of
		     *	the embedded plugin on the printed page.  On the
		     *	Macintosh, platformPrint is the printer port; on
		     *	Windows, platformPrint is the handle to the printing
		     *	device context.
		     */

//			NPWindow* printWindow =
//				&(printInfo->print.embedPrint.window);
//			void* platformPrint =
//				printInfo->print.embedPrint.platformPrint;
		}
	}
}


//========================================================================

Minitel::Minitel(BRect rect)
	: BView		(rect,
				 "Minitel",
				 B_FOLLOW_NONE,
				 B_WILL_DRAW)
{
	BRect			r = rect;
	MinitelView*	view;

	SetViewColor(0, 0, 0);	// To hide where views to always butt up to each other.
	r.OffsetTo(0, 0);
#ifdef USE_KEYBOARD_PANEL
	r.bottom -= kKEYBOARD_BAR_HEIGHT;
#endif
	AddChild(view = new MinitelView(r));
#ifdef USE_KEYBOARD_PANEL
	r.top = r.bottom + 1;
	r.bottom = r.top + kKEYBOARD_BAR_HEIGHT;
	r.left--;
	r.right++;
	AddChild(new KeyboardPanel(r, view));
#endif
}
