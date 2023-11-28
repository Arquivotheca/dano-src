#include <File.h>
#include <Resources.h>
#include <Entry.h>
#include <Slider.h>
#include <TextView.h>
#include <TextControl.h>
#include <Button.h>
#include <Menu.h>
#include <PopUpMenu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Path.h>
#include <FindDirectory.h>
#include <Directory.h>
#include <Roster.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "PrefsAppExport.h"
#include "KeyboardAddOn.h"

KeyboardView*the_view;

///////////////////////////////////////////////////
// format of the keybaord settings file

typedef struct {
	bigtime_t	key_repeat_delay;
	int32		key_repeat_rate;
} kb_settings;

#define kb_settings_file "Keyboard_settings"

////////////////////////////////////
// defaults

#define kb_default_delay 250000
#define kb_default_repeat_rate 240

////////////////////////////////////

PPAddOn* get_addon_object(image_id i,PPWindow*w) {
	return (PPAddOn*)new KeyboardAddOn(i,w);
}

KeyboardAddOn::KeyboardAddOn(image_id i,PPWindow*w):PPAddOn(i,w) {
}

bool KeyboardAddOn::UseAddOn() {
	return true;
}

BBitmap* KeyboardAddOn::Icon() {
	return (BBitmap*)InstantiateFromResource(Resources,"icon");
}

BBitmap* KeyboardAddOn::SmallIcon() {
	return (BBitmap*)InstantiateFromResource(Resources,"smallicon");
}

char* KeyboardAddOn::Name() {
	size_t foo_size;
	return (char*)Resources->LoadResource(B_STRING_TYPE,"add-on name",&foo_size);
}

char* KeyboardAddOn::InternalName() {
	return "keyboard";
}

BView* KeyboardAddOn::MakeView() {
	the_view=(KeyboardView*)InstantiateFromResource(Resources,"view");
	return the_view;
}

void KeyboardAddOn::PanelActivated(bool a) {
	if (a) {
		the_view->PanelActivated();
	}
}

//////////////////////////////////////////////////////////////

KeyboardView::KeyboardView(BMessage*m):BView(m) {
}

BArchivable* KeyboardView::Instantiate(BMessage*m) {
	if (!validate_instantiation(m,"KeyboardView")) {
		return NULL;
	}
	return new KeyboardView(m);
}

void KeyboardView::AllAttached() {
	((BSlider*)FindView("delayslider"))->SetTarget(this);
	bigtime_t kd;
	if (get_key_repeat_delay(&kd)!=B_OK) {
		kd=kb_default_delay;
	}
	((BSlider*)FindView("delayslider"))->SetValue(kd/250000LL);
	((BSlider*)FindView("repeatslider"))->SetTarget(this);
	int32 krr;
	if (get_key_repeat_rate(&krr)!=B_OK) {
		krr=kb_default_repeat_rate;
	}
	((BSlider*)FindView("repeatslider"))->SetValue(krr);
	((BButton*)FindView("default"))->SetTarget(this);
	((BButton*)FindView("revert"))->SetTarget(this);
	((BButton*)FindView("edit"))->SetTarget(this);
}

void KeyboardView::MessageReceived(BMessage*m) {
	switch(m->what) {
		case 'kbrr' : {
			set_key_repeat_rate(m->FindInt32("be:value"));
			((BButton*)FindView("revert"))->SetEnabled(EnableRevert(((BSlider*)FindView("delayslider"))->Value(),((BSlider*)FindView("repeatslider"))->Value()));
			break;
		}
		case 'kbde' : {
			set_key_repeat_delay(250000LL*m->FindInt32("be:value"));
			((BButton*)FindView("revert"))->SetEnabled(EnableRevert(((BSlider*)FindView("delayslider"))->Value(),((BSlider*)FindView("repeatslider"))->Value()));
			break;
		}
		case 'dflt' : {
			set_key_repeat_delay(kb_default_delay);
			((BSlider*)FindView("delayslider"))->SetValue(kb_default_delay/250000LL);
			set_key_repeat_rate(kb_default_repeat_rate);
			((BSlider*)FindView("repeatslider"))->SetValue(kb_default_repeat_rate);
			printf("default\n");
			break;
		}
		case 'rvrt' : {
			set_key_repeat_delay(initial_delay);
			((BSlider*)FindView("delayslider"))->SetValue(initial_delay/250000LL);
			set_key_repeat_rate(initial_repeat);
			((BSlider*)FindView("repeatslider"))->SetValue(initial_repeat);
			((BButton*)FindView("revert"))->SetEnabled(false);
			break;
		}
		case 'edit' : {
			printf("edit\n");
			be_roster->Launch("application/x-vnd.Be-KBRD");
			break;
		}
		default : {
			BView::MessageReceived(m);
			break;
		}
	}
}

void KeyboardView::AllDetached() {
	kb_settings settings;
	if (get_key_repeat_delay(&settings.key_repeat_delay)!=B_OK) return;
	if (get_key_repeat_rate(&settings.key_repeat_rate)!=B_OK) return;
	BPath path;
	if (find_directory (B_USER_SETTINGS_DIRECTORY, &path, true) == B_OK) {
		long ref;
		path.Append(kb_settings_file);
		if ((ref = creat(path.Path(), 0644)) >= 0) {
			write (ref, &settings, sizeof (kb_settings));
			close(ref);
		}
	}
}

void KeyboardView::PanelActivated() {
	initial_repeat=((BSlider*)FindView("repeatslider"))->Value();
	initial_delay=((BSlider*)FindView("delayslider"))->Value();
	((BButton*)FindView("revert"))->SetEnabled(false);
}

bool KeyboardView::EnableRevert(int32 cd, int32 cr) {
	return cd!=initial_delay||cr!=initial_repeat;
}
