
#include "SCSIDevice.h"
#include "Pages.h"

#include <SoundFile.h>
#include <Entry.h>

void hexout(void *bytes, uint len);

status_t CloseSession(SCSIDevice &dev)
{
	uchar cmd[10] = { 0x5b, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	return dev.CmdOut(NULL, 0, cmd, 10);		
}

status_t SyncCache(SCSIDevice &dev)
{
	uchar cmd[10] = { 0x35, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	return dev.CmdOut(NULL, 0, cmd, 10);	
}

status_t SendCueSheet(SCSIDevice &dev, void *data, size_t size)
{
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
	
	return dev.CmdOut(data, size, cmd, 10);
}

uchar blank[2048*25];
uchar fill[2048*25];

status_t WriteBlocks(SCSIDevice &dev, int lba, size_t count, 
					 void *data, size_t length)
{
	uint32 LBA = ((uint32) lba);
	uchar cmd[10];
	
	cmd[0] = 0x2a;
	cmd[1] = 0x00;
	cmd[2] = ((LBA >> 24) & 0xff);	
	cmd[3] = ((LBA >> 16) & 0xff);	
	cmd[4] = ((LBA >> 8 ) & 0xff);	
	cmd[5] = (LBA & 0xff);
	cmd[6] = 0x00;	
	cmd[7] = (count >> 8) & 0xff;
	cmd[8] = count & 0xff;
	cmd[9] = 0x00;		
	
	return dev.CmdOut(data, length, cmd, 10);
}



int main(int argc, char *argv[])
{
	if(argc != 2) return 0;

	SCSIDevice dev(argv[1]);

	InquiryInfo info;
	if(dev.Inquiry(info)){
		fprintf(stderr,"bad device\n");
		return 1;
	}

	printf("%d \"%s\" \"%s\" \"%s\"\n",
		   info.DeviceType(),info.Vendor(),info.Product(),info.Revision());
	
	for(int c=0;dev.TestReady();c++){
		fprintf(stderr,"not ready...\n");
		if(c==5) {
			return 1;
		}
		snooze(1000000);
	}
	
	fprintf(stderr,"READY\n");	
	
	WriteCachePage wcp;
	if(wcp.ReadPage(dev)){
		fprintf(stderr,"cannot read write cache page\n");
	} else {
		wcp.SetWriteCache(true);
		if(wcp.WritePage(dev)){
			fprintf(stderr,"cannot modify write cache page\n");
		}
	}
	
	WriteParamsPage wpp;
	if(wpp.ReadPage(dev)){
		fprintf(stderr,"cannot read write params\n");
	} else {
		wpp.PrepForDAO();
		if(wpp.WritePage(dev)){
			fprintf(stderr,"cannot modify write params page\n");
			return 0;
		}
	}

#if 0		
	int i;
	TrackInfo ti(0);
	printf("--  Sn Start    NextAddr FreeBlks PktSize  TrkSize  DM CP BL PK FP RS TM DM\n");    
	for(i=1;ti.ReadPage(dev,i)==B_OK;i++){
		printf("%02x: %02x %08x %08x %08x %08x %08x %s  %s  %s  %s  %s  %s  %02x %02x\n",
			   i,ti.SessionNumber(),
			   ti.TrackStartAddr(),ti.NextWritableAddr(),ti.FreeBlocks(),
			   ti.FixedPacketSize(),ti.TrackSize(), 
			   ti.Damaged()?"x":"-", ti.Copy()?"x":"-", ti.Blank()?"x":"-",
			   ti.Packet()?"x":"-", ti.FixedPacket()?"x":"-", 
			   ti.Reserved()?"x":"-", ti.TrackMode(), ti.DataMode());
	}
#endif
#if 0
	uchar cuesheet[32] = {
		0x41, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, // Lead In
		0x41, 0x01, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00,
		0x41, 0x01, 0x01, 0x10, 0x00, 0x00, 0x02, 0x00,
		0x41, 0xaa, 0x01, 0x14, 0x00, 0x00,   52, 0x00
	};
#endif
#if 0
	uchar cuesheet[] = {
		0x41, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, // Lead In	
		0x41, 0x01, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00,
		0x41, 0x01, 0x01, 0x10, 0x00, 0x00, 0x02, 0x00,
		0x41, 0x02, 0x00, 0x10, 0x00, 0x01, 0x00, 0x00,
		0x41, 0x02, 0x01, 0x10, 0x00, 0x01, 0x02, 0x00,
		0x41, 0xaa, 0x01, 0x14, 0x00, 0x03, 0x00, 0x00
	};
#endif	
	uchar cuesheet[32] = {
		0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, // Lead In
		0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, // Virtual Void
		0x01, 0xaa, 0x01, 0x01, 0x00, 3,    6,    60
	};
	if(SendCueSheet(dev, cuesheet, 8*4)) {
		fprintf(stderr,"cannot send cuesheet\n");
		dev.DumpErrorInfo();
		
		return 0;
	}
	
	TrackInfo ti(0);
	printf("--  Sn Start    NextAddr FreeBlks PktSize  TrkSize  DM CP BL PK FP RS TM DM\n");    
	ti.ReadPage(dev,1);
		printf("%02x: %02x %08x %08x %08x %08x %08x %s  %s  %s  %s  %s  %s  %02x %02x\n",
			   1,ti.SessionNumber(),
			   ti.TrackStartAddr(),ti.NextWritableAddr(),ti.FreeBlocks(),
			   ti.FixedPacketSize(),ti.TrackSize(), 
			   ti.Damaged()?"x":"-", ti.Copy()?"x":"-", ti.Blank()?"x":"-",
			   ti.Packet()?"x":"-", ti.FixedPacket()?"x":"-", 
			   ti.Reserved()?"x":"-", ti.TrackMode(), ti.DataMode());
	
	int i;
	uint fsz = 2352;
	
	BEntry entry("/source/rel/exp/buildcd/extras/all/optional/sound/virtual (void)");
	if(entry.InitCheck()){
		return 0;
	}
	entry_ref ref;
	entry.GetRef(&ref);
	BSoundFile sf(&ref,B_READ_ONLY);
	if(sf.InitCheck()){
		return 0;
	}
	
	memset(blank, 0, fsz*15);
	memset(fill, 0x42, fsz*15);
#if 1
	for(i=-150;i<0;i+=15) {
retry1:
		if(WriteBlocks(dev, i, 15, blank, fsz*15)) {
			if(dev.IsError(2,4,8)) {
				fprintf(stderr,"@");
				snooze(100000);
				goto retry1;
			}
			fprintf(stderr,"X");
			break;
		} else {
			fprintf(stderr,".");
		}
	}
	
#endif	 
	for(i=0;i<13860;i+=15){
retry2:
		sf.ReadFrames((char*)fill,(fsz/4)*15);
		for(int j=0;j<fsz*15;j+=2){
			uchar t = fill[j];
			fill[j] = fill[j+1];
			fill[j+1] = t;
		}
		
#if 1
		if(WriteBlocks(dev, i, 15, fill, fsz*15)) {
			if(dev.IsError(2,4,8)) {
				fprintf(stderr,"@");
				snooze(100000);
				goto retry2;
			}
			fprintf(stderr,"X");
			break;
		} else {
			fprintf(stderr,"d");
		}
#endif
	}
#if 0
	for(i=0;i<50*75;i+=15){
retry2:
		for(int j=0;j<15;j++){
			fill[j*fsz+0] = (i+j) >> 8;
			fill[j*fsz+1] = (i+j);
		}
		if(WriteBlocks(dev, i, 15, fill, fsz*15)) {
			if(dev.IsError(2,4,8)) {
				fprintf(stderr,"@");
				snooze(100000);
				goto retry2;
			}
			fprintf(stderr,"X");
			break;
		} else {
			fprintf(stderr,"d");
		}
	}
#endif	
//	if(CloseSession(dev)) {
//		fprintf(stderr,"close session failed\n");
//	}
	
	if(SyncCache(dev)) {
		fprintf(stderr,"sync cache failed\n");
	}
		
	return 0;
}