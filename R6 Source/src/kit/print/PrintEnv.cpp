// ***********************************************************************
// libbe.so
//
// (c) 2000, Be Incorporated
//
// Written by : Mathias Agopian
//
// ***********************************************************************

#include <stdio.h>
#include <unistd.h>

#include <StorageKit.h>
#include <Application.h>
#include <Roster.h>
#include <Message.h>
#include <String.h>
#include <AppPrefs.h>
#include <Bitmap.h>
#include <View.h>
#include <fs_attr.h>

#include <print/PrinterAddOn.h>
#include <print/PrinterConfigAddOn.h>
#include <print/TransportIO.h>
#include <print/TransportAddOn.h>
#include <print/PrintJobEditSettings.h>
#include <print/PrintPanel.h>

#if (PRINTING_FOR_DESKTOP)
#	include <print/PrintConfigView.h>
#	include "AboutBox.h"	// For the ressources
#endif

#if (PRINTING_FOR_DESKTOP)
#	define M_CREATE_SPOOLFILE	1
#else
#	define M_CREATE_SPOOLFILE	0
#endif

#include <pr_server.h>

#include "ListAddOns.h"
#include "PrintWindows.h"
#include "PrintEnv.h"

#if 0
#	define D(_x)	_x
#else
#	define D(_x)
#endif
#define bug		printf

TPrintTools::TPrintTools()
{
}

TPrintTools::~TPrintTools()
{
}

// --------------------------------------------------------------
// #pragma mark -

status_t TPrintTools::PrinterAddedOrRemoved(const char *device)
{
	const char *transport = NULL;

	// HACK / TODO: We should not hardcode this, however USB is the only
	// transport that supports hotplug.
	// so, let's live with that for now...
	if (strstr(device, "/dev/printer/usb"))
		transport = "Print/transport/USB";
	
	if (transport == NULL)
		return B_ERROR;

	BMessage replyMsg;
	if (BTransportIO::Probe(transport+sizeof("Print/transport"), replyMsg) == B_OK)
	{
		D(replyMsg.PrintToStream());
		ParsePrinter(transport, replyMsg);
	}
	RemovePrinters(); // Remove no more connected printers (unless they have jobs waiting)


	// Is there a default Printer >
	BString printerName;
	if (get_default_printer(printerName) != B_OK)
	{ // no there's not, or it's not valid -> Select the first printer we found
		BPath path;
		find_directory(B_USER_PRINTERS_DIRECTORY, &path);
		BDirectory dir(path.Path());
		BEntry entry;
		if (dir.GetNextEntry(&entry) == B_OK)
		{
			char printerName[256];
			entry.GetName(printerName);
			set_default_printer(printerName);
		}
	}
	else
		return B_OK;

	return get_default_printer(printerName);
}


status_t TPrintTools::AddPrinters()
{
	char *addons = BPrivate::ListAddOns("Print/transport");

	D(bug("AddPrinters() : %s\n", addons));

	if (addons!=NULL)
	{
		char *tok = addons;
		char *ttok;
		while ((tok = strtok_r(tok, ":", &ttok)) != NULL)
		{
			char *transport = tok;
			tok = NULL;

			D(bug("Testing: %s\n", transport));

			BMessage replyMsg;
			if (BTransportIO::Probe(transport+sizeof("Print/transport"), replyMsg) != B_OK)
				continue;
			D(replyMsg.PrintToStream());
			
			ParsePrinter(transport, replyMsg);
		}
	}
	delete addons;

	RemovePrinters(); // Remove no more connected printers (unless they have jobs waiting)
	return B_OK;
}


status_t TPrintTools::ParsePrinter(const char *transport, const BMessage& replyMsg)
{
	// Iterate through each printer found
	int index = 0;
	BMessage printer;
	while (replyMsg.FindMessage(B_TRANSPORT_MSG_PRINTERS, index++, &printer) == B_OK)
	{ // Get the info on each found printer
		char tempPtrinterName[256];
		const char *printerName	= printer.FindString("DEVID:DES:");
		const char *manufac		= printer.FindString("DEVID:MFG:");
		const char *model		= printer.FindString("DEVID:MDL:");
		const char *devicePath	= printer.FindString(B_TRANSPORT_DEV_UNIQUE_NAME);
		if (printerName == NULL)
			printerName	= printer.FindString("DEVID:DESCRIPTION:");

		if (printerName == NULL)
		{ // Build a name from the manufacturer and the model
			sprintf(tempPtrinterName, "%s %s", manufac, model);
			printerName = tempPtrinterName;
		}

		if ((printerName) && (devicePath) && (manufac) && (model))
		{ // try to find an add-on suitable for this printer
			D(bug("\tname         = %s\n", printerName));
			D(bug("\tmanufacturer = %s\n", manufac));
			D(bug("\tmodel        = %s\n", model));
			D(bug("\tTransport    = %s\n", transport));
			D(bug("\tdevice       = %s\n", devicePath));

			BString relative_path;
			if (match_driver(relative_path, manufac, model) == B_OK)
			{ // we've found the corresponding add-on
				D(bug("\taddon        = %s\n", relative_path.String()));
				const char *transportName = transport + sizeof("Print/transport");
				const char *driverName = relative_path.String() + sizeof("Print");
				CreatePrinter(printerName, model, transportName, devicePath, driverName, true);
			}
			else
			{ // here we could create the printer with a flag
			  // that says we didn't find a suitable driver							
			}

		}
	}
	return B_OK;
}


status_t TPrintTools::match_driver(BString& relative_path, const char *mfg, const char *mdl)
{
	int32 index;
	bool mfg_match, mdl_match;
	relative_path = B_EMPTY_STRING;

	char *addons = BPrivate::ListAddOns("Print");
	if (addons)
	{
		char *tok = addons;
		char *ttok;
		while ((tok = strtok_r(tok, ":", &ttok)) != NULL)
		{
			uint32 count = 0;
			BPrinterAddOn::printer_id_t *printerID;
			BPrinterAddOn *(*instantiate_printer_addon)(int32 index, BDataIO *, BNode *);
			BPrinterAddOn *printerAddOn = NULL;
			char *addon_path = tok;
			tok = NULL;

			// make sure this is a loadable image
			image_id image = load_add_on(addon_path);
			if (image < 0)
				continue;
			
			// find the instantiate addon function
			if (get_image_symbol(image, B_INSTANTIATE_PRINTER_ADDON_FUNCTION, B_SYMBOL_TYPE_TEXT, (void **)&instantiate_printer_addon) != B_OK)
				goto error;

			// Instantiate that addon
			index = 0;
			while ((printerAddOn = instantiate_printer_addon(index++, NULL, NULL)) != NULL)
			{
				// Get the device ID
				if ((printerAddOn->GetSupportedDeviceID(&count, &printerID) != B_OK) || (count == 0))
					goto error;
	
				// Does is match?
				for (int i=0 ; i<count ; i++)
				{
					if (printerID[i].mfg == NULL)
						goto error;
					mfg_match = (strcmp(printerID[i].mfg, mfg) == 0);
					mdl_match = (printerID[i].mdl) && (strcmp(printerID[i].mdl, mdl) == 0);				
					if (mfg_match)
					{ // Manufacturer is OK, we'll choose this one if we don't find better
						relative_path = addon_path;
						if (mdl_match)
						{ // That's it! this add-on's OK
							delete printerAddOn;
							unload_add_on(image);
							goto matched;
						}
						break;
					}
				}
			}

		error:
			delete printerAddOn;
			unload_add_on(image);
		}
	}
matched:
	delete addons;
	return ((relative_path != B_EMPTY_STRING) ? (B_OK) : (B_ENTRY_NOT_FOUND));
}


status_t TPrintTools::CreatePrinter(	const char *pn,
										const char *m,
										const char *tn,
										const char *dev,
										const char *dn,
										bool pnp)
{
	return create_printer(pn, m, tn, dev, dn, pnp, 0);
}

status_t TPrintTools::CreatePrinter(	const char *pn,
										const char *m,
										const char *tn,
										const char *dev,
										const char *dn,
										bool pnp,
										int32 attr)
{
	return create_printer(pn, m, tn, dev, dn, pnp, attr);
}

status_t TPrintTools::create_printer(	const char *printer_name,
										const char *model,
										const char *transportName,
										const char *device,
										const char *driverName,
										bool pnp,
										int32 attributes)
{
	bool printer_existed_already = false;
	image_id image = -1;
	BPrinterConfigAddOn *printerAddOn = NULL;
	status_t err;

	// create the printer spool directory
	// As a side effect, a B_NODE_MONITOR message will be sent
	// - to the print_server
	// - to ourself
	//  which will start to monitor this printer
	BPath spool_path;
	find_directory(B_USER_PRINTERS_DIRECTORY, &spool_path, true);
	BDirectory dir(spool_path.Path());
	if (((err = dir.CreateDirectory(printer_name, &dir)) != B_OK) && (err != B_FILE_EXISTS))
		return err;
	if (err == B_FILE_EXISTS)
		printer_existed_already = true;
	spool_path.Append(printer_name);
	BDirectory spoolDir(spool_path.Path());
	if ((err = spoolDir.InitCheck()) != B_OK)
		return err;
	BNodeInfo nodeInfo(&spoolDir);
	nodeInfo.SetType(PSRV_PRINTER_MIMETYPE);	
	
	do
	{
		if ((pnp) && (printer_existed_already))
		{ // the printer already existed. We don't need to update all attributes, etc...
			int32 pnpState = 1;
			spoolDir.WriteAttr(PSRV_PRINTER_ATTR_PNP,	B_UINT32_TYPE, 0, &pnpState, sizeof(uint32));
			break;
		}

		// assign general attributes
		spoolDir.WriteAttr(PSRV_PRINTER_ATTR_DRV_NAME,	B_STRING_TYPE, 0, driverName, strlen(driverName)+1);
		spoolDir.WriteAttr(PSRV_PRINTER_ATTR_PRT_NAME,	B_STRING_TYPE, 0, printer_name, strlen(printer_name)+1);
		spoolDir.WriteAttr(PSRV_PRINTER_ATTR_STATE,		B_STRING_TYPE, 0, PSRV_PRINTER_ATTR_FREE, strlen(PSRV_PRINTER_ATTR_FREE)+1);
		spoolDir.WriteAttr(PSRV_PRINTER_ATTR_TRANSPORT,	B_STRING_TYPE, 0, transportName, strlen(transportName)+1);
		if (device)
			spoolDir.WriteAttr(PSRV_PRINTER_ATTR_TRANSPORT_ADDR, B_STRING_TYPE, 0, device, strlen(device)+1);

		const char *type = (attributes & BTransportAddOn::B_TRANSPORT_IS_NETWORK) ? "Network" : "Local";
		spoolDir.WriteAttr(PSRV_PRINTER_ATTR_CNX, B_STRING_TYPE, 0, type, strlen(type)+1);

		if (model)
			spoolDir.WriteAttr(PSRV_PRINTER_ATTR_MDL, B_STRING_TYPE, 0, model, strlen(model)+1);
		if (pnp)
		{
			int32 pnpState = 1;
			spoolDir.WriteAttr(PSRV_PRINTER_ATTR_PNP,	B_UINT32_TYPE, 0, &pnpState, sizeof(uint32));
		}

		// Load the add-on
		BString addon_path = "Print/";
		addon_path << driverName;
		if ((err = (status_t)load_add_on(addon_path.String())) < B_OK)
			break;
		
		// find the instantiate addon function
		BPrinterConfigAddOn *(*instantiate_printer_config_addon)(BTransportIO *, BNode *);
		char *(*add_printer)(const char *driver_name);

		image = (image_id)err;
		if ((err = get_image_symbol(image, B_INSTANTIATE_PRINTER_CONFIG_ADDON_FUNCTION, B_SYMBOL_TYPE_TEXT, (void **)&instantiate_printer_config_addon)) == B_OK)
		{
			BTransportIO transport(&spoolDir);
			if ((err = transport.InitCheck()) != B_OK)
				break;
			
			// Instantiate that addon
			if ((printerAddOn = instantiate_printer_config_addon(&transport, &spoolDir)) == NULL)
			{
				err = B_ERROR;
				break;
			}
			if (pnp)
			{
				if ((err = printerAddOn->AddPrinterQuiet(printer_name, &spoolDir)) != B_OK)
				{   // TODO: Doesn't remove the printer from the list, but we should show the user that there's
					// no driver suitable for this printer (an error message from the driver would be good)
					D(bug("ERROR from the printer driver: %s\n", err);)
				}
			}
			else
			{
#if (PRINTING_FOR_DESKTOP)
				BPrintConfigView *addPrinterView = printerAddOn->AddPrinter(printer_name, &spoolDir);
				BPrintConfigView *transportView = transport.GetConfigView();
				if (addPrinterView || transportView)
				{
					BMessage result;
					BPrintJobEditSettings settings;
					settings.SetPrinterName(printer_name);					
					BPrintPanel panel(BPrintPanel::B_CONFIG_PRINTER, &settings.Message(), NULL, NULL, BPrintPanel::B_HIDE_WHEN_DONE|BPrintPanel::B_MODAL_PANEL);
					if (addPrinterView)	panel.AddPane(BPrintPanel::B_APP_OPTION_PANE, addPrinterView);
					if (transportView)	panel.AddPane(BPrintPanel::B_APP_OPTION_PANE, transportView);
					err = panel.Go(result);
				}
#else
				err = B_NOT_ALLOWED;
#endif
			}
		}
		else if ((err = get_image_symbol(image, "add_printer", B_SYMBOL_TYPE_TEXT, (void **)&add_printer)) == B_OK)
		{ // Just for compatibility
			if (add_printer(printer_name) == NULL)
				err = B_CANCELED;				

#if (PRINTING_FOR_DESKTOP)
			BTransportIO transport(&spoolDir);
			if ((err = transport.InitCheck()) != B_OK)
				break;
			BPrintConfigView *transportView = transport.GetConfigView();
			if (transportView)
			{
				BMessage result;
				BPrintJobEditSettings settings;
				settings.SetPrinterName(printer_name);					
				BPrintPanel panel(BPrintPanel::B_CONFIG_PRINTER, &settings.Message(), NULL, NULL, BPrintPanel::B_HIDE_WHEN_DONE|BPrintPanel::B_MODAL_PANEL);
				if (transportView)	panel.AddPane(BPrintPanel::B_APP_OPTION_PANE, transportView);
				err = panel.Go(result);
			}
#endif								
		}
	} while (false);

	if (err != B_OK)
	{
		// delete the spool directory
		// As a side effect, a B_NODE_MONITOR message will be sent
		// - to the print_server which will stop to monitor this printer
		// - to ourself which will update its list
		rmdir(spool_path.Path());
	}

	delete printerAddOn;
	if (image > 0)
		unload_add_on(image);
	return err;
}

status_t TPrintTools::RemovePrinters()
{ // Remove no more connected printers (unless they have jobs waiting)
	BPath settingsPath;
	find_directory(B_USER_PRINTERS_DIRECTORY, &settingsPath);
	BEntry entry;
	BDirectory dir(settingsPath.Path());
	while (dir.GetNextEntry(&entry) == B_OK)
	{
		BNode node(&entry);
		uint32 pnpState;
		if (node.ReadAttr(PSRV_PRINTER_ATTR_PNP, B_UINT32_TYPE, 0, &pnpState, sizeof(uint32)) == sizeof(uint32))
		{ // Only try to remove PnP printers (the ones that have been added by CreatePrinter() )
			if (pnpState == 1) // Mark the printer as "removable" for the next time
			{
				pnpState = 0;
				node.WriteAttr(PSRV_PRINTER_ATTR_PNP,	B_UINT32_TYPE, 0, &pnpState, sizeof(uint32));
			}
			else if (pnpState == 0) // Must remove that printer, because this is a PnP printer and it was not detected this time
				entry.Remove(); // will fail if jobs are waiting (which is what we want)
		}
	}
	return B_OK;
}

status_t TPrintTools::GetTransports(BMessage *transports)
{
	transports->MakeEmpty();
	char *addons = BPrivate::ListAddOns("Print/transport");

	D(bug("GetTransports() : %s\n", addons));

	if (addons!=NULL)
	{
		char *tok = addons;
		char *ttok;
		while ((tok = strtok_r(tok, ":", &ttok)) != NULL)
		{
			char *transport = tok;
			tok = NULL;

			D(bug("Testing: %s\n", transport));

			BMessage replyMsg;
			const char *fname = transport + sizeof("Print/transport");
			if (BTransportIO::Probe(fname, replyMsg) != B_OK)
				continue;
			
			D(replyMsg.PrintToStream());

			replyMsg.AddString("filename", fname);
			transports->AddMessage("transports", &replyMsg);
		}
	}

	delete addons;
	return B_OK;
}

image_id TPrintTools::LoadDriverAddOn(BNode *printer, BNode *spool)
{
	BString path("Print/");
	bool preview = false;
	ssize_t err;

	// Findout if we are in preview mode
	if (spool)
	{
		uint32 flags;
		if (spool->ReadAttr(PSRV_SPOOL_ATTR_PREVIEW, B_UINT32_TYPE, 0, &flags, 4) == 4)
			preview = ((flags & 0x1) != 0);	// TODO: use a define instead of 0x01
	}

	if (preview == false)
	{
		// Get the driver name attribute
		char driver_name[B_FILE_NAME_LENGTH];
		err = printer->ReadAttr(PSRV_PRINTER_ATTR_DRV_NAME, B_STRING_TYPE, 0, driver_name, sizeof(driver_name));
		if (err < 0) 		return err;
		else if (err == 0)	return B_ERROR;
		path << driver_name;
	}
	else
	{
		path << "Preview";		// TODO: we shouldn't hardcode the preview driver's name
	}

	return load_add_on(path.String());
}

// This called by Wagner to make sure all printers are free at boot time
status_t TPrintTools::FreeAllPrinters()
{
#if (PRINTING_FOR_DESKTOP)
	return B_UNSUPPORTED;
#else
	status_t result;
	BPath path;
	find_directory(B_USER_PRINTERS_DIRECTORY, &path);
	BDirectory printers(path.Path());
	if ((result = printers.InitCheck()) != B_OK)
		return B_OK; // No printer installed

	PrinterManager manager(true);
	BEntry entry;
	while (printers.GetNextEntry(&entry) == B_OK)
	{ // Iterate over all printers and set them free
		BNode a_printer(&entry);
		manager.set_printer_free(&a_printer);
	}
	return B_OK;
#endif
}

// -----------------------------------------------------------------
// #pragma mark -

status_t TPrintTools::ClearCancel(BNode *printer)
{
	if (printer)
	{ // Clear the cancel status of the printer
		printer->RemoveAttr(PSRV_SPOOL_ATTR_STATUS);
		return B_OK;
	}
	return B_BAD_VALUE;
}

bool TPrintTools::CancelRequested(BNode *printer, BNode *spool)
{
	if (spool)
	{ // We have a spoolfile
		char status[256];		
		if (spool->ReadAttr(PSRV_SPOOL_ATTR_STATUS, B_STRING_TYPE, 0, status, sizeof(status)) > 0)
			if (!strcmp(status, PSRV_JOB_STATUS_CANCELLING))
				return true;
		return false;
	}
	else if (printer)
	{ // We don't have a spool file. So the Canceling attribute should be availlable on the printer's node
		char status[256];		
		if (printer->ReadAttr(PSRV_SPOOL_ATTR_STATUS, B_STRING_TYPE, 0, status, sizeof(status)) > 0)
			if (!strcmp(status, PSRV_JOB_STATUS_CANCELLING))
				return true;
		return false;
	}
	return false;	// Should not be there. both printer and spool are NULL!
}


status_t TPrintTools::CancelJob(BNode *spool, bool)
{ // Request cancelling of this job
	if (!spool)
		return B_BAD_VALUE;

	ssize_t result;
	char status[256];		
	if ((result = spool->ReadAttr(PSRV_SPOOL_ATTR_STATUS, B_STRING_TYPE, 0, status, sizeof(status))) < 0)
		return result;

	if (!strcmp(status, PSRV_JOB_STATUS_COMPLETED))		return B_OK;
	if (!strcmp(status, PSRV_JOB_STATUS_CANCELLED))		return B_OK;
	if (!strcmp(status, PSRV_JOB_STATUS_FAILED))		return B_OK;
	if (!strcmp(status, PSRV_JOB_STATUS_WAITING))		return B_OK;
	if (!strcmp(status, PSRV_JOB_STATUS_CANCELLING))	return B_OK;

	result = spool->WriteAttr(PSRV_SPOOL_ATTR_STATUS, B_STRING_TYPE, 0, PSRV_JOB_STATUS_CANCELLING, strlen(PSRV_JOB_STATUS_CANCELLING)+1);
	if (result < 0)
		return (status_t)result;
	return B_OK;
}

status_t TPrintTools::CancelJobs(BNode *printer, bool synchronous)
{ // Walk through all job on this printer and mark them as cancelling
	
	#if (M_CREATE_SPOOLFILE == 0)

		ssize_t result;
		char status[256];		
		if ((result = printer->ReadAttr(PSRV_SPOOL_ATTR_STATUS, B_STRING_TYPE, 0, status, sizeof(status))) < 0)
			return B_OK;	// If the attribute doesn't exsits that means that there was no printjob

		// Mark this job as cancelling...
		result = printer->WriteAttr(PSRV_SPOOL_ATTR_STATUS, B_STRING_TYPE, 0, PSRV_JOB_STATUS_CANCELLING, strlen(PSRV_JOB_STATUS_CANCELLING)+1);
		if (result < 0)
			return (status_t)result;

		// Here we must wait if synchronous==true
		if (synchronous)
		{
			const bigtime_t last = system_time();
			const bigtime_t TIMEOUT = 5000000; // 5 sec max
			do
			{ // This is an active waiting. It's acceptable here. Node Monitoring would be better
				if (CancelRequested(printer, NULL) == false)
					return B_OK;
				snooze(500000); // wait 1/2 sec
			} while((system_time()-last) < TIMEOUT);
			return B_TIMED_OUT;
		}

		return B_OK;

	#else

		node_ref ref;
		status_t result = printer->GetNodeRef(&ref);
		if (result != B_OK)
			return result;

		BDirectory dir(&ref);
		if ((result = dir.InitCheck()) != B_OK)
			return result;
		
		BEntry entry;
		while (dir.GetNextEntry(&entry) == B_OK)
		{
			BNode job(&entry);
			CancelJob(&job, synchronous);
		}
		return B_OK;

	#endif
}

status_t TPrintTools::CancelJobs(const char *printer_name, bool synchronous)
{ // Cancels all jobs of this printer
	BNode printer;
	if (GetPrinterNode(printer_name, &printer) !=  B_OK)
		return B_INVALID_PRINTER;
	return CancelJobs(&printer, synchronous);
}

status_t TPrintTools::CancelAllJobs(bool synchronous)
{ //Cancels all jobs of all printers
	status_t result;
	BPath path;
	find_directory(B_USER_PRINTERS_DIRECTORY, &path);
	BDirectory printers(path.Path());
	if ((result = printers.InitCheck()) != B_OK)
		return B_NO_PRINTER;

	BEntry entry;
	while (printers.GetNextEntry(&entry) == B_OK)
	{
		BNode a_printer(&entry);
		CancelJobs(&a_printer, synchronous);
	}
	return B_OK;
}

status_t TPrintTools::GetPrinterNode(const char *printer_name, BNode *printer)
{
	// if printer_name==NULL, try to use the default printer
	BString current_printer;
	status_t result;
	if (printer_name == NULL)
	{ // Get the default printer's name
		if (((result = get_default_printer(current_printer)) != B_OK)  || (current_printer.Length() == 0))
			return B_BAD_VALUE;
		printer_name = current_printer.String();
	}
	// Get printer's node
	BPath path;
	find_directory(B_USER_PRINTERS_DIRECTORY, &path);
	path.Append(printer_name);
	return printer->SetTo(path.Path());
}

status_t TPrintTools::GetPrinterSettingsNode(const char *printer_name, BNode *printer)
{
	// if printer_name==NULL, try to use the default printer
	BString current_printer;
	status_t result;
	if (printer_name == NULL)
	{ // Get the default printer's name
		if (((result = get_default_printer(current_printer)) != B_OK)  || (current_printer.Length() == 0))
			return B_BAD_VALUE;
		printer_name = current_printer.String();
	}
	// Get printer's node
	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	path.Append("Be/PRNT");
	create_directory(path.Path(), 0777);
	path.Append(printer_name);
	BFile file(path.Path(), B_READ_WRITE | B_CREATE_FILE);
	return printer->SetTo(path.Path());
}


// --------------------------------------------------------------
// #pragma mark -

status_t TPrintTools::LoadSettings(const char *printer_name, BMessage& settings, const char *user)
{
	BNode printer;
	if (GetPrinterSettingsNode(printer_name, &printer) !=  B_OK)
		return B_INVALID_PRINTER;
	return LoadSettings(&printer, settings, user);
}

status_t TPrintTools::SaveSettings(const char *printer_name, const BMessage& settings, const char *user)
{
	BNode printer;
	if (GetPrinterSettingsNode(printer_name, &printer) !=  B_OK)
		return B_INVALID_PRINTER;
	return SaveSettings(&printer, settings, user);
}

status_t TPrintTools::LoadSettings(BNode *printer, BMessage& settings, const char */*user*/)
{
	status_t err;
	attr_info info;
	if ((err = printer->GetAttrInfo(PSRV_PRINTER_ATTR_SETTINGS, &info)) != B_OK)
		return err;

	char *buffer = new char[info.size];
	const size_t read = printer->ReadAttr(PSRV_PRINTER_ATTR_SETTINGS, B_MESSAGE_TYPE, 0, buffer, info.size);
	if (read == info.size)	err = settings.Unflatten(buffer);
	else					err = B_ERROR;
	delete [] buffer;

	return err;
}

status_t TPrintTools::SaveSettings(BNode *printer, const BMessage& settings, const char */*user*/)
{
	status_t error = B_OK;
	const size_t length = settings.FlattenedSize();
	char *buffer = new char[length];
	if ((error = settings.Flatten(buffer, length)) == B_OK)
	{
		const size_t written = printer->WriteAttr(PSRV_PRINTER_ATTR_SETTINGS, B_MESSAGE_TYPE, 0, buffer, length);
		if (written != length)
			error = B_ERROR;
	}
	delete [] buffer;
	return error;
}

// --------------------------------------------------------------
// #pragma mark -

void TPrintTools::GetPreferredSize(BView *v, float *x, float *y)
{
#if (PRINTING_FOR_DESKTOP)
	BRect frame(0,0,0,0);
	const int c = v->CountChildren();
	for (int i=0 ; i<c ; i++)
	{
		BView *child = v->ChildAt(i);
		if ((child->Flags() & B_WILL_DRAW) == 0)
			continue; // Don't take into account view that won't draw.
		BPrintConfigView *configView = dynamic_cast<BPrintConfigView *>(child);
		if (configView)
		{
			float w,h;
			configView->GetMinimumSize(&w, &h);
			frame = frame | BRect(0,0,w-1,h-1).OffsetToSelf(configView->Frame().LeftTop());
		} else {
			frame = frame | child->Frame();
		}
	}
	*x = frame.Width();
	*y = frame.Height();
#else
	*x = *y = 0.0f;
#endif
}

// function to retrieve strings from the ressource fork
const char *TPrintTools::RsrcGetString(int index)
{
#if (PRINTING_FOR_DESKTOP)
	size_t size;
	const void *data = GetLibbeResources()->LoadResource(B_STRING_TYPE, 300 + index, &size);
	return (const char *)data;
#else
	return B_EMPTY_STRING;
#endif
}

// ///////////////////////////////////////////////////////////////////////////
// PrinterManager
// ///////////////////////////////////////////////////////////////////////////
// #pragma mark -

PrinterManager::PrinterManager(bool standAlone = false)
	: fStandAlone(standAlone)
{
}

PrinterManager::~PrinterManager()
{
}

status_t PrinterManager::set_printer_busy(BNode *a_printer, BNode *job)
{
	// If the printer has a sharable transport, it's never "busy"
	if (has_sharable_transport(a_printer, job))
		return B_OK;

	// Then, mark the printer as "busy"
	const size_t len = strlen(PSRV_PRINTER_ATTR_BUSY)+1;
	size_t l = a_printer->WriteAttr(PSRV_PRINTER_ATTR_STATE, B_STRING_TYPE,0, PSRV_PRINTER_ATTR_BUSY, len);
	if (l != len)
		return B_ERROR;
	
	// And finaly, mark the port as busy too
	return set_printerport_busy(a_printer);
}

status_t PrinterManager::set_printer_free(BNode *a_printer)
{
	// Mark this printer's transport as free
	set_printerport_free(a_printer);
	
	// Mark this printer "free"
	const size_t len = strlen(PSRV_PRINTER_ATTR_FREE)+1;
	size_t l = a_printer->WriteAttr(PSRV_PRINTER_ATTR_STATE, B_STRING_TYPE,0, PSRV_PRINTER_ATTR_FREE, len);
	return ((l == len) ? (B_OK) : (B_ERROR));
}

bool PrinterManager::is_printer_free(BNode *a_printer)
{
	char state[5];
	a_printer->ReadAttr(PSRV_PRINTER_ATTR_STATE, B_STRING_TYPE, 0, state, sizeof(state));
	if (!strcmp(state, PSRV_PRINTER_ATTR_BUSY))
		return false;
	
	// if the transport associated to this printer is not free, then this printer is not free either
	return is_printerport_free(a_printer);
}

status_t PrinterManager::set_printerport_busy(BNode *printer)
{
	if (fStandAlone)
		return B_OK;
	return p_set_printerport_busy(printer);
}
status_t PrinterManager::set_printerport_free(BNode *printer)
{
	if (fStandAlone)
		return B_OK;
	return p_set_printerport_free(printer);
}
bool PrinterManager::is_printerport_free(BNode *printer)
{
	if (fStandAlone)
		return true;
	return p_is_printerport_free(printer);
}

status_t PrinterManager::p_set_printerport_busy(BNode *printer)
{
	
	PrinterPort p(printer, true);	// create a busy port
	const int32 c = fPrinterPortList.CountItems();
	for (int i=0 ; i<c ; i++)
	{
		PrinterPort& q = *static_cast<PrinterPort *>(fPrinterPortList.ItemAt(i));
		if ((p == q) == false)
			continue;
		p.SetBusy();
		return B_OK;
	}
	fPrinterPortList.AddItem(new PrinterPort(p));
	return B_OK;
}

status_t PrinterManager::p_set_printerport_free(BNode *printer)
{
	PrinterPort p(printer);
	const int32 c = fPrinterPortList.CountItems();
	for (int i=0 ; i<c ; i++)
	{
		PrinterPort& q = *static_cast<PrinterPort *>(fPrinterPortList.ItemAt(i));
		if (p == q)
			return q.SetFree();
	}
	return B_OK;
}

bool PrinterManager::p_is_printerport_free(BNode *printer)
{
	PrinterPort p(printer);
	const int32 c = fPrinterPortList.CountItems();
	for (int i=0 ; i<c ; i++)
	{
		PrinterPort& q = *static_cast<PrinterPort *>(fPrinterPortList.ItemAt(i));
		if (p == q)
			return q.IsFree();
	}
	return true;
}

bool PrinterManager::has_sharable_transport(BNode *a_printer, BNode *job)
{
	uint32 flags;
	if ((job) && (job->ReadAttr(PSRV_SPOOL_ATTR_PREVIEW, B_UINT32_TYPE, 0, &flags, 4) == 4) && ((flags & 0x1) != 0))
	{ // We are in preview mode - always sharable
		return true;
	}
	else
	{ // Check if we are using the "None" transport - always sharable
		char type[256];	
		long err = a_printer->ReadAttr(PSRV_PRINTER_ATTR_TRANSPORT, B_STRING_TYPE, 0, type, sizeof(type));
		if (err > 0)
		{ // "None" is a "virtual" transport that does nothing
			if (!strcmp(type, "None"))	
				return true;
		}
	}
	
	// Check if the selected transport is sharable
	BTransportIO transport(a_printer);
	if (transport.IsSharable())
		return true;

	return false;	
}


// ///////////////////////////////////////////////////////////////////////////
// PrinterPort
// ///////////////////////////////////////////////////////////////////////////
// #pragma mark -

PrinterPort::PrinterPort(BNode *printer, bool busy)
{
	char transport_name[B_FILE_NAME_LENGTH];
	char dev_name[B_OS_NAME_LENGTH];
	printer->ReadAttr(M_ATTR_TRANSPORT_NAME, B_STRING_TYPE, 0, transport_name, B_FILE_NAME_LENGTH);
	printer->ReadAttr(M_ATTR_TRANSPORT_ADDR, B_STRING_TYPE, 0, dev_name, B_OS_NAME_LENGTH);
	fTransport = transport_name;
	fTransportAddr = dev_name;
	fBusy = busy;
}

PrinterPort::PrinterPort(const PrinterPort& port)
{
	fTransport = port.fTransport;
	fTransportAddr = port.fTransportAddr;
	fBusy = port.fBusy;
}

status_t PrinterPort::SetBusy()
{
	fBusy = true;
	return B_OK;
}

status_t PrinterPort::SetFree()
{
	fBusy = false;
	return B_OK;
}

bool PrinterPort::IsFree()
{
	return !fBusy;
}

bool PrinterPort::operator == (const PrinterPort& port) const
{
	return ((fTransport == port.fTransport) && (fTransportAddr == port.fTransportAddr));
}

// ************************************************************
// PageIterator
// ************************************************************
// #pragma mark -

PageIterator::PageIterator(const int32 first, const int32 last, const int32 nbCopies, bool assembled, bool reverse)
	: 	fAssembled(assembled),
		fReverse(reverse),
		fNbCopies(nbCopies),
		fFirst(first),
		fLast(last)
{
	Rewind();
}

PageIterator::~PageIterator()
{
}

status_t PageIterator::Rewind()
{
	fCurrentPage = (fReverse) ? (fLast) : (fFirst);
	fCurrentCopie = fNbCopies;
	return B_OK;
}

status_t PageIterator::Next(int32 *page, int32 *nbCopies)
{
	if (fCurrentCopie == 0)
		return B_ERROR;

	// Get the current page
	*page = fCurrentPage;

next_page:
	// Calculate the next one
	if ((!fReverse) && (fAssembled))
	{ // 1,2,3,4  1,2,3,4  1,2,3,4 ...
		*nbCopies = 1;
		fCurrentPage++;
		if (fCurrentPage > fLast)
		{
			fCurrentPage = fFirst;
			fCurrentCopie--;
		}
	}
	else if ((!fReverse) && (!fAssembled))
	{ // 1,1,1  2,2,2  3,3,3  4,4,4 ...
		*nbCopies = fNbCopies;
		fCurrentPage++;
		if (fCurrentPage >= fLast)
			fCurrentCopie = 0;
	}
	else if ((fReverse) && (fAssembled))
	{ // 4,3,2,1  4,3,2,1  4,3,2,1  ...
		*nbCopies = 1;
		fCurrentPage--;
		if (fCurrentPage < fFirst)
		{
			fCurrentPage = fLast;
			fCurrentCopie--;
		}
	}
	else if ((fReverse) && (!fAssembled))
	{ // 4,4,4  3,3,3  2,2,2  1,1,1 ...
		*nbCopies = fNbCopies;
		fCurrentPage--;
		if (fCurrentPage < fFirst)
			fCurrentCopie = 0;
	}

	if ((fCurrentCopie) && (fCurrentPage == *page))
	{ // optim: if next page is the same than current one, don't print it but inc the nb of copies
		(*nbCopies) = (*nbCopies) + 1;
		goto next_page;
	}
	
	return B_OK;
}

// --------------------------------------------------------------
// #pragma mark -

status_t launch_printer_prefs()
{
	status_t rv;
	rv = be_roster->Launch(PSRV_PRINT_PREFS_SIG);
	if(rv == B_OK || rv == B_ALREADY_RUNNING)
		return B_OK;	
	return B_ERROR;
}

//------------------------------------------------------------------

void run_select_printer_panel()
{
	launch_printer_prefs();
}

//------------------------------------------------------------------

void run_add_printer_panel()
{
	if (launch_printer_prefs() != B_OK)
		return;

	BMessenger messenger(PSRV_PRINT_PREFS_SIG);
	if (messenger.IsValid() == false)
		return;

	BMessage a_message(msg_add_printer);
	BMessage reply;
	messenger.SendMessage(&a_message, &reply);
}

//------------------------------------------------------------------

status_t set_default_printer(const char *printer_name)
{
	if (printer_name == NULL)
		return B_BAD_VALUE;

	BPath path;
	find_directory(B_USER_PRINTERS_DIRECTORY, &path);
	path.Append(printer_name);
	BEntry entry(path.Path());
	if (entry.IsDirectory() == false)
		return B_BAD_VALUE;

	const char *kDefaultPrinter = "be:Default printer";
	status_t result;
	BAppPrefs settings("printer_data", PSRV_PRINT_SERVER_SIG);
	if ((result = settings.InitCheck()) != B_OK)
		return result;

	BString old;
	if (settings.Load() == B_OK)
	{ // get the old selected printer name
		const char *s =	settings.FindString(kDefaultPrinter);
		if (s)
			old = s;
	}

	// Save the new default printer setting
	settings.RemoveName(kDefaultPrinter);
	settings.AddString(kDefaultPrinter, printer_name);
	if ((result = settings.Save()) != B_OK)
		return result;
	
	// Tell everyone we changed the default printer
	BMessage m(B_PRINTER_CHANGED);
	be_roster->Broadcast(&m);

	// make sure the folder has the right mime type
	// !!!! WARNING !!!!
	// Be sure to call this code AFTER settings.Save()
	// This will trigger a node monitor message (SetType, SetIcon, ...)
	// needed by Printers to know the default printer has changed

	if (old.Length())
	{ // unset the old default icon
		BPath path;
		find_directory(B_USER_PRINTERS_DIRECTORY, &path);
		path.Append(old.String());
		BEntry entry(path.Path());
		BNode node(&entry);
		BNodeInfo nodeInfo(&node);
		nodeInfo.SetType(PSRV_PRINTER_MIMETYPE);
		nodeInfo.SetIcon(NULL, B_MINI_ICON);
		nodeInfo.SetIcon(NULL, B_LARGE_ICON);
	}

 	// set the new type and icon
	BNode node(&entry);
	BNodeInfo nodeInfo(&node);
	nodeInfo.SetType(PSRV_PRINTER_MIMETYPE);

	#if (PRINTING_FOR_DESKTOP)

		if (be_app)
		{
			BBitmap *mini;
			BBitmap *large;
			BImageResources *resourcehandler = GetLibbeResources();
			if (resourcehandler->GetBitmapResource('BBMP', 205, &mini) == B_OK)
				nodeInfo.SetIcon(mini, B_MINI_ICON);
			if (resourcehandler->GetBitmapResource('BBMP', 204, &large) == B_OK)
				nodeInfo.SetIcon(large, B_LARGE_ICON);
			delete mini;
			delete large;
		}

	#endif

	return B_OK;
}

status_t get_default_printer(BString& printer_name)
{
	const char *kDefaultPrinter = "be:Default printer";
	// Get the setting file that contains the default selected printer
	status_t result;
	BAppPrefs settings("printer_data", PSRV_PRINT_SERVER_SIG);
	if ((result = settings.InitCheck()) != B_OK)
		return result;

	if ((result = settings.Load()) != B_OK)
	{ // the file does not exists - try the old method
		BPath path;
		find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);
		path.Append("printer_data");
		BFile settings_file(path.Path(), B_READ_ONLY);
		if (settings_file.InitCheck() != B_OK)
			return settings_file.InitCheck();
		char temp[256];
		if (settings_file.Read(&temp, 256) <= 0)
			return B_ERROR;
		settings.AddString(kDefaultPrinter, temp);
		settings.Save();
	}
		
	printer_name = B_EMPTY_STRING;
	const char *p = settings.FindString(kDefaultPrinter);
	if (p == NULL)
		return  B_NO_PRINTER;
	
	// So we got the the default printer name. Make sure this printer exists.	
	BPath path;
	find_directory(B_USER_PRINTERS_DIRECTORY, &path);
	path.Append(p);
	BEntry entry(path.Path());
	if (entry.Exists() == false)
		return B_INVALID_PRINTER;

	// Ok, the default printer exists
	printer_name = p;
	return B_OK;
}
