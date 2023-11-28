//  NetNetDevScans.cpp
//
//	russ 5/21/98 
// 
//	(c) 1997-98 Be, Inc. All rights reserved


#include <Path.h>
#include <FindDirectory.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <NetDevice.h>
#include <Entry.h>
#include <Directory.h>
#include <Alert.h>

#include "NetDevScan.h"

TNetDevScanItem::TNetDevScanItem(const char *netDevScanName, const char* netDevScanPath, image_id image)
{
	fNetDevScanName = strdup(netDevScanName);
	fNetDevScanPath = strdup(netDevScanPath);
	fLinkName = NULL;
	fDisplay = true;
	fInterfaceName = NULL;
	fImage = image;
}

TNetDevScanItem::~TNetDevScanItem()
{
	if (fNetDevScanName)
		free(fNetDevScanName);
	if (fNetDevScanPath)
		free(fNetDevScanPath);
	if (fInterfaceName)
		free(fInterfaceName);
}

char*
TNetDevScanItem::NetDevScanName()
{
	return fNetDevScanName;
}

char*
TNetDevScanItem::LinkName()
{	
	return fLinkName;
}

bool
TNetDevScanItem::Display()
{	
	return fDisplay;
}

void
TNetDevScanItem::SetNetDevScanName(const char* name)
{
	if (fNetDevScanName)
		free(fNetDevScanName);
	fNetDevScanName = strdup(name);
}

void
TNetDevScanItem::SetLinkName(const char* name)
{
	if (fLinkName)
		free(fLinkName);
	fLinkName = strdup(name);
}

void
TNetDevScanItem::SetDisplay(const bool display)
{
	fDisplay = display;
}

char*
TNetDevScanItem::NetDevScanPath()
{
	return fNetDevScanPath;
}

void
TNetDevScanItem::SetNetDevScanPath(const char* name)
{
	if (fNetDevScanPath)
		free(fNetDevScanPath);
	fNetDevScanPath = strdup(name);
}

char*
TNetDevScanItem::InterfaceName() 
{
	return fInterfaceName;
}

void
TNetDevScanItem::SetInterfaceName(const char* name) 
{
	if (fInterfaceName)
		free(fInterfaceName);
	fInterfaceName = strdup(name);
}

void
TNetDevScanItem::SetInterfaceNum(const int num) 
{
    char buf[256];
	
	if (fInterfaceName){
		sprintf(buf,"%s (%d)", fInterfaceName, num);
		free(fInterfaceName);
	}
		
	fInterfaceName = strdup(buf);
}
 
BNetConfig *
TNetDevScanItem::GetNetConfig() 
{
	BNetConfig* (*func_open)(const char *);
	char* procname = "open_config"; // Exported function.
	char* ifname = "0";  			// Currently unsed in dll.
	
	int status = get_image_symbol(fImage, procname,B_SYMBOL_TYPE_TEXT, 
		(void **)&func_open);
	if (status < B_NO_ERROR)
		return (NULL);
	else
		return ((*func_open)(ifname)); // Calls open_config.

}

void
TNetDevScanItem::GetInterfaceName() 
{
	BNetConfig* nc = GetNetConfig();
	if (!nc) {
		if (fInterfaceName)
			free(fInterfaceName);
		fInterfaceName = NULL;
	} else {
		char name[100];
	
    	nc->GetPrettyName(name, sizeof(name)); // Dll call.

		SetInterfaceName(name);
	}
}

void
TNetDevScanItem::Configure(const char* name, BCallbackHandler* callback,
	net_settings* settings, bool autoconfigure) 
{
	BNetConfig* nc = GetNetConfig();
	  
    if (nc == NULL) // If BNetConfig class not found.
	     return;
	     		
	// Popup addon configuration dialog.
	nc->Config(name, settings, callback, autoconfigure);
}

// ********************************************************************************

static const char *kDeviveDir = "/dev/net/";

TNetDevScanList::TNetDevScanList()
	: BList()
{
	BPath addonPath;
	
	BDirectory	deviceDir;
	BDirectory	interfaceDir;
	BEntry	deviceEntry;
	BEntry	interfaceEntry;
	// The published device directory
	// is also the addon name.
	char deviceName[64];
	char interfaceNumName[64];
	int interfaceNum;
	char interfacePath[256];
	
	TNetDevScanItem* item;
	image_id image;
	
	char buf[256];

	if (find_directory (B_BEOS_ADDONS_DIRECTORY, &addonPath) != B_OK)
		return;		
	addonPath.Append ("net_server");	
	
	// The addon list is formed by looking scanning the /dev/net
	// directory and looking for the published names. The addon
	// name is the directory name and each interface is enumerated
	// within it. For example, /dev/net/pkgig/0, /dev/net/pkgig/1
	// show there are two Packet Engine Gigabit cards.	The device
	// name is pkgig and the interface number name is 0 and 1.
	if(deviceDir.SetTo(kDeviveDir) == B_OK){
		while(deviceDir.GetNextEntry(&deviceEntry) == B_OK){
			deviceEntry.GetName(deviceName);
			sprintf(interfacePath, "%s%s", kDeviveDir, deviceName);
			if(interfaceDir.SetTo(interfacePath) == B_OK){
				interfaceNum = 1;
				while(interfaceDir.GetNextEntry(&interfaceEntry) == B_OK){
					interfaceEntry.GetName(interfaceNumName);				
					sprintf(buf, "%s/%s", addonPath.Path(), deviceName);
					image = load_add_on(buf); // Load up dll.
					if (image >= B_NO_ERROR) {	
						item = new TNetDevScanItem(deviceName, buf, image);
						if (item) {
							item->GetInterfaceName(); // Get the pretty name from addon.
							// Cat the (1), (2), etc to pretty name.
							item->SetInterfaceNum(interfaceNum); 
							sprintf(buf,"%s/%s", interfacePath, interfaceNumName);

							item->SetLinkName(buf); // /dev/net/pkgig/0, 1, 2 etc.
							AddItem(item);
						}
						else
							printf("Can't build add on item.\n");	
					}
					interfaceNum++;
				} 
			} 			
        } //while
	}
	
	////////////////////////////////////////////////////////
	// Add some ISA items that are not "published". This is lame
	// but jumpered cards need to be added also. It's easiest
	// to kinda of treat them like regular scanned items then
	// special case the few areas like here and GeneralTabPage
	// update.
	sprintf(buf, "%s/%s", addonPath.Path(), "ne2000");
	image = load_add_on(buf); // Load up dll.
	if (image >= B_NO_ERROR) {
		item = new TNetDevScanItem("ne2000", buf, image);
		if (item) {
			item->GetInterfaceName(); // Get the pretty name from addon.
			// Cat the (1) to pretty name.
			item->SetInterfaceNum(1); 
			item->SetLinkName("/dev/net/ether/0");
			item->SetDisplay(false);				
			AddItem(item);
		}
	}
	
	sprintf(buf, "%s/%s", addonPath.Path(), "3c503");
	image = load_add_on(buf); // Load up dll.
	if (image >= B_NO_ERROR) {
		item = new TNetDevScanItem("3c503", buf, image);
		if (item) {
			item->GetInterfaceName(); // Get the pretty name from addon.
			// Cat the (1) to pretty name.
			item->SetInterfaceNum(1); 
			item->SetLinkName("/dev/net/ether/0");
			item->SetDisplay(false);						
			AddItem(item);
		}
	}
	///////////////////////////////////////////////////////
	
		
}

TNetDevScanList::~TNetDevScanList()
{
	TNetDevScanItem* item;
	while (CountItems() > 0) {
		item = (TNetDevScanItem*)RemoveItem((int32)0);
		if (item)
			delete item;
	}
}


// Interface name is the network section name. Example: interface0.
// Pretty name is the nice form of the driver name. Example: 3com XL 905B.
void 
TNetDevScanList::ConfigNetDev(int32 index, const char* interfaceName,
	BCallbackHandler* callback, net_settings* settings, bool autoconfigure) 
{
	TNetDevScanItem* item = ItemAt(index);
	if (item)
		item->Configure(interfaceName, callback, settings, autoconfigure);
}

void 
TNetDevScanList::ConfigNetDev(const char *prettyName, const char* interfaceName,
	BCallbackHandler* callback, net_settings* settings, bool autoconfigure) 
{
	TNetDevScanItem* item = ItemAt(prettyName);
	if (item)
		item->Configure(interfaceName, callback, settings, autoconfigure);
}

char *
TNetDevScanList::NetDevScanName(int32 listIndex)
{
 	TNetDevScanItem* item = ItemAt(listIndex);
	if (item)
		return item->NetDevScanName();
	else
		return NULL;
}

char *
TNetDevScanList::NetDevScanName(const char *interfaceName)
{	
	TNetDevScanItem* item = ItemAt(interfaceName);

	if (item)
		return item->NetDevScanName();
	else
		return NULL;
}

char *
TNetDevScanList::LinkName(const char *interfaceName)
{
	TNetDevScanItem* item = ItemAt(interfaceName);

	if (item)		
		return item->LinkName();				
	else
		return NULL;
}

// The interface name is the pretty name.
char *
TNetDevScanList::InterfaceName(int32 listIndex)
{
	TNetDevScanItem* item = ItemAt(listIndex);
	if (item)
		return item->InterfaceName();
	else
		return NULL;
}

char *
TNetDevScanList::InterfaceName(const char *netDevScanName)
{
	bool found=false;
	int32 index=0;
	TNetDevScanItem* item=NULL;
	
	while (true) {
		if (index >= CountItems())
			break;
		else {
			item = ItemAt(index);
			if (item) {
				char* name = item->NetDevScanName();
				if (name && strcmp(netDevScanName, name) == 0) {
					found = true;
					break;
				}
			}
			index++;
		}
	}
	
	if (item && found)
		return item->InterfaceName();
	else
		return NULL;
}

TNetDevScanItem*
TNetDevScanList::ItemAt(int32 listIndex) 
{
	return (TNetDevScanItem*)BList::ItemAt(listIndex);
}

TNetDevScanItem*
TNetDevScanList::ItemAt(const char* interfaceName) 
{
	bool found=false;
	int32 index=0;
	TNetDevScanItem* item=NULL;
	
	while (true) {
		if (index >= CountItems())
			break;
		else {
			item = (TNetDevScanItem*)ItemAt(index);
			if (item) {
				char* name = item->InterfaceName();																	
				if (name && strcmp(interfaceName, name) == 0) {				
					found = true;
					break;
				}
			}
			index++;
		}
	}
	
	return item;
}

void
TNetDevScanList::PrintToStream()
{
	TNetDevScanItem* item;
	for (int32 i=0 ; i<CountItems() ; i++) {
		item = ItemAt(i);
		if (item) {
			printf("#%ld: add-on: %s, pretty name:%s\n", i, item->NetDevScanName(), item->InterfaceName());
		}
	}
}
