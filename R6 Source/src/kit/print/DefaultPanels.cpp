// ***********************************************************************
// libbe.so
//
// (c) 2000, Be Incorporated
//
// Written by : Mathias Agopian
//
// ***********************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <Window.h>
#include <Box.h>
#include <Button.h>
#include <RadioButton.h>
#include <CheckBox.h>
#include <TextControl.h>
#include <StringView.h>
#include <MenuField.h>
#include <Menu.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Region.h>
#include <Beep.h>

#include <print/PrinterAddOnDefs.h>
#include <print/PrintConfigView.h>
#include <print/PrintPanel.h>
#include <PrintEnv.h>

// For the ressources
#include "AboutBox.h"

#include "DefaultPanels.h"
#include "BMPView.h"

#define BORDER 8.0f
#define PRINTER_MODE	"BE:printermode"
#define PAPER_FORMAT	"BE:paperformat"
#define PAPER_FEED		"BE:tray"

#define mGetString(_x)	(TPrintTools::RsrcGetString(_x))

JobOptionPanel::JobOptionPanel(BPrinterConfigAddOn& addon)
	:	BPrintConfigView(BRect(0,0,0,0), mGetString(0), B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW),
		BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE, B_KEY_DOWN),
		fDriver(addon),
		fRbColor(NULL),
		fRbBlack(NULL)
{
	
}

void JobOptionPanel::DetachedFromWindow()
{
	// remove the key_filter
	Window()->RemoveCommonFilter(this);
}

void JobOptionPanel::AttachedToWindow()
{
	BPrintConfigView::AttachedToWindow();
	SetViewColor(Parent()->ViewColor());

	// Create and attach the key_filter
	Window()->AddCommonFilter(this);

	// Get the printer modes
	fCountPrinterModes = fDriver.PrinterModes(&fPrinterModes);

	BRect totalFrame;
	BBox *box, *tbox;
	BRadioButton *rb;
	BStringView *sv;
	BTextControl *tc;
	BCheckBox *cb;
	BMenuField *mf;
	BMenu *mn;
	BRect r;

	BPrintJobSettings& settings = fDriver.Settings();
	char buffer[256];

	// ------------------------------------------------------------
	// Page range box
	r.Set(BORDER, BORDER, 0,0);
	box = new BBox(r);
	box->SetLabel(mGetString(1));
	AddChild(box);

		r.OffsetTo(B_ORIGIN);
		r.InsetBy(BORDER, BORDER);
		r.top += BORDER*2;
		rb = fRbAllPage = new BRadioButton(r, NULL, mGetString(2), NULL);
		box->AddChild(rb);
		rb->ResizeToPreferred();
		
		r.top = rb->Frame().bottom + BORDER/4;
		rb = fRbPages = new BRadioButton(r, NULL, mGetString(3), NULL);
		box->AddChild(rb);
		rb->ResizeToPreferred();


			BString firstPage, lastPage;
			if ((settings.NbPages() > 0) && (settings.LastPage() < LONG_MAX))
			{ // The page settings are valid. Use them.
				firstPage << settings.FirstPage();
				lastPage << settings.LastPage();
			}

			// From
			BRect f(rb->Frame());
			f.Set(f.right, f.top, f.right+40, f.bottom);
			tc = fTcPages = new BTextControl(f, NULL, NULL, firstPage.String(), NULL);
			box->AddChild(tc);
			tc->ResizeToPreferred();
			tc->ResizeTo(tc->StringWidth("888")+16, tc->Bounds().Height());
			tc->SetDivider(BORDER/2);

			// "to" text
			const char *to = mGetString(17);
			f = tc->Frame().OffsetToCopy(tc->Frame().right + BORDER/2, tc->Frame().top+2);
			box->AddChild(sv = new BStringView(f, NULL, to));
			sv->ResizeToPreferred();
			sv->ResizeTo(sv->StringWidth(to), sv->Bounds().Height());

			// To
			f = tc->Frame().OffsetToCopy(sv->Frame().right+1, tc->Frame().top);
			tc = fTcPagesTo = new BTextControl(f, NULL, NULL, lastPage.String(), NULL);
			box->AddChild(tc);
			tc->ResizeToPreferred();
			tc->ResizeTo(tc->StringWidth("888")+16, tc->Bounds().Height());
			tc->SetDivider(BORDER/2);
	
		r.top = rb->Frame().bottom + BORDER/4;
		rb = fRbCurrentPage = new BRadioButton(r, NULL, mGetString(4), NULL);
		box->AddChild(rb);
		rb->ResizeToPreferred();
	
		r.top = rb->Frame().bottom + BORDER/4;
		rb = fRbSelection = new BRadioButton(r, NULL, mGetString(5), NULL);
		box->AddChild(rb);	
		rb->ResizeToPreferred();

		// Select the appropriate control
		if ((settings.NbPages() > 0) && (settings.LastPage() < LONG_MAX))
			fRbPages->SetValue(B_CONTROL_ON);
		else
			fRbAllPage->SetValue(B_CONTROL_ON);

	// Resize the box
	float x, y;
	TPrintTools::GetPreferredSize(box, &x, &y);
	box->ResizeTo(x+BORDER, y+BORDER);
	totalFrame = box->Frame();
	tbox = box;

	// ------------------------------------------------------------
	// Nb of copies box
	r = box->Frame();
	r.Set(r.right + BORDER, BORDER, 0,0);
	box = new BBox(r);
	box->SetLabel(mGetString(6));
	AddChild(box);

		r.OffsetTo(B_ORIGIN);
		r.InsetBy(BORDER, BORDER);
		r.top += BORDER*2;
		tc = fTcCopies = new BTextControl(r, NULL, mGetString(7), "1", NULL);
		sprintf(buffer, "%u", settings.NbCopies());
		tc->SetText(buffer);
		box->AddChild(tc);
		tc->ResizeToPreferred();
		tc->ResizeTo(tc->Bounds().Width()/2.5+40, tc->Bounds().Height());

		r.top = tc->Frame().bottom + BORDER/4;
		cb = fCbAssembled = new BCheckBox(r, NULL, mGetString(8), new BMessage('copy'));
		cb->SetTarget(this);
		cb->SetValue((settings.Attributes() & B_PRINT_ATTR_ASSEMBLED)? B_CONTROL_ON : B_CONTROL_OFF);
		box->AddChild(cb);
		cb->ResizeToPreferred();

		r.top = cb->Frame().bottom + BORDER/4;
		cb = fCbReverseOrder = new BCheckBox(r, NULL, mGetString(9), new BMessage('copy'));
		cb->SetValue((settings.Attributes() & B_PRINT_ATTR_REVERSE)? B_CONTROL_ON : B_CONTROL_OFF);
		cb->SetTarget(this);
		box->AddChild(cb);
		cb->ResizeToPreferred();

		r.top = cb->Frame().bottom + BORDER/2;
		fCopyBitmap = new BMPView(r, NULL, B_FOLLOW_TOP | B_FOLLOW_LEFT);
		box->AddChild(fCopyBitmap);		
		fCopyBitmap->SetBitmap(get_copy_bitmap());
		fCopyBitmap->ResizeToPreferred();

	// Resize the box
	TPrintTools::GetPreferredSize(box, &x, &y);
	box->ResizeTo(x+BORDER, y+BORDER);
	
	// Resize the 2 boxes to the higher
	totalFrame = totalFrame | box->Frame();
	box->ResizeTo(box->Frame().Width()+1, totalFrame.Height()+1);
	tbox->ResizeTo(tbox->Frame().Width()+1, totalFrame.Height()+1);

	// ------------------------------------------------------------
	// quality box
	r = totalFrame;
	r.Set(BORDER, r.bottom + BORDER, 0, 0);
	box = new BBox(r);
	box->SetLabel(mGetString(10));
	AddChild(box);

		r.OffsetTo(B_ORIGIN);
		r.InsetBy(BORDER, BORDER);
		r.top += BORDER*2;

		const float divider = max_c(be_plain_font->StringWidth(mGetString(12)),
									be_plain_font->StringWidth(mGetString(14))) + BORDER;
		mn = fMPaperType = new BPopUpMenu(mGetString(11));
		mf = fMfPaperType = new BMenuField(r, NULL, mGetString(12), mn);
		box->AddChild(mf);
		mf->ResizeToPreferred();
		mf->SetDivider(divider);

		r.top = mf->Frame().bottom + BORDER/4;
		mn = fMResolution = new BPopUpMenu(mGetString(13));
		mf = fMfResolution = new BMenuField(r, NULL, mGetString(14), mn);
		box->AddChild(mf);
		mf->ResizeToPreferred();
		mf->SetDivider(divider);

		// Find the default paper and if we need the 'color' GUI.
		if (settings.Message().FindInt32(PRINTER_MODE, &fCurrentPrinterMode) != B_OK)
			fCurrentPrinterMode = -1;

		fCurrentPaper = -1;
		BMenuItem *item;
		bool color_ui = false;
		for (int i=0 ; i<fCountPrinterModes ; i++)
		{ 
			if ((item = fMPaperType->FindItem(fPrinterModes[i].paper)) == NULL)
			{
				BMessage *m = new BMessage('papr');
				item = new BMenuItem(fPrinterModes[i].paper, m);
				item->SetTarget(this);
				fMPaperType->AddItem(item);
			}
			if (fCurrentPrinterMode == -1)
			{
				if (fPrinterModes[i].attributes & BPrinterConfigAddOn::printer_mode_t::B_IS_DEFAULT_PAPER)
					item->SetMarked(true);
			}
			else if (fCurrentPrinterMode == i)
				item->SetMarked(true);
			
			if ((fPrinterModes[i].attributes & BPrinterConfigAddOn::printer_mode_t::B_BLACK_COLOR) == BPrinterConfigAddOn::printer_mode_t::B_BLACK_COLOR)
				color_ui = true;
		}
		if ((fMPaperType->FindMarked() == NULL) && (fMPaperType->ItemAt(0)))
			fMPaperType->ItemAt(0)->SetMarked(true);
		PopulateQualityMenu(fMPaperType->IndexOf(fMPaperType->FindMarked()));

		if (color_ui)
		{
			r.top = mf->Frame().bottom + BORDER/4;
			rb = fRbColor = new BRadioButton(r, NULL, mGetString(15), NULL);
			rb->SetValue(settings.Color() ? B_CONTROL_ON : B_CONTROL_OFF);
			box->AddChild(rb);
			rb->ResizeToPreferred();

			r.top = rb->Frame().bottom + BORDER/4;
			rb = fRbBlack = new BRadioButton(r, NULL, mGetString(16), NULL);
			rb->SetValue(settings.Color() ? B_CONTROL_OFF : B_CONTROL_ON);
			box->AddChild(rb);
			rb->ResizeToPreferred();
		}

	// Resize the box
	TPrintTools::GetPreferredSize(box, &x, &y);
	box->ResizeTo(totalFrame.Width(), y+BORDER);

	// ------------------------------------------------------------
	// Resize the view to its content
	ResizeToPreferred();

	update_ui(UPD_PAGES);
}

status_t JobOptionPanel::PopulateQualityMenu(int32 paper)
{
	if (fCurrentPaper == paper)
		return B_OK;

	int32 first = -1;
	BMenuItem *item;
	while ((item = fMResolution->RemoveItem((int32)0)) != NULL)
		delete item;	
	for (int i=0 ; i<fCountPrinterModes ; i++)
	{
		if (fMPaperType->IndexOf(fMPaperType->FindItem(fPrinterModes[i].paper)) == paper)
		{
			if (first == -1)
				first = i;
			BMessage *m = new BMessage('qual');
			m->AddInt32(PRINTER_MODE, i);
			item = new BMenuItem(fPrinterModes[i].quality, m);
			item->SetTarget(this);
			fMResolution->AddItem(item);			
			if  (fCurrentPrinterMode == -1)
			{
				if (fPrinterModes[i].attributes & BPrinterConfigAddOn::printer_mode_t::B_IS_DEFAULT_QUALITY)
				{
					fCurrentPrinterMode = i;
					item->SetMarked(true);
				}
			}
			else if (fCurrentPrinterMode == i)
				item->SetMarked(true);
		}
	}
	if (fMResolution->FindMarked() == NULL)
	{
		fMResolution->ItemAt(0)->SetMarked(true);
		fCurrentPrinterMode = first;
	}
	fMPaperType->IndexOf(fMPaperType->FindMarked());

	if ((fRbColor) && (fRbBlack))
	{ // Update the color ui
		bool b;
		b = ((fPrinterModes[fCurrentPrinterMode].attributes & BPrinterConfigAddOn::printer_mode_t::B_BLACK_ONLY) != 0);
		fRbBlack->SetEnabled(b);
		fRbBlack->SetValue(b ? B_CONTROL_OFF : B_CONTROL_ON);	
		b = ((fPrinterModes[fCurrentPrinterMode].attributes & BPrinterConfigAddOn::printer_mode_t::B_COLOR_ONLY) != 0);
		fRbColor->SetEnabled(b);
		fRbColor->SetValue(b ? B_CONTROL_OFF : B_CONTROL_ON);
	}
	fCurrentPaper = paper;
	return B_OK;
}



status_t JobOptionPanel::Save()
{
	status_t err;
	BPrintJobEditSettings& settings = fDriver.Settings();

	if (fRbAllPage->Value() == B_CONTROL_ON)
	{ // Print all Pages
		settings.SetFirstPage(1);
		settings.SetLastPage(LONG_MAX);
	}
	else
	{
		settings.SetFirstPage(atoi(fTcPages->Text()));
		settings.SetLastPage(atoi(fTcPagesTo->Text()));
		if (settings.FirstPage() > settings.LastPage())
		{ // The pages values are not consistant.
			beep();
			for (int i=0 ; i<2 ; i++)
			{	
				fTcPages->MakeFocus(true);
				snooze(80000);
				fTcPages->MakeFocus(false);
				snooze(80000);
			}
			fTcPages->MakeFocus(true);
			return B_BAD_VALUE;
		}
	}

	settings.SetNbCopies(atoi(fTcCopies->Text()));
	if (fRbColor)
		settings.SetColor(fRbColor->Value() != B_CONTROL_OFF);

	settings.SetAttributes(settings.Attributes() & ~B_PRINT_ATTR_ASSEMBLED);
	settings.SetAttributes(settings.Attributes() & ~B_PRINT_ATTR_REVERSE);
	if (fCbAssembled->Value() == B_CONTROL_ON)		settings.SetAttributes(settings.Attributes() | B_PRINT_ATTR_ASSEMBLED);
	if (fCbReverseOrder->Value() == B_CONTROL_ON)	settings.SetAttributes(settings.Attributes() | B_PRINT_ATTR_REVERSE);

	if ((err = fDriver.PrinterModeSelected(fCurrentPrinterMode)) != B_OK)
	{ // Select the BControl that caused the error
		fMfResolution->MakeFocus(true);
		return err;
	}
	settings.Message().RemoveName(PRINTER_MODE);
	settings.Message().AddInt32(PRINTER_MODE, fCurrentPrinterMode);
	return B_OK;
}

void JobOptionPanel::GetPreferredSize(float *x, float *y)
{
	TPrintTools::GetPreferredSize(this, x, y);
	*x += BORDER;
	*y += BORDER;
}

void JobOptionPanel::update_ui(int32 code)
{
	// Lock our window
	if (LockLooper() == false)
		return;

	switch (code)
	{
		case UPD_PAGES:
			{
				// Enable or disable 'current page' and 'current selection'.
				// If it is disabled and was selected, just select 'all pages' instead.
				fRbCurrentPage->SetEnabled(fDriver.CurrentPage() != 0);
				if ((fRbCurrentPage->Value() == B_CONTROL_ON) && (fRbCurrentPage->IsEnabled() == false))
					fRbAllPage->SetValue(B_CONTROL_ON);
	
				fRbSelection->SetEnabled(fDriver.DocSelection());
				if ((fRbSelection->Value() == B_CONTROL_ON) && (fRbSelection->IsEnabled() == false))
					fRbAllPage->SetValue(B_CONTROL_ON);
	
				if (fRbCurrentPage->Value() == B_CONTROL_ON)
				{ // if 'currentpage' is selected, update the textcontrol with it
					BString currentPage;
					currentPage << fDriver.CurrentPage();
					fTcPages->SetText(currentPage.String());
					fTcPagesTo->SetText(currentPage.String());
				}
				else if ((fRbAllPage->Value() == B_CONTROL_ON) && (fDriver.PageCount()>0))
				{ // if all is selected, update the textcontrol
					BString lastPage;
					lastPage << fDriver.PageCount();
					fTcPages->SetText("1");
					fTcPagesTo->SetText(lastPage.String());
				}
	
				// If we have only one page, just disable the textboxes
				const bool onlyOnePage = (fDriver.PageCount() == 1);
				fTcPages->SetEnabled(!onlyOnePage);
				fTcPagesTo->SetEnabled(!onlyOnePage);
				break;
			}
		case UPD_SETTINGS:
			{
				BPrintJobSettings& settings = fDriver.Settings();
				char buffer[256];
				sprintf(buffer, "%u", settings.NbCopies());
				fTcCopies->SetText(buffer);
				fCbAssembled->SetValue((settings.Attributes() & B_PRINT_ATTR_ASSEMBLED)? B_CONTROL_ON : B_CONTROL_OFF);
				fCbReverseOrder->SetValue((settings.Attributes() & B_PRINT_ATTR_REVERSE)? B_CONTROL_ON : B_CONTROL_OFF);
				if (fRbColor && fRbBlack)
				{
					fRbColor->SetValue(settings.Color() ? B_CONTROL_ON : B_CONTROL_OFF);
					fRbBlack->SetValue(settings.Color() ? B_CONTROL_OFF : B_CONTROL_ON);
				}
			}
			break;
	}

	UnlockLooper();
}

void JobOptionPanel::MessageReceived(BMessage *message)
{
	switch (message->what)
	{
		case 'papr':
			PopulateQualityMenu(message->FindInt32("index"));
			break;
		case 'qual':
			fCurrentPrinterMode = message->FindInt32(PRINTER_MODE);
			break;
		case 'copy':
			fCopyBitmap->SetBitmap(get_copy_bitmap());
			break;
		default:
			BPrintConfigView::MessageReceived(message);
	}
}

filter_result JobOptionPanel::Filter(BMessage *msg, BHandler **target)
{
	const uchar *bytes = 0;
	long key;
	ulong mods = 0;
	if (*target == fTcCopies->ChildAt(0))
	{
		mods = msg->FindInt32("modifiers");
		if (mods & B_COMMAND_KEY)
			return B_DISPATCH_MESSAGE;
		if (msg->FindString("bytes", (const char **)&bytes) != B_NO_ERROR)
			return B_DISPATCH_MESSAGE;
		key =  bytes[0];
		if ((key == B_ENTER) || (key == B_TAB))
		{
			// TODO: should make sure the user didn't used a crazy parameter
			return B_DISPATCH_MESSAGE;
		}
		if (isdigit(key) || iscntrl(key))
			return B_DISPATCH_MESSAGE;
		return B_SKIP_MESSAGE;
	}
	else if ((*target == fTcPages->ChildAt(0)) || (*target == fTcPagesTo->ChildAt(0)))
	{
		mods = msg->FindInt32("modifiers");
		if (mods & B_COMMAND_KEY)
			return B_DISPATCH_MESSAGE;
		if (msg->FindString("bytes", (const char **)&bytes) != B_NO_ERROR)
			return B_DISPATCH_MESSAGE;
		key =  bytes[0];
		if ((key == B_ENTER) || (key == B_TAB))
		{
			// TODO: should make sure the user didn't used crazy parameters
			return B_DISPATCH_MESSAGE;
		}
		if (isdigit(key) || iscntrl(key))	// || (key == '-') || (key == ';') || (key == ','))
		{
			fRbPages->SetValue(B_CONTROL_ON);
			return B_DISPATCH_MESSAGE;
		}
		return B_SKIP_MESSAGE;
	}
	return B_DISPATCH_MESSAGE;
}



// ----------------------------------------------------------------------
// #pragma mark -

ConfigPagePanel::ConfigPagePanel(BPrinterConfigAddOn& addon)
	:	BPrintConfigView(BRect(0,0,0,0), mGetString(18), B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW),
		BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE, B_KEY_DOWN),
		fDriver(addon)
{
}

void ConfigPagePanel::AttachedToWindow()
{
	BPrintConfigView::AttachedToWindow();
	SetViewColor(Parent()->ViewColor());

	// Create and attach the key_filter
	Window()->AddCommonFilter(this);

	BRect r;
	BRect totalFrame;

	float x, y;
	BView *view;
	BBox *box;
	BRadioButton *rb;
	BCheckBox *cb;
	BMenuField *mf;
	BMenu *mn;

	BPrintJobSettings& settings = fDriver.Settings();
	char buffer[256];

	int32 tray;
	if (settings.Message().FindInt32(PAPER_FEED, &tray) != B_OK)					tray = 0;
	if (settings.Message().FindInt32(PAPER_FORMAT, &fCurrentPaperFormat) != B_OK)	fCurrentPaperFormat = -1;
	if (tray >= fDriver.CountTrays())
	{
		tray = 0;
		fCurrentPaperFormat = -1;
	}

	// ------------------------------------------------------------
	// Popup menus

	r.Set(BORDER, BORDER, BORDER, BORDER);
	view = new BView(r, NULL, B_FOLLOW_LEFT|B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE_JUMP);
	view->SetViewColor(ViewColor());
	AddChild(view);

		r.OffsetTo(B_ORIGIN);
		r.InsetBy(BORDER, BORDER);

		const float divider = max_c(be_plain_font->StringWidth(mGetString(19)),
									be_plain_font->StringWidth(mGetString(20))) + BORDER;
		mn = fMPaperFeeds = new BPopUpMenu(mGetString(21));
		mf = fMfPaperFeeds = new BMenuField(r, NULL, mGetString(19), mn);
		view->AddChild(mf);
		mf->ResizeToPreferred();
		mf->SetDivider(divider);

		r.top = mf->Frame().bottom + BORDER/4;
		mn = fMPaperFormats = new BPopUpMenu(mGetString(22));
		mf = fMfPaperFormats = new BMenuField(r, NULL, mGetString(20), mn);
		view->AddChild(mf);
		mf->ResizeToPreferred();
		mf->SetDivider(divider);
			
		for (int i=0 ; i<fDriver.CountTrays() ; i++)
		{
			BMessage *m = new BMessage('tray');
			BMenuItem *item = new BMenuItem(fDriver.TrayName(i), m);
			item->SetTarget(this);
			fMPaperFeeds->AddItem(item);
			if (i == tray)
				item->SetMarked(true);
		}
		
		fCurrentTray = -1;
		PopulatePaperFormatsMenu(tray);	// Select the Tray, and populate the paper format menu

	// Resize the box
	TPrintTools::GetPreferredSize(view, &x, &y);
	view->ResizeTo(x+BORDER, y+BORDER);
	totalFrame = view->Frame();

	// ------------------------------------------------------------
	// Orientation box
	r = view->Frame();
	r.Set(r.left, r.bottom + BORDER, 0,0);
	box = fBbOrientation = new BBox(r);
	box->SetLabel(mGetString(32));
	AddChild(box);

		r.OffsetTo(B_ORIGIN);
		r.InsetBy(BORDER, BORDER);
		r.top += BORDER*2;
		rb = fRbPortrait = new BRadioButton(r, NULL, mGetString(23), new BMessage('updt'));
		rb->SetValue((settings.Orientation() == B_PRINT_LAYOUT_PORTRAIT) ? B_CONTROL_ON : B_CONTROL_OFF);
		rb->SetTarget(this);
		box->AddChild(rb);
		rb->ResizeToPreferred();
		
		r.top = rb->Frame().bottom + BORDER/4;
		rb = fRbLandscape = new BRadioButton(r, NULL, mGetString(24), new BMessage('updt'));
		rb->SetValue((settings.Orientation() == B_PRINT_LAYOUT_LANDSCAPE) ? B_CONTROL_ON : B_CONTROL_OFF);
		rb->SetTarget(this);
		box->AddChild(rb);
		rb->ResizeToPreferred();

	// Resize the box
	TPrintTools::GetPreferredSize(box, &x, &y);
	box->ResizeTo(x+BORDER, y+BORDER);
	totalFrame = totalFrame | box->Frame();

	// ------------------------------------------------------------
	// Mirror box
	// Nb of copies box
	r = box->Frame();
	r.Set(r.left, r.bottom + BORDER, 0,0);
	box = fBbMirror = new BBox(r);
	box->SetLabel("Mirror");
	AddChild(box);

		r.OffsetTo(B_ORIGIN);
		r.InsetBy(BORDER, BORDER);
		r.top += BORDER*2;
		cb = fCbHMirror = new BCheckBox(r, NULL, mGetString(25), new BMessage('updt'));
		cb->SetTarget(this);
		cb->SetValue((settings.Attributes() & B_PRINT_ATTR_H_MIRROR) ? B_CONTROL_ON : B_CONTROL_OFF);
		box->AddChild(cb);
		cb->ResizeToPreferred();
		
		r.top = cb->Frame().bottom + BORDER/4;
		cb = fCbVMirror = new BCheckBox(r, NULL, mGetString(26), new BMessage('updt'));
		cb->SetTarget(this);
		cb->SetValue((settings.Attributes() & B_PRINT_ATTR_V_MIRROR) ? B_CONTROL_ON : B_CONTROL_OFF);
		box->AddChild(cb);
		cb->ResizeToPreferred();


	// Resize the box
	TPrintTools::GetPreferredSize(box, &x, &y);
	box->ResizeTo(x+BORDER, y+BORDER);
	totalFrame = totalFrame | box->Frame();

	float w = max_c(fBbOrientation->Frame().Width(), fBbMirror->Frame().Width());
	fBbOrientation->ResizeTo(w, fBbOrientation->Frame().Height());
	fBbMirror->ResizeTo(w, fBbMirror->Frame().Height());

	// ------------------------------------------------------------
	// the page preview view
	r.left = fBbOrientation->Frame().right + 2*BORDER;
	r.top = fBbOrientation->Frame().top;
	r.bottom = fBbMirror->Frame().bottom;
	r.right = r.left + (r.bottom-r.top);
	fPaperPreview = new MPreviewPage(r);
	update_paper();
	AddChild(fPaperPreview);

	// ------------------------------------------------------------
	// Resize the view to its content
	ResizeToPreferred();
	update_ui(UPD_PAGES);
}

void ConfigPagePanel::DetachedFromWindow()
{
	// remove the key_filter
	Window()->RemoveCommonFilter(this);
}

void ConfigPagePanel::GetPreferredSize(float *x, float *y)
{
	TPrintTools::GetPreferredSize(this, x, y);
	*x += BORDER;
	*y += BORDER;
}

void ConfigPagePanel::update_ui(int32 code)
{
	// Lock our window
	if (LockLooper() == false)
		return;

	// TODO: update the GUI
	switch (code)
	{
		case UPD_PAGES:
			break;
		case UPD_SETTINGS:
			break;
	}

	UnlockLooper();
}

void ConfigPagePanel::MessageReceived(BMessage *message)
{
	switch (message->what)
	{
		case 'updt':
			update_paper();
			break;

		case 'tray':
			fCurrentPaperFormat = -1;
			PopulatePaperFormatsMenu(message->FindInt32("index"));
			update_paper();
			break;

		case 'frmt':
			fCurrentPaperFormat = message->FindInt32("index");
			update_paper();
			break;

		default:
			BPrintConfigView::MessageReceived(message);
	}
}

void ConfigPagePanel::update_paper()
{
	BPrintPaper p =fPaperFormats[fCurrentPaperFormat];
	if (fRbLandscape->Value() == B_CONTROL_ON)		p.SetLandscape();
	if (fCbHMirror->Value() == B_CONTROL_ON)		p.SetHMirror(true);
	if (fCbVMirror->Value() == B_CONTROL_ON)		p.SetVMirror(true);
	if (fPaperPreview)
		fPaperPreview->Update(p);
}

status_t ConfigPagePanel::Save()
{
	status_t err;
	BPrintJobEditSettings& settings = fDriver.Settings();

	settings.SetAttributes(settings.Attributes() & ~B_PRINT_ATTR_H_MIRROR);
	settings.SetAttributes(settings.Attributes() & ~B_PRINT_ATTR_V_MIRROR);
	if (fCbHMirror->Value() == B_CONTROL_ON)	settings.SetAttributes(settings.Attributes() | B_PRINT_ATTR_H_MIRROR);
	if (fCbVMirror->Value() == B_CONTROL_ON)	settings.SetAttributes(settings.Attributes() | B_PRINT_ATTR_V_MIRROR);
	if (fRbPortrait->Value() == B_CONTROL_ON)	settings.SetOrientation(B_PRINT_LAYOUT_PORTRAIT);
	if (fRbLandscape->Value() == B_CONTROL_ON)	settings.SetOrientation(B_PRINT_LAYOUT_LANDSCAPE);

	if ((err = fDriver.PaperSelected(fCurrentTray, fCurrentPaperFormat)) != B_OK)
	{ // Select the BControl that caused the error
		fMfPaperFeeds->MakeFocus(true);
		return err;
	}

	settings.Message().RemoveName(PAPER_FEED);
	settings.Message().AddInt32(PAPER_FEED, fCurrentTray);
	settings.Message().RemoveName(PAPER_FORMAT);
	settings.Message().AddInt32(PAPER_FORMAT, fCurrentPaperFormat);
	return B_OK;
}

status_t ConfigPagePanel::PopulatePaperFormatsMenu(int32 tray)
{
	if (fCurrentTray == tray)
		return B_OK;

	// Get the printer modes
	fCountPaperFormats = fDriver.PaperFormats(tray, &fPaperFormats);
	if (fCurrentPaperFormat >= fCountPaperFormats)
		 fCurrentPaperFormat = -1; // The current paper format is not valid!
	
	BMenuItem *item;
	while ((item = fMPaperFormats->RemoveItem((int32)0)) != NULL)
		delete item;	

	for (int i=0 ; i<fCountPaperFormats ; i++)
	{
		BMessage *m = new BMessage('frmt');
		item = new BMenuItem(fPaperFormats[i].PrettyName(), m);
		item->SetTarget(this);
		fMPaperFormats->AddItem(item);

		if (fCurrentPaperFormat == -1)
		{
			if (fPaperFormats[i].id == BPrintPaper::DefaultFormat())
			{
				fCurrentPaperFormat = i;
				item->SetMarked(true);
			}
		}
		else
		{
			if (i == fCurrentPaperFormat)
				item->SetMarked(true);
		}
	}

	if (fCurrentPaperFormat == -1)
	{ // There was no default paper format. Pick one.
		fCurrentPaperFormat = 0;
		if ((item = fMPaperFormats->FindItem(fPaperFormats[fCurrentPaperFormat].PrettyName())))
			item->SetMarked(true);
	}
	
	fCurrentTray = tray;
	return B_OK;
}

filter_result ConfigPagePanel::Filter(BMessage *message, BHandler **target)
{
	return B_DISPATCH_MESSAGE;
}






// ----------------------------------------------------------------------
// #pragma mark -

MPreviewPage::MPreviewPage(BRect frame)
	: BView(frame, "MPreviewPage", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW),
	fScale(1.0f)
{
	#ifdef B_BEOS_VERSION_DANO
	SetDoubleBuffering(B_UPDATE_INVALIDATED);
	#endif
}

MPreviewPage::~MPreviewPage(void)
{
}
	
void MPreviewPage::Draw(BRect frame)
{
	// Sanity check.
	if ((fPaper.PaperWidth() == 0) || (fPaper.PaperHeight() == 0))
		return;

	BRect bounds = Bounds();
	bounds.top += fFontHeight + fFontHeight*0.5f;
	bounds.bottom -= fFontHeight*0.5f;
	
	const float f = 0.925f;
	const float wh = fPaper.AspectRatio();
	float w = bounds.Width() * f;
	float h = w * wh;
	if (h >= bounds.Height()) {
		h = bounds.Height();
		w = h/wh;
	}
	
	BRect r(0, 0, w-1, h-1);
	r.OffsetTo((bounds.Width() - w)*0.5f + bounds.left, (bounds.Height() - h)*0.5f + bounds.top);

	BRect pr = fPaper.PrintableRect();
	const float coef = w/fPaper.PaperWidth();
	const float lm = pr.left * coef;
	const float tm = pr.top * coef;
	const float rm = (fPaper.PaperWidth() - pr.right) * coef;
	const float bm = (fPaper.PaperHeight() - pr.bottom) * coef;

	// Draw the margins
	SetHighColor(255,255,255);
	SetLowColor(HighColor());
	FillRect(r.InsetByCopy(1,1));
	SetHighColor(192,192,192);
	StrokeLine(BPoint(r.left  + lm,	r.top),			BPoint(r.left  + lm,r.bottom),		B_MIXED_COLORS);
	StrokeLine(BPoint(r.right - rm,	r.top),			BPoint(r.right - rm,r.bottom),		B_MIXED_COLORS);
	StrokeLine(BPoint(r.left,		r.top+tm),		BPoint(r.right,		r.top+tm),		B_MIXED_COLORS);
	StrokeLine(BPoint(r.left,		r.bottom-bm),	BPoint(r.right,		r.bottom-bm),	B_MIXED_COLORS);

	// Area of the text
	BRect clipRect = r;
	clipRect.left += lm;
	clipRect.top += tm;
	clipRect.right -= rm;
	clipRect.bottom -= bm;
	clipRect.InsetBy(2,2);

	// Page relief
	SetHighColor(0,0,0);
	StrokeRect(r);
	SetHighColor(0,0,0,128);
	SetLowColor(ViewColor());
	SetDrawingMode(B_OP_ALPHA);
	StrokeLine(BPoint(r.left+1, r.bottom+1), BPoint(r.right+1,r.bottom+1));
	StrokeLine(BPoint(r.right+1,r.bottom+1), BPoint(r.right+1,r.top+1));
	SetDrawingMode(B_OP_COPY);

	// Draw the page name
	SetFont(be_plain_font);
	SetHighColor(0,0,0);
	SetLowColor(ViewColor());
	char buf[256];
	// TODO: we want to use localisation here
	if (true)
	{
		sprintf(buf,	"%s (%4.1f in x %4.1f in)",
						fPaper.PrettyName(),
						BPrintPaper::milimeter_to_inch(fPaper.PaperWidth()),
						BPrintPaper::milimeter_to_inch(fPaper.PaperHeight()));
	}
	else
	{
		sprintf(buf,	"%s (%4u mm x %4u mm)",
						fPaper.PrettyName(),
						(unsigned int)(fPaper.PaperWidth()+0.5),
						(unsigned int)(fPaper.PaperHeight()+0.5));
	}
	DrawString(buf, BPoint(4, 1*fFontHeight));
}

void MPreviewPage::AttachedToWindow(void)
{
	SetViewColor(Parent()->ViewColor());
	SetLowColor(ViewColor());
	SetPenSize(1.0f);
	SetDrawingMode(B_OP_COPY);
	SetFont(be_plain_font);
	font_height fh;
	be_plain_font->GetHeight(&fh);
	fFontHeight = fh.ascent + fh.descent + fh.leading;
	fOldScaleTxt[0] = 0;
}
	
void MPreviewPage::Update(const BPrintPaper& paper)
{
	fPaper = paper;
	if (Window())
	{ // Redraw the page only if it is on screen
		Invalidate();
		Window()->UpdateIfNeeded();
	}
}


// ----------------------------------------------------------------------
// #pragma mark -

ToolsPanel::ToolsPanel(BPrinterConfigAddOn& addon)
	:	BPrintConfigView(BRect(0,0,0,0), mGetString(27), B_FOLLOW_LEFT | B_FOLLOW_TOP, B_PULSE_NEEDED | B_WILL_DRAW),
		fDriver(addon),
		fLastTime(system_time())
{
	ResizeTo(4*BORDER+36*6+10*5, 16+4*BORDER+105);
	fCleanNozzle = (fDriver.CleanPrintHeads(true) == B_OK);			// Clean head availlable
	fCheckNozzle = (fDriver.PrintNozzleCheckPattern(true) == B_OK);	// Nozzle Check availlable
	fStatusBar = NULL;
}

void ToolsPanel::AttachedToWindow()
{
	BPrintConfigView::AttachedToWindow();
	SetViewColor(Parent()->ViewColor());

	const int alpha = 128;
	const rgb_color C = {0,255,255,alpha};
	const rgb_color M = {255,0,255,alpha};
	const rgb_color Y = {255,255,0,alpha};
	const rgb_color K = {0,0,0,alpha};
	const rgb_color c = {192,255,255,alpha};
	const rgb_color m = {255,192,255,alpha};
	
	BPoint p(BORDER, BORDER+16);
	float o;
	fInkContainers[3] = new InkContainer(p, K); o = fInkContainers[3]->Bounds().Width()+1+10; p.x += o;
	fInkContainers[0] = new InkContainer(p, C); p.x += o;
	fInkContainers[1] = new InkContainer(p, M); p.x += o;
	fInkContainers[2] = new InkContainer(p, Y); p.x += o;
	fInkContainers[4] = new InkContainer(p, c); p.x += o;
	fInkContainers[5] = new InkContainer(p, m);

	BBox *box = new BBox(Bounds().InsetByCopy(BORDER, BORDER), NULL, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	box->SetLabel(mGetString(60));
	AddChild(box);
	for (int i=0 ; i<6 ; i++)
		box->AddChild(fInkContainers[i]);

	BRect r(box->Frame());
	r.top = r.bottom + BORDER;
	r.bottom = r.top;

	if (fCheckNozzle) {
		fCheckButton = new BButton(r, NULL, mGetString(61), new BMessage('chck'));
		AddChild(fCheckButton);
		fCheckButton->ResizeToPreferred();
		fCheckButton->SetTarget(this);
		r = fCheckButton->Frame();
		r.OffsetBy(r.Width()+1+BORDER, 0);
	}

	if (fCleanNozzle) {
		fCleanButton = new BButton(r, NULL, mGetString(62), new BMessage('clen'));
		AddChild(fCleanButton);
		fCleanButton->ResizeToPreferred();
		fCleanButton->SetTarget(this);
		r = fCleanButton->Frame();
		r.OffsetBy(r.Width()+1+BORDER, 0);

		BRect rsb;
		rsb.top = r.bottom + BORDER;
		rsb.bottom = rsb.top;
		rsb.left = box->Frame().left;
		rsb.right = box->Frame().right;
		fStatusBar = new BStatusBar(rsb, NULL, mGetString(63));
		AddChild(fStatusBar);
		fStatusBar->SetBarHeight(8);
		fStatusBar->Hide();
	}


	ResizeToPreferred();
}

void ToolsPanel::MessageReceived(BMessage *message)
{
	if (fStatus.status <= printer_status_t::B_OFFLINE) {
		switch (message->what) {
			case 'chck':
				fDriver.PrintNozzleCheckPattern();
				fStatus.status = printer_status_t::B_PRINTING;
				return;
			case 'clen':
				fDriver.CleanPrintHeads();
				fStatus.status = printer_status_t::B_CLEANING;
				return;
		}
	}
	BPrintConfigView::MessageReceived(message);
}

void ToolsPanel::Pulse()
{
	if ((system_time() - fLastTime) < 1000000)
		return;		
	fLastTime = system_time();
	const int32 old_time_sec = fStatus.time_sec;
	if (fDriver.PrinterStatus(&fStatus) == B_OK) {
		if (fStatus.time_sec >= 0) {
			//printf("time = %d\n", fStatus.time_sec);
			if (old_time_sec == -1) {
				fStatusBar->Show();
				fStatusBar->SetMaxValue(fStatus.time_sec);
			} else {
				fStatusBar->Update((fStatusBar->MaxValue() - fStatus.time_sec) - fStatusBar->CurrentValue());
			}
		} else {
			if (old_time_sec != -1) {
				fStatusBar->Hide();
			}
		}
		for (int i=0 ; i<6 ; i++)
			fInkContainers[i]->SetInkLevel(fStatus.CMYKcmyk[i]);
	}
}

void ToolsPanel::GetPreferredSize(float *x, float *y)
{
	TPrintTools::GetPreferredSize(this, x, y);
	*x += BORDER;
	*y += BORDER;
}

// ----------------------------------------------------------------------
// #pragma mark -

InkContainer::InkContainer(const BPoint& pos, const rgb_color inkColor)
	: 	BView(BRect(pos,pos), NULL, B_FOLLOW_LEFT|B_FOLLOW_TOP, B_WILL_DRAW),
		fContainerBitmap(NULL),
		fContainerBitmapFull(NULL),
		fLevelBitmap(NULL),
		fColor(inkColor)
{
	GetLibbeResources()->GetBitmapResource('BBMP', 207, &fContainerBitmap);
	GetLibbeResources()->GetBitmapResource('BBMP', 208, &fContainerBitmapFull);

	ResizeTo(fContainerBitmap->Bounds().Width(), fContainerBitmap->Bounds().Height());
	
	fLevelBitmap = new BBitmap(fContainerBitmap->Bounds(), B_BITMAP_ACCEPTS_VIEWS, B_RGB32);
	fView = new BView(fLevelBitmap->Bounds(), NULL, B_FOLLOW_LEFT|B_FOLLOW_TOP, B_WILL_DRAW);
	fLevelBitmap->AddChild(fView);
}

void InkContainer::AttachedToWindow()
{
	SetViewColor(B_TRANSPARENT_32_BIT);
	SetInkLevel(255);
}

InkContainer::~InkContainer()
{
	delete fContainerBitmap;
	delete fContainerBitmapFull;
	delete fLevelBitmap;
}

void InkContainer::Draw(BRect frame)
{
	if (fLevelBitmap)
		DrawBitmap(fLevelBitmap);
}

void InkContainer::SetInkLevel(const int value)
{
	fLevelBitmap->Lock();
	BRect fillArea = fContainerBitmap->Bounds();
	fView->SetDrawingMode(B_OP_COPY);
	fView->SetHighColor(Parent()->ViewColor());
	fView->FillRect(fView->Bounds());
	if ((value < 0) || (value > 100))
	{ // disabled
		SetToolTipText("");
		fView->SetDrawingMode(B_OP_OVER);
		fView->DrawBitmap(fContainerBitmap);
		fView->SetHighColor(255,255,255,128);
		fView->SetDrawingMode(B_OP_ALPHA);

		fillArea.InsetBy(1,0);				fView->FillRect(fillArea);
		fillArea.InsetBy(-1,1);
		BRect t = fillArea;
		fillArea.right = fillArea.left;		fView->FillRect(fillArea);
		t.left = t.right;					fView->FillRect(t);
	}
	else
	{ // level
		SetToolTipText((BString() << " " << value << "% ").String());
		fView->SetDrawingMode(B_OP_OVER);
		fillArea.top = fillArea.bottom - (fillArea.Height() * 0.01f * value);
		fillArea.InsetBy(1,1);
		fView->DrawBitmap(fContainerBitmap);
		fView->DrawBitmap(fContainerBitmapFull, fillArea, fillArea);
		fView->SetDrawingMode(B_OP_ALPHA);
		fView->SetHighColor(fColor);
		fView->FillRect(fillArea);
		fView->SetHighColor(255,255,255,128);
		fillArea.bottom = fillArea.top;
		fView->FillRect(fillArea);
	}	
	fView->SetDrawingMode(B_OP_COPY);
	fView->Sync();
	fLevelBitmap->Unlock();
	
	if (LockLooper())
	{ // redraw the bitmap
		Invalidate();
		Window()->UpdateIfNeeded();
		UnlockLooper();
	}
}

// ----------------------------------------------------------------------
// #pragma mark -

OldApiPanel::OldApiPanel(BPrintPanel& panel)
	:	BPrintConfigView(BRect(0,0,0,0), mGetString(28), B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW),
		fPanel(panel)
{
	fSettings = fPanel.Settings();
}

void OldApiPanel::AttachedToWindow()
{
	BPrintConfigView::AttachedToWindow();
	SetViewColor(Parent()->ViewColor());

	BControl *control;
	BBox *box;
	BRect r;

	r.Set(BORDER, BORDER, 0,0);
	box = new BBox(r);
	box->SetLabel(mGetString(29));
	AddChild(box);

		r.OffsetTo(B_ORIGIN);
		r.InsetBy(BORDER, BORDER);
		r.top += BORDER*2;
		control = new BButton(r, NULL, mGetString(30), new BMessage('page'));
		box->AddChild(control);
		control->ResizeToPreferred();
		control->SetTarget(this);
		if (fPanel.PanelAction() == BPrintPanel::B_CONFIG_PAGE)
			control->MakeFocus();
		
		r.top = control->Frame().bottom + BORDER/4;
		control = new BButton(r, NULL, mGetString(31), new BMessage('jobs'));
		box->AddChild(control);
		control->ResizeToPreferred();
		control->SetTarget(this);
		if (fPanel.PanelAction() == BPrintPanel::B_CONFIG_JOB)
			control->MakeFocus();

	// Resize the box
	float x, y;
	TPrintTools::GetPreferredSize(box, &x, &y);
	box->ResizeTo(250, y*2+BORDER);
	ResizeToPreferred();

}

status_t OldApiPanel::Save(BMessage& settings)
{
	settings = fSettings;
	return B_OK;
}

void OldApiPanel::MessageReceived(BMessage *message)
{
	switch (message->what)
	{
		case 'page':
			fPanel.config_page(fSettings);
			break;
		case 'jobs':
			fPanel.config_job(fSettings);
			break;
	}
}

void OldApiPanel::GetPreferredSize(float *x, float *y)
{
	TPrintTools::GetPreferredSize(this, x, y);
	*x += BORDER;
	*y += BORDER;
}

