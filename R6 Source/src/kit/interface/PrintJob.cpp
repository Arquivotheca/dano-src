//******************************************************************************
//
//	File:		PrintJob.cpp
//
//	Written by:	Benoit Schillings, Mathias Agopian
//
//	Copyright 1996-2000, Be Incorporated
//
//******************************************************************************


#ifndef _DEBUG_H
#	include <Debug.h>
#endif

#include <BeBuild.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include <InterfaceDefs.h>
#include <Roster.h>
#include <Messenger.h>
#include <Application.h>
#include <Window.h>
#include <View.h>
#include <Alert.h>
#include <Picture.h>
#include <ScrollBar.h>
#include <File.h>
#include <Directory.h>
#include <NodeInfo.h>
#include <Path.h>
#include <FindDirectory.h>
#include <Region.h>
#include <String.h>
#include <AppPrefs.h>

// for _printing_state_
#include <interface_misc.h>

// needed for signatures
#include <pr_server.h>

// Our own include file
#include <PrintJob.h>
#include <print/PrintJobSettings.h>
#include <print/TransportIO.h>
#include <print/PrinterAddOn.h>
#include <print/PrinterConfigAddOn.h>
#include <print/DirectPrintJob.h>

#include "PrintEnv.h"
#include "PrintStream.h"

#if (PRINTING_FOR_DESKTOP)
#	define M_CREATE_SPOOLFILE	1
#	define NO_PRINT_SERVER		0
#	define NO_PRINT_PANEL		0
#	define HANDLE_ERRORS		1
#else
#	define M_CREATE_SPOOLFILE	0
#	define NO_PRINT_SERVER		1
#	define NO_PRINT_PANEL		1
#	define HANDLE_ERRORS		0
#endif

#if (HANDLE_ERRORS)
#	include "AboutBox.h"	// For the ressources
#endif

#if (NO_PRINT_PANEL == 0)
#	include <print/PrintPanel.h>
#endif

#define IO_BUFFER_SIZE 16384

//------------------------------------------------------------------
// Usefull defines
//------------------------------------------------------------------

#define m _m_rprivate

//#define DEBUG 1

#if DEBUG
#	define D(_x)	_x
#else
#	define D(_x)
#endif

#define E(_x)	_x
#define bug		printf


//------------------------------------------------------------------
// Defined types
//------------------------------------------------------------------

#include "PrintJob_private.h"

using namespace BPrivate;

//------------------------------------------------------------------

BPrintJob::BPrintJob(const char *job_name)
	: 	_m_private(new printjob_private),
		m(*_m_private)
{
	m.print_job_name = (char *)job_name;
	if ((m.fInitCheck = InitObject()) == B_OK)
		m.fInitCheck = SetPrinter();
}

BPrintJob::BPrintJob(const BMessage& settings, const char *job_name, bool preview)
	: 	_m_private(new printjob_private),
		m(*_m_private)
{
	m.print_job_name = (char *)job_name;
	m.fPreview = preview;
	if ((m.fInitCheck = InitObject()) == B_OK)
		m.fInitCheck = SetSettings(new BMessage(settings));
}

BPrintJob::BPrintJob(const char *printer, const char *job_name)
	: 	_m_private(new printjob_private),
		m(*_m_private)
{
	m.raw = true;
	m.print_job_name = (char *)job_name;
	if ((m.fInitCheck = InitObject()) == B_OK)
		m.fInitCheck = SetPrinter(printer);
}

BPrintJob::~BPrintJob()
{
	CleanUpSpoolData();
	delete _m_private;
}

status_t BPrintJob::SetPrinter(const char *printerName)
{
	status_t result;
	if ((printerName == NULL) || (strlen(printerName) == 0))
	{ // Get the default printer's name
		BString current_printer;
		if (((result = get_default_printer(current_printer)) != B_OK)  || (current_printer.Length() == 0))
			return B_NO_PRINTER;
		m.fPrinterName = current_printer;
	}
	else
		m.fPrinterName = printerName;

	// Get printer's node
	BPath path;
	find_directory(B_USER_PRINTERS_DIRECTORY, &path);
	path.Append(m.fPrinterName.String());	
	m.fPrinterNode.SetTo(path.Path());	
	if ((result = m.fPrinterNode.InitCheck()) != B_OK)
		return B_INVALID_PRINTER;
	return B_OK;
}

status_t BPrintJob::CleanUpSpoolData()
{
	if ((m.fPrinterManager) && (m.fJobStarted))
	{ // Free the printer if we're in stand alone mode
		m.fPrinterManager->set_printer_free(&PrinterNode());
	}

	if (m.spoolFile)
	{
		delete m.spoolFile;
		m.spoolFile = NULL;
		unlink(m.spool_file_name);
	}

	cleanup_spool_list(m.fPageHeadersList, m.fPagePicturesList);
	m.fPageHeadersList = NULL;
	m.fPagePicturesList = NULL;	
	return B_OK;
}


status_t BPrintJob::Reset(const char *job_name)
{
	CancelJob();
	m.re_init();
	if (job_name)
	{ // There is a new name for the PrintJob
		free(m.print_job_name);
		m.print_job_name = (char *)job_name;
	}
	m.fInitCheck = InitObject();
	return InitCheck();
}


void BPrintJob::cleanup_spool_list(BList *header, BList *pic)
{
	if (header)
	{
		const int32 count = header->CountItems();
		for (int32 i=0 ; i<count ; i++)
			delete static_cast<_page_header_*>(header->RemoveItem(0L));
		delete header;
	}

	if (pic)
	{
		const int32 count = pic->CountItems();
		for (int32 i=0 ; i<count; i++)
		{
			_page_pics_ *tmp = static_cast<_page_pics_*>(pic->RemoveItem(0L));
			delete tmp->picture;
			delete tmp;
		}
		delete pic;
	}
}

status_t BPrintJob::InitCheck() const
{
	return m.fInitCheck;
}

status_t BPrintJob::InitObject()
{
	if (m.print_job_name)
		m.print_job_name = strdup(m.print_job_name);
	else
	{ // Build a name for the application's name and current window's name
		app_info appInfo;
		be_app->GetAppInfo(&appInfo);
		thread_info threadInfo;
		get_thread_info(appInfo.thread, &threadInfo);
		char name[256];
		BWindow *window = dynamic_cast<BWindow *>(BLooper::LooperForThread(find_thread(NULL)));
		if (window)	sprintf(name, "%s (%s)", threadInfo.name, window->Title());
		else		sprintf(name, "%s", threadInfo.name);
		m.print_job_name = strdup(name);
	}

	// Is there a print_server ?
#if NO_PRINT_SERVER
	m.fStandAlone = true;
#else
	ServerMessenger() = BMessenger(PSRV_PRINT_SERVER_SIG, -1);
	if (ServerMessenger().IsValid() == false)
	{
		status_t result = be_roster->Launch(PSRV_PRINT_SERVER_SIG);
		if (result != B_OK)
		{ // No print_server. This may not be an error on IAD
			m.fStandAlone = true;
		}
	}
#endif

	// Standalone is not supported in RAW mode
	if ((m.fStandAlone) && (m.raw))
		return B_UNSUPPORTED;

	// In stand alone mode, we need a printer manager (because we don't have a print-server)
	if (m.fStandAlone)
		m.fPrinterManager = new PrinterManager(m.fStandAlone);

	// update the internal settings, be sure it's empty
	m.fSettings.ClearSettings();

	// Init complete
	return B_OK;
}

BPositionIO* BPrintJob::spool_file() const
{
	return m.spoolFile;
}

//------------------------------------------------------------------
// #pragma mark -

BMessenger& BPrintJob::ServerMessenger() const
{
	return m.fMessenger;
}

BNode& BPrintJob::PrinterNode() const
{
	return m.fPrinterNode;
}

//------------------------------------------------------------------

void BPrintJob::check_status(const char *f)
{
	if (InitCheck() == B_OK)
		return;

	bug("BPrintJob: %s, ", f);
	switch (InitCheck())
	{
		case B_NO_PRINTER:
			bug("No printer");
			break;
		case B_INVALID_PRINTER:
			bug("Invalid printer");
			break;
		case B_INVALID_PRINT_SETTINGS:
			bug("Invalid printer settings");
			break;
		default:
			bug("%s", strerror(InitCheck()));
			break;
	}
	bug(" -- you must call InitCheck()\n");
}

status_t BPrintJob::get_default_settings()
{
	BMessage settings;

	image_id addon_image = load_driver_addon(&PrinterNode());
	if (addon_image < 0)
		return (status_t)addon_image;

	status_t err;
	BMessage * (*get_driver_settings)(BNode*, BMessage*);
	BPrinterConfigAddOn *(*instantiate_printer_config_addon)(BTransportIO *, BNode *);
	if ((err = get_image_symbol(addon_image, B_INSTANTIATE_PRINTER_CONFIG_ADDON_FUNCTION, B_SYMBOL_TYPE_TEXT, (void **)&instantiate_printer_config_addon)) == B_OK)
	{
		BTransportIO transport(&PrinterNode());
		if ((err = transport.InitCheck()) == B_OK)
		{
			BPrinterConfigAddOn *printerAddOn;
			if ((printerAddOn = instantiate_printer_config_addon(&transport, &PrinterNode())) != NULL)
				settings = printerAddOn->Settings().Message();
			else
				err = B_BAD_DRIVER;
			delete printerAddOn;

			if (settings.HasInt32("be:newapi") == false)
				settings.AddInt32("be:newapi", 0x00000001);
		}
	}
	else if ((err = get_image_symbol(addon_image, "default_settings", B_SYMBOL_TYPE_TEXT, (void**)&get_driver_settings)) == B_OK)
	{ // Try the old way of handling print job for compatibility
		BMessage *m = get_driver_settings(&PrinterNode(), NULL);
		if (m == NULL)	err = B_ERROR;
		else			settings = *m;
		delete m;

		settings.RemoveName("be:newapi");
	}

	unload_add_on(addon_image);
	
	if (err == B_OK)
		update_settings(settings);

	return err;
}

//------------------------------------------------------------------

image_id BPrintJob::load_driver_addon(BNode *node, BNode *job)
{
	return TPrintTools::LoadDriverAddOn(node, job);
}

status_t BPrintJob::EndLastPage()
{
	if (m.m_curPageHeader->pictureCount == 0)
		return B_OK;

	if (m.fStandAlone)
	{
		m.current_header.page_count++;
		m.fPageHeadersList->AddItem((void *)m.m_curPageHeader);
		m.m_curPageHeader = NULL;
	}
	else
	{
		if (m.spoolFile == NULL)
			return B_ERROR;
		m.current_header.page_count++;
		m.spoolFile->Seek(0, SEEK_END);
		if (m.m_curPageHeaderOffset)
		{
			m.m_curPageHeader->nextPage = 0;
			m.spoolFile->Seek(m.m_curPageHeaderOffset, SEEK_SET);
			m.spoolFile->Write(m.m_curPageHeader, sizeof(_page_header_));
			m.spoolFile->Seek(0, SEEK_END);
		}
	}
	return B_OK;
}


//------------------------------------------------------------------
// #pragma mark -

// BPrintJob::RecurseView()
//
// BRect r : is what part of the view you want to draw
// BPoint origin : is where you want the B_ORIGIN to be in the top view's coordinate system
//
void BPrintJob::RecurseView(BView *v, BPoint origin, BPicture *p, BRect r)
{
	BRegion region;
	region.Set(BRect((int)r.left, (int)r.top, (int)(r.right+0.5f), (int)(r.bottom+0.5f)));

	// Set the clipping region of this view as seen by the application
	((_printing_state_ *)(v->pr_state))->clipping = r;

	// Erase the view and draw
	rgb_color vColor = v->ViewColor();
	v->AppendToPicture(p);
		v->PushState();
			v->SetOrigin(origin);
			v->SetScale(m.scale);
			v->ConstrainClippingRegion(&region);			
			if ((vColor.red != B_TRANSPARENT_COLOR.red) ||
				(vColor.green != B_TRANSPARENT_COLOR.green) ||
				(vColor.blue != B_TRANSPARENT_COLOR.blue) ||
				(vColor.alpha != B_TRANSPARENT_COLOR.alpha))
			{
				rgb_color oldColor = v->HighColor();
				v->SetHighColor(vColor);
				v->FillRect(r);
				v->SetHighColor(oldColor);
			}
			v->f_is_printing = true;
			v->Draw(r);
			v->f_is_printing = false;
		v->PopState();
	v->EndPicture();

	// Go through the list of childreen
	BView *child = v->ChildAt(0);
	while (child)
	{
		if ((child->IsHidden(child) == false) && (child->Flags() & B_WILL_DRAW))
		{ // Hidden() and not B_WILL_DRAW BView never receive update messages.
			BPoint childPositionInParent = v->Bounds().LeftTop() + (child->Frame().LeftTop() - child->Bounds().LeftTop());
			BPoint childOrigin = origin + childPositionInParent;
			BRect updateRect = child->Bounds() & r.OffsetToCopy(r.LeftTop() - childPositionInParent);
			if (updateRect.IsValid())
			{
				if (dynamic_cast<BScrollBar *>(child))
				{	// We don't know how to render the ScrollBars, yet
					// don't draw it at all
				}
				else
				{
					v->f_is_printing = true;
					RecurseView(child, childOrigin, p, updateRect);
					v->f_is_printing = false;
				}
			}
		}
		child = child->NextSibling();
	}

	// Draw After childreen
	if (v->Flags() & B_DRAW_ON_CHILDREN)
	{
		v->AppendToPicture(p);
			v->PushState();
				v->SetOrigin(origin);
				v->SetScale(m.scale);
				v->ConstrainClippingRegion(&region);			
				v->f_is_printing = true;
				v->DrawAfterChildren(r);
				v->f_is_printing = false;
			v->PopState();
		v->EndPicture();
	}
}

void BPrintJob::AddPicture(BPicture *picture, BRect rect, BPoint where)
{
	if (m.fStandAlone)
	{
		m.m_curPageHeader->pictureCount++;
		_page_pics_ *pic = new _page_pics_(where, rect, picture);
		m.fPagePicturesList->AddItem((void *)pic);
	}
	else
	{
		if (m.spoolFile == NULL)
			return;
		m.m_curPageHeader->pictureCount++;
		m.spoolFile->Write(&where, sizeof(BPoint));
		m.spoolFile->Write(&rect, sizeof(BRect));
		picture->Flatten(m.spoolFile);
		delete picture;
	}
}

//------------------------------------------------------------------
// #pragma mark -
status_t BPrintJob::SetScale(float scale_factor)
{
	if (dynamic_cast<BDirectPrintJob *>(this))
		return B_UNSUPPORTED;
	m.scale = scale_factor;
	return B_OK;
}

status_t BPrintJob::DrawView(BView *a_view, BRect a_rect, BPoint where)
{
	if (dynamic_cast<BDirectPrintJob *>(this))
		return B_UNSUPPORTED;
	E(check_status("DrawView"));
	if (InitCheck() != B_OK)
		return InitCheck();

	if (CanContinue() == false)
		return B_CANCELED;

	if (a_view->LockLooper())
	{ 	// Clip this view with the PrintableRect()
		// We can't do that here, because the scalling is done by the application
		// thus, usable_size() should be scalled accordingly -which is not possible, without
		// another hack-
		// Just don't clip. That's not a problem!		
		// 		a_rect = a_rect & usable_size.OffsetToCopy(a_rect.LeftTop() - where);
		if (a_rect.IsValid())
		{
			BPicture *a_picture = new BPicture;
			BPoint origin = B_ORIGIN - a_rect.LeftTop();
			RecurseView(a_view, origin, a_picture, a_rect);
			AddPicture(a_picture, a_rect*m.scale, where);
		}
		a_view->UnlockLooper();
	}

	return B_OK;
}

status_t BPrintJob::DrawPictures(BPicture * const *pictures, const BRect *clips, const BPoint *points, const uint32 nb_pictures)
{
	if (dynamic_cast<BDirectPrintJob *>(this))
		return B_UNSUPPORTED;
	E(check_status("DrawPictures"));
	if (InitCheck() != B_OK)
		return InitCheck();

	if (CanContinue() == false)
		return B_CANCELED;

	// Do some sanity checks because all those data comes from the application
	if ((pictures == NULL) || (clips == NULL) || (points == NULL) || (nb_pictures == 0))
		return B_BAD_VALUE;

	// First, check that we have valid pointers for the BPictures and valid BRect
	for (unsigned int i=0 ; i<nb_pictures ; i++)
	{
		if ((pictures[i] == NULL) || (clips[i].IsValid() == false))
			return B_BAD_VALUE;
	}

	// Add the pictures to our list
	for (unsigned int i=0 ; i<nb_pictures ; i++)
		AddPicture(pictures[i], clips[i], points[i]);

	return B_OK;
}

//------------------------------------------------------------------

status_t BPrintJob::ConfigJob()
{
	E(check_status("ConfigJob"));
	if (InitCheck() != B_OK)
		return InitCheck();

	// If the job has started, we can't configure anything anymore
	if (m.fJobStarted)
		return B_NOT_ALLOWED;

	// Check the settings message
	if (IsSettingsMessageValid((BMessage *)&(m.fSettings.Message())) == false)
	{
		E(bug("BPrintJob: ConfigJob, Invalid printer settings\n"));
		return B_INVALID_PRINT_SETTINGS;
	}
	
	BMessage response;
	status_t result;

#if (NO_PRINT_PANEL == 0)
	BMessage settings(m.fSettings.Message());
	BPrintPanel panel(BPrintPanel::B_CONFIG_JOB, &settings, NULL, NULL, BPrintPanel::B_HIDE_WHEN_DONE | BPrintPanel::B_MODAL_PANEL);
	result = panel.Go(response);
#else
	// TODO: here, we must get the current settings (IAD)
	result = get_default_settings();
	return result;
#endif

	if (result != B_OK)
	{ // The user canceled, or there was an error
		m.fCancelRequested = true;
		return result;
	}

	SetSettings(new BMessage(response));

	// TODO: Check the page ranges are correct
	return B_OK;
}

//------------------------------------------------------------------

status_t BPrintJob::ConfigPage()
{
	E(check_status("ConfigPage"));
	if (InitCheck() != B_OK)
		return InitCheck();

	// If the job has started, we can't configure anything anymore
	if (m.fJobStarted)
		return B_NOT_ALLOWED;

	BMessage response;
	status_t result;

#if (NO_PRINT_PANEL == 0)
	BMessage settings(m.fSettings.Message());
	BPrintPanel panel(BPrintPanel::B_CONFIG_PAGE, &settings, NULL, NULL, BPrintPanel::B_HIDE_WHEN_DONE | BPrintPanel::B_MODAL_PANEL);
	result = panel.Go(response);
#else
	// TODO: here, we must get the current settings (IAD)
	result = get_default_settings();
	return result;
#endif

	if (result != B_OK)
	{ // The user canceled, or there was an error
		m.fCancelRequested = true;
		return result;
	}

	// Save the settings
	SetSettings(new BMessage(response));

	return B_OK;
}

status_t BPrintJob::CommitJob()
{
	if (dynamic_cast<BDirectPrintJob *>(this))
		return B_UNSUPPORTED;

	// Check the state of the BPrintJob
	E(check_status("CommitJob"));
	if (InitCheck() != B_OK)
		return InitCheck();

	// Make sure user has not aborted the job
	if (CanContinue() == false)
		return B_CANCELED;

	// Write the last page
	status_t result;
	if (!m.raw)
	{
		if ((result = EndLastPage()) != B_OK)
		{
			E(bug("BPrintJob: CommitJob, EndLastPage() failed\n"));
			CleanUpSpoolData();
			return result;
		}

		// Have we any page to print?
		if (m.current_header.page_count == 0)
		{ // this is actually an error, that should return an error code
			E(bug("BPrintJob: CommitJob, Invalid page range.\n"));
			CleanUpSpoolData();
			return ERANGE;
		}
	}

	BNode *spool_node = NULL;
	if (m.spoolFile) spool_node = dynamic_cast<BNode *>(m.spoolFile);

	// Write the Attributes to the spoolfile
	if (m.spoolFile)
	{ // We may not have a spoolFile if we are in standalone mode
		// Here, don't write the "status" attribute. It's creation starts the job.
		uint32 flags = ((m.fPreview == true) ? 0x01 : 0);	// TODO: use a define common with PrintEnv.h here
		int32 err = B_OK;
		app_info theInfo;
		be_app->GetAppInfo(&theInfo);
		BPath path(m.spool_file_name);
		if (spool_node) {
			spool_node->WriteAttr(PSRV_SPOOL_ATTR_JOB_NAME,		B_STRING_TYPE, 0, path.Leaf(), strlen(path.Leaf())+1);
			spool_node->WriteAttr(PSRV_SPOOL_ATTR_MIMETYPE,		B_STRING_TYPE, 0, theInfo.signature, strlen(theInfo.signature)+1);
			spool_node->WriteAttr(PSRV_SPOOL_ATTR_ERRCODE,		B_SSIZE_T_TYPE,0, &err, 4);
			spool_node->WriteAttr(PSRV_SPOOL_ATTR_DESCRIPTION,	B_STRING_TYPE, 0, m.print_job_name, strlen(m.print_job_name)+1);
			spool_node->WriteAttr(PSRV_SPOOL_ATTR_PRINTER,		B_STRING_TYPE, 0, m.fPrinterName.String(), m.fPrinterName.Length()+1);
			spool_node->WriteAttr(PSRV_SPOOL_ATTR_PREVIEW,		B_UINT32_TYPE, 0, &flags, 4);
			spool_node->WriteAttr(PSRV_SPOOL_ATTR_PAGECOUNT,	B_INT32_TYPE,  0, &(m.current_header.page_count), sizeof(m.current_header.page_count));
		}
	}

	if (m.fStandAlone)
	{
		status_t err;	
		image_id addon_image = err = load_driver_addon(&PrinterNode(), spool_node);
		if (addon_image < 0)
		{ // Set an error code on the spool job
			err = ((err==B_ENTRY_NOT_FOUND) || (err==B_FILE_NOT_FOUND)) ? (B_NO_DRIVER) : (B_BAD_DRIVER);
			if ((m.spoolFile) && (spool_node))
			{ // To mimic the print_server behaviour
				spool_node->WriteAttr(PSRV_SPOOL_ATTR_ERRCODE, B_SSIZE_T_TYPE, 0, &err, sizeof(err));
				spool_node->WriteAttr(PSRV_SPOOL_ATTR_STATUS, B_STRING_TYPE, 0, PSRV_JOB_STATUS_FAILED, strlen(PSRV_JOB_STATUS_FAILED)+1);
			}
			return err;
		}

		BFile *spool = NULL;
		if (m.spoolFile) {
			BFile *file = dynamic_cast<BFile *>(m.spoolFile);
			if (file)
				spool = new BFile(*file);
		}

		BNode *printer = new BNode(PrinterNode());
		BMessage *threadData = new BMessage;
		threadData->AddMessage("setup_msg", &(m.fSettings.Message()));
		threadData->AddInt32("page_count", m.current_header.page_count);
		threadData->AddPointer("header_list", m.fPageHeadersList);	
		threadData->AddPointer("picture_list", m.fPagePicturesList);	
		threadData->AddPointer("printer", (void *)printer);
		threadData->AddPointer("spool", (void *)spool);
		threadData->AddString("spoolname", m.spool_file_name);
		threadData->AddInt32("image", addon_image);
		thread_id tid = spawn_thread((thread_func)_take_job_add_on_thread, "be:add_on_thread", B_NORMAL_PRIORITY, (void *)threadData);
		if (tid < 0)
		{ // There was an error launching the thread??
			// Set an error code to the spool file and set the job to "Failed"
			E(bug("BPrintJob: CommitJob, spawn_thread() failed\n"));
			CleanUpSpoolData();
			delete printer;
			delete spool;
			delete threadData;
			return (status_t)tid;
		}
		resume_thread(tid);
	}
	else
	{
		// This is an error here to not have a spool file
		if (m.spoolFile == NULL)
			return B_ERROR;
		m.spoolFile->Seek(0, SEEK_SET);
		m.spoolFile->Write(&(m.current_header), sizeof((m.current_header)));	
		// Here, the print_server will see that there
		// is a new Job, thanks to node monitoring
		if (spool_node)
			spool_node->WriteAttr(PSRV_SPOOL_ATTR_STATUS, B_STRING_TYPE, 0, PSRV_JOB_STATUS_WAITING, strlen(PSRV_JOB_STATUS_WAITING)+1);
	}

	// Delete the spoolfile's BFile
	delete m.spoolFile;
	m.spoolFile = NULL;
	
	// Put the BPrintJob in it's original state, ready to be used again!
	m.re_init();

	return B_OK;
}

long BPrintJob::_take_job_add_on_thread(BMessage *threadData)
{
	status_t err;
	const char *spool_file_name = NULL;
	BFile *spool = NULL;
	BNode *printer_node = NULL;
	BList *headerList = NULL;
	BList *pictureList = NULL;
	image_id addon_image;
	threadData->FindInt32("image", &addon_image);
	threadData->FindPointer("printer", (void **)&printer_node);
	threadData->FindPointer("spool", (void **)&spool);
	threadData->FindString("spoolname", &spool_file_name);
	threadData->FindPointer("header_list", (void **)&headerList);	
	threadData->FindPointer("picture_list", (void **)&pictureList);	
	
	BMessage *(*take_job_msg)(BNode*, BMessage*);
	BPrinterAddOn *(*instantiate_printer_addon)(int32, BTransportIO *, BNode *);

	// Mark the job as processing
	if (spool) { // If we have a job, mark it as 'processing'
		spool->WriteAttr(PSRV_SPOOL_ATTR_STATUS, B_STRING_TYPE, 0, PSRV_JOB_STATUS_PROCESSING, strlen(PSRV_JOB_STATUS_PROCESSING)+1);
	} else { // If not, mark the 'printer' as processing
		printer_node->WriteAttr(PSRV_SPOOL_ATTR_STATUS, B_STRING_TYPE, 0, PSRV_JOB_STATUS_PROCESSING, strlen(PSRV_JOB_STATUS_PROCESSING)+1);
	}

	// find the instantiate addon function
	if ((err = get_image_symbol(addon_image, B_INSTANTIATE_PRINTER_ADDON_FUNCTION, B_SYMBOL_TYPE_TEXT, (void **)&instantiate_printer_addon)) == B_OK)
	{ // Instantiate that addon
		BTransportIO transport(printer_node, spool, IO_BUFFER_SIZE);
		if ((err = transport.InitCheck()) == B_OK)
		{
			BPrinterAddOn *printerAddOn;
			if ((printerAddOn = instantiate_printer_addon(0, &transport, printer_node)) != NULL)
			{
				err = printerAddOn->TakeJob(threadData);
			}
			else
				err = B_BAD_DRIVER;
			delete printerAddOn;
		}
	}
	else if ((err = get_image_symbol(addon_image, "take_job_msg", B_SYMBOL_TYPE_TEXT, (void**)&take_job_msg)) == B_OK)
	{ // Try the old way of handling print job for compatibility
		delete (take_job_msg(printer_node, threadData));
	}

	// Free the printer
	PrinterManager printerManager(true);
	printerManager.set_printer_free(printer_node);

	if (spool)
	{ // Set the job status
		if ((err != B_OK) && (err != B_CANCELED))
		{ // There was an error. Set the attribute and don't delete the spoolfile.
			spool->WriteAttr(PSRV_SPOOL_ATTR_ERRCODE, B_SSIZE_T_TYPE, 0, &err, sizeof(err));
			spool->WriteAttr(PSRV_SPOOL_ATTR_STATUS, B_STRING_TYPE, 0, PSRV_JOB_STATUS_FAILED, strlen(PSRV_JOB_STATUS_FAILED)+1);
		}
		else
		{ // Job completed correctly, remove the spoolfile.
			delete printer_node;
			printer_node = NULL;
			if (spool_file_name)
				unlink(spool_file_name);
		}
	}
	else
	{ // Make sure to clear the cancel status
		TPrintTools::ClearCancel(printer_node);	
	}

	// Cleanup everything...
	cleanup_spool_list(headerList, pictureList);
	unload_add_on(addon_image);
	delete printer_node;
	delete spool;
	delete threadData; // destroy it last.
	return err;
}

//------------------------------------------------------------------
// #pragma mark -

status_t BPrintJob::GetResolution(int32 *xdpi, int32 *ydpi)
{
	E(check_status("GetResolution"));
	if (InitCheck() != B_OK)
		return InitCheck();

	*xdpi = *ydpi = -1;
	status_t result = B_OK;
	if (m.fSettings.IsEmpty() && ((result = get_default_settings()) != B_OK))
		return result;

	*xdpi = m.fSettings.DeviceXdpi();
	*ydpi = m.fSettings.DeviceYdpi();

	return B_OK;
}

//------------------------------------------------------------------

int32 BPrintJob::FirstPage()
{
	E(check_status("FirstPage"));
	if (InitCheck() != B_OK)
		return InitCheck();

	if (m.fSettings.IsEmpty() && (get_default_settings() != B_OK))
		return 1;

	return max_c(1, m.fSettings.FirstPage());
}

//------------------------------------------------------------------

int32 BPrintJob::LastPage()
{
	E(check_status("LastPage"));
	if (InitCheck() != B_OK)
		return InitCheck();

	if (m.fSettings.IsEmpty() && (get_default_settings() != B_OK))
		return LONG_MAX;

	return min_c(LONG_MAX, m.fSettings.LastPage());
}

//------------------------------------------------------------------

int32 BPrintJob::PrinterType(void *) const
{
	return m.fSettings.Color();
}

//------------------------------------------------------------------

BRect BPrintJob::PaperRect()
{
	E(check_status("PaperRect"));

	if (m.fSettings.IsEmpty())
		get_default_settings();

	return m.fSettings.PaperRect();
}

//------------------------------------------------------------------

BRect BPrintJob::PrintableRect()
{
	E(check_status("PrintableRect"));

	if (m.fSettings.IsEmpty())
		get_default_settings();

	return m.fSettings.PrintableRect();
}

//------------------------------------------------------------------

bool BPrintJob::CanContinue()
{
	return (m.fCancelRequested == false);
}

//------------------------------------------------------------------
// #pragma mark -

void BPrintJob::CancelJob()
{
	m.fCancelRequested = true;
	CleanUpSpoolData();
}

//------------------------------------------------------------------

status_t BPrintJob::BeginJob()
{
	if (dynamic_cast<BDirectPrintJob *>(this))
		return B_UNSUPPORTED;

	E(check_status("BeginJob"));
	if (InitCheck() != B_OK)
		return InitCheck();

	// If we still have no settings, it's time to get the default.
	if (m.fSettings.IsEmpty())
		get_default_settings();

	m.current_header.version = B_SPOOL_VERSION;
	m.current_header.page_count = 0;
	m.current_header.first_page = -1;
	m.current_header.type = ((m.raw) ? (spool_header_t::RAW) : (spool_header_t::PICTURE));
	m.current_header.flags = 0;
	m.current_header._reserved_3_ = 0;
	m.current_header._reserved_4_ = 0;
	m.current_header._reserved_5_ = 0;

	// Create the spoolfile, even in StandAlone mode (just for UI and error codes)
	MangleName(m.spool_file_name);
	BPath path;
	find_directory(B_USER_PRINTERS_DIRECTORY, &path, true);
	path.Append(m.fPrinterName.String());
	path.Append(m.spool_file_name);
	strcpy(m.spool_file_name, path.Path());
	status_t result;
	
	// don't create the spool file on BeIA
	#if (M_CREATE_SPOOLFILE)
	m.spoolFile = new BFile(m.spool_file_name, B_CREATE_FILE | B_FAIL_IF_EXISTS | B_READ_WRITE);
	#else
	m.spoolFile = NULL;
	#endif


	// We have a printer manager, then use it!
	// (This means that the print-server is not here [we're in stand alone mode])
	if (m.fPrinterManager)
	{
		if (m.fPrinterManager->is_printer_free(&PrinterNode()) == false)
		{ // Cancel the print-job, since there's nothing else to do.
			CancelJob();
			return B_PRINTER_BUSY;
		}
		// TODO: there's a little race-condition here - should fix it.
		m.fPrinterManager->set_printer_busy(&PrinterNode(), dynamic_cast<BFile *>(m.spoolFile));
	}


	if (m.fStandAlone)
	{
		m.fPageHeadersList = new BList;
		m.fPagePicturesList = new BList;
		BFile *spoolfile = dynamic_cast<BFile *>(m.spoolFile);
		if ((spoolfile) && (spoolfile->InitCheck() != B_OK))
		{ // This is not an error in standalone mode, just delete the BFile and continue
			delete m.spoolFile;
			m.spoolFile = NULL;
		}
	}
	else
	{
		BFile *spoolfile = dynamic_cast<BFile *>(m.spoolFile);
		if (spoolfile)
		{ // Our spoolfile is not a BFile, so assume there was no error.
			if ((result = spoolfile->InitCheck()) != B_OK)
			{ // Spool file not created! (gasp!)
				delete m.spoolFile;
				m.spoolFile = NULL;
				return result;
			}
		}
	}	

	if (m.spoolFile)
	{ // Set the type of the spool file
		BFile *spoolfile = dynamic_cast<BFile *>(m.spoolFile);
		if (spoolfile) {
			BNodeInfo info(spoolfile);
			info.SetType(PSRV_PRINTER_SPOOL_MIMETYPE);
		}

		if (m.fStandAlone == false)
		{ // But in standalone mode, we don't write into it (not necessary)
			m.spoolFile->Write(&m.current_header, sizeof(m.current_header));
			// don't add the setting message in raw mode
			if (!m.raw) {
				AddSetupSpec();
			}

			m.current_header.first_page = m.spoolFile->Position();

			if (!m.raw) {
				m.m_curPageHeaderOffset = m.current_header.first_page;
				m.spoolFile->Write(m.m_curPageHeader, sizeof(_page_header_));
			}
		}
	}

	// Just so that we know we successfuly started a print-job
	m.fJobStarted = true;
	return B_OK;
}

//------------------------------------------------------------------

const BPrintJobSettings& BPrintJob::JobSettings()
{
	if (m.fSettings.IsEmpty())
		get_default_settings();
	return m.fSettings;
}

BMessage *BPrintJob::Settings()
{
	return new BMessage(JobSettings().Message());
}

//------------------------------------------------------------------

status_t BPrintJob::SetSettings(BMessage *setup)
{
	// If the job has started, we can't configure anything anymore
	if (m.fJobStarted)
		return B_NOT_ALLOWED;

	if (setup == NULL)
	{ // Clear the current settings
		m.fSettings.ClearSettings();
		return B_OK;
	}
	
	// update the settings message
	update_settings(*setup);
	delete setup;

	// Update the printer
	return SetPrinter(m.fSettings.PrinterName().String());
}

//------------------------------------------------------------------

bool BPrintJob::IsSettingsMessageValid(BMessage *msg) const
{
	// TODO: here we could be more clever (for eg: we may return false if the msg is not at all a printer setting message)
	if (msg == NULL)
		return false;
	return true;
}

//------------------------------------------------------------------

status_t BPrintJob::SpoolPage()
{
	if (dynamic_cast<BDirectPrintJob *>(this))
		return B_UNSUPPORTED;

	E(check_status("SpoolPage"));
	if (InitCheck() != B_OK)
		return InitCheck();

	if (CanContinue() == false)
		return B_CANCELED;

	if (m.fStandAlone)
	{
		if (m.m_curPageHeader->pictureCount == 0)
			return B_OK;	

		m.m_curPageHeader->nextPage = m.fPagePicturesList->CountItems();
		m.fPageHeadersList->AddItem((void  *)m.m_curPageHeader);

		// New header for the new page
		m.m_curPageHeader = new _page_header_;

		// Next page...
		m.current_header.page_count++;
	}
	else
	{
		if (m.spoolFile == NULL)
			return B_ERROR;
	
		#ifdef DEMO_CD
		BPicture *mark = AddWatermark(paper_size);
		AddPicture(mark, paper_size, B_ORIGIN);
		#endif
	
		if (m.m_curPageHeader->pictureCount == 0)
			return B_OK;
		
		m.current_header.page_count++;
		m.spoolFile->Seek(0, SEEK_END);
		if (m.m_curPageHeaderOffset)
		{
			m.m_curPageHeader->nextPage = m.spoolFile->Position();
			m.spoolFile->Seek(m.m_curPageHeaderOffset, SEEK_SET);
			m.spoolFile->Write(m.m_curPageHeader, sizeof(_page_header_));
			m.spoolFile->Seek(m.m_curPageHeader->nextPage, SEEK_SET);
		}
	
		m.m_curPageHeader->nextPage = 0;
		m.m_curPageHeader->pictureCount = 0;
		m.m_curPageHeaderOffset = m.spoolFile->Position();
		m.spoolFile->Write(m.m_curPageHeader, sizeof(_page_header_));
	}

	return B_OK;
}


//------------------------------------------------------------------
// #pragma mark -

void BPrintJob::HandleError(status_t error) const
{
#if (HANDLE_ERRORS)
	size_t size;
	const char *string;
	const char *ok		= (const char *)GetLibbeResources()->LoadResource(B_STRING_TYPE, "be:print:ok", &size);
	const char *cancel	= (const char *)GetLibbeResources()->LoadResource(B_STRING_TYPE, "be:print:cancel", &size);
	const char *yes		= (const char *)GetLibbeResources()->LoadResource(B_STRING_TYPE, "be:print:yes", &size);
	const char *no		= (const char *)GetLibbeResources()->LoadResource(B_STRING_TYPE, "be:print:no", &size);

	string = (const char *)GetLibbeResources()->LoadResource(B_STRING_TYPE, "be:print:error", &size); 
	BString s(string);
	switch (error)
	{
		case B_OK:
			return;
		case B_NO_PRINTER:
			s << (const char *)GetLibbeResources()->LoadResource(B_STRING_TYPE, "B_NO_PRINTER", &size);
			if ((new BAlert(NULL, s.String(), no, yes))->Go() == 1)
				run_add_printer_panel();
			return;
		case B_INVALID_PRINT_SETTINGS:
			s << (const char *)GetLibbeResources()->LoadResource(B_STRING_TYPE, "B_INVALID_PRINT_SETTINGS", &size);
			(new BAlert(NULL, s.String(), ok))->Go();
			return;
		case B_INVALID_PRINTER:
			s << (const char *)GetLibbeResources()->LoadResource(B_STRING_TYPE, "B_INVALID_PRINTER", &size);
			(new BAlert(NULL, s.String(), ok))->Go();
			return;
		default:
			s << strerror(error) << " [" << error << "]\n";
	}

	s << (const char *)GetLibbeResources()->LoadResource(B_STRING_TYPE, "B_ERROR", &size);
	(new BAlert(NULL, s.String(), cancel, NULL, NULL, B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_STOP_ALERT))->Go();
#else
	printf("BPrintJob::HandleError() - %s [%08lx]\n", strerror(error), error);
#endif
}


//------------------------------------------------------------------
// #pragma mark -


void BPrintJob::MangleName(char *filename)
{
	int32 a_time = (system_time()/1000);
	if (a_time < 0)
		a_time = -a_time;

	sprintf(filename, "%s@%ld", m.print_job_name, a_time);

	// remove slashes !
	char c;
	char *p = filename;
	while((c = *p) != 0)
	{
		if (c == '/')
			*p = 'x';
		p++;
	}
}

//------------------------------------------------------------------

void BPrintJob::AddSetupSpec()
{
	if (m.fSettings.IsEmpty() == false)
		m.fSettings.Message().Flatten(m.spoolFile);
}

//------------------------------------------------------------------

status_t BPrintJob::update_settings(const BMessage& setup)
{
	if (setup.IsEmpty())	m.fSettings.ClearSettings();
	else					m.fSettings.SetSettings(setup);
	return B_OK;
}


//------------------------------------------------------------------
// #pragma mark -


#ifdef DEMO_CD

float GetMaxFontSize(BRect view_size, char *str, BView *view)
{
	float w = view_size.Width();
	float h = view_size.Height();
	float hyp_len = sqrt(w*w + h*h);
	float strWidth;	
	float fSize = ceil(hyp_len / strlen(str));
	view->PushState();
		do {
			fSize += 2;
			view->SetFontSize(fSize);
			strWidth = view->StringWidth(str);		
		} while(strWidth +fSize < hyp_len);
	view->PopState();	
	return (fSize-2);
}

float GetWatermarkAngle(BRect view_size)
{
	float w = view_size.Width();
	float h = view_size.Height();

	float theta = atan(h/w) * (180.0 / M_PI);
	return (-theta);
}

BFont* GetWatermarkFont()
{
	BFont *wmFont;
	font_family family;
	font_style style;

	/* search for family and style; return if found */
	int32 numFamilies = count_font_families();
	for(int32 i=0; i < numFamilies; i++) {
		if(get_font_family(i, &family) == B_OK) {
			if(strcmp(family, "Baskerville") == 0) {
				int32 numStyles = count_font_styles(family);
				for(int32 j=0; j < numStyles; j++) {
					if(get_font_style(family, j, &style) == B_OK) {
						if(strcmp(style, "Roman") == 0) {
							wmFont = new BFont;
							wmFont->SetFamilyAndStyle(family, style);
							return wmFont;
						}	
					}					
				}
			}
		}
	}
	
	/* Font not found, use be_plain_font */
	wmFont = new BFont(be_plain_font);
	return wmFont;
	
}

BPicture* AddWatermark(BRect p_size)
{

	BWindow *win = new BWindow(BRect(0,0,10,10), "win", B_TITLED_WINDOW, 0);
	BView *view = new BView(p_size, "v", 0, B_WILL_DRAW);

	view->SetViewColor(B_TRANSPARENT_32_BIT);
	win->AddChild(view);

	static bool firstPass = true;
	static float fontsize;
	static float angle;
	BPoint pt;

	if(firstPass) {	
		fontsize = GetMaxFontSize(p_size, " BeOS Demo CD ", view);
		angle = GetWatermarkAngle(p_size);
		firstPass = false;
	}

	if(angle <= -45.0) {
		pt.x = 0;
		pt.y = (fontsize/2)/sin(-angle*M_PI/180.0);
	} else {
		pt.y = fontsize * sin(-angle*M_PI/180.0);
		pt.x = (fontsize/2)/sin(-angle*M_PI/180.0);
		pt.x -= fontsize * cos(-angle*M_PI/180.0);
	}
			
	BFont *font = GetWatermarkFont();	
	font->SetSize(fontsize);
	font->SetRotation(angle);

	win->Lock();	

	view->BeginPicture(new BPicture);
		view->SetHighColor(0,0,0,64);
		view->SetDrawingMode(B_OP_ALPHA);

		view->MovePenTo(pt);
		view->SetFont(font);
		view->DrawString(" BeOS Demo CD");
	BPicture *pic = view->EndPicture();

	win->Unlock();	
	win->Quit();
	
	return pic;
}

#else

float GetMaxFontSize(BRect, char *, BView *) { return 0.0f; }
float GetWatermarkAngle(BRect) { return 0.0f; }
BFont* GetWatermarkFont() { return NULL; }
BPicture* AddWatermark(BRect) { return NULL; }

#endif

//------------------------------------------------------------------
// #pragma mark -

void BPrintJob::_ReservedPrintJob1() {}
void BPrintJob::_ReservedPrintJob2() {}
void BPrintJob::_ReservedPrintJob3() {}
void BPrintJob::_ReservedPrintJob4() {}
