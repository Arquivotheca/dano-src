#include <Application.h> 
#include <InterfaceKit.h> 
#include <StorageKit.h> 

#include <stdio.h> 
#include <string.h> 
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <TrackerAddon.h> 

#define CIFS_REFRESH_IOCTL 100175

void process_refs(entry_ref dir_ref, BMessage *msg, void *) 
{ 
	BPath path; 
	BEntry entry(&dir_ref); 
	entry.GetPath(&path); 
   
	int dfd = open(path.Path(), O_RDONLY, 0);
	if (dfd < 0) {
		//printf("Tried to open %s, got %s\n",path.Path(),strerror(dfd));
		return;
	}
	
	if (ioctl(dfd, 100175, NULL) < 0) {
		//printf("ioctl failed\n");
	}

	close(dfd);
	return;
} 

main() 
{ 
	new BApplication("application/x-sample-tracker-add-on"); 
 	(new BAlert("", "Sample Tracker Add-on", "swell"))->Go(); 
	delete be_app; 
}