#include <stdlib.h>
#include <stdio.h>

#include <OS.h>
#include <SupportDefs.h>
#include <Binder.h>
#include <Application.h>
#include <Message.h>
#include <String.h>
#include <Entry.h>
#include <File.h>
#include <Path.h>
#include <FindDirectory.h>
#include <Directory.h>
#include <Errors.h>
#include <Messenger.h>
#include <Autolock.h>

#include <www/TellBrowser.h>

#include <print/PrinterConfigAddOn.h>
#include <print/TransportIO.h>
#include "pr_server.h"

#include "smoove.h"
#include "PrintEnv.h"

#define	checkpoint	; //printf("%s\n", __PRETTY_FUNCTION__);


// ////////////////////////////////////////////////////////////

get_status_t PrintBinderNode::ReadProperty(const char *name, property &prop, const property_list &args)
{
	checkpoint;
	if (!strcmp(name, "Selected"))
	{
		return selected(prop);
	}
	else if (!strcmp(name, "Select"))
	{
		prop = "";
		if (select(args[0]) == B_OK)
		{
			NotifyListeners(B_PROPERTY_CHANGED, name);
			prop = name;
		}
		return B_OK;
	}
	else if (!strcmp(name, "be:update"))
	{
		return update_printers(prop);
	}
	return BinderNode::ReadProperty(name, prop, args);
}

// ============================================================

get_status_t PrintBinderNode::selected(property &prop)
{
	BString printerName;
	status_t err = get_default_printer(printerName);
	if (err != B_OK)
		return err;
	prop = printerName.String();
	return B_OK;
}

get_status_t PrintBinderNode::update_printers(property &prop)
{
	char buf[256];
	BPath path;
	find_directory(B_USER_PRINTERS_DIRECTORY, &path);
	BDirectory dir(path.Path());
	BEntry entry;

	// First, remove the bindernode that are gone
	BinderNode::property printing = BinderNode::Root() / "service" / "printing";
	BinderNode::iterator i = printing->Properties();
	BinderNode::property printer;
	BString p;	
 	
 	while ((p = i.Next()).String() != "")
	{
		printer = printing[p.String()];
		if (printer.IsObject())
		{	
			dir.Rewind();
			while (dir.GetNextEntry(&entry) == B_OK)
			{
				entry.GetName(buf);
				if (!strcmp(buf, p.String()))
					goto found;
			}
			printing[p.String()] = BinderNode::property::undefined;
		}
		found: continue;
	}

	// Now create a node for new printers
	dir.Rewind();
	while (dir.GetNextEntry(&entry) == B_OK)
	{
		entry.GetName(buf);
		BinderNode::iterator i = printing->Properties();
		while ((p = i.Next()).String() != "")
		{
			printer = printing[p.String()];
			if (printer.IsObject() && (!strcmp(buf, p.String())))
				goto found2;
		}
		new ConnectedPrinterBinderNode(buf);
		found2: continue;
	}
	prop = "";
	return B_OK;
}

get_status_t PrintBinderNode::select(const property &prop)
{
	return set_default_printer(prop.String().String());
}

// ////////////////////////////////////////////////////////////
// #pragma mark -

ConnectedPrinterBinderNode::ConnectedPrinterBinderNode(const char *printerName)
	:	BinderNode(),
		fDriverInfoCached(false),
		fNbProperties(sizeof(fPropertyList)/sizeof(*fPropertyList)),
		fPrinterName(printerName),
		fAddonImageID(-1),
		fTransport(NULL),
		fDriver(NULL),
		fPrinter(NULL),
		fLockCount(0),
		fPaperFormatsNames(NULL),
		fNbPapers(0),
		fModes(NULL),
		fNbModes(0),
		fSelectedResolution(0),
		fSelectedPaperFormat(0),
		fStatus("B_UNKNOWN"),
		fInkLevel(255)
{
	checkpoint;
	fPropertyList[0] = "PrettyName";
	fPropertyList[1] = "Status";
	fPropertyList[2] = "SelectedResolution";
	fPropertyList[3] = "papers";
	fPropertyList[4] = "paper_formats";
	fPropertyList[5] = "SelectedPaperFormat";
	fPropertyList[6] = "PaperType";
	fPropertyList[7] = "GetOrientation";
	fPropertyList[8] = "InkLevel";

	BinderNode::property printing = BinderNode::Root() / "service" / "printing";
	printing[printerName] = this;
}

ConnectedPrinterBinderNode::~ConnectedPrinterBinderNode()
{
	if (fLockCount > 1)
		fLockCount = 1;	// Force unload - Should not happen.
	unload_printer_addon();
	delete [] fModes;
	delete [] fPaperFormatsNames;
}

status_t ConnectedPrinterBinderNode::OpenProperties(void **cookie, void *)
{
	checkpoint;
	*cookie = new cookie_t;
	return B_OK;
}

status_t ConnectedPrinterBinderNode::NextProperty(void *cookie, char *nameBuf, int32 *len)
{
	checkpoint;
	cookie_t& the_cookie = *((cookie_t *)cookie);

next_property:
	if (the_cookie.index>=0 && the_cookie.index<fNbProperties)
	{
		if ((the_cookie.index == 8) && (fInkLevel > 100))
		{ // InkLevel
			the_cookie.index++;
			goto next_property;
		}

		strncpy(nameBuf, fPropertyList[the_cookie.index], *len-1);
		nameBuf[*len-1]=0;
		*len = strlen(nameBuf);
		the_cookie.index++;
		return B_OK;
	}

	return BinderNode::NextProperty(cookie, nameBuf, len);
}

status_t ConnectedPrinterBinderNode::CloseProperties(void *cookie)
{
	checkpoint;
	delete cookie;
	return BinderNode::CloseProperties(cookie);
}

put_status_t ConnectedPrinterBinderNode::WriteProperty(const char *name, const property &prop)
{
	checkpoint;
	if (!strcmp(name, fPropertyList[1]))
	{ // Status
		if (fStatus != prop.String())
		{ // update this property only if it is actually different
			fStatus = prop;
			NotifyListeners(B_PROPERTY_CHANGED, name);
		}
		return B_OK;
	}
	else if (!strcmp(name, fPropertyList[8]))
	{ // InkLevel
		if (fInkLevel != prop.Number())
		{ // update this property only if it is actually different
			fInkLevel = prop;
			NotifyListeners(B_PROPERTY_CHANGED, name);
		}
		return B_OK;
	}
	return BinderNode::WriteProperty(name, prop);
}

get_status_t ConnectedPrinterBinderNode::ReadProperty(const char *name, property &prop, const property_list &args)
{
	checkpoint;
	if (!strcmp(name, fPropertyList[0]))
	{ // PrettyName
		prop = fPrinterName.String();
		return B_OK;
	}
	else if (!strcmp(name, fPropertyList[1]))
	{ // Status
		prop = fStatus;
		return B_OK;
	}
	else if (!strcmp(name, fPropertyList[8]))
	{ // InkLevel
		if (fInkLevel > 100)
			return B_ERROR;
		prop = fInkLevel;
		return B_OK;
	}
	else if (!strcmp(name, "Print"))
	{
		BMessenger messenger("application/x-vnd.Web");
		if (messenger.IsValid() == false)
			return B_ERROR;

		if (args.CountItems() == 0)
			return EINVAL;

		// Lock the driver
		if (load_printer_addon() == B_OK)
		{
			// fPrinter is valid only after load_printer_addon() is called
			{ // Use a block here to limit the existance of PrinterManager
				PrinterManager printManager(true);
				if (printManager.is_printer_free(fPrinter) == false)
				{ // Cancel the print-job, since there's nothing else to do.
					prop = "B_ERROR";
					// Unlock the driver
					unload_printer_addon();
					return B_OK;
				}
			}

			// Make sure we have all initialized correctly
			cache_driver_infos();
		
			BMessage print(TB_PRINT);
			print.AddString("frame", args[0]);
			if (args.CountItems() == 2)
				print.AddString("header", args[1]);	
			
			// Paper Orientation
			if (fOrientation == 'p')		fSettings.SetOrientation(B_PRINT_LAYOUT_PORTRAIT);
			else if (fOrientation == 'l')	fSettings.SetOrientation(B_PRINT_LAYOUT_LANDSCAPE);

			Driver().SetSettings(fSettings.Message());			// Configure the driver with our current settings
			Driver().PrinterModeSelected(fSelectedResolution);	// Select the resolution
			Driver().PaperSelected(0, fSelectedPaperFormat);	// Select the paper
			fSettings = Driver().Settings();					// Get back the settings

			// Add the printer's name (needed by BPrintJob)
			fSettings.Message().RemoveName("BEIA:resolution");
			fSettings.Message().RemoveName("BEIA:paper");
			fSettings.Message().RemoveName("BEIA:orientation");
			fSettings.Message().AddInt32("BEIA:resolution", fSelectedResolution);
			fSettings.Message().AddInt32("BEIA:paper", fSelectedPaperFormat);
			fSettings.Message().AddInt32("BEIA:orientation", (int32)fOrientation);

			// Save the setting message
			TPrintTools::SaveSettings(fPrinterName.String(), fSettings.Message());

			// Unlock the driver
			unload_printer_addon();

			// Send TB_PRINT to wagner
			print.AddMessage("settings", &fSettings.Message());
			messenger.SendMessage(&print);
			prop = "B_OK";
			return B_OK;
		} else {
			property result;
			WriteProperty(fPropertyList[1], (result = "B_UNKNOWN"));
			WriteProperty(fPropertyList[1], (result = "B_ERROR"));
			prop = "B_OK";
			return B_OK;
		}		
		return B_ERROR;
	}
	else if (!strcmp(name, "Cancel"))
	{
		if (TPrintTools::CancelJobs(fPrinterName.String()) != B_OK)
			return B_ERROR;
		prop="";
		return B_OK;
	}

	// ======================================
	// Paper types/resolution
	// ======================================

	else if (!strcmp(name, fPropertyList[3]))
	{ // papers
		if (create_paper_types_node() != B_OK)	
			return B_ERROR;
		prop = PaperTypeAtom;
		return B_OK;
	}
	else if (!strcmp(name, fPropertyList[2]))
	{ // SelectedResolution
		if (create_paper_types_node() != B_OK)	
			return B_ERROR;
		if (cache_driver_infos() != B_OK)
			return B_ERROR;
		const BPrinterConfigAddOn::printer_mode_t *mode;
		if (get_nth_mode(fSelectedResolution, &mode) != B_OK)
		{
			prop = "";
			return B_OK;
		}
		prop = mode->quality;
		return B_OK;
	}
	else if (!strcmp(name, fPropertyList[6]))
	{ // PaperType
		if (create_paper_types_node() != B_OK)	
			return B_ERROR;
		if (cache_driver_infos() != B_OK)
			return B_ERROR;
		const BPrinterConfigAddOn::printer_mode_t *mode;
		if (get_nth_mode(fSelectedResolution, &mode) != B_OK)
		{
			prop = "";
			return B_OK;
		}
		prop = mode->paper;
		return B_OK;
	}
	else if (!strcmp(name, "SelectPrintMode"))
	{ // SelectPrintMode
		if (args.CountItems() != 1)
			return EINVAL;
		fSelectedResolution = (int)(args[0]);
		NotifyListeners(B_PROPERTY_CHANGED, name);
		prop="";
		return B_OK;
	}

	// ======================================
	// Paper formats
	// ======================================

	else if (!strcmp(name, fPropertyList[7]))
	{ // GetOrientation
		if (fOrientation == 'p')		prop="portrait";
		else if (fOrientation == 'l')	prop="landscape";
		else							return EINVAL;
		return B_OK;
	}
	else if (!strcmp(name, "SetOrientation"))
	{ // SetOrientation
		if (args.CountItems() != 1)
			return EINVAL;
		const char c = (args[0].String().String())[0];
		if ((c == 'p') || (c == 'l'))
			fOrientation = c;
		prop = "";
		return B_OK;
	}

	else if (!strcmp(name, fPropertyList[4]))
	{ // paper_formats
		if (cache_driver_infos() != B_OK)
			return B_ERROR;
		if (create_paper_formats_node() != B_OK)
			return B_ERROR;		
		prop = PaperFormatsAtom; 
		return B_OK;
	}
	else if (!strcmp(name, fPropertyList[5]))
	{ // SelectedPaperFormat
		if (cache_driver_infos() != B_OK)
			return B_ERROR;
		if ((fSelectedPaperFormat < 0) || (fSelectedPaperFormat >= fNbPapers))
			return B_ERROR;
		prop = fPaperFormatsNames[fSelectedPaperFormat].String();
		return B_OK;
	}
	else if (!strcmp(name, "SelectPaperFormat"))
	{ // SelectPaperFormat
		if (args.CountItems() != 1)
			return EINVAL;
		fSelectedPaperFormat = (int)(args[0]);
		NotifyListeners(B_PROPERTY_CHANGED, name);
		prop="";
		return B_OK;
	}

	return BinderNode::ReadProperty(name, prop, args);
}

status_t ConnectedPrinterBinderNode::create_paper_types_node()
{
	if ((BinderNode *)PaperTypeAtom == NULL)
		PaperTypeAtom = new PaperTypesNode(*this);
	return B_OK;
}

status_t ConnectedPrinterBinderNode::create_paper_formats_node()
{
	// Paper formats
	if ((BinderNode *)PaperFormatsAtom == NULL)
	{ // Setup the paper sizes right now
		BinderContainer *r = new BinderContainer;
		r->SetOrdered(true);
		PaperFormatsAtom = r;
		// Get the papers formats supported by the driver
		for (int i=0 ; i<fNbPapers ; i++)
			r->AddProperty(fPaperFormatsNames[i].String(), property(i));
	}
	return B_OK;
}

status_t ConnectedPrinterBinderNode::load_printer_addon()
{
	BAutolock lock(fDriverCountLock);
	if (lock.IsLocked() == false)
		return B_ERROR;

	if (fLockCount > 0)
	{ // Driver already there
		fLockCount++;
		return B_OK;
	}

	// Init the printer
	BPath path;
	find_directory(B_USER_PRINTERS_DIRECTORY, &path);
	path.Append(fPrinterName.String());
	fPrinter = new BDirectory(path.Path());

	// get the driver name attribute
	status_t err;
	if ((err = fAddonImageID = TPrintTools::LoadDriverAddOn(fPrinter, NULL)) > 0)
	{
		BPrinterConfigAddOn *(*instantiate_printer_config_addon)(BTransportIO *, BNode *);
		if ((err = get_image_symbol(fAddonImageID, B_INSTANTIATE_PRINTER_CONFIG_ADDON_FUNCTION, B_SYMBOL_TYPE_TEXT, (void **)&instantiate_printer_config_addon)) == B_OK)
		{
			fTransport = new BTransportIO(fPrinter);
			fDriver = instantiate_printer_config_addon(fTransport, fPrinter);
			fLockCount++;
			if ((err = fDriver->InitCheck()) != B_OK)
				unload_printer_addon();
		}
	}

	return  err;
}

status_t ConnectedPrinterBinderNode::unload_printer_addon()
{
	BAutolock lock(fDriverCountLock);
	if (lock.IsLocked() == false)
		return B_ERROR;

	fLockCount--;
	if (fLockCount == 0)
	{
		if (fDriver)			delete fDriver;
		if (fTransport)			delete fTransport;
		if (fPrinter)			delete fPrinter;
		if (fAddonImageID>0)	unload_add_on(fAddonImageID);
		fDriver = NULL;
		fTransport = NULL;
		fPrinter = NULL;
		fAddonImageID = -1;
	}

	return B_OK;
}

status_t ConnectedPrinterBinderNode::cache_driver_infos()
{
	if (fDriverInfoCached == false)
	{
		// Lock the driver
		if (load_printer_addon() != B_OK)
			return B_ERROR;	

		BMessage settings;
		if (TPrintTools::LoadSettings(fPrinterName.String(), settings) == B_OK)
		{ // The printer has a default setting. Configure the driver with it.
			Driver().SetSettings(settings);
			fSettings.SetSettings(settings);
		}
		// Get back the settings from the driver.
		fSettings = Driver().Settings();
		fSettings.SetPrinterName(fPrinterName.String());
		fSettings.Message().RemoveName("be:newapi"); // This is a new api driver.
		fSettings.Message().AddInt32("be:newapi", 0x00000001);
		if (fSettings.Message().FindInt32("BEIA:resolution", &fSelectedResolution) != B_OK)
			fSelectedResolution = -1;	
		if (fSettings.Message().FindInt32("BEIA:paper", &fSelectedPaperFormat) != B_OK)
			fSelectedPaperFormat = -1;

		// Printer modes
		const BPrinterConfigAddOn::printer_mode_t *modes;
		fNbModes = Driver().PrinterModes(&modes);
		fModes = new LocalPrinterModes[fNbModes];
		for (int i=0 ; i<fNbModes ; i++)
		{
			fModes[i] = modes[i];
			// If we don't have any settings yet, use the default ones.
			if ((modes[i].attributes & BPrinterConfigAddOn::printer_mode_t::B_IS_DEFAULT_QUALITY) && (fSelectedResolution < 0))
				fSelectedResolution = i;
		}

		// Get the papers formats supported by the driver
		const BPrintPaper *papers;
		fNbPapers = Driver().PaperFormats(0, &papers);
		fPaperFormatsNames = new BString[fNbPapers];
		for (int i=0 ; i<fNbPapers ; i++)
		{
			// If we don't have any settings yet, use the default ones.
			fPaperFormatsNames[i] = papers[i].PrettyName();
			if ((papers[i].id == BPrintPaper::DefaultFormat()) && (fSelectedPaperFormat < 0))
				fSelectedPaperFormat = i;
		}

		// Extract our settings from the message
		if (fSelectedResolution >= fNbModes)
			fSelectedResolution = 0;		

		if (fSelectedPaperFormat >= fNbPapers)
			fSelectedPaperFormat = 0;

		// Paper Orientation
		fOrientation = 'p';
		if (fSettings.Orientation() == B_PRINT_LAYOUT_PORTRAIT)		fOrientation = 'p';
		if (fSettings.Orientation() == B_PRINT_LAYOUT_LANDSCAPE)	fOrientation = 'l';

		// Unlock the driver
		unload_printer_addon();
		fDriverInfoCached = true;
	}
	return B_OK;
}

status_t ConnectedPrinterBinderNode::get_nth_mode(int i, BPrinterConfigAddOn::printer_mode_t const ** const mode)
{
	if (i<0)
		return B_BAD_VALUE;

	if (cache_driver_infos() != B_OK)
		return B_ERROR;
	
	if (i>=fNbModes)
		return B_BAD_VALUE;

	*mode = fModes+i;
	return B_OK;
}

ConnectedPrinterBinderNode::LocalPrinterModes::LocalPrinterModes()
{
	 paper = NULL;
	 quality = NULL;
}

ConnectedPrinterBinderNode::LocalPrinterModes::~LocalPrinterModes()
{
	free((void *)paper);
	free((void *)quality);
}

ConnectedPrinterBinderNode::LocalPrinterModes& ConnectedPrinterBinderNode::LocalPrinterModes::operator = (const BPrinterConfigAddOn::printer_mode_t& copy)
{
	paper = strdup(copy.paper);
	quality = strdup(copy.quality);
	attributes = copy.attributes;
	return *this;
}		

// ////////////////////////////////////////////////////////////
// #pragma mark -


PaperTypesNode::PaperTypesNode(ConnectedPrinterBinderNode& node)
	:	m_parent(&node),
		printer(node)
{
	SetOrdered(true);
}

status_t PaperTypesNode::OpenProperties(void **cookie, void *copyCookie)
{
	if (update_modes_if_needed() != B_OK)
		return B_ERROR;
	return BinderContainer::OpenProperties(cookie, copyCookie);
}

get_status_t PaperTypesNode::ReadProperty(const char *name, property &prop, const property_list &args)
{
	if (update_modes_if_needed() != B_OK)
		return B_ERROR;
	return BinderContainer::ReadProperty(name, prop, args);
}

status_t PaperTypesNode::update_modes_if_needed()
{
	if (printer.cache_driver_infos() != B_OK)
		return B_ERROR;

	const BPrinterConfigAddOn::printer_mode_t *modes = printer.Modes();
	const int nbModes = printer.NbModes();

	SmartArray<const char *> paperList;
	for (int i=0 ; i<nbModes ; i++)
	{
		BinderContainer *r;

		// Make sure we don't already have processed this paper
		const char *p = modes[i].paper;
		for (int j=0 ; j<paperList.CountItems() ; j++)
			if (!strcmp(paperList[j], p))
				goto done;
		paperList.AddItem(p);
			
		// Now add the resolutions availlable for this paper
		r = new BinderContainer;
		r->SetOrdered(true);
		AddProperty(p, property(r));
		for (int j=0 ; j<nbModes ; j++)
			if (!strcmp(modes[j].paper, p))
				r->AddProperty(modes[j].quality, property(j));
	done:
		continue;
	}
	return B_OK;
}
