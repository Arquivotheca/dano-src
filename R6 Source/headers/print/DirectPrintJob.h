/*******************************************************************************
/
/	File:			DirectPrintJob.h
/
/	Copyright 1996-2001, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef	_DIRECT_PRINTJOB_H
#define	_DIRECT_PRINTJOB_H

#include <OS.h>
#include <Message.h>
#include <Rect.h>
#include <PrintJob.h>
#include <print/PrinterRasterAddOn.h>
#include <print/PrinterAddOnDefs.h>

class BView;
class BPicture;
class BBitmap;

namespace BPrivate
{
	class PrinterManager;
	struct direct_printjob_private;
}

class BDirectPrintJob : public BPrintJob
{
public:
			BDirectPrintJob(const char *job_name = NULL);
			BDirectPrintJob(const BMessage& settings, const char *job_name = NULL);
	virtual	~BDirectPrintJob();

	// //////////////////////////////////////////////////////
	// Job creation / Spooling
	status_t	StartJob(uint32 nbPages, uint32 flags = 0);
	void		CancelJob();
	bool		CanContinue();

protected:
	// //////////////////////////////////////////////////////
	// Hooks
	
	struct printer_descriptor_t
	{ // all dimentions in pixels
		uint32 width;
		uint32 height;
		color_space space;
		float aspect_ratio;	// ydpi/xdpi
		uint32 _rsvr[4];
	};

	struct bitmap_rect_t
	{ // all dimentions in pixels
		uint32 page;
		uint32 left;
		uint32 top;
		uint32 width;
		uint32 height;
		uint32 _rsvr[4];
	};
	
	virtual status_t BeginJob(const printer_descriptor_t& page);
	virtual status_t EndJob();
	virtual status_t BeginPage(const uint32 page);
	virtual status_t EndPage();
	virtual status_t FillBitmap(const bitmap_rect_t& rect, BBitmap& bitmap) = 0;

private:
			BDirectPrintJob(const BDirectPrintJob &);
	BDirectPrintJob& operator = (const BDirectPrintJob &);

	// //////////////////////////////////////////////////////
	// Cannot be used from BDirectPrintJob : B_UNSUPPORTED
	virtual	status_t	DrawView(BView *, BRect, BPoint);
			status_t	DrawPictures(BPicture * const *, const BRect *, const BPoint *, const uint32);
			status_t	BeginJob();
			status_t	SpoolPage();
			status_t	CommitJob();	
			status_t	SetScale(float scale_factor = 1.0f);

	// //////////////////////////////////////////////////////
	// FBC
	virtual status_t _ReservedDirectPrintJob1();
	virtual status_t _ReservedDirectPrintJob2();
	virtual status_t _ReservedDirectPrintJob3();
	virtual status_t _ReservedDirectPrintJob4();
	virtual status_t Perform(int32 code, ...);

private:
	friend class BPrinterRasterAddOn;
	status_t InitObject();
	BPrivate::PrinterManager *printer_manager();
	BPrivate::direct_printjob_private *_m_private;
	BPrivate::direct_printjob_private& _m_rprivate;
	uint32 _reserved0[3];
};

 
#endif

