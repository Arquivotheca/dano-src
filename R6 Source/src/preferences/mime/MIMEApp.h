//
// (c) 1997, Be Inc.
//

#ifndef __MIME_APP__
#define __MIME_APP__

#ifndef _BE_H
#include <Application.h>
#include <File.h>
#include <FilePanel.h>
#endif

extern BFilePanel *FileTyperOpenPanel();

class MIMEApp : public BApplication {

public:
	MIMEApp(const char *);
	virtual ~MIMEApp();

	virtual void MessageReceived(BMessage *msg);
	virtual void RefsReceived(BMessage *inMessage);
	virtual void ArgvReceived(int32 argc, char **argv);

	void SaveInitialSettings(BRect rect, bool showAllTypes);
	void OpenWindow(BMessage *);
	void OpenWindow(entry_ref *);
	void OpenWindow(BList *refList);
	
	BFilePanel *SavePanel();
	BFilePanel *OpenPanel();

protected:
	virtual bool QuitRequested();
	void GetInitialSettings(BRect *rect, bool *showAllTypes);
	BFile *prefs;
	BFilePanel *savePanel;
	BFilePanel *openPanel;
};

#endif
