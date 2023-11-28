/*
 *		iCD Drive Detection class
 *			all code by Joseph D. Groover, JR
 *				some code detection methods borrowed from
 *				various uniz command line utils
 *					... methods, not code (i.e. how to do stuff...)
 *					you know, because BeOS ain't the same as Unices..
 *					
*/



#include <stdio.h>
#include <Application.h>

#include <Directory.h>
#include <Entry.h>
#include <storage/Path.h>
#include <String.h>

#include <Drivers.h>
#include <unistd.h>

#include "DriveDetect.h"

IDriveDetect	::	IDriveDetect()
{	fSearchComplete = false;
	fNumDevices = -1;
	fDiscoveredPaths = new BString[50];
}

void
IDriveDetect	::	BeginSearch(bool a)
{
	resume_thread(	spawn_thread(AsyncSearchThread, "iCD Device Discovery", 8, this)	);

	if (!a){while (!IsReady()){snooze(1000);}}
}

void
IDriveDetect	::	CrawlPathForFile(const char*f, const char*p)
{	BDirectory dir(p);
	if (dir.InitCheck() == B_OK)
	{	BEntry entry;
		while (dir.GetNextEntry(&entry) >= 0)
		{	BPath path;entry.GetPath(&path);
			if(entry.IsDirectory())
				{
					CrawlPathForFile(f,path.Path());
				}
				else if (!strcmp(path.Leaf(), f))
				{
					if (IsDeviceCompatible(path.Path()))
						{	fNumDevices	++;			fDiscoveredPaths[fNumDevices] = path.Path();		
							printf("Found: %s\n", path.Path());	
						}	// end if compatible
				}// end else if
		}// end while
	}// end if InitCheck()
} // end CrawlPathForFile(...)

bool
IDriveDetect	::	IsDeviceCompatible(const char*p)
{
	device_geometry g;
	int f; if (	(f=open(p,0, B_READ_ONLY)) <0	){printf("\tERROR: cannot open device\n");return false;}
	if (ioctl(f, B_GET_GEOMETRY, &g, sizeof(g))){return false;}
	printf("DEVICE IS: %s\n", g.removable ? "Removable":"Not Removable");
	if (g.removable){return false;}	// if cannot be removed, it is not a CD-burner
	return(true);
}


int32
IDriveDetect	::	AsyncSearchThread(IDriveDetect*t)
{
	t->CrawlPathForFile("raw","/dev/disk");
	t->SetReady(true);
	return(0);
}

