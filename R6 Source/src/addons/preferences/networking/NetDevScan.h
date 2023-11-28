//  NetNetDevScans.cpp
//
//	russ 5/21/98 
// 
//	(c) 1997-98 Be, Inc. All rights reserved

#ifndef _NETADDONS_H
#define _NETADDONS_H

#include "Resource.h"

#include <image.h> // Kernel dll calls and structs
#include <NetDevice.h>
#include <List.h>

class NetDevScan 
{
	public:
		NetDevScan();
		~NetDevScan();
		void LoadNetDevScans();
		void GetNetDevScanNames(char** imagenames, char** interfacenames, 
									int* names_count);
		void GetNetDevScanNames(char* imagename, char* interfacename); 
		BNetConfig* GetNetConfigFromNetDevScan(const char* imagename);
				
		// Call the addon's BNetConfig::Config method. The interface name
		// also refered to as the device is the section in the settings
		// file where this device is found. Autoconfigure will not show
		// the popup dialog but will put the device entries under the 
		// interface section.
		void GetInfoFromNetDevScan(BCallbackHandler* done_msg, const char *imagename,
								net_settings* h_ncw, const char *interfacename, 
									bool autoconfigure);	
															
	private:
		int16 image_count;
		image_id addon_image[kMaxNetDevScan]; // Used to get_image_symbols()
		char* addon_name[kMaxNetDevScan]; // The file name
};

//

class TNetDevScanItem
{
public:
					TNetDevScanItem(const char* netDevScanName, const char* netDevScanPath,
						image_id image);						
					~TNetDevScanItem();
					
	 	char*		NetDevScanName();
		char*		LinkName();
		bool		Display();
		void		SetNetDevScanName(const char* name);
		void		SetLinkName(const char* name);
		void		SetDisplay(const bool display);
		
		char*		NetDevScanPath();
		void		SetNetDevScanPath(const char* path);
		
		char* 		InterfaceName();
		void		SetInterfaceName(const char* name);
		void		SetInterfaceNum(const int num);

		BNetConfig* GetNetConfig();
		void 		GetInterfaceName();
		void 		Configure(const char* name,
						BCallbackHandler* callback, net_settings* settings,
						bool autoconfigure);

private:
		char*		fNetDevScanName;
		char*		fLinkName;
		bool 		fDisplay;
		char* 		fNetDevScanPath;
		char*		fInterfaceName;
		image_id	fImage;	
};

class TNetDevScanList : public BList
{
public:
					TNetDevScanList();								
					~TNetDevScanList();
					
		void		ConfigNetDev(int32 index, const char* name,
						BCallbackHandler* callback, net_settings* settings,
						bool autoconfigure);
		void		ConfigNetDev(const char* interfaceName, const char* name,
						BCallbackHandler* callback, net_settings* settings,
						bool autoconfigure);
		
		// Example: pkgig 
		char*		NetDevScanName(int32 listIndex);
		char* 		NetDevScanName(const char* interfaceName);
         
        // Example: /dev/net/pkgig/0
		char* 		LinkName(const char* linkName);
		
		// Example: Packet Engines GNICII(1)
		char*		InterfaceName(int32 listIndex);
		char* 		InterfaceName(const char* netDevScanName);
		
		TNetDevScanItem* ItemAt(int32 listIndex);
		TNetDevScanItem* ItemAt(const char* interfaceName);
		
		void		PrintToStream();
};

#endif // _NETADDONS

