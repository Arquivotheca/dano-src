#ifndef TPRINT_TOOLS_H
#define TPRINT_TOOLS_H

#include <SupportDefs.h>
#include <String.h>
#include <Message.h>
#include <List.h>
#include <Node.h>
#include <image.h>

#define PSRV_PRINTER_ATTR_FREE		"free"
#define PSRV_PRINTER_ATTR_BUSY		"busy"

namespace BPrivate
{

//-----------------------------------------------------------------
// class TPrintTools
//-----------------------------------------------------------------

class TPrintTools
{
public:
			TPrintTools();
	virtual ~TPrintTools();

	// Printer and transports management
	static status_t PrinterAddedOrRemoved(const char *device);
	static status_t AddPrinters();
	static status_t ParsePrinter(const char *transport, const BMessage& replyMsg);

	static status_t CreatePrinter(	const char *printer_name,
									const char *model,
									const char *transport_path,
									const char *device,
									const char *addon_path,
									bool pnp);
	static status_t CreatePrinter(	const char *printer_name,
									const char *model,
									const char *transport_path,
									const char *device,
									const char *addon_path,
									bool pnp,
									int32 attr);

	static status_t GetTransports(BMessage *transports);
	static status_t match_driver(BString& relative_path, const char *mfg, const char *mdl);
	static status_t RemovePrinters();
	static status_t GetPrinterNode(const char *name, BNode *printer);
	static status_t GetPrinterSettingsNode(const char *printer_name, BNode *printer);
	static status_t FreeAllPrinters();

	// GUI tools	
	static void GetPreferredSize(BView *v, float *x, float *y);
	static const char *RsrcGetString(int index);

	// Loading an addon (preview job taken into account)
	static image_id LoadDriverAddOn(BNode *printer, BNode *spool = NULL);

	// Cancelling jobs
	static bool CancelRequested(BNode *printer, BNode *spool);
	static status_t CancelJob(BNode *spool, bool synchronous = false);
	static status_t CancelJobs(BNode *printer, bool synchronous = false);
	static status_t CancelJobs(const char *name = NULL, bool synchronous = false);
	static status_t CancelAllJobs(bool synchronous = false);
	static status_t ClearCancel(BNode *printer);
	
	// Default Settings management
	static status_t LoadSettings(const char *name, BMessage& settings, const char *user=NULL);
	static status_t SaveSettings(const char *name, const BMessage& settings, const char *user=NULL);
	static status_t LoadSettings(BNode *printer, BMessage& settings, const char *user=NULL);
	static status_t SaveSettings(BNode *printer, const BMessage& settings, const char *user=NULL);
private:
	static status_t create_printer(	const char *printer_name,
									const char *model,
									const char *transportName,
									const char *device,
									const char *driverName,
									bool pnp,
									int32 attributes);
};

//-----------------------------------------------------------------
// class PrinterPort
//-----------------------------------------------------------------

class PrinterPort
{
public:
	PrinterPort(BNode *printer, bool busy = false);
	PrinterPort(const PrinterPort& port);
	bool operator == (const PrinterPort& port) const;
	status_t SetBusy();
	status_t SetFree();
	bool IsFree();
private:
	BString fTransport;
	BString fTransportAddr;
	bool fBusy;
};

//-----------------------------------------------------------------
// class PrinterManager
//-----------------------------------------------------------------

class PrinterManager
{
public:
			PrinterManager(bool standAlone = false);
	virtual ~PrinterManager();
	status_t set_printer_busy(BNode *printer, BNode *job);
	status_t set_printer_free(BNode *printer);
	bool is_printer_free(BNode *printer);
private:
	status_t set_printerport_busy(BNode *printer);
	status_t set_printerport_free(BNode *printer);
	bool is_printerport_free(BNode *printer);

	status_t p_set_printerport_busy(BNode *printer);
	status_t p_set_printerport_free(BNode *printer);
	bool p_is_printerport_free(BNode *printer);

	bool has_sharable_transport(BNode *printer, BNode *job);
private:
	BList fPrinterPortList;
	bool fStandAlone;
};


//-----------------------------------------------------------------
// class PageIterator
//-----------------------------------------------------------------

class PageIterator
{
public:
	PageIterator(const int32 first, const int32 last, const int32 nbCopies=1, bool assembled=false, bool reverse=false);
	~PageIterator();
	status_t Rewind();
	status_t Next(int32 *page, int32 *nbCopies);
private:
	bool fAssembled;
	bool fReverse;
	int32 fNbCopies;
	int32 fFirst;
	int32 fLast;
	int32 fCurrentPage;
	int32 fCurrentCopie;
};


} using namespace BPrivate;

status_t get_default_printer(BString& printer_name);

#endif

