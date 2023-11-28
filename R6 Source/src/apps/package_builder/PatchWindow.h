// PatchWindow.h


#ifndef _PATCHWINDOW_H
#define _PATCHWINDOW_H

#include <AppFileInfo.h>
#include "ChildWindow.h"
//#include "WView.h"

extern status_t GetVersionInfo(BFile *file, version_info *info);
extern void BuildVersionString(version_info *info, BString *str);

class PackWindow;
class PackList;

class PatchWindow : public ChildWindow
{
public:
	PatchWindow(const char *title,PackWindow *_pw);

	// virtual void	AllAttached();
	virtual void	MessageReceived(BMessage *);
	// virtual void	QuitRequested();

private:
	bool			busy;
	
	PackWindow		*pw;
	BView			*parentList;
	
	entry_ref		oldFileRef;
	entry_ref		newFileRef;

	void			MakeFilePanel(long msgCode, const char *prompt );
	BWindow			*filePanelW;
};


class PatchView : public BView
{
public:
	PatchView(BRect frame,const char *name,PackWindow *_pw);

	virtual void AllAttached();
	virtual void AttachedToWindow();
	virtual void Draw(BRect updt);
	virtual void MessageReceived(BMessage *);
private:
	
	PackWindow		*pw;	
	void		 AddChildLabels(BRect,const char *);
};

#endif
