#if ! defined IMAGEPANEL_INCLUDED
#define IMAGEPANEL_INCLUDED

#include <FilePanel.h>

struct entry_ref;
class BNode;
class BMessenger;
class BMessage;
class BButton;
class PreviewZone;
class BTranslationRoster;
class BTranslatorRoster;

class ImageFilter : public BRefFilter
{
	BTranslatorRoster *roster;
public:
			ImageFilter();
	bool	Filter(const entry_ref *, BNode *node, struct stat *, const char *);
};

class ImagePanel : public BFilePanel
{
	PreviewZone *prev;
public:
	ImagePanel(BMessenger *target = 0, entry_ref *start_directory = 0,
		BMessage *message = 0, BRefFilter *rf = 0);
	void	SelectionChanged();
};

#endif
