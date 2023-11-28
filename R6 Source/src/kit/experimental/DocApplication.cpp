#include <experimental/DocApplication.h>
#include <experimental/DocWindow.h>

#include <assert.h>
#include <FilePanel.h>
#include <NodeMonitor.h>
#include <Entry.h>
#include <Path.h>
#include <Node.h>
#include <Menu.h>
#include <MenuItem.h>
#include <stdio.h>
#include <Screen.h>
#include <Roster.h>

#define DEFAULTWINDOWX 300
#define DEFAULTWINDOWY 300

DocApplication::DocApplication(const char *signature, DocWindowFactory f)
 : BApplication(signature), factory(f),
   lastframe(20, 20, DEFAULTWINDOWX + 20, DEFAULTWINDOWY + 20),
   untitledcount(0), openpanel(0),
	fSignature(signature)
{
}

bool DocApplication::QuitRequested()
{
	return BApplication::QuitRequested();
}

void DocApplication::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case B_NODE_MONITOR :
			{
			node_ref nref; 
			if(msg->FindInt32("device", &nref.device) == B_OK &&
				msg->FindInt64("node", &nref.node) == B_OK)
			{
				int32	max = window_list.CountItems();
				int32	i;

				for(i = 0; i < max; i++)
				{
					window_info *info = (window_info *)window_list.ItemAt(i);
					if(info->ref && info->nref == nref)
					{
						info->w->EntryChanged(msg);
						break;	// there can be only one
					}
				}
			}
			}
			break;

		case B_PRINTER_CHANGED :
			// broadcast to all windows
			break;

		case DOC_APP_ACTIVATE_WINDOW :
			{
				int32 num;
				if(msg->FindInt32("window", &num) == B_OK)
				{
					window_info *info = (window_info *)window_list.ItemAt(num);
					if(info)
						info->w->Activate();
				}
			}
			break;

		case DOC_APP_NEW_WINDOW :
		case B_SILENT_RELAUNCH :
			New();
			break;

		case DOC_APP_OPEN :
			FileOpen();
			break;

		default :
			BApplication::MessageReceived(msg);
	}
}

void DocApplication::RefsReceived(BMessage *msg)
{
	inherited::RefsReceived(msg);
	LoadRefs(msg);
}

void DocApplication::ArgvReceived(int32 argc, char **argv)
{
	BEntry		e;
	BMessage	refs(B_REFS_RECEIVED);
	bool		found = false;

	for(int32 i = 1; i < argc; i++)
	{
		if(e.SetTo(argv[i]) == B_OK && e.Exists() == true)
		{
			entry_ref	ref;
			e.GetRef(&ref);
			refs.AddRef("refs", &ref);
			found = true;
		}
	}

	if(found)
		LoadRefs(&refs);
}

void DocApplication::LoadRefs(BMessage *msg)
{
	entry_ref	ref;
	for(int32 i = 0; msg->FindRef("refs", i, &ref) == B_OK; i++)
	{
		int32	max = window_list.CountItems();
		int32	j;

		for(j = 0; j < max; j++)
		{
			window_info *info = (window_info *)window_list.ItemAt(j);
			if(info->ref && *info->ref == ref)
			{
				info->w->Activate();
				be_roster->AddToRecentDocuments(&ref, fSignature.String());
				break;
			}
		}

		if(j == max)
		{
			BPath	path;
			BEntry	e(&ref);
			e.GetPath(&path);
			DocWindow *w = factory(this, &ref, path.Leaf(), B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, 0, B_CURRENT_WORKSPACE);

			if(w)
			{
				be_roster->AddToRecentDocuments(&ref, fSignature.String());

				BRect	screenframe = BScreen(w).Frame();
				lastframe.OffsetBy(20, 20);
				w->WindowFrame(&lastframe);
				WindowFrame(&lastframe);
				if(lastframe.bottom > screenframe.bottom)
				{
					lastframe.bottom = lastframe.Height() + 20;
					lastframe.top = 20;
				}
				if(lastframe.right > screenframe.right)
				{
					lastframe.right = lastframe.Width() + 20;
					lastframe.left = 20;
				}
				w->MoveTo(lastframe.LeftTop());
				w->ResizeTo(lastframe.Width(), lastframe.Height());

				w->Hide();
				w->Show();

				// the window now is running but is hidden
				if(w->Lock())
				{
					if(e.SetTo(&ref) == B_OK && w->Load(&e) == B_OK) {
						w->Show();
						w->Unlock();
					} else {
						w->Quit();
					}
				}
			}
		}
	}
}

void DocApplication::AddWindow(DocWindow *w, entry_ref *ref)
{
	// Add this window to the window menu of windows that have it
	int32		max = window_list.CountItems();
	int32		i;

	for(i = 0; i < max; i++)
	{
		window_info *info = (window_info *)window_list.ItemAt(i);
		if(info->windowmenu && info->w->Lock())
		{
			BMessage *msg = new BMessage(DOC_APP_ACTIVATE_WINDOW);
			msg->AddInt32("window", max);
			BMenuItem *act = new BMenuItem(w->Title(), msg, max < 9 ? max + '1' : 0);
			act->SetTarget(this);
			info->windowmenu->AddItem(act);
			info->w->Unlock();
		}
	}

	window_info	*info = new window_info;
	info->ref = ref;
	info->w = w;
	info->windowmenu = 0;
	window_list.AddItem(info);

	if(ref)
	{
		BEntry		e(ref);
		e.GetNodeRef(&info->nref);
		watch_node(&info->nref, B_WATCH_NAME | B_WATCH_STAT | B_WATCH_ATTR, be_app_messenger);
	}
}

BMenu *DocApplication::WindowMenu(DocWindow *w)
{
	int32		max = window_list.CountItems();
	int32		i;
	window_info	*found = 0;
	BMenu		*window = 0;

	assert(max > 0);

	for(i = 0; i < max; i++)
	{
		window_info *info = (window_info *)window_list.ItemAt(i);
		if(info->w == w)
			found = info;
	}

	if(found)
	{
		window = new BMenu("Window");
		found->windowmenu = window;

		for(i = 0; i < max; i++)
		{
			window_info *info = (window_info *)window_list.ItemAt(i);
			BMessage *msg = new BMessage(DOC_APP_ACTIVATE_WINDOW);
			msg->AddInt32("window", i);
			BMenuItem *act = new BMenuItem(info->w->Title(), msg, i < 9 ? i + '1' : 0);
			act->SetTarget(this);
			window->AddItem(act);
		}
	}

	return window;
}

void DocApplication::UpdateWindowMenu(DocWindow *w)
{
	int32	max = window_list.CountItems();
	int32	i;
	BMenu	*menu = 0;

	assert(max > 0);

	for(i = 0; i < max; i++)
	{
		window_info *info = (window_info *)window_list.ItemAt(i);
		if(info->w == w)
		{
			menu = info->windowmenu;
			break;
		}
	}

	if(menu)
	{
		assert(menu->CountItems() == max);

		for(i = 0; i < max; i++)
		{
			BMenuItem *it = menu->ItemAt(i);
			window_info *info = (window_info *)window_list.ItemAt(i);
			if(strcmp(it->Label(), info->w->Title()) != 0)
				it->SetLabel(info->w->Title());
		}
	}
}

void DocApplication::ChangeWindow(DocWindow *w, entry_ref *ref)
{
	int32	max = window_list.CountItems();
	int32	i;

	assert(max > 0);

	for(i = 0; i < max; i++)
	{
		window_info *info = (window_info *)window_list.ItemAt(i);
		if(info->w == w)
		{
			if(info->ref)
				watch_node(&info->nref, B_STOP_WATCHING, be_app_messenger);

			info->ref = ref;

			if(info->ref)
			{
				BEntry		e(info->ref);
				e.GetNodeRef(&info->nref);
				watch_node(&info->nref, B_WATCH_NAME | B_WATCH_STAT | B_WATCH_ATTR, be_app_messenger);
			}
			break;
		}
	}
}

void DocApplication::RemoveWindow(DocWindow *w)
{
	int32	max = window_list.CountItems();
	int32	i;

	assert(max > 0);

	for(i = 0; i < max; i++)
	{
		window_info *info = (window_info *)window_list.ItemAt(i);
		if(info->w == w)
		{
			window_list.RemoveItem(info);
			if(info->ref)
				watch_node(&info->nref, B_STOP_WATCHING, be_app_messenger);
			delete info;
			break;
		}
	}

	if(max-- <= 1)
		PostMessage(B_QUIT_REQUESTED);
	else
	{
		// Remove this window from the window menu of windows that have it
		for(int32 j = 0; j < max; j++)
		{
			window_info *info = (window_info *)window_list.ItemAt(j);
			if(info->windowmenu && info->w->Lock())
			{
				info->windowmenu->RemoveItem(i);
				for(int32 k = i; k < max; k++)
				{
					BMenuItem *it = info->windowmenu->ItemAt(k);
					assert(it != 0);
					it->SetShortcut(k < 9 ? k + '1' : 0, 0);
					BMessage *msg = new BMessage(DOC_APP_ACTIVATE_WINDOW);
					msg->AddInt32("window", k);
					it->SetMessage(msg);
				}
				info->w->Unlock();
			}
		}
	}
}

void DocApplication::ReadyToRun()
{
	if(window_list.CountItems() == 0)
		New();
	BApplication::ReadyToRun();
}

void DocApplication::FileOpen()
{
	if(! openpanel)
		openpanel = new BFilePanel();

	openpanel->Show();
}

void DocApplication::New()
{
	char	title[30];

	untitledcount++;
	sprintf(title, "Untitled %ld", untitledcount);
	DocWindow *w = factory(this, 0, title, B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS, B_CURRENT_WORKSPACE);

	BRect	screenframe = BScreen(w).Frame();
	lastframe.OffsetBy(20, 20);
	w->WindowFrame(&lastframe);
	WindowFrame(&lastframe);
	if(lastframe.bottom > screenframe.bottom)
	{
		lastframe.bottom = lastframe.Height() + 20;
		lastframe.top = 20;
	}
	if(lastframe.right > screenframe.right)
	{
		lastframe.right = lastframe.Width() + 20;
		lastframe.left = 20;
	}
	w->MoveTo(lastframe.LeftTop());
	w->ResizeTo(lastframe.Width(), lastframe.Height());

	w->Show();
}

void DocApplication::WindowFrame(BRect */*proposed*/)
{
}

