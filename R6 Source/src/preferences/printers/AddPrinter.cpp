#include <stdio.h>
#include <fcntl.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <Debug.h>
#include <Application.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Screen.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <MenuItem.h>
#include <TextControl.h>
#include <Window.h>
#include <Bitmap.h>
#include <FindDirectory.h>
#include <String.h>
#include <Alert.h>
#include <AppFileInfo.h>
#include <Messenger.h>
#include <TranslationUtils.h>
#include <SerialPort.h>
#include <Box.h>

#include <pr_server.h>
#include <PrintEnv.h>
#include <print/TransportAddOn.h>

#include "PrefPanel.h"
#include "AddPrinter.h"
#include "BackgroundBox.h"


extern const char *RsrcGetString(int index);

const int32 BTN_HEIGHT		= 24;
const int32 BTN_WIDTH		= 75;
const int32 TEXT_CONTROL_HEIGHT = 19;

/************************/

/*** message constants ***/

/* for LocalPrinterView */

const uint32 LV_ADD_MSG			= 'LVAD';
const uint32 LV_PRNAME_MSG		= 'LVPR';
const uint32 LV_DRIVER_MSG		= 'LVDR';
const uint32 LV_TRANSPORT_MSG	= 'LVTR';
const uint32 LV_TRANSPORT_SUBMENU_MSG	= 'LVSB';
const uint32 IV_CANCEL_MSG	= 'CANC';


#define ADDON_SEARCH_PATHS 2
BPath gAddOnSearchPaths[ADDON_SEARCH_PATHS];

void AddPrinterToList(PrinterNode*, BPopUpMenu*);
void DisallowPrinterNameChars(BTextControl*);


// ************************************************************************

static bool reject(char *name)
{
	return (strcmp(name, B_APP_MIME_TYPE));
}

// ************************************************************************

static bool IsPrinterNameTaken(const char *pName)
{
	BPath spoolDir;
	find_directory(B_USER_PRINTERS_DIRECTORY, &spoolDir);
	spoolDir.Append(pName);
	BEntry entry(spoolDir.Path());
	return (entry.Exists());
}

// ************************************************************************
// #pragma mark -

TAddPrinterWindow::TAddPrinterWindow(	BWindow* owner,
										bool,
									 	const char *)
	: BWindow(	BRect(0, 0, 400, 40+144+BTN_HEIGHT), RsrcGetString(151),
				B_MODAL_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL, 
				B_NOT_RESIZABLE | B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE)
{
	find_directory (B_USER_ADDONS_DIRECTORY, &(gAddOnSearchPaths[0]));
	find_directory (B_BEOS_ADDONS_DIRECTORY, &(gAddOnSearchPaths[1]));
	gAddOnSearchPaths[0].Append("Print");
	gAddOnSearchPaths[1].Append("Print");

	fOwner = owner;	
	AddChild(localView = new LocalPrinterView(Bounds(), fOwner));
	
	// center the window
	BRect ownerFrame = owner->Frame();
	BPoint pt;
	pt.x = (ownerFrame.Width() - Bounds().Width())*0.5f + ownerFrame.left;
	pt.y = (ownerFrame.Height() - Bounds().Height())*0.5f + ownerFrame.top;
	MoveTo(pt);
}

void TAddPrinterWindow::MessageReceived(BMessage* theMessage)
{
	switch (theMessage->what)
	{
		 case IV_CANCEL_MSG:
		 {
			if (fOwner) fOwner->PostMessage('canc');
			PostMessage(B_QUIT_REQUESTED);
			break;
		 }
		
		 case LV_ADD_MSG:
		 case LV_PRNAME_MSG:
		 case LV_DRIVER_MSG:
		 case LV_TRANSPORT_MSG:
		 case LV_TRANSPORT_SUBMENU_MSG:
			PostMessage(theMessage, localView);
			break;
		
		 default:
			BWindow::MessageReceived(theMessage);
			break;
	}
}

/*******************************************************************************/
// #pragma mark -

LocalPrinterView::LocalPrinterView(BRect R, BWindow *initiator)
	: 	BView(R, "localview", B_FOLLOW_NONE, B_WILL_DRAW),
		fOwner(initiator),
		fLastMenuItem(NULL)
{
	BStringView *textString;	

	AddChild(backBox = new TBackgroundBox(R));

	largePrintIcon = BTranslationUtils::GetBitmap("local_icon");
		
	BRect tmpRect;
	tmpRect.Set(84,10,300,10+15);
	textString = new BStringView(tmpRect, "toptext", RsrcGetString(144));
	BFont font(be_bold_font);
	font.SetSize(12);
	textString->SetFont(&font);
	backBox->AddChild(textString);

	// seperator line...
	tmpRect.top = tmpRect.bottom + 6;
	tmpRect.bottom = tmpRect.top+1;
	tmpRect.left = 10+64+6;
	tmpRect.right = R.right - 6;
	
	topSepRect = tmpRect;
	
	tmpRect.right = R.right-10;
	tmpRect.top = tmpRect.bottom + 30;
	tmpRect.bottom = tmpRect.top + TEXT_CONTROL_HEIGHT;
	tmpRect.left = 10+64+50;

	prName = new BTextControl(tmpRect, "printerName", RsrcGetString(134), 0, new BMessage(LV_PRNAME_MSG));
	prName->SetModificationMessage(new BMessage(LV_PRNAME_MSG));							
	prName->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	prName->SetDivider(this->StringWidth(RsrcGetString(133))+5);
	DisallowPrinterNameChars(prName);
	backBox->AddChild(prName);

	tmpRect.top = tmpRect.bottom + 8;
	tmpRect.bottom = tmpRect.top + TEXT_CONTROL_HEIGHT;	
	
	BPopUpMenu *driver = new BPopUpMenu(RsrcGetString(145));
	driverMenu = new BMenuField(tmpRect, "driverMenu", RsrcGetString(132), driver);
	backBox->AddChild(driverMenu);
	driverMenu->SetAlignment(B_ALIGN_RIGHT);
	driverMenu->SetDivider(this->StringWidth(RsrcGetString(133))+5);
	PopulateDriverList(driver);

	tmpRect.top = tmpRect.bottom + 8;
	tmpRect.bottom = tmpRect.top + TEXT_CONTROL_HEIGHT;	

	BPopUpMenu *trans = new BPopUpMenu(RsrcGetString(145));
	transportMenu = new BMenuField(tmpRect, "transportMenu", RsrcGetString(133), trans);
	backBox->AddChild(transportMenu);
	transportMenu->SetAlignment(B_ALIGN_RIGHT);
	transportMenu->SetDivider(this->StringWidth(RsrcGetString(133))+5);

	BMessage transports;
	if (TPrintTools::GetTransports(&transports) != B_OK)
		return;
	PopulateTransportList(transports, trans, BTransportAddOn::B_TRANSPORT_IS_NETWORK, 0);
	trans->AddItem(new BSeparatorItem);
	PopulateTransportList(transports, trans, BTransportAddOn::B_TRANSPORT_IS_NETWORK, BTransportAddOn::B_TRANSPORT_IS_NETWORK);


	// seperator line...
	tmpRect.top = R.bottom-10-BTN_HEIGHT-10;
	tmpRect.bottom = tmpRect.top + 1;
	tmpRect.left = R.left + 6;
	tmpRect.right = R.right - 6;
	bottomSepRect = tmpRect;

	cancelBtn = new BButton(	BRect(	R.right-10-BTN_WIDTH-10-BTN_WIDTH,	R.bottom-10-BTN_HEIGHT,
										R.right-10-BTN_WIDTH-10,			R.bottom-10),
								"lvcancel", RsrcGetString(109), new BMessage(IV_CANCEL_MSG));
	backBox->AddChild(cancelBtn);

	addBtn = new BButton(		BRect(	R.right-10-BTN_WIDTH,	R.bottom-10-BTN_HEIGHT,
										R.right-10,				R.bottom-10),
								"lvadd", RsrcGetString(114), new BMessage(LV_ADD_MSG));
	addBtn->SetEnabled(false);
	backBox->AddChild(addBtn);

	transportSelected = false;
	printerSelected = false;
	driverSelected = false;	

	((TBackgroundBox *)backBox)->AddLine(topSepRect, true);
	((TBackgroundBox *)backBox)->AddLine(bottomSepRect, true);
}

LocalPrinterView::~LocalPrinterView()
{
	delete largePrintIcon;
}


void LocalPrinterView::Draw(BRect)
{
	if (largePrintIcon)
		backBox->DrawBitmapAsync(largePrintIcon, BPoint(10,10));
}

void LocalPrinterView::Show()
{
	addBtn->MakeDefault(true);
	BView::Show();
}

void LocalPrinterView::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case LV_PRNAME_MSG:
			printerSelected = true;
			EnableAddButton();
			break;	
		
		case LV_TRANSPORT_MSG:
			MarkTransportItem(msg);
			fAttributes = msg->FindInt32("attributes");
			transport = msg->FindString("label");
			if (msg->HasString("sub_type"))
				transport_subtype = msg->FindString("sub_type");			
			transportSelected = true;
			if (((prName->Text() == NULL) || (strlen(prName->Text()) == 0)) && (msg->HasString("prettyname")))
				prName->SetText(msg->FindString("prettyname"));
			EnableAddButton();
			break;

		case LV_TRANSPORT_SUBMENU_MSG:
			MarkTransportItem(msg);
			fAttributes = msg->FindInt32("attributes");
			transport = msg->FindString("label");
			transportSelected = false;
			EnableAddButton();
			break;
			
		case LV_DRIVER_MSG:
			driverName = msg->FindString("label");		
			driverSelected = true;
			EnableAddButton();
			break;

	 	case LV_ADD_MSG:
		{
			printerName = prName->Text();
			if (IsPrinterNameTaken(printerName.String()))
			{
				char str[1024];
				const char *fmt = RsrcGetString(146);
				sprintf(str, fmt, printerName.String());
				(new BAlert(NULL, str, RsrcGetString(109)))->Go();
				break;
			}
			if (fOwner)
			{
				BMessage msg(msg_config_printer);
				GetSettings(&msg);
				fOwner->PostMessage(&msg);
			}
			Window()->PostMessage(B_QUIT_REQUESTED);
			break;		
		}
					
		default:
			BView::MessageReceived(msg);
			break;
	}
}

void LocalPrinterView::GetSettings(BMessage *msg)
{
	msg->AddString("printer name",	printerName.String());
	msg->AddString("driver",		driverName.String());
	msg->AddString("transport",		transport.String());
	msg->AddInt32("attributes",		fAttributes);
	if (transport_subtype.String() != B_EMPTY_STRING)
		msg->AddString("transport path", transport_subtype.String());
}

void LocalPrinterView::EnableAddButton()
{
	if (driverName == "Preview")
	{
		transport = "None";
		transportSelected = true;
		transportMenu->SetEnabled(false);
	}
	else
	{
		transportMenu->SetEnabled(true);
		if (transport == "None")
			transportSelected = false;
	}
		
	addBtn->SetEnabled(driverSelected && transportSelected && printerSelected);
}


void LocalPrinterView::PopulateDriverList(BPopUpMenu *menu)
{
	for (int i=0; i < ADDON_SEARCH_PATHS; i++)
		GetDriversForPath(menu, gAddOnSearchPaths[i].Path());
	menu->SetRadioMode(true);	
}

void LocalPrinterView::GetDriversForPath(BMenu *menu, const char *path)
{
	BEntry			entry;
	BNode			node;
	BNodeInfo		nodeInfo;
	char			buffer[256];
	BMessage 		*tmpMsg;

	BDirectory 		dir(path);
	dir.Rewind();	
	
	while (dir.GetNextEntry(&entry) == B_OK) {
		node.SetTo(&entry);
		nodeInfo.SetTo(&node);

		nodeInfo.GetType(buffer);
		if (reject(buffer)) {
			continue;
		}
		
		entry.GetName(buffer);
		tmpMsg = new BMessage(LV_DRIVER_MSG);
		tmpMsg->AddString("label", buffer);
		menu->AddItem(new BMenuItem(buffer, tmpMsg));
	}
}


void LocalPrinterView::PopulateTransportList(const BMessage& transports, BPopUpMenu *menu, uint32 mask, uint32 value)
{
	// Build the transport menu
	BMessage t;
	for (int i=0 ; (transports.FindMessage("transports", i, &t) == B_OK) ; i++)
	{
		const char *fname = t.FindString("filename");			// File name
		const char *name = t.FindString(B_TRANSPORT_NAME);		// Pretty name
		const uint32 attributes = t.FindInt32(B_TRANSPORT_ATTRIBUTES);	// Attributes
		
		if (!name)
			continue;
		
		if ((attributes & mask) != value)
			continue;
		
		if (t.HasMessage(B_TRANSPORT_MSG_PRINTERS))
		{
			BMessage printers;
			BMenu *submenu = NULL;
			BMenuItem *item = NULL;
			for (int i=0 ; (t.FindMessage(B_TRANSPORT_MSG_PRINTERS, i, &printers) == B_OK) ; i++)
			{
				const char *printerName	= printers.FindString("DEVID:DES:");
				const char *portName	= printers.FindString(B_TRANSPORT_DEV_DESCRIPTION);
				const char *dev_name = printers.FindString(B_TRANSPORT_DEV_UNIQUE_NAME);
				const char *dev_class = printers.FindString("DEVID:CLS:");

				if (printerName == NULL)
					printerName = printers.FindString("DEVID:DESCRIPTION:");

				if (dev_class == NULL)
					dev_class = printers.FindString("DEVID:CLASS:");

				// Make sure the connected device is a printer
				if ((dev_class) && (strcmp(dev_class, "PRINTER")))
					continue;

				if (submenu == NULL)
				{
					submenu = new BMenu(name);
					submenu->SetTriggersEnabled(false);
					submenu->SetRadioMode(true);
					item = new BMenuItem(submenu, new BMessage(LV_TRANSPORT_SUBMENU_MSG));
					menu->AddItem(item);
				}

				// If a "port name" is provided by the transport, then use it.
				// else, use the dev_name (device name)
				BString pretty_name;
				if (portName)
				{
					pretty_name = portName;
				}
				else
				{
					pretty_name = dev_name;
					if ((printerName) && (BString(printerName) != B_EMPTY_STRING))
						pretty_name << " (" << printerName << ")";
				}
				
				// If we don't have a printerName
				// (eg: no printer connected on the parallel port)
				// Then, empty string
				if (printerName == NULL)
					printerName = B_EMPTY_STRING;

				BMessage *msg = new BMessage(LV_TRANSPORT_MSG);
				msg->AddMessage("transport", &t);
				msg->AddString("prettyname", printerName);
				msg->AddString("label", fname);
				msg->AddString("sub_type", dev_name);
				msg->AddPointer("superitem", item);
				msg->AddInt32("attributes", attributes);
				submenu->AddItem(new BMenuItem(pretty_name.String(), msg));
			}
		}
		else
		{
			BMessage *msg = new BMessage(LV_TRANSPORT_MSG);
			msg->AddMessage("transport", &t);
			msg->AddString("label", fname);
			msg->AddInt32("attributes", attributes);
			menu->AddItem(new BMenuItem(name, msg));
		}
	}
}

void LocalPrinterView::MarkTransportItem(const BMessage *msg)
{
	if (fLastMenuItem)
		fLastMenuItem->SetMarked(false);
	msg->FindPointer("source", (void **)&fLastMenuItem);

	BMenuItem *item;
	if (msg->FindPointer("superitem", (void**)&item) == B_OK)
		item->SetMarked(true);
}


// #pragma mark -

void DisallowPrinterNameChars(BTextControl *text)
{
	text->TextView()->DisallowChar('/');
	text->TextView()->DisallowChar('*');
	text->TextView()->DisallowChar('?');
	text->TextView()->DisallowChar('~');
}
							
