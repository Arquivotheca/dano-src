
#include "SCSIDevice.h"
 
void hexout(void *bytes, uint len)
{
	uint ofs = 0;
	uchar *data = (uchar *) bytes;
	
	while(len){
		if(!(ofs % 16)) printf("%04x: ",ofs);
		printf("%02x ", data[ofs]);
		ofs++;len--;
		if(len && !(ofs % 16)) printf("\n");
	}
	printf("\n");
}

SCSIDevice::SCSIDevice(const char *path)
{
	timeout = 1000000 * 60;
	if(path){
		fd = open(path, O_RDWR);
		if(fd >= 0){
			this->path = strdup(path);
			return;
		}
	}
	this->path = NULL;
	fd = -1;
}


SCSIDevice::~SCSIDevice()
{
	if(fd != -1) {
		close(fd);
	}
	if(path){
		free(path);
	}
}
status_t 
SCSIDevice::TestReady(void)
{
	uchar sense[32];
	
	if(fd == -1) return B_ERROR;
	
	int e;
	raw_device_command rdc;
	rdc.data = NULL;
	rdc.data_length = 0;
	rdc.flags = 0;
	rdc.sense_data = sense;
	rdc.sense_data_length = 32;
	rdc.timeout = 1000000;
	rdc.command_length = 6;
	rdc.command[0] = 0x00;
	rdc.command[1] = 0x00;
	rdc.command[2] = 0x00;
	rdc.command[3] = 0x00;	
	rdc.command[4] = 0x00;	
	rdc.command[5] = 0x00;	
	
	e = ioctl(fd, B_RAW_DEVICE_COMMAND, &rdc, sizeof(rdc));
	if((e == -1) || (rdc.cam_status != CAM_REQ_CMP)) return B_ERROR;
	return rdc.scsi_status ? B_ERROR : B_OK;
}

const char *
SCSIDevice::Path(void)
{
	return path;
}


status_t
SCSIDevice::InitCheck(void)
{
	if(fd < 0) {
		return B_ERROR;
	} else {
		return B_OK;
	}
}

status_t 
SCSIDevice::Inquiry(InquiryInfo &info)
{
	status_t res;
	int i;
	uchar cmd[6] = { 0x12, 0x00, 0x00, 0x00, 36, 0x00 };
	res = CmdIn(info.data, 36, cmd, 6);
	memcpy(info.vendor, info.data + 8, 8);
	info.vendor[8] = 0;
	for(i=7;(info.vendor[i]==' ')&&(i>=0);i--) info.vendor[i]=0;
	memcpy(info.product, info.data + 16, 16);
	info.product[16] = 0;
	for(i=15;(info.product[i]==' ')&&(i>=0);i--) info.product[i]=0;
	memcpy(info.revision, info.data + 32, 4);
	info.revision[4] = 0;
	for(i=3;(info.revision[i]==' ')&&(i>=0);i--) info.revision[i]=0;
	return res;
}

status_t 
SCSIDevice::CmdIn(void *data, size_t len, uchar *cmd, size_t cmdlen)
{
	raw_device_command rdc;
	rdc.command_length = cmdlen;
	rdc.data = data;
	rdc.data_length = len;
	rdc.flags = B_RAW_DEVICE_DATA_IN | B_RAW_DEVICE_SHORT_READ_VALID;
	memcpy(rdc.command,cmd,cmdlen);
	return Execute(&rdc);
}

status_t 
SCSIDevice::CmdOut(void *data, size_t len, uchar *cmd, size_t cmdlen)
{
	raw_device_command rdc;
	rdc.command_length = cmdlen;
	rdc.data = data;
	rdc.data_length = len;
	rdc.flags = 0;
	memcpy(rdc.command,cmd,cmdlen);
	return Execute(&rdc);
}


static char *skeys[] = 
{ "NO SENSE", "RECOVERED ERROR", "NOT READY", "MEDIUM ERROR", "HARDWARE ERROR",
  "ILLEGAL REQUEST", "UNIT ATTENTION", "DATA PROTECT", "BLANK CHECK", "?",
  "COPY ABORTED", "ABORTED COMMAND", "EQUAL", "VOLUME OVERFLOW", "MISCOMPARE",
  "?"
};


void 
SCSIDevice::ErrorInfo(uchar *sense)
{
	fprintf(stderr,"SCSI ERROR: 0x%02x (%s) ASC/ASCQ 0x%02x/0x%02x\n",
			sense[2] & 0x0f, skeys[sense[2]&0x0f], sense[12], sense[13]);
	hexout(sense,32);
}

void 
SCSIDevice::DumpErrorInfo(void)
{
	fprintf(stderr,"SCSI ERROR: 0x%02x (%s) ASC/ASCQ 0x%02x/0x%02x\n",
			sense[2] & 0x0f, skeys[sense[2]&0x0f], sense[12], sense[13]);
	hexout(sense,32);
}



status_t
SCSIDevice::Execute(raw_device_command *rdc)
{
	if(fd == -1) return B_ERROR;
	
	int e;
	memset(sense,0,32);
	
	rdc->sense_data = sense;
	rdc->sense_data_length = 32;
	rdc->timeout = timeout;
	
	e = ioctl(fd, B_RAW_DEVICE_COMMAND, rdc, sizeof(raw_device_command));
	if((e == -1) || (rdc->cam_status != CAM_REQ_CMP)) {
		if(rdc->cam_status != CAM_DATA_RUN_ERR) {
//			ErrorInfo(sense);
			return B_ERROR;
		}
	}
	if(rdc->scsi_status) fprintf(stderr,"<SS:%02x>\n",rdc->scsi_status);
	
	return rdc->scsi_status ? B_ERROR : B_OK;
}


status_t
ModePage::ReadPage(SCSIDevice *dev)
{
	status_t res;
	uchar cmd[10] = { 0x5a, 0x08, 0, 0, 0, 0, 0, 0, 0, 0 };
	
	cmd[2] = number;
	cmd[8] = (length+8) & 0xff;
	cmd[7] = ((length+8) >> 8) & 0xff;
	
	res = dev->CmdIn(realdata, length + 8, cmd, 10);	
	//printf("Mode Sense 0x%02x\n",number);
	//hexout(data,length);
	
	return res;
}

status_t
ModePage::WritePage(SCSIDevice *dev)
{
	uchar cmd[10] = { 0x55, 0x10, 0, 0, 0, 0, 0, 0, 0, 0 };
	
	uint writelen = data[1] + 2;
	if(writelen > length) writelen = length;
	
	cmd[8] = (writelen+8) & 0xff;
	cmd[7] = ((writelen+8) >> 8) & 0xff;
	
	realdata[0] = 0;
	realdata[1] = 0;

	//printf("Mode Select 0x%02x\n",number);
	//hexout(data,writelen);
	
	return dev->CmdOut(realdata, writelen + 8, cmd, 10);
}



ModePage::ModePage(uint number, uint length)
{
	realdata = (uchar *) malloc(length + 8);
	data = realdata + 8;
	this->number = number & 0x3f;
	this->length = length;
}


ModePage::~ModePage()
{
	free(realdata);
}

uint 
ModePage::ReadByteAt(uint ofs)
{
	return data[ofs];
}

void 
ModePage::WriteByteAt(uint ofs, uint value)
{
	data[ofs] = value;
}

uint32 
ModePage::ReadUIntAt(uint ofs, uint len)
{
	uint n = 0;
	uint i;
	for(i=0;i<len;i++){
		n = (n << 8) | data[ofs+i];
	}
	return n;
}

void 
ModePage::WriteUIntAt(uint ofs, uint len, uint value)
{
	uint i;
	for(i=1;i<=len;i++){
		data[ofs+len-i] = value & 0xff;
		value >>= 8;
	}
}

static uchar bitmasks[8] = { 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff };
	
uchar 
ModePage::ReadBitsAt(uint ofs, uint start, uint len)
{
	len--;
	len &= 0x07;
	return (data[ofs] >> start) & bitmasks[len];
}

void 
ModePage::WriteBitsAt(uint ofs, uint start, uint len, uint value)
{
	len--;
	len &= 0x07;
	data[ofs] = (data[ofs] & (~(bitmasks[len]<<start)) |
					((value & bitmasks[len])<<start));
}

