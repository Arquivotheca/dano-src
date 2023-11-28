#include "Wrapper.h"
#include <StringView.h>
#include <stdlib.h>
#include <string.h>

Wrapper::Wrapper(BMessage *message, image_id image)
 : BScreenSaver(message, image)
{
	settingsopen = false;

	if(get_image_symbol(image, "module_initialize", B_SYMBOL_TYPE_TEXT, (void **)&module_initialize) != B_OK)
		module_initialize = 0;
	if(get_image_symbol(image, "module_cleanup", B_SYMBOL_TYPE_TEXT, (void **)&module_cleanup) != B_OK)
		module_cleanup = 0;
	if(get_image_symbol(image, "module_start_saving", B_SYMBOL_TYPE_TEXT, (void **)&module_stop_saving) != B_OK)
		module_stop_saving = 0;
	if(get_image_symbol(image, "module_stop_saving", B_SYMBOL_TYPE_TEXT, (void **)&module_stop_saving) != B_OK)
		module_stop_saving = 0;
	if(get_image_symbol(image, "module_start_config", B_SYMBOL_TYPE_TEXT, (void **)&module_start_config) != B_OK)
		module_start_config = 0;
	if(get_image_symbol(image, "module_stop_config", B_SYMBOL_TYPE_TEXT, (void **)&module_stop_config) != B_OK)
		module_stop_config = 0;

	init(message);
}

Wrapper::~Wrapper()
{
	BMessage into;
	term(&into);
}

void Wrapper::init(BMessage *message)
{
	if(module_initialize)
	{
		const void *set;
		void *data = 0;
		int32 setsize = 0;

		if(message->FindData("settings", B_RAW_TYPE, &set, &setsize) == B_OK)
		{
			data = malloc(setsize);
			if(data == 0)
				setsize = 0;
			else
				memcpy(data, set, setsize);
		}
		else
		{
			data = 0;
			setsize = 0;
		}

		module_initialize(data, setsize);
	}

	if(settingsopen)
		StartConfig(settings);
}

status_t Wrapper::SaveState(BMessage *into) const
{
	Wrapper *nonconst = (Wrapper *)this;
	nonconst->term(into);
	nonconst->init(into);
	return B_OK;
}

void Wrapper::term(BMessage *into)
{
	if(settingsopen)
		StopConfig();

	if(module_cleanup)
	{
		void *outset = 0;
		int32 outsetsize = 0;

		module_cleanup(&outset, &outsetsize);
		if(outset != 0 && outsetsize != 0)
		{
			if(into->HasData("settings", B_RAW_TYPE))
				into->ReplaceData("settings", B_RAW_TYPE, outset, outsetsize);
			else
				into->AddData("settings", B_RAW_TYPE, outset, outsetsize, false);
		}

		free(outset);
	}
}

void Wrapper::StartConfig(BView *view)
{
	settings = view;
	settingsopen = true;
	if(module_start_config)
		module_start_config(view);
}

void Wrapper::StopConfig()
{
	if(module_stop_config)
		module_stop_config();
}

status_t Wrapper::StartSaver(BView *v, bool preview)
{
	if(preview)
		return B_ERROR;
	else
	{
		SetTickSize(100000);
		v->SetViewColor(0, 0, 0);

		if(module_start_saving)
			module_start_saving(v);

		return B_OK;
	}
}

void Wrapper::StopSaver()
{
	if(module_stop_saving)
		module_stop_saving();
}
