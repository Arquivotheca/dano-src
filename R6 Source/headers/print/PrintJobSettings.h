// ***********************************************************************
// libbe.so
//
// (c) 2000, Be Incorporated
//
// Written by : Mathias Agopian
//
// ***********************************************************************

#ifndef _PRINT_SETTINGS_H_
#define _PRINT_SETTINGS_H_

#include <Node.h>
#include <DataIO.h>
#include <File.h>
#include <Picture.h>
#include <Rect.h>
#include <Point.h>
#include <Message.h>
#include <SupportDefs.h>

#include <print/PrinterAddOnDefs.h>

class BPrintJobSettings
{
public:
			BPrintJobSettings();
			BPrintJobSettings(const BMessage& settings);
	virtual ~BPrintJobSettings();

	void SetSettings(const BMessage& settings);
	void ClearSettings();
	bool IsEmpty() const;
	const BMessage& Message() const;

public:
	BString	PrinterName() const;			// The printer name
	uint32	FirstPage() const;				// Returns the first page to be printed, one based inclusive
	uint32	LastPage() const;				// Returns the last page to be printed, one based inclusive
	uint32	NumberOfCopies() const;			// Number of copies of the document (minimum is 1)
	uint32	Xdpi() const;					// Returns the horizonatal resolution of the page (user)
	uint32	Ydpi() const;					// Returns the vertical resolution of the page (user)
	uint32	DeviceXdpi() const;				// Returns the horizonatal resolution of the printer
	uint32	DeviceYdpi() const;				// Returns the vertical resolution of the printer
	bool	Color() const;					// true if color printing has to be used
	float	Scale() const;					// Scale factor 1.0f is no scaling
	BRect	PaperRect() const;				// Logical paper rect as it's/will be seen by the application
	BRect	PrintableRect() const;			// Logical printable rect as it's/will be seen by the application
	BRect	DeviceArea() const;				// Returns the out of the door paper size exprimed in 1/72th of DPI
	BRect	DevicePrintableArea() const;	// Returns the printable area (that is Area less margins)
	orientation_type Orientation() const;	// layout portrait/landscape
	uint32 Attributes() const;				// Attributes of the page (mirror, etc...)
	uint32	NumberOfPages() const;			// Returns LastPage() - FirstPage() + 1
	BRect	DeviceMargins() const;			// Returns the printer's margins exprimed in 1/72th of DPI

	// Deprecated. will go away.
	uint32	NbCopies() const {return NumberOfCopies();}
	uint32	NbPages() const {return NumberOfPages();}

protected:
	BMessage fSettings;

protected:
	static const char *kIdPrinterName;
	static const char *kIdFirstPage;
	static const char *kIdLastPage;
	static const char *kIdNumberOfCopies;
	static const char *kIdXdpi;
	static const char *kIdYdpi;
	static const char *kIdDeviceXdpi;
	static const char *kIdDeviceYdpi;
	static const char *kIdColor;
	static const char *kIdScale;
	static const char *kIdPaperRect;
	static const char *kIdPrintableRect;
	static const char *kIdDeviceArea;
	static const char *kIdDevicePrintableArea;
	static const char *kIdOrientation;
	static const char *kIdAttribute;
};



#endif
