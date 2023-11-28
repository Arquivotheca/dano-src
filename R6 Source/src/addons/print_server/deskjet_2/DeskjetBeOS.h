//--------------------------------------------------------------------
//
//	Written by: Mathias Agopian
//	
//	Copyright 2000 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef _DESKJET_BEOS_H
#define _DESKJET_BEOS_H

#include <ListItem.h>
#include <Node.h>
#include <Directory.h>

#include "HPPrintAPI.h"
#include <print/PrinterAddOnDefs.h>
#include <print/PrinterConfigAddOn.h>
#include <print/PrinterRasterAddOn.h>
#include <print/TransportIO.h>
#include <print/PrintConfigView.h>
#include <print/PrintPaper.h>


class BDeskjet;
class BSystemServices;
class BView;
class BBitmap;

// /////////////////////////////////////////////////////////////////////////////////////

class DeskjetTools
{
public:
	DeskjetTools(BTransportIO *transport, BDeskjet *deskjet = NULL)
		:	fTransport(transport), fCancelRequested(false),
			fPrinterStatus(printer_status_t::B_UNKNOWN),
			fDeskjet(deskjet)
	{
	}
	status_t ConvertErrorCodes(int hperror) const;
	BTransportIO *Transport() { return fTransport; }
	BDeskjet *Deskjet() { return fDeskjet; }

	int32 fPrinterStatus;
	bool fCancelRequested;

private:
	BTransportIO *fTransport;
	BDeskjet *fDeskjet;
};

// /////////////////////////////////////////////////////////////////////////////////////

class BDeskjet : public BPrinterRasterAddOn, public BTransportIOErrorHandler
{
public:
			BDeskjet(BTransportIO* transport, BNode *printer_file);
	virtual ~BDeskjet();
private:
	virtual	status_t BeginJob();
	virtual	status_t EndJob();
	virtual status_t BeginPage(const print_bitmap_t *bitmap);
	virtual status_t EndPage();
	virtual status_t OutputData(const print_bitmap_t *bitmap);
	virtual status_t GetSupportedDeviceID(uint32 *count, printer_id_t **ids);
	virtual status_t Cancel();
	virtual bool Error(status_t& io_error, bool retry);
	virtual status_t InitCheck() { return fInitCheck; }
private:
	status_t fInitCheck;
	status_t ConvertErrorCodes(int hperror);
	BSystemServices *fSystem;
	PrintContext *fPrintContext;
	Job *fJob;
	uint8 *fTempBuffer;
	DeskjetTools *fDeskjetTools;
};

// /////////////////////////////////////////////////////////////////////////////////////

class BDeskjetConfig : public BPrinterConfigAddOn
{
friend class BSystemServices;
public:
			BDeskjetConfig(BTransportIO* transport, BNode *printer_file);
	virtual ~BDeskjetConfig();
private:
	virtual BPrintConfigView *AddPrinter(const char *printername, BDirectory *spooldir);
	virtual int32 PrinterModes(printer_mode_t const **modes);
	virtual status_t PrinterModeSelected(int32 index);
	virtual status_t PaperSelected(int32 tray, int32 paper);
	virtual int32 PaperFormats(int32 tray, BPrintPaper const **papers);
			void set_paper_format(int id, PAPER_SIZE hpid, int index, const char *name = NULL);

	virtual status_t CleanPrintHeads(bool is_feature_availlable=false);
	virtual status_t PrintNozzleCheckPattern(bool is_feature_availlable=false);
	virtual status_t PrinterStatus(printer_status_t *status);
	virtual status_t InitCheck() { return fInitCheck; }

	enum
	{
		HP_PHOTO_SIZE = BPrintPaper::B_START_USERDEF_PAPER
	};
	status_t fInitCheck;
	DeskjetTools *fDeskjetTools;
	BSystemServices *fSystem;
	PrintContext *fPrintContext;
	printer_mode_t *fPrinterModes;
	int32 fPrinterModesCount;
	BPrintPaper fPaperFormats[4];
	int *fBeToHpPrintModes;
};

// /////////////////////////////////////////////////////////////////////////////////////

class BSystemServices : public SystemServices
{
public:
			BSystemServices(DeskjetTools *deskjet);
	virtual ~BSystemServices();
	void InitDevice();
    virtual void DisplayPrinterStatus(DISPLAY_STATUS ePrinterStatus);
private:
	virtual BOOL PrinterIsAlive();
    virtual DRIVER_ERROR BusyWait(DWORD msec);
    virtual DRIVER_ERROR ReadDeviceID(BYTE* strID, int iSize);
    virtual BYTE* AllocMem (int iMemSize);
	virtual void FreeMem (BYTE* pMem);
    virtual BOOL GetStatusInfo(BYTE* bStatReg);
    virtual DRIVER_ERROR ToDevice(const BYTE* pBuffer, DWORD* wCount);
    virtual DRIVER_ERROR FromDevice(char* pReadBuff, WORD* wReadCount);
    virtual BOOL YieldToSystem (void);
    virtual BYTE GetRandomNumber();
    virtual DWORD GetSystemTickCount (void);
    virtual float power(float x, float y);
    virtual DRIVER_ERROR FlushIO();
    virtual DRIVER_ERROR AbortIO();

	DeskjetTools *fDeskjet;

	enum
	{
	    CRC_ALEA_INIT = 0x1f3678ef,
	    CRC_ALEA_STEP = 0x249b06d7
	};

	int32 crc_alea;
};

// /////////////////////////////////////////////////////////////////////////////////////

class AddPrinterView : public BPrintConfigView
{
public:
			AddPrinterView(PrintContext *context, BNode *printer);
	virtual ~AddPrinterView();

	virtual status_t Save();
	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage *message);
	virtual void GetMinimumSize(float *width, float *height);

private:

	class BDJItem : public BStringItem
	{ public:
		BDJItem(const char *name, int32 v) : BStringItem(name), fValue(v) { }
		int32 fValue;
	};
	
	BNode *Printer() { return fPrinter; };
	
	int32 fDeskJetType;
	BListView *fPrinterList;
	BNode *fPrinter;
};

#endif
