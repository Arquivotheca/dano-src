#include <Be.h>
#include "PrefsWindow.h"
#include "LabelView.h"
#include "PackWindow.h"
#include "RWindow.h"
#include "PackMessages.h"
#include "PackArc.h"

#include "Util.h"
#include "MyDebug.h"


enum {
	M_DO_ACTIVATE = 'DoAC'
};

enum {
	M_WIND_CHOSEN = 'WChs',
	M_APP_PREFS = 	'APrf',
	M_ARC_INSTALL =	'AIns',
	M_CD_INSTALL =	'CDIn',
	M_SET_AUTOC =	'SAtC',
	M_DO_CLOSE =	'DClo'
};

/************* for reference only
enum {
	CD_INSTALL =		0x0001,
	AUTO_COMPRESS =		0x0002,
	HAS_RECORD_REFS =   0x0004
};
***************/

PrefsWindow::PrefsWindow(UserPrefs prefData)
	: BWindow(BRect(0,0,320,140),"Preferences",
		B_TITLED_WINDOW,B_NOT_ZOOMABLE | B_NOT_RESIZABLE),
	thePrefs(prefData)
{
	Lock();
	
	PositionWindow(this,0.4,0.4);
	
	#if DEBUG
		PRINT(("auto compression is %s\n",
				(prefData & AUTO_COMPRESS) ? "on" : "off"));
		PRINT(("cd-rom install is %s\n",
				(prefData & CD_INSTALL) ? "on" : "off"));
	#endif
	
	AddChild(new PrefsView(Bounds()));

	Show();
	Unlock();
}

void PrefsWindow::MessageReceived(BMessage *msg)
{	
	PRINT(("PrefsWindow got message\n"));
	
	switch(msg->what) {
		case M_DO_ACTIVATE:
			Activate();
			break;
		case M_DO_NEW:
		case M_DO_CLOSE:
		case M_NAME_WINDOW:
			FindView("prefs")->MessageReceived(msg);
			break;
		default:
			BWindow::MessageReceived(msg);
			PRINT(("prefs window got message\n"));
			break;
	}
}

////////////////////////////////////////////////
#define TOP_SEGMENT_HEIGHT 36

PrefsView::PrefsView(BRect fr)
	: BView(fr,"prefs",B_FOLLOW_ALL, B_WILL_DRAW)
{
	BRect r = Bounds();
	r.top += TOP_SEGMENT_HEIGHT + 4;
	// r.bottom -= 40;
	docPrefsView = new DocPrefsView(r);
	AddChild(docPrefsView);
	appPrefsView = new AppPrefsView(r);
	AddChild(appPrefsView);
	currentView = appPrefsView;
}

void PrefsView::AttachedToWindow()
{
	BView::AttachedToWindow();
	SetViewColor(light_gray_background);
		
	BRect r = Bounds();
	dPopup = new DocsPopup("docs",this,TRUE,TRUE);
		
	r.InsetBy(12,10);
	r.bottom = r.top + 14;
	r.right = r.left + 310;
	BMenuField *popField = new BMenuField(r,"Documents",
				"Apply settings to:",dPopup);
	AddChild(popField);
		
	////// saveBtn->SetTarget(this);	
}

void DrawHSeparator(float left, float right, float y, BView *v)
{
	rgb_color saveColor = v->HighColor();
	
	v->SetHighColor(128,128,128);	
	v->StrokeLine(BPoint(left,y),BPoint(right,y));
	v->SetHighColor(255,255,255);
	y++;
	v->StrokeLine(BPoint(left,y),BPoint(right,y));
								
	v->SetHighColor(saveColor);
}

/*****************

bool PrefsView::CheckSave(const char *txt)
{
	PrefsWindow *pw = (PrefsWindow *)Window();

	BAlert *svAlert = new BAlert("",txt,"Don't Svae","Cancel","Save");
	long res = svAlert->Go();
	
	switch(res) {
		case 0:
			// don't save
			// do nothing
			pw->dirty = FALSE;
			return TRUE;
			break;
		case 1:
			// cancel
			// switch back menu setting
			dPopup->RevertItem();
			return FALSE;
			break;
		case 2:
			// save it
			BMessage *saveMsg = new BMessage(M_SAVE);
			MessageReceived(saveMsg);
			delete saveMsg;
			return TRUE;
			break;
	}
	return TRUE;
	//PRINT(("result is %d\n",res));
	//pw->dirty = FALSE;
} ***************/

void PrefsView::MessageReceived(BMessage *msg)
{
	BWindow *wind;
	PrefsWindow *pw = (PrefsWindow *)Window();
	
	switch(msg->what) {
		case M_WIND_CHOSEN: {
			PRINT(("Wind Chosen\n"));
			if (currentView != docPrefsView) {
				currentView = docPrefsView;
				Window()->ResizeTo(Window()->Frame().Width(),130);
				appPrefsView->Hide();
				docPrefsView->Show();
				docPrefsView->MakeFocus();
			}
			// send the message on for window setting
			docPrefsView->MessageReceived(msg);
			break;
		}
		case M_APP_PREFS: {
			PRINT(("App Chosen\n"));
			if (currentView != appPrefsView) {
				currentView = appPrefsView;
				Window()->ResizeTo(Window()->Frame().Width(),140);
				appPrefsView->Show();
				appPrefsView->MakeFocus();
				docPrefsView->Hide();
			}
			break;
		}
		case M_SAVE: {
			if (currentView) {
				currentView->MessageReceived(msg);
			}
			break;
		}
		case M_DO_NEW: {
			// add a new window to the menu
			
			msg->FindPointer("window",reinterpret_cast<void **>(&wind));
			dPopup->AddWindow(wind);
			break;
		}
		case M_NAME_WINDOW: {
			msg->FindPointer("window",reinterpret_cast<void **>(&wind));
			dPopup->SetWindowTitle(wind, msg->FindString("title"));
			break;
		}
		case M_DO_CLOSE: {
			msg->FindPointer("window",reinterpret_cast<void **>(&wind));
			dPopup->RemoveWindow(wind);
			break;
		}
		default:
			break;
	}
}

void PrefsView::Draw(BRect up)
{
	BView::Draw(up);
	
	float left = Bounds().left;
	float right = Bounds().right;
	
	DrawHSeparator(left,right,TOP_SEGMENT_HEIGHT,this);
}

/***************
void PrefsView::EnableSave(bool state)
{
	saveBtn->SetEnabled(state);
	saveBtn->MakeDefault(TRUE);
} *************/

//////////////////////////////////////////////////////////////////////////
MyControlView::MyControlView(	BRect frame,
								const char *name,
								ulong resizeMask,
								ulong flags)
	: BView(frame,name,resizeMask,flags)
{
}

void MyControlView::Show()
{
	if (!IsHidden())
		return;	

	BView::Show();

/***
	BView *child;
	long index = 0;
	while(child = ChildAt(index++)) {
		if (is_kind_of(child,BControl)) {
			child->SetFlags(child->Flags() | B_NAVIGABLE);
			PRINT(("marking child %d as navigable\n",index));
		}
#if 0
		if (child->Flags() & B_NAVIGABLE) {
			PRINT(("Child %d is navigable\n",index));
		}
		else
			PRINT(("Child %d is NOT navigable\n",index));
#endif
	}
***/
}

void MyControlView::Hide()
{
	if (IsHidden())
		return;
	
	BView::Hide();
/***
	BView *child;
	long index = 0;
	while(child = ChildAt(index++)) {
		if (is_kind_of(child,BControl)) {
			child->SetFlags((child->Flags()) & (~B_NAVIGABLE));
			PRINT(("marking child %d as NOT navigable\n",index));
		}
#if 0
		if (child->Flags() & B_NAVIGABLE) {
			PRINT(("Child %d is navigable\n",index));
		}
		else
			PRINT(("Child %d is NOT navigable\n",index));
#endif
	}
***/
}

///////////////////////////////////////////////////////////////////////////

DocPrefsView::DocPrefsView(BRect fr)
	: MyControlView(fr,"docprefs",B_FOLLOW_ALL,B_WILL_DRAW)
{
	BRect r = Bounds();
	
	SetViewColor(light_gray_background);
	
	r.InsetBy(12,12);
	r.bottom = r.top + 14;
	autoC = new BCheckBox(r,"autocomp","Auto Compress Files",
							new BMessage(M_SET_AUTOC));
	AddChild(autoC);
	autoC->SetTarget(Window()->FindView("prefs"));
	autoC->SetViewColor(light_gray_background);

	r.top = r.bottom + 8;
	r.bottom = r.top + 48;
	r.left += 14;
		
	helpView = new LabelView(r,"Check this option to have files and folders\
 automatically compressed and saved to a temporary file as they are added.");
	AddChild(helpView);
	
	Hide();
}

void DocPrefsView::AttachedToWindow()
{
	MyControlView::AttachedToWindow();
	BCheckBox *cbox = (BCheckBox *)FindView("autocomp");
	cbox->SetTarget(this);
		
	PRINT(("autoC target is %d\n",cbox->Target()));
}

void DocPrefsView::AllAttached()
{
	MyControlView::AllAttached();
	
	BRect r = helpView->Frame();
	
	// put this in all attahced to make sure helpView has sized its frame
	r.top = r.bottom + 9;
	r.bottom = r.top + 12;
	disView = new BStringView(r,"disabledtext","This option is disabled for CD-ROM installs");
	
	PRINT(("adding disView\n"));
	AddChild(disView);
	disView->SetFont(be_plain_font);

	disView->SetViewColor(light_gray_background);
	disView->Hide();
}

void DocPrefsView::DoSave()
{
	// set the window value
	if (theWindow->Lock()) {
		theWindow->fileFlags = flags;
		if (theWindow->arcFile) 
			theWindow->arcFile->arcFlags = flags;
			theWindow->attribDirty = TRUE;
		theWindow->Unlock();
	}
	/***********
	// disable save button
	pv->EnableSave(FALSE);
	pw->dirty = FALSE;
	*************/
}

void DocPrefsView::MessageReceived(BMessage *msg)
{
	PrefsWindow *pw = (PrefsWindow *)Window();
	PrefsView *pv = (PrefsView *)pw->FindView("prefs");
	
	//dPrintMessage("DocPrefsView",msg);
	
	switch(msg->what) {
		case M_SET_AUTOC: {
			BCheckBox *autoC = (BCheckBox *)FindView("autocomp");
			bool autoComp = autoC->Value();
			doError("Changes will take place the next time this document is opened.");
			// set the internal value
			if (autoComp)
				flags |= AUTO_COMPRESS;
			else
				flags &= ~AUTO_COMPRESS;

			DoSave();
			break;
		}
		case M_WIND_CHOSEN: {
			msg->FindPointer("window",reinterpret_cast<void **>(&theWindow));
			if (theWindow && theWindow->Lock()) {
				// get the saved setting for this document
				flags = theWindow->fileFlags;
				theWindow->Unlock();
				autoC = (BCheckBox *)FindView("autocomp");
				autoC->SetValue(flags & AUTO_COMPRESS);
				
				bool disable = flags & CD_INSTALL;
				autoC->SetEnabled(!disable);
				// helpView->SetEnabled(!disable);
				
				if (disable) {
					if (disView->IsHidden())
						disView->Show();
					helpView->SetHighColor(disabled_color);
					helpView->Invalidate();
				}
				else {
					if (!disView->IsHidden())
						disView->Hide();
					helpView->SetHighColor(0,0,0);
					helpView->Invalidate();
				} 
			}
			else {
				//doError("This window no longer exists");
				// ideally we switch	
			}
			break;
		}
		case M_SAVE: {
			DoSave();
			break;
		}
	}
}


/////////////////////////////////////////////////////////////////////////////

AppPrefsView::AppPrefsView(BRect fr)
	: MyControlView(fr,"docprefs",B_FOLLOW_ALL,B_WILL_DRAW)
{
	BRect r = Bounds();
	
	SetViewColor(light_gray_background);
}

void AppPrefsView::AttachedToWindow()
{
	MyControlView::AttachedToWindow();
	PrefsWindow *w = (PrefsWindow *)Window();
	
	
	#if DEBUG
		PRINT(("auto compression is %s\n",
				(w->thePrefs & AUTO_COMPRESS) ? "on" : "off"));
		PRINT(("cd-rom install is %s\n",
				(w->thePrefs & CD_INSTALL) ? "on" : "off"));
	#endif 
	
	
	
	BRect r = Bounds();
	
	r.InsetBy(12,10);
	r.bottom = r.top + 12;
	
	BStringView *sv;
	sv = new BStringView(r,"intypelabel","Install Type:");
	AddChild(sv);
	sv->SetViewColor(light_gray_background);
	sv->SetFont(be_plain_font);
	
	r.left += 12;
	r.top = r.bottom + 6;
	r.bottom = r.top + 12;
	BRadioButton *rb;
	rb = new BRadioButton(r,"archivebtn","Compressed Archive Install",
				new BMessage(M_ARC_INSTALL));
	AddChild(rb);
	rb->SetValue(!(w->thePrefs & CD_INSTALL));
	rb->SetTarget(this);
	
	BRect cr = r;
	cr.left += 20;
	cr.top = cr.bottom + 6;
	cr.bottom = cr.top + 12;
	BCheckBox *autoC = new BCheckBox(cr,"autocomp","Auto Compress Files",
							new BMessage(M_SET_AUTOC));
	AddChild(autoC);
	autoC->SetValue(w->thePrefs & AUTO_COMPRESS);
	autoC->SetTarget(this);
	autoC->SetViewColor(light_gray_background);
				
	r.top = cr.bottom + 12;
	r.bottom = r.top + 12;
	rb = new BRadioButton(r,"cdbtn","CD-ROM Install",
				new BMessage(M_CD_INSTALL));
	AddChild(rb);
	rb->SetValue(w->thePrefs & CD_INSTALL);
	rb->SetTarget(this);
	rb->SetEnabled(false);

	MakeFocus(TRUE);		
#if 0
	AddChild(new LabelView(autoC,"Check this option to have files and folders\
 automatically compressed and saved to a temporary file as they are added."));
#endif
}

void AppPrefsView::DoSave()
{
	PrefsWindow *pw = (PrefsWindow *)Window();
	be_app->Lock();
	((PackApp *)be_app)->prefData = pw->thePrefs;
	be_app->Unlock();
	be_app->PostMessage(M_SAVE);
}

void AppPrefsView::MessageReceived(BMessage *msg)
{
	PrefsWindow *pw = (PrefsWindow *)Window();
	PrefsView *pv = (PrefsView *)pw->FindView("prefs");
	
	//dPrintMessage("AppPrefsView",msg);
	
	switch(msg->what) {
		case M_ARC_INSTALL:
			PRINT(("GOT archive install\n"));
			// turn off cd install
			pw->thePrefs &= ~CD_INSTALL;
			DoSave();
			break;
		case M_CD_INSTALL:
			PRINT(("Got cd install\n"));
			pw->thePrefs |= CD_INSTALL;
			DoSave();
			break;
		case M_SET_AUTOC:
			PRINT(("Got set auto compress\n"));
			BControl *ctl;
			msg->FindPointer("source",reinterpret_cast<void **>(&ctl));
			if (ctl->Value()) {
				pw->thePrefs |= AUTO_COMPRESS;
			}
			else {
				pw->thePrefs &= ~AUTO_COMPRESS;				
			}
			DoSave();
			break;
		case M_SAVE:
			DoSave();
			break;
		default:
			break;
	}
}

/////////////////////////////////////////////////////////////////////////////

/*******
LabelView::LabelView(BView *sibling, const char *text, long indent)
		: BTextView(BRect(0,0,100,10),B_EMPTY_STRING,
					BRect(0,0,100,10),
					B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
					B_WILL_DRAW),
		fSib(sibling),
		fIndent(indent),
		fText(text)
{
	SetText(text);
}

void	LabelView::AttachedToWindow()
{
	DEBUGGER();
	BTextView::AttachedToWindow();
	SetViewColor(light_gray_background);
	
	SetLowColor(light_gray_background);
	
	BRect sframe = fSib->Frame();
	MoveTo(sframe.left + fIndent, sframe.bottom + 2);
	ResizeTo(sframe.Width() - fIndent, 24);
	SetTextRect(Bounds());
	SetWordWrap(TRUE);
	MakeSelectable(FALSE);
	MakeEditable(FALSE);
	
	SetFont(be_plain_font);
}
*********/

////////////////////////////////////////////////////////////////////////

//extern BList RWindow::windowList;
//extern BLocker RWindow::windowListLock;


DocsPopup::DocsPopup(	const char *title,BView *pView,
			bool radioMode,bool autoRename,menu_layout layout)
	: BPopUpMenu(title,radioMode,autoRename,layout),
	  tView(pView)
{
	BMenuItem *mitem = new BMenuItem("New Package Files",new BMessage(M_APP_PREFS));
	mitem->SetTarget(tView);
	AddItem(mitem);
	AddSeparatorItem();
	ItemAt(0)->SetMarked(TRUE);

	BList winds = RWindow::WindowList();
	
	long count = winds.CountItems();
	for (long i = 0; i < count; i++) {
		RWindow *win = (RWindow *)winds.ItemAt(i);
		if (win->Lock()) {
			BMessage *msg = new BMessage(M_WIND_CHOSEN);
			msg->AddPointer("window",win);
			BMenuItem *it = new BMenuItem(win->Title(),msg);
			it->SetTarget(tView);
			AddItem(it);
			win->Unlock();
		}	
	}
}

void DocsPopup::RevertItem()
{
	// unmark current item
	this->Superitem()->SetLabel(prevItem->Label());
	prevItem->SetMarked(TRUE);
	
	PRINT(("REVERT ITEM LABEL IS %s\n",prevItem->Label()));
}

void DocsPopup::AddWindow(BWindow *win)
{
	long i;
	for (i = CountItems()-1; i >= 2; i--) {
		BMenuItem *it = ItemAt(i);
		BMessage *msg = it->Message();
		if (msg) {
			BWindow *curWin;
			msg->FindPointer("window",reinterpret_cast<void **>(&curWin));
			if (curWin == win) {
				break;
			}
		}
	}
	if (i < 2 && win->Lock()) {
		BMessage *msg = new BMessage(M_WIND_CHOSEN);
		msg->AddPointer("window",win);
		BMenuItem *it = new BMenuItem(win->Title(),msg);
		it->SetTarget(tView);
		AddItem(it);
		win->Unlock();
	}
}

void DocsPopup::SetWindowTitle(BWindow *win, const char *title)
{
	for (long i = CountItems()-1; i >= 2; i--) {
		BMenuItem *it = ItemAt(i);
		BMessage *msg = it->Message();
		BWindow *curWin;
		msg->FindPointer("window",reinterpret_cast<void **>(&curWin));
		if (curWin == win) {
			it->SetLabel(title);
			if (it->IsMarked()) {
				// update the popupbar
				this->Superitem()->SetLabel(title);
			}
		}
	}
}

long DocsPopup::RemoveWindow(BWindow *win)
{
	for (long i = CountItems()-1; i >= 2; i--) {
		BMenuItem *it = ItemAt(i);
		BWindow *curWin;
		it->Message()->FindPointer("window",reinterpret_cast<void **>(&curWin));
		if (curWin == win) {
			RemoveItem(i);
			if (it->IsMarked()) {
				BMenuItem *def = this->ItemAt(0);
				def->SetMarked(TRUE);
				this->Superitem()->SetLabel(def->Label());
				
				BMessage msg(*it->Message());
				msg.AddBool("nosave",TRUE);
				def->Target()->Looper()->PostMessage(&msg,def->Target());
			}
			// delete it;
			return i;
		}
	}
	// no item removed
	return 0;
}
