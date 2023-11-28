#include "Random.h"
#include <StringView.h>
#include <CheckBox.h>
#include <ScrollView.h>
#include <Slider.h>
#include <View.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

class CheckboxList : public BView
{
	float		bottom;
	float		scrollbary;
	float		initialy;
	BScrollView	*scrollview;
	BScrollBar	*sb;
	BList		checkboxlist;

public:
				CheckboxList(BRect frame, const char *name, uint32 resizeMask, uint32 flags);
				~CheckboxList();
	void		TargetedByScrollView(BScrollView *sv);
	void		AttachedToWindow();
	void		AllAttached();
	void		AddCheckbox(const char *name, const char *path, bool checked);
	void		StartRefresh();
	void		EndRefresh(BMessage *settings);
	void		AdjustScrollbars();
};

CheckboxList::CheckboxList(BRect frame, const char *name, uint32 resizeMask, uint32 flags)
 : BView(frame, name, resizeMask, flags)
{
	bottom = 0;
	sb = 0;
	BCheckBox *check = new BCheckBox(BRect(0, 0, 50, 20), "test", "test", new BMessage('test'));
	float w;
	check->GetPreferredSize(&w, &scrollbary);
	delete check;
	initialy = Bounds().Height();
}

CheckboxList::~CheckboxList()
{
}

void CheckboxList::TargetedByScrollView(BScrollView *sv)
{
	scrollview = sv;
}

void CheckboxList::AttachedToWindow()
{
	for(BView *v = ChildAt(0); v; v = v->NextSibling())
	{
		BCheckBox *c = dynamic_cast<BCheckBox *>(v);
		if(c)
			c->SetTarget(Parent());
	}
}

void CheckboxList::AllAttached()
{
	sb = scrollview->ScrollBar(B_VERTICAL);
	if(sb)
		sb->SetSteps(scrollbary, initialy);
}

void CheckboxList::StartRefresh()
{
	checkboxlist.MakeEmpty();
	for(BView *v = ChildAt(0); v; v = v->NextSibling())
	{
		BCheckBox *c = dynamic_cast<BCheckBox *>(v);
		if(c)
			checkboxlist.AddItem(v);
	}
}

void CheckboxList::EndRefresh(BMessage *settings)
{
	int32	i;
	int32	max = checkboxlist.CountItems();

	for(i = 0; i < max; i++)
	{
		BCheckBox *c = (BCheckBox *)checkboxlist.ItemAt(i);
		RemoveChild(c);

		// scroll controls up and shrink view
		Window()->BeginViewTransaction();
		BRect r = c->Frame();
		for(BView *v = ChildAt(0); v; v = v->NextSibling())
			if(v->Frame().top >= r.bottom)
				v->MoveBy(0, -scrollbary);
		bottom -= scrollbary;
		ResizeBy(0, -scrollbary);
		Window()->EndViewTransaction();

		settings->RemoveName(c->Name());
		delete c;
	}
	AdjustScrollbars();
}

void CheckboxList::AddCheckbox(const char *name, const char *path, bool checked)
{
	BCheckBox	*check;

	if((check = (BCheckBox *)FindView(name)) == 0)
	{
		BRect	b = Bounds();

		// create checkbox
		BMessage *msg = new BMessage('clic');
		msg->AddString("name", name);
		msg->AddString("path", path);
		check = new BCheckBox(BRect(0, bottom, b.Width(), bottom + scrollbary), name, name, msg);

		// find (sorted) insertion point insertion
		float	top = bottom;
		for(BView *v = ChildAt(0); v; v = v->NextSibling())
			if(v->Frame().top < top && strcasecmp(v->Name(), name) > 0)
				top = v->Frame().top;

		// expand view
		bottom += scrollbary;
		if(bottom > initialy)
			ResizeTo(b.Width(), bottom);

		// scroll controls down and new control up
		Window()->BeginViewTransaction();
		for(BView *v = ChildAt(0); v; v = v->NextSibling())
			if(v->Frame().top >= top)
			{
				v->MoveBy(0, scrollbary);
				check->MoveBy(0, -scrollbary);
			}
		Window()->EndViewTransaction();

		// add new control
		AddChild(check);

		if(Parent())
			check->SetTarget(Parent());

		AdjustScrollbars();
	}
	checkboxlist.RemoveItem(check);
	check->SetValue(checked ? 1 : 0);
}

void CheckboxList::AdjustScrollbars()
{
	if(sb)
	{
		if(bottom > initialy)
		{
			sb->SetProportion(initialy / bottom);
			sb->SetRange(0, bottom - initialy);
		}
		else
			sb->SetRange(0, 0);
	}
}


class ConfigView : public BView
{
	BScrollView		*list;
	CheckboxList	*view;
//	BCheckBox		*flip;
//	BSlider			*time;
	BMessage		*settings;

public:
					ConfigView(BMessage *settings, BRect frame, const char *name, uint32 resizeMask, uint32 flags);
	void			AttachedToWindow();
	void			SetModules(const BMessage *msg);
	void			MessageReceived(BMessage *msg);
};

ConfigView::ConfigView(BMessage *s, BRect frame, const char *name, uint32 resizeMask, uint32 flags)
 : BView(frame, name, resizeMask, flags), settings(s)
{
	AddChild(new BStringView(BRect(10, 0, 200, 18), B_EMPTY_STRING, "Random module picker"));
	view = new CheckboxList(BRect(15, 30, 180, 120), B_EMPTY_STRING, 0, 0);
	list = new BScrollView("list", view, B_FOLLOW_LEFT | B_FOLLOW_TOP, 0, false, true);
	AddChild(list);

//	flip = new BCheckBox(BRect(20, 130, 180, 145), "flip", "Switch every", new BMessage('flip'));
//	AddChild(flip);
//	time = new BSlider(BRect(20, 150, 140, 200), "time", "Time", new BMessage('time'), 1, 30);
//	AddChild(time);
}

void ConfigView::AttachedToWindow()
{
	SetViewColor(Parent()->ViewColor());
//	flip->SetTarget(this);
//	time->SetTarget(this);
}

void ConfigView::SetModules(const BMessage *msg)
{
	int32		i;
	const char	*name;
	const char	*path;
	BMessage	module;

	view->StartRefresh();
	for(i = 0; msg->FindString("name", i, &name) == B_OK &&
		msg->FindString("path", i, &path) == B_OK &&
		msg->FindMessage("info", i, &module) == B_OK; i++)
	{
		bool	rand;
		if(module.FindBool("randomize", &rand) != B_OK || rand == true)
		{
			// find in settings
			const char *str;
			if(settings->FindString(name, &str) != B_OK)
				str = path;
			settings->RemoveName(name);
			settings->AddString(name, str);
			view->AddCheckbox(name, path, *str ? true : false);
		}
	}
	view->EndRefresh(settings);
}

void ConfigView::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
//		case 'flip' :
//			time->SetEnabled(flip->Value() ? true : false);
//			break;
//
//		case 'time' :
//			break;

		case 'clic' :
			{
			const char	*name;
			const char	*path;
			int32		value;
			if(msg->FindString("name", &name) == B_OK &&
				msg->FindString("path", &path) == B_OK &&
				msg->FindInt32("be:value", &value) == B_OK)
			{
				settings->RemoveName(name);
				settings->AddString(name, value ? path : "");
			}
			}
			break;

		default :
			BView::MessageReceived(msg);
			break;
	}
}


// MAIN INSTANTIATION FUNCTION
extern "C" _EXPORT BScreenSaver *instantiate_screen_saver(BMessage *message, image_id image)
{
	return new Random(message, image);
}

Random::Random(BMessage *message, image_id image)
 : BScreenSaver(message, image), current(0)
{
	message->FindMessage("modules", &modulesettings);
	srand(time(0L));
}

status_t Random::SaveState(BMessage *msg) const
{
	msg->AddMessage("modules", &modulesettings);
	return B_OK;
}

void Random::SupplyInfo(BMessage *info) const
{
	info->AddBool("randomize", false);
}

void Random::ModulesChanged(const BMessage *msg)
{
	if(config && config->LockLooper())
	{
		config->SetModules(msg);
		config->UnlockLooper();
	}
}

void Random::StartConfig(BView *view)
{
	config = new ConfigView(&modulesettings, BRect(0, 0, 241, 130), B_EMPTY_STRING, 0, 0);
	view->AddChild(config);
}

status_t Random::StartSaver(BView *view, bool preview)
{
	if(! preview)
	{
		const char	*path;
		int32		modules = 0;
		BMessage	selection;

		// enumerate available modules
		int32 max = modulesettings.CountNames(B_STRING_TYPE);
		int32 i;
		for(i = 0; i < max; i++)
		{
			char		*name;
			type_code	t;
			int32		found;
			int32		j;

			modulesettings.GetInfo(B_STRING_TYPE, i, &name, &t, &found);

			for(j = 0; j < found; j++)
			{
				if(modulesettings.FindString(name, j, &path) == B_OK &&
					path && *path)
				{
					selection.AddString("path", path);
					modules++;
				}
			}
		}

		// pick a module
		int32 sel = (int32)((float)rand() / RAND_MAX * (float)modules);
		if(max && selection.FindString("path", sel, &path) == B_OK)
		{
			// load module
			if((currentid = load_add_on(path)) >= 0)
			{
				typedef BScreenSaver *(*instantiate_func)(BMessage *msg, image_id id);
	
				instantiate_func	modinst;
				BMessage settings;	// TODO: load real settings
				if(get_image_symbol(currentid, "instantiate_screen_saver", B_SYMBOL_TYPE_TEXT, (void **)&modinst) != B_OK ||
					modinst == 0 ||
					(current = modinst(&settings, currentid)) == 0 ||
					current->InitCheck() != B_OK)
				{
					delete current;
					current = 0;
					unload_add_on(currentid);
					currentid = B_ERROR;
				}
			}
		}

		if(current)
		{
			current->StartSaver(view, preview);
			SetTickSize(current->TickSize());
			SetLoop(current->LoopOnCount(), current->LoopOffCount());
		}
	}

	return preview ? B_ERROR : B_OK;	// won't work in preview mode
}

void Random::StopSaver()
{
	if(current)
	{
		current->StopSaver();
		delete current;
		current = 0;
		unload_add_on(currentid);
		currentid = B_ERROR;
	}
}

void Random::Draw(BView *view, int32 frame)
{
	if(current)
	{
		current->Draw(view, frame);
		SetTickSize(current->TickSize());
		SetLoop(current->LoopOnCount(), current->LoopOffCount());
	}
}

void Random::DirectDraw(int32 frame)
{
	if(current)
	{
		current->DirectDraw(frame);
		SetTickSize(current->TickSize());
		SetLoop(current->LoopOnCount(), current->LoopOffCount());
	}
}

void Random::DirectConnected(direct_buffer_info *info)
{
	if(current)
	{
		current->DirectConnected(info);
		SetTickSize(current->TickSize());
		SetLoop(current->LoopOnCount(), current->LoopOffCount());
	}
}
