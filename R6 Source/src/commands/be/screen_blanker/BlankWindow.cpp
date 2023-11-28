#include "BlankWindow.h"
#include "runner_global.h"
#include "Blackness.h"
#include "ssdefs.h"
#include "OldScreenSaver.h"
#include <stdlib.h>
#include <Application.h>
#include <View.h>
#include <image.h>
#include <Path.h>
#include <FindDirectory.h>
#include <Screen.h>

#include <stdio.h>
#include <string.h>
#include <signal.h>

void suicide(int);

class BlankView : public BView
{
	sem_id	visible;

public:
	BlankView(BRect frame, sem_id sem)
	 : BView(frame, "Screen Saver View", B_FOLLOW_ALL, B_WILL_DRAW), visible(sem)
	{
	}

	void AttachedToWindow()
	{
		SetHighColor(255, 255, 255);
		SetViewColor(B_TRANSPARENT_32_BIT);
		SetLowColor(0, 0, 0);
	}

	void Draw(BRect)
	{
		release_sem(visible);
	}
};

BlankWindow::BlankWindow()
#if SCREENBLANKER_DONT_USE_DIRECTWINDOW
 : BWindow(
#else
 : BDirectWindow(
#endif // SCREENBLANKER_DONT_USE_DIRECTWINDOW
 	BScreen().Frame(), "screen saver window",
	B_BORDERED_WINDOW_LOOK, B_MODAL_ALL_WINDOW_FEEL,
	B_NOT_MOVABLE | B_NOT_H_RESIZABLE | B_NOT_RESIZABLE |
	B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE |
	B_WILL_ACCEPT_FIRST_CLICK,
	B_ALL_WORKSPACES)
{
	module = 0;

	visible = create_sem(0, "WindowSem");

	quitting = false;
	direct_connected = false;
	drawing_lock = create_sem(1, "draw locker");

	view = new BlankView(Bounds(), visible);
	AddChild(view);
	view->MakeFocus();

#if !SCREENBLANKER_DONT_USE_DIRECTWINDOW
	SetFullScreen(true);
#endif // !SCREENBLANKER_DONT_USE_DIRECTWINDOW

	BMessage		module_settings;
	*modulename = 0;
	if(module_path && *module_path)
	{
		BPath			p(module_path);
		strcpy(modulename, kModuleSettingsPrefix);
		strcat(modulename, p.Leaf());
		global_settings.FindMessage(modulename, &module_settings);
	}

	image_id	moduleid = 0;

	if(module_path && (moduleid = load_add_on(module_path)) >= 0)
	{
		typedef BScreenSaver *(*instantiate_func)(BMessage *msg, image_id id);

		instantiate_func	modinst;

		if(get_image_symbol(moduleid, "instantiate_screen_saver", B_SYMBOL_TYPE_TEXT, (void **)&modinst) != B_OK ||
			modinst == 0 ||
			(module = modinst(&module_settings, moduleid)) == 0 ||
			module->InitCheck() != B_OK)
		{
			delete module;
			module = new Blackness(&module_settings, 0);
		}
	}
	else
		module = new Blackness(&module_settings, 0);

	// crash protection!
	disable_debugger(1);
	signal(SIGILL, suicide);
	signal(SIGABRT, suicide);
	signal(SIGPIPE, suicide);
	signal(SIGFPE, suicide);
	signal(SIGSEGV, suicide);
	signal(SIGHUP, suicide);
	signal(SIGINT, suicide);
	signal(SIGQUIT, suicide);
	signal(SIGTERM, suicide);

	if(module)
	{
		if(module->StartSaver(view, false) == B_OK)
		{
			thread = spawn_thread(thread_entry_stub, "animation", B_LOW_PRIORITY, this);
			resume_thread(thread);
		}
	}
}

void suicide(int)
{
	be_app->ShowCursor();
	thread_info	info;
	if(get_thread_info(find_thread(0), &info) == B_OK)
		kill_team(info.team);
}

BlankWindow::~BlankWindow()
{
	quitting = true;		// Connection is dying
	Hide();
	Sync();

	// acquire to ensure the drawing thread is out of the DirectTick
	acquire_sem(drawing_lock);

	// delete so the drawing thread doesn't enter
	delete_sem(drawing_lock);

	// don't need this no synchronization issues
	delete_sem(visible);

	if(module)
	{
		module->StopSaver();

		// We are going to fill this with the settings
		BMessage module_settings;
		if(*modulename && module->SaveState(&module_settings) == B_OK)
		{
			global_settings.RemoveName(modulename);
			global_settings.AddMessage(modulename, &module_settings);
		}
	}
}

#if !SCREENBLANKER_DONT_USE_DIRECTWINDOW
void BlankWindow::DirectConnected(direct_buffer_info *info)
{
	acquire_sem(drawing_lock);
	switch(info->buffer_state & B_DIRECT_MODE_MASK)
	{
		case B_DIRECT_START :
			direct_connected = true;
			/* FALL-THROUGH */

		case B_DIRECT_MODIFY: 
			if(module)
				module->DirectConnected(info);
			break;

		case B_DIRECT_STOP :
			if(module)
				module->DirectConnected(info);
			direct_connected = false;
			break;
	}
	release_sem(drawing_lock);
}
#endif // !SCREENBLANKER_DONT_USE_DIRECTWINDOW

int32 BlankWindow::thread_entry_stub(void *param)
{
	disable_debugger(1);
	signal(SIGILL, suicide);
	signal(SIGABRT, suicide);
	signal(SIGPIPE, suicide);
	signal(SIGFPE, suicide);
	signal(SIGSEGV, suicide);
	signal(SIGHUP, suicide);
	signal(SIGINT, suicide);
	signal(SIGQUIT, suicide);
	signal(SIGTERM, suicide);

	if(((BlankWindow *)param)->Lock())
		return ((BlankWindow *)param)->ModuleThread();
	else
		return 0;
}

int32 BlankWindow::ModuleThread()
{
	// copy the view pointer on the stack
	BView	*v = view;
	int32	frame = 0;

	Unlock();

	Show();

	// wait for view to become visible
	acquire_sem(visible);

	while(! quitting)
	{
		bigtime_t	start = system_time();
		int32		on = module->LoopOnCount();
		int32		off = on ? module->LoopOffCount() : 0;
		bool		dodraw = ! (on + off) || frame < on;

		if(dodraw && acquire_sem(drawing_lock) == B_OK)
		{
			if(direct_connected)
				module->DirectDraw(frame);

			release_sem(drawing_lock);
		}

		// lock the looper, quit if the window went away, the
		// window destructor will cleanup
		if(! v->LockLooper())
			break;

		if(dodraw)
		{
			module->Draw(v, frame);
			v->Sync();
		}

		v->UnlockLooper();

		frame++;
		if(on + off)
			frame %= (on + off);

		bigtime_t	delay = module->TickSize() - (system_time() - start);
		if(delay > 100)
			snooze(delay);
	}

	return 0;
}
