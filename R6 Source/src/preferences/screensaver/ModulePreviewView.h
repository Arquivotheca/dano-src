#if ! defined MODULEPREVIEWVIEW_INCLUDED
#define MODULEPREVIEWVIEW_INCLUDED

#include <View.h>
#include <OS.h>

class BBitmap;
class ModuleListItem;

class ModulePreviewView : public BView
{
	long				ModuleThread();
	static int32		thread_entry_stub(void *param);
	BBitmap				*offscreen;
	BView				*renderer;
	ModuleListItem		*module;
	sem_id				quitsem;
	sem_id				modaccess;
	thread_id			thread;

public:
			ModulePreviewView(ModuleListItem *mod, BRect frame);
			~ModulePreviewView();
	void	AttachedToWindow();
	void	DetachedFromWindow();
	void	Draw(BRect r);
};

#endif
