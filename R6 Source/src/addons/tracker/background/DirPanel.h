#if ! defined DIRPANEL_INCLUDED
#define DIRPANEL_INCLUDED

#include <FilePanel.h>

struct entry_ref;
class BNode;
class BMessenger;
class BMessage;
class BButton;

class DirFilter : public BRefFilter
{
public:
	bool Filter(const entry_ref *, BNode *node, struct stat *, const char *);
};

class DirPanel : public BFilePanel
{
	BButton	*selbutton;
	void 	SetDir(entry_ref *r, BMessage *msg);

public:
			DirPanel(BMessenger *target = 0, entry_ref *start_directory = 0,
				BMessage *message = 0, BRefFilter *rf = 0);
	void	SelectionChanged();
	void	SetTarget(BMessenger bellhop);
	void	SetPanelDirectory(entry_ref *dir);
};

#endif
