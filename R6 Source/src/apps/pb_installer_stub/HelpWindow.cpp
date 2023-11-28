// HelpWindow.cpp
#include <Screen.h>
#include <Button.h>
#include <ScrollView.h>
#include "HelpWindow.h"
#include "HelpButton.h"
#include "Util.h"

#include "InstallerType.h"

#if (USING_HELP)

HelpWindow::HelpWindow( const char *title,
						BPoint where, const char *helpText )
	:	BWindow(BRect(0,0,240,200),title, B_TITLED_WINDOW,
				B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE)
{
	Lock();
	MoveTo(where);
	
	BRect fr = Frame();
	BRect sc;
	
	{
		BScreen si(this);
		sc = si.Frame();
	}
	
	if (fr.right > sc.right) {
		// move left
		MoveTo(sc.right - fr.Width() - 4,fr.top);
	}
	else if (fr.left < sc.left) {
		// move right
		MoveTo(sc.left + 4, fr.top);
	}
	
	if (fr.bottom > sc.bottom) {
		// move up
		MoveTo(Frame().left,sc.bottom - fr.Height() - 4);
	}
	
	AddChild(new HelpView(Bounds(),helpText));	

	Show();
	Unlock();
}

void HelpWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case M_DO_ACTIVATE: {
			Activate();
			break;
		}
		case M_SET_HTEXT: {
			BTextView *tv = (BTextView *)FindView("helptext");
			if (tv) {
				long len;
				const char *textPtr;
				msg->FindData("helptext",B_POINTER_TYPE,(const void**)&textPtr,&len);
				tv->SetText(textPtr);
			}
			break;
		}
		default:
			BWindow::MessageReceived(msg);
			break;
	}
}

///////////////////////////////////////////////////////////////////

HelpView::HelpView( BRect frame,
					const char *_helpText)
	:	BView(frame,"helpview",B_FOLLOW_ALL,B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
		helpText(_helpText)
{
	SetViewColor(light_gray_background);
}

void HelpView::AttachedToWindow()
{
	BView::AttachedToWindow();
	
	Window()->SetSizeLimits(2*BUTTON_WIDTH,
							8192,
							BOTTOM_MARGIN*2,
							8192);

	/////////// setup text view ///////////////////

	BRect r = Bounds();
	r.right -= B_V_SCROLL_BAR_WIDTH;
	r.bottom -= BOTTOM_MARGIN;
	
	BRect tr = r;
	tr.OffsetTo(0,0);
	tr.InsetBy(4,4);
	BTextView *textView =
		new HelpTextView(r,"helptext",helpText,tr,B_FOLLOW_ALL,B_WILL_DRAW);
	
	BScrollView *sv = new BScrollView("helpscroller",textView,
							B_FOLLOW_ALL,B_NAVIGABLE,FALSE,TRUE,B_NO_BORDER);
	AddChild(sv);
	sv->MakeFocus(TRUE);
	
	///////// setup Done button ///////////////////
	
	r = Bounds();
	r.InsetBy(B_V_SCROLL_BAR_WIDTH,10.0);
	r.left = r.right - BUTTON_WIDTH;
	r.top = r.bottom - BUTTON_HEIGHT;
	
	BButton *done = new BButton(r,"done","Done",
						new BMessage(B_QUIT_REQUESTED),
						B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	AddChild(done);	
	done->MakeDefault(TRUE);
}

void HelpView::Draw(BRect updt)
{
	BView::Draw(updt);
	
	DrawHSeparator(0,Bounds().right,Bounds().bottom - BOTTOM_MARGIN + 2,this);
}

//////////////////// HelpTextView ///////////////

HelpTextView::HelpTextView(BRect frame,
								const char *name,
								const char *theText,
								BRect textRect, 
								ulong resizeMask,
								ulong flags)
	:	BTextView(frame,name,textRect,resizeMask,flags),
		helpText(theText)
{
}

void HelpTextView::AttachedToWindow()
{
	BTextView::AttachedToWindow();
	
	SetFont(be_plain_font);
	//SetFontSize(12);

	if (helpText)
		SetText(helpText);

	// MakeResizable(sv);
	SetWordWrap(TRUE);
	MakeEditable(FALSE);
	MakeSelectable(FALSE);
}

void HelpTextView::FrameResized(float w, float h)
{
	BTextView::FrameResized(w,h);

	BRect b = Bounds();
	b.InsetBy(4,4);
	SetTextRect(b);
}

#endif
