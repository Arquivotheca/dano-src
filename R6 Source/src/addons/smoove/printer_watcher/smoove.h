#include <Binder.h>
#include <print/PrinterConfigAddOn.h>

// ////////////////////////////////////////////////////////////

class PrintBinderNode : public BinderNode
{
protected:
	virtual	get_status_t ReadProperty(const char *name, property &prop, const property_list &args = empty_arg_list);
private:
	get_status_t selected(property &prop);
	get_status_t select(const property &prop);
	get_status_t update_printers(property &prop);
};

// ////////////////////////////////////////////////////////////

class BTransportIO;
class BPrinterConfigAddOn;
class BDirectory;

class PaperTypesNode;

class ConnectedPrinterBinderNode : public BinderNode
{
public:
	ConnectedPrinterBinderNode(const char *printerName);
	~ConnectedPrinterBinderNode();
	
protected:
	virtual	status_t OpenProperties(void **cookie, void *copyCookie);
	virtual	status_t NextProperty(void *cookie, char *nameBuf, int32 *len);
	virtual	status_t CloseProperties(void *cookie);
	virtual	get_status_t ReadProperty(const char *name, property &prop, const property_list &args = empty_arg_list);
	virtual	put_status_t WriteProperty(const char *name, const property &prop);

	status_t cache_driver_infos();
	BPrinterConfigAddOn::printer_mode_t *Modes() { return fModes; }
	int NbModes() { return fNbModes; }

private:
	friend class PaperTypesNode;

	status_t create_paper_types_node();
	status_t create_paper_formats_node();
	status_t load_printer_addon();
	status_t unload_printer_addon();
	BPrinterConfigAddOn &Driver() { return *fDriver; };

	// Cache
	bool fDriverInfoCached;

	// Functions-properties
	struct cookie_t { cookie_t() : index(0) { }; int index; };
	const char *fPropertyList[9];
	int	fNbProperties;

	// Driver management
	BString fPrinterName;
	image_id fAddonImageID;
	BTransportIO *fTransport;
	BPrinterConfigAddOn *fDriver;
	BDirectory *fPrinter;
	BLocker fDriverCountLock;
	int32 fLockCount;	

	binder_node PaperTypeAtom;		// points to a PaperTypesNode
	binder_node PaperFormatsAtom;	// points to a BinderContainer

	// Settings
	struct LocalPrinterModes : public BPrinterConfigAddOn::printer_mode_t
	{
		LocalPrinterModes();
		~LocalPrinterModes();
		LocalPrinterModes& operator = (const BPrinterConfigAddOn::printer_mode_t& copy);
	};
	
	BPrintJobEditSettings fSettings;
	BString *fPaperFormatsNames;
	int fNbPapers;
	LocalPrinterModes *fModes;
	int fNbModes;
	int32 fSelectedResolution;
	int32 fSelectedPaperFormat;
	char fOrientation;	// 'p' or 'l'
	
	// Status
	BString fStatus;
	double fInkLevel;

	status_t get_nth_mode(int i, BPrinterConfigAddOn::printer_mode_t const ** const mode);
};

// ////////////////////////////////////////////////////////////

class PaperTypesNode : public BinderContainer
{
public:
			PaperTypesNode(ConnectedPrinterBinderNode &node);
	virtual	status_t OpenProperties(void **cookie, void *copyCookie);
	virtual	get_status_t ReadProperty(const char *name, property &prop, const property_list &args = empty_arg_list);
	status_t update_modes_if_needed();
private:
	binder_node m_parent;
	ConnectedPrinterBinderNode& printer;
};

