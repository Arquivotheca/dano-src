#ifndef ADD_PRINTER_H
#define ADD_PRINTER_H

#include <Button.h>
#include <StringView.h>
#include <Window.h>
#include <View.h>
#include <RadioButton.h>
#include <String.h>
#include <FilePanel.h>
#include <printclient.h>

class LocalPrinterView;
class BPopUpMenu;
class BMenu;
class BMenuField;
class BTextControl;

class TAddPrinterWindow : public BWindow
{ 
 public:
			TAddPrinterWindow(BWindow* owner, bool NetworkPrinterOnly = false, const char *netPrinterPath = NULL);
	virtual void MessageReceived(BMessage* msg);
 private:
	BWindow				*fOwner;
	LocalPrinterView	*localView;
};

class LocalPrinterView : public BView
{
 public:
						LocalPrinterView(BRect R, BWindow *initiator);
						~LocalPrinterView();
						
	void				MessageReceived(BMessage *msg);
	void				Show();
	
	void				GetSettings(BMessage *msg);

	void				Draw(BRect);
	
 private:

	BWindow				*fOwner;
	BMenuItem 			*fLastMenuItem;

	void 				PopulateDriverList(BPopUpMenu *menu);
	void				PopulateTransportList(const BMessage& transports, BPopUpMenu *menu, uint32 mask, uint32 value);

	void				EnableAddButton();

	void				MarkTransportItem(const BMessage *msg);
	void 				GetDriversForPath(BMenu*, const char*);
	
	BButton				*cancelBtn;
	BButton				*addBtn;
	
	BMenuField			*driverMenu;
	BMenuField			*transportMenu;

	BStringView			*driverText;
	BStringView			*transportText;

	BTextControl		*prName;
	
	BView				*backBox;

	BBitmap				*largePrintIcon;
	BRect				topSepRect;
	BRect				bottomSepRect;

	BString				transport;
	BString				transport_subtype;
	BString				printerName;
	BString				driverName;
	int32				fAttributes;

	bool				transportSelected;
	bool				driverSelected;
	bool				printerSelected;
};

#endif
