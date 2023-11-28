#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Alert.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <OS.h>
#include <Path.h>
#include <PrintJob.h>
#include <Roster.h>
#include <Screen.h>
#include <NodeMonitor.h>
#include <NodeInfo.h>
#include <Mime.h>
#include <Bitmap.h>
#include <ListItem.h>
#include <AppPrefs.h>
#include <InterfaceDefs.h>
#include <PrintEnv.h>
#include <TranslationUtils.h>
#include <print/PrintJobSettings.h>
#include <print/PrintPanel.h>


// Needed for the signatures of print related stuffs
#include <pr_server.h>

#include "AddPrinter.h"
#include "PrefPanel.h"

#define kWindowWidth	483
#define kWindowHeight	404
#define kButtonWidth	80

extern const char *RsrcGetString(int index);

#define 	kBtnCancelJob		100
#define 	kBtnDeleteJob		101
#define 	kBtnRestartJob		102
#define 	kLabelJobNoPrinter	103
#define 	kLabelJobPrinter	104

// ************************************************************************

TWindow::TWindow(BPath path)
	:	BWindow(	BRect(0, 0, kWindowWidth, kWindowHeight), RsrcGetString(150),
					B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_NOT_H_RESIZABLE),
		fAddPrinterWindow(NULL),
		fPrintPanel(NULL)
{
	// Set window size limits
	float minWidth, maxWidth, minHeight, maxHeight;
	GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);
	SetSizeLimits(kWindowWidth, kWindowWidth, kWindowHeight, maxHeight);
	
	// nicer background view
	BRect r(Bounds());
	r.InsetBy(-1, -1);
	AddChild(fBG = new TBackgroundBox(r));

	//	add printer list
	r.left += 13;
	r.right -= 11;
	r.top += 13;
	r.bottom = r.top + 215;
	fPrinterBox = new TPrinterBox(path, r);
	
	// add job list
	r.top = r.bottom + 18;
	r.bottom = r.top + 149;
	fJobBox = new TJobBox(r);

	// Get/Create the printer directory
	find_directory(B_USER_PRINTERS_DIRECTORY, &fPrinterDirectory);
	mkdir(fPrinterDirectory.Path(), 0777);

	// Get the default printer
	GetCurrentPrinter();

	// Add various panels
	fBG->AddChild(fPrinterBox);
	fBG->AddChild(fJobBox);

	// Get preferrences (must be called AFTER AddChild())
	GetPrefs();

	// Select the default printer
	fPrinterBox->Select(fCurPrinterName.String());
}

TWindow::~TWindow()
{
	delete fPrintPanel;
}

void TWindow::MessageReceived(BMessage* m)
{
	switch (m->what)
	{
		case B_NODE_MONITOR:
			// This message happen when either the printer folder is modified
			// (B_WATCH_DIRECTORY), or when a printer is modified (in this case
			// it comes from a TPrinterItem::Watcher()) (attributes or entries).
			fPrinterBox->NodeEvent(m);
			break;
			
		case JOB_CREATED:
		case JOB_DESTROYED:
		case JOB_MODIFIED:
		case JOB_COUNT_MODIFIED:
		case PRINTER_MODIFIED:
		case FAILED_CHANGED:
			// Theses messages comes from a Watcher()
			fPrinterBox->WatcherEvent(m);
			break;
	
		case msg_single_click:
			// Enable buttons
			fPrinterBox->UpdateControls();
			fJobBox->UpdateControls();
			break;
			
		case msg_double_click:
		case msg_set_default:
			{ // This has the same functionality as SelectPrinter did (send message)
				const TPrinterItem *it = fPrinterBox->SelectedPrinter();
				if (it)
					SetCurrentPrinter(it->Name());
			}
			break;
				
		case msg_add_printer:
			// Open the add printer panel
			if ((fAddPrinterWindow) && (fAddPrinterWindow->Lock()))
			{ // The window already exists and is already opened (using a BMessenger would be better)
				fAddPrinterWindow->Unlock();
				break;
			}			
			(fAddPrinterWindow = new TAddPrinterWindow(this))->Show();
			break;
		
		case msg_remove_printer:	
			// Remove the currently selected printer		
			fPrinterBox->RemovePrinter();
			break;
			
		case msg_setup_printer:
			// default settings for this printer
			SetupPrinter();
			break;
		case B_CANCEL:
			delete fPrintPanel;
			fPrintPanel = NULL;
			break;
		case BPrintPanel::B_SETUP_PRINTER:
			if (fPrintPanel) {
				BPrintJobSettings s(fPrintPanel->Settings());
				TPrintTools::SaveSettings(s.PrinterName().String(), s.Message(), NULL);
				delete fPrintPanel;
				fPrintPanel = NULL;
			}
			break;
		
		case msg_probe_printers:
			// Scan for connected printers
			TPrintTools::AddPrinters();
			break;
			
		case msg_abort_job:
			fJobBox->AbortJob();
			break;

		case msg_ask_abort_job:
			fJobBox->AskAbortJob();
			break;

		case msg_restart_job:
			fJobBox->RestartJob();
			break;

		case msg_config_printer:
			configure_add_on(m);
			break;

		default:
			BWindow::MessageReceived(m);
	}
}

// -------------------------------------------------------------------

void TWindow::GetPrefs()
{
	BAppPrefs settings;
	BRect frame;
	if (	(settings.InitCheck() == B_OK) &&
			(settings.Load() == B_OK) &&
			(settings.FindRect("frame", &frame) == B_OK))
	{
		ResizeTo(frame.Width(), frame.Height());
		if (BScreen(B_MAIN_SCREEN_ID).Frame().Contains(frame.LeftTop()))
		{
			MoveTo(frame.LeftTop());
			return;
		}
	}

	// 	if prefs dont yet exist or the window is not onscreen, center the window
	BRect screenFrame = (BScreen(B_MAIN_SCREEN_ID).Frame());
	BPoint pt;
	pt.x = screenFrame.Width()/2 - Bounds().Width()/2;
	pt.y = screenFrame.Height()/2 - Bounds().Height()/2;
	if (screenFrame.Contains(pt))
		MoveTo(pt);
}

void TWindow::SetPrefs()
{
	BAppPrefs settings;
	if (settings.InitCheck() == B_OK)
	{
		settings.AddRect("frame", Frame());
		settings.Save();
	}
}

// -------------------------------------------------------------------

bool TWindow::QuitRequested()
{
	// Stop watching the settings file
	SetPrefs();
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

void TWindow::SetJobEntry(const BEntry *entry)
{
	fJobBox->SetEntry(entry);
}

void TWindow::JobEvent(BMessage *m)
{
	fJobBox->WatcherEvent(m);
}

void TWindow::EmptyJobList()
{
	fJobBox->EmptyList();
}

status_t TWindow::GetCurrentPrinter(void)
{
	const char *kDefaultPrinter = "be:Default printer";
	BAppPrefs settings("printer_data", PSRV_PRINT_SERVER_SIG);
	status_t result;
	if ((result = settings.InitCheck()) != B_OK)
		return result;
	if ((result = settings.Load()) != B_OK)
		return result;
	fCurPrinterName = settings.FindString(kDefaultPrinter);
	return B_OK;
}

status_t TWindow::SetCurrentPrinter(const char *name)
{
	status_t result;
	if ((result = set_default_printer(name)) != B_OK)
		return result;
	fCurPrinterName = name;
	return B_OK;
}

const char* TWindow::CurrentPrinter()
{
	return fCurPrinterName.String();
}



status_t TWindow::configure_add_on(BMessage *msg)
{
	// extract info from the message...
	const char	*printerName	= msg->FindString("printer name");
	const char	*driverName		= msg->FindString("driver");
	const char	*transportName	= msg->FindString("transport");
	const char	*device			= msg->FindString("transport path");
	const char	*model			= msg->FindString("model");
	const int32 attributes 		= msg->FindInt32("attributes");

	status_t err;
	if ((err = TPrintTools::CreatePrinter(printerName, model, transportName, device, driverName, false, attributes)) == B_OK)
	{
		if (strlen(CurrentPrinter()) > 0)
		{
			char str[1024];
			const char *fmt = RsrcGetString(105);
			sprintf(str, fmt, printerName);
			if ((new BAlert(NULL, str, RsrcGetString(107), RsrcGetString(106)))->Go() == 1)
				SetCurrentPrinter(printerName);
		}
		else
		{
			// the new printer is the ONLY printer, so make it
			// the default!
			SetCurrentPrinter(printerName);
		}
		return B_OK;
	}
	else
	{
		if (err == B_CANCELED)
			return B_CANCELED;	// B_CANCELLED is not really an error
		char str[1024];
		const char *fmt = RsrcGetString(110);
		sprintf(str, fmt, strerror(err));
		(new BAlert(NULL, str, RsrcGetString(109), NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
	}
	return err;
}



// ************************************************************************
// #pragma mark -

TPrinterItem::TPrinterItem(BLooper *looper, const char *path)
	:	BListItem(),
		fIcon(NULL),
		fDriverName(NULL),
		fTransport(NULL),
		fComments(NULL),
		fName(NULL),
		fWatcher(NULL),
		fJobCount(0),
		fIsChecked(false),
		fFailedJobs(false)
{
	BEntry entry(path);
	entry.GetNodeRef(&fRef);
	fWatcher = new TStdWatcher(path, looper);
}

TPrinterItem::~TPrinterItem()
{
	// Acutally ANSI C, says We can free the NULL pointer
	free(fName);
	free(fDriverName);
	free(fTransport);
	free(fComments);
	delete fWatcher;
	delete fIcon;
}

void TPrinterItem::SetIcon(const BBitmap *icon)
{
	delete fIcon;		
	fIcon = icon;	// the object get ownership of the icon.
}

void TPrinterItem::SetJobCount(int32 job_count)
{
	fJobCount = job_count;
}

void TPrinterItem::SetFailed(bool f)
{
	if (fFailedJobs == f)
		return;
	fNeedsUpdate = true;
	fFailedJobs = f;
}

void TPrinterItem::SetInfos(	const char *name,
								const char *driver,
								const char *transport,
								const char *comments,
								bool is_checked,
								bool *need_update,
								bool *need_icon)
{
	*need_update = false;
	*need_icon = false;

	if (!fName || (strcmp(fName, name) != 0)) {
		free(fName);
		fName = strdup(name);
		*need_update = true;
	}
	if (!fDriverName || (strcmp(fDriverName, driver) != 0)) {
		free(fDriverName);
		fDriverName = strdup(driver);
		*need_update = true;
	}
	if (!fTransport || (strcmp(fTransport, transport) != 0)) {
		free(fTransport);
		fTransport = strdup(transport);
		*need_update = true;
	}
	if (!fComments || (strcmp(fComments, comments) != 0)) {
		free(fComments);
		fComments = strdup(comments);
		*need_update = true;
	}

	if ((!fIcon) || (is_checked != fIsChecked)) {
		fIsChecked = is_checked;
		*need_update = true;
		*need_icon = true;
	}
	
	// Updates that don't depend of file's attributes
	if (fNeedsUpdate)
	{
		*need_update = true;
		*need_icon = true;
		fNeedsUpdate = true;
	}
}

void TPrinterItem::DrawItem(BView *owner, BRect bounds, bool complete)
{
	char		job_buffer[4*128+32];
	char		transport_buffer[256];
	char		buffer[4*128+32];
	char		*t_buffer[5];
	char		*t_source[5];
	float		width, width2, width3;
	BFont		font;
	BPoint 		where;
	
	// Create the string describing the job count.
	if (fJobCount == 0)			strcpy(job_buffer, RsrcGetString(111));
	else if (fJobCount == 1)	strcpy(job_buffer, RsrcGetString(112));
	else						sprintf(job_buffer,RsrcGetString(123), fJobCount);

	// Create the string describing the transport
	sprintf(transport_buffer, "%s", fTransport);
	
	// Calculate the width of the various columns.
	width = Width()-2.0;
	width3 = floor((width-52.0)*0.31);
	width2 = width-(52.0+width3);
	
	// Truncate the strings to fit the space available.
	owner->GetFont(&font);
	t_buffer[0] = buffer+0;
	t_buffer[1] = buffer+128;
	t_buffer[2] = buffer+256;
	t_buffer[3] = buffer+384;	
	t_buffer[4] = buffer+512;

	t_source[0] = fName;
	t_source[1] = fDriverName;
	t_source[2] = fComments;
	t_source[3] = job_buffer;	
	t_source[4] = transport_buffer;

	font.GetTruncatedStrings((const char**)t_source, 2, B_TRUNCATE_END, width2-5.0f, t_buffer);
	font.GetTruncatedStrings((const char**)(t_source+2), 1, B_TRUNCATE_END, width2+width3, t_buffer+2);
	font.GetTruncatedStrings((const char**)(t_source+3), 2, B_TRUNCATE_END, width3, t_buffer+3);
	
	// We want to restore the initial state of the owner when leaving.
	owner->PushState();
	
	if (IsSelected()) {
		owner->SetLowColor(ui_color(B_MENU_SELECTED_BACKGROUND_COLOR));
		owner->SetHighColor(ui_color(B_MENU_SELECTED_ITEM_TEXT_COLOR));
		complete = true;
	} else {
		owner->SetLowColor(owner->ViewColor());
		owner->SetHighColor(ui_color(B_MENU_ITEM_TEXT_COLOR));
	}

	if (complete)
		owner->FillRect(bounds, B_SOLID_LOW);
	
	// Draw the icon on the left side
	owner->SetDrawingMode(B_OP_OVER);
	owner->DrawBitmap(fIcon, bounds.LeftTop() + BPoint(2.0f, 1.0f));
	owner->SetDrawingMode(B_OP_COPY);
	
	// Draw the text infos.
	where.x = bounds.left + 52.0f;
	where.y = bounds.top + fv0;
	owner->DrawString(t_buffer[0], where);
		
	where.y += fdv;
	owner->DrawString(t_buffer[1], where);
		
	where.y += fdv;
	owner->DrawString(t_buffer[2], where);
		
	where.x = bounds.left + 52.0f + width2;
	where.y = bounds.top + fv0;
	owner->DrawString(t_buffer[3], where);

	where.y += fdv;
	owner->DrawString(t_buffer[4], where);

	// Put the owner back in its initial state.
	owner->PopState();
}

void TPrinterItem::SetIcon(bool local)
{
	// Redraw the icon
	const BRect frame(B_ORIGIN, BPoint(47.0f, 47.0f));
	const BRect large(B_ORIGIN, BPoint(B_LARGE_ICON-1, B_LARGE_ICON-1));
	
	BBitmap *drawing	= new BBitmap(frame, B_COLOR_8_BIT, true);
	BBitmap *source		= new BBitmap(large, B_COLOR_8_BIT);
	BView *drawer		= new BView(frame, NULL, B_FOLLOW_NONE, 0);
	drawing->AddChild(drawer);
	drawing->Lock();

	// Erase the background of the icon with the transparent color
	drawer->SetDrawingMode(B_OP_COPY);
	drawer->SetHighColor(B_TRANSPARENT_32_BIT);
	drawer->FillRect(frame);
	drawer->SetDrawingMode(B_OP_OVER);

	{
		BBitmap *s = BTranslationUtils::GetBitmap((local) ? ("kLocalBack") : ("kNetworkBack"));
		drawer->DrawBitmap(s, BPoint(16, 0));
		delete s;
	}

	// Draw the printer specific icon. For now, just use the standard icon.
	BMimeType mime_type(PSRV_PRINTER_MIMETYPE);
	mime_type.GetIcon(source, B_LARGE_ICON);
	drawer->DrawBitmap(source, BPoint(0, 16));	
	
	// Add the checkmak if this is the default printer.
	if (fIsChecked)
	{
		BBitmap *s = BTranslationUtils::GetBitmap("checkmark_icon");
		drawer->DrawBitmap(s, BPoint(0, 16));
		delete s;
	}
	
	if (fFailedJobs)
	{
		BBitmap *s = BTranslationUtils::GetBitmap("attention_icon");
		drawer->DrawBitmap(s, BPoint(20, 31));
		delete s;
	}

	// Create an non drawing copie of the resulting bitmap. 
	drawer->Sync();
	BBitmap *icon = new BBitmap(frame, B_COLOR_8_BIT);
	memcpy(icon->Bits(), drawing->Bits(), icon->BitsLength());
	SetIcon(icon);	// Set the icon -> SetIcon() gets the ownership of the icon

	drawing->Unlock();	
	delete drawing;
	delete source;
}

void TPrinterItem::Update(BView *owner, const BFont *font)
{
	float		height;
	font_height finfo;

	// Check the vertical metric of the font.
	font->GetHeight(&finfo);
	fv0 = ceil(finfo.ascent) + 1.0;
	fdv = ceil(finfo.ascent) + ceil(finfo.descent) + ceil(finfo.leading);

	// Calculate the minimal Height of a cell and set it.
	height = fdv*3.0+1.0;
	if (height < 50.0) height = 50.0;
	fdv0 = floor((height - (fdv*3.0+1.0))*0.5);
	fv0 += fdv0;

	SetHeight(height);
	SetWidth(owner->Bounds().Width());
}

// ************************************************************************

static status_t FSDeleteFolder(BEntry* dir_entry, bool delete_top_dir)
{
	entry_ref	ref;
	BEntry		entry;
	BDirectory	dir;
	status_t	err;

	dir.SetTo(dir_entry);
	dir.Rewind();

	// loop through everything in folder and delete it, skipping trouble files
	while (true)
	{
		if (dir.GetNextEntry(&entry) != B_NO_ERROR)
			break;
		entry.GetRef(&ref);
		if (entry.IsDirectory())
			err = FSDeleteFolder(&entry, delete_top_dir);
		else
			err = entry.Remove();
		if (err == B_NO_ERROR)
			dir.Rewind();
	}

	dir_entry->GetRef(&ref);

	if (delete_top_dir)
		return dir_entry->Remove();
	return B_NO_ERROR;
}

// -----------------------------------------------------------------------
// #pragma mark -

TPrinterBox::TPrinterBox(BPath path, BRect frame)
	:	 BBox(frame, "box", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP),
		fLastSelectedPrinter(NULL)
{	
	// Initialise internal variables
	fPath = path;
	BEntry dir(fPath.Path());
	dir.GetNodeRef(&fRef);

	//	listview
	BRect r(14, 28,
			Bounds().Width()-kButtonWidth-20-B_V_SCROLL_BAR_WIDTH-3,
			Bounds().Height()-10);
			
	fPrinterList = new BListView(r, "printer_list", B_SINGLE_SELECTION_LIST, B_FOLLOW_TOP_BOTTOM);
	fPrinterList->SetSelectionMessage(new BMessage(msg_single_click));
	fPrinterList->SetInvocationMessage(new BMessage(msg_double_click));

	//	scroller
	AddChild(fListScroller = new BScrollView("scroll", fPrinterList, B_FOLLOW_ALL, B_FRAME_EVENTS | B_WILL_DRAW, false, true));
	
	// 	add btn
	r.Set(Bounds().Width()-kButtonWidth-11, 28, Bounds().Width()-11, 28);
	AddChild(fAddBtn = new BButton(r, "bt:add", RsrcGetString(114), new BMessage(msg_add_printer)));

	//	delete btn
	r.top = fAddBtn->Frame().bottom + 9;
	r.bottom = r.top + 16;
	AddChild(fRemoveBtn = new BButton(r, "bt:remove", RsrcGetString(115), new BMessage(msg_remove_printer)));

	//	make default btn
	r.top = fRemoveBtn->Frame().bottom + 9;
	r.bottom = r.top + 16;
	AddChild(fSetDefaultBtn = new BButton(r, "bt:default", RsrcGetString(116), new BMessage(msg_set_default)));

	//	setup btn
	r.top = fSetDefaultBtn->Frame().bottom + 9;
	r.bottom = r.top + 16;
	fSetupBtn = new BButton(r, "bt:setup", RsrcGetString(118), new BMessage(msg_setup_printer));
	AddChild(fSetupBtn);

	//	probe btn
	r.top = fSetupBtn->Frame().bottom + 9 + 16 + 9;
	r.bottom = r.top + 16;
	AddChild(fProbeBtn = new BButton(r, "bt:probe", RsrcGetString(117),	new BMessage(msg_probe_printers)));
	
	// set label
	SetLabel(RsrcGetString(119));
}

TPrinterBox::~TPrinterBox()
{
	// Stop the node monitor (if any)
	watch_node(&fRef, B_STOP_WATCHING, fWindow);
	
	// Free the printer list. It is important to free the BListItem because they allocate resources
	while (fPrinterList->IsEmpty() == false)
		delete fPrinterList->RemoveItem((int32)0);
}

void TPrinterBox::AttachedToWindow()
{
	BBox::AttachedToWindow();

	// We need a pointer on our window ASAP
	fWindow = dynamic_cast<TWindow*>(Window());

	// Start the node monitor
	watch_node(&fRef, B_WATCH_DIRECTORY, fWindow);
	
	// Populate the list a first time
	BDirectory dir(&fRef);
	BEntry entry;
	dir.Rewind();	
	while (dir.GetNextEntry(&entry) == B_OK)
		UpdateItem(NULL, &entry);		
	Select(fWindow->CurrentPrinter());
	UpdateControls();
}

bool TPrinterBox::RemovePrinter()
{
	const TPrinterItem *it = SelectedPrinter();
	if (it == NULL)
		return false;
	BPath printerPath(fPath);
	printerPath.Append(it->Name());
	return RemovePrinter(printerPath.Path());
}

bool TPrinterBox::RemovePrinter(const char *path)
{
	if (path == NULL)
		return false;
	
	// Check if the queue is empty, and alert the user if required.
	BDirectory dir(path);
	if (dir.CountEntries() > 0)
	{
		BAlert *a = new BAlert(NULL, RsrcGetString(120),
			RsrcGetString(121), RsrcGetString(109), NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		if (a->Go() == 1)
			return false;
	}
	BEntry entry(path);
	FSDeleteFolder(&entry, true);
	return true;
}

void TWindow::SetupPrinter()
{
	const TPrinterItem *it = fPrinterBox->SelectedPrinter();
	if (it == NULL) return;
	if (fPrintPanel == NULL) {
		BMessenger m(this);
		fPrintPanel = new TPrintPanel(this, BPrintPanel::B_CONFIG_PAGE, NULL, &m, NULL, 0);
		fPrintPanel->Window()->SetFeel(B_MODAL_APP_WINDOW_FEEL);
		if (fPrintPanel->SetPrinter(it->Name()) != B_OK) {
			delete fPrintPanel;
			fPrintPanel = NULL;
			return;
		}
	}	
	fPrintPanel->Show();
}

status_t TPrinterBox::NodeEvent(BMessage *m)
{
	int32 opcode;
	if (m->FindInt32("opcode", &opcode) != B_OK)
		return B_ERROR;

	entry_ref eref;
	const char *name;
	m->FindString("name", &name);
	m->FindInt32("device", &eref.device);
	m->FindInt64("directory", &eref.directory);
	eref.set_name(name);

	node_ref ref;
	m->FindInt32("device", &ref.device);
	m->FindInt64("node", &ref.node);

	if (opcode == B_ENTRY_MOVED)
	{ // figure out if we must add to or remove from the node
		node_ref parent;
		m->FindInt32("device", &parent.device);
		m->FindInt64("to directory", &parent.node);
		opcode = B_ENTRY_REMOVED;
		if (parent == fRef)
		{ // Entry CREATED
			opcode = B_ENTRY_CREATED;
			m->FindInt64("to directory", &eref.directory);
		}
	}

	if (opcode == B_ENTRY_CREATED)
	{
		BEntry entry(&eref);		
		UpdateItem(NULL, &entry);
		UpdateControls();
		return B_OK;
	}
	else if (opcode == B_ENTRY_REMOVED)
	{
		TPrinterItem *it;
		int32 index = 0;
		while ((it = (TPrinterItem *)fPrinterList->ItemAt(index)) != NULL)
		{
			// Find the printer that has been removed
			if (ref == it->Ref())
			{
				fPrinterList->RemoveItem(it);
	
				if (it == LastSelectedPrinter())
				{ // If the removed printer was the selected one in our list	
					it->Watcher()->SetFlags(WATCH_COUNT);
					dynamic_cast<TWindow *>(Window())->EmptyJobList();
					SetLastSelectedPrinter(NULL);
				}

				// If the printer we've just removed is the current default printer
				// (the one the print_server is actually using)
				if (strcmp(it->Name(), ((TWindow *)Window())->CurrentPrinter()) == 0)
				{
					// What we should do here is not clear to me.
					// There is no selected default printer.
					// But this is actually not a bug or a problem
					// So do nothing.
				}

				delete it;
				UpdateControls();
				return B_OK;	
			}
			index++;
		}
		return B_ERROR;
	}

	return B_ERROR;
}

void TPrinterBox::WatcherEvent(BMessage *m)
{
	int32 job_count;
	
	// Identify the item and its index (if it exists at all)
	node_ref ref;
	node_ref ref2;
	if ((m->FindInt32("device", &ref.device) != B_OK) ||
		(m->FindInt64("node", &ref.node) != B_OK))
		return;

	TPrinterItem *it;
	int32 index = 0;
	while ((it = (TPrinterItem *)fPrinterList->ItemAt(index)) != NULL)
	{
		if (it->Watcher()->Ref() == ref)
			goto item_found;
		index++;
	}
	return;
	
item_found:
	// Dispatch the message
	switch (m->what)
	{
		case JOB_CREATED:
		case JOB_DESTROYED:
		case JOB_MODIFIED:
			if (it->IsSelected())
				((TWindow*)Window())->JobEvent(m);
			break;
		case JOB_COUNT_MODIFIED:
			if (m->FindInt32("job_count", &job_count) != B_OK)
				return;
			it->SetJobCount(job_count);
			fPrinterList->InvalidateItem(index);
			break;

		case FAILED_CHANGED:
			it->SetFailed(m->FindBool("has_failed"));
		case PRINTER_MODIFIED:
			{
				// Re-read the settings. The default printer may have changed
				fWindow->GetCurrentPrinter();
				BDirectory dir(&fRef);
				BEntry entry;
				while (dir.GetNextEntry(&entry) == B_OK)
				{
					entry.GetNodeRef(&ref2);
					if (ref == ref2)
					{ // the printer modified is in our list
						if (UpdateItem(it, &entry))
						{
							fPrinterList->InvalidateItem(index);
							UpdateControls();
						}
						break;
					}
				}
			}
			break;
	}
}

bool TPrinterBox::UpdateItem(TPrinterItem *it, BEntry *entry)
{	
	// It is possible that the entry has been deleted before we arrive here
	// This happens in the case the printer is created but there is an error
	// during the printer configuration; the printer is then suppressed
	// but we will receive this message (node monitor)
	if (entry->Exists() == false)
		return false;

	// Create the item if necessary.
	if (!it)
	{
		BPath path(entry);
		it = new TPrinterItem(Window(), path.Path());
		fPrinterList->AddItem(it);
		it->Watcher()->SetFlags(WATCH_COUNT);
	}
	
	// Get info about the printer state and parameters
	char driver[256], comments[256], transport[256], connection[256];
	char buf[256];
	node_ref ref;
	entry->GetName(buf);
	entry->GetNodeRef(&ref);
	BNode node(entry);
	if (node.InitCheck() == B_OK)
	{
		if (node.ReadAttr(PSRV_PRINTER_ATTR_DRV_NAME,	B_STRING_TYPE, 0, driver, 255) <= 0)		driver[0] = 0;
		if (node.ReadAttr(PSRV_PRINTER_ATTR_COMMENTS,	B_STRING_TYPE, 0, comments, 255) <= 0)		comments[0] = 0;
		if (node.ReadAttr(PSRV_PRINTER_ATTR_TRANSPORT,	B_STRING_TYPE, 0, transport, 255) <= 0)		transport[0] = 0;
		if (node.ReadAttr(PSRV_PRINTER_ATTR_CNX,		B_STRING_TYPE, 0, connection, 255) <= 0)	connection[0] = 0;
	}
	else
	{
		driver[0] = 0;
		comments[0] = 0;
		transport[0] = 0;
		connection[0] = 0;
	}

	bool is_checked = (strcmp(fWindow->CurrentPrinter(), buf) == 0);

	// Check if the status changed.	
	bool need_update, need_icon;
	it->SetInfos(buf, driver, transport, comments, is_checked, &need_update, &need_icon);
	if (need_icon)
		it->SetIcon((strcmp(connection, "Local") == 0));
	return need_update;
}

void TPrinterBox::Select(const char* name)
{
	if (name == NULL)
		return;

	// Look for a printer with the proper name
	TPrinterItem *it;
	bool found = false;
	int32 index = 0;
	while ((it = (TPrinterItem *)fPrinterList->ItemAt(index)) != NULL)
	{
		if (strcmp(it->Name(), name) == 0)
		{
			found = true;
			break;
		}
		index++;
	}
	
	// If one is found, change the selection and move the display
	if (found)
	{
		fPrinterList->Select(index);
		fPrinterList->ScrollToSelection();
	}
	UpdateControls();
}

const TPrinterItem* TPrinterBox::SelectedPrinter()
{
	return (TPrinterItem *)fPrinterList->ItemAt(fPrinterList->CurrentSelection());
}

const TPrinterItem* TPrinterBox::LastSelectedPrinter()
{
	return fLastSelectedPrinter;
}

void TPrinterBox::SetLastSelectedPrinter(const TPrinterItem *it)
{
	fLastSelectedPrinter = it;
}

void TPrinterBox::UpdateControls()
{
	bool selected = (fPrinterList->CountItems() > 0) && (fPrinterList->CurrentSelection() != -1);
	bool different = false;
	BEntry *new_entry = NULL;
	
	
	// First verify that the Selected printer has changed
	// In this case, stop watching jobs for the old one
	const TPrinterItem *it = LastSelectedPrinter();
	if (it && (it != SelectedPrinter()))
	{
		it->Watcher()->SetFlags(WATCH_COUNT);
		dynamic_cast<TWindow *>(Window())->EmptyJobList();
	}

	// Then be sure that a printer is selected
	BEntry entry;	// Must stay out of the next statement (used by pointer in SetJobEntry)
	it = NULL;
	if (selected)
	{
		it = SelectedPrinter();
		different = (it && strcmp(it->Name(), fWindow->CurrentPrinter()));

		BDirectory dir(&fRef);
		if (dir.FindEntry(it->Name(), &entry) == B_OK)
			new_entry = &entry;

		// Start watching jobs
		if (it->Watcher()->Flags() == WATCH_COUNT)
			it->Watcher()->SetFlags(WATCH_COUNT | WATCH_ITEMS);
	}
	SetLastSelectedPrinter(it);
	
	// Update the job list with the job of the new selected printer
	dynamic_cast<TWindow *>(Window())->SetJobEntry(new_entry);
	
	// Enable or disable buttons depending of the current selection
	fSetupBtn->SetEnabled(selected);
	fRemoveBtn->SetEnabled(selected);
	fSetDefaultBtn->SetEnabled(different);
}

// ************************************************************************
// #pragma mark -

TJobItem::TJobItem(const char* name, const char* app_signature, const char* path,
	const char* status, status_t errorcode, int32 page_count, float size) : BListItem()
{
	entry_ref ref;
	
	// Get a copy of the simple parameters
	fName = strdup(name);
	fPath = strdup(path);
	fStatus = strdup(status);
	fPageCount = page_count;
	fErrorCode = errorcode;
	fSize = size;
	if (fSize < 0.01f)
		fSize = 0.01f;
	// Get the icon based on the MimeType
	GetIcon(app_signature);
}

TJobItem::~TJobItem()
{
	free(fName);
	free(fPath);
	free(fStatus);
	delete fAppIcon;
}


void TJobItem::DrawItem(BView *owner, BRect bounds, bool complete)
{
	char job_buffer[64];
	char buffer[2*128+64];
	char *t_buffer[4];
	char *t_source[4];
	float width, width2, width3;
	BFont font;
	BPoint where;
	
	// Create the string describing the page count.
	if (fPageCount == 0)		strcpy(job_buffer, RsrcGetString(123));
	else if (fPageCount == 1)	strcpy(job_buffer, RsrcGetString(124));
	else						sprintf(job_buffer,RsrcGetString(125), fPageCount);
	
	// Create the string describing the size
	if (fSize < 1.0f)			sprintf(job_buffer+32, RsrcGetString(126), (int32)(1024.0f*fSize));
	else if (fSize < 100.0f)	sprintf(job_buffer+32, RsrcGetString(127), fSize);
	else if (fSize < 1000.0f)	sprintf(job_buffer+32, RsrcGetString(128), (int32)fSize);
	else if (fSize < 102400.0f)	sprintf(job_buffer+32, RsrcGetString(129), fSize*(1.0f/1024.0f));
	else						sprintf(job_buffer+32, RsrcGetString(130), (int32)(fSize*(1.0f/1024.0f)));
	
	// Calculate the width of the various columns.
	width = Width() - 2.0f;
	width3 = floor((width - 22.0f) * 0.24f);
	width2 = width-(26.0f + width3);
	
	// Truncate the strings to fit the space available.

	owner->GetFont(&font);
	t_buffer[0] = buffer+0;
	t_buffer[1] = buffer+128;
	t_buffer[2] = buffer+256;
	t_buffer[3] = buffer+288;	

	char status[128];
	if (strcmp(fStatus, PSRV_JOB_STATUS_FAILED)) {
		t_source[1] = fStatus;
	} else {
		t_source[1] = status;
		strcpy(status, fStatus);
		strcat(status, ": ");
		switch (fErrorCode)
		{
			case B_ERROR:	strcat(status, RsrcGetString(131));	break;
			default: 		strcat(status, strerror(fErrorCode));
		}
	}
	
	t_source[0] = fName;
	t_source[2] = job_buffer;
	t_source[3] = job_buffer+32;	
	font.GetTruncatedStrings((const char**)t_source, 2, B_TRUNCATE_END, width2-7.0, t_buffer);
	font.GetTruncatedStrings((const char**)(t_source+2), 2, B_TRUNCATE_END, width3-3.0, t_buffer+2);
	
	// We want to restore the initial state of the owner when leaving.
	owner->PushState();
	
	rgb_color text_color;
	if (IsSelected()) { 
		owner->SetLowColor(ui_color(B_MENU_SELECTED_BACKGROUND_COLOR));
		owner->SetHighColor(text_color = ui_color(B_MENU_SELECTED_ITEM_TEXT_COLOR));
		complete = true;
	} else {
		owner->SetLowColor(owner->ViewColor());
		owner->SetHighColor(text_color = ui_color(B_MENU_ITEM_TEXT_COLOR));
	}

	if (complete)
		owner->FillRect(bounds, B_SOLID_LOW);
	
	// Draw the icon on the left side
	owner->SetDrawingMode(B_OP_OVER);
	owner->DrawBitmapAsync(fAppIcon, bounds.LeftTop() + BPoint(5,6));
	owner->SetDrawingMode(B_OP_COPY);
	
	// Draw the text infos.
	where.x = bounds.left + 28.0f;
	where.y = bounds.top + fv0;
	owner->DrawString(t_buffer[0], where);
		
	where.y += fdv;
	if (!strcmp(fStatus, PSRV_JOB_STATUS_FAILED))
		owner->SetHighColor(ui_color(B_FAILURE_COLOR));
	owner->DrawString(t_buffer[1], where);
	owner->SetHighColor(text_color);

	where.x = bounds.left + 26.0 + width2;
	where.y = bounds.top + fv0;
	owner->DrawString(t_buffer[2], where);

	where.y += fdv;
	owner->DrawString(t_buffer[3], where);
		
	// Put the owner back in its initial state.
	owner->PopState();
	owner->Sync();
}

void TJobItem::Update(BView *owner, const BFont *font)
{
	float		height;
	font_height finfo;
		
	// Check the vertical metric of the font.
	font->GetHeight(&finfo);
	fv0 = ceil(finfo.ascent) + 1.0;
	fdv = ceil(finfo.ascent) + ceil(finfo.descent) + ceil(finfo.leading);

	// Calculate the minimal Height of a cell and set it.
	height = fdv*2.0+1.0;
	if (height < 32.0) height = 32.0;
	fdv0 = 0.0;

	fv0 += fdv0;
	SetHeight(height);
	SetWidth(owner->Bounds().Width());
}

void TJobItem::GetIcon(const char *mime_type)
{
	entry_ref ref;
	be_roster->FindApp(mime_type, &ref);
	fAppIcon = new BBitmap(BRect(0, 0, B_MINI_ICON-1, B_MINI_ICON-1), B_COLOR_8_BIT);
	BNodeInfo::GetTrackerIcon(&ref, fAppIcon, B_MINI_ICON);
}

void TJobItem::JobModified(BMessage *m)
{
	char		*path;
	char		*mime;
	char		*name;
	char		*status;
	int32		page_count;
	float		size;
	
	if (m->FindString("name", (const char**)&name) == B_OK) {
		free(fName);
		fName = strdup(name);
	}
	if (m->FindString("path", (const char**)&path) == B_OK) {
		free(fPath);
		fPath = strdup(path);
	}
	if (m->FindString("status", (const char**)&status) == B_OK) {
		free(fStatus);
		fStatus = strdup(status);
		if (m->FindInt32("errorcode", &fErrorCode) != B_OK)
			fErrorCode = B_ERROR;
	}
	if (m->FindString("mime", (const char**)&mime) == B_OK) {
		delete fAppIcon;
		GetIcon(mime);
	}
	if (m->FindInt32("page_count", &page_count) == B_OK)
		fPageCount = page_count;
	if (m->FindFloat("size", &size) == B_OK)
		fSize = size;
}					

// ************************************************************************
// #pragma mark -

TJobBox::TJobBox(BRect frame)
	: BBox(frame, "box", B_FOLLOW_BOTTOM | B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP),
	fEntry(NULL)
{
	//	listview
	BRect r(14, 28, Bounds().Width()-kButtonWidth-20-B_V_SCROLL_BAR_WIDTH-3, Bounds().Height()-10);
	fJobList = new BListView(r, "lv:jobs");
	fJobList->SetSelectionMessage(new BMessage(msg_single_click));
	fJobList->SetInvocationMessage(new BMessage(msg_double_click));

	//	scroller
	fListScroller = new BScrollView("scroll", fJobList, B_FOLLOW_ALL, B_FRAME_EVENTS | B_WILL_DRAW, false, true);
	AddChild(fListScroller);
	
	// 	add btn
	r.Set(Bounds().Width()-kButtonWidth-11, 28, Bounds().Width()-11, 28);
	fAbortBtn = new BButton(r, "bt:cancel", RsrcGetString(kBtnCancelJob), new BMessage(msg_abort_job));
	AddChild(fAbortBtn);

	r.OffsetBy(0, 4+fAbortBtn->Bounds().Height());
	fRetryBtn = new BButton(r, "bt:retry", RsrcGetString(kBtnRestartJob), new BMessage(msg_restart_job));
	AddChild(fRetryBtn);

	// init button states.
	UpdateControls();

	// set label
 	SetLabel(RsrcGetString(kLabelJobNoPrinter));
}

TJobBox::~TJobBox()
{
	EmptyList();
	delete fEntry;
}

bool TJobBox::AskAbortJob()
{
	TJobItem *job = SelectedJob();
	if (job == NULL)
		return false;
	BNode node(job->Path());
	TPrintTools::CancelJob(&node);
	return true;
}

bool TJobBox::AbortJob()
{
	TJobItem *job = SelectedJob();
	if (job == NULL)
		return false;
	BEntry entry(job->Path());
	entry.Remove();
	fJobList->RemoveItem(fJobList->IndexOf(job));
	delete job;
	return true;
}

bool TJobBox::RestartJob()
{
	TJobItem *job = SelectedJob();
	if (job == NULL)
		return false;
	BNode node(job->Path());
	node.WriteAttr(PSRV_SPOOL_ATTR_STATUS, B_STRING_TYPE, 0, PSRV_JOB_STATUS_WAITING, sizeof(PSRV_JOB_STATUS_WAITING)+1);
	return true;
}

void TJobBox::EmptyList()
{
	while (fJobList->IsEmpty() == false)
		delete fJobList->RemoveItem((int32)0);
}

void TJobBox::WatcherEvent(BMessage *m)
{
	const char	*path;
	const char	*mime;
	const char	*name;
	const char	*status;
	int32		page_count;
	int32		errorcode;
	float		size;
	TJobItem	*it;
	
	switch (m->what)
	{
		case JOB_CREATED:
			if (m->FindString("name", &name) != B_OK)				break;
			if (m->FindString("path", &path) != B_OK)				break;
			if (m->FindString("status", &status) != B_OK)			break;
			if (m->FindString("mime", &mime) != B_OK)				break;
			if (m->FindInt32("page_count", &page_count) != B_OK)	break;
			if (m->FindFloat("size", &size) != B_OK)				break;
			if (m->FindInt32("errorcode", &errorcode) != B_OK)
				errorcode = B_ERROR;
			it = new TJobItem(name, mime, path, status, (status_t)errorcode, page_count, size);
			fJobList->AddItem(it);
			UpdateControls();
			break;	
			
		case JOB_DESTROYED:
			if (m->FindString("path", &path) != B_OK)
				break;
			if ((it = FindItem(path)) == NULL)
				break;
			fJobList->RemoveItem(it);
			delete it;
			UpdateControls();
			break;
	
		case JOB_MODIFIED:
			if (m->FindString("old_path", &path) != B_OK)
				break;
			if ((it = FindItem(path)) == NULL)
				break;
			it->JobModified(m);
			fJobList->InvalidateItem(fJobList->IndexOf(it));
			UpdateControls();
			break;
	}
}

TJobItem *TJobBox::FindItem(const char *path)
{
	int32		index;
	TJobItem	*it;
	
	index = 0;
	while ((it = (TJobItem *)fJobList->ItemAt(index)) != NULL) {
		if (strcmp(it->Path(), path) == 0)
			return it;
		index++;
	}
	return NULL;
}

void TJobBox::SetEntry(const BEntry *entry)
{
	char label[16+B_FILE_NAME_LENGTH];

	// The entry refered by the job list is unchanged.
	if ((entry != NULL) && (fEntry != NULL)) 
		if (*entry == *fEntry)
			return;

	if ((entry == NULL) && (fEntry == NULL))
		return;
		
	delete fEntry;
	if (entry)
	{
		fEntry = new BEntry(*entry);
		fJobList->Select(0);
		fJobList->ScrollToSelection();
		strcpy(label, RsrcGetString(kLabelJobPrinter));
		entry->GetName(label + strlen(label));
		SetLabel(label);
	}
	else
	{
		fEntry = NULL;
		SetLabel(RsrcGetString(kLabelJobNoPrinter));
	}
}

void TJobBox::UpdateControls()
{
	// status				button1			button2		text1			text2
	// waiting: 			>canceling		-			Cancel Job
	// failed: 				delete			retry		Delete Job		Restart Job		
	// canceled:			[delete]		[retry]		[Delete Job]	[Restart Job]
	// completed: 			[delete]		[again]		[Delete Job]	[Restart Job]
	// processing:	 		>canceling		-			Cancel Job
	// cancel in progress:	-				-

	// If there is a selection, enable the Abort command
	TJobItem *it = SelectedJob();
	if (it)
	{
		const char *status = it->Status();
		if (!strcmp(status, PSRV_JOB_STATUS_WAITING))
		{
			fAbortBtn->SetEnabled(true);
			fAbortBtn->SetLabel(RsrcGetString(kBtnCancelJob));
			fAbortBtn->SetMessage(new BMessage(msg_abort_job));
			fRetryBtn->SetEnabled(false);
		}
		else if (!strcmp(status, PSRV_JOB_STATUS_FAILED))
		{
			fAbortBtn->SetEnabled(true);
			fAbortBtn->SetLabel(RsrcGetString(kBtnDeleteJob));
			fAbortBtn->SetMessage(new BMessage(msg_abort_job));
			fRetryBtn->SetEnabled(true);
			fRetryBtn->SetLabel(RsrcGetString(kBtnRestartJob));
		}
		else if (!strcmp(status, PSRV_JOB_STATUS_CANCELLED))
		{
			fAbortBtn->SetEnabled(true);
			fAbortBtn->SetLabel(RsrcGetString(kBtnDeleteJob));
			fAbortBtn->SetMessage(new BMessage(msg_abort_job));
			fRetryBtn->SetEnabled(true);
			fRetryBtn->SetLabel(RsrcGetString(kBtnRestartJob));
		}
		else if (!strcmp(status, PSRV_JOB_STATUS_COMPLETED))
		{
			fAbortBtn->SetEnabled(true);
			fAbortBtn->SetLabel(RsrcGetString(kBtnDeleteJob));
			fAbortBtn->SetMessage(new BMessage(msg_abort_job));
			fRetryBtn->SetEnabled(true);
			fRetryBtn->SetLabel(RsrcGetString(kBtnRestartJob));
		}
		else if (!strcmp(status, PSRV_JOB_STATUS_PROCESSING))
		{
			fAbortBtn->SetEnabled(true);
			fAbortBtn->SetLabel(RsrcGetString(kBtnCancelJob));
			fAbortBtn->SetMessage(new BMessage(msg_ask_abort_job));
			fRetryBtn->SetEnabled(false);
		}
		else
		{
			fAbortBtn->SetEnabled(false);
			fRetryBtn->SetEnabled(false);
		}
	}
	else
	{
		fAbortBtn->SetEnabled(false);
		fRetryBtn->SetEnabled(false);
	}
}

TJobItem* TJobBox::SelectedJob()
{
	return static_cast<TJobItem *>(fJobList->ItemAt(fJobList->CurrentSelection()));
}

// ************************************************************************
// #pragma mark -

TPrintPanel::TPrintPanel(	TWindow *printer,
							print_panel_action action,
							const BMessage *settings,
							const BMessenger *target,
							const BMessage *template_message,
							uint32 flags)
		: BPrintPanel(action, settings, target, template_message, flags),
			fPrinterPanel(printer)
{
}

TPrintPanel::~TPrintPanel()
{
}

void TPrintPanel::PrinterChanged()
{
	BPrintPanel::PrinterChanged();
	BPrintJobSettings s(Settings());
	if (fPrinterPanel->Lock()) {
		fPrinterPanel->fPrinterBox->Select(s.PrinterName().String());
		fPrinterPanel->Unlock();
	}
}


