#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <Directory.h>
#include <Entry.h>
#include <Path.h>
#include <Drivers.h>

#include <List.h>

#include "SCSIDevice.h"

#include "CDMMCDriver.h"

static CDMMCDriver cd_mmc_driver;

static void checkdevice(const char *path, BList *list)
{
	device_geometry geo;
	
	int fd;
	if((fd = open(path,O_RDONLY)) < 0) return;
	if(ioctl(fd,B_GET_GEOMETRY,&geo,sizeof(geo))) return;
	close(fd);
	if(geo.device_type == B_CD) {
		SCSIDevice *dev = new SCSIDevice(path);
		if(dev->InitCheck()) return;
		
		if(cd_mmc_driver.IsSupportedDevice(dev)){
			printf("Acceptable device /dev/ path:\n\t%s\n", path);
			list->AddItem(cd_mmc_driver.GetDriverInstance(dev,NULL));
		} else {
			delete dev;
		}
	}
}

static void walkpath(const char *path, BList *list)
{
	printf("Scanning path %s\n", path);
	BDirectory dir(path);
	if(dir.InitCheck() == B_OK){
		BEntry entry;
		while(dir.GetNextEntry(&entry) >= 0) 
		{
			BPath name;
			entry.GetPath(&name);
			if(entry.IsDirectory()) {walkpath(name.Path(),list);	} 
			//else if(!strcmp(name.Leaf(),"raw")){checkdevice(name.Path(),list);}
		}
	}			
}

void 
CDDriver::GetInstances(BList *list)
{
	printf("GetInstances()\n");
	walkpath("/dev/disk",list);
}


int
main()
{
//	int i;
//	CDDriver *cdd;
	BList *list = new BList();
	
	walkpath("/dev/disk",list);
//	for(i=0;(cdd = ((CDDriver *) list->ItemAt(i)));i++){
//		printf("%s: %s\n",cdd->Name(), cdd->DeviceName());
//	}
}
