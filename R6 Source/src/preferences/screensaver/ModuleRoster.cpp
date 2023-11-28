#include "Blanket.h"
#include "ModuleRoster.h"
#include "DeepCopy.h"
#include "ssdefs.h"
#include "Settings.h"

#include "OldScreenSaver.h"
#include <FindDirectory.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <Directory.h>
#include <Beep.h>
#include <String.h>

#include <stdio.h>
#include <string.h>

struct module_info
{
	BPath		path;
	BMessage	data;
};

ModuleRoster::ModuleRoster()
 : BLooper("Module Roster")
{
	PostMessage(BLANKET_ROSTER_INIT);
}

ModuleRoster::~ModuleRoster()
{
	stop_watching(this);
}

void ModuleRoster::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case B_REFS_RECEIVED :
			Install(msg);
			break;

		case BLANKET_ROSTER_INIT :
			{
				node_ref	dirref;
				BEntry		direntry;
				BDirectory	dir;
				BPath		addons;

				if(find_directory(B_BEOS_ADDONS_DIRECTORY, &addons) == B_OK &&
					addons.Append(kScreenSaversDir) == B_OK &&
					direntry.SetTo(addons.Path()) == B_OK &&
					direntry.GetNodeRef(&dirref) == B_NO_ERROR)
					watch_node(&dirref, B_WATCH_DIRECTORY, this);

				if(find_directory(B_USER_ADDONS_DIRECTORY, &addons) == B_OK &&
					addons.Append(kScreenSaversDir) == B_OK &&
					direntry.SetTo(addons.Path()) == B_OK &&
					direntry.GetNodeRef(&dirref) == B_NO_ERROR)
					watch_node(&dirref, B_WATCH_DIRECTORY, this);

				ModuleListDiffs();
			}
			break;

		case B_NODE_MONITOR :
			// something happened in the module directory: load changes
			ModuleListDiffs();
			break;

		case BLANKET_ROSTER_START :
			if(msg->FindMessenger("target", &view) == B_OK &&
				msg->FindMessage("addmsg", &addnotif) == B_OK &&
				msg->FindMessage("remmsg", &remnotif) == B_OK &&
				msg->FindMessage("modmsg", &modulesnotif) == B_OK &&
				view.IsValid())
			{
				int32 max = modules.CountItems();
				for(int32 j = 0; j < max; j++)
				{
					module_info *info = (module_info *)modules.ItemAt(j);

					// clone notification message and send it
					BMessage notif(addnotif);
					notif.AddString("module", info->path.Path());
					view.SendMessage(&notif);
				}

				ModulesChangedNotification();
			}
			break;

		default :
			BLooper::MessageReceived(msg);
			break;
	}
}

void ModuleRoster::ModuleList(BDirectory *dir, BList *modules)
{
	BEntry		ent;
	if(dir->Rewind() == B_OK)
	{
		while(dir->GetNextEntry(&ent) == B_NO_ERROR)
		{
			BPath		path;
			BMessage	data;
			ent.GetPath(&path);

			// first check if it's a Saver add-on
			if(ModuleRoster::IsModule(path, &data))
			{
				module_info	*info = new module_info;
				info->path = path;
				info->data = data;
				modules->AddItem(info);
			}
		}
	}
}

void ModuleRoster::ModuleListDiffs()
{
	BList		newmods;
	BDirectory	dir;
	BPath		addons;

	if(find_directory(B_BEOS_ADDONS_DIRECTORY, &addons) == B_OK &&
		addons.Append(kScreenSaversDir) == B_OK &&
		dir.SetTo(addons.Path()) == B_NO_ERROR)
		ModuleList(&dir, &newmods);

	if(find_directory(B_USER_ADDONS_DIRECTORY, &addons) == B_OK &&
		addons.Append(kScreenSaversDir) == B_OK &&
		dir.SetTo(addons.Path()) == B_NO_ERROR)
		ModuleList(&dir, &newmods);

	int32	max = newmods.CountItems();
	for(int32 i = 0; i < max; i++)
	{
		int32		oldmax = modules.CountItems();
		module_info	*curr = (module_info *)newmods.ItemAt(i);
		module_info	*scan = 0;
		BPath		path;
		int32		j;
		for(j = 0; j < oldmax; j++)
		{
			scan = (module_info *)modules.ItemAt(j);
			if(scan->path == curr->path)
				break;
		}

		if(j < oldmax)
		{
			modules.RemoveItem(j);	// found it
			delete scan;
		}
		else
			if(view.IsValid())
			{
				// clone notification message and send it
				BMessage notif(addnotif);
				notif.AddString("module", curr->path.Path());
				view.SendMessage(&notif);
			}
	}

	// delete leftover modules and replace old module list with new one
	max = modules.CountItems();
	for(int32 j = max - 1; j >= 0; j--)
	{
		module_info *info = (module_info *)modules.ItemAt(j);

		if(view.IsValid())
		{
			// clone notification message and send it
			BMessage notif(remnotif);
			notif.AddString("module", info->path.Path());
			view.SendMessage(&notif);
		}

		modules.RemoveItem(j);
		delete info;
	}
	modules = newmods;

	if(view.IsValid())
		ModulesChangedNotification();
}

void ModuleRoster::ModulesChangedNotification()
{
	int32 max = modules.CountItems();
	BMessage	metadata;
	for(int32 j = 0; j < max; j++)
	{
		module_info *info = (module_info *)modules.ItemAt(j);
		metadata.AddString("name", info->path.Leaf());
		metadata.AddString("path", info->path.Path());
		metadata.AddMessage("info", &info->data);
	}

	// clone notification message and send it
	BMessage notif(modulesnotif);
	notif.AddMessage("metadata", &metadata);
	view.SendMessage(&notif);
}

bool ModuleRoster::IsModule(BPath path, BMessage *data)
{
	image_id	addon_image;
	bool		ismodule = false;

	if((addon_image = load_add_on(path.Path())) != B_ERROR)
	{
		int *saverVersionID;
		if(get_image_symbol(addon_image, "saverVersionID", B_SYMBOL_TYPE_DATA, (void **)&saverVersionID) == B_OK)
		{
			BScreenSaver	*mod;
			BMessage		module_settings;
			BMessage		*m = AcquireSettings();
			char			name[128];

			// get settings
			strcpy(name, kModuleSettingsPrefix);
			strcat(name, path.Leaf());
			m->FindMessage(name, &module_settings);
			ReleaseSettings();

			// instantiate screen saver
			typedef BScreenSaver *(*instantiate_func)(BMessage *msg, image_id id);

			instantiate_func	modinst;

			if(get_image_symbol(addon_image, "instantiate_screen_saver", B_SYMBOL_TYPE_TEXT, (void **)&modinst) == B_OK && modinst)
			{
				if((mod = modinst(&module_settings, addon_image)) != 0)
				{
					if(mod->InitCheck() == B_OK)
						// get data
						mod->SupplyInfo(data);
		
					delete mod;
				}
				ismodule = true;
			}
		}
		else
			ismodule = true;

		unload_add_on(addon_image);
	}

	return ismodule;
}

void ModuleRoster::Install(BMessage *msg)
{
	BList		goodrefs;
	entry_ref	ref;

	for(int32 i = 0; msg->FindRef("refs", i, &ref) == B_NO_ERROR; i++)
	{
		BEntry	e;
		if(e.SetTo(&ref) == B_NO_ERROR)
		{
			BPath path;
			BMessage data;
			e.GetPath(&path);
			if(ModuleRoster::IsModule(path, &data))
				goodrefs.AddItem(new entry_ref(ref));
		}
	}

	// copy to user's screen savers directory
	BPath		addons;
	if(goodrefs.CountItems() > 0 &&
		find_directory(B_USER_ADDONS_DIRECTORY, &addons) == B_OK &&
		addons.Append(kScreenSaversDir) == B_OK)
	{
		BDirectory dir(addons.Path());

		int32 max = goodrefs.CountItems();
		for(int32 i = 0; i < max; i++)
		{
			entry_ref	*ref = (entry_ref *)goodrefs.ItemAt(i);
			if(! DeepCopy(dir, 0, ref))
				beep();
		}

		max = goodrefs.CountItems();
		for(int32 i = 0; i < max; i++)
		{
			entry_ref	*ref = (entry_ref *)goodrefs.ItemAt(i);
			delete ref;
		}
	}
	else
		beep();
}
