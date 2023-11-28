/*******************************************************************************
/
/	File:			PrintJob.h
/
/   Description:    BPrintJob runs a printing session.
/
/	Copyright 1996-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef	_PRINTSESSION_H
#define	_PRINTSESSION_H

#include <OS.h>
#include <image.h>
#include <Node.h>
#include <Message.h>
#include <Rect.h>

class BPicture;
class BView;
class BPrintJobSettings;
class BPositionIO;
class BList;

namespace BPrivate
{
	struct printjob_private;
}


class BPrintJob
{
public:
			BPrintJob(const char *job_name = NULL);
			BPrintJob(const BMessage& settings, const char *job_name = NULL, bool preview = false);
	virtual	~BPrintJob();
	status_t InitCheck() const;
	status_t Reset(const char *job_name = NULL);	// Returns what InitCheck() would return
	void HandleError(status_t error) const;


	// //////////////////////////////////////////////////////
	// Settings query
	const BPrintJobSettings& JobSettings();
	// ----- wrappers around querying JobSettings() ---------
	BRect		PaperRect();
	BRect		PrintableRect();
	status_t	GetResolution(int32 *xdpi, int32 *ydpi);
	int32		FirstPage();
	int32		LastPage();
	int32		PrinterType(void * = NULL) const;
		enum
		{ // These values are returned by PrinterType() -  deprecated
			B_BW_PRINTER = 0,
			B_COLOR_PRINTER
		};


	// //////////////////////////////////////////////////////
	// Job creation / Spooling
	status_t	BeginJob();	
	status_t	SetScale(float scale_factor = 1.0f);	// SetScale might be called _prior_ DrawView()
	status_t	SpoolPage();
	status_t	CommitJob();	
	void		CancelJob();
	bool		CanContinue();

virtual	status_t	DrawView(		BView *view,
									BRect clip,
									BPoint where);

		status_t	DrawPictures(	BPicture * const *pictures,
	                                const BRect *clips,
	                                const BPoint *where,
	                                const uint32 nb_pictures = 1);
 		
	// //////////////////////////////////////////////////////
	// User Interface 
	// ----- deprecated calls, use BPrintPanel instead -----
	status_t	ConfigPage();
	status_t	ConfigJob();
	BMessage 	*Settings();
	status_t	SetSettings(BMessage *msg);
	bool		IsSettingsMessageValid(BMessage *msg) const;

//----- Private or reserved -----------------------------------------

private:
	friend class BRawPrintJob;
	friend class BDirectPrintJob;
	friend class BPrinterRasterAddon;

	virtual void _ReservedPrintJob1();
	virtual void _ReservedPrintJob2();
	virtual void _ReservedPrintJob3();
	virtual void _ReservedPrintJob4();

				BPrintJob(const BPrintJob &);
				BPrintJob(const char *printer, const char *job_name);
	BPrintJob&	operator = (const BPrintJob &);

	void RecurseView(BView *, BPoint, BPicture *, BRect);
	void MangleName(char *);
	status_t EndLastPage();
	void AddSetupSpec();
	void AddPicture(BPicture *, BRect, BPoint);
	image_id load_driver_addon(BNode *node, BNode *job = NULL);
	inline BNode& PrinterNode() const;
	inline BMessenger& ServerMessenger() const;
	status_t update_settings(const BMessage&);
	status_t InitObject();
	status_t get_default_settings();
	void check_status(const char *);
	static void cleanup_spool_list(BList *, BList *);
	status_t CleanUpSpoolData();
	static long _take_job_add_on_thread(BMessage *);
	status_t SetPrinter(const char * = NULL);
	BPositionIO* spool_file() const;

private:
	BPrivate::printjob_private *_m_private;
	BPrivate::printjob_private& _m_rprivate;
	uint32 _reserved0[3];

	#if _R4_COMPATIBLE_
	uint32 _reserved1[89];
	public:
	// Just here for compatibility with old drivers
	struct print_file_header
	{ // Needed by printer old drivers
		int32	version;
		int32	page_count;
		off_t	first_page;
		int32	_reserved_3_;
		int32	_reserved_4_;
		int32	_reserved_5_;
	};
	#endif
};


#endif

