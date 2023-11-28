// LicenseView.cpp
#include <ScrollView.h>
#include <Button.h>
#include "LicenseView.h"
#include "Util.h"
#include "MyDebug.h"

LicenseView::LicenseView(BRect b,char *t,size_t sz, char *styles)
	: BView(b,"top",B_FOLLOW_ALL,B_WILL_DRAW | B_FRAME_EVENTS)
{
	ltext = t;
	lsize = sz;
	lstyle = styles;

	SetViewColor(light_gray_background);
}

void LicenseView::AttachedToWindow()
{
	BView::AttachedToWindow();
	
	BRect r = Bounds();
	r.InsetBy(16,16);
	r.right -= B_V_SCROLL_BAR_WIDTH;
	r.bottom -=  40;
	BRect tr = r;
	tr.OffsetTo(0,0);
	tr.InsetBy(3,3);
	licView = new BTextView(r,"licensetext",tr,B_FOLLOW_ALL,B_WILL_DRAW);
	
	BScrollView *sv = new BScrollView("scroller",licView,
							B_FOLLOW_ALL,B_NAVIGABLE,FALSE,TRUE);
	AddChild(sv);

	sv->MakeFocus(TRUE);
	
	licView->SetText(ltext,lsize);	
	licView->SetWordWrap(TRUE);
	licView->MakeEditable(FALSE);
	licView->MakeSelectable(FALSE);

	if (lstyle) {
		licView->SetStylable(TRUE);
		licView->SetRunArray(0,lsize,licView->UnflattenRunArray(lstyle));
		free(lstyle); // free the style buffer
	}
	else {
		BFont	viewFont(be_fixed_font);
		viewFont.SetSize(11);
		viewFont.SetSpacing(B_BITMAP_SPACING);

		licView->SetFontAndColor(&viewFont);
	}
	

	
	// free the text buffer
	free(ltext);
	
	
	r = Bounds();
	r.InsetBy(16,16);
	r.top = r.bottom - 24;
	r.left = r.right - 80;
	
	BButton *ok = new BButton(r,"ok","Agree",new BMessage(B_QUIT_REQUESTED),
						B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	AddChild(ok);
	
	r.right = r.left - 16;
	r.left = r.right - 80;
	BMessage *q = new BMessage(B_QUIT_REQUESTED);
	q->AddBool("cancelled",TRUE);
	BButton *canc = new BButton(r,"cancel","Disagree",q,
						B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	AddChild(canc);
	
	ok->MakeDefault(TRUE);
}

void LicenseView::FrameResized(float w, float h)
{
	BView::FrameResized(w, h);
	BRect tr = licView->Bounds();
	tr.InsetBy(3,3);
	licView->SetTextRect(tr);
}
