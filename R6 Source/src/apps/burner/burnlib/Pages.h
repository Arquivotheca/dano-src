
#ifndef PAGES_H
#define PAGES_H

#include "SCSIDevice.h"

class WriteCachePage : public ModePage
{
public:
	WriteCachePage() : ModePage(0x08,12) { }
	void SetWriteCache(bool enabled) 
		{ data[2] = enabled ? (data[2] | 0x04) : (data[2] & 0xfb);}
};

class WriteParamsPage : public ModePage
{
public:
	WriteParamsPage() : ModePage(5,56) { }
	void PrepForDAO(void){
		data[2] = 0x02; // Session-At-Once, No Test
		data[3] = 0x10; // Original / No Track Mode
		data[4] = 0x00;
		data[8] = 0x00; // CDROM/CDDA
		
		memset(data+16,0,16); // MCN
		memset(data+32,0,16); // ISRC
	}
};

class TrackInfo : public ModePage
{
public:
	TrackInfo(uint number) : ModePage(number,28) {};
	virtual status_t ReadPage(SCSIDevice *dev) {
		uchar cmd[10] = { 0x52, 0x01, 0, 0, 0, 0, 0, 0, 0, 0 };
	
		cmd[5] = number;
		cmd[8] = length & 0xff;
		cmd[7] = (length >> 8) & 0xff;
	
		return dev->CmdIn(data, length, cmd, 10);	
	};
	status_t ReadPage(SCSIDevice *dev, uint n){
		number = n;
		return ReadPage(dev);
	}
	
	virtual status_t WritePage(SCSIDevice */*dev*/){
		return B_ERROR;
	};
	
	uint TrackStartAddr(void){ return ReadUIntAt(8,4); }
	uint NextWritableAddr(void){ return ReadUIntAt(12,4); }
	uint FreeBlocks(void){ return ReadUIntAt(16,4); }
	uint FixedPacketSize(void){ return ReadUIntAt(20,4); }
	uint TrackSize(void){ return ReadUIntAt(24,4); }
	uint SessionNumber(void){ return ReadByteAt(3); }
	bool Damaged(void) { return data[5]&0x20?true:false; }
	bool Copy(void) { return data[5]&0x10?true:false; }
	bool Blank(void) { return data[6]&0x40?true:false; }
	bool Packet(void) { return data[6]&0x20?true:false; }
	bool FixedPacket(void) { return data[6]&0x10?true:false; }
	bool Reserved(void) { return data[6]&0x80?true:false; }
	uint TrackMode(void) { return data[5]&0x0f; }
	uint DataMode(void) { return data[6]&0x0f; }
};

class DiscInfo : public ModePage
{
public:
	DiscInfo(void) : ModePage(0,32) {};
	virtual status_t ReadPage(SCSIDevice *dev) {
		uchar cmd[10] = { 0x51, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	
		cmd[8] = length & 0xff;
		cmd[7] = (length >> 8) & 0xff;
	
		return dev->CmdIn(data, length, cmd, 10);	
	};
	virtual status_t WritePage(SCSIDevice */*dev*/){
		return B_ERROR;
	};
	uint LastSessionStart() { return ReadUIntAt(16,4); }
	uint LastSessionStop() { return ReadUIntAt(20,4); }
	bool Blank() { return (data[2] & 0x03) == 0 ? true : false; }
};
#endif

