// ***********************************************************************
// libbe.so
//
// (c) 2000, Be Incorporated
//
// Written by : Mathias Agopian
//
// ***********************************************************************

#ifndef _PRINT_EDIT_SETTINGS_H_
#define _PRINT_EDIT_SETTINGS_H_

#include <Node.h>
#include <DataIO.h>
#include <File.h>
#include <Picture.h>
#include <Rect.h>
#include <Point.h>
#include <Message.h>
#include <SupportDefs.h>

#include <print/PrinterAddOnDefs.h>
#include <print/PrintJobSettings.h>


class BPrintJobEditSettings : public BPrintJobSettings
{
public:
			BPrintJobEditSettings();
			BPrintJobEditSettings(const BMessage& settings);
	virtual ~BPrintJobEditSettings();

	BMessage& Message();
	
public:
	status_t SetPrinterName(const char *printer_name);
	status_t SetFirstPage(uint32 page);
	status_t SetLastPage(uint32 page);
	status_t SetNumberOfCopies(uint32 copies);
	status_t SetXdpi(uint32 xdpi = 72);
	status_t SetYdpi(uint32 ydpi = 72);
	status_t SetDeviceXdpi(uint32);
	status_t SetDeviceYdpi(uint32);
	status_t SetColor(bool color = true);
	status_t SetScale(float scale = 1.0f);
	status_t SetPaperRect(BRect);
	status_t SetPrintableRect(BRect);
	status_t SetDeviceArea(BRect);
	status_t SetDevicePrintableArea(BRect);
	status_t SetOrientation(orientation_type orientation = B_PRINT_LAYOUT_PORTRAIT);
	status_t SetAttributes(uint32 attributes = B_PRINT_ATTR_ASSEMBLED);
	status_t Normalize();

	// Deprecated. Will go away.
	status_t SetNbCopies(uint32 copies) {return SetNumberOfCopies(copies);}

private:
	status_t UpdateInt32(const char *, int32);
	status_t UpdateBool(const char *, bool);
	status_t UpdateFloat(const char *, float);
	status_t UpdateRect(const char *, BRect);
	status_t UpdateString(const char *, const char *);
	status_t UpdateMessage(const char *, const BMessage *);
};



#endif
