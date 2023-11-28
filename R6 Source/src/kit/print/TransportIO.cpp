#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <Autolock.h>
#include <TypeConstants.h>
#include <Path.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <String.h>
#include <Errors.h>
#include <USB_printer.h>

#include <print/TransportIO.h>

#if (PRINTING_FOR_DESKTOP)
#	include <print/PrintConfigView.h>
#endif

#include <pr_server.h>

#define SPEED_STATISTICS	1

namespace BPrivate
{

	// ***************************************************************
	// MBuffer, for aynchronous I/O
	// ***************************************************************
	
	class MBuffer : public BDataIO
	{
	public:
					MBuffer(ssize_t length);
		virtual		~MBuffer(void);	
		virtual		ssize_t		Read		(void *buffer, size_t numBytes);
		virtual		ssize_t		Write		(const void *buffer, size_t numBytes);
					status_t	Sync		(void);
					void 		Quit		(void);
					void 		Abort		(void);
					ssize_t		GetSize		(void) {return fBufLength;};
	private:
		ssize_t BytesLeft(void)	{ return (fBufLength-fNbBytes); }
		char *fBufPtr;
		char *fBufEndPtr;
		ssize_t fBufLength;
		char *fReadPtr;
		char *fWritePtr;
		ssize_t fNbBytes;	
		BLocker	fLock;
		sem_id fWriteSem;
		sem_id fReadSem;
		bool fQuitRequested;
		bool fWriterLocked;
	};
	
	struct transport_io_private
	{
		size_t fAsyncBufferSize;
		BTransportAddOn *fTransportDevice;
		status_t fDeviceStatus;
		BPrivate::MBuffer *fBuffer;
		thread_id fThreadID;
		image_id fAddonImage;
		BTransportIOErrorHandler *fHandleErrorObject;
		bool fWillPrint;
		BNode *spool;
		#if SPEED_STATISTICS
		bigtime_t process_time;
		size_t process_nb;
		#endif
	};	

	class FakeAddOn : public BTransportAddOn
	{ public:
		FakeAddOn() : BTransportAddOn(NULL) { }
		virtual ssize_t Read(void *buffer, size_t size) { return B_NOT_ALLOWED; }
		virtual ssize_t Write(const void *buffer, size_t size) { return B_NOT_ALLOWED; }
		virtual uint32 GetAttributes() const  { return B_TRANSPORT_IS_SHARABLE; }
		virtual const char *Name() const  { return "None"; }
	};
	
} using namespace BPrivate;

// ***************************************************************
// BTransportIO
// ***************************************************************

BTransportIO::BTransportIO(BNode *printer)
	: BDataIO(),
	_fPrivate(new transport_io_private),
	m(*_fPrivate)
{
	m.fAsyncBufferSize = 0;
	m.fTransportDevice = NULL;
	m.fBuffer = NULL;
	m.fAddonImage = -1;
	m.fHandleErrorObject = NULL;
	m.fWillPrint = false;
	m.spool = NULL;
	#if SPEED_STATISTICS
	m.process_time = 0;
	m.process_nb = 0;
	#endif
	m.fDeviceStatus = InitObject(printer, m.fAsyncBufferSize);
}

BTransportIO::BTransportIO(BNode *printer, BNode *spool, size_t asyncBuffer)
	: BDataIO(),
	_fPrivate(new transport_io_private),
	m(*_fPrivate)
{
	m.fAsyncBufferSize = asyncBuffer;
	m.fTransportDevice = NULL;
	m.fBuffer = NULL;
	m.fAddonImage = -1;
	m.fHandleErrorObject = NULL;
	m.fWillPrint = true;
	m.spool = spool;
	#if SPEED_STATISTICS
	m.process_time = 0;
	m.process_nb = 0;
	#endif
	m.fDeviceStatus = InitObject(printer, m.fAsyncBufferSize);
}

status_t BTransportIO::InitObject(BNode *printer, size_t asyncBuffer)
{
	// find the transport
	status_t error;
	char transport_name[B_FILE_NAME_LENGTH];

	if (printer->ReadAttr(M_ATTR_TRANSPORT_NAME, B_STRING_TYPE, 0, transport_name, B_FILE_NAME_LENGTH) < 0)
	{ // Printer does not have an associated transport
		return B_NO_TRANSPORT;
	}
	
	if (!strcmp(transport_name, "None"))
	{ // No transport needed for this driver (Preview, probably)
		m.fTransportDevice = new FakeAddOn();
		return B_OK;
	}

	if (m.spool) // can be NULL on IAD
	{ // No transport needed in case of preview
		uint32 flags;
		bool preview = false;
		if (m.spool->ReadAttr(PSRV_SPOOL_ATTR_PREVIEW, B_UINT32_TYPE, 0, &flags, 4) == 4)
			preview = ((flags & 0x1) != 0);	// TODO: use a define instead of 0x01
		if (preview == true)
		{ // No transport needed, use the fake transport
			m.fTransportDevice = new FakeAddOn();
			return B_OK;
		}
	}

	BString path(M_TRANSPORT_ADDON_PATH);
	path << "/" << transport_name;
	m.fAddonImage = load_add_on(path.String());
	if (m.fAddonImage < 0)
	{ // The transport add-on does not exist or is damaged
		return B_BAD_TRANSPORT;
	}

	BTransportAddOn *(*instantiate_transport_addon)(BNode *printer_file);
	if ((error = get_image_symbol(m.fAddonImage, B_INSTANTIATE_TRANSPORT_ADDON_FUNCTION, B_SYMBOL_TYPE_TEXT, (void**)&instantiate_transport_addon)) < 0)
		return B_BAD_TRANSPORT;

	m.fTransportDevice = instantiate_transport_addon(printer);
	if (m.fTransportDevice->InitCheck() != B_OK)
		return m.fTransportDevice->InitCheck();

	if (m.fWillPrint)
	{
		BMessage jobInfo;
		if (m.spool)	// can be NULL on IAD
		{
			int32 pc;
			char n[257], d[257], p[257], c[257];
			m.spool->ReadAttr(PSRV_SPOOL_ATTR_JOB_NAME, B_STRING_TYPE, 0, n, 256);		n[256]=0;
			m.spool->ReadAttr(PSRV_SPOOL_ATTR_PAGECOUNT, B_INT32_TYPE, 0, &pc, 4);
			m.spool->ReadAttr(PSRV_SPOOL_ATTR_DESCRIPTION, B_STRING_TYPE, 0, d, 256);	d[256]=0;
			m.spool->ReadAttr(PSRV_SPOOL_ATTR_PRINTER, B_STRING_TYPE, 0, p, 256);		p[256]=0;
			m.spool->ReadAttr(PSRV_SPOOL_ATTR_MIMETYPE, B_STRING_TYPE, 0, c, 256);		c[256]=0;
			jobInfo.AddString(B_JOB_NAME, n);
			jobInfo.AddInt32(B_JOB_PAGE_COUNT, pc);
			jobInfo.AddString(B_JOB_DESCRIPTION, d);
			jobInfo.AddString(B_JOB_PRINTER_NAME, p);
			jobInfo.AddString(B_JOB_CREATOR_MIMETYPE, c);
		}
		if ((error = m.fTransportDevice->BeginPrintjob(jobInfo)) != B_OK)
			return error;
	}

	if (m.fAsyncBufferSize)
	{ // For buffered-multithreaded I/O
		m.fBuffer = new MBuffer(m.fAsyncBufferSize);
		m.fThreadID = spawn_thread(_fThreadFunc, "BTransportIO:send", B_NORMAL_PRIORITY, this); 
		resume_thread(m.fThreadID);
	}

	// The transport is initialized
	return B_OK;
}

BTransportIO::~BTransportIO()
{
	#if SPEED_STATISTICS
	if ((m.process_nb) && (m.process_time))
		printf("BTransportIO: output speed %lu KB/s\n",
				(uint32)((m.process_nb * (1000000.0/1024.0)) / m.process_time));
	#endif

	if (m.fBuffer)
	{ // flush the buffered I/O
		m.fBuffer->Quit();
		status_t result;
		wait_for_thread(m.fThreadID, &result);
		delete m.fBuffer;
	}

	if (m.fWillPrint)
		m.fTransportDevice->EndPrintjob();

	// clean-up the transport stuff...
	delete m.fTransportDevice;

	if (m.fAddonImage >= 0)
		unload_add_on(m.fAddonImage);

	delete _fPrivate;
}

status_t BTransportIO::GetDeviceID(BString& device_id) const {
	return m.fTransportDevice->GetDeviceID(device_id);
}
status_t BTransportIO::GetPortStatus(uint8 *status) const {
	return m.fTransportDevice->GetPortStatus(status);
}
status_t BTransportIO::SoftReset() const {
	return m.fTransportDevice->SoftReset();
}

BDataIO *BTransportIO::DataIO()
{
	return this;
}

uint32 BTransportIO::GetAttributes() const {
	return m.fTransportDevice->GetAttributes();
}
bool BTransportIO::IsReadable() const {
	return m.fTransportDevice->IsReadable();
}
bool BTransportIO::IsSharable() const {
	return m.fTransportDevice->IsSharable();
}
bool BTransportIO::IsHotPlug() const {
	return m.fTransportDevice->IsHotPlug();
}
bool BTransportIO::IsNetwork() const {
	return m.fTransportDevice->IsNetwork();
}

bool BTransportIO::handle_io_error(status_t& io_error, bool retry)
{
	if (m.fHandleErrorObject)
		return m.fHandleErrorObject->Error(io_error, retry);
	return false;
}

BPrintConfigView *BTransportIO::GetConfigView() {
	return m.fTransportDevice->GetConfigView();
}

status_t BTransportIO::SetErrorHandler(BTransportIOErrorHandler *error)
{
	m.fHandleErrorObject = error;
	return B_OK;
}

status_t BTransportIO::SetAccessMode(int access)
{
	return m.fTransportDevice->SetAccessMode(access);
}

int BTransportIO::AccessMode()
{
	return m.fTransportDevice->AccessMode();
}

ssize_t BTransportIO::Read(void *buffer, size_t numBytes)
{ // We don't care of error reading from the device. This will be handled by the driver
	return m.fTransportDevice->Read(buffer, numBytes);
}
	
ssize_t BTransportIO::Write(const void *buffer, size_t numBytes)
{
	if (m.fAsyncBufferSize)
	{ // Asynchronous output
		size_t to_send = numBytes;
		while (to_send > 0)
		{
			ssize_t sent = m.fBuffer->Write(buffer, to_send);
			if (sent < 0)
				return sent;
			to_send -= sent;
			buffer = ((char *)buffer) + sent;
		}
	}
	else
	{ // Synchronous output
		numBytes = _write(buffer, numBytes);
	}

	return numBytes;
}

ssize_t BTransportIO::_write(const void *buffer, size_t numBytes)
{
	ssize_t to_send;
	ssize_t sent;
	ssize_t result = numBytes;

	// Send data
	to_send = numBytes;
	sent = 0;
	while (to_send > 0)
	{
		// Send data to device

		#if SPEED_STATISTICS
			const bigtime_t now = system_time();
		#endif

		sent = m.fTransportDevice->Write(buffer, to_send);

		#if SPEED_STATISTICS
			if (sent >= 0) {
				m.process_nb += sent;
				m.process_time += (system_time() - now);
			}
		#endif

		if (sent < 0)
		{
			// There was an unrecoverable error (I/O error)
			status_t error = (status_t)sent;
			if (handle_io_error(error, false) == false)	// error is a status_t&
			{ //It is not possible to continue, report the error.
				result = error;
				break;
			}
			sent = 0; // No data where sent, then retry.
		}
		else if (sent < to_send)
		{
			// There was no error, but not all the data were sent. retry?
			status_t error = B_IO_ERROR;
			if (handle_io_error(error, true) == false)	// error is a status_t&
			{ // User canceled, return the error code
				result = error;
				break;
			}
			buffer = ((char *)buffer) + sent;
		}
		to_send -= sent;
	}

	return result;
}

status_t BTransportIO::InitCheck() const
{
	return m.fDeviceStatus;
}

status_t BTransportIO::Sync(void)
{
	if (m.fBuffer)
		return m.fBuffer->Sync();
	return B_OK;
}

int32 BTransportIO::_fThreadFunc(void *arg)
{
	BTransportIO *object = (BTransportIO *)arg;
	return (object->fThreadFunc());
}

int32 BTransportIO::fThreadFunc(void)
{
	// allocate the buffer on a 64 bytes boundary. It increases USB performance.
	const size_t length = m.fBuffer->GetSize();
	void *real_buffer = malloc(length + 64);
	if (!real_buffer) {
		m.fBuffer->Abort();
		return B_NO_MEMORY;
	}

	char *buffer = (char *)((((uintptr_t)real_buffer) + 64) & ~63);
	ssize_t sent;
	while ((sent = m.fBuffer->Read(buffer, length)) > 0)
	{ // There was an error when sending data to the transport device
		if (_write(buffer, sent) != sent)
		{ // delete the semaphores
			m.fBuffer->Abort();
			break;
		}
	}

	free(real_buffer);
	return 0;
}

// -------------------------------------------------------------------
// #pragma mark -

status_t BTransportIO::Probe(const char *transportName, BMessage& printers)
{
	status_t (*probe_port)(BMessage *in_out);

	BString path(M_TRANSPORT_ADDON_PATH);
	path << "/" << transportName;

	image_id image = load_add_on(path.String());
	if (image < 0)
	{ // The transport add-on does not exist or is damaged
		return B_BAD_TRANSPORT;
	}

	BTransportAddOn *(*instantiate_transport_addon)(BNode *printer_file);
	if ((get_image_symbol(image, B_INSTANTIATE_TRANSPORT_ADDON_FUNCTION, B_SYMBOL_TYPE_TEXT, (void**)&instantiate_transport_addon)) < 0)
	{
		unload_add_on(image);
		return B_BAD_TRANSPORT;
	}

	BTransportAddOn *transport = instantiate_transport_addon(NULL);
	if (transport->InitCheck() != B_OK)
	{
		unload_add_on(image);
		return transport->InitCheck();
	}

	transport->ProbePrinters(&printers);
	delete transport;
	unload_add_on(image);
	return B_OK;
}

// #pragma mark -

status_t BTransportIO::Perform(int32 selector, void * data)
{
	if (	(selector == USB_PRINTER_SET_STATUS_CAPABILITY) &&
			(m.fTransportDevice) &&
			(GetAttributes() & BTransportAddOn::B_TRANSPORT_IS_USB))
	{ // This is a gross hack, to be able to pass IOCTCLs to the USB Transport - Needed for HP printer (haa!)
		return m.fTransportDevice->Perform(selector, data);
	}
	return B_ERROR;
}

status_t BTransportIO::_Reserved_BTransportIO_0(int32 arg, ...) { return B_ERROR; }
status_t BTransportIO::_Reserved_BTransportIO_1(int32 arg, ...) { return B_ERROR; }
status_t BTransportIO::_Reserved_BTransportIO_2(int32 arg, ...) { return B_ERROR; }
status_t BTransportIO::_Reserved_BTransportIO_3(int32 arg, ...) { return B_ERROR; }
status_t BTransportIO::_Reserved_BTransportIO_4(int32 arg, ...) { return B_ERROR; }
status_t BTransportIO::_Reserved_BTransportIO_5(int32 arg, ...) { return B_ERROR; }
status_t BTransportIO::_Reserved_BTransportIO_6(int32 arg, ...) { return B_ERROR; }
status_t BTransportIO::_Reserved_BTransportIO_7(int32 arg, ...) { return B_ERROR; }


// ***************************************************************
// BTransportIOErrorHandler
// ***************************************************************
// #pragma mark -

BTransportIOErrorHandler::BTransportIOErrorHandler()
{
}

BTransportIOErrorHandler::~BTransportIOErrorHandler()
{
}

status_t BTransportIOErrorHandler::Perform(int32 selector, void * data) { return B_ERROR; }
status_t BTransportIOErrorHandler::_Reserved_BTransportIOErrorHandler_0(int32 arg, ...) { return B_ERROR; }
status_t BTransportIOErrorHandler::_Reserved_BTransportIOErrorHandler_1(int32 arg, ...) { return B_ERROR; }
status_t BTransportIOErrorHandler::_Reserved_BTransportIOErrorHandler_2(int32 arg, ...) { return B_ERROR; }
status_t BTransportIOErrorHandler::_Reserved_BTransportIOErrorHandler_3(int32 arg, ...) { return B_ERROR; }

// ***************************************************************
// MBuffer, for aynchronous I/O
// ***************************************************************
// #pragma mark -

MBuffer::MBuffer(ssize_t length)
	:	fBufLength(length),
		fNbBytes(0),
		fQuitRequested(false),
		fWriterLocked(false)
{
	fBufPtr = (char *)(malloc(fBufLength));
	fBufEndPtr = fBufPtr + fBufLength;
	fReadPtr = fWritePtr = fBufPtr;
	fWriteSem = create_sem(0, "MBuffer:write");
	fReadSem = create_sem(0, "MBuffer:read");
}

MBuffer::~MBuffer(void)
{
	delete_sem(fWriteSem);
	delete_sem(fReadSem);
	free(fBufPtr);
}


void MBuffer::Quit(void)
{
	BAutolock autolock(fLock);
	fQuitRequested = true;
	release_sem(fReadSem);
}

void MBuffer::Abort(void)
{
	BAutolock autolock(fLock);
	delete_sem(fWriteSem);
	delete_sem(fReadSem);
}

status_t MBuffer::Sync(void)
{
	if (!fLock.Lock())
		return B_ERROR;
	if (fNbBytes>0)
	{
		fWriterLocked = true;					
		release_sem(fReadSem);
		fLock.Unlock();
		return acquire_sem(fWriteSem);
	}
	fLock.Unlock();
	return B_OK;
}


ssize_t	MBuffer::Write(const void *buffer, size_t numBytes)
{
	if (!fBufPtr)
		return B_NO_MEMORY;

	if (numBytes == 0)
		return 0;

	while (true)
	{
		if (!fLock.Lock())
			return B_ERROR;

		if (BytesLeft())
		{
 			if (BytesLeft() < numBytes)
 				numBytes = BytesLeft();
 
 			ssize_t left = (ssize_t)fBufEndPtr - ((ssize_t)fWritePtr + numBytes);
			if (left >= 0)
			{
				memcpy(fWritePtr, buffer, numBytes);
				fWritePtr += numBytes;
			}
			else
			{
				ssize_t toEnd = (ssize_t)fBufEndPtr - (ssize_t)fWritePtr;
				memcpy(fWritePtr, buffer, toEnd);
				memcpy(fBufPtr, (char *)buffer+toEnd, numBytes-toEnd);
				fWritePtr = fBufPtr + (numBytes-toEnd);
			}
			fNbBytes += numBytes;
			release_sem(fReadSem);
			fLock.Unlock();
			return numBytes;
		}
		else
		{
			fWriterLocked = true;
			fLock.Unlock();
			if (acquire_sem(fWriteSem) != B_OK)
				return B_ERROR;
		}
	}
	return B_ERROR;
}

ssize_t	MBuffer::Read(void *buffer, size_t numBytes)
{
	if (!fBufPtr)
		return B_NO_MEMORY;

	if (numBytes == 0)
		return 0;

	while (true)
	{
		if (!fLock.Lock())
			return B_ERROR;
	
		ssize_t toRead = min_c(numBytes, fNbBytes);
		if (toRead > 0)	// TODO: we should have a limit here. For eg, wait to have at least 64 bytes (better for USB).
		{
			numBytes = toRead;
			ssize_t left = (ssize_t)fBufEndPtr - ((ssize_t)fReadPtr + numBytes);
			if (left >= 0)
			{
				memcpy(buffer, fReadPtr, numBytes);
				fReadPtr += numBytes;
			}
			else
			{
				ssize_t toEnd = (ssize_t)fBufEndPtr - (ssize_t)fReadPtr;
				memcpy(buffer, fReadPtr, toEnd);
				memcpy((char *)buffer+toEnd, fBufPtr, numBytes-toEnd);
				fReadPtr = fBufPtr + (numBytes-toEnd);
			}
			fNbBytes -= numBytes;
			if (fWriterLocked == true)
			{
				fWriterLocked = false;
				release_sem(fWriteSem);
			}
			fLock.Unlock();
			return numBytes;
		}
		else
		{
			if (fQuitRequested)
			{
				fLock.Unlock();
				return 0;
			}

			fLock.Unlock();
			if (acquire_sem(fReadSem) != B_OK)
				return B_ERROR;
		}
	}

	return B_ERROR;
}
