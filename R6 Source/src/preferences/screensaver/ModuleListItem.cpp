#include "ModuleListItem.h"
#include "ssdefs.h"
#include "Blackness.h"
#include "Settings.h"
#include "Wrapper.h"

#include <Application.h>
#include <Message.h>
#include <Entry.h>
#include <Path.h>
#include <FindDirectory.h>
#include <Roster.h>
#include <image.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

ModuleListItem::ModuleListItem(BPath path)
 : BStringItem(path.Leaf()), addon(path)
{
	id = -1;
	mod = 0;
}

ModuleListItem::~ModuleListItem()
{
	Unload();
}

void ModuleListItem::GetSettingsName(char *name)
{
	strcpy(name, kModuleSettingsPrefix);
	strcat(name, addon.Leaf());
}

status_t ModuleListItem::Load()
{
	if(mod)
		return id >= 0 ? B_OK : id;

	BMessage	module_settings;
	BMessage	*m = AcquireSettings();
	char		name[128];
	GetSettingsName(name);
	m->FindMessage(name, &module_settings);
	ReleaseSettings();

	if((id = load_add_on(addon.Path())) >= 0)
	{
		typedef BScreenSaver *(*instantiate_func)(BMessage *msg, image_id id);

		instantiate_func	modinst;

		if(get_image_symbol(id, "instantiate_screen_saver", B_SYMBOL_TYPE_TEXT, (void **)&modinst) != B_OK ||
			modinst == 0 ||
			(mod = modinst(&module_settings, id)) == 0 ||
			mod->InitCheck() != B_OK)
		{
			delete mod;
			mod = 0;
			unload_add_on(id);
			id = B_ERROR;
		}
	}

	return id >= 0 ? B_OK : id;
}

const char *ModuleListItem::InternalName()
{
	return Text();
}

void ModuleListItem::SaveSettings()
{
	BMessage module_settings;
	if(mod->SaveState(&module_settings) == B_OK)
	{
		BMessage *m = AcquireSettings();
		char name[128];
		GetSettingsName(name);
		m->RemoveName(name);
		m->AddMessage(name, &module_settings);
		ReleaseSettings();
	}
}

void ModuleListItem::ModulesChanged(const BMessage *msg)
{
	if(mod)
		mod->ModulesChanged(msg);
}

void ModuleListItem::Unload()
{
	if(mod)
	{
		SaveSettings();
		delete mod;
		mod = 0;
		unload_add_on(id);
		id = -1;
	}
}

const BPath *ModuleListItem::Path() const
{
	return &addon;
}

void ModuleListItem::Test()
{
	SaveSettings();

	BEntry e(addon.Path());
	entry_ref ref;
	e.GetRef(&ref);
	BMessage msg(B_REFS_RECEIVED);
	msg.AddRef("refs", &ref); // data copied from ref
	msg.AddBool("_testmode_", true);
#if SCREENSAVER_LAUNCH_BY_PATH
	get_ref_for_path(kModuleRunnerPath, &ref); // ref reset
	be_roster->Launch(&ref, &msg);
#else
	be_roster->Launch(module_runner_signature, &msg);
#endif
}

BlacknessListItem::BlacknessListItem()
 : ModuleListItem("Blackness")
{
}

const char *BlacknessListItem::InternalName()
{
	return "";
}

status_t BlacknessListItem::Load()
{
	BMessage	module_settings;
	mod = new Blackness(&module_settings, id);

	return B_OK;
}

void BlacknessListItem::Unload()
{
	if(mod)
	{
		delete mod;
		mod = 0;
	}
}

void BlacknessListItem::Test()
{
	entry_ref ref;
	BMessage msg(B_REFS_RECEIVED);
	msg.AddRef("refs", &ref); // data copied from ref
#if SCREENSAVER_LAUNCH_BY_PATH
	get_ref_for_path(kModuleRunnerPath, &ref); // ref reset
	be_roster->Launch(&ref, &msg);
#else
	be_roster->Launch(module_runner_signature, &msg);
#endif
}
