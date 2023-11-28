
#ifndef SCSIDEVICE_H
#define SCSIDEVICE_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <scsi.h>
#include <CAM.h>

class SCSIDevice;

class ModePage 
{
public:
	ModePage(uint number, uint length);
	virtual ~ModePage();
	
	uint ReadByteAt(uint ofs);
	void WriteByteAt(uint ofs, uint value);
	uint32 ReadUIntAt(uint ofs, uint len);
	void WriteUIntAt(uint ofs, uint len, uint value);
	uchar ReadBitsAt(uint ofs, uint start, uint len);
	void WriteBitsAt(uint ofs, uint start, uint len, uint value);
	
	virtual status_t ReadPage(SCSIDevice *dev);
	virtual status_t WritePage(SCSIDevice *dev);
	
protected:
	uchar *data;
	uchar *realdata;
	uint number;
	uint length;
};

class InquiryInfo
{
public:
	InquiryInfo(){
		revision[0] = 0;
		product[0] = 0;
		vendor[0] = 0;
		memset(data,0,36);
	}
	char *Revision(void){ return revision; }
	char *Product(void){ return product; }
	char *Vendor(void) { return vendor; }
	uint DeviceType(void) { return data[0] & 0x1f; }
	uint Qualifier(void) { return (data[0] & 0xe0) >> 5; }
	
private:
	friend class SCSIDevice;
	char revision[5];
	char product[17];
	char vendor[9];
	uchar data[36];	
};


class SCSIDevice 
{

public:
	SCSIDevice(const char *path);
	~SCSIDevice();
	
	status_t InitCheck(void);

	status_t TestReady(void);
	status_t Inquiry(InquiryInfo &info);
	
	status_t CmdIn(void *data, size_t len, uchar *cmd, size_t cmdlen);
	status_t CmdOut(void *data, size_t len, uchar *cmd, size_t cmdlen);
	
	uint GetSenseKey(void) { return sense[2] & 0x0f; }
	uint GetASC(void) { return sense[12]; }
	uint GetASCQ(void) { return sense[13]; }
	
	bool IsError(uint key, uint asc, uint ascq){
		return ((key == (uint)(sense[2] & 0x0f)) && (asc == sense[12]) && (ascq == sense[13])) ? true : false;
	}
	void DumpErrorInfo(void);
	
	const char* Path(void);
	
private:
	uchar sense[32];
	status_t Execute(raw_device_command *rdc);
	
	bigtime_t timeout;
	void ErrorInfo(uchar *sense);
	
	int fd;
	char *path;
};


#endif

