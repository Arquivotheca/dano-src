// ===========================================================================
//	JavaGlyph.cpp
//  Copyright 1998 by Be Incorporated.
// ===========================================================================

//#define JAVAGE

#include "JavaGlyph.h"
#include <View.h>
#ifdef JAVAGE
#include "MWAppletBuddy.h"
#endif
#include "URLView.h"
#include "HTMLDoc.h"
#include "BeDrawPort.h"

#include <Application.h>

int32 JavaGlyph::sJavaCount = 0;


#ifdef JAVAGE

class JavaAppletMonitor : public MWAppletMonitor {
public:
						JavaAppletMonitor(BMessenger messenger);

	virtual void		ShowStatus(const char *status);
	virtual void		ShowDoc(const char *url, const char *target);
	virtual void		ReportState(MWAppletState state);

private:
	BMessenger			mMessenger;
};


JavaAppletMonitor::JavaAppletMonitor(
	BMessenger	messenger)
{
	mMessenger = messenger;
}


void
JavaAppletMonitor::ShowStatus(
	const char	*status)
{
//	printf("ShowStatus: %s\n", status);
}


void
JavaAppletMonitor::ShowDoc(
	const char 	*url,
	const char	*target)
{
//	printf("ShowDoc: %s, %s\n", url, target);

	bool inCurrent = true;
	if (target != NULL) {
		if (target[0] == '\0')						// current frame
			inCurrent = true;
		else if (strcmp(target, "_self") == 0)		// current frame
			inCurrent = true;
		else if (strcmp(target, "_parent") == 0)	// parent frame
			inCurrent = true;
		else if (strcmp(target, "_top") == 0)		// topmost frame
			inCurrent = true;
		else if (strcmp(target, "_blank") == 0)		// new unnamed window
			inCurrent = false;
		else
			inCurrent = false;						// new window named target	
	}

	BMessage msg(HTML_MSG + HM_ANCHOR);
	msg.AddString("url", url);

	if (inCurrent)
		mMessenger.SendMessage(&msg);
	else
		be_app->PostMessage(&msg);
}


void
JavaAppletMonitor::ReportState(
	MWAppletState	state)
{
}

#endif


JavaGlyph::JavaGlyph(Document* htmlDoc)
	: ObjectGlyph(htmlDoc)
{
//	printf("new JavaGlyph %x\n", this);

	mAppletBuddy = NULL;
	mAppletMonitor = NULL;
	mLayoutState = -1;

#ifdef JAVAGE
	if (atomic_add(&sJavaCount, 1) == 0) {
//		printf("MWAppletBuddy::StartJava()\n");
		MWAppletBuddy::StartJava();
	}
#endif 
}


JavaGlyph::~JavaGlyph()
{
#ifdef JAVAGE
	if (mAppletBuddy != NULL) {
		delete (mAppletBuddy);		// deletes the view for you
		mAppletBuddy = NULL;
	}

	if (mAppletMonitor != NULL) {
		delete (mAppletMonitor);
		mAppletMonitor = NULL;
	}


	//if (atomic_add(&sJavaCount, -1) == 1) {
	//	printf("MWAppletBuddy::StopJava()\n");
	//	MWAppletBuddy::StopJava();
	//}
#endif
}


void
JavaGlyph::Draw(
	DrawPort	*drawPort)
{
#ifndef JAVAGE
	ObjectGlyph::Draw(drawPort);
#else
	if (mLayoutState < 1) {
		Ord		left = GetLeft();
		Ord		top = GetTop();
		BView 	*parent = ((BeDrawPort *)drawPort)->GetView();
		BView	*view = mAppletBuddy->View();
	
		left -= parent->Bounds().left;
		top -= parent->Bounds().top;
		
		BRect r = view->Bounds();
	
		if (r.left != left || r.top != top) 
			view->MoveTo(left,top);

		mLayoutState = 1;
	}
#endif
}


void
JavaGlyph::Layout(
	DrawPort	*drawPort)
{
#ifndef JAVAGE
	ObjectGlyph::Layout(drawPort);
#else
	if (mLayoutState < 0) {
		BView	*view = ((BeDrawPort *)drawPort)->GetView();
		BWindow	*window = view->Window();
		OrdRect	r;
		GetBounds(&r);
		
		mAppletMonitor = new JavaAppletMonitor(BMessenger(view, window));

//		printf("new MWAppletBuddy(%s)\n%s\n", (char *)mURL, (char *)mTag);
		mAppletBuddy = new MWAppletBuddy(BRect(r.left, r.top, r.right, r.bottom), 
										 mURL, window, mURL, mTag, mAppletMonitor);
		view->AddChild(mAppletBuddy->View());

		mLayoutState = 0;
	}
#endif
}

void
JavaGlyph::CleanupJava()
{
#ifdef JAVAGE
	if (atomic_add(&sJavaCount, -1) == 1) {
//		printf("MWAppletBuddy::StopJava()\n");
		MWAppletBuddy::StopJava();
	}
#endif
}