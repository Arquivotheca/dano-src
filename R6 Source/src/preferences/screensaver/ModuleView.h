#if ! defined MODULEVIEW_INCLUDED
#define MODULEVIEW_INCLUDED 1

#include <View.h>
#include <Messenger.h>
#include <Rect.h>
#include <StorageDefs.h>

class BMessage;
class BListView;
class BBox;
class BScreenSaver;
class ModuleListView;
class ModuleListItem;
class MockupView;
class BFilePanel;
class BButton;
class BPath;

class ModuleView : public BView
{
	ModuleListView		*list;
	BView				*blankpreview;
	MockupView			*mockup;
	BBox				*box;
	BMessenger			r;
	BButton				*test;
	BButton				*add;
	ModuleListItem		*currentmod;
	BMessage			metadata;
	char				modulename[B_PATH_NAME_LENGTH];
	BFilePanel			*filepanel;

public:
					ModuleView(BRect frame, const char *name, BMessenger roster);
					~ModuleView();
	void			SaveState();
	void			AttachedToWindow();
	void			DetachedFromWindow();
	void			MessageReceived(BMessage *msg);
	void			AddMod(ModuleListItem *mod);
	void			RemoveMod(BPath path);
	void			SelectMod(int32 index);
};

#endif
