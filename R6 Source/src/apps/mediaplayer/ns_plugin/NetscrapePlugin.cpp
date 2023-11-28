#include <Window.h>
#include <stdio.h>
#include "npapi.h"
#include "PluginView.h"

class NetscapePluginView : public PluginView {
public:
	NetscapePluginView(BRect rect);	
};

struct InstanceInfo {
	NetscapePluginView* playerView;
};

NetscapePluginView::NetscapePluginView(BRect rect)
	:	PluginView(rect)
{
}

NPError NPP_Initialize(void)
{
    return NPERR_NO_ERROR;
}

jref NPP_GetJavaClass()
{
    return NULL;
}

void NPP_Shutdown(void)
{
}

NPError NPP_New(NPMIMEType, NPP instance, uint16 , int16 argc,
	char *argn[], char *argv[], NPSavedData*)
{
	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	NetscapePluginView *view = new NetscapePluginView(BRect(0,0,100,100));
	for (int i = 0; i < argc; i++)
		view->SetProperty(argn[i], argv[i]);

    InstanceInfo* instanceInfo = (InstanceInfo*) NPN_MemAlloc(sizeof(InstanceInfo));
	if (instanceInfo == NULL)
		return NPERR_OUT_OF_MEMORY_ERROR;

	instanceInfo->playerView = view;
	instance->pdata = instanceInfo;

	return NPERR_NO_ERROR;
}

NPError NPP_Destroy(NPP instance, NPSavedData**)
{
	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	NetscapePluginView *pluginView = 0;
	InstanceInfo* instanceInfo = (InstanceInfo*) instance->pdata;
	if ((instanceInfo != NULL) && (instanceInfo->playerView != NULL)) {
		pluginView = (NetscapePluginView*) instanceInfo->playerView;
		NPN_MemFree(instance->pdata);
		instance->pdata = NULL;
	}

	if (pluginView == 0)
		return NPERR_NO_ERROR;

	BWindow *window = pluginView->Window();
	if (window->Lock()) {
		pluginView->RemoveSelf();
		delete pluginView;
		window->Unlock();
	}

	return NPERR_NO_ERROR;
}

NPError NPP_SetWindow(NPP instance, NPWindow* npWindow)
{
	InstanceInfo* instanceInfo;
	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	if (npWindow == NULL)
		return NPERR_NO_ERROR;

	instanceInfo = (InstanceInfo*) instance->pdata;
	BView* parent = (BView*) npWindow->window;
	BWindow *window = parent->Window();
	if (window->Lock()) {
		// Opera sometimes tries to attach me to the window
		// twice.  Check that I am not attached yet before continuing.
		if (instanceInfo->playerView->Window() == 0) {
			BRect bounds = parent->Bounds();
			parent->AddChild(instanceInfo->playerView);
			instanceInfo->playerView->ResizeTo(bounds.Width(), bounds.Height());
		}

		window->Unlock();
	}

	return NPERR_NO_ERROR;
}

NPError NPP_NewStream(NPP instance, NPMIMEType, NPStream*, 
	NPBool, uint16 *stype)
{
	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	*stype = NP_ASFILE;
	return NPERR_NO_ERROR;
}


int32 NPP_WriteReady(NPP, NPStream*)
{
	return 0xfffffff;
}

int32 NPP_Write(NPP, NPStream*, int32, int32, void*)
{
	return -1;
}

NPError NPP_DestroyStream(NPP instance, NPStream*, NPError)
{
	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	return NPERR_NO_ERROR;
}

void NPP_StreamAsFile(NPP , NPStream *, const char* )
{
}

void NPP_Print(NPP instance, NPPrint* printInfo)
{
	if (printInfo == NULL)
		return;

	if (instance != NULL)
		if (printInfo->mode == NP_FULL)
			printInfo->print.fullPrint.pluginPrinted = FALSE;
}

// This is needed so this module will be linked (as it is otherwise never
// referenced by the media player, only as an add-on)
void _STUB_LINK_ME()
{
}

