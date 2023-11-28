// ***********************************************************************
// libbe.so
//
// (c) 2000, Be Incorporated
//
// Written by : Mathias Agopian
//
// ***********************************************************************

#include <stdio.h>

#include <Application.h>
#include <Path.h>
#include <Directory.h>
#include <Node.h>
#include <TranslationUtils.h>
#include <Bitmap.h>
#include <Roster.h>
#include <pr_server.h>

#include "PrintPanelWindow.h"
#include "PrintWindows.h"
#include "PrintEnv.h"
#include "BMPView.h"

// For bitmap
#include "AboutBox.h"

#define mGetString(_x)	(TPrintTools::RsrcGetString(_x))

PrintPanelWindow::PrintPanelWindow(BPrintPanel *panel)
			: 	BWindow(BRect(0,0,0,0), mGetString(52), B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS),
			fPanel(panel),
			fTabView(NULL),
			fAsyncSem(-1)
{
	if (fPanel->Flags() & BPrintPanel::B_MODAL_PANEL)
		SetFeel(B_MODAL_APP_WINDOW_FEEL);

	// Add the background view
	BRect r = Bounds();
	BView *b = new BView(r, B_PRINT_PANEL_BACKGROUND, B_FOLLOW_ALL_SIDES, 0);
	b->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));		
	AddChild(b);
	
	//BBitmap *bitmap = BTranslationUtils::GetBitmap("/boot/home/documents/graphismes/Textures/Texture_miscrock_009.jpg");
	//b->SetFlags(b->Flags() | B_DRAW_ON_CHILDREN);
	//b->SetViewColor(B_TRANSPARENT_32_BIT);
	//b->SetViewBitmap(bitmap);
	//delete bitmap;

	// Add the ControlView (load/save buttons)
	r = Bounds();
	fControlView = new ControlView;
	fControlView->MoveTo(r.right - (fControlView->Frame().Width()+1.0f), r.bottom - (fControlView->Bounds().Height()+1.0f));
	b->AddChild(fControlView);

	// Add the printer info box
	fPrinterBox = new PrinterBox(fPanel->PanelAction() == BPrintPanel::B_CONFIG_PRINTER);
	b->AddChild(fPrinterBox);
	UpdatePrinterList(); // Need this before ResizeToPreferred()
	fPrinterBox->ResizeToPreferred();
	fPrinterBox->MoveTo(BORDER*4, BORDER*3);

	// And add the tab view
	r.top = (fPrinterBox->Frame().bottom + BORDER*2);
	r.bottom -= (fControlView->Bounds().Height()+1.0f);
	r.InsetBy(BORDER, BORDER);
	b->AddChild(fTabView = new PrintTabView(r));

	// Resize and Center the window
	update_size();
}

void PrintPanelWindow::update_size()
{
	BRect r(0,0,0,0);
	BRect f;

	float x, y;
	fTabView->GetPreferredSize(&x, &y);
	f = fTabView->Frame();
	f.right = f.left + x;
	f.bottom = f.top + y + BORDER;
	if (f.Width() < 6*BORDER) {
		fPrinterBox->GetPreferredSize(&x, &y);
		f.right = f.left + x + 6*BORDER;
	}
	r = r | f;
	
	// the PrintBox width must not excess the TabView width
	BRect t = fPrinterBox->Frame();
	t.right = f.right - 6*BORDER;
	f.Set(0,0,t.Width(),t.Height());
	f += fPrinterBox->Frame().LeftTop();
	r = r | f;

	f = r;
	f.bottom += fControlView->Frame().Height()+1.0f;
	f.right += BORDER;
	r = r | f;

	if (IsHidden() || ((r.Width() > Frame().Width()) || (r.Height() > Frame().Height())))
	{ // If the window is hidden or, needs to be resized UP, do it!
		fPrinterBox->ResizeTo(t.Width(), t.Height());
		ResizeTo(r.Width(), r.Height());
		fControlView->MoveTo(Bounds().right - (fControlView->Frame().Width()+1.0f), fTabView->Frame().bottom+1.0f);
	}

	if (IsHidden())
	{ // Center the window only if it's hidden
		BRect screenFrame = BScreen(B_MAIN_SCREEN_ID).Frame();
		MoveTo((screenFrame.Width()- Frame().Width())*0.5f, (screenFrame.Height() - Frame().Height())*0.5f);
	}

	// Allways set window size limits
	float minWidth, maxWidth, minHeight, maxHeight;
	GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);
	SetSizeLimits(r.Width()+1.0f, maxWidth, r.Height()+1.0f, maxHeight);
}

PrintTabView *PrintPanelWindow::TabView() const
{
	return fTabView;
}

void PrintPanelWindow::MessageReceived(BMessage *message)
{
	switch (message->what)
	{
		case MSG_OK:
			save();
			break;

		case MSG_CANCEL:
			cancel();
			break;

		case MSG_ADD_PRINTER:
			run_add_printer_panel();
			break;
		
		case MSG_PRINTER_PREFS:
			run_select_printer_panel();
			break;

		case MSG_PRINTER_SELECTED:
		{ // A new printer has been selected. Let's switch to this one.
			BMenuItem *source;
			if (message->FindPointer("source", (void **)&source) == B_OK)
				fPanel->set_printer(source->Label());
			break;
		}

		default:
			BWindow::MessageReceived(message);
	}
}

bool PrintPanelWindow::QuitRequested()
{
	cancel();
	if (IsHidden() == false)
		Hide();	// Hide the window
	return false;
}
		
void PrintPanelWindow::save()
{
	status_t result = fPanel->save();
	if (result != B_OK)
		return;
	fReturnValue = result;
	if (fAsyncSem != -1)
		delete_sem(fAsyncSem);
}

void PrintPanelWindow::cancel()
{
	fPanel->cancel();	// Cancel cannot fail!
	fReturnValue = B_CANCELED;
	if (fAsyncSem != -1)
		delete_sem(fAsyncSem);
}
		
		
status_t PrintPanelWindow::Go()
{ // We need this for compatibility with BPrintJob

	BWindow *window = dynamic_cast<BWindow *>(BLooper::LooperForThread(find_thread(NULL)));
	fAsyncSem = create_sem(0, "async");			
	if (IsHidden() == true)
		Show();	// Dislay the window
	
	status_t err;
	if (window) { // A window is being blocked. We'll keep the window updated by calling UpdateIfNeeded.
		while (true)  {
			while ((err = acquire_sem_etc(fAsyncSem, 1, B_TIMEOUT, 50000)) == B_INTERRUPTED) { /* Nothing to do */ }
			if (err == B_BAD_SEM_ID) break;
			window->UpdateIfNeeded();
		}
	} else { while ((err = acquire_sem(fAsyncSem)) == B_INTERRUPTED) { /* Nothing to do */ } }
	return fReturnValue;
}

void PrintPanelWindow::UpdatePrinterList()
{
	fPrinterBox->UpdatePrinterList();
}

status_t PrintPanelWindow::Select(const char *name)
{
	if (name == NULL)
		return B_BAD_VALUE;
	return fPrinterBox->Select(name);
}

// --------------------------------------------------------------------
#pragma mark -

PrintTabView::PrintTabView(BRect r)
	: BTabView(r, B_PRINT_PANEL_TABVIEW, B_WIDTH_FROM_WIDEST, B_FOLLOW_ALL,
				B_WILL_DRAW | B_NAVIGABLE_JUMP |
				B_FRAME_EVENTS | B_NAVIGABLE)
{
	// HACK: that's not very pretty but BTabView is naze
 	fContainer = FindView("view container");
}

PrintTabView::~PrintTabView()
{
}

void PrintTabView::AddTab(BView *target, BTab *tab)
{
	if (TabAt(0))
		target->Hide();
	BString name = "be:";
	name << target->Name();
	CView *view = new CView(BRect(-16,-16,-8,-8), name.String(), B_FOLLOW_LEFT|B_FOLLOW_TOP, 0);
	view->fTabView = target;
	BTabView::AddTab(view, tab);
	tab->SetLabel(target->Name());
	if (fContainer)
		fContainer->AddChild(target);
}

BTab *PrintTabView::RemoveTab(int32 tab_index)
{
	BView *view = static_cast<CView *>(TabAt(tab_index)->View())->fTabView;
	view->RemoveSelf();
	delete view;
	return BTabView::RemoveTab(tab_index);
}

void PrintTabView::GetPreferredSize(float *x, float *y)
{
	const BRect bounds = Bounds();
	const BRect container_bounds = fContainer->Bounds();
	const float w = bounds.Width() - container_bounds.Width();
	const float h = bounds.Height() - container_bounds.Height();
	float dw, dh;
	TPrintTools::GetPreferredSize(fContainer, &dw, &dh);
	*x = w + dw;
	*y = h + dh;
}

void PrintTabView::Select(int32 tab)
{
	if (tab == Selection())
		return;
	BView *view = static_cast<CView *>(TabAt(Selection())->View())->fTabView;
	if (view->IsHidden(view) == false)
		view->Hide();
	BTabView::Select(tab);
	view = static_cast<CView *>(TabAt(Selection())->View())->fTabView;
	if (view->IsHidden(view) == true)
		view->Show();
}


// --------------------------------------------------------------------
#pragma mark -

PrinterBox::PrinterBox(bool configPrinter)
	: 	BView(BRect(0,0,0,0), "be:printerbox", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_WILL_DRAW | B_DRAW_ON_CHILDREN),
		fPrinterWatcher(NULL),
		fPrinterBitmap(NULL)
{
	#ifdef B_BEOS_VERSION_DANO
	SetDoubleBuffering(B_UPDATE_INVALIDATED | B_UPDATE_RESIZED);
	#endif

	fPrinterMenu = new BPopUpMenu(mGetString(53));
	fPrinterMenuField = new BMenuField(BRect(0,0,0,0), NULL, NULL, fPrinterMenu);
	AddChild(fPrinterMenuField);
	fPrinterMenuField->ResizeToPreferred();
	fPrinterMenuField->SetDivider(0);

	const float divider = 80.0f;  // TODO: should be more clever here
	const int v1 = 2;
	const int v2 = 2+3;
	BRect frame(0, fPrinterMenuField->Frame().bottom + v1, 0, 0);
	BStringView *v;
	v = new BStringView(frame, NULL, mGetString(54));
	v->SetFont(be_bold_font);
	v->ResizeToPreferred();
	AddChild(v);
	frame.OffsetBy(divider, v1);
	(fPrinterStatus = new BStringView(frame, NULL, B_EMPTY_STRING, B_TRUNCATE_MIDDLE, B_FOLLOW_LEFT|B_FOLLOW_RIGHT))->ResizeToPreferred();
	AddChild(fPrinterStatus);
	frame.OffsetBy(-divider, v->Frame().Height()-v2);

	v = new BStringView(frame, NULL, mGetString(55));
	v->SetFont(be_bold_font);
	v->ResizeToPreferred();
	AddChild(v);
	frame.OffsetBy(divider, v1);
	(fPrinterDriver = new BStringView(frame, NULL, B_EMPTY_STRING, B_TRUNCATE_MIDDLE, B_FOLLOW_LEFT|B_FOLLOW_RIGHT))->ResizeToPreferred();
	AddChild(fPrinterDriver);
	frame.OffsetBy(-divider, v->Frame().Height()-v2);

	v = new BStringView(frame, NULL, mGetString(56));
	v->SetFont(be_bold_font);
	v->ResizeToPreferred();
	AddChild(v);
	frame.OffsetBy(divider, v1);
	(fPrinterTransport = new BStringView(frame, NULL, B_EMPTY_STRING, B_TRUNCATE_MIDDLE, B_FOLLOW_LEFT|B_FOLLOW_RIGHT))->ResizeToPreferred();
	AddChild(fPrinterTransport);
	frame.OffsetBy(-divider, v->Frame().Height()-v2);

	v = new BStringView(frame, NULL, mGetString(57));
	v->SetFont(be_bold_font);
	v->ResizeToPreferred();
	AddChild(v);
	frame.OffsetBy(divider, v1);
	(fPrinterComments = new BStringView(frame, NULL, B_EMPTY_STRING, B_TRUNCATE_MIDDLE, B_FOLLOW_LEFT|B_FOLLOW_RIGHT))->ResizeToPreferred();
	AddChild(fPrinterComments);

	if (configPrinter)
		fPrinterMenuField->SetEnabled(false);

	frame = Bounds();
	fPrinterBitmap = new BMPView(frame, NULL, B_FOLLOW_TOP | B_FOLLOW_RIGHT);
	AddChild(fPrinterBitmap);
	fPrinterBitmap->SetBitmap(206);
	fPrinterBitmap->ResizeToPreferred();
	fPrinterBitmap->MoveTo(frame.right - fPrinterBitmap->Bounds().Width() - 1, frame.top);
}

PrinterBox::~PrinterBox()
{
	delete fPrinterWatcher;
}

void PrinterBox::GetPreferredSize(float *w, float *h)
{
	fPrinterMenuField->ResizeToPreferred();
	BRect f0, f1, f2, f3;
	float ww,hh;
	fPrinterStatus->GetPreferredSize(&ww, &hh);		f0.Set(0,0,ww-1,hh-1); f0 += fPrinterStatus->Frame().LeftTop();
	fPrinterDriver->GetPreferredSize(&ww, &hh);		f1.Set(0,0,ww-1,hh-1); f1 += fPrinterDriver->Frame().LeftTop();
	fPrinterTransport->GetPreferredSize(&ww, &hh);	f2.Set(0,0,ww-1,hh-1); f2 += fPrinterTransport->Frame().LeftTop();
	fPrinterComments->GetPreferredSize(&ww, &hh);	f3.Set(0,0,ww-1,hh-1); f3 += fPrinterComments->Frame().LeftTop();
	BRect frame = fPrinterMenuField->Frame() | f0 | f1 | f2 | f3;
	*w = frame.Width() + fPrinterBitmap->Frame().Width() + 1.0f;
	*h = frame.Height() + 1.0f;
}

void PrinterBox::AttachedToWindow()
{
	SetColorsFromParent();
}

void PrinterBox::UpdatePrinterList()
{
	BMenuItem *it;
	while ((it = fPrinterMenu->ItemAt(0)))
		delete fPrinterMenu->RemoveItem((int32)0);
	BPath path;
	find_directory(B_USER_PRINTERS_DIRECTORY, &path);
	BDirectory dir(path.Path());
	BEntry entry;
	while (dir.GetNextEntry(&entry) == B_OK)
	{
		char buf[256];
		entry.GetName(buf);
		fPrinterMenu->AddItem(new BMenuItem(buf, new BMessage(PrintPanelWindow::MSG_PRINTER_SELECTED)));
	}

	// don't add the supplementary items if called from the pref printer panel!	
	app_info info;
	be_app->GetAppInfo(&info);
	if (strcmp(info.signature, PSRV_PRINT_PREFS_SIG)) {
		fPrinterMenu->AddItem(new BSeparatorItem());
		fPrinterMenu->AddItem(new NotMarkableMenuItem(mGetString(50), new BMessage(PrintPanelWindow::MSG_ADD_PRINTER)));
		fPrinterMenu->AddItem(new NotMarkableMenuItem(mGetString(51), new BMessage(PrintPanelWindow::MSG_PRINTER_PREFS)));
	}
}

status_t PrinterBox::Select(const char *name)
{
	delete fPrinterWatcher;
	fPrinterWatcher = new PrinterWatcher(this, name);
	UpdatePrinter(name);
	return B_OK;
}

void PrinterBox::UpdatePrinter(const char *name)
{
	BMenuItem *item = fPrinterMenu->FindItem(name);
	if (item == NULL)
		return;	
	item->SetMarked(true);

	char driver[256];
	char transport[256];
	char state[256];
	char comments[256];
	BPath path;
	find_directory(B_USER_PRINTERS_DIRECTORY, &path);
	path.Append(name);
	BNode node(path.Path());
	if (node.InitCheck() == B_OK)
	{
		if (node.ReadAttr(PSRV_PRINTER_ATTR_DRV_NAME,	B_STRING_TYPE, 0, driver, 255) <= 0)	driver[0] = 0;
		if (node.ReadAttr(PSRV_PRINTER_ATTR_TRANSPORT,	B_STRING_TYPE, 0, transport, 255) <= 0)	transport[0] = 0;
		if (node.ReadAttr(PSRV_PRINTER_ATTR_STATE,		B_STRING_TYPE, 0, state, 255) <= 0)		state[0] = 0;
		if (node.ReadAttr(PSRV_PRINTER_ATTR_COMMENTS,	B_STRING_TYPE, 0, comments, 255) <= 0)	comments[0] = 0;
	}

	if (!strcmp(state, PSRV_PRINTER_ATTR_FREE)) {
		fPrinterStatus->SetHighUIColor(B_UI_PANEL_TEXT_COLOR);
		fPrinterStatus->SetText(mGetString(58));
	} else if (!strcmp(state, PSRV_PRINTER_ATTR_BUSY)) {
		fPrinterStatus->SetHighUIColor(B_UI_PANEL_TEXT_COLOR);
		fPrinterStatus->SetText(mGetString(59));
	} else {
		fPrinterStatus->SetHighUIColor(B_UI_FAILURE_COLOR);
		fPrinterStatus->SetText(state);
	}
	fPrinterDriver->SetText(driver);
	fPrinterTransport->SetText(transport);
	fPrinterComments->SetText(comments);

	fPrinterMenuField->ResizeToPreferred();
	fPrinterStatus->ResizeToPreferred();
	fPrinterDriver->ResizeToPreferred();
	fPrinterTransport->ResizeToPreferred();
	fPrinterComments->ResizeToPreferred();
}

// ------------------------------------------------------------
#pragma mark -

PrinterBox::PrinterWatcher::PrinterWatcher(PrinterBox *view, const char *name)
	: 	NodeWatcher(view->Window()),
		fPanel(view),
		fCurrentPrinter(name)
{
	BPath path;
	find_directory(B_USER_PRINTERS_DIRECTORY, &path);
	path.Append(name);
	BNode node(path.Path());
	node.GetNodeRef(&fWatchNodeRef);
	StartWatching(fWatchNodeRef, B_WATCH_ATTR);
};

PrinterBox::PrinterWatcher::~PrinterWatcher()
{
	StopWatching(fWatchNodeRef);
}

void PrinterBox::PrinterWatcher::AttributeChanged(node_ref& nref)
{
	if (fPanel->LockLooper()) {
		fPanel->UpdatePrinter(fCurrentPrinter.String());
		fPanel->UnlockLooper();
	}
}

