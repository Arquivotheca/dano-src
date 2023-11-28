#include <Application.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Directory.h>
#include <Node.h>
#include <GHandler.h>

#include "smoove.h"
#include "PrintEnv.h"
#include "NodeWatcher.h"

#define M_DEVICE_TO_WATCH	"/dev/printer/usb"

// ///////////////////////////////////////////////////////////////

//root / services / printing
//
//    *Selected()                            -> "unique_printer_name"
//    *Select(unique_printer_name)
//
//    stylus_740/
//        (...)
//    stylus_760/
//        (...)
//    unique_printer_name/
//        
//        *PrettyName()                     -> "Epson Stylus 740"
//        *Print()
//        *Cancel()
//        *Status()                         -> an code that identify the status
//        *InkLevel()                       -> 0 to 100% (double), exists if availlable
//        *SelectPrintMode(cookie)
//        *SelectedResolution()             -> "res3"
//        *SelectedPaperType()              -> "Coated Paper"
//        *SelectPaperFormat()
//        *SelectedPaperFormat()            -> "Letter"
//        *SetOrientation()
//        GetOrientation()            		-> "portrait/landscape"
//        papers/
//            "Plain Paper"/
//                res0()                    -> cookie
//                res1()
//                res2()
//            "Coated Paper"/
//                res3()
//                res4()
//                res5()
//            "Photo Paper"/
//                res6()
//                res7()
//        paper_formats/
//            "Letter"						-> cookie
//            "A4"							-> cookie


// ///////////////////////////////////////////////////////////////

class USBPrinterWatcher : public NodeWatcher
{
public:
	USBPrinterWatcher() : NodeWatcher(be_app)
	{
		add_printer();
		BNode node(M_DEVICE_TO_WATCH);
		node.GetNodeRef(&fWatchNodeRef);
		StartWatching(fWatchNodeRef, B_WATCH_DIRECTORY);
	};
	~USBPrinterWatcher() { StopWatching(fWatchNodeRef); }
	virtual void EntryCreated(entry_ref& , node_ref& )
	{
		add_printer();
	}
	virtual void EntryRemoved(node_ref& , node_ref& )
	{ // Because of a bug in devfs, we stop and relaunch the node monitor each time a device is removed
		StopWatching(fWatchNodeRef);
		TPrintTools::PrinterAddedOrRemoved(M_DEVICE_TO_WATCH);
		BinderNode::property update = BinderNode::Root() / "service" / "printing" / "be:update";
		BNode node(M_DEVICE_TO_WATCH);
		node.GetNodeRef(&fWatchNodeRef);
		StartWatching(fWatchNodeRef, B_WATCH_DIRECTORY);
	}

private:
	void add_printer()
	{
		if (TPrintTools::PrinterAddedOrRemoved(M_DEVICE_TO_WATCH) == B_OK)
			BinderNode::property update = BinderNode::Root() / "service" / "printing" / "be:update";
	}

	node_ref fWatchNodeRef;
};

// ///////////////////////////////////////////////////////////////

class PrinterWatcherSmooved : public GHandler
{
public:
	PrinterWatcherSmooved() : GHandler(), fWatcher(NULL)
	{
		BinderNode::property printing = BinderNode::Root() / "service" / "printing";
		(new PrintBinderNode)->StackOnto(printing.Object());
		fWatcher = new USBPrinterWatcher;
	}

	virtual ~PrinterWatcherSmooved()
	{
		delete fWatcher;
	}

private:
	USBPrinterWatcher *fWatcher;
};

// ///////////////////////////////////////////////////////////////

extern "C" _EXPORT GHandler *return_handler(GDispatcher * /* dispatcher */)
{
	return new PrinterWatcherSmooved();
}

