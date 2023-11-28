//--------------------------------------------------------------------
//
//	Written by: Mathias Agopian
//	
//	Copyright 2000 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>

#include <Application.h>
#include <Window.h>
#include <View.h>
#include <ScrollView.h>
#include <Screen.h>
#include <Rect.h>
#include <Button.h>
#include <Screen.h>
#include <Region.h>
#include <Menu.h>
#include <PopUpMenu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <TextControl.h>

#include "PreviewWindow.h"

static unsigned char ZoomIn[] = {16,1,6,6,
	7,0,29,192,48,96,96,48,64,16,208,24,144,8,208,24,
	72,16,255,48,130,124,147,254,187,63,146,63,130,31,254,14,
	7,0,31,192,63,224,127,240,127,240,255,248,255,248,255,248,
	127,240,255,240,255,252,255,254,255,63,254,63,254,31,254,14
};

static unsigned char ZoomOut[] = {16,1,6,6,
	7,0,29,192,48,96,96,48,64,16,208,24,144,8,208,24,
	72,16,255,48,130,124,131,254,187,63,130,63,130,31,254,14,
	7,0,31,192,63,224,127,240,127,240,255,248,255,248,255,248,
	127,240,255,240,255,252,255,254,255,63,254,63,254,31,254,14
};

#define BORDER	7

PreviewWindow::PreviewWindow(BPrinterAddOn& addon)
	:	BWindow(BRect(50,50,200,200), "Preview", B_DOCUMENT_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS | B_NOT_MINIMIZABLE),
		fAddOn(addon)
{
	fSem = create_sem(0,"sync");

	// Just to have size reference
	font_height fh;
	be_plain_font->GetHeight(&fh);
	fFontHeight = (fh.ascent+fh.descent+fh.leading);

	// We need a background
	BView *background = new BView(Bounds(), NULL, B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	background->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(background);

	// The control view
	fControlView = new PreviewControlView(BPoint(BORDER, BORDER));
	background->AddChild(fControlView);
	
	// The Page view
	BRect r = background->Bounds();
	r.top = fControlView->Frame().bottom + BORDER;
	r.right -= B_V_SCROLL_BAR_WIDTH;
	r.bottom -= B_H_SCROLL_BAR_HEIGHT;
	fView = new PRView(r, addon, fAddOn.Settings().FirstPage());

	// The ScrollView
	fScrollView = new BScrollView(NULL, fView, B_FOLLOW_ALL_SIDES, B_FRAME_EVENTS, true, true, B_NO_BORDER);
	background->AddChild(fScrollView);
	fScrollView->SetViewColor(background->ViewColor());

	fView->set_scale(1.0f);
	
	// Resize the Window
	float w, h;
	minimal_size(&w, &h);
	ResizeTo(w, h);

	// Center the window
	BRect screenFrame = BScreen(B_MAIN_SCREEN_ID).Frame();
	BPoint pt = screenFrame.LeftTop();
	pt.x += (screenFrame.Width()- Bounds().Width())*0.5f;
	pt.y += (screenFrame.Height() - Bounds().Height())*0.5f;
	MoveTo(pt);	

	// Allways set window size limits
	float minWidth, maxWidth, minHeight, maxHeight;
	GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);
	minWidth = fControlView->Frame().Width() + 1.0f + BORDER*2;
	minHeight = fControlView->Frame().Height() + 1.0f + BORDER*2 + B_H_SCROLL_BAR_HEIGHT + 64;
	SetSizeLimits(minWidth, maxWidth, minHeight, maxHeight);

	// Update the control's view buttons
	update_buttons();
}

void PreviewWindow::Zoom(BPoint o, float w, float h)
{
//	minimal_size(&w, &h);
	BWindow::Zoom(o, w, h);
}

PreviewWindow::~PreviewWindow()
{
	delete_sem(fSem);
}

void PreviewWindow::MessageReceived(BMessage *m)
{
	switch (m->what)
	{
		case 'next':
			if (fView->Page() < (int32)fAddOn.Settings().LastPage())
			{
				fView->do_page(fView->Page()+1);
				update_buttons();
			}
			break;

		case 'prev':
			if (fView->Page() > (int32)fAddOn.Settings().FirstPage())
			{
				fView->do_page(fView->Page()-1);
				update_buttons();
			}
			break;
		
		case 'page':
			{
				BTextControl *source;
				m->FindPointer("source", (void **)&source);
				int32 p = atoi(source->Text());
				if ((p >= (int32)fAddOn.Settings().FirstPage()) && (p <= (int32)fAddOn.Settings().LastPage()))
				{
					fView->do_page(p);
					update_buttons();
				}
			}
		
		case 'SCAL':
			if (m->FindFloat("scale") != 0)
				fView->set_scale(m->FindFloat("scale"));
			break;

		case 'OutR':	
			fView->set_scale(fAddOn.Settings().DeviceXdpi() / 72.0f);
			update_menufield();
			break;

		case 'FITS':
			{ // compute the scaling factor so that the page fits on screen
				const float aspectRatio = fView->ContentRect().Height() / fView->ContentRect().Width();
				BRect r = best_rect_for_ratio(fView->Bounds(), aspectRatio);
				const float physic_to_page = 72.0f / fAddOn.Settings().Xdpi();
//				BRect realRect = fAddOn.Settings().DeviceArea().InsetByCopy(-BORDER, -BORDER);
				BRect realRect = fAddOn.Settings().PaperRect().InsetByCopy(-BORDER, -BORDER);
				realRect.right += 1;
				realRect.bottom += fFontHeight + 1;
				const float scale = ((r.Width()+1)*physic_to_page) / (realRect.Width()+1);
				fView->set_scale(scale);
				update_menufield();
			}
			break;

		default:
			BWindow::MessageReceived(m);
	}
}
	
void PreviewWindow::update_buttons()
{
	if (fAddOn.Settings().NbPages() <= 1)
		fControlView->SetButtons(false, false);
	else if (fView->Page() >= (int32)fAddOn.Settings().LastPage())		
		fControlView->SetButtons(true, false);
	else if (fView->Page() <= (int32)fAddOn.Settings().FirstPage())		
		fControlView->SetButtons(false, true);
	else
		fControlView->SetButtons(true, true);
	fControlView->SetPage(fView->Page());		
}

void PreviewWindow::update_menufield()
{
	BString s;
	s << fView->scale()*100.0f << "%";
	fControlView->SetScaleLabel(s.String());
}

void PreviewWindow::Wait()
{
	sem_id sem = fSem;
	while (acquire_sem(sem) == B_INTERRUPTED) { };
}


void PreviewWindow::FrameResized(float w, float h)
{
	BWindow::FrameResized(w, h);
	update_scrollbars();
}

void PreviewWindow::minimal_size(float *w, float *h)
{
	*w = fView->ContentWidth() + B_V_SCROLL_BAR_WIDTH;
	*h = fView->ContentHeight() + fControlView->Bounds().Height() + BORDER + B_H_SCROLL_BAR_HEIGHT + BORDER;
}

void PreviewWindow::update_scrollbars()
{
	BScrollBar *s;
	s = fScrollView->ScrollBar(B_HORIZONTAL);
	if (fView->Bounds().Width() >= fView->ContentWidth())
	{
		s->SetProportion(1.0f);
		s->SetRange(0, 0);
	}
	else
	{
		s->SetProportion((fView->Bounds().Width() / fView->ContentWidth()));
		float min, max;
		s->GetRange(&min, &max);
		if (max != fView->ContentWidth()-fView->Bounds().Width())
			s->SetRange(0, fView->ContentWidth()-fView->Bounds().Width());
	}
	s->SetSteps(fFontHeight*fView->scale(), fView->Bounds().Width());

	s = fScrollView->ScrollBar(B_VERTICAL);
	if (fView->Bounds().Height() >= fView->ContentHeight())
	{
		s->SetProportion(1.0f);
		s->SetRange(0, 0);
	}
	else
	{
		s->SetProportion((fView->Bounds().Height() / fView->ContentHeight()));
		s->SetRange(0, fView->ContentHeight()-fView->Bounds().Height());
		float min, max;
		s->GetRange(&min, &max);
		if (max != fView->ContentHeight()-fView->Bounds().Height())
			s->SetRange(0, fView->ContentHeight()-fView->Bounds().Height());
	}
	s->SetSteps(fFontHeight*fView->scale(), fView->Bounds().Height());
}

BRect PreviewWindow::best_rect_for_ratio(const BRect& r, const float hw)
{
	float w, h;
	w = r.Width()+1.0f;
	h = w * hw;
	if (h > r.Height()+1.0f)
	{
		h = r.Height()+1.0f;
		w = h / hw;
	}
	BRect output(0,0,w-1,h-1);
	output.OffsetBy(0.5f*(r.Width()-w), 0.5f*(r.Height()-h));
	return output;
}

// ///////////////////////////////////////////////////////////////////////////////////////////
// #pragma mark -

PRView::PRView(BRect frame, BPrinterAddOn& addon, int cur_page)
	:	BView(frame, NULL, B_FOLLOW_ALL_SIDES, B_FRAME_EVENTS | B_WILL_DRAW | B_SUBPIXEL_PRECISE | B_NAVIGABLE),
		fAddOn(addon),
		fPosition(B_ORIGIN),
		fTrackingForScrolling(false),
		pPicturesTab(NULL),
		page(cur_page)
{
}

PRView::~PRView(void)
{
	if (pPicturesTab)
	{ // Free the read page
		pPicturesTab = NULL;
		fAddOn.PutPage(&fPageInfo);
	}
}

void PRView::AttachedToWindow()
{
	SetViewColor(B_TRANSPARENT_32_BIT);

	font_height fh;
	be_plain_font->GetHeight(&fh);
	fFontHeight = (fh.ascent+fh.descent+fh.leading);
	SetFont(be_plain_font);

	// Read the data for this page
	do_page(page);
}

void PRView::MouseDown(BPoint pt)
{
	MakeFocus(true);
	BWindow *wind = Window();
	int32 buttons;
	BMessage *msg = wind->CurrentMessage();
	if (msg->FindInt32("buttons", &buttons) == B_OK)
	{
		if (buttons == B_PRIMARY_MOUSE_BUTTON)
		{
			int32 modifiers = 0;
			msg->FindInt32("modifiers", &modifiers);
			if ((modifiers & (B_SHIFT_KEY)))	set_scale(scale()*0.5f);
			else								set_scale(scale()*2.0f);
			// TODO: center clicked point in the view where the mouse is
		}
		else if (buttons == B_SECONDARY_MOUSE_BUTTON)
		{ // Scrolling - asynchronous controls
			SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY|B_LOCK_WINDOW_FOCUS);
			startSrolling = ConvertToScreen(pt) + Bounds().LeftTop();
			fTrackingForScrolling = true;
			be_app->SetCursor(B_HAND_CURSOR);
		}
	}
	BView::MouseDown(pt);
}

void PRView::MouseUp(BPoint)
{
	fTrackingForScrolling = false;
}

void PRView::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	if (fTrackingForScrolling)
	{
		uint32 buttons;
		GetMouse(&point, &buttons, false);
		BPoint p = startSrolling - ConvertToScreen(point);
		if (Bounds().LeftTop() != p)
			ScrollTo(p);
	}
	else
	{
		switch (transit)
		{
			case B_ENTERED_VIEW:	be_app->SetCursor(ZoomIn);			break;
			case B_EXITED_VIEW:		be_app->SetCursor(B_HAND_CURSOR);	break;
			default:
				if ((modifiers() & (B_SHIFT_KEY)))	be_app->SetCursor(ZoomOut);
				else								be_app->SetCursor(ZoomIn);
		}
	}
	BView::MouseMoved(point, transit, message);
}

void PRView::FrameResized(float w, float h)
{
	BView::FrameResized(w, h);
	// Center the frame, always on even pixels
	float x = 0.5 * (Bounds().Width()-ContentWidth());
	float y = 0.5 * (Bounds().Height()-ContentHeight());
	if (x<0) x=0;
	if (y<0) y=0;
	x = (float)((int)x & ~0x1);
	y = (float)((int)y & ~0x1);
	set_position(BPoint(x, y));
}

void PRView::set_position(const BPoint& p)
{
	if (p != fPosition)
	{
		if (LockLooper())
		{
			BRect r = ContentRect().InsetByCopy(-BORDER, -BORDER);
			fPosition = p;
			BRect n = ContentRect().InsetByCopy(-BORDER, -BORDER);
			BRegion invalidate;
			invalidate.Set(r);
			invalidate.Exclude(n.InsetByCopy(BORDER, BORDER));
			CopyBits(r, n);
			SetHighColor(Parent()->ViewColor());
			FillRegion(&invalidate);
			UnlockLooper();
		}
	}
}

BRect PRView::ContentRect()
{
//	BRect r = fAddOn.Settings().DeviceArea() * fViewScale;
	BRect r = fAddOn.Settings().PaperRect() * fViewScale;
	r.OffsetTo(BORDER, BORDER);
	r.OffsetBy(fPosition);
	r.right += 1;
	r.bottom += fFontHeight + 1;
	return r;
}

void PRView::Draw(BRect frame)
{
	const rgb_color cWhite = {255,255,255,255};
//	BRect paper_rect		= fAddOn.Settings().DeviceArea() * fViewScale + BPoint(BORDER, BORDER) + fPosition;
//	BRect printable_rect	= fAddOn.Settings().DevicePrintableArea() * fViewScale + paper_rect.LeftTop();	

	BRect paper_rect		= fAddOn.Settings().PaperRect() * fViewScale + BPoint(BORDER, BORDER) + fPosition;
	BRect printable_rect	= fAddOn.Settings().PrintableRect() * fViewScale + paper_rect.LeftTop();	

	// Erase the background
	BRegion background;
	background.Set(frame);
	background.Exclude(paper_rect.InsetByCopy(1,1));
	SetHighColor(Parent()->ViewColor());
	FillRegion(&background);

	if (paper_rect.Intersects(frame))
	{
		// erase the page
		SetHighColor(cWhite);
		SetLowColor(cWhite);
		FillRect(paper_rect.InsetByCopy(1,1));
	
		if (pPicturesTab)
		{
			// Draw the page
			do_image(	fViewScale,
						printable_rect,
						picture_count,
						pPicturesTab,
						Points,
						Clips);
		}
		
		// Draw the margins
		SetScale(1.0f);
		ConstrainClippingRegion(NULL);
		SetHighColor(192,192,192);
		SetPenSize(1.0f);
		SetDrawingMode(B_OP_COPY);
		StrokeLine(BPoint(printable_rect.left, paper_rect.top),		BPoint(printable_rect.left, paper_rect.bottom),		B_MIXED_COLORS);
		StrokeLine(BPoint(printable_rect.right, paper_rect.top),	BPoint(printable_rect. right,paper_rect.bottom),	B_MIXED_COLORS);
		StrokeLine(BPoint(paper_rect.left, printable_rect.top),		BPoint(paper_rect.right, printable_rect.top),		B_MIXED_COLORS);
		StrokeLine(BPoint(paper_rect.left, printable_rect.bottom),	BPoint(paper_rect.right, printable_rect.bottom),	B_MIXED_COLORS);
		SetHighColor(0,0,0);
		StrokeRect(paper_rect);
		StrokeLine(BPoint(paper_rect.left+1, paper_rect.bottom+1),	BPoint(paper_rect.right+1, paper_rect.bottom+1));
		StrokeLine(BPoint(paper_rect.right+1, paper_rect.bottom+1),	BPoint(paper_rect.right+1, paper_rect.top+1));
	}
	else
	{
		SetPenSize(1.0f);
		SetDrawingMode(B_OP_COPY);
		SetHighColor(0,0,0);
		SetLowColor(Parent()->ViewColor());
	}


	MovePenTo(paper_rect.left+2, paper_rect.bottom + fFontHeight);
	const int32 firstPage = fAddOn.Settings().FirstPage();
	BString pageString("Page: ");
	pageString << page << " (" << page-firstPage+1 << " / " << fAddOn.Settings().NbPages() << ")";
	DrawString(pageString.String());
}

void PRView::do_image(float scalefactor, BRect raster_rect, uint32 picture_count, BPicture *pPicturesTab[], BPoint *where, BRect *clips)
{
	// First of all, specify the resolution
	const BRect noScaledRasterRect = raster_rect/scalefactor;
	SetScale(scalefactor);

	// Disable anti-aliasing if scale > output resolution
	if (scale()*72.0f >= fAddOn.Settings().DeviceXdpi())
		ForceFontFlags(B_DISABLE_ANTIALIASING);
	else
		ForceFontFlags(B_DISABLE_HINTING);

	// go through the list of pictures
	for (uint32 p=0 ; p<picture_count ; p++)
	{
		// Calc the rect of the BPicture in the user's coordinate system
		BRect tmp_rect = clips[p];
		tmp_rect.OffsetTo(noScaledRasterRect.LeftTop() + where[p]);

		// If this BPicture intersects the current raster
		BRect clip_rect = (tmp_rect & noScaledRasterRect);
		clip_rect.left		= (int)(clip_rect.left + 0.5f);
		clip_rect.top		= (int)(clip_rect.top + 0.5f);
		clip_rect.right		= ((int)(clip_rect.right + 0.5f)) - 1;
		clip_rect.bottom	= ((int)(clip_rect.bottom + 0.5f)) - 1;

		if (clip_rect.IsValid())
		{
			// Then, clip to this intersection
			BRegion tmp_region;			
			tmp_region.Set(clip_rect);
			ConstrainClippingRegion(&tmp_region);

			// render the BPicture
			BPoint tmp_point = noScaledRasterRect.LeftTop() + where[p];
			DrawPictureAsync(pPicturesTab[p], tmp_point);
		}
	}
	ForceFontFlags(0);
}

status_t PRView::do_page(uint32 page_number)
{
	status_t result = B_OK;

	if (pPicturesTab)
	{ // Free the read page
		pPicturesTab = NULL;
		fAddOn.PutPage(&fPageInfo);
	}

	if ((result = fAddOn.GetPage(page_number, &fPageInfo)) != B_OK)
		return result;

	// Find out how many pictures are used to describe that page.
	picture_count = fPageInfo.picture_count;
	Points = fPageInfo.points;
	Clips = fPageInfo.clips;
	pPicturesTab = fPageInfo.pictures;
	page = fPageInfo.page;
	
	if (Window())
	{
		Invalidate(ContentRect());
		Window()->UpdateIfNeeded();
	}
	return B_OK;
}


void PRView::set_scale(float f)
{
	if (f == scale())
		return;

	const float physic_to_page = 72.0f / fAddOn.Settings().Xdpi();
	if (physic_to_page*f < 0.1f)
		f = 0.1f/physic_to_page;
	else if (physic_to_page*f > 10.0f)
		f = 10.0f/physic_to_page;
	
	if (f != scale())
	{
		fUserScale = f;
		fViewScale = physic_to_page * f;
		Invalidate();
		Window()->UpdateIfNeeded();
	
		// update the scrollbars
		Window()->FrameResized(Window()->Bounds().Width(), Window()->Bounds().Height());
		// update the page position
		FrameResized(Bounds().Width(), Bounds().Height());
	}
}

float PRView::ContentWidth()
{
	return ContentRect().Width() + 1.0f + BORDER*2;
}

float PRView::ContentHeight()
{
	return ContentRect().Height() + 1.0f + BORDER*2;
}

// ///////////////////////////////////////////////////////////////////////////////////////////
// #pragma mark -


PreviewControlView::PreviewControlView(const BPoint& position)
	: BView(BRect(position, position), NULL, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_NAVIGABLE_JUMP)
{
	BRect frame(0,0,0,0);
	BView *c;
	BMessage *m;

	// 'Previous' button
	c = fPrev = new BButton(BRect(BORDER, BORDER, 0, 0), "be:prev", "Previous", new BMessage('prev'));
	c->ResizeToPreferred();
	AddChild(c);
	frame = frame | c->Frame();

	// 'Next' button
	c = fNext = new BButton(BRect(c->Frame().right+BORDER, BORDER, 0, 0), "be:next", "Next", new BMessage('next'));
	c->ResizeToPreferred();
	AddChild(c);
	frame = frame | c->Frame();
	
	// We need a 'page' text control
	c = fTcPages = new BTextControl(BRect(c->Frame().right+BORDER*2, BORDER+3, 0, 0), NULL, "Page:", "999", new BMessage('page'));
	AddChild(c);
	c->ResizeToPreferred();
	fTcPages->SetDivider(36);	// TODO: remove this hardcoded value
	fTcPages->SetText(B_EMPTY_STRING);
	frame = frame | c->Frame();
	
	// Scale menu field
	fScaleMenu = new BPopUpMenu("Select");
	c = fScaleMenuField = new BMenuField(BRect(c->Frame().right+BORDER*2, BORDER+4, 0, 0), NULL, "Scale:", fScaleMenu);
	AddChild(c);

	m = new BMessage('SCAL');	m->AddFloat("scale", 0.25f);	fScaleMenu->AddItem(new BMenuItem("1 : 4", m));
	m = new BMessage('SCAL');	m->AddFloat("scale", 0.333f);	fScaleMenu->AddItem(new BMenuItem("1 : 3", m));
	m = new BMessage('SCAL');	m->AddFloat("scale", 0.5f);		fScaleMenu->AddItem(new BMenuItem("1 : 2", m));
	m = new BMessage('SCAL');	m->AddFloat("scale", 1.0f);		fScaleMenu->AddItem(new BMenuItem("Real size (1 : 1)", m, '1'));
	m = new BMessage('SCAL');	m->AddFloat("scale", 2.0f);		fScaleMenu->AddItem(new BMenuItem("2 : 1", m));
	m = new BMessage('SCAL');	m->AddFloat("scale", 3.0f);		fScaleMenu->AddItem(new BMenuItem("3 : 1", m));
	m = new BMessage('SCAL');	m->AddFloat("scale", 4.0f);		fScaleMenu->AddItem(new BMenuItem("4 : 1", m));
	fScaleMenu->AddItem(new BSeparatorItem());
	m = new BMessage('OutR');	m->AddFloat("scale", 0.5f);	fScaleMenu->AddItem(new BMenuItem("Output resolution", m, 'O'));
	m = new BMessage('FITS');	m->AddFloat("scale", 0.5f);	fScaleMenu->AddItem(new BMenuItem("Fit to screen", m, 'F'));
	fScaleMenu->ItemAt(3)->SetMarked(true);	// TODO: be carefull here, hardcoded value
	c->ResizeToPreferred();
	fScaleMenuField->SetDivider(36);		// TODO: remove this hardcoded value
	c->ResizeTo(160, c->Frame().Height());	// TODO: remove this hardcoded value
	frame = frame | c->Frame();

	// Resize the view to its minimal size
	frame.right += BORDER;
	frame.bottom += BORDER;
	ResizeTo(frame.Width(), frame.Height());
}

void PreviewControlView::AttachedToWindow()
{
	SetViewColor(Parent()->ViewColor());
}

void PreviewControlView::SetButtons(bool prev, bool next)
{
	fPrev->SetEnabled(prev);
	fNext->SetEnabled(next);
}

void PreviewControlView::SetPage(int32 page)
{
	BString s;
	s << page;
	fTcPages->SetText(s.String());
}

void PreviewControlView::SetScaleLabel(const char *label)
{
	fScaleMenuField->MenuItem()->SetLabel(label);
	BMenuItem *item = fScaleMenu->FindMarked();
	if (item)
		item->SetMarked(false);
}

