//  Settings.cpp
//
//	russ 5/21/98
//  duncan 9/27/99
// 
//	(c) 1997-98 Be, Inc. All rights reserved


#include <FindDirectory.h>
#include <NodeMonitor.h>
#include <Entry.h>
#include <Application.h>
#include <Alert.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "Settings.h"

Settings::Settings()
{
	m_hncw = net_settings_open(NULL);
	InitData();
}

// WARNING: This can be called from different threads!
void Settings::ReloadData()
{
	StopWatchSettingsFileNode();

	m_hncw = net_settings_open(NULL);
	InitData();

	// scan subscribers
	int32 subcount = subscribers.CountItems();
	for(int32 i = 0; i < subcount; i++)
	{
		SettingsSubscriber *sub = (SettingsSubscriber *)subscribers.ItemAt(i);
		if(sub->LockSubscriber())
		{
			sub->LoadSettings();
			sub->UnlockSubscriber();
		}
	}

	WatchSettingsFileNode();         
}

void Settings::InitData()
{
	char interfaces[NC_MAXVALLEN];
	char protocols[NC_MAXVALLEN];
	char outstring[NC_MAXVALLEN];
	char linkname[NC_MAXVALLEN];
	char preferred[NC_MAXVALLEN];
	int retval;
	char buf[NC_MAXVALLEN];
	InterfaceSettings	*intf;

   	if (m_hncw == NULL)
   		return;

	strcpy(m_UserName, "");
	strcpy(m_Password, "");
	strcpy(m_DNSEnabled, "");

	*preferred = 0;
	// Fill members from network settings file	(libnet dll)		
	find_net_setting(m_hncw, "GLOBAL", "USERNAME", m_UserName, NC_MAXVALLEN);
	find_net_setting(m_hncw, "GLOBAL", "PASSWORD", m_Password, NC_MAXVALLEN);
	find_net_setting(m_hncw, "GLOBAL", "PREFERRED", preferred, NC_MAXVALLEN);
	
	// Loop through interfaces and fill the interface item list.
	// From file examp: PROTOCOLS = appletalk dhcp
	strcpy(m_AppleTalkEnabled, "0");
	find_net_setting(m_hncw, "GLOBAL", "PROTOCOLS", protocols, NC_MAXVALLEN);
	do
	{ 
		retval = GetNextFieldStr(protocols, outstring);	
		if (strcmp(outstring, "appletalk") == 0)
			strcpy(m_AppleTalkEnabled, "1");					
	} 
	while (retval == 1);
					
	find_net_setting(m_hncw, "GLOBAL", "FTP_ENABLED", m_FTPEnabled, NC_MAXVALLEN);
	find_net_setting(m_hncw, "GLOBAL", "IP_FORWARD", m_IPForward, NC_MAXVALLEN);
	find_net_setting(m_hncw, "GLOBAL", "DNS_ENABLED", m_DNSEnabled, NC_MAXVALLEN);
	find_net_setting(m_hncw, "GLOBAL", "TELNETD_ENABLED", m_TelnetdEnabled, NC_MAXVALLEN);
	
	// Mostly 1st time in		
	if (strcmp(m_DNSEnabled, "") == 0)
		strcpy(m_DNSEnabled, "1");

	// Loop through interfaces and fill the interface item list
	// From file examp: INTERFACES = interface1 interface0 interface2
	strcpy(interfaces, "");
	find_net_setting(m_hncw, "GLOBAL", "INTERFACES", interfaces, NC_MAXVALLEN);
	if (strcmp(interfaces, "") == 0) 
		return; // No interfaces

	m_Preferred = 0;

	do
	{ 
		retval = GetNextFieldStr(interfaces, outstring); 

		find_net_setting(m_hncw, outstring, "DEVICELINK", linkname, NC_MAXVALLEN);						

		intf = 0;
		int cnt = interfacelist.CountItems();
		for(int i = 0; i < cnt; i++)
		{
			InterfaceSettings *scan = (InterfaceSettings *)interfacelist.ItemAt(i);
			if(strcmp(scan->m_LinkName, linkname) == 0)
			{
				intf = scan;
				break;
			}
		}

		if(! intf)
		{
			intf = new InterfaceSettings();
			intf->m_Added = false;
			strcpy(intf->m_LinkName, linkname);
			interfacelist.AddItem(intf);
		}
		intf->m_Configured = true;
		strcpy(intf->m_InterfaceName, outstring);

		if(! m_Preferred || strcmp(preferred, outstring) == 0)
			m_Preferred = intf;

		find_net_setting(m_hncw, outstring, "DEVICECONFIG", intf->m_AddOnName, NC_MAXVALLEN);

		// If using the old style modem interface where interfaceX: could
		// be a modem don't load it. Currently we only load modem0: This stops
		// the mixing of Dialup and Lan interfaces
		if (strcmp(intf->m_AddOnName, "ppp") == 0)
		{
			delete intf;
			continue;
		}

		find_net_setting(m_hncw, outstring, "IPADDRESS", intf->m_IPAddress, NC_MAXVALLEN);
		find_net_setting(m_hncw, outstring, "NETMASK", intf->m_SubnetMask, NC_MAXVALLEN);
		find_net_setting(m_hncw, outstring, "PRETTYNAME", intf->m_PrettyName, NC_MAXVALLEN);	
		find_net_setting(m_hncw, outstring, "ENABLED", buf, NC_MAXVALLEN);															
		intf->m_Enabled = atol(buf);								
		find_net_setting(m_hncw, outstring, "DHCP", intf->m_UseDHCP, NC_MAXVALLEN);						
		find_net_setting(m_hncw, outstring, "HOSTNAME", intf->m_HostName, NC_MAXVALLEN);
		find_net_setting(m_hncw, outstring, "DNS_DOMAIN", intf->m_DNSDomain, NC_MAXVALLEN);
		find_net_setting(m_hncw, outstring, "DNS_PRIMARY", intf->m_DNSPrimary, NC_MAXVALLEN);
		find_net_setting(m_hncw, outstring, "DNS_SECONDARY", intf->m_DNSSecondary, NC_MAXVALLEN);
		find_net_setting(m_hncw, outstring, "ROUTER", intf->m_Router, NC_MAXVALLEN);
	}    
	while(retval == 1);

	SetDirty(0, false);
}

int Settings::Save()
{
	char	interfaces[NC_MAXVALLEN];
   	char	protocols[NC_MAXVALLEN];
	char	outprotocols[NC_MAXVALLEN];
	InterfaceSettings *intf;
	char	outstring[NC_MAXVALLEN];
   	int		retval;
	char	*ptr;
	char	buf[128];

	// scan subscribers
	int32 subcount = subscribers.CountItems();
	for(int32 i = 0; i < subcount; i++)
	{
		SettingsSubscriber *sub = (SettingsSubscriber *)subscribers.ItemAt(i);
		if(sub->LockSubscriber())
		{
			sub->UnloadSettings();
			sub->UnlockSubscriber();
		}
	}

	if(m_hncw)
	{
		gid_t	oldgid = getgid();
		gid_t	olduid = getuid();

		setgid(0);
		setuid(0);

	   	// Create heading if one does not exist
	   	set_net_setting(m_hncw, "GLOBAL", NULL, NULL);		
		set_net_setting(m_hncw, "GLOBAL", "USERNAME", m_UserName);	

		if(m_Preferred)
			set_net_setting(m_hncw, "GLOBAL", "PREFERRED", m_Preferred->m_InterfaceName);	

		// Build up the protocol string.
		// Examp: PROTOCOLS = appletalk
		// Don't step on other protocols.
		strcpy(protocols, "");
		strcpy(outprotocols, "");
		find_net_setting(m_hncw, "GLOBAL", "PROTOCOLS", outprotocols, NC_MAXVALLEN);
		if (strcmp(outprotocols, "") != 0)
		{
			do
			{
				retval = GetNextFieldStr(outprotocols, outstring);
				if (strcmp(outstring, "appletalk") != 0)
				{
					strcat(protocols, outstring);
					strcat(protocols, " ");
				}
			}
			while (retval == 1);
		}

		if (strcmp(m_AppleTalkEnabled, "1") == 0)
			strcat(protocols, "appletalk ");

		if (strcmp(protocols, "") != 0)
		{
			ptr = protocols;
			ptr[strlen(protocols) - 1] = '\0';
		}

		set_net_setting(m_hncw, "GLOBAL", "PROTOCOLS", protocols);
		set_net_setting(m_hncw, "GLOBAL", "PASSWORD", m_Password);	
		set_net_setting(m_hncw, "GLOBAL", "FTP_ENABLED", m_FTPEnabled);
		set_net_setting(m_hncw, "GLOBAL", "IP_FORWARD", m_IPForward);
		set_net_setting(m_hncw, "GLOBAL", "DNS_ENABLED", m_DNSEnabled);
		set_net_setting(m_hncw, "GLOBAL", "TELNETD_ENABLED", m_TelnetdEnabled);
		set_net_setting(m_hncw, "GLOBAL", "VERSION", "V1.00");

		// Loop through interfaces
		strcpy(interfaces, "");
		int cnt = interfacelist.CountItems();
		for(int i = 0; i < cnt; i++)
		{
			intf = (InterfaceSettings *)interfacelist.ItemAt(i);
			set_net_setting(m_hncw, intf->m_InterfaceName, NULL, NULL);	
			strcat(interfaces, intf->m_InterfaceName);
			strcat(interfaces, " ");
			set_net_setting(m_hncw, intf->m_InterfaceName, "DEVICECONFIG",  intf->m_AddOnName);
			set_net_setting(m_hncw, intf->m_InterfaceName, "DEVICELINK", intf->m_LinkName);
			set_net_setting(m_hncw, intf->m_InterfaceName, "DEVICETYPE", "ETHERNET");		
			set_net_setting(m_hncw, intf->m_InterfaceName, "IPADDRESS", intf->m_IPAddress); 
			set_net_setting(m_hncw, intf->m_InterfaceName, "NETMASK", intf->m_SubnetMask);
			set_net_setting(m_hncw, intf->m_InterfaceName, "PRETTYNAME", intf->m_PrettyName);
			sprintf(buf, "%i", intf->m_Enabled);				
			set_net_setting(m_hncw, intf->m_InterfaceName, "ENABLED", buf);								
			set_net_setting(m_hncw, intf->m_InterfaceName, "DHCP", intf->m_UseDHCP);		
			set_net_setting(m_hncw, intf->m_InterfaceName, "HOSTNAME", intf->m_HostName);
			set_net_setting(m_hncw, intf->m_InterfaceName, "DNS_DOMAIN", intf->m_DNSDomain);
			set_net_setting(m_hncw, intf->m_InterfaceName, "DNS_PRIMARY", intf->m_DNSPrimary);
			set_net_setting(m_hncw, intf->m_InterfaceName, "DNS_SECONDARY", intf->m_DNSSecondary);
			set_net_setting(m_hncw, intf->m_InterfaceName, "ROUTER", intf->m_Router);
		}
		ptr = interfaces;
		ptr[strlen(interfaces) - 1] = '\0';
		set_net_setting(m_hncw, "GLOBAL", "INTERFACES", interfaces);

		net_settings_save(m_hncw);
		SetDirty(0, false);

		setgid(oldgid);
		setuid(olduid);
	}

	return 1;   
}

int Settings::GetNextFieldStr(char* instring, char* outstring)
{                                  
  char* ptr;
  char* ptr1;
  char* ptr2;    
   
   ptr  = instring;
   
   if((ptr1 = strchr(ptr, ' ')) == NULL){
       strcpy(outstring, instring);
       return (0);
   }
     
   *ptr1 = '\0';                         
   ptr2 = ++ptr1;
       
   for(int n = NC_MAXVALLEN; n >= 0; n--){  //Trim trailing blanks
          ptr1--;
          if (*ptr1 == ' ') 
             *ptr1 = '\0';
          else
             break;     
    }       
             
   strcpy(outstring, ptr);  	
   strcpy(instring, ptr2);	      	      
       
   return (1);                            
}

bool Settings::IsDirty()
{
	return m_Dirty;
}

//	this is an explicit set of m_Dirty, regardless of the internal state
//	of the data, its intended to be this way, do not change it!
void Settings::SetDirty(SettingsSubscriber *who, bool b)
{
	if(b && who->LockSubscriber())
	{
		who->UnloadSettings();
		who->UnlockSubscriber();
	}

	// scan subscribers
	int32 subcount = subscribers.CountItems();
	for(int32 i = 0; i < subcount; i++)
	{
		SettingsSubscriber *sub = (SettingsSubscriber *)subscribers.ItemAt(i);
		if(sub->LockSubscriber())
		{
			if(b && sub != who)
				sub->LoadSettings();

			sub->CanRevert(b);
			sub->CanSave(b);
			sub->UnlockSubscriber();
		}
	}

	m_Dirty = b;
}

void Settings::Subscribe(SettingsSubscriber *sub)
{
	subscribers.AddItem(sub);
	// send it settings
	if(sub->LockSubscriber())
	{
		sub->LoadSettings();
		sub->CanRevert(m_Dirty);
		sub->CanSave(m_Dirty);
		sub->UnlockSubscriber();
	}
}

bool Settings::Unsubscribe(SettingsSubscriber *sub)
{
	// grab the settings before it quits
	if(sub->LockSubscriber())
	{
		sub->UnloadSettings();
		sub->UnlockSubscriber();
	}
	return subscribers.RemoveItem(sub);
}

// to be called with id 0.0
int Settings::DuplicateSettingsFile(const char* FilePathAndName)
{
	FILE *fsrc;
	char fsrcpath[kMaxStrLen];
	FILE *fdst;
	char buf[80 * 2];
	
	find_directory(B_COMMON_SETTINGS_DIRECTORY, -1, false, fsrcpath, PATH_MAX);
	strcat(fsrcpath, "/");
	strcat(fsrcpath, "network");
	fsrc = fopen(fsrcpath, "r");
	fdst = fopen(FilePathAndName, "w+");
	
	if (fdst == NULL){
		fclose(fdst);
		fclose(fsrc);
		return 0;
	}		
		
	if (fsrc != NULL) {
		while (fgets(buf, sizeof(buf), fsrc)) {
			fputs(buf, fdst);
     	}
	}	
	
	fclose(fdst);
	fclose(fsrc);
		
	return 1;
}

// to be called with id 0.0
int Settings::DuplicateRetoreFile(const char* FilePathAndName)
{
	FILE *fsrc;
	char fdstpath[kMaxStrLen];
	FILE *fdst;
	char buf[80 * 2];
	
	find_directory(B_COMMON_SETTINGS_DIRECTORY, -1, false, fdstpath, PATH_MAX);
	strcat(fdstpath, "/");
	strcat(fdstpath, "network");
	fdst = fopen(fdstpath, "w+");
	fsrc = fopen(FilePathAndName, "r");
	
	if (fdst == NULL){
		fclose(fdst);
		fclose(fsrc);
		return 0;
	}		
		
	if (fsrc != NULL) {
		while (fgets(buf, sizeof(buf), fsrc)) {
			fputs(buf, fdst);
     	}
	}	
	
	fclose(fdst);
	fclose(fsrc);
		
	return 1;
}

void Settings::SetWatchLooper(BLooper *looper)
{
	watchlooper = looper;
}

status_t Settings::WatchSettingsFileNode()
{
	node_ref nref;	
	char fname[PATH_MAX];
		
	find_directory(B_COMMON_SETTINGS_DIRECTORY, -1, false, fname, PATH_MAX);
	strcat(fname, "/");
	strcat(fname, "network");
	
	BEntry entry(fname);
	entry.GetNodeRef(&nref);
	
	return (watch_node(&nref, B_WATCH_STAT, BMessenger(watchlooper)));
}

status_t Settings::StopWatchSettingsFileNode()
{
	node_ref nref;	
	char fname[PATH_MAX];
		
	find_directory(B_COMMON_SETTINGS_DIRECTORY, -1, false, fname, PATH_MAX);
	strcat(fname, "/");
	strcat(fname, "network");
	
	BEntry entry(fname);
	entry.GetNodeRef(&nref);
	
	return (watch_node(&nref, B_STOP_WATCHING, BMessenger(watchlooper)));

}
