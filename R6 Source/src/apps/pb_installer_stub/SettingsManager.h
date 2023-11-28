#ifndef _SETTINGSMANAGER_H_
#define _SETTINGSMANAGER_H_

#include <Locker.h>
#include <Message.h>
#include <Entry.h>

const long kViewedReport = 1L;
const long kNewReport = 0L;

// SettingsManager.h

class SettingsManager
{
public:
	SettingsManager(bool readOnly = true);
	~SettingsManager();
	
	void		SetupDefaults();
	void		SaveSettings();
	void		ReadSettings();
	
	inline bool		Lock();
	inline void		Unlock();

	BMessage		reg;
	BMessage		data;
		
	//------------- Download Prefs --------------//

//	void			SetDlPath(const char *);
//	const char		*GetDlPath() const;			// Download Path*
	
//	void			SetDlFlags(int32);
//	int32			GetDlFlags() const;
	enum {
		DL_AUTORESUME =	0x00000001,
		DL_AUTOLAUNCH = 0x00000002
	};

	//------------- Comm Prefs ------------------//
//	void		SetCheckFreq(short v);			// check when*
//	int16		GetCheckFreq() const;
	enum {
		EVERY_HOUR, EVERY_DAY, EVERY_WEEK, CHECK_NEVER
	};

//	void		SetCheckDay(int16 v);			// day 0 - 6*
//	int16		GetCheckDay() const;

//	void		SetCheckHr(int16 v);				// hr 0 - 23*
//	int16		GetCheckHr() const;
	
//	void		SetListenerAuto(bool v);
//	bool		GetListenerAuto() const;
	
//	void		SetServer(const char *name);
//	const char	*GetServer() const;
	
	//------------- Install Prefs ---------------//
//	void			SetInstallLog(bool);
//	bool			GetInstallLog() const;		// Installer creates log files
	
//	void			SetLogPath(const char *);
//	const char		*GetLogPath() const;
	
//	void			SetInstallPath(const char *);
//	const char		*GetInstallPath() const;
	
//	void			SetUseInstPath(bool);
//	bool			GetUseInstPath() const;

//	void			SetInstPreview(bool);		// Installer creates log files
//	bool			GetInstPreview() const;	
	enum {
		INST_LOG = 0x0001,
		INST_PATH = 0x0002,
		INST_PREVIEW = 0x0004
	};

	//------------- Register Prefs --------------//
//	void		SetRegisterMode(int16);	
//	int16		GetRegisterMode() const;		// when to register
	enum {
		DO_REGISTER = 0,
		NO_REGISTER
	};

//	void		SetRegInfo(const char *,const char *);
//	BMessage	*GetRegMessage() const;
		
	//------------- Update Prefs ----------------//
//	void		SetAutoInstFlags(int32);			// auto download/install mode*
//	int32		GetAutoInstFlags() const;		// for updates (see enums below)
	enum {
		UPDT_AUTODOWNLOAD	= 0x00000001,
		UPDT_AUTOINSTALL	= 0x00000002
	};

	//---------------- Uninstall Prefs -----------------//
//	void		SetUninstMode(short);
//	int16		GetUninstMode() const;
	enum {
		UNINST_TRASH = 0,
		UNINST_DELETE
	};	
//	void		SetUninstFlags(int32);
//	int32		GetUninstFlags() const;
	enum {
		UNINST_ARCHIVE = 0x00000001
	};

	//----------------- Application Prefs -------------//
//	short		curPanel;	// locking!
	enum {
		DOWNLOAD_PANEL, INSTALL_PANEL, REGISTER_PANEL,
		UPDATE_PANEL, UNINSTALL_PANEL
	};

//	BPoint		settWindowPosition;
//	BPoint		manWindowPosition;
	
	BEntry		sEntry;
	
	//----------------- Logging Prefs ----------------//
	
//	void			setLog_Flag(bool on, int32 flag);
//	bool			getLog_Flag(int32 mask);
	
//	void 			setNotification_freq(short v);
//	short			getNotification_freq();
	enum {
		EMAIL_ON, EMAIL_OFF
	};
	
	//----------------- General Prefs ----------------//
	
//	uint32		GetLastConnection() const;
//	void		SetLastConnection(uint32);
	
private: /***********************************/
//	char		*dlPath;		// locking!
//	int32		dlFlags;
	
//	int16		checkFreq;	
//	int16		checkDay;	// 0-6
//	int16		checkHr;	// 0-23
//	bool		listenerAuto;
//	char		*serverName;

//	BMessage	*regMsg;
	
//	bool		installerLog;	// locking!
//	char		*logPath;
//	bool		installPreview;
//	bool		useInstallPath;
//	char		*installPath;
	
//	int16		regMode;		// locking!
//	int32		updtInstMode;	// locking! for updates
//	int16		uinstMode;
//	int32		uinstFlags;

//	int32		Log_flags;
//	short 		notification_freq;
			
//	int32		lastScheduledConnection;
	
	BLocker		sLock;	
//	void 		SetString(char *&oldStr, const char *newStr);

	bool		fReadOnly;
};

inline bool	SettingsManager::Lock()
{
	return sLock.Lock();
}

inline void	SettingsManager::Unlock()
{
	sLock.Unlock();
}

#endif

