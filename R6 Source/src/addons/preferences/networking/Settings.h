//  Settings.h
//
//	russ 5/21/98
//  duncan 9/27/99
// 
//	(c) 1997-98 Be, Inc. All rights reserved

#if ! defined SETTINGS_INCLUDED
#define SETTINGS_INCLUDED

#include "Resource.h"

#include <netconfig.h>
#include <List.h>

class SettingsSubscriber
{
public:
	virtual bool LockSubscriber() = 0;
	virtual void UnlockSubscriber() = 0;
	virtual void CanRevert(bool flag) = 0;
	virtual void CanSave(bool flag) = 0;
	virtual void LoadSettings() = 0;
	virtual void UnloadSettings() = 0;
};

struct InterfaceSettings
{
	// The section name as it appears
	// in the network settings file.
	// Examp: interface0
	char	m_InterfaceName[kMaxStrLen];     

	// The user space addon name.
	// Examp: pkgig
	char	m_AddOnName[kMaxStrLen];     

	// Examp: Packet Engines GNICII.
	char	m_PrettyName[kMaxStrLen];	 

	// Examp: /dev/net/pkgig/0, 1, 2
	char	m_LinkName[kMaxStrLen];

	char	m_IPAddress[kMaxStrLen];
	char	m_SubnetMask[kMaxStrLen];
	char	m_UseDHCP[kMaxStrLen];
	bool	m_Enabled;	 

	char	m_HostName[NC_MAXVALLEN];
	char	m_DNSDomain[NC_MAXVALLEN];
	char	m_DNSPrimary[NC_MAXVALLEN];
	char	m_DNSSecondary[NC_MAXVALLEN];
	char	m_Router[NC_MAXVALLEN];

	// This interface has a config panel
	bool m_Configured;

	// This interface has a config panel
	bool m_Jumpered;

	// startup: this interface has been added
	bool m_Added;
};

class Settings
{
public:
				Settings();		

	void		InitData();
	void		ReloadData();

	// Enable or disable save and revert functions		
	bool		IsDirty();
	void		SetDirty(SettingsSubscriber *who, bool b);

	int			Save();
	
	void		SetWatchLooper(BLooper *looper);
	status_t	WatchSettingsFileNode();
	status_t	StopWatchSettingsFileNode();		

	void		Subscribe(SettingsSubscriber *s);
	bool		Unsubscribe(SettingsSubscriber *s);

	// Data from network settings file

	// ftp/telnet settings
	char		m_FTPEnabled[NC_MAXVALLEN];
	char		m_TelnetdEnabled[NC_MAXVALLEN];
	char		m_UserName[NC_MAXVALLEN];
	char		m_Password[NC_MAXVALLEN];

	// services
	char		m_AppleTalkEnabled[NC_MAXVALLEN];	
	char		m_IPForward[NC_MAXVALLEN];

	// misc
	char		m_DNSEnabled[NC_MAXVALLEN];

	// interfaces
	BList		interfacelist;		// BList of InterfaceSettings
	InterfaceSettings *m_Preferred;

	// Handle to the network settings file operations.
	// The handle can be passed to different processes
	// and all writes are cashed until a  net_settings_save(ncw)
	// is done. The access routines are in libnet dll and in
	// netconfig.h, and net_settings.h
	net_settings	*m_hncw; 

private:
	bool		m_Dirty;		// settings dirty
	BLooper		*watchlooper;	// the looper that will notify about settings file changes

	BList		subscribers;	// list of SettingsSubscriber

	int			GetNextFieldStr(char* instring, char* outstring);
	int			DuplicateSettingsFile(const char* FilePathAndName);
	int			DuplicateRetoreFile(const char* FilePathAndName);			
};
		
#endif // SETTINGS_INCLUDED
