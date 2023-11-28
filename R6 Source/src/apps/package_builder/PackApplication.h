#ifndef _PACKAPPLICATION_H
#define _PACKAPPLICATION_H


#define APP_SIGNATURE "application/x-scode-PBuilder"
#define FILE_SIGNATURE "application/x-scode-DPkg"
#define INSTALLER_SIGNATURE "application/x-scode-SEAInst"

typedef ushort UserPrefs;

class PackApp : public BApplication {

public:
			 	PackApp();
virtual void 	ReadyToRun();
virtual void 	ArgvReceived(int32 argc, char **argv);
virtual	void	RefsReceived(BMessage *a_message);
virtual void 	AboutRequested();
virtual bool 	QuitRequested();
virtual void 	MessageReceived(BMessage *);
// virtual void	FilePanelClosed(BMessage *);
		bool	IsOpen(BEntry *);
		
void			CleanUpTempFiles();

void		 	ReadPreferences();
void			WritePreferences();
status_t 		GetPrefsFile(BEntry *prefFile);

bool			CompatibleMode();
void			SetCompatibleMode(bool compatible);

private:
friend class PackWindow;
friend class AppPrefsView;
		
		long		untitledCount;
		BMessenger 	prefsWindowMessenger;
		BEntry		fPrefEntry;
		UserPrefs	prefData;
		
		BMessenger	openPanelMsngr;
		bool		fR4Compatible;
};

inline bool PackApp::CompatibleMode()
{
	return fR4Compatible;
}


#endif
