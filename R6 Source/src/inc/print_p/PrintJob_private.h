// ***********************************************************************
// libprint.so
//
// (c) 2000, Be Incorporated
//
// Written by : Mathias Agopian
//
// ***********************************************************************

#ifndef _PRINTJOB_PRIVATE_H_
#define _PRINTJOB_PRIVATE_H_

namespace BPrivate
{
	struct printjob_private
	{
		printjob_private()
			: 	print_job_name(NULL),
				spoolFile(NULL),
				fCancelRequested(false),
				fStandAlone(false),
				fInitCheck(B_NO_INIT),
				fPageHeadersList(NULL),
				fPagePicturesList(NULL),
				fPrinterManager(NULL),
				fJobStarted(false),
				fPreview(false),
				raw(false),
				scale(1.0f)
		{
			m_curPageHeader = new _page_header_;
		}
		
		~printjob_private()
		{
			free(print_job_name);
			delete m_curPageHeader;
			delete fPrinterManager;
		}
		
		status_t re_init()
		{
			spoolFile = NULL;
			strcpy(spool_file_name, B_EMPTY_STRING);
			fPageHeadersList = NULL;
			fPagePicturesList = NULL;
			fCancelRequested = false;
			delete m_curPageHeader;
			m_curPageHeader = new _page_header_;
			fJobStarted = false;
			scale = 1.0f;
			return B_OK;
		}
		
		char *print_job_name;
		BPositionIO *spoolFile;
		spool_header_t current_header;
		char spool_file_name[256];	
		bool fCancelRequested;
		bool fStandAlone;
		BMessenger fMessenger;
		BString fPrinterName;
		BNode fPrinterNode;
		int32 fPrinterType;
		BPrintJobSettings fSettings;
		_page_header_ *m_curPageHeader;
		off_t m_curPageHeaderOffset;
		status_t fInitCheck;
		BList *fPageHeadersList;
		BList *fPagePicturesList;
		PrinterManager *fPrinterManager;
		bool fJobStarted;
		bool fPreview;
		bool raw;
		float scale;
	};
};


#endif
