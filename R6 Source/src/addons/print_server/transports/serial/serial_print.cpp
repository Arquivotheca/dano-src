#include "serial_print.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <Entry.h>
#include <Message.h>
#include <Directory.h>
#include <string.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <Application.h>
#include <String.h>
#include <Alert.h>


#define EXPORT_GUI	0
#define EOT '\004'


extern "C" _EXPORT BDataIO* init_transport(BMessage *msg, BMessage *reply);
extern "C" _EXPORT bool exit_transport();
extern "C" _EXPORT status_t probe_printers(BMessage *printers);

#if (EXPORT_GUI)
extern "C" _EXPORT BWindow* configure_transport(const BNode *printer);
#endif

static const char *kTransportName = "Serial Port";
static SerialPrint *sport = NULL;

BDataIO* init_transport(BMessage *msg, BMessage *reply)
{
	BNode printer;
	BNode *p;
	if (msg->FindPointer("printer", (void **)&p) == B_OK)
		printer = *p;

	// This is for compatibility only (will go away)
	if (printer.InitCheck() != B_OK)
	{
		const char *printer_path = msg->FindString("printer_file");
		if (printer_path)
			printer.SetTo(printer_path);
		if (printer.InitCheck() != B_OK)
			return NULL;
	}			

	// Get the device name
	char dev_name[B_OS_NAME_LENGTH];
	const char *s = msg->FindString("dev_name");
	if (s)
		strcpy(dev_name, s);
	else
	{ // We try the old method, to stay compatible (for developpement only, will go away)
		// For serial, we must save the transport addresse (actually is usefull for readability only)	
		printer.ReadAttr("transport_address", B_STRING_TYPE, 0, dev_name, B_OS_NAME_LENGTH);
	}

	// Write the device name
	printer.WriteAttr("_serial/DeviceName", B_STRING_TYPE, 0, dev_name, strlen(dev_name)+1);

	// create the transport...	
	sport = new SerialPrint(printer, msg);
	if (sport->InitCheck())
	{
		reply->what = 'okok';
		reply->AddBool("bidirectional", true);
		reply->AddBool("multiple", false);
		reply->AddString("name", kTransportName);
		return sport;
	}

	delete sport;
	sport = NULL;
	return NULL;
}


bool exit_transport()
{
	delete sport;
	sport = NULL;
	return true;
}

status_t probe_printers(BMessage *inout)
{
	// First some informations on this transport
	inout->MakeEmpty();
	inout->AddBool("bidirectional", true);
	inout->AddBool("multiple", false);
	inout->AddString("name", kTransportName);

	BSerialPort serial;
	int32 n = 0;
	for (int32 n=serial.CountDevices()-1 ; n>=0 ; n--)
	{
		char devName[B_OS_NAME_LENGTH];
		serial.GetDeviceName(n, devName); 
		BMessage reply;
		reply.AddString("dev", devName);
		inout->AddMessage("printers", &reply);
	}
	return B_OK;
}

#if (EXPORT_GUI)
BWindow* configure_transport(BNode *printer)
{
	const char *buf = msg->FindString("printer_file");
	return new SerialConfigWindow(BRect(300,300, 550,540), printer);
}
#endif

// -----------------------------------------------------
#pragma mark -

SerialPrint::SerialPrint(BNode& dir, BMessage *msg)
	: isValid(false)
{
	// extract any info out of the printer driver message...
	shouldWriteEOT = true;
	if (msg->HasBool("should_write_eot"))
		shouldWriteEOT = msg->FindBool("should_write_eot");

	// Read in attributes from spool directory...
	char dataRate[B_OS_NAME_LENGTH];
	char parity[B_OS_NAME_LENGTH];
	char dataBits[B_OS_NAME_LENGTH];
	char stopBits[B_OS_NAME_LENGTH];
	char flowControl[B_OS_NAME_LENGTH];

	ssize_t rv = dir.ReadAttr("_serial/DeviceName", B_ASCII_TYPE, 0, deviceName, B_OS_NAME_LENGTH);		
	if (rv <= 0)
		return;

	dir.ReadAttr("_serial/DataRate", B_ASCII_TYPE, 0, dataRate, B_OS_NAME_LENGTH);
	dir.ReadAttr("_serial/Parity", B_ASCII_TYPE, 0, parity, B_OS_NAME_LENGTH);
	dir.ReadAttr("_serial/DataBits", B_ASCII_TYPE, 0, dataBits, B_OS_NAME_LENGTH);
	dir.ReadAttr("_serial/StopBits", B_ASCII_TYPE, 0, stopBits, B_OS_NAME_LENGTH);
	dir.ReadAttr("_serial/FlowControl", B_ASCII_TYPE, 0, flowControl, B_OS_NAME_LENGTH);

	bsp = new BSerialPort();
	if (bsp->Open(deviceName) <= 0)
		return;

	// now that we're open, set the port settings...
	if(!strcmp(dataRate, "9600"))			bsp->SetDataRate(B_9600_BPS);
	else if (!strcmp(dataRate, "19200"))	bsp->SetDataRate(B_19200_BPS);
	else if (!strcmp(dataRate, "31250"))	bsp->SetDataRate(B_31250_BPS);
	else if (!strcmp(dataRate, "38400"))	bsp->SetDataRate(B_38400_BPS);
	else if (!strcmp(dataRate, "57600"))	bsp->SetDataRate(B_57600_BPS);
	else if (!strcmp(dataRate, "115200"))	bsp->SetDataRate(B_115200_BPS);
	else if (!strcmp(dataRate, "230400"))	bsp->SetDataRate(B_230400_BPS);
	else									bsp->SetDataRate(B_9600_BPS);
		
	if(!strcmp(dataBits, "7"))			bsp->SetDataBits(B_DATA_BITS_7);
	else if (!strcmp(dataBits, "8"))	bsp->SetDataBits(B_DATA_BITS_8);
	else								bsp->SetDataBits(B_DATA_BITS_8);
	
	if(!strcmp(stopBits, "1"))			bsp->SetStopBits(B_STOP_BITS_1);
	else if (!strcmp(stopBits, "2"))	bsp->SetStopBits(B_STOP_BITS_2);
	else								bsp->SetStopBits(B_STOP_BITS_1);
	
	if(!strcmp(dataBits, "7"))			bsp->SetDataBits(B_DATA_BITS_7);
	else if (!strcmp(dataBits, "8"))	bsp->SetDataBits(B_DATA_BITS_8);
	else								bsp->SetDataBits(B_DATA_BITS_8);

	if(!strcmp(parity, "No"))			bsp->SetParityMode(B_NO_PARITY);
	else if (!strcmp(parity, "Even"))	bsp->SetParityMode(B_EVEN_PARITY);
	else if (!strcmp(parity, "Odd"))	bsp->SetParityMode(B_ODD_PARITY);
	else								bsp->SetParityMode(B_NO_PARITY);
	

	if(!strcmp(flowControl, "No"))				bsp->SetFlowControl(0);
	else if (!strcmp(flowControl, "Software"))	bsp->SetFlowControl(B_SOFTWARE_CONTROL);
	else if (!strcmp(flowControl, "Hardware"))	bsp->SetFlowControl(B_HARDWARE_CONTROL);
	else										bsp->SetFlowControl(B_HARDWARE_CONTROL);
	
	bsp->SetBlocking(true);	
	bsp->SetTimeout(500000);
	isValid = true;
	
	if(shouldWriteEOT)
	{
		char x = EOT;
		bsp->Write(&x, 1);
	}
}

SerialPrint::~SerialPrint()
{
	if (bsp)
	{
		if(shouldWriteEOT)
		{
			char x = EOT;
			bsp->Write(&x, 1);
		}
		bsp->Close();
		delete bsp;
	}
}

bool SerialPrint::InitCheck()
{
	return (isValid);
}

ssize_t SerialPrint::Write(const void* buffer, size_t numBytes)
{
	return bsp->Write(buffer, numBytes);
}

ssize_t SerialPrint::Read(void* buffer, size_t numBytes)
{
	return bsp->Read(buffer, numBytes);
}


// -----------------------------------------------------
#pragma mark -

#if (EXPORT_GUI)

/**** begin configure stuff... *****/

#define DATA_RATE_TEXT "Data Rate:"
#define PARITY_TEXT "Parity:"
#define FLOW_TEXT "Flow Control:"
#define STOP_BITS_TEXT "Stop Bits:"
#define DATA_BITS_TEXT "Data Bits:"
#define DEVICES_TEXT "Device Name:"

const uint32 UNSET_MSG = 'UNST';
const uint32 SAVE_MSG = 'SAVE';
const uint32 CANCEL_MSG = 'CANC';
const uint32 DEVICE_MSG = 'DEVM';
const uint32 BPS_9600 = '9600';
const uint32 BPS_19200 = '192K';
const uint32 BPS_31250 = '3125';
const uint32 BPS_38400 = '3840';
const uint32 BPS_57600 = '5760';
const uint32 BPS_115200 = '1152';
const uint32 BPS_230400 = '2304';
const uint32 STOP_BITS_1 = 'STB1';
const uint32 STOP_BITS_2 = 'STB2';
const uint32 DATA_BITS_7 = 'DAT7';
const uint32 DATA_BITS_8 = 'DAT8';
const uint32 NO_PARITY = 'NPRA';
const uint32 EVEN_PARITY = 'EPRA';
const uint32 ODD_PARITY = 'OPRA';
const uint32 NO_FLOW_CONTROL = 'NFLO';
const uint32 HARDWARE_CONTROL = 'HFLO';
const uint32 SOFTWARE_CONTROL = 'SFLO';

const int32 BTN_HEIGHT		= 24;
const int32 BTN_WIDTH		= 75;


SerialConfigWindow::SerialConfigWindow(BRect R, const char *printer_file)
	: BWindow(R, "Configure Serial Port", B_TITLED_WINDOW, B_NOT_RESIZABLE)
{
	BRect B = Bounds();

	printerEntry = new BEntry(printer_file);
	if(printerEntry->InitCheck() != B_OK)
	{
		(new BAlert("", "Couldn't find the printer file!", "Bummer"))->Go();
		return;
	}
	backBox = new BBox(B);
	backBox->SetBorder(B_PLAIN_BORDER);
	backBox->SetViewColor(216,216,216);
	AddChild(backBox);

	BRect tmpRect(10,10,B.right-10,25);
	textString = new BStringView(tmpRect, "toptext", "Configure Serial Port");
	BFont font(be_bold_font);
	font.SetSize(12);
	textString->SetFont(&font);
	backBox->AddChild(textString);

	tmpRect.top = tmpRect.bottom + 8;
	tmpRect.bottom = tmpRect.top;
	tmpRect.left = B.left + 6;
	tmpRect.right = B.right - 6;
	backBox->AddChild(new BBox(tmpRect));
	
	tmpRect.Set(B.right-10-BTN_WIDTH, B.bottom-10-BTN_HEIGHT,
				B.right-10, B.bottom-10);
	saveBtn = new BButton(tmpRect, "save", "Save", new BMessage(SAVE_MSG));
	
	tmpRect.right = tmpRect.left - 10;
	tmpRect.left = tmpRect.right - BTN_WIDTH;
	cancelBtn = new BButton(tmpRect, "cancel", "Cancel", new BMessage(CANCEL_MSG));

	tmpRect.bottom = tmpRect.top - 10;
	tmpRect.top = tmpRect.bottom;
	tmpRect.left = B.left + 6;
	tmpRect.right = B.right - 6;
	backBox->AddChild(new BBox(tmpRect));

	tmpRect.bottom = tmpRect.top - 10 - 5;
	tmpRect.top = tmpRect.bottom - 15;
	tmpRect.left = B.left + 30 + 35;	

	BPopUpMenu *popMenu;
		
	popMenu = new BPopUpMenu("choose one");
	popMenu->AddItem(new BMenuItem("None", new BMessage(NO_FLOW_CONTROL)));
	popMenu->AddItem(new BMenuItem("Software", new BMessage(SOFTWARE_CONTROL)));
	popMenu->AddItem(new BMenuItem("Hardware", new BMessage(HARDWARE_CONTROL)));
	flowControl = new BMenuField(tmpRect, "flow", FLOW_TEXT, popMenu);
	flowControl->SetAlignment(B_ALIGN_RIGHT);
	flowControl->SetDivider(backBox->StringWidth(DEVICES_TEXT));

	tmpRect.bottom = tmpRect.top - 10;
	tmpRect.top = tmpRect.bottom - 15;
		
	popMenu = new BPopUpMenu("choose one");
	popMenu->AddItem(new BMenuItem("Seven", new BMessage(DATA_BITS_7)));
	popMenu->AddItem(new BMenuItem("Eight", new BMessage(DATA_BITS_8)));
	dataBits = new BMenuField(tmpRect, "databits", DATA_BITS_TEXT, popMenu);
	dataBits->SetAlignment(B_ALIGN_RIGHT);
	dataBits->SetDivider(backBox->StringWidth(DEVICES_TEXT));

	tmpRect.bottom = tmpRect.top - 10;
	tmpRect.top = tmpRect.bottom - 15;
		
	popMenu = new BPopUpMenu("choose one");
	popMenu->AddItem(new BMenuItem("One", new BMessage(STOP_BITS_1)));
	popMenu->AddItem(new BMenuItem("Two", new BMessage(STOP_BITS_2)));
	stopBits = new BMenuField(tmpRect, "stopbits", STOP_BITS_TEXT, popMenu);
	stopBits->SetAlignment(B_ALIGN_RIGHT);
	stopBits->SetDivider(backBox->StringWidth(DEVICES_TEXT));
	
	tmpRect.bottom = tmpRect.top - 10;
	tmpRect.top = tmpRect.bottom - 15;
		
	popMenu = new BPopUpMenu("choose one");
	popMenu->AddItem(new BMenuItem("None", new BMessage(NO_PARITY)));
	popMenu->AddItem(new BMenuItem("Even", new BMessage(EVEN_PARITY)));
	popMenu->AddItem(new BMenuItem("Odd", new BMessage(ODD_PARITY)));
	parity = new BMenuField(tmpRect, "parity", PARITY_TEXT, popMenu);
	parity->SetAlignment(B_ALIGN_RIGHT);
	parity->SetDivider(backBox->StringWidth(DEVICES_TEXT));

	tmpRect.bottom = tmpRect.top - 10;
	tmpRect.top = tmpRect.bottom - 15;
		
	popMenu = new BPopUpMenu("choose one");
	popMenu->AddItem(new BMenuItem("9600 BPS", new BMessage(BPS_9600)));
	popMenu->AddItem(new BMenuItem("19200 BPS", new BMessage(BPS_19200)));
	popMenu->AddItem(new BMenuItem("31250 BPS", new BMessage(BPS_31250)));
	popMenu->AddItem(new BMenuItem("38400 BPS", new BMessage(BPS_38400)));
	popMenu->AddItem(new BMenuItem("57600 BPS", new BMessage(BPS_57600)));
	popMenu->AddItem(new BMenuItem("115200 BPS", new BMessage(BPS_115200)));
	popMenu->AddItem(new BMenuItem("230400 BPS", new BMessage(BPS_230400)));
	dataRate = new BMenuField(tmpRect, "datarate", DATA_RATE_TEXT, popMenu);
	dataRate->SetAlignment(B_ALIGN_RIGHT);
	dataRate->SetDivider(backBox->StringWidth(DEVICES_TEXT));
	
	tmpRect.bottom = tmpRect.top - 10;
	tmpRect.top = tmpRect.bottom - 15;
		
	popMenu = new BPopUpMenu("choose one");
	PopulateDeviceList(popMenu);
	deviceNames = new BMenuField(tmpRect, "devices", DEVICES_TEXT, popMenu);
	deviceNames->SetAlignment(B_ALIGN_RIGHT);
	deviceNames->SetDivider(backBox->StringWidth(DEVICES_TEXT));

	backBox->AddChild(deviceNames);
	backBox->AddChild(dataRate);		
	backBox->AddChild(parity);		
	backBox->AddChild(dataBits);		
	backBox->AddChild(stopBits);		
	backBox->AddChild(flowControl);		
	backBox->AddChild(cancelBtn);		
	backBox->AddChild(saveBtn);		

	GetCurrentSettings();
	
	dirty = false;
	Show();

};

SerialConfigWindow::~SerialConfigWindow()
{
	// nothing yet
}

void
SerialConfigWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
	 case BPS_9600:
		newDataRate = BPS_9600;
		dirty = true;
		break;
	 case BPS_19200:
		newDataRate = BPS_19200;
		dirty = true;
		break;
	 case BPS_31250:
		newDataRate = BPS_31250;
		dirty = true;
		break;
	 case BPS_38400:
		newDataRate = BPS_38400;
		dirty = true;
		break;
	 case BPS_57600:
		newDataRate = BPS_57600;
		dirty = true;
		break;
	 case BPS_115200:
		newDataRate = BPS_115200;
		dirty = true;
		break;
	 case BPS_230400:
		newDataRate = BPS_230400;
		dirty = true;
		break;
	 case DATA_BITS_7:
		newDataBits = DATA_BITS_7;
		dirty = true;
		break;
	 case DATA_BITS_8:
		newDataBits = DATA_BITS_8;
		dirty = true;
		break;
	 case STOP_BITS_1:
		newStopBits = STOP_BITS_1;
		dirty = true;
		break;
	 case STOP_BITS_2:
		newStopBits = STOP_BITS_2;
		dirty = true;
		break;
	 case NO_PARITY:
		newParity = NO_PARITY;
		dirty = true;
		break;
	 case ODD_PARITY:
		newParity = ODD_PARITY;
		dirty = true;
		break;
	 case EVEN_PARITY:
		newParity = EVEN_PARITY;
		dirty = true;
		break;
	 case HARDWARE_CONTROL:
		newFlowControl = HARDWARE_CONTROL;
		dirty = true;
		break;
	 case SOFTWARE_CONTROL:
		newFlowControl = SOFTWARE_CONTROL;
		dirty = true;
		break;
	 case NO_FLOW_CONTROL:	
		newFlowControl = NO_FLOW_CONTROL;
		dirty = true;
		break;
	 case DEVICE_MSG:
	 {
		BString dev = msg->FindString("label");
		newDevice = dev;
		dirty = true;
		break;
	 }
	 case SAVE_MSG:
		SetSettings();
		PostMessage(B_QUIT_REQUESTED, this);
		break;
	 case CANCEL_MSG:
		PostMessage(B_QUIT_REQUESTED, this);
		break;
	 default:
		BWindow::MessageReceived(msg);
		break;
	}

}

bool
SerialConfigWindow::QuitRequested()
{
	if(dirty)
	{
		BAlert *a = new BAlert("", "Save these serial settings?", "Cancel", "Save");
		long rv = a->Go();
		if(rv == 1) SetSettings();
	}
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

void
SerialConfigWindow::PopulateDeviceList(BPopUpMenu *menu)
{
	BSerialPort port;
	BMessage *tmpMsg;
	BMenuItem *menuItem;
	char device[B_OS_NAME_LENGTH];
	
	int32 n = port.CountDevices();
	for(int i=0; i < n; i++)
	{
		port.GetDeviceName(i, device);
		tmpMsg = new BMessage(DEVICE_MSG);
		tmpMsg->AddString("label", device);
		menuItem = new BMenuItem(device, tmpMsg);
		if(curDevice == device) menuItem->SetMarked(true);
		menu->AddItem(menuItem);		
	}
}

void
SerialConfigWindow::GetCurrentSettings()
{
	BNode node(printerEntry);
	char buf[255];
	ssize_t rv;
	
	BMenuItem *item;
	
	curDataRate = UNSET_MSG;
	curParity = UNSET_MSG;
	curStopBits = UNSET_MSG;
	curDataBits = UNSET_MSG;
	curFlowControl = UNSET_MSG;
	
	newDataRate = UNSET_MSG;
	newParity = UNSET_MSG;
	newStopBits = UNSET_MSG;
	newDataBits = UNSET_MSG;
	newFlowControl = UNSET_MSG;

	rv = node.ReadAttr("_serial/DataRate", B_ASCII_TYPE, 0, buf, 255);
	if(rv > 0)
	{
		if(!strcmp(buf, "9600"))
		{
			curDataRate = BPS_9600;
		}
		else if (!strcmp(buf, "19200"))
		{
			curDataRate = BPS_19200;
		}
		else if (!strcmp(buf, "31250"))
		{
			curDataRate = BPS_31250;
		}
		else if (!strcmp(buf, "38400"))
		{
			curDataRate = BPS_38400;
		}
		else if (!strcmp(buf, "57600"))
		{
			curDataRate = BPS_57600;
		}
		else if (!strcmp(buf, "115200"))
		{
			curDataRate = BPS_115200;
		}
		else if (!strcmp(buf, "230400"))
		{
			curDataRate = BPS_230400;
		}

		item = dataRate->Menu()->FindItem(curDataRate);
		if(item) item->SetMarked(true);
		
	}
		
	rv = node.ReadAttr("_serial/DataBits", B_ASCII_TYPE, 0, buf, 255);
	if(rv > 0)
	{
		if(!strcmp(buf, "7"))
		{
			curDataBits = DATA_BITS_7;
		}
		else if (!strcmp(buf, "8"))
		{
			curDataBits = DATA_BITS_8;
		}

		item = dataBits->Menu()->FindItem(curDataBits);
		if(item) item->SetMarked(true);
	}

	rv = node.ReadAttr("_serial/StopBits", B_ASCII_TYPE, 0, buf, 255);
	if(rv > 0)
	{
		if(!strcmp(buf, "1"))
		{
			curStopBits = STOP_BITS_1;
		}
		else if (!strcmp(buf, "2"))
		{
			curStopBits = STOP_BITS_2;
		}

		item = stopBits->Menu()->FindItem(curStopBits);
		if(item) item->SetMarked(true);
	}
	
	rv = node.ReadAttr("_serial/Parity", B_ASCII_TYPE, 0, buf, 255);
	if(rv > 0)
	{
		if(!strcmp(buf, "No"))
		{
			curParity = NO_PARITY;
		}
		else if (!strcmp(buf, "Even"))
		{
			curParity = EVEN_PARITY;
		}
		else if (!strcmp(buf, "Odd"))
		{
			curParity = ODD_PARITY;
		}

		item = parity->Menu()->FindItem(curParity);
		if(item) item->SetMarked(true);
	}
		
	rv = node.ReadAttr("_serial/FlowControl", B_ASCII_TYPE, 0, buf, 255);
	if(rv > 0)
	{
		if(!strcmp(buf, "No"))
		{
			curFlowControl = NO_FLOW_CONTROL;
		}
		else if (!strcmp(buf, "Software"))
		{
			curFlowControl = SOFTWARE_CONTROL;
		}		
		else if (!strcmp(buf, "Hardware"))
		{
			curFlowControl = HARDWARE_CONTROL;
		}

		item = flowControl->Menu()->FindItem(curFlowControl);
		if(item) item->SetMarked(true);
	}

	rv = node.ReadAttr("_serial/DeviceName", B_ASCII_TYPE, 0, buf, 255);
	if(rv > 0)
	{
		curDevice = buf;
		item = deviceNames->Menu()->FindItem(buf);
		if(item) item->SetMarked(true);
	}
	
}

bool
SerialConfigWindow::SetSettings()
{
	BString tmp;
	
	if(newDevice.Length() == 0) newDevice = curDevice;
	if(newDataRate == UNSET_MSG) newDataRate = curDataRate;
	if(newStopBits == UNSET_MSG) newStopBits = curStopBits;
	if(newParity == UNSET_MSG) newParity = curParity;
	if(newFlowControl == UNSET_MSG) newFlowControl = curFlowControl;
	
	
	BNode node(printerEntry);
	if(node.InitCheck() != B_OK)
	{
		return false;
	}
	
	if(curDevice != newDevice)
	{
		node.WriteAttr("_serial/DeviceName", B_STRING_TYPE, 0,
						newDevice.String(), newDevice.Length()+1);
	}
	
	if(curDataRate != newDataRate)
	{
		switch(newDataRate)
		{
		 case BPS_9600:
			tmp = "9600";
			break;
		 case BPS_19200:
			tmp = "19200";
			break;
		 case BPS_31250:
			tmp = "31250";
			break;
		 case BPS_38400:
			tmp = "38400";
			break;
		 case BPS_57600:
			tmp = "57600";
			break;
		 case BPS_115200:
			tmp = "115200";
			break;
		 case BPS_230400:
			tmp = "230400";
			break;
		};		
		node.WriteAttr("_serial/DataRate", B_STRING_TYPE, 0, tmp.String(), tmp.Length()+1);
	}
	
	if(curStopBits != newStopBits)
	{
		switch(newStopBits)
		{
		 case STOP_BITS_1:
			tmp = "1";
			break;
		 case STOP_BITS_2:
			tmp = "2";
			break;
		}
		node.WriteAttr("_serial/StopBits", B_STRING_TYPE, 0, tmp.String(), tmp.Length()+1);
	}

	if(curDataBits != newDataBits)
	{
		switch(newDataBits)
		{
		 case DATA_BITS_7:
			tmp = "7";
			break;
		 case DATA_BITS_8:
			tmp = "8";
			break;
		}
		node.WriteAttr("_serial/DataBits", B_STRING_TYPE, 0, tmp.String(), tmp.Length()+1);
	}

	if(curParity != newParity)
	{
		switch(newParity)
		{
		 case NO_PARITY:
			tmp = "No";
			break;
		 case EVEN_PARITY:
			tmp = "Even";
			break;
		 case ODD_PARITY:
			tmp = "Odd";
			break;
		}
		node.WriteAttr("_serial/Parity", B_STRING_TYPE, 0, tmp.String(), tmp.Length()+1);			
	}
	
	if(curFlowControl != newFlowControl)
	{
		switch(newFlowControl)
		{
		 case NO_FLOW_CONTROL:
			tmp = "No";
			break;
		 case SOFTWARE_CONTROL:
			tmp = "Software";
			break;			
		 case HARDWARE_CONTROL:
			tmp = "Hardware";
			break;
		}
		node.WriteAttr("_serial/FlowControl", B_STRING_TYPE, 0, tmp.String(), tmp.Length()+1);			
	}
	
	dirty = false;
	return true;
}


#endif

