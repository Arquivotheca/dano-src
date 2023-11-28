#include <stdio.h>
#include "CDMMCDriver.h"

#include "SCSIDevice.h"
#include "CDTrack.h"

#include "Pages.h"

#define REALBURN 1

status_t CloseSession(SCSIDevice *dev)
{
	uchar cmd[10] = { 0x5b, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	return dev->CmdOut(NULL, 0, cmd, 10);		
}

status_t SyncCache(SCSIDevice *dev)
{
/*
	uchar cmd[10];
	cmd[0] = 0x5D;
	cmd[1] = 0x00;
	cmd[2] = 0x00;
	cmd[3] = 0x00;	
	cmd[4] = 0x00;	
	cmd[5] = 0x00;	
	cmd[6] = ((size >> 16) & 0xff);	
	cmd[7] = ((size >> 8 ) & 0xff);	
	cmd[8] = (size & 0xff);	
	cmd[9] = 0x00;	
	
	return dev->CmdOut(data, size, cmd, 10);\
*/return B_OK;
}

status_t DoOPC(SCSIDevice *dev)
{
	return B_OK;
}


status_t WriteBlocks(SCSIDevice *dev, int lba, size_t count, 
					 void *data, size_t length)
{
	return B_OK;
}


float
CDMMCDriver::PercentFifo(void)
{return 0.0;}

float
CDMMCDriver::PercentDone(void)
{
	return 0.0;
}

CDMMCDriver::CDMMCDriver(void)
{
	errmsg[0] = 0;
	looper = NULL;
	dev = NULL;
}


CDMMCDriver::~CDMMCDriver()
{
}

const char *
CDMMCDriver::Name(void)
{
	return "SCSI-3 MMC";
}

const char *
CDMMCDriver::DeviceName(void)
{
	if(dev) {
		InquiryInfo ii;
		if(dev->Inquiry(ii) == B_OK){
			sprintf(devname,"%s %s %s",ii.Vendor(),ii.Product(),ii.Revision());
			return devname;
		} else {
			return "MMC-3 Compliant CDR";
		}
	} else {
		return "<none>";
	}
}

const char *
CDMMCDriver::DevicePath(void)
{
	if(dev) {
		return dev->Path();
	} else {
		return "<none>";
	}
}


bool 
CDMMCDriver::IsSupportedDevice(SCSIDevice *dev)
{
	WriteParamsPage wpp;
	if(wpp.ReadPage(dev)){
		return false;
	} else {
		return true;
	}
}

bool 
CDMMCDriver::Burning(void)
{
	return false;
}

bool 
CDMMCDriver::IsBlankDisc(void)
{
	return false;
}

CDDriver *
CDMMCDriver::GetDriverInstance(SCSIDevice *dev, BLooper *looper)
{
	CDMMCDriver *d = new CDMMCDriver();
	d->dev = dev;
	d->looper = looper;
	return d;
}
status_t 
CDMMCDriver::Check(CDTrack */*tracks*/)
{
	if(!dev) {
		Error("no device?");
		return B_ERROR;
	}
	return B_OK;
}

status_t 
CDMMCDriver::Start(CDTrack *first)
{

return B_OK;
	
}
void 
CDMMCDriver::Error(const char *error, bool scsi_info)
{
	if(scsi_info){
		sprintf(errmsg,"%s (%02x:%02x:%02x)", error,
				dev->GetSenseKey(), dev->GetASC(), dev->GetASCQ());
	} else {
		strncpy(errmsg,error,255);
	}
}


status_t 
CDMMCDriver::Abort(void)
{
	return B_ERROR;
}

char *
CDMMCDriver::GetError(void)
{
	return errmsg;
}
