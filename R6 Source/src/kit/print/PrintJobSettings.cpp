// ***********************************************************************
// libbe.so
//
// (c) 2000, Be Incorporated
//
// Written by : Mathias Agopian
//
// ***********************************************************************

#include <stdio.h>
#include <String.h>

#include <print/PrintJobSettings.h>
#include <print/PrintJobEditSettings.h>

const char *BPrintJobSettings::kIdPrinterName			= "be:printer_name";
const char *BPrintJobSettings::kIdFirstPage				= "be:first_page";
const char *BPrintJobSettings::kIdLastPage				= "be:last_page";
const char *BPrintJobSettings::kIdNumberOfCopies		= "be:copies";
const char *BPrintJobSettings::kIdXdpi					= "be:xdpi";
const char *BPrintJobSettings::kIdYdpi					= "be:ydpi";
const char *BPrintJobSettings::kIdDeviceXdpi			= "be:device_xdpi";
const char *BPrintJobSettings::kIdDeviceYdpi			= "be:device_ydpi";
const char *BPrintJobSettings::kIdColor					= "be:color";
const char *BPrintJobSettings::kIdScale					= "be:scale";
const char *BPrintJobSettings::kIdPaperRect				= "be:paper_rect";
const char *BPrintJobSettings::kIdPrintableRect			= "be:printable_rect";
const char *BPrintJobSettings::kIdDeviceArea			= "be:dev_area";
const char *BPrintJobSettings::kIdDevicePrintableArea	= "be:dev_printable_area";
const char *BPrintJobSettings::kIdOrientation			= "be:orientation";
const char *BPrintJobSettings::kIdAttribute				= "be:attributes";
//const char *BPrintJobSettings::kIdTray					= "be:tray";
//const char *BPrintJobSettings::kIdPaperEnum				= "be:paper_enum";

BPrintJobSettings::BPrintJobSettings()
{
	ClearSettings();
}

BPrintJobSettings::BPrintJobSettings(const BMessage& settings)
{
	SetSettings(settings);
}

BPrintJobSettings::~BPrintJobSettings()
{
	fSettings.MakeEmpty();
}

void BPrintJobSettings::SetSettings(const BMessage& settings)
{	
	// save the new settings
	fSettings = settings;
	
	// Little black magic to keep backward compatibility
	if (settings.HasRect("printable_rect")) {
		fSettings.RemoveName(kIdPrintableRect);	fSettings.AddRect(kIdPrintableRect,	settings.FindRect("printable_rect"));
	}
	if (settings.HasRect("paper_rect")) {
		fSettings.RemoveName(kIdPaperRect);		fSettings.AddRect(kIdPaperRect,	settings.FindRect("paper_rect"));
	}
	if (settings.HasInt32("first_page")) {
		fSettings.RemoveName(kIdFirstPage);		fSettings.AddInt32(kIdFirstPage, settings.FindInt32("first_page"));
	}
	if (settings.HasInt32("last_page")) {
		fSettings.RemoveName(kIdLastPage);		fSettings.AddInt32(kIdLastPage,	settings.FindInt32("last_page"));
	}
	if (settings.HasInt32("xres")) {
		fSettings.RemoveName(kIdDeviceXdpi);	fSettings.AddInt32(kIdDeviceXdpi, settings.FindInt32("xres"));
	}
	if (settings.HasInt32("yres")) {
		fSettings.RemoveName(kIdDeviceYdpi);	fSettings.AddInt32(kIdDeviceYdpi, settings.FindInt32("yres"));
	}

	if (!fSettings.HasInt32(kIdFirstPage))		fSettings.AddInt32(kIdFirstPage, 1);
	if (!fSettings.HasInt32(kIdLastPage))		fSettings.AddInt32(kIdLastPage, LONG_MAX);
	if (!fSettings.HasInt32(kIdNumberOfCopies))	fSettings.AddInt32(kIdNumberOfCopies, 1);
	if (!fSettings.HasInt32(kIdXdpi))			fSettings.AddInt32(kIdXdpi, 72);
	if (!fSettings.HasInt32(kIdYdpi))			fSettings.AddInt32(kIdYdpi, 72);
	if (!fSettings.HasInt32(kIdDeviceXdpi))		fSettings.AddInt32(kIdDeviceXdpi, 72);
	if (!fSettings.HasInt32(kIdDeviceYdpi))		fSettings.AddInt32(kIdDeviceYdpi, 72);
	if (!fSettings.HasBool(kIdColor))			fSettings.AddBool(kIdColor, true);
	if (!fSettings.HasFloat(kIdScale))			fSettings.AddFloat(kIdScale, 1.0f);
	if (!fSettings.HasInt32(kIdOrientation))	fSettings.AddInt32(kIdOrientation, B_PRINT_LAYOUT_PORTRAIT);
	if (!fSettings.HasInt32(kIdAttribute))		fSettings.AddInt32(kIdAttribute, B_PRINT_ATTR_ASSEMBLED);
}

void BPrintJobSettings::ClearSettings()
{
	fSettings.MakeEmpty();
}

bool BPrintJobSettings::IsEmpty() const
{
	return fSettings.IsEmpty();
}

const BMessage& BPrintJobSettings::Message() const
{
	return fSettings;
}

// ***********************************************************
#pragma mark -

BString	BPrintJobSettings::PrinterName() const
{
	const char *p;
	if ((p = fSettings.FindString(kIdPrinterName)))
		return BString(p);
	return BString();
}

uint32 BPrintJobSettings::FirstPage() const
{
	int32 page;
	if (fSettings.FindInt32(kIdFirstPage, &page) != B_OK)
		return 1;
	return (uint32)page;
}

uint32 BPrintJobSettings::LastPage() const
{
	int32 page;
	if (fSettings.FindInt32(kIdLastPage, &page) != B_OK)
		return LONG_MAX;
	return (uint32)page;
}

uint32 BPrintJobSettings::NumberOfCopies() const
{
	return (uint32)fSettings.FindInt32(kIdNumberOfCopies);
}

uint32 BPrintJobSettings::Xdpi() const
{
	return (uint32)fSettings.FindInt32(kIdXdpi);
}

uint32 BPrintJobSettings::Ydpi() const
{
	return (uint32)fSettings.FindInt32(kIdYdpi);
}

uint32 BPrintJobSettings::DeviceXdpi() const
{
	return (uint32)fSettings.FindInt32(kIdDeviceXdpi);
}

uint32 BPrintJobSettings::DeviceYdpi() const
{
	return (uint32)fSettings.FindInt32(kIdDeviceYdpi);
}

bool BPrintJobSettings::Color() const
{
	return fSettings.FindBool(kIdColor);
}

float BPrintJobSettings::Scale() const
{
	return fSettings.FindFloat(kIdScale);
}

BRect BPrintJobSettings::PaperRect() const
{
	return fSettings.FindRect(kIdPaperRect);
}

BRect BPrintJobSettings::PrintableRect() const
{
	return fSettings.FindRect(kIdPrintableRect);
}

BRect BPrintJobSettings::DeviceArea() const
{
	return fSettings.FindRect(kIdDeviceArea);
}

BRect BPrintJobSettings::DevicePrintableArea() const
{
	return fSettings.FindRect(kIdDevicePrintableArea);
}

orientation_type BPrintJobSettings::Orientation() const
{
	return (orientation_type)fSettings.FindInt32(kIdOrientation);
}

uint32 BPrintJobSettings::Attributes() const
{
	return (uint32)fSettings.FindInt32(kIdAttribute);
}

uint32 BPrintJobSettings::NumberOfPages() const
{
	const uint32 f = FirstPage();
	const uint32 l = LastPage();
	if ((f == 0) || (f > l))
		return 0;
	return (l - f + 1);
}

BRect BPrintJobSettings::DeviceMargins() const
{
	BRect p = PaperRect();
	BRect q = PrintableRect();
	if ((p.IsValid() == false) || (q.IsValid() == false))
		return BRect(0,0,0,0);
	BRect m(q.left-p.left, q.top-p.top, p.right-q.right, p.bottom-q.bottom);
	return m;
}

// ----------------------------------------------------
#pragma mark -

BPrintJobEditSettings::BPrintJobEditSettings()
	: BPrintJobSettings()
{
}

BPrintJobEditSettings::BPrintJobEditSettings(const BMessage& settings)
	: BPrintJobSettings(settings)
{
}

BPrintJobEditSettings::~BPrintJobEditSettings()
{
}

BMessage& BPrintJobEditSettings::Message()
{
	return fSettings;
}

status_t BPrintJobEditSettings::SetPrinterName(const char *printer_name)
{
	return UpdateString(kIdPrinterName, printer_name);
}
	
status_t BPrintJobEditSettings::SetFirstPage(uint32 page)
{
	UpdateInt32("first_page", page);
	return UpdateInt32(kIdFirstPage, page);
}

status_t BPrintJobEditSettings::SetLastPage(uint32 page)
{
	UpdateInt32("last_page", page);
	return UpdateInt32(kIdLastPage, page);
}

status_t BPrintJobEditSettings::SetNumberOfCopies(uint32 copies)
{
	return UpdateInt32(kIdNumberOfCopies, copies);
}

status_t BPrintJobEditSettings::SetXdpi(uint32 xdpi)
{
	return UpdateInt32(kIdXdpi, xdpi);
}

status_t BPrintJobEditSettings::SetYdpi(uint32 ydpi)
{
	return UpdateInt32(kIdYdpi, ydpi);
}

status_t BPrintJobEditSettings::SetDeviceXdpi(uint32 xdpi)
{
	UpdateInt32("xres", xdpi);
	return UpdateInt32(kIdDeviceXdpi, xdpi);
}

status_t BPrintJobEditSettings::SetDeviceYdpi(uint32 ydpi)
{
	UpdateInt32("yres", ydpi);
	return UpdateInt32(kIdDeviceYdpi, ydpi);
}

status_t BPrintJobEditSettings::SetColor(bool color)
{
	return UpdateBool(kIdColor, color);
}

status_t BPrintJobEditSettings::SetScale(float scale)
{
	return UpdateFloat(kIdScale, scale);
}

status_t BPrintJobEditSettings::SetPaperRect(BRect r)
{
	UpdateRect("paper_rect", r);
	return UpdateRect(kIdPaperRect, r);
}

status_t BPrintJobEditSettings::SetPrintableRect(BRect r)
{
	UpdateRect("printable_rect", r);
	return UpdateRect(kIdPrintableRect, r);
}

status_t BPrintJobEditSettings::SetDeviceArea(BRect r)
{
	return UpdateRect(kIdDeviceArea, r);
}

status_t BPrintJobEditSettings::SetDevicePrintableArea(BRect r)
{
	return UpdateRect(kIdDevicePrintableArea, r);
}

status_t BPrintJobEditSettings::SetOrientation(orientation_type orientation)
{
	return UpdateInt32(kIdOrientation, orientation);
}

status_t BPrintJobEditSettings::SetAttributes(uint32 attributes)
{
	return UpdateInt32(kIdAttribute, attributes);
}

// ---------------------------

status_t BPrintJobEditSettings::Normalize()
{ // Remove all non 'be:' items
	// REVISIT
	const char *name;
	type_code type;
	long count;
	for (int i=0 ; (fSettings.GetInfo(B_ANY_TYPE, i, &name, &type, &count) == B_OK) ; i++)
	{
		if (strncmp(name, "be:", 3))
		{
			if (!strcmp(name, "printable_rect"))	continue;
			if (!strcmp(name, "paper_rect"))		continue;
			if (!strcmp(name, "first_page"))		continue;
			if (!strcmp(name, "last_page"))			continue;
			if (!strcmp(name, "xres"))				continue;
			if (!strcmp(name, "yres"))				continue;
			fSettings.RemoveName(name);
			i--;
		}
	}
	return B_OK;
}

status_t BPrintJobEditSettings::UpdateInt32(const char *name, int32 i)
{
	if (fSettings.HasInt32(name))
		return fSettings.ReplaceInt32(name, i);
	return fSettings.AddInt32(name, i);
}

status_t BPrintJobEditSettings::UpdateBool(const char *name, bool i)
{
	if (fSettings.HasBool(name))
		return fSettings.ReplaceBool(name, i);
	return fSettings.AddBool(name, i);
}


status_t BPrintJobEditSettings::UpdateFloat(const char *name, float i)
{
	if (fSettings.HasFloat(name))
		return fSettings.ReplaceFloat(name, i);
	return fSettings.AddFloat(name, i);
}

status_t BPrintJobEditSettings::UpdateRect(const char *name, BRect i)
{
	if (fSettings.HasRect(name))
		return fSettings.ReplaceRect(name, i);
	return fSettings.AddRect(name, i);
}
status_t BPrintJobEditSettings::UpdateString(const char *name, const char *i)
{
	if (fSettings.HasString(name))
		return fSettings.ReplaceString(name, i);
	return fSettings.AddString(name, i);
}

status_t BPrintJobEditSettings::UpdateMessage(const char *name, const BMessage *i)
{
	if (fSettings.HasMessage(name))
		return fSettings.ReplaceMessage(name, i);
	return fSettings.AddMessage(name, i);
}
