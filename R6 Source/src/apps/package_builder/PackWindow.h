// PackWindow.h
#include "RWindow.h"
#include "GroupList.h"
#include "Attributes.h"

#include "MyDebug.h"

#include "DestinationList.h"

class PackList;
class PackArc;

enum {
	M_PACK_SETTINGS	=	'MPaS'
};


class PackWindow : public RWindow {
public:
			PackWindow(char *name, 
						PackArc *theFile = NULL,
						bool immedCompress = TRUE,
						bool cdInstall = FALSE);
	virtual ~PackWindow();
			
	virtual	bool	QuitRequested();
	virtual void	MessageReceived(BMessage *msg);

	virtual void	MenusBeginning();
	virtual void	SetTitle(const char *title);
	
	void			CloseChild(BMessenger &);
	
	void			CheckDirty(long &dirtyCounty, long &byteCount,bool &needDelete);
	void			DoSave(bool closing,long dCount, long bCount,
						bool needDelete, int32 buildInstaller = 0);
	void			DoSaveAs(BMessage *msg);

	status_t 		MakeTempFile(BEntry *newFile,const char *name);
	void			AddItems(int32 mode);
	void			UpdateDocModeDisplay();

	void			SetCompatibleMode(bool compatible);
	uint32			CheckCompatibility(uint16 version);
	
	PackList		*listing;
	
	// ?????
	// if auto, contains the tempfile 
	// otherwise has the realfile (or if untitled it is null)
	PackArc			*arcFile;
	
	// if auto contains the realfile
	// null if untitled or non-auto
	
	// if auto, contains the temp file
	BEntry			*tempFile;
	BEntry			*realFile;
	
	// copy of file flags
	uint32			fileFlags;
	
	
	// is this an untitled file
	bool			isUntitled;
	// has this untitled file been modified at all
	// maybe we can get rid of this one??
	bool			modified;
	// are we doing auto compression?
	bool			autoCompress;
	// is any catalog attribute or other data dirty
	bool			attribDirty;
	// the temp file may be clean but the real file may need updating
	// this is marked when auto operations take place (compression/deletion)
	bool			realUpdateNeeded;
	
	// is this a cd-install?
	bool			isCDInstall;

	#if MEMDEBUG
	ulong			memoryColor;
	#endif
	
	// pending messages (don't allow quit,save,build etc...)
	long			msgCount;

	// installer settings stuff
	AttribData			attrib;
	
	// child windows (make into an array with enums)
	enum {
		kGroupsWind = 0, kDestWind, kSetWind, kPatchWind,
		kPackSetWind, kSplashWind, kMaxChildWind
	};
	
	BMessenger			childWindows[kMaxChildWind];
	
	// not used???
	long				calcThreadCount;
	
	// acquired for each group of compression
	sem_id				calcGroupSem;

private:
	void	SetUpMenus();
	void	MakeDefaultAttributes();
	
	BMessenger			buildPanelMsngr;
	BMessenger			savePanelMsngr;
	BMessenger			extractPanelMsngr;
	BMessenger			addPanelMsngr;
	
	int32				addMode;
	
	char	*seaFileName;
	char	*instFileName;
};


class PackView : public BView {

public:
				PackView(BRect frame, char *name); 
virtual	void	AttachedToWindow();

private:
	BScrollView		*scroller;
};
