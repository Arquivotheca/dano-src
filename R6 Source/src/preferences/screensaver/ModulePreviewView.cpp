#include "ModulePreviewView.h"
#include "ModuleListItem.h"
#include <Application.h>
#include <AppFileInfo.h>
#include <Screen.h>
#include <Bitmap.h>
#include <Debug.h>
#include <Roster.h>

#include <stdio.h>

ModulePreviewView::ModulePreviewView(ModuleListItem *mod, BRect frame)
 : BView(frame, "preview", B_WILL_DRAW, 0), module(mod), thread(B_ERROR)
{
	ASSERT(mod);
}

ModulePreviewView::~ModulePreviewView()
{
}

void ModulePreviewView::AttachedToWindow()
{
	offscreen = new BBitmap(Bounds(), BScreen().ColorSpace(), true);
	renderer = new BView(Bounds(), "renderer", 0, 0);
	renderer->SetHighColor(255, 255, 255);
	renderer->SetViewColor(0, 0, 0);
	renderer->SetLowColor(0, 0, 0);
	offscreen->AddChild(renderer);

	// start the module saver thread
	offscreen->Lock();
	status_t ret = module->mod->StartSaver(renderer, true);
	renderer->Sync();
	offscreen->Unlock();

	BBitmap *logo = 0;
	if(ret != B_OK)
	{
		app_info	info;
	
		if(be_app->GetAppInfo(&info) == B_OK)
		{
			BFile appfile(&info.ref, B_READ_ONLY);
	
			if(appfile.InitCheck() == B_NO_ERROR)
			{
				BAppFileInfo appfileinfo(&appfile);
				logo = new BBitmap(BRect(0.0,0.0,31.0,31.0), B_COLOR_8_BIT);
				if(appfileinfo.GetIcon(logo, B_LARGE_ICON) != B_NO_ERROR)
				{
					delete logo;
					logo = 0;
				}
			}
		}
	}

	offscreen->Lock();
	if(logo)
		renderer->SetLowColor(119, 116, 119, 255);
	renderer->FillRect(renderer->Bounds(), B_SOLID_LOW);
	if(logo)
	{
		BRect dst = logo->Bounds();
		dst.OffsetBy((renderer->Bounds().Width() - dst.Width()) / 2,
				(renderer->Bounds().Height() - dst.Height()) / 2);
		renderer->DrawBitmap(logo, logo->Bounds(), dst);
	}
	renderer->Sync();
	offscreen->Unlock();

	delete logo;

	quitsem = B_ERROR;
	modaccess = B_ERROR;
	if(ret == B_OK)
	{
		quitsem = create_sem(0, "quit sem");
		modaccess = create_sem(1, "mod access sem");
		resume_thread(thread = spawn_thread(thread_entry_stub, "preview animator", B_LOW_PRIORITY, this));
	}
}

void ModulePreviewView::DetachedFromWindow()
{
	// make it quit
	delete_sem(quitsem);
	quitsem = B_ERROR;

	// prevent the thread from using the module
	acquire_sem(modaccess);
	delete_sem(modaccess);

	ASSERT(module);
	ASSERT(module->mod);

	// stop the module saver thread
	module->mod->StopSaver();

	offscreen->RemoveChild(renderer);
	delete renderer;
	delete offscreen;
}

void ModulePreviewView::Draw(BRect r)
{
	DrawBitmapAsync(offscreen, r, r);
}

int32 ModulePreviewView::thread_entry_stub(void *param)
{
	if(((ModulePreviewView *)param)->LockLooper())
		return ((ModulePreviewView *)param)->ModuleThread();
	else
		return 0;
}

int32 ModulePreviewView::ModuleThread()
{
	int32		frame = 0;
	bigtime_t	delay;
	sem_id		sem = quitsem;		// store semaphore id on the stack
	sem_id		modsem = modaccess;		// store semaphore id on the stack

	UnlockLooper();

	do
	{
		// lock the looper, quit if the window went away, the
		// window destructor will cleanup
		if(! LockLooper())
			break;

		// if we can no longer access the module then quit
		if(acquire_sem(modsem) != B_OK)
		{
			UnlockLooper();
			break;
		}

		if(IsHidden())
			delay = 100000;
		else
		{
			bigtime_t	start = system_time();
			int32		on = module->mod->LoopOnCount();
			int32		off = on ? module->mod->LoopOffCount() : 0;
			bool		dodraw = ! (on + off) || frame < on;

			if(dodraw)
			{
				offscreen->Lock();

				ASSERT(module);
				ASSERT(module->mod);

				module->mod->Draw(renderer, frame);
				renderer->Sync();
				offscreen->Unlock();

				DrawBitmap(offscreen);
				Sync();
			}

			delay = module->mod->TickSize() - (system_time() - start);

			frame++;
			if(on + off)
				frame %= (on + off);
		}

		release_sem(modsem);
		UnlockLooper();
	}
	while(acquire_sem_etc(sem, 1,  B_TIMEOUT, delay > 100 ? delay : 1) == B_TIMED_OUT);

	return 0;
}
