/*	ViewPropHandler.cpp
 *	$Id: ViewPropHandler.cpp,v 1.1 1996/12/04 11:58:43 hplus Exp elvis $
 *	Generic scripting of a BView
 */

#include "ViewPropHandler.h"
#include <View.h>
#include "LazyPropHandler.h"
#include "MessengerHandler.h"
#include "NotifyLazyPropHandler.h"


ViewPropHandler::ViewPropHandler(
	BView *				view) :
	LazyScriptHandler((long)view, view->Name())
{
	fView = view;
}


ScriptHandler *
ViewPropHandler::GetSubHandler(
	const char *			propertyName,
	EForm					form,
	const SData &			data)
{
	int propCount = -1;
	LazyPropHandler *lprop;

	if (!strcmp(propertyName, "property") && (form == formIndex)) {
		propCount = data.index;
	}
	if (!strcmp(propertyName, "messenger") || (0 == --propCount)) {
		return new MessengerHandler("messenger", fView);
	}
	if (!strcmp(propertyName, "frame") || (0 == --propCount)) {
		fFrame = fView->Frame();
		lprop = new LazyPropHandler("frame", &fFrame, sizeof(BRect), B_RECT_TYPE, false);
		lprop->SetHandler(this);
		return lprop;
	}
	if (!strcmp(propertyName, "color") || (0 == --propCount)) {
		fColor = fView->ViewColor();
		lprop = new NotifyLazyPropHandler<ViewPropHandler, long>("color", &fColor, sizeof(fColor), B_RGB_COLOR_TYPE,
				*this, 0);
		lprop->SetHandler(this);
		return lprop;
	}
	if (!strcmp(propertyName, "view") || (0 == --propCount)) {
		long fIndex = -1;
		BView *subView = NULL;
		switch (form) {
		case formReverseIndex:
			fIndex = fView->CountChildren()+data.index;
			break;
		case formIndex:
			fIndex = data.index-1;
			break;
		case formFirst:
			fIndex = 0;
			break;
		case formLast:
			fIndex = fView->CountChildren()-1;
			break;
		case formName:
			subView = fView->FindView(data.name);
			break;
		case formID:
		case formDirect:
		default:
			/*	These forms are not supported
			 */
			break;
		}
		if (fIndex >= 0)
			subView = fView->ChildAt(fIndex);
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
	if (propCount > 0) {
		SData newData;
		newData.index = propCount;
		return inherited::GetSubHandler(propertyName, formIndex, newData);
	}
	return inherited::GetSubHandler(propertyName, form, data);
}


void
ViewPropHandler::NotifyChange(
		NotifyLazyPropHandler<ViewPropHandler, long> &
							prop,
		long				what)
{
	fView->Window()->Lock();
	fView->SetViewColor(fColor);
	fView->Window()->Unlock();
}
	



