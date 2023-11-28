#ifndef L_DRIVE_DETECT_H
#define L_DRIVE_DETECT_H

#include <stdio.h>
#include <List.h>


typedef struct
{
	const char*name;
	const char*path;
	const char*rev;
	const char*type;
}icd_cd_rw_device;

typedef struct{
	int32				count_devices;
	icd_cd_rw_device*	devices;
}icd_cd_array;



class IDriveDetect
{
	public:
		IDriveDetect();
		
		void	BeginSearch(bool async = false);	// in all cases I launch the thread,
													// but in one case I wait till the thread
													// is complete before returning
		bool			IsReady()	const	{return(fSearchComplete);};
		icd_cd_array	Devices() 	const 	{return(fDeviceArray);};	// array of discovered devices
		
		void			SetReady(bool x) {fSearchComplete = x;};
	private:
		static int32	AsyncSearchThread(IDriveDetect*);
		
		void	CrawlPathForFile(const char* file_name, const char* path);
		bool	IsDeviceCompatible(const char*dev_path);	
		
		// data members, duh
		BString*		fDiscoveredPaths;	// BString is tiny and flexible
		bool			fSearchComplete;
		icd_cd_array	fDeviceArray;		
		int32			fNumDevices;


};



#endif

