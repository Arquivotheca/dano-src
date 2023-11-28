//--------------------------------------------------------------------
//
//   FILE:  PostScript.cpp
//   REVS:  $Revision: 1.8 $
//   NAME:  Benoit
//
//  Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.
//
//  Change history:
//
//	01/28/98	MAV001	Changed PPD directory from B_BEOS_SYSTEM_DIRECTORY
//						to B_BEOS_ETC_DIRECTORY.
//--------------------------------------------------------------------

#include <BeBuild.h>	// must be first include!

#include <fcntl.h>  
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include "Convert.h"

#include <ClassInfo.h>
#include <Debug.h>
#include <Path.h>

#define P // _sPrintf
#define PP // printf

#include <Application.h>
#include <StorageDefs.h>
#include <File.h>
#include <Volume.h>
#include <Directory.h>
#include <Alert.h>
#include <View.h>
#include <TextControl.h>
#include <Bitmap.h>
#include <Region.h>
#include <FilePanel.h>
#include <Node.h>
#include <NodeInfo.h>
#include <String.h>
#include <StopWatch.h>
#include <errno.h>
#include <FindDirectory.h>
#include <InterfaceDefs.h>
#include <PrintJob.h>

#include "printclient.h"
#include "ppd_control.h"
#include "PSGenerator.h"


//------------------------------------------------------------------

extern "C" {
	extern int yylineno;
	extern FILE *yyin;
	extern int yyrestart();
	extern int yyparse();
	extern int yywrap();
#include "parser_interface.h"
}

#include "Postscript.h"
#include "PS_Print.h"
#include "PS_Setup.h"
#include "Status.h"

//------------------------------------------------------------------
// Here is the list of standard functions that you need to export
// to get a working print server addon.
// More functions might be added later but those 3 functions are the
// strict minimum, later functions will be optionals.
//------------------------------------------------------------------

extern "C" _EXPORT BMessage	*take_job(BFile *spool_file, BNode *printer_file, BMessage* msg);
extern "C" _EXPORT BMessage	*config_page(BNode *printer_file, BMessage* msg);
extern "C" _EXPORT BMessage	*config_job(BNode *printer_file, BMessage* msg);
extern "C" _EXPORT char		*add_printer(char *driver_name);
extern "C" _EXPORT BMessage *default_settings(BNode *printer_file, BMessage* msg);

_EXPORT void SendOutput(BDataIO*, const char*);
_EXPORT void SendOutput(BDataIO*, const char*, long size);

void SendOutput(BDataIO *transport, const char *data)
{
	const long size = strlen(data);
	SendOutput(transport, data, size);
}

void SendOutput(BDataIO *transport, const char *data, long size)
{
	transport->Write(data, size);
}


//------------------------------------------------------------------
// Very impressive constructor for the PS... put your stuff here !
//------------------------------------------------------------------

TPrintDrv::TPrintDrv()
{
	entry = NULL;
	a_bitmap = NULL;
}

//------------------------------------------------------------------
// Very impressive destructor too.
//------------------------------------------------------------------

TPrintDrv::~TPrintDrv()
{
	delete (PPD::Instance());
}

//------------------------------------------------------------------

int	TPrintDrv::ReadPPD(const char* ppd_file)
{
	// Opening ppd file
	FILE *file = fopen(ppd_file, "r");
	if (file == NULL)
		return -1;

	yylineno = 1;
	yyin = file;
	yyrestart(file);
	int rv = yyparse();
	if (rv != 0)
	{
		BString str = "Your printer's description file couldn't be ";
		str << "parsed completely.  As a result, some of the printer's ";
		str << "features may be unavailable.";
		(new BAlert("", str.String(), "Uh, thanks."))->Go();
	}
	P("ReadPPD: yyparse return %d.\n", rv);

	yywrap();
	
	fclose(file);
	return rv;
}

//------------------------------------------------------------------

BMessage *TPrintDrv::take_job(BFile *the_file, BNode *printer_file, BMessage *msg)
{
	BMessage *result;
	long error;

	PSGenerator gen(this, printer_file);
	error = gen.HandleJob(the_file);

	if (error >= 0){
		result = new BMessage('okok');
		the_file->WriteAttr("_spool/Status", B_STRING_TYPE, 0, "Completed", strlen("Completed")+1);
	} else if (error == PSGenerator::B_USER_CANCELED) {
		result = new BMessage('okok');
		the_file->WriteAttr("_spool/Status", B_STRING_TYPE, 0, "Cancelled", strlen("Cancelled")+1);
	} else {
		result = new BMessage('baad');
		the_file->WriteAttr("_spool/Status", B_STRING_TYPE, 0, "Failed", strlen("Failed")+1);
	}

	return (result);
}

//------------------------------------------------------------------
// Main entry point for job handling.
// We receive the spool_file, the printer configuration file, and the
// setup message.

BMessage *take_job(BFile* spool_file, BNode *printer_file, BMessage* msg)
{
	TPrintDrv the_drv;
	return the_drv.take_job(spool_file, printer_file, msg);
}

//------------------------------------------------------------------

long TPrintDrv::do_page_setup(BMessage *msg, char *pr_name)
{
	return (new BSetup(msg, pr_name, this))->Go();
}

//------------------------------------------------------------------

BMessage *TPrintDrv::page_setup(BMessage *msg, char *pr_name)
{
	BRect page_size;
	msg = new BMessage(*msg);			//clone it !
	long val = do_page_setup(msg, pr_name);
	if (val < 0) {
		delete msg;
		return NULL;
	}

	msg->what = 'okok';

	if (!msg->HasRect("paper_rect")) {
		page_size.top = 0;
		page_size.left = 0;
		page_size.bottom = 11 * 72;
		page_size.right = 8.5 * 72;
		msg->AddRect("paper_rect", page_size);
	}	
	
	if (!msg->HasRect("printable_rect")) {
		page_size.top = 0.5 * 72;
		page_size.left = 0.5 * 72;
		page_size.bottom = (11-0.5) * 72;
		page_size.right = (8.5-0.5) * 72;
		msg->AddRect("printable_rect", page_size);
	}	

	return(msg);
}

//------------------------------------------------------------------

long TPrintDrv::do_print_setup(BMessage *msg, char *pr_name)
{
	return (new BPrint(msg, pr_name))->Go();
}

//------------------------------------------------------------------

BMessage *TPrintDrv::print_setup(BMessage *msg, char *pr_name)
{
	msg = new BMessage(*msg);			//clone it !
	long val = do_print_setup(msg, pr_name);
	if (val < 0) {
		delete msg;
		return(0);
	}

	msg->what = 'okok';

	return(msg);
}

//------------------------------------------------------------------
BMessage *TPrintDrv::get_default_settings(BNode *printer)
{
	status_t length;
	char ppd_name[B_FILE_NAME_LENGTH];
	
	length = printer->ReadAttr("ppd_name",
							 	B_STRING_TYPE,
							 	0,
							 	ppd_name,
							 	B_FILE_NAME_LENGTH);

	if (length < 0)
		return NULL;

	if (ReadPPD(ppd_name) < 0)
		return NULL;

	BMessage *settings = new BMessage;

	Invocation *inv;
	UI *ui;
	PPD *ppd = PPD::Instance();

	/* Get the Paper Rect */
	ui = ppd->GetUI("PageSize");
	if (ui)
	{
		float width, height;
		const char *def_name = ui->Default();
		inv = ppd->FindInvocation("PaperDimension", def_name, 0);
		if (inv)
		{
			char *dimString = inv->GetFirstString();
			sscanf(dimString, "%f %f", &width, &height);
			delete dimString;
			BRect page_rect(0,0,width,height);
			settings->AddRect("paper_rect", page_rect);		
		}	

		/* Get the Printable Rect */
		ui = ppd->GetUI("PageRegion");
		if (ui)
		{
			const char *def_name = ui->Default();
			inv = ppd->FindInvocation("ImageableArea", def_name, 0);
			if (inv)
			{
				float llx, lly, urx, ury;
				char *dimString = inv->GetFirstString();
				sscanf(dimString, "%f %f %f %f", &llx, &lly, &urx, &ury);
				delete dimString;
	
	
				BRect printable_rect(	llx,
										height-ury,
										urx,
										height-lly);
	
				settings->AddRect("printable_rect", printable_rect);			
			}
		}	
	}

	// add icky resolution
	settings->AddInt32("xres", 300);
 	settings->AddInt32("yres", 300);

	return settings;
}

//------------------------------------------------------------------
// This is the main entry point used to handle dialogs/user interaction
// specifing the way printing should be done for the CURRENT job !
// printer_file is the printer configuration for the driver, the BMessage
// is either a blank message OR a previously saved page setup message
// from a previous job.
// This function will return a new BMessage containing the page setup
// fields + any optional field.
// This BMessage will be passed again to the driver when the job is
// started.
//------------------------------------------------------------------

BMessage *config_page(BNode *printer_file,BMessage* msg)
{
	// We will use the printer name as window name.
	char name[128];
	printer_file->ReadAttr("Printer Name",
							 B_STRING_TYPE,
				  			 0,
							 name,
							 128);

	TPrintDrv the_drv;
	return the_drv.page_setup(msg, name);
}

//------------------------------------------------------------------

BMessage *config_job(BNode *printer_file,BMessage* msg)
{
	// We will use the printer name as window name.
	char name[128];
	printer_file->ReadAttr("Printer Name",
							 B_STRING_TYPE,
				  			 0,
							 name,
							 128);

	TPrintDrv the_drv;
	return the_drv.print_setup(msg, name);
}

//------------------------------------------------------------------

BMessage *default_settings(BNode *printer_file, BMessage *msg)
{
	TPrintDrv the_drv;
	return the_drv.get_default_settings(printer_file);
}


//------------------------------------------------------------------
// Nice to get functions in logical order... this is the foward
// declaration for save_setup.

void save_setup(char *driver_name, char *printer_name, char *ppd_name, char *pad_name);

//------------------------------------------------------------------
// Configure printer is called when the user wants to create a new
// printer accessed with the current driver.
// The driver name is passed to the function since for localisation
// issue, the name might be different from the initial one.
// add_printer will display any dialog/user interaction needed to
// identify/name a printer to be accessed using the driver.

char *add_printer(char *driver_name)
{
	int32 sem = create_sem(0, "print_setup");

	TStatusWindow* selectionWindow = new TStatusWindow(driver_name);
	
	//	find the window that owns this thread
	//	use it for updates later
	thread_id	this_tid = find_thread(NULL);
	BWindow* wind = NULL;

	BLooper* loop = BLooper::LooperForThread(this_tid);
	if (loop)
		wind = cast_as(loop, BWindow);
		
	selectionWindow->Show();

	// wait for the user to select cancel or ok
	while (selectionWindow->NotDone()) {
		if (wind)
			wind->UpdateIfNeeded();
		snooze(50000);
	}
	
	release_sem(sem);
	acquire_sem(sem);

	bool result = selectionWindow->Result();

	char ppd_name[512];
	if (result) {

		//	copy the printer name, address and printer type
		const char *str;

		str = selectionWindow->PPD();
		if (str || strlen(str) > 0)
			strcpy(ppd_name, selectionWindow->PPD());
		else
			ppd_name[0] = 0;

	}	else {
		ppd_name[0] = 0;
	}
	
	// close the dialog.
	selectionWindow->Lock();
	selectionWindow->Close();
	delete_sem(sem);

	// if the name is null or one of the lists was empty or there 
	// wasn't a selection
	// just clean up and exit.
	if (!result	|| 	strlen(ppd_name) == 0)
		return 0;

	// otherwise create a new setup.
	save_setup(NULL, driver_name, ppd_name, NULL);

	return driver_name;
}

//------------------------------------------------------------------
// This function called by add_printer will create a new printer
// file which will define all the info later required by the driver
// to access that printer.

void save_setup(char *driver_name, char *printer_name, char *ppd_name, char *pad_name)
{
	BPath ppd;
	find_directory(B_USER_PRINTERS_DIRECTORY, &ppd);
	ppd.Append(printer_name);

	BDirectory dir(ppd.Path());
	if (dir.InitCheck() != B_OK)
		return;

	dir.WriteAttr("ppd_name", B_STRING_TYPE, 0, ppd_name, strlen(ppd_name) + 1);
}

