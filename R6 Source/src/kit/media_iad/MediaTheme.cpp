/*	BMediaTheme.cpp	*/

#include "ParameterWeb.h"
#include "_BMSystemTheme.h"
#include "tr_debug.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <Locker.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Autolock.h>
#include <image.h>
#include <Slider.h>
#include <Screen.h>
#include <TranslationUtils.h>
#include <Bitmap.h>
#include <CheckBox.h>
#include <OptionPopUp.h>
#include <ChannelSlider.h>
#include "TVChannelControl.h"


#if !NDEBUG
#define FPRINTF fprintf
#else
#define FPRINTF (void)
#endif

#define PREFS //FPRINTF

static BLocker _themeLock("Media Control Theme Lock");


BMediaTheme * BMediaTheme::_mDefaultTheme = NULL;


BMediaTheme::BMediaTheme(
	const char * name,
	const char * info,
	const entry_ref * add_on,
	int32 theme_id)
{
	_mName = name ? strdup(name) : NULL;
	_mInfo = info ? strdup(info) : NULL;
	_mID = theme_id;
	if (add_on) {
		_mAddOn = true;
		_mAddOnRef = *add_on;
	}
	else {
		_mAddOn = false;
	}
}


BMediaTheme::~BMediaTheme()
{
	free(_mName);
	free(_mInfo);
}


const char *
BMediaTheme::Name()
{
	return _mName;
}


const char *
BMediaTheme::Info()
{
	return _mInfo;
}


int32
BMediaTheme::ID()
{
	return _mID;
}


bool
BMediaTheme::GetRef(
	entry_ref * out_ref)
{
	if (_mAddOn) *out_ref = _mAddOnRef;
	return _mAddOn;
}



BView *
BMediaTheme::ViewFor(
	BParameterWeb * web,
	const BRect * hintRect,
	BMediaTheme * theme)
{
	BAutolock lock(_themeLock);
	if (theme == NULL) theme = PreferredTheme();
	if (!dcheck(theme != NULL)) return NULL;
	return theme->MakeViewFor(web, hintRect);
}


status_t
BMediaTheme::SetPreferredTheme(
	BMediaTheme * theme)
{
	BAutolock lock(_themeLock);
	if (theme != _mDefaultTheme)
		delete _mDefaultTheme;
	_mDefaultTheme = theme;
	entry_ref the_add_on;
	BPath path;
	status_t err = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (err < B_OK) return err;
	err = path.Append("Media Themes");
	if ((err < B_OK) && (err != ENOENT)) return err;
	if ((theme == NULL) || !theme->GetRef(&the_add_on))
	{
//		In-app theme is only in effect for that app while the app is running.
	}
	else
	{
		FILE * f = fopen(path.Path(), "w");
		if (!f) return B_NOT_ALLOWED;
		PREFS(f, "ID = %d\n", theme->ID());
		/* re-use path variable */
		BEntry ent(&the_add_on);
		err = ent.GetPath(&path);
		if (err < B_OK) return err;
		PREFS(f, "PATH = \"%s\"\n", path.Path());
		fclose(f);
	}
	return B_OK;
}


extern "C" {
	typedef BMediaTheme * (*make_theme_func)(int32, image_id);
	typedef status_t (*get_nth_theme_func)(int32, const char **, const char **, int32 *);
}

	static bool
	assign_int32(
		const char * assign,
		int32 * i)
	{
		while (*assign && isspace(*assign))
			assign++;
		if (*assign != '=') return false;
		while (*assign && isspace(*assign))
			assign++;
		char * out = NULL;
		long temp = strtol(assign, &out, 10);
		if (out == NULL || out == assign) return false;
		*i = temp;
		return true;
	}

	static bool
	assign_str(
		const char * assign,
		char * out)
	{
		char * save = out;
		while (*assign && isspace(*assign))
			assign++;
		if (*assign != '=') return false;
		while (*assign && isspace(*assign))
			assign++;
		bool quote = false;
		if (*assign == '\"') {
			quote = true;
			assign++;
		}
		while (*assign) {
			*(out++) = *(assign++);
		}
		if (out == save) return false;
		if (out[-1] == '\n') out--;
		if (out == save) return false;
		if (quote && (out[-1] == '\"')) out--;
		if (out == save) return false;
		*out = 0;
		return true;
	}


BMediaTheme *
BMediaTheme::PreferredTheme()
{
	BAutolock lock(_themeLock);
	if (_mDefaultTheme != NULL) return _mDefaultTheme;

	/* If not already loaded, read the prefs and try to load what they say */
	BPath path;
	FILE * f = NULL;
	status_t err = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	char line[1100] = "";
	char tmp[1024] = "";
	int32 id = 0;
	int lc = 0;
	if (err < B_OK) goto bad;
	err = path.Append("Media Themes");
	if (err < B_OK) goto bad;
	f = fopen(path.Path(), "r");
	if (!f) goto bad;
	while (!feof(f) && !ferror(f))
	{
		line[0] = 0;
		fgets(line, 1099, f);
		if (!line[0]) break;
		lc++;
		char * lp = line;
		while (*lp && isspace(*lp)) lp++;
		if (lp[0] == '#' || !*lp) continue;
		if (!strncmp(lp, "ID", 2) && assign_int32(&lp[2], &id)) continue;
		if (!strncmp(lp, "PATH", 4) && assign_str(&lp[4], tmp)) continue;
		fprintf(stderr, "Bad syntax line %d of settings/Media Themes\n", lc);
	}
	fclose(f);
	f = NULL;
	if (tmp[0] && !path.SetTo(tmp)) {
		image_id addon = load_add_on(path.Path());
		if (addon < B_OK) goto bad;
		make_theme_func make;
		if (get_image_symbol(addon, "make_theme", B_SYMBOL_TYPE_ANY, (void**)&make) < B_OK) {
			unload_add_on(addon);
			goto bad;
		}
		_mDefaultTheme = (*make)(id, addon);
		if (!_mDefaultTheme) {
			unload_add_on(addon);
			goto bad;
		}
		goto good;
	}
bad:
	/* The cop-out is to load the system theme */
	if (f) fclose(f);
	_mDefaultTheme = new _BMSystemTheme;
good:
	return _mDefaultTheme;
}


BControl *
BMediaTheme::MakeFallbackViewFor(
	BParameter * control)
{
	if (!dcheck(control != NULL)) return NULL;

	switch (control->Type())
	{
	case BParameter::B_NULL_PARAMETER: {
			return NULL;
		} break;
	case BParameter::B_CONTINUOUS_PARAMETER: {
			BContinuousParameter * slider = dynamic_cast<BContinuousParameter *>(control);
			if (!slider) {
				dlog("BParameter lies about its type (%d)", control->Type());
				return NULL;
			}
			char limit_lo[64], limit_hi[64];
			float scale = slider->ValueStep();
			if (scale < 0.0000001) scale = 1.0;
			sprintf(limit_lo, "%g %s", slider->MinValue(), slider->Unit());
			sprintf(limit_hi, "%g %s", slider->MaxValue(), slider->Unit());
			BControl * ret = NULL;
			if (!strcmp(control->Kind(), B_GAIN) ||
					!strcmp(control->Kind(), B_MASTER_GAIN))
			{
				BChannelSlider * foo = new BChannelSlider(BRect(0,0,31,191),
						slider->Name(), slider->Name(), NULL, slider->CountChannels());
				foo->SetLimitLabels(limit_lo, limit_hi);
				for (int ix=0; ix<slider->CountChannels(); ix++)
					foo->SetLimitsFor(ix, 0, (slider->MaxValue()-
						slider->MinValue())/scale);
				ret = foo;
			}
			else if (!strcmp(slider->Kind(), B_BALANCE))
			{
				BSlider * foo = new BSlider(BRect(0,0,60,31), slider->Name(),
					NULL, NULL, 0, slider->MaxValue()/scale - slider->MinValue()/scale,
					B_TRIANGLE_THUMB, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_NAVIGABLE | B_WILL_DRAW |
					B_FRAME_EVENTS);
				foo->SetLimitLabels("L", "R");
				rgb_color bar;
				bar.red = 0;
				bar.green = 0;
				bar.blue = 0;
				bar.alpha = 255;
				foo->SetBarColor(bar);
				foo->UseFillColor(true, &bar);
				ret = foo;
			}
			else
			{
				BSlider * foo = new BSlider(BRect(0,0,100,31), slider->Name(),
					slider->Name(), NULL, 0, slider->MaxValue()/scale - slider->MinValue()/scale,
					B_BLOCK_THUMB, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_NAVIGABLE | B_WILL_DRAW |
					B_FRAME_EVENTS);
				foo->SetLimitLabels(limit_lo, limit_hi);
				ret = foo;
			}
			ret->ResizeToPreferred();
			return ret;
		} break;
	case BParameter::B_DISCRETE_PARAMETER: {
			font_height fh;
			be_plain_font->GetHeight(&fh);
			float h = fh.ascent + fh.descent + fh.leading;
			if (h < 17)
				h = 17;
			BDiscreteParameter * selector = dynamic_cast<BDiscreteParameter *>(control);
			if (!selector) {
				dlog("BParameter lies about its type (%d)", selector->Type());
				return NULL;
			}
			/*** Might switch on Kind() and range here (Mute, RadioButtons, ...) ***/
			if (!strcmp(control->Kind(), B_MUTE))
			{
				BCheckBox * cb = new BCheckBox(
						BRect(0,0,be_plain_font->StringWidth(control->Name())+h,h),
						control->Name(), control->Name(), NULL);
				cb->ResizeToPreferred();
				return cb;
			}
			else if (!strcmp(control->Kind(), B_ENABLE))
			{
				BCheckBox * cb = new BCheckBox(
						BRect(0,0,be_plain_font->StringWidth(control->Name())+h,h),
						control->Name(), control->Name(), NULL);
				cb->ResizeToPreferred();
				return cb;
			}
			else if (!strcmp(control->Kind(), B_TUNER_CHANNEL)) {
				TVChannelControl * tc = new TVChannelControl(
						BRect(0,0,170,80), control->Name(), control->Name(),
						NULL, B_FOLLOW_NONE, B_WILL_DRAW, control->Name());
				for (int ix=0; ix<=selector->CountItems(); ix++) {
					const char * item = (const char *)selector->ItemNameAt(ix);
					if (item != NULL) {
						int32 value = (int32)selector->ItemValueAt(ix);
						tc->AddOption(item, value);
					}
				}
				tc->ResizeToPreferred();
				return tc;
			}
			else
			{
				h += 5;
				float wid = 2*be_plain_font->StringWidth(control->Name());
				if (wid < 140) wid = 140;
				float w = be_plain_font->StringWidth(control->Name()) + 38;
				for (int ix=0; ix<selector->CountItems(); ix++)
				{
					const char * item = (const char *)selector->ItemNameAt(ix);
					if ((item != NULL) && (w + be_plain_font->StringWidth(item) > wid))
						wid = w + be_plain_font->StringWidth(item);
				}
				BOptionPopUp * sel = new BOptionPopUp(
						BRect(0,0,wid,h),
						control->Name(), control->Name(), NULL, false,
						B_FOLLOW_LEFT|B_FOLLOW_TOP, B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE);
				FPRINTF(stderr, "Making popup for kind '%s'\n", control->Kind());
				for (int ix=0; ix<=selector->CountItems(); ix++)
				{
					const char * item = (const char *)selector->ItemNameAt(ix);
					if (item != NULL)
					{
						int32 value = (int32)selector->ItemValueAt(ix);
						sel->AddOption(item, value);
					}
					else
					{
						FPRINTF(stderr, "item %d of control %s is NULL\n",
								ix, control->Name());
					}
				}
//				sel->ResizeToPreferred();
				return sel;
			}
		} break;
	default:
		dlog("Failed FallbackViewFor for type %d\n", control->Type());
		break;
	}
	return NULL;
}


		/* Mmmh, stuffing! */
status_t
BMediaTheme::_Reserved_ControlTheme_0(void *)
{
	return B_ERROR;
}

status_t
BMediaTheme::_Reserved_ControlTheme_1(void *)
{
	return B_ERROR;
}

status_t
BMediaTheme::_Reserved_ControlTheme_2(void *)
{
	return B_ERROR;
}

status_t
BMediaTheme::_Reserved_ControlTheme_3(void *)
{
	return B_ERROR;
}

status_t
BMediaTheme::_Reserved_ControlTheme_4(void *)
{
	return B_ERROR;
}

status_t
BMediaTheme::_Reserved_ControlTheme_5(void *)
{
	return B_ERROR;
}

status_t
BMediaTheme::_Reserved_ControlTheme_6(void *)
{
	return B_ERROR;
}

status_t
BMediaTheme::_Reserved_ControlTheme_7(void *)
{
	return B_ERROR;
}


static BBitmap * s_backgrounds[BMediaTheme::B_HILITE_FG+1];
static const char * const s_bitmap_name[] = {
	"/boot/home/config/media/bitmaps/general_bg",
	"/boot/home/config/media/bitmaps/settings_bg",
	"/boot/home/config/media/bitmaps/presentation_bg",
	"/boot/home/config/media/bitmaps/edit_bg",
	"/boot/home/config/media/bitmaps/control_bg",
	"/boot/home/config/media/bitmaps/hilite_bg"
};
static uchar s_color_index_bg[] = {
	28, 24, 28, 30, 28, 200
};
static uchar s_color_index_fg[] = {
	0, 0, 8, 10, 8, 0
};

BBitmap *
BMediaTheme::BackgroundBitmapFor(
	bg_kind bg)
{
	BAutolock lock(_themeLock);
	if (bg < B_GENERAL_BG) bg = B_GENERAL_BG;
	if (bg > B_HILITE_BG) bg = B_GENERAL_BG;
	if (!s_backgrounds[bg]) {
		s_backgrounds[bg] = BTranslationUtils::GetBitmap(s_bitmap_name[bg]);
		if (!s_backgrounds[bg]) {
			s_backgrounds[bg] = new BBitmap(BRect(0,0,15,7), B_CMAP8);
			memset(s_backgrounds[bg], s_color_index_bg[bg], 16*8);
		}
	}
	return s_backgrounds[bg];
}

rgb_color
BMediaTheme::BackgroundColorFor(
	bg_kind bg)
{
	if (bg < B_GENERAL_BG) bg = B_GENERAL_BG;
	if (bg > B_HILITE_BG) bg = B_GENERAL_BG;
	return BScreen().ColorForIndex(s_color_index_bg[bg]);
}

rgb_color
BMediaTheme::ForegroundColorFor(
	fg_kind fg)
{
	if (fg < B_GENERAL_FG) fg = B_GENERAL_FG;
	if (fg > B_HILITE_FG) fg = B_GENERAL_FG;
	return BScreen().ColorForIndex(s_color_index_fg[fg]);
}



