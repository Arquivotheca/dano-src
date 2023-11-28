/*	WindowPropHandler.cpp
 *	$Id: WindowPropHandler.cpp,v 1.1 1996/12/04 11:58:44 hplus Exp elvis $
 *	A window as property. Stock object that can be used with non-ScriptHandler windows.
 */

#include "WindowPropHandler.h"
#include <Window.h>
#include <string.h>
#include "LazyMallocStringProp.h"
#include "WindowFrameHandler.h"
#include "ViewPropHandler.h"


enum {
	NAME,
	VISIBLE,
	ACTIVE
};


WindowPropHandler::WindowPropHandler(
	const char *			name,
	BWindow *				window) :
	LazyScriptHandler((long)window, name)
{
	fWindow = window;
	fWindow->Lock();
	fName = strdup(fWindow->Title());
	fVisible = !fWindow->IsHidden();
	fActive = fWindow->IsActive();
	fWindow->Unlock();
}


void
WindowPropHandler::NotifyChange(
	NotifyLazyPropHandler<WindowPropHandler, long> &
							/*notifier*/,
	long					what)
{
	fWindow->Lock();
	switch (what) {
	case NAME:
		fWindow->SetTitle(fName);
		break;
	case VISIBLE:
		if (fVisible) {
			fWindow->Show();
		} else {
			fWindow->Hide();
		}
		break;
	case ACTIVE:
		fWindow->Activate(fActive);
		break;
	}
	fWindow->Unlock();
}


ScriptHandler *
WindowPropHandler::GetSubHandler(
	const char *			propertyName,
	EForm					form,
	const SData &			data)
{
/*	Since we may be a lazy property, and reference ourselves with another lazy property, 
 *	we tell the lazy properties to reference us so we don't die ahead of time
 */
	NotifyLazyPropHandler<WindowPropHandler, long> *lprop = NULL;

	int propCount = -1;
	if (!strcmp(propertyName, "property") && (form == formIndex)) {
		propCount = data.index;
	}
	if (!strcmp(propertyName, "frame") || (0 == --propCount)) {
		WindowFrameHandler *wfh = new WindowFrameHandler("frame", fWindow);
		return wfh;
//		lprop = new NotifyLazyPropHandler<WindowPropHandler, long>
//			("frame", &fFrame, sizeof(BRect), B_RECT_TYPE, *this, FRAME);
//		lprop->SetHandler(this);
//		return lprop;
	}
	if (!strcmp(propertyName, "name") || (0 == --propCount)) {
		LazyMallocStringProp<WindowPropHandler, long> *mstp =
			new LazyMallocStringProp<WindowPropHandler, long>("name", &fName, *this, NAME);
		mstp->SetHandler(this);
		return mstp;
	}
	if (!strcmp(propertyName, "visible") || (0 == --propCount)) {
		lprop = new NotifyLazyPropHandler<WindowPropHandler, long>
			("visible", &fVisible, sizeof(fVisible), B_BOOL_TYPE, *this, VISIBLE);
		lprop->SetHandler(this);
		return lprop;
	}
	if (!strcmp(propertyName, "active") || (0 == --propCount)) {
		lprop = new NotifyLazyPropHandler<WindowPropHandler, long>
			("active", &fActive, sizeof(fActive), B_BOOL_TYPE, *this, ACTIVE);
		lprop->SetHandler(this);
		return lprop;
	}
	if (!strcmp(propertyName, "view") || (0 == --propCount)) {
		if (propCount == 0) {	//	the property, not the item
			LazyPropHandler *lph = new LazyPropHandler("view", NULL, 0, B_RAW_TYPE, false);
			lph->SetHandler(this);
			return lph;
		}
		long fIndex = -1;
		BView *subView = NULL;
		switch (form) {
		case formReverseIndex:
			fIndex = fWindow->CountChildren()+data.index;
			break;
		case formIndex:
			fIndex = data.index-1;
			break;
		case formFirst:
			fIndex = 0;
			break;
		case formLast:
			fIndex = fWindow->CountChildren()-1;
			break;
		case formName:
			subView = fWindow->FindView(data.name);
			break;
		case formDirect:
		case formID:
		default:
			/*	These forms are not supported
			 */
			break;
		}
		if (fIndex >= 0)
			subView = fWindow->ChildAt(fIndex);
		if (!subView)
			return NULL;
		ScriptHandler *ret = dynamic_cast<ScriptHandler *>(subView);
		if (ret)
			return ret->Reference();
		/*	For views that are not ScriptHandlers in their own right, 
		 *	we use the stock ViewPropHandler to give a certain sense of scriptability.
		 */
		return new ViewPropHandler(subView);
	}
	if (!strcmp(propertyName, "property")) {
		if (form != formName)
			return NULL;
		return GetSubHandler(data.name, formDirect, data);
	}
	return NULL;
}


status_t
WindowPropHandler::PerformScriptAction(
	BMessage *			message,
	BMessage * &		reply,
	bool&				/*wasDeferred*/)
{
	switch (message->what) {
	case kCloseVerb:
		bool	doit = false;
		if (fWindow->Lock()) {
			doit = fWindow->QuitRequested();
			if (doit)
				fWindow->Quit();
		}
		else {
			fWindow->Unlock();		
		}
		if (!reply)
			reply = new BMessage(kReplyVerb);
		reply->AddBool(kDefaultDataName, doit);
		return B_NO_ERROR;
	}
	return SCRIPT_BAD_VERB;
}
