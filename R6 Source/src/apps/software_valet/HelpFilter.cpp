#include "HelpFilter.h"

#include <string.h>
#include <malloc.h>
#include <ClassInfo.h>
#include <Message.h>
#include <Window.h>

class ClearHelpFilter : public BMessageFilter
{
public:
				ClearHelpFilter(BStringView *v);
	virtual		filter_result Filter(BMessage *m, BHandler **);
private:
	BStringView	*fView;
	int32		lastToken;
};


/////// implementation /


HelpFilter::HelpFilter(const char *txt)
	:	BMessageFilter(B_MOUSE_MOVED),
		fTxt(NULL)
{
	if (txt)
		fTxt = strdup(txt);	
}

HelpFilter::~HelpFilter()
{
	free(fTxt);
}

filter_result	HelpFilter::Filter(BMessage *, BHandler **h)
{
	BWindow *w = cast_as((*h)->Looper(),BWindow);
	if (!w)
		return B_DISPATCH_MESSAGE;
	
	BStringView *fDst;
	fDst = cast_as(w->FindView("__help_text__"),BStringView);
	if (fDst && !*fDst->Text()) {
		fDst->SetText(fTxt);
	}
	return B_DISPATCH_MESSAGE;
}


ClearHelpFilter::ClearHelpFilter(BStringView *v)
	:	BMessageFilter(B_MOUSE_MOVED),
		fView(v),
		lastToken(-1)
{
}

filter_result	ClearHelpFilter::Filter(BMessage *m, BHandler **)
{
	int32 token = m->FindInt32("_view_token_");
	if (token != lastToken) {
		fView->SetText(B_EMPTY_STRING);
		lastToken = token;
	}
	return B_DISPATCH_MESSAGE;
}


HelpStringView::HelpStringView(BRect bounds,
				const char *name, 
				const char *text,
				uint32 resizeFlags,
				uint32 flags)
	:	BStringView(bounds,name,text,resizeFlags,flags)
{
}

void HelpStringView::AttachedToWindow()
{
	BStringView::AttachedToWindow();
	
	Window()->AddCommonFilter(new ClearHelpFilter(this));
}
