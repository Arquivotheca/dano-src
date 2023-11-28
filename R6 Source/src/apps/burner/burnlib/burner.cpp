
#include "SCSIDevice.h"
#include "Pages.h"
#include "CDTrack.h"
#include "CDDataSource.h"
#include "MediaFileDataSource.h"
#include "CDCueSheet.h"
#include "CDMMCDriver.h"

#include <SoundFile.h>
#include <Entry.h>

int main(int argc, char *argv[])
{
	if(argc < 2) return 0;

	SCSIDevice dev(argv[1]);
	CDMMCDriver cd_mmc_driver;
	
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
	
	CDTrack *first = NULL;
	CDTrack *t = NULL;
	
	int i;
	int e = 0;
	
	for(i = 2; i < argc; i++){		
		CDDataSource *s;
		bool isdata;
		
		if(!strncmp(argv[i],"/dev",4)){
			s = new CDDiskDeviceDataSource(argv[i]);
			if(s->InitCheck()){
				delete s;
				fprintf(stderr,"Cannot open disk device '%s'\n",argv[i]);
				e++;
				continue;
			} else {
				goto got_one;
			}
		}
		
		{
			BEntry entry(argv[i]);
			if(entry.InitCheck()){
				fprintf(stderr,"Cannot open '%s'\n",argv[i]);
				e++;
				continue;
			}
			
			s = new MediaFileDataSource(&entry);
			if(s->InitCheck()){
				delete s;
				s = new CDDataFileDataSource(&entry);
				if(s->InitCheck()){
					fprintf(stderr,"Cannot open '%s'\n",argv[i]);
					e++;
					continue;
				}
				isdata = true;
				fprintf(stderr,"%02d: data  \"%s\"\n",i-1,argv[i]);
			} else {
				isdata = false;
				fprintf(stderr,"%02d: audio \"%s\"\n",i-1,argv[i]);
			}
		}		

got_one:		
		CDTrack *t0 = new CDTrack(s);
		
		if(t) {
			t->SetNext(t0);
			t = t0;
		} else {
			t = t0;
			first = t0;
		}
	}
	
	if(e) return 0;
	if(!t) {
		fprintf(stderr,"no tracks?\n");
		return 0;
	}
	fprintf(stderr,"---\n");

	CDDriver *di = cd_mmc_driver.GetDriverInstance(&dev,NULL);
	if(!di){
		fprintf(stderr,"cannot get a driver instance\n");
		return 1;
	}
	
	if(di->Check(first)){
		fprintf(stderr,"driver doesn't like our music\n");
		return 1;
	}
	
	if(di->Start(first)){
		fprintf(stderr,"cannot start the burn\n");
		return 1;
	}
	
	while(di->Burning()){
		fprintf(stderr,"Fifo %6.2f%%   Done %6.2f%%\r",
				di->PercentFifo()*100.0,di->PercentDone()*100.0);
		snooze(1000000);
	}
	fprintf(stderr,"Fifo %6.2f%%   Done %6.2f%%\n",0.0,100.0);
	fprintf(stderr,"Status: %s\n",di->GetError()); 
		
	return 0;
}
