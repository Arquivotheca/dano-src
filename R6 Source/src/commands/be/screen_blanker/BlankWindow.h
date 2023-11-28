#if ! defined BLANKWINDOW_INCLUDED
#define BLANKWINDOW_INCLUDED

class BScreenSaver;
#include <OS.h>

#if SCREENBLANKER_DONT_USE_DIRECTWINDOW
#include <Window.h>

class BlankWindow : public BWindow
#else // SCREENBLANKER_DONT_USE_DIRECTWINDOW
#include <DirectWindow.h>

class BlankWindow : public BDirectWindow
#endif // SCREENBLANKER_DONT_USE_DIRECTWINDOW
{
	long				ModuleThread();

	static int32		thread_entry_stub(void *param);
	thread_id			thread;
	sem_id				visible;
	sem_id				drawing_lock;
	BView				*view;
	BScreenSaver		*module;
	volatile bool		quitting;
	volatile bool		direct_connected;
	BArchivable			*archive;
	char				modulename[B_FILE_NAME_LENGTH];

public:
			BlankWindow();
			~BlankWindow();
#if !SCREENBLANKER_DONT_USE_DIRECTWINDOW
	void	DirectConnected(direct_buffer_info *info);
#endif // !SCREENBLANKER_DONT_USE_DIRECTWINDOW
};

#endif
