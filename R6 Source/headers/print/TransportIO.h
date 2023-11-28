// ***********************************************************************
// libprint.so
//
// (c) 2000, Be Incorporated
//
// Written by : Mathias Agopian
//
// ***********************************************************************

#ifndef _TRANSPORT_IO_H_
#define _TRANSPORT_IO_H_

#include <DataIO.h>
#include <Node.h>
#include <OS.h>
#include <SupportDefs.h>
#include <String.h>
#include <print/TransportAddOn.h>


// ***************************************************************
// A few defines
// ***************************************************************

#define M_TRANSPORT_ADDON_PATH	"Print/transport"
#define M_ATTR_TRANSPORT_NAME	"transport"

namespace BPrivate
{
	class MBuffer;
	struct transport_io_private;
}


class BTransportIOErrorHandler;
class BPrintConfigView;


class BTransportIO : BDataIO
{
public:
			BTransportIO(BNode *printer);
			BTransportIO(BNode *printer, BNode *spool, size_t = 0);
	virtual ~BTransportIO();

	// Change access mode
	status_t SetAccessMode(int access_mode);
	int AccessMode();

	// BDataIO inherited methods	
	virtual ssize_t Read(void *buffer, size_t size);
	virtual ssize_t Write(const void *buffer, size_t size);

	// Access to the stream
	status_t Sync(void);
	status_t GetDeviceID(BString& device_id) const;
	status_t GetPortStatus(uint8 *status) const;
	status_t SoftReset() const;
	status_t InitCheck() const;
	BDataIO *DataIO();

	// User hook for error handling
	status_t SetErrorHandler(BTransportIOErrorHandler *error);

	// This transport's attributes (features)
	uint32 GetAttributes() const;
	bool IsReadable() const;
	bool IsSharable() const;
	bool IsHotPlug() const;
	bool IsNetwork() const;

	// Printer probing
	static status_t Probe(const char *transportName, BMessage& printers);

	// Get the configuration view
	BPrintConfigView *GetConfigView();

	virtual status_t Perform(int32 selector, void *data);
private:
	BTransportIO();
	BTransportIO(const BTransportIO&);
	BTransportIO& operator = (const BTransportIO &);
	virtual status_t _Reserved_BTransportIO_0(int32 arg, ...);
	virtual status_t _Reserved_BTransportIO_1(int32 arg, ...);
	virtual status_t _Reserved_BTransportIO_2(int32 arg, ...);
	virtual status_t _Reserved_BTransportIO_3(int32 arg, ...);
	virtual status_t _Reserved_BTransportIO_4(int32 arg, ...);
	virtual status_t _Reserved_BTransportIO_5(int32 arg, ...);
	virtual status_t _Reserved_BTransportIO_6(int32 arg, ...);
	virtual status_t _Reserved_BTransportIO_7(int32 arg, ...);


	status_t InitObject(BNode *, size_t);
	static	int32 _fThreadFunc(void *);
			int32 fThreadFunc(void);
			ssize_t _write(const void *, size_t);
	bool handle_io_error(status_t&, bool);

private:
	BPrivate::transport_io_private	*_fPrivate;
	BPrivate::transport_io_private&	m;
	uint32 reserved[4];
};



// Error handling object
class BTransportIOErrorHandler
{
public:
			BTransportIOErrorHandler();
	virtual ~BTransportIOErrorHandler();
	virtual bool Error(status_t& io_error, bool retry) = 0;
private:
	BTransportIOErrorHandler(const BTransportIO&);
	BTransportIOErrorHandler& operator = (const BTransportIO &);
	virtual status_t _Reserved_BTransportIOErrorHandler_0(int32 arg, ...);
	virtual status_t _Reserved_BTransportIOErrorHandler_1(int32 arg, ...);
	virtual status_t _Reserved_BTransportIOErrorHandler_2(int32 arg, ...);
	virtual status_t _Reserved_BTransportIOErrorHandler_3(int32 arg, ...);
	virtual status_t Perform(int32 selector, void *data);
	uint32 reserved[4];
};

#endif
