// ***********************************************************************
// libprint.so
//
// (c) 2000, Be Incorporated
//
// Written by : Mathias Agopian
//
// ***********************************************************************

#ifndef _TRANSPORT_ADDON_H_
#define _TRANSPORT_ADDON_H_

#include <DataIO.h>
#include <Node.h>
#include <OS.h>
#include <SupportDefs.h>
#include <String.h>

class BTransportAddOn;
class BPrintConfigView;

// Must return a pointer to a newly created BTransportAddOn.
extern "C" BTransportAddOn *instantiate_transport_addon(BNode *printer_file);

#define B_INSTANTIATE_TRANSPORT_ADDON_FUNCTION	"instantiate_transport_addon"

// Name of the BMessage fields that must be filled by the BTransportAddOn
#define B_TRANSPORT_MSG_PRINTERS	"be:printers"
#define B_TRANSPORT_NAME			"be:name"
#define B_TRANSPORT_ATTRIBUTES		"be:attributes"
#define B_TRANSPORT_DEV_UNIQUE_NAME	"be:dev"
#define B_TRANSPORT_DEV_PATH		"be:pdev"
#define B_TRANSPORT_DEV_ID			"be:device_id"
#define B_TRANSPORT_DEV_DESCRIPTION	"be:port"

// Name of the fields in the BeginPrintjob's BMessage
#define B_JOB_NAME					"be:name"
#define B_JOB_PAGE_COUNT			"be:page_count"
#define B_JOB_DESCRIPTION			"be:job_description"
#define B_JOB_PRINTER_NAME			"be:printer_name"
#define B_JOB_CREATOR_MIMETYPE		"be:creator"


#define M_ATTR_TRANSPORT_ADDR		"transport_address"


// ***************************************************************
// A few defines
// ***************************************************************


class BTransportAddOn : public BDataIO
{
public:
	enum
	{
		B_TRANSPORT_IS_READABLE = 0x00000001,
		B_TRANSPORT_IS_SHARABLE = 0x00000002,
		B_TRAMSPORT_IS_HOT_PLUG = 0x00000004,
		B_TRANSPORT_IS_NETWORK  = 0x00000008,
		B_TRANSPORT_IS_USB  	= 0x00008000
	};

			BTransportAddOn(BNode *printer = NULL);
	virtual ~BTransportAddOn();

	// Allow to switch the current access mode
	virtual status_t SetAccessMode(int access_mode);
	virtual int AccessMode();

	// The BDataIO part
	virtual ssize_t Read(void *buffer, size_t size) = 0;
	virtual ssize_t Write(const void *buffer, size_t size) = 0;
	
	// Returns informations about this transport
	virtual uint32 GetAttributes() const = 0;
	virtual const char *Name() const = 0;

	// Implement this to be notified when the printjob is actually begining
	virtual status_t BeginPrintjob(const BMessage& jobInfos);
	virtual status_t EndPrintjob();

	// Implement this if the transport is B_TRAMSPORT_IS_HOT_PLUG or "probable"
	virtual status_t ProbePrinters(BMessage *printers);

	// Specials actions on the connected printer
	virtual status_t GetDeviceID(BString& device_id) const;
	virtual status_t GetPortStatus(uint8 *status) const;
	virtual status_t SoftReset() const;
	virtual	status_t InitCheck() const;

	// Implement this to have a configurable transport
	virtual BPrintConfigView *GetConfigView();

	// Helper function to decode the GetAttributes() result
	bool IsReadable() const;
	bool IsSharable() const;
	bool IsHotPlug() const;
	bool IsNetwork() const;
	
	virtual status_t Perform(int32 selector, void *data);
private:
	BTransportAddOn();
	BTransportAddOn(const BTransportAddOn&);
	BTransportAddOn& operator = (const BTransportAddOn &);
	virtual status_t _Reserved_BTransportAddOn_0(int32 arg, ...);
	virtual status_t _Reserved_BTransportAddOn_1(int32 arg, ...);
	virtual status_t _Reserved_BTransportAddOn_2(int32 arg, ...);
	virtual status_t _Reserved_BTransportAddOn_3(int32 arg, ...);
	uint32 reserved[4];
	int fLocalAccessMode;
};

#endif
