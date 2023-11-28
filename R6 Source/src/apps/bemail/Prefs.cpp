//--------------------------------------------------------------------
//	
//	Prefs.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1997 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <InterfaceKit.h>
#include <StorageKit.h>
#include <E-mail.h>
#include <Application.h>

#include "Mail.h"
#include "Prefs.h"

#define BUTTON_WIDTH		 70
#define BUTTON_HEIGHT		 20

#define FONT_X1				 20
#define FONT_Y1				 15
#define FONT_X2				(PREF_WIDTH - FONT_X1)
#define FONT_Y2				(FONT_Y1 + 20)
#define FONT_TEXT			"Font:"

#define SIZE_X1				FONT_X1
#define SIZE_Y1				(FONT_Y2 + 10)
#define SIZE_X2				(PREF_WIDTH - SIZE_X1)
#define SIZE_Y2				(SIZE_Y1 + 20)
#define SIZE_TEXT			"Size:"

#define LEVEL_X1			FONT_X1
#define LEVEL_Y1			(SIZE_Y2 + 10)
#define LEVEL_X2			(PREF_WIDTH - LEVEL_X1)
#define LEVEL_Y2			(LEVEL_Y1 + 20)
#define LEVEL_TEXT			"User Level:"

#define WRAP_X1				FONT_X1
#define WRAP_Y1				(LEVEL_Y2 + 10)
#define WRAP_X2				(PREF_WIDTH - WRAP_X1)
#define WRAP_Y2				(WRAP_Y1 + 20)
#define WRAP_TEXT			"Text Wrapping:"

#define SIG_X1				FONT_X1
#define SIG_Y1				(WRAP_Y2 + 10)
#define SIG_X2				(PREF_WIDTH - SIG_X1)
#define SIG_Y2				(SIG_Y1 + 20)
#define SIGNATURE_TEXT		"Auto Signature:"

#define ENC_X1				FONT_X1
#define ENC_Y1				(SIG_Y2 + 10)
#define ENC_X2				(PREF_WIDTH - ENC_X1)
#define ENC_Y2				(ENC_Y1 + 20)
#define ENCODING_TEXT		"Encoding:"

#define BAR_X1				FONT_X1
#define BAR_Y1				(ENC_Y2 + 10)
#define BAR_X2				(PREF_WIDTH - BAR_X1)
#define BAR_Y2				(BAR_Y1 + 20)
#define BUTTONBAR_TEXT		"Button Bar:"

#define OK_BUTTON_X1		(PREF_WIDTH - BUTTON_WIDTH - 6)
#define OK_BUTTON_Y1		(PREF_HEIGHT - (BUTTON_HEIGHT + 10))
#define OK_BUTTON_X2		(OK_BUTTON_X1 + BUTTON_WIDTH)
#define OK_BUTTON_Y2		(OK_BUTTON_Y1 + BUTTON_HEIGHT)
#define OK_BUTTON_TEXT		"Done"

#define REVERT_BUTTON_X1	(10)
#define REVERT_BUTTON_Y1	OK_BUTTON_Y1
#define REVERT_BUTTON_X2	(REVERT_BUTTON_X1 + BUTTON_WIDTH)
#define REVERT_BUTTON_Y2	OK_BUTTON_Y2
#define REVERT_BUTTON_TEXT	"Revert"

enum	P_MESSAGES			{P_OK = 128, P_CANCEL, P_REVERT, P_FONT,
							 P_SIZE, P_LEVEL, P_WRAP, P_SIG, P_ENC, 
							 P_BUTTON_BAR};
								
extern BPoint	prefs_window;

struct EncodingItem {
	char	*name;
	uint32	flavor;
};
const EncodingItem kEncodings[] = {
	// B_MS_WINDOWS is a superset of B_ISO1, MS mailers lie and send Windows chars as ISO-1
	{"ISO-8859-1", B_MS_WINDOWS_CONVERSION},
	{"ISO-8859-2", B_ISO2_CONVERSION},
	{"ISO-8859-3", B_ISO3_CONVERSION},
	{"ISO-8859-4", B_ISO4_CONVERSION},
	{"ISO-8859-5", B_ISO5_CONVERSION},
	{"ISO-8859-6", B_ISO6_CONVERSION},
	{"ISO-8859-7", B_ISO7_CONVERSION},
	{"ISO-8859-8", B_ISO8_CONVERSION},
	{"ISO-8859-9", B_ISO9_CONVERSION},
	{"ISO-8859-10", B_ISO10_CONVERSION},
	{"ISO-2022-JP", B_JIS_CONVERSION},
	{"KOI8-R",		B_KOI8R_CONVERSION},
	{"Windows-1251",B_MS_WINDOWS_1251_CONVERSION},
	{"DOS-866",		B_MS_DOS_866_CONVERSION},
	{"ISO-8859-13", B_ISO13_CONVERSION},
	{"ISO-8859-14", B_ISO14_CONVERSION},
	{"ISO-8859-15", B_ISO15_CONVERSION}
};

//====================================================================

TPrefsWindow::TPrefsWindow(BRect rect, BFont *font, int32 *level,
							bool *wrap, char **sig, uint32 *encoding, bool *buttonBar)
			  :BWindow(rect, "BeMail Preferences", B_TITLED_WINDOW,
									B_NOT_RESIZABLE |
			  						B_NOT_ZOOMABLE)
{
	BBox		*box;
	BFont		menu_font;
	BMenuField	*menu;
	BRect		r;

	fNewFont = font;
	fFont = *fNewFont;
	fNewLevel = level;
	fLevel = *fNewLevel;
	fNewWrap = wrap;
	fWrap = *fNewWrap;
	fNewSignature = sig;
	fNewEncoding = encoding;
	fEncoding = *fNewEncoding;
	fNewButtonBar = buttonBar;
	fButtonBar = *fNewButtonBar;
	fSignature = (char *)malloc(strlen(*fNewSignature) + 1);
	strcpy(fSignature, *fNewSignature);

	r = Bounds();
//	r.InsetBy(-1, -1);
	box = new BBox(r, NULL, B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS | 	
	  B_NAVIGABLE_JUMP, B_PLAIN_BORDER);
	AddChild(box);

	r.Set(OK_BUTTON_X1, OK_BUTTON_Y1, OK_BUTTON_X2, OK_BUTTON_Y2);
	fOK = new BButton(r, "ok", OK_BUTTON_TEXT, new BMessage(P_OK));
	fOK->MakeDefault(true);
	box->AddChild(fOK);

	r.OffsetBy( -(OK_BUTTON_X2 - OK_BUTTON_X1 + 10), 0);
	fCancel = new BButton(r, "cancel", "Cancel", new BMessage(P_CANCEL));
	box->AddChild(fCancel);


	r.Set(REVERT_BUTTON_X1, REVERT_BUTTON_Y1, REVERT_BUTTON_X2, REVERT_BUTTON_Y2);
	fRevert = new BButton(r, "revert", REVERT_BUTTON_TEXT, new BMessage(P_REVERT));
	fRevert->SetEnabled(false);
	box->AddChild(fRevert);

	r.Set(FONT_X1, FONT_Y1, FONT_X2, FONT_Y2);
	fFontMenu = BuildFontMenu(font);
	menu = new BMenuField(r, "font", FONT_TEXT, fFontMenu,
				B_FOLLOW_ALL,
				B_WILL_DRAW |
				B_NAVIGABLE |
				B_NAVIGABLE_JUMP);
	menu->GetFont(&menu_font);
	menu->SetDivider(menu_font.StringWidth(WRAP_TEXT) + 7);
	menu->SetAlignment(B_ALIGN_RIGHT);
	box->AddChild(menu);

	r.Set(SIZE_X1, SIZE_Y1, SIZE_X2, SIZE_Y2);
	fSizeMenu = BuildSizeMenu(font);
	menu = new BMenuField(r, "size", SIZE_TEXT, fSizeMenu,
				B_FOLLOW_ALL,
				B_WILL_DRAW |
				B_NAVIGABLE |
				B_NAVIGABLE_JUMP);
	menu->SetDivider(menu_font.StringWidth(WRAP_TEXT) + 7);
	menu->SetAlignment(B_ALIGN_RIGHT);
	box->AddChild(menu);

	r.Set(LEVEL_X1, LEVEL_Y1, LEVEL_X2, LEVEL_Y2);
	fLevelMenu = BuildLevelMenu(*level);
	menu = new BMenuField(r, "level", LEVEL_TEXT, fLevelMenu,
				B_FOLLOW_ALL,
				B_WILL_DRAW |
				B_NAVIGABLE |
				B_NAVIGABLE_JUMP);
	menu->SetDivider(menu_font.StringWidth(WRAP_TEXT) + 7);
	menu->SetAlignment(B_ALIGN_RIGHT);
	box->AddChild(menu);

	r.Set(WRAP_X1, WRAP_Y1, WRAP_X2, WRAP_Y2);
	fWrapMenu = BuildWrapMenu(*wrap);
	menu = new BMenuField(r, "wrap", WRAP_TEXT, fWrapMenu,
				B_FOLLOW_ALL,
				B_WILL_DRAW |
				B_NAVIGABLE |
				B_NAVIGABLE_JUMP);
	menu->SetDivider(menu_font.StringWidth(WRAP_TEXT) + 7);
	menu->SetAlignment(B_ALIGN_RIGHT);
	box->AddChild(menu);

	r.Set(SIG_X1, SIG_Y1, SIG_X2, SIG_Y2);
	fSignatureMenu = BuildSignatureMenu(*sig);
	menu = new BMenuField(r, "sig", SIGNATURE_TEXT, fSignatureMenu,
				B_FOLLOW_ALL,
				B_WILL_DRAW |
				B_NAVIGABLE |
				B_NAVIGABLE_JUMP);
	menu->SetDivider(menu_font.StringWidth(WRAP_TEXT) + 7);
	menu->SetAlignment(B_ALIGN_RIGHT);
	box->AddChild(menu);

	r.Set(ENC_X1, ENC_Y1, ENC_X2, ENC_Y2);
	fEncodingMenu = BuildEncodingMenu(fEncoding);
	menu = new BMenuField(r, "enc", ENCODING_TEXT, fEncodingMenu,
				B_FOLLOW_ALL,
				B_WILL_DRAW |
				B_NAVIGABLE |
				B_NAVIGABLE_JUMP);
	menu->SetDivider(menu_font.StringWidth(WRAP_TEXT) + 7);
	menu->SetAlignment(B_ALIGN_RIGHT);
	box->AddChild(menu);
	
	r.Set(BAR_X1, BAR_Y1, BAR_X2, BAR_Y2);
	fButtonBarMenu = BuildButtonBarMenu(*buttonBar);
	menu = new BMenuField(r, "bar", BUTTONBAR_TEXT, fButtonBarMenu,
				B_FOLLOW_ALL,
				B_WILL_DRAW |
				B_NAVIGABLE |
				B_NAVIGABLE_JUMP);
	menu->SetDivider(menu_font.StringWidth(WRAP_TEXT) + 7);
	menu->SetAlignment(B_ALIGN_RIGHT);
	box->AddChild(menu);	

	Show();
}

//--------------------------------------------------------------------

TPrefsWindow::~TPrefsWindow()
{
	BMessage	msg(WINDOW_CLOSED);

	prefs_window = Frame().LeftTop();

	msg.AddInt32("kind", PREFS_WINDOW);
	be_app->PostMessage(&msg);
}

//--------------------------------------------------------------------

void TPrefsWindow::MessageReceived(BMessage *msg)
{
	bool		changed;
	bool		revert = true;
	const char	*family;
	const char	*signature;
	const char	*style;
	char		label[256];
	int32		new_size;
	int32		old_size;
	font_family	new_family;
	font_family	old_family;
	font_style	new_style;
	font_style	old_style;
	BMenuItem	*item;
	BMessage	message;

	switch (msg->what) {
		case P_OK:
			be_app->PostMessage( PREFS_CHANGED );
			Quit();
			break;

		case P_CANCEL:
			revert = false;
		case P_REVERT:
			fFont.GetFamilyAndStyle(&old_family, &old_style);
			fNewFont->GetFamilyAndStyle(&new_family, &new_style);
			old_size = (int32) fFont.Size();
			new_size = (int32) fNewFont->Size();
			if ((strcmp(old_family, new_family)) || (strcmp(old_style, new_style)) ||
				(old_size != new_size)) {
				fNewFont->SetFamilyAndStyle(old_family, old_style);
				if (revert) {
					sprintf(label, "%s %s", old_family, old_style);
					item = fFontMenu->FindItem(label);
					item->SetMarked(true);
				}
			
				fNewFont->SetSize(old_size);
				if (revert) {
					sprintf(label, "%d", old_size);
					item = fSizeMenu->FindItem(label);
					item->SetMarked(true);
				}
				message.what = M_FONT;
				be_app->PostMessage(&message);
			}
			*fNewLevel = fLevel;
			*fNewWrap = fWrap;
			
			if (strcmp(fSignature, *fNewSignature)) {
				free(*fNewSignature);
				*fNewSignature = (char *)malloc(strlen(fSignature) + 1);
				strcpy(*fNewSignature, fSignature);
			}

			*fNewEncoding = fEncoding;
			*fNewButtonBar = fButtonBar;
			
			be_app->PostMessage( PREFS_CHANGED );
			
			if (revert) {
				if (fLevel == L_EXPERT)
					strcpy(label, "Expert");
				else
					strcpy(label, "Beginner");
				item = fLevelMenu->FindItem(label);
				if (item)
					item->SetMarked(true);
	
				if (fWrap)
					strcpy(label, "On");
				else
					strcpy(label, "Off");
				item = fWrapMenu->FindItem(label);
				if (item)
					item->SetMarked(true);

				item = fSignatureMenu->FindItem(fSignature);
				if (item)
					item->SetMarked(true);

				for (int index = 0; index < sizeof(kEncodings) / 
					sizeof(kEncodings[0]); index++) {
					if (kEncodings[index].flavor == *fNewEncoding) {
						item = fEncodingMenu->FindItem(kEncodings[index].name);
						if (item)
							item->SetMarked(true);
						break;
					}
				}
			}
			else
				Quit();
			break;

		case P_FONT:
			family=NULL;
			style=NULL;
			int32 family_menu_index;
			if (msg->FindString("font", &family) == B_OK) {
				msg->FindString("style", &style);
				fNewFont->SetFamilyAndStyle(family, style);
				message.what = M_FONT;
				be_app->PostMessage(&message);
			}

			/* grab this little tidbit so we can set the correct Family */
			if(msg->FindInt32("parent_index", &family_menu_index) == B_OK) {
				fFontMenu->ItemAt(family_menu_index)->SetMarked(true);
			}

			break;
		case P_SIZE:
			old_size = (int32) fNewFont->Size();
			msg->FindInt32("size", &new_size);
			if (old_size != new_size) {
				fNewFont->SetSize(new_size);
				message.what = M_FONT;
				be_app->PostMessage(&message);
			}
			break;
		case P_LEVEL:
			msg->FindInt32("level", fNewLevel);
			break;
		case P_WRAP:
			msg->FindBool("wrap", fNewWrap);
			break;
		case P_SIG:
			free(*fNewSignature);
			if (msg->FindString("signature", &signature) == B_NO_ERROR) {
				*fNewSignature = (char *)malloc(strlen(signature) + 1);
				strcpy(*fNewSignature, signature);
			}
			else {
				*fNewSignature = (char *)malloc(strlen(SIG_NONE) + 1);
				strcpy(*fNewSignature, SIG_NONE);
			}
			break;
		case P_ENC:
			msg->FindInt32("encoding", (int32 *)fNewEncoding);
			break;
		case P_BUTTON_BAR:
			msg->FindInt8("bar", (int8 *)fNewButtonBar);
			be_app->PostMessage( PREFS_CHANGED );
			break;

		default:
			BWindow::MessageReceived(msg);
	}
	fFont.GetFamilyAndStyle(&old_family, &old_style);
	fNewFont->GetFamilyAndStyle(&new_family, &new_style);
	old_size = (int32) fFont.Size();
	new_size = (int32) fNewFont->Size();
	changed =	((old_size != new_size) ||
				(fLevel != *fNewLevel) ||
				(fWrap != *fNewWrap) ||
				(strcmp(old_family, new_family)) ||
				(strcmp(old_style, new_style)) ||
				(strcmp(fSignature, *fNewSignature)) ||
				(fEncoding != *fNewEncoding) || 
				(fButtonBar != *fNewButtonBar));
	fRevert->SetEnabled(changed);
}

//--------------------------------------------------------------------

BPopUpMenu *TPrefsWindow::BuildFontMenu(BFont *font)
{

	font_family	def_family;
	font_style	def_style;
	font_family	f_family;
	font_style	f_style;

	BPopUpMenu *menu = new BPopUpMenu("");
	font->GetFamilyAndStyle(&def_family, &def_style);

	int32 family_menu_index=0;
	int family_count = count_font_families();
	for (int family_loop = 0; family_loop < family_count; family_loop++) {
		get_font_family(family_loop, &f_family);
		BMenu *family_menu = new BMenu(f_family);

		int style_count = count_font_styles(f_family);
		for (int style_loop = 0; style_loop < style_count; style_loop++) {
			get_font_style(f_family, style_loop, &f_style);

			BMessage *msg = new BMessage(P_FONT);
			msg->AddString("font", f_family);
			msg->AddString("style", f_style);
			/* we send this to make setting the Family easier when things change */
			msg->AddInt32("parent_index", family_menu_index);

			BMenuItem *item = new BMenuItem(f_style, msg);
			family_menu->AddItem(item);
			if ((strcmp(def_family, f_family) == 0) && (strcmp(def_style, f_style) == 0)) {
				item->SetMarked(true);
			}
			item->SetTarget(this);
		}

		menu->AddItem(family_menu);
		BMenuItem *item = menu->ItemAt(family_menu_index);
		BMessage *msg = new BMessage(P_FONT);
		msg->AddString("font", f_family);
		
		item->SetMessage(msg);
		item->SetTarget(this);
		if (strcmp(def_family, f_family) == 0) { item->SetMarked(true); }
		family_menu_index++;
	}
	return menu;
}

//--------------------------------------------------------------------

BPopUpMenu *TPrefsWindow::BuildLevelMenu(int32 level)
{
	BMenuItem	*item;
	BMessage	*msg;
	BPopUpMenu	*menu;

	menu = new BPopUpMenu("");
	msg = new BMessage(P_LEVEL);
	msg->AddInt32("level", L_BEGINNER);
	menu->AddItem(item = new BMenuItem("Beginner", msg));
	if (level == L_BEGINNER)
		item->SetMarked(true);

	msg = new BMessage(P_LEVEL);
	msg->AddInt32("level", L_EXPERT);
	menu->AddItem(item = new BMenuItem("Expert", msg));
	if (level == L_EXPERT)
		item->SetMarked(true);

	return menu;
}

//--------------------------------------------------------------------

BPopUpMenu *TPrefsWindow::BuildSignatureMenu(char *sig)
{
	char			name[B_FILE_NAME_LENGTH];
	int32			index = 0;
	BEntry			entry;
	BFile			file;
	BMenuItem		*item;
	BMessage		*msg;
	BPopUpMenu		*menu;
	BQuery			query;
	BVolume			vol;
	BVolumeRoster	volume;

	menu = new BPopUpMenu("");
	
	msg = new BMessage(P_SIG);
	msg->AddString("signature", SIG_NONE);
	menu->AddItem(item = new BMenuItem(SIG_NONE, msg));
	if (!strcmp(sig, SIG_NONE))
		item->SetMarked(true);
	
	msg = new BMessage(P_SIG);
	msg->AddString("signature", SIG_RANDOM);
	menu->AddItem(item = new BMenuItem(SIG_RANDOM, msg));
	if (!strcmp(sig, SIG_RANDOM))
		item->SetMarked(true);
	menu->AddSeparatorItem();

	volume.GetBootVolume(&vol);
	query.SetVolume(&vol);
	query.SetPredicate("_signature = *");
	query.Fetch();

	while (query.GetNextEntry(&entry) == B_NO_ERROR) {
		file.SetTo(&entry, O_RDONLY);
		if (file.InitCheck() == B_NO_ERROR) {
			msg = new BMessage(P_SIG);
			file.ReadAttr("_signature", B_STRING_TYPE, 0, name, sizeof(name));
			msg->AddString("signature", name);
			menu->AddItem(item = new BMenuItem(name, msg));
			if (!strcmp(sig, name))
				item->SetMarked(true);
		}
	}
	return menu;
}


//--------------------------------------------------------------------

BPopUpMenu *TPrefsWindow::BuildSizeMenu(BFont *font)
{
	char		label[16];
	int32		loop;
	int32		sizes[] = {9, 10, 12, 14, 18, 24};
	float		size;
	BMenuItem	*item;
	BMessage	*msg;
	BPopUpMenu	*menu;

	menu = new BPopUpMenu("");
	size = font->Size();
	for (loop = 0; loop < sizeof(sizes) / sizeof(int32); loop++) {
		msg = new BMessage(P_SIZE);
		msg->AddInt32("size", sizes[loop]);
		sprintf(label, "%d", sizes[loop]);
		menu->AddItem(item = new BMenuItem(label, msg));
		if (sizes[loop] == (int32)size)
			item->SetMarked(true);
	}
	return menu;
}

//--------------------------------------------------------------------

BPopUpMenu *TPrefsWindow::BuildWrapMenu(bool wrap)
{
	BMenuItem	*item;
	BMessage	*msg;
	BPopUpMenu	*menu;

	menu = new BPopUpMenu("");
	msg = new BMessage(P_WRAP);
	msg->AddBool("wrap", true);
	menu->AddItem(item = new BMenuItem("On", msg));
	if (wrap)
		item->SetMarked(true);

	msg = new BMessage(P_WRAP);
	msg->AddInt32("wrap", false);
	menu->AddItem(item = new BMenuItem("Off", msg));
	if (!wrap)
		item->SetMarked(true);

	return menu;
}

//--------------------------------------------------------------------

BPopUpMenu *TPrefsWindow::BuildEncodingMenu(uint32 encoding)
{
	int32		loop;
	BMenuItem	*item;
	BMessage	*msg;
	BPopUpMenu	*menu;

	menu = new BPopUpMenu("");
	for (loop = 0; loop < sizeof(kEncodings) / sizeof(kEncodings[0]); loop++) {
		msg = new BMessage(P_ENC);
		msg->AddInt32("encoding", kEncodings[loop].flavor);
		menu->AddItem(item = new BMenuItem(kEncodings[loop].name, msg));
		if (encoding == kEncodings[loop].flavor)
			item->SetMarked(true);
	}
	return menu;
}

//--------------------------------------------------------------------

BPopUpMenu *TPrefsWindow::BuildButtonBarMenu(bool show)
{
	BMenuItem	*item;
	BMessage	*msg;
	BPopUpMenu	*menu;
	
	menu = new BPopUpMenu("");
	
	msg = new BMessage(P_BUTTON_BAR);
	msg->AddInt8("bar", 1);
	menu->AddItem(item = new BMenuItem("Show Icons & Labels", msg));
	if (show & 1)
		item->SetMarked(true);
	
	msg = new BMessage(P_BUTTON_BAR);
	msg->AddInt8("bar", 2);
	menu->AddItem(item = new BMenuItem("Show Icons Only", msg));
	if (show & 2)
		item->SetMarked(true);
	
	msg = new BMessage(P_BUTTON_BAR);
	msg->AddInt8("bar", 0);
	menu->AddItem(item = new BMenuItem("Hide", msg));
	if (!show)
		item->SetMarked(true);
	return menu;
}
