//PrefsWindow.h

#include "Util.h"
#include "PackApplication.h"

#ifndef _PREFSWINDOW_H
#define _PREFSWINDOW_H

#include <PopUpMenu.h>
#include <Window.h>

class PackWindow;
class BCheckBox;
class BStringView;

class PrefsWindow : public BWindow
{
public:
	PrefsWindow(UserPrefs prefData);
	virtual void		MessageReceived(BMessage *msg);
//	bool dirty;
	UserPrefs	thePrefs;
};

class DocsPopup;

class MyControlView : public BView
{
public:
	MyControlView(	BRect frame,
					const char *name,
					ulong resizeMask,
					ulong flags);
	virtual void	Show();
	virtual void	Hide();
};


class PrefsView : public BView
{
public:
	PrefsView(BRect frame);
	
virtual void 		AttachedToWindow();
virtual void 		Draw(BRect update);
virtual void		MessageReceived(BMessage *msg);
////////	void				EnableSave(bool state = TRUE);
////////	bool				CheckSave(const char *txt);

private:
	BView *docPrefsView;
	BView *appPrefsView;
	BView *currentView;
	/// BButton *saveBtn;
	DocsPopup *dPopup;
};

class LabelView;

class DocPrefsView : public MyControlView
{
public:
	DocPrefsView(BRect fr);
virtual void		MessageReceived(BMessage *msg);
virtual void		AttachedToWindow();
virtual void		AllAttached();
		void		DoSave();
		
BCheckBox			*autoC;

PackWindow			*theWindow;
uint32				flags;
LabelView			*helpView;
BStringView			*disView;
};


/*******
class LabelView : public BTextView
{
public:
	LabelView(BView *sibling, const char *text, long indent = 12);
			
virtual	void			AttachedToWindow();

private:
	BView *fSib;
	long fIndent;
	const char *fText;
};
******/

class AppPrefsView : public MyControlView
{
public:
		AppPrefsView(BRect fr);
virtual void		MessageReceived(BMessage *msg);
virtual void		AttachedToWindow();
		void		DoSave();
};

class DocsPopup : public BPopUpMenu
{
public:
	DocsPopup(	const char *title,
			BView *tView,
			bool radioMode = TRUE,
			bool autoRename = TRUE,
			menu_layout layout = B_ITEMS_IN_COLUMN);

	void		AddWindow(BWindow *);
	long		RemoveWindow(BWindow *);
	void		SetWindowTitle(BWindow *,const char *);
	void		RevertItem();
	
	BView 		*tView;
	BMenuItem 	*curItem;
	BMenuItem	*prevItem;
};

/******************
class RevMenuItem : public BMenuItem
{
public:
		RevMenuItem(const char *label,
					BMessage *message,
					char shortcut = 0,
					ulong modifiers = 0)
			: BMenuItem(label,message,shortcut,modifiers) {};
					
virtual	void		SetMarked(bool state) {
						inherited::SetMarked(state);
						if (!state) {
							((DocsPopup *)Menu())->prevItem = this;
						}
					};
};
*********************/

#endif
