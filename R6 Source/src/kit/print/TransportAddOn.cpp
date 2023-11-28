#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <Autolock.h>
#include <TypeConstants.h>
#include <Path.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <String.h>
#include <Errors.h>
#include <Entry.h>
#include <USB_printer.h>

#include <print/TransportAddOn.h>


// ***************************************************************
// BTransportAddOn
// ***************************************************************

BTransportAddOn::BTransportAddOn(BNode *printer)
	: 	BDataIO(),
		fLocalAccessMode(B_NO_INIT)
{
}

BTransportAddOn::~BTransportAddOn()
{
}

status_t BTransportAddOn::SetAccessMode(int access)
{
	if (((GetAttributes() & B_TRANSPORT_IS_READABLE) == 0) &&
		(access!=O_WRONLY))
		return EPERM;
	fLocalAccessMode = access;
	return B_OK;
}

int BTransportAddOn::AccessMode()
{
	return fLocalAccessMode;
}

status_t BTransportAddOn::BeginPrintjob(const BMessage&)
{
	// We will start to print. So make sure the transport is opened for writing at least
	// Try READ and WRITE first	
	if (SetAccessMode(O_RDWR) != B_OK)
		SetAccessMode(O_WRONLY);	// Failure, try write only.
	return InitCheck();
}

status_t BTransportAddOn::EndPrintjob()
{
	// Printjob finished. We can switch back to READ_ONLY mode
	SetAccessMode(O_RDONLY);
	return InitCheck();
}

status_t BTransportAddOn::GetDeviceID(BString& device_id) const
{
	device_id = B_EMPTY_STRING;
	return B_OK;
}

status_t BTransportAddOn::GetPortStatus(uint8 *status) const
{
	if (status == NULL)
		return B_BAD_VALUE;
	return B_NOT_ALLOWED;
}

status_t BTransportAddOn::SoftReset() const {
	return B_NOT_ALLOWED;
}
status_t BTransportAddOn::InitCheck() const {
	return B_NO_INIT;
}

status_t BTransportAddOn::ProbePrinters(BMessage *printers)
{
	printers->MakeEmpty();
	return B_OK;
}

bool BTransportAddOn::IsReadable() const {
	return ((GetAttributes() & B_TRANSPORT_IS_READABLE) != 0);
}
bool BTransportAddOn::IsSharable() const {
	return ((GetAttributes() & B_TRANSPORT_IS_SHARABLE) != 0);
}
bool BTransportAddOn::IsHotPlug() const {
	return ((GetAttributes() & B_TRAMSPORT_IS_HOT_PLUG) != 0);
}
bool BTransportAddOn::IsNetwork() const {
	return ((GetAttributes() & B_TRANSPORT_IS_NETWORK) != 0);
}

BPrintConfigView *BTransportAddOn::GetConfigView() {
	return NULL;
}

status_t BTransportAddOn::Perform(int32 selector, void * data) { return B_ERROR; }
status_t BTransportAddOn::_Reserved_BTransportAddOn_0(int32 arg, ...) { return B_ERROR; }
status_t BTransportAddOn::_Reserved_BTransportAddOn_1(int32 arg, ...) { return B_ERROR; }
status_t BTransportAddOn::_Reserved_BTransportAddOn_2(int32 arg, ...) { return B_ERROR; }
status_t BTransportAddOn::_Reserved_BTransportAddOn_3(int32 arg, ...) { return B_ERROR; }

