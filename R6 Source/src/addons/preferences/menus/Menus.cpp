//******************************************************************************
//
//
//	Copyright 1996, Be Incorporated
//
//******************************************************************************

#define DEBUG 1
#include <Debug.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef _FILE_H
#include <File.h>
#endif
#ifndef _WINDOW_H
#include <Window.h>
#endif
#ifndef _STRING_VIEW_H
#include <StringView.h>
#endif
#ifndef _TEXT_VIEW_H
#include <TextView.h>
#endif
#ifndef _MENU_BAR_H
#include <MenuBar.h>
#endif
#ifndef _MENU_ITEM_H
#include <MenuItem.h>
#endif
#ifndef _BUTTON_H
#include <Button.h>
#endif
#ifndef _RADIO_BUTTON_H
#include <RadioButton.h>
#endif
#ifndef _BOX_H
#include <Box.h>
#endif
#ifndef _SCREEN_H
#include <Screen.h>
#endif
#ifndef _ALERT_H
#include <Alert.h>
#endif
#ifndef _PATH_H
#include <Path.h>
#endif
#ifndef _FIND_DIRECTORY_H
#include <FindDirectory.h>
#endif

#ifndef MENU_COLOR_CONTROL_H
#include "MenuColorControl.h"
#endif

#include <ClassInfo.h>
#include <MenuItem.h>

#include "IconMenuItem.h"

#include <interface_misc.h>


status_t remap_kb(uchar cmd_key, uchar ctl_key,
	bool do_right = false, uchar rcmd_key = 0, uchar ropt_key = 0);

#include "Menus.h"

class TMenuItem : public BMenuItem {
public:
				TMenuItem(BMessage *msg, long style);
virtual	void	DrawContent();
virtual	void	GetContentSize(float *w, float *h);

		long	fStyle;
};

class TMenuBar : public BMenuBar {
public:
				TMenuBar(BRect r, const char *name);
virtual	void	FrameResized(float width, float height);
};

#define LEFT_W_MARGIN	40
#define RIGHT_W_MARGIN	40
#define HORZ_W_MARGIN	(LEFT_W_MARGIN + RIGHT_W_MARGIN)
#define TOP_W_MARGIN	11
#define BOTTOM_W_MARGIN	44
#define VERT_W_MARGIN	(TOP_W_MARGIN + BOTTOM_W_MARGIN)

/*------------------------------------------------------------*/

TMenuBar::TMenuBar(BRect r, const char *name)
	:BMenuBar(r, name, B_FOLLOW_NONE, B_ITEMS_IN_COLUMN, true)
{
	SetFlags(Flags() | B_FRAME_EVENTS);
}


/*------------------------------------------------------------*/

void TMenuBar::FrameResized(float /*width*/, float /*height*/)
{
#if 0
//+	PRINT(("w=%.2f, h=%.2f\n", width, height));

	if (width < 10 || height < 10)
		return;

	BWindow *w = Window();

	// maintain the right margin
	w->ResizeTo(width + HORZ_W_MARGIN, height + VERT_W_MARGIN);
//+	PRINT_OBJECT(Window()->Bounds());
#endif
}

/*------------------------------------------------------------*/
static BMenu *find_menu(BMenuBar *mb, const char *label)
{
	BMenuItem *item = mb->FindItem(label);
	if (!item) {
		char buf[200];
		sprintf(buf, "someone renamed the \"%s\" item", label);
		debugger(buf);
	}
	BMenu *menu = item->Submenu();
	if (!menu) {
		char buf[200];
		sprintf(buf, "Item \"%s\" must have a submenu", label);
		debugger(buf);
	}
	return menu;
}

/*------------------------------------------------------------*/

class FontMenu : public BMenu {
public:
	FontMenu(const BFont *);
	void SetCurrent(const BFont *font);
};

FontMenu::FontMenu(const BFont *font)
	:	BMenu("Font")
{
	font_family	fontFamily;
	font_style fontStyle;
	uint32 flags;

	int32 familyCount = count_font_families();

	font_family selectedFamilyName;
	font_style selectedStyleName;
	font->GetFamilyAndStyle(&selectedFamilyName, &selectedStyleName);

	for (int32 familyIndex = 0; familyIndex < familyCount; familyIndex++)
		if (get_font_family(familyIndex, &fontFamily, &flags) == B_OK) {

			BMenu *submenu = new BMenu(fontFamily);
			bool currentFamily = !strcmp(selectedFamilyName, fontFamily);

			int32 styleCount = count_font_styles(fontFamily);
			for (int32 styleIndex = 0; styleIndex < styleCount; styleIndex++)
				if (get_font_style(fontFamily, styleIndex, &fontStyle, &flags) 
					== B_OK) {

					BMessage *message = new BMessage(CMD_FONT_FACE);
					message->AddString("familyName", fontFamily);
					message->AddString("styleName", fontStyle);

					BMenuItem *item = new BMenuItem(fontStyle, message);
					submenu->AddItem(item);

					// mark if current
					item->SetMarked((currentFamily 
						&& strcmp(selectedStyleName, fontStyle) == 0));

				}

			AddItem(submenu);

			// selecting a family only is a shortcut for selecting the 
			// default style of the family; SetFamilyAndStyle will handle it

			BMessage *message = new BMessage(CMD_FONT_FACE);
			message->AddString("familyName", fontFamily);
			BMenuItem *item = ItemAt(familyIndex);
			item->SetMessage(message);

			// if current family, mark the family item too
			item->SetMarked(currentFamily);
		}
}

void
FontMenu::SetCurrent(const BFont *font)
{
	font_family selectedFamilyName;
	font_style selectedStyleName;
	font->GetFamilyAndStyle(&selectedFamilyName, &selectedStyleName);

	// select the correct menu item and unselect the old one
	int32 itemCount = CountItems();
	for (int32 itemIndex = 0; itemIndex < itemCount; itemIndex++) {

		BMenuItem *item = ItemAt(itemIndex);
		bool currentFamily = (strcmp(item->Label(), selectedFamilyName) == 0);
		item->SetMarked(currentFamily);

		BMenu *subMenu = item->Submenu();
		ASSERT(subMenu);

		if (subMenu){
			int32 subItemCount = subMenu->CountItems();
			for (int32 subItemIndex = 0; subItemIndex < subItemCount; subItemIndex++) {
				BMenuItem *subItem = subMenu->ItemAt(subItemIndex);
				subItem->SetMarked(currentFamily &&
					(strcmp(subItem->Label(), selectedStyleName) == 0));
			}
		}
	}
}

//-----------------------------------------------
const unsigned char _menudata_control_[] = {		/* 17 x 11 pixels */
	0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x13,0xff,0xff,0xff,
	0x3f,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x17,0x13,0xff,0xff,0xff,
	0x3f,0x1c,0x13,0x00,0x00,0x13,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x17,0x13,0xff,0xff,0xff,
	0x3f,0x1c,0x00,0x17,0x17,0x00,0x1c,0x00,0x00,0x00,0x1c,0x00,0x1c,0x1c,0x1c,0x17,0x13,0xff,0xff,0xff,
	0x3f,0x1c,0x00,0x1c,0x1c,0x1c,0x1c,0x1c,0x00,0x1c,0x1c,0x00,0x1c,0x1c,0x1c,0x17,0x13,0xff,0xff,0xff,
	0x3f,0x1c,0x00,0x1c,0x1c,0x1c,0x1c,0x1c,0x00,0x1c,0x1c,0x00,0x1c,0x1c,0x1c,0x17,0x13,0xff,0xff,0xff,
	0x3f,0x1c,0x00,0x17,0x17,0x00,0x1c,0x1c,0x00,0x1c,0x1c,0x00,0x1c,0x1c,0x1c,0x17,0x13,0xff,0xff,0xff,
	0x3f,0x1c,0x13,0x00,0x00,0x13,0x1c,0x1c,0x00,0x1c,0x1c,0x00,0x00,0x00,0x1c,0x17,0x13,0xff,0xff,0xff,
	0x3f,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x17,0x13,0xff,0xff,0xff,
	0x3f,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x13,0xff,0xff,0xff,
	0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0xff,0xff,0xff
};

const unsigned char _menudata_alt_[] = {		/* 17 x 11 pixels */
    0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x13,0x00,0x00,0x00,
    0x3f,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x17,0x13,0x00,0x00,0x00,
    0x3f,0x1c,0x1c,0x13,0x00,0x00,0x13,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x17,0x13,0x00,0x00,0x00,
    0x3f,0x1c,0x1c,0x00,0x1c,0x1c,0x00,0x1c,0x00,0x1c,0x1c,0x00,0x00,0x00,0x1c,0x17,0x13,0x00,0x00,0x00,
    0x3f,0x1c,0x1c,0x00,0x1c,0x1c,0x00,0x1c,0x00,0x1c,0x1c,0x1c,0x00,0x1c,0x1c,0x17,0x13,0x00,0x00,0x00,
    0x3f,0x1c,0x1c,0x00,0x00,0x00,0x00,0x1c,0x00,0x1c,0x1c,0x1c,0x00,0x1c,0x1c,0x17,0x13,0x00,0x00,0x00,
    0x3f,0x1c,0x1c,0x00,0x1c,0x1c,0x00,0x1c,0x00,0x1c,0x1c,0x1c,0x00,0x1c,0x1c,0x17,0x13,0x00,0x00,0x00,
    0x3f,0x1c,0x1c,0x00,0x1c,0x1c,0x00,0x1c,0x00,0x00,0x00,0x1c,0x00,0x1c,0x1c,0x17,0x13,0x00,0x00,0x00,
    0x3f,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x17,0x13,0x00,0x00,0x00,
    0x3f,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x13,0x00,0x00,0x00,
    0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x00,0x00,0x00
};

const unsigned char _menudata_cmd_sym_[] = {		/* 17 x 11 pixels */
	0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,
	0x13,0xff,0xff,0xff,
	0x3f,0x1c,0x1c,0x1c,0x13,0x00,0x13,0x1c,0x1c,0x13,0x00,0x13,0x1c,0x1c,0x1c,0x17,
	0x13,0xff,0xff,0xff,
	0x3f,0x1c,0x1c,0x1c,0x00,0x1c,0x00,0x1c,0x1c,0x00,0x1c,0x00,0x1c,0x1c,0x1c,0x17,
	0x13,0xff,0xff,0xff,
	0x3f,0x1c,0x1c,0x1c,0x13,0x00,0x00,0x00,0x00,0x00,0x00,0x13,0x1c,0x1c,0x1c,0x17,
	0x13,0xff,0xff,0xff,
	0x3f,0x1c,0x1c,0x1c,0x1c,0x1c,0x00,0x1c,0x1c,0x00,0x1c,0x1c,0x1c,0x1c,0x1c,0x17,
	0x13,0xff,0xff,0xff,
	0x3f,0x1c,0x1c,0x1c,0x1c,0x1c,0x00,0x1c,0x1c,0x00,0x1c,0x1c,0x1c,0x1c,0x1c,0x17,
	0x13,0xff,0xff,0xff,
	0x3f,0x1c,0x1c,0x1c,0x13,0x00,0x00,0x00,0x00,0x00,0x00,0x13,0x1c,0x1c,0x1c,0x17,
	0x13,0xff,0xff,0xff,
	0x3f,0x1c,0x1c,0x1c,0x00,0x1c,0x00,0x1c,0x1c,0x00,0x1c,0x00,0x1c,0x1c,0x1c,0x17,
	0x13,0xff,0xff,0xff,
	0x3f,0x1c,0x1c,0x1c,0x13,0x00,0x13,0x1c,0x1c,0x13,0x00,0x13,0x1c,0x1c,0x1c,0x17,
	0x13,0xff,0xff,0xff,
	0x3f,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,
	0x13,0xff,0xff,0x3f,
	0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,
	0x13,0xff,0xff,0xff,
};

MenusView::MenusView()
 : BView(BRect(0, 0, 350, 310), "menus", B_FOLLOW_NONE, B_WILL_DRAW)
{
	BRect			wbounds;
	BRect			b;
	BMenu			*menu;
	BMenuItem		*item;

	fColorWindow = NULL;
	wbounds = Bounds();
	b = wbounds;

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	b.top += TOP_W_MARGIN;
	b.left += LEFT_W_MARGIN;
	b.right -= RIGHT_W_MARGIN;
	b.bottom -= BOTTOM_W_MARGIN;

	fMenuBar = new TMenuBar(b, "MainMenu");
	AddChild(fMenuBar);

	get_menu_info(&fMenuInfo);
	memcpy(&fInitMenuInfo, &fMenuInfo, sizeof(menu_info));

	/*
	 Menu for setting the Font Face
	*/
	BFont initialFont;
	initialFont.SetFamilyAndStyle(fMenuInfo.f_family, fMenuInfo.f_style);
	menu = new FontMenu(&initialFont);

	fMenuBar->AddItem(menu);

	/*
	 Menu for setting the Font size
	*/
	menu = new BMenu("Font Size");
	fMenuBar->AddItem(menu);
	menu->SetRadioMode(true);
	menu->AddItem(item = new BMenuItem("9", new BMessage(CMD_FONT_SIZE)));
	menu->AddItem(item = new BMenuItem("10", new BMessage(CMD_FONT_SIZE)));
	menu->AddItem(item = new BMenuItem("11", new BMessage(CMD_FONT_SIZE)));
	menu->AddItem(item = new BMenuItem("12", new BMessage(CMD_FONT_SIZE)));
	menu->AddItem(item = new BMenuItem("14", new BMessage(CMD_FONT_SIZE)));
	menu->AddItem(item = new BMenuItem("18", new BMessage(CMD_FONT_SIZE)));

	fMenuBar->AddSeparatorItem();

	/*
 	 for turning on/off the stickiness of menus
	*/
	fMenuBar->AddItem(item = new BMenuItem("Click to Open",
							new BMessage(CMD_ALLOW_STICKY)));

	// to force the menu to have space for check marks.
	// UpdateMenu will remove the check it approrpriate.
	item->SetMarked(true);

	/*
	 Menu for showing trigger chars all the time, or only in sticky mode
	*/
	fMenuBar->AddItem(item = new BMenuItem("Always Show Triggers",
							new BMessage(CMD_PERMANENT_TRIGGER)));

	fMenuBar->AddSeparatorItem();

	// duncan: color scheme disabled for Maui
	/*
	 Menu for setting the the background color
	*/
	fMenuBar->AddItem(new BMenuItem("Color Scheme...",
							new BMessage(CMD_BACKGROUND_COLOR)));

	/*
	 Menu for setting the 'separator' style
	*/
	menu = new BMenu("Separator Style");
	fMenuBar->AddItem(menu);
	menu->SetRadioMode(true);
	menu->AddItem(item = new TMenuItem(new BMessage(CMD_SEPARATOR_STYLE), 0));
	menu->AddItem(item = new TMenuItem(new BMessage(CMD_SEPARATOR_STYLE), 1));
	menu->AddItem(item = new TMenuItem(new BMessage(CMD_SEPARATOR_STYLE), 2));

	fInitMacStyleShortcut = false;
	fInitWinStyleShortcut = false;
	fShowShortcutItems = true;

	if (fShowShortcutItems) {
		fMenuBar->AddSeparatorItem();

		/*
		 Menu for setting the keyboard shortcut char
		*/
		key_map		*map;
		char		*kbuf;
		system_info	info;


		get_key_map(&map, &kbuf);

		get_system_info(&info);

		bool	mac_plat = (info.platform_type == B_MAC_PLATFORM);
		bool	mac_style;
		bool	win_style;

		mac_style = (map->left_control_key==0x5c) &&
					(map->left_command_key==0x5d);
		win_style = (map->left_control_key==0x5d) &&
					(map->left_command_key==0x5c);
		fInitMacStyleShortcut = mac_style;
		fInitWinStyleShortcut = win_style;
		fInitCmdKey = map->left_command_key;
		fInitCtlKey = map->left_control_key;
		fInitRCmdKey = map->right_command_key;
		fInitROptKey = map->right_option_key;

		BBitmap	*alt_bm;
		BBitmap	*ctl_bm;
		alt_bm = new BBitmap(BRect(0,0,17-1,10), B_COLOR_8_BIT);
		alt_bm->SetBits(mac_plat ? _menudata_cmd_sym_ : _menudata_alt_,
			alt_bm->BitsLength(), 0, B_COLOR_8_BIT);
		ctl_bm = new BBitmap(BRect(0,0,17-1,10), B_COLOR_8_BIT);
		ctl_bm->SetBits(_menudata_control_, ctl_bm->BitsLength(), 0,
			B_COLOR_8_BIT);

		fWinStyleItem = new TIconMenuItem(ctl_bm, "as Shortcut Key",
			new BMessage(CMD_WIN_STYLE_SHORTCUT));
		fMenuBar->AddItem(fWinStyleItem);
		fWinStyleItem->SetMarked(win_style);

		fMacStyleItem = new TIconMenuItem(alt_bm, "as Shortcut Key",
			new BMessage(CMD_MAC_STYLE_SHORTCUT));
		fMenuBar->AddItem(fMacStyleItem);
		fMacStyleItem->SetMarked(mac_style);

		free(map);
		free(kbuf);
	}

	/* -------------------- */
	
	UpdateMenu(&fMenuInfo);

	BButton	*but;

	b.left = wbounds.left + 5;
	b.right = b.left + 75;
	b.top = wbounds.bottom - 25;
	b.bottom = b.top + 25;

	but = new BButton(b, "", "Defaults", new BMessage(CMD_DEFAULTS),
		B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
//	but->ResizeToPreferred();
//	but->MoveTo(wbounds.left+10, wbounds.bottom - 10 - but->Bounds().Height());
	AddChild(but);
	dflt_btn = but;

//+	PRINT_OBJECT(but->Frame());
	b = but->Frame();
	
	b.OffsetBy(but->Bounds().Width() + 10, 0);

//+	PRINT_OBJECT(b);
	but = new BButton(b, "", "Revert", new BMessage(CMD_REVERT),
		B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	AddChild(but);
	rev_btn = but;

	but->SetEnabled(false);
	fRevert = but;
}

/*------------------------------------------------------------*/
void MenusView::AttachedToWindow()
{
	dflt_btn->SetTarget(this);
	rev_btn->SetTarget(this);
	MakeTargets(fMenuBar, this);
}

void MenusView::MakeTargets(BMenu *menu, BMessenger target)
{
	menu->SetTargetForItems(target);
	int32 max = menu->CountItems();
	for(int32 i = 0; i < max; i++)
	{
		BMenu *m = menu->SubmenuAt(i);
		if(m)
			MakeTargets(m, target);
	}
}

/*------------------------------------------------------------*/
status_t remap_kb(uchar cmd_key, uchar ctl_key,
	bool do_right, uchar rcmd_key, uchar ropt_key)
{
	char	buf[100];
	FILE	*fd;
	size_t	r;

	sprintf(buf, "keymap -l");
	fd = popen(buf, "w");
	if (fd == NULL) {
		printf("error in popen()\n");
		return B_ERROR;
	}

	sprintf(buf, "LCommand = 0x%x\n", cmd_key);
	r = fwrite(buf, sizeof(char), strlen(buf), fd);
//+	printf("fwrite = %d\n", r);
	sprintf(buf, "LControl = 0x%x\n", ctl_key);
	r = fwrite(buf, sizeof(char), strlen(buf), fd);
//+	printf("fwrite = %d\n", r);

	if (do_right) {
		sprintf(buf, "RCommand = 0x%x\n", rcmd_key);
		r = fwrite(buf, sizeof(char), strlen(buf), fd);
//+		printf("fwrite = %d\n", r);
		sprintf(buf, "ROption = 0x%x\n", ropt_key);
		r = fwrite(buf, sizeof(char), strlen(buf), fd);
//+		printf("fwrite = %d\n", r);
	}

	fclose(fd);
	return B_OK;
}


/*------------------------------------------------------------*/

void	recursive_bgcolor(BMenu *menu, rgb_color color)
{
	menu->SetViewColor(color);
	
	int32	c = menu->CountItems();
	while (--c >= 0) {
		BMenu	*sub = menu->SubmenuAt(c);
		if (sub) {
			recursive_bgcolor(sub, color);
		}
	}

}

/*------------------------------------------------------------*/

void MenusView::MessageReceived(BMessage *msg)
{
	if (msg->WasDropped()) {
		rgb_color *color;
		long size;
		if (msg->FindData("RGBColor", 'RGBC', (const void**) &color, (ssize_t*) 
			&size) == B_NO_ERROR) {
			BMessage	m(CMD_COLOR_CHANGE);
			m.AddData("RGBColor", 'RGBC', color, size);
			MessageReceived(&m);
			return;
		}
#if 0
		rgb_color *color;
		long size;
		if (msg->FindData("RGBColor", 'RGBC', &color, &size) == B_NO_ERROR) {
			fMenuInfo.background_color = *color;
			fMenuBar->SetViewColor(fMenuInfo.background_color);
			set_menu_info(&fMenuInfo);
			CheckDirty();
			fMenuBar->Invalidate();

			if (fColorWindow && fColorWindow->Lock()) {
				BButton *but = (BButton *) fColorWindow->FindView("revert");
				rgb_color c = fMenuInfo.background_color;
				if (c.red != fRevertColor.red ||
					c.green != fRevertColor.green ||
					c.blue != fRevertColor.blue)
						but->SetEnabled(true);
				else
						but->SetEnabled(false);
				fColorWindow->Unlock();
			}
			return;
		}
#endif
	}
	switch (msg->what) {
		case CMD_BACKGROUND_COLOR:
			{
			if (fColorWindow && fColorWindow->Lock()) {
				fColorWindow->Activate();
				fColorWindow->Unlock();
				break;
			}

			BRect	screen_r = BScreen().Frame();
			screen_r.bottom -= 50;
			screen_r.right -= 50;
			BPoint pt = Frame().LeftTop();
			pt.x += 50;
			pt.y += 50;
			if (pt.x > screen_r.right)
				pt.x = screen_r.right;
			if (pt.y > screen_r.bottom)
				pt.y = screen_r.bottom;

			fColorWindow = NULL;
			BRect	r(pt.x,pt.y,pt.x+320,pt.y+80);
			BWindow	*dlg = new BWindow(r, "Menu Color Scheme",
				B_TITLED_WINDOW,
				B_NOT_RESIZABLE | B_NOT_ZOOMABLE);

			dlg->Lock();
			r.OffsetTo(B_ORIGIN);

//+	BBox *topv = new BBox(b, "top", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS,
//+		B_PLAIN_BORDER);
//+	AddChild(topv);

			BView *v = new BBox(r, "topv", B_FOLLOW_ALL, B_WILL_DRAW,
				B_PLAIN_BORDER);
			dlg->AddChild(v);
			v->SetViewColor(216, 216, 216, 0);

			BColorControl	*cc = new TMenuColorControl(BPoint(11,11),
				B_CELLS_32x8, 9, "ColorControl",
				new BMessage(CMD_COLOR_CHANGE), false, this);
			v->AddChild(cc);
			cc->SetTarget(this);
			rgb_color orig = fMenuInfo.background_color;
			orig.alpha = 255;
			cc->SetValue(orig);
			fRevertColor = fMenuInfo.background_color;

			BButton *but;

			r = cc->Frame();
			r.left -= 1;
			r.top = r.bottom + 10;
			r.bottom = r.top + 18;
			r.right = r.left + 75;
			but = new BButton(r, "default", "Default",
				new BMessage(CMD_DEFAULT_COLOR));
			v->AddChild(but);
			but->SetTarget(this);

//+			r = cc->Frame();
//+			r.left = r.right - 75;
//+			r.top = r.bottom + 10;
//+			r.bottom = r.top + 18;
			r.OffsetBy(but->Frame().Width() + 9, 0);
			but = new BButton(r, "revert", "Revert",
				new BMessage(CMD_REVERT_COLOR));
			v->AddChild(but);
			but->SetTarget(this);
			but->SetEnabled(false);

//+			dlg->ResizeTo(r.right + 10, r.bottom + 10);
			dlg->ResizeTo(cc->Frame().right + 10, but->Frame().bottom + 10);

			dlg->Show();
			fColorWindow = dlg;
			dlg->Unlock();
			break;
			}
		case CMD_REVERT_COLOR:
			RevertColor();
			break;
		case CMD_TMP_COLOR_CHANGE:
			{
//+			PRINT(("CMD_TMP_COLOR_CHANGE\n"));
			long color = msg->FindInt32("color");
			ASSERT(fColorWindow);
			rgb_color	rgb;

			rgb.red = (color >> 24);
			rgb.green = (color >> 16);
			rgb.blue = (color >> 8);
			rgb.alpha = 0;

			fMenuInfo.background_color = rgb;
//+			fMenuBar->SetViewColor(fMenuInfo.background_color);
			recursive_bgcolor(fMenuBar, fMenuInfo.background_color);
			set_menu_info(&fMenuInfo);
			fMenuBar->Invalidate();
			break;
			}
		case CMD_COLOR_CHANGE:
			{
//+			PRINT(("CMD_COLOR_CHANGE\n"));
			BColorControl	*cc = NULL;
			void			*ptr;
			BArchivable		*obj;
			bool			post_revert;
			msg->FindPointer("source", &ptr);
			rgb_color	color;
			if (ptr) {
				obj = (BArchivable *) ptr;
				cc = cast_as(obj, BColorControl);
				ASSERT(cc);
				ASSERT(fColorWindow);
				color = cc->ValueAsColor();
			} else {
				ASSERT(msg->HasData("RGBColor", 'RGBC'));
				rgb_color	*cp;
				long		size;
				msg->FindData("RGBColor", 'RGBC', (const void**)&cp, 
				  (ssize_t*)&size);
				color = *cp;
			}
			post_revert = (ptr != NULL);


			int32		thresh = 0;

			thresh = (color.red + color.green + color.blue) / 3;
			
			if (thresh <= 100) {
				BAlert	*a = new BAlert("",
					"That color is too dark for the menu background.",
					"Cancel Change", NULL, NULL,
					B_WIDTH_FROM_WIDEST, B_STOP_ALERT);
				a->Go();
				if (post_revert)
					RevertColor();
				break;
			}

			fMenuInfo.background_color = color;

			recursive_bgcolor(fMenuBar, fMenuInfo.background_color);

//+			fMenuBar->SetViewColor(fMenuInfo.background_color);
//+			
//+			int32	c = fMenuBar->CountItems();
//+			while (--c >= 0) {
//+				BMenu	*sub = fMenuBar->SubmenuAt(c);
//+				if (sub) {
//+					sub->SetViewColor(fMenuInfo.background_color);
//+				}
//+			}

			set_menu_info(&fMenuInfo);
			CheckDirty();
			fMenuBar->Invalidate();

			if (fColorWindow && fColorWindow->Lock()) {
				BButton *but = (BButton *) fColorWindow->FindView("revert");
				rgb_color c = fMenuInfo.background_color;
				if (c.red != fRevertColor.red ||
					c.green != fRevertColor.green ||
					c.blue != fRevertColor.blue)
						but->SetEnabled(true);
				else
						but->SetEnabled(false);

				if (!cc) {
					cc = (BColorControl*)fColorWindow->FindView("ColorControl");
					cc->SetValue(c);
				}
					
				fColorWindow->Unlock();
			}
			break;
			}

		case CMD_FONT_FACE:
			{
			BFont aFont;
			
			char *familyName;
			char *styleName = NULL;
	
			if (msg->FindString("familyName", (const char**) &familyName) == B_OK){
				msg->FindString("styleName", (const char**) &styleName);

				aFont.SetFamilyAndStyle(familyName, styleName);
				aFont.GetFamilyAndStyle(&fMenuInfo.f_family,&fMenuInfo.f_style);
				set_menu_info(&fMenuInfo);
				CheckDirty();
				InvalMenu(fMenuBar);
			}
			break;
			}
		case CMD_FONT_SIZE:
			{
			BMenuItem *item;
			void		*ptr;
			BArchivable *obj;
			msg->FindPointer("source", &ptr);
			obj = (BArchivable *) ptr;
			item = cast_as(obj, BMenuItem);

			ASSERT(item);
			long size = atoi(item->Label());
			fMenuInfo.font_size = size;
			set_menu_info(&fMenuInfo);
			CheckDirty();
			InvalMenu(fMenuBar);
			break;
			}
		case CMD_ALLOW_STICKY:
			{
			BMenuItem *item;
			void		*ptr;
			BArchivable *obj;
			msg->FindPointer("source", &ptr);
			obj = (BArchivable *) ptr;
			item = cast_as(obj, BMenuItem);
			ASSERT(item);
			item->SetMarked(!item->IsMarked());
			fMenuInfo.click_to_open = item->IsMarked();
			set_menu_info(&fMenuInfo);

			BMenuItem *trig = fMenuBar->FindItem("Always Show Triggers");
			ASSERT(trig);
			trig->SetEnabled(fMenuInfo.click_to_open);
			CheckDirty();
			if (fMenuInfo.triggers_always_shown)
				fMenuBar->Invalidate();
			else
				fMenuBar->Invalidate(item->Frame());
			break;
			}
		case CMD_PERMANENT_TRIGGER:
			{
			BMenuItem *item;
			void		*ptr;
			BArchivable *obj;
			msg->FindPointer("source", &ptr);
			obj = (BArchivable *) ptr;
			item = cast_as(obj, BMenuItem);
			ASSERT(item);
			item->SetMarked(!item->IsMarked());
			fMenuInfo.triggers_always_shown = item->IsMarked();
			set_menu_info(&fMenuInfo);
			CheckDirty();
			if (fMenuInfo.click_to_open) {
				fMenuBar->Invalidate();
			} else
				fMenuBar->Invalidate(item->Frame());
			break;
			}
		case CMD_DEFAULTS:
			{
			fMenuInfo.font_size = 12;
			fMenuInfo.background_color.red = 
				fMenuInfo.background_color.green = 
				fMenuInfo.background_color.blue = 219;

			fMenuInfo.separator = 0;
			fMenuInfo.click_to_open = true;
			fMenuInfo.triggers_always_shown = false;

			be_plain_font->GetFamilyAndStyle(&fMenuInfo.f_family, 
											 &fMenuInfo.f_style);
//+			PRINT(("family=%s, style=%s\n",
//+				fMenuInfo.f_family, fMenuInfo.f_style));

			set_menu_info(&fMenuInfo);
			UpdateMenu(&fMenuInfo);

			if (fShowShortcutItems) {
				system_info info;
				get_system_info(&info);
				bool	power_mac = (info.platform_type == B_MAC_PLATFORM);
				
				// decided to use mac_style as default on all platforms (#12276)
//+				bool	mac_style = (info.platform_type == B_MAC_PLATFORM) ||
//+					(info.platform_type == B_BEBOX_PLATFORM);
				bool	mac_style = true;

				bool	dirty = false;
				if (fMacStyleItem->IsMarked() != mac_style) {
					fMacStyleItem->SetMarked(mac_style);
					dirty = true;
				}
				if (fWinStyleItem->IsMarked() != !mac_style){
					fWinStyleItem->SetMarked(!mac_style);
					dirty = true;
				}
				if (dirty) {
					if (power_mac) {
						remap_kb(0x5d, 0x5c);
					} else {
						remap_kb(mac_style ? 0x5d:0x5c, mac_style ? 0x5c:0x5d,
							true, mac_style ? 0x5f:0x60, mac_style ? 0x60:0x5f);
					}
				}
			}

			CheckDirty();
			InvalMenu(fMenuBar);

			break;
			}
		case CMD_REVERT:
			{
			memcpy(&fMenuInfo, &fInitMenuInfo, sizeof(menu_info));
			set_menu_info(&fMenuInfo);
			UpdateMenu(&fMenuInfo);
			InvalMenu(fMenuBar);
			fRevert->SetEnabled(false);

			if (fColorWindow && fColorWindow->Lock()) {
				BColorControl *cc = (BColorControl *)
										fColorWindow->FindView("ColorControl");
				ASSERT(cc);
				BButton *but = (BButton *) fColorWindow->FindView("revert");
				ASSERT(but);

				cc->SetValue(fRevertColor);
				but->SetEnabled(false);
				fColorWindow->Unlock();
			}

			if (fShowShortcutItems) {
				bool		dirty = false;
				system_info info;

				get_system_info(&info);
				if (fMacStyleItem->IsMarked() != fInitMacStyleShortcut) {
					fMacStyleItem->SetMarked(fInitMacStyleShortcut);
					dirty = true;
				}
				if (fWinStyleItem->IsMarked() != fInitWinStyleShortcut){
					fWinStyleItem->SetMarked(fInitWinStyleShortcut);
					dirty = true;
				}
				if (dirty) {
					remap_kb(fInitCmdKey, fInitCtlKey, true,
						fInitRCmdKey, fInitROptKey);
				}
			}

			break;
			}
		case CMD_SEPARATOR_STYLE:
			{
			BMenuItem *item;
			void *ptr;
			msg->FindPointer("source", &ptr);
			item = dynamic_cast<BMenuItem *>((BArchivable *)ptr);
			ASSERT(item);
			long style = msg->FindInt32("style");
			fMenuInfo.separator = style;
			set_menu_info(&fMenuInfo);
			CheckDirty();

			// only need to invalidate the separator items
			for (long i = 0; (item = fMenuBar->ItemAt(i)) != 0; i++) {
				if (dynamic_cast<BSeparatorItem *>(item))
					fMenuBar->Invalidate(item->Frame());
			}
			break;
			}
		case CMD_DEFAULT_COLOR:
			{
			ASSERT(fColorWindow);
			BColorControl *cc = (BColorControl *)
				fColorWindow->FindView("ColorControl");
			ASSERT(cc);

			rgb_color dc = {219, 219, 219, 0};

			fMenuInfo.background_color = dc;
//+			fMenuBar->SetViewColor(fMenuInfo.background_color);
			recursive_bgcolor(fMenuBar, fMenuInfo.background_color);
			set_menu_info(&fMenuInfo);
			CheckDirty();
			fMenuBar->Invalidate();

			if (fColorWindow->Lock()) {
				cc->SetValue(dc);
				BButton *but = (BButton *) fColorWindow->FindView("revert");
				if (dc.red != fRevertColor.red ||
					dc.green != fRevertColor.green ||
					dc.blue != fRevertColor.blue)
						but->SetEnabled(true);
				else
						but->SetEnabled(false);
				fColorWindow->Unlock();
			}
			break;
			}
		case CMD_MAC_STYLE_SHORTCUT:
		case CMD_WIN_STYLE_SHORTCUT: {
			bool		mac_style = (msg->what == CMD_MAC_STYLE_SHORTCUT);
			BMenuItem	*item = mac_style ? fMacStyleItem : fWinStyleItem;
			bool		force = false;
			status_t	err;

			system_info info;
			get_system_info(&info);
			bool	power_mac = (info.platform_type == B_MAC_PLATFORM);

			if (item->IsMarked())
				break;

			msg->FindBool("force", &force);
			if (!force &&
				!fMacStyleItem->IsMarked() && !fWinStyleItem->IsMarked()) {
				// neither style is currently selected. So we're looking
				// at a custom layout. Give the person a warning.
				BAlert	*a = new BAlert("", "Warning: you seem to have done a custom remapping of your modifier keys, including the Control and Alt keys. If you continue those changes will be overwritten.",
					"Cancel", "Continue", NULL,
					B_WIDTH_FROM_WIDEST, B_OFFSET_SPACING, B_WARNING_ALERT);
				int32	s = a->Go();
				if (s == 0)				// Cancel
					break;
			}

			if (power_mac) {
				err = remap_kb(mac_style ? 0x5d:0x5c, mac_style ? 0x5c:0x5d);
			} else  {
				err = remap_kb(mac_style ? 0x5d:0x5c, mac_style ? 0x5c:0x5d,
						true, mac_style ? 0x5f:0x60, mac_style ? 0x60:0x5f);
			}

			if (err) {
				BAlert	*alert = new BAlert("", "Command failed!", "Sorry",
					NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
				alert->Go(NULL);
				break;
			}
			item->SetMarked(!item->IsMarked());
			if (mac_style) {
				fWinStyleItem->SetMarked(false);
			} else {
				fMacStyleItem->SetMarked(false);
			}

			set_modifier_key(B_LEFT_COMMAND_KEY, mac_style ? 0x5d : 0x5c);
			set_modifier_key(B_LEFT_CONTROL_KEY, mac_style ? 0x5c : 0x5d);
			fMenuBar->Invalidate(fWinStyleItem->Frame());
			fMenuBar->Invalidate(fMacStyleItem->Frame());
			CheckDirty();
			break;
		}
		default:
			BView::MessageReceived(msg);
			break;
	}
}

void MenusView::RevertColor()
{
//+			PRINT(("CMD_REVERT_COLOR\n"));
	BColorControl *cc = NULL;
	if (fColorWindow) {
		cc = (BColorControl *) fColorWindow->FindView("ColorControl");
		ASSERT(cc);
	}

	fMenuInfo.background_color = fRevertColor;
	fMenuBar->SetViewColor(fMenuInfo.background_color);
	set_menu_info(&fMenuInfo);
	CheckDirty();
	fMenuBar->Invalidate();

	if (fColorWindow && fColorWindow->Lock()) {
		cc->SetValue(fRevertColor);
		BButton *but = (BButton *) fColorWindow->FindView("revert");
		if (but)
			but->SetEnabled(false);
		fColorWindow->Unlock();
	}
}

/*------------------------------------------------------------*/

void MenusView::CheckDirty()
{
	bool dirty = memcmp(&fInitMenuInfo, &fMenuInfo, sizeof(menu_info)) != 0;
	if (!dirty && fShowShortcutItems) {
		if (fMacStyleItem->IsMarked() != fInitMacStyleShortcut)
			dirty = true;
		else if (fWinStyleItem->IsMarked() != fInitWinStyleShortcut)
			dirty = true;
	}
	fRevert->SetEnabled(dirty);
}

/*------------------------------------------------------------*/

void MenusView::UpdateMenu(menu_info *minfo)
{
	BMenu		*menu;

	/* ------------------ */
	FontMenu *fontMenu = dynamic_cast<FontMenu *>(find_menu(fMenuBar, "Font"));
	ASSERT(fontMenu);
	
	// check the default font name
	BFont defaultFont;
	defaultFont.SetFamilyAndStyle(minfo->f_family, minfo->f_style);
	fontMenu->SetCurrent(&defaultFont);
	/* ------------------ */
	menu = find_menu(fMenuBar, "Font Size");

	// check the default font size
	char	str[10];
	long	s = (long) minfo->font_size;
	sprintf(str, "%li", s);
	BMenuItem *item = menu->FindItem(str);
	if (item)
		item->SetMarked(true);

	/* ------------------ */
	item = fMenuBar->FindItem("Click to Open");
	if (!item) {
		debugger("someone renamed the \"Click to Open\" item");
	}

	item->SetMarked(minfo->click_to_open);

	/* ------------------ */
	item = fMenuBar->FindItem("Always Show Triggers");
	if (!item) {
		debugger("someone renamed the \"Always Show Triggers\" item");
	}

	item->SetMarked(minfo->triggers_always_shown);
	item->SetEnabled(minfo->click_to_open);

	/* ------------------ */
	menu = find_menu(fMenuBar, "Separator Style");

	item = menu->ItemAt(minfo->separator);
	if (item)
		item->SetMarked(true);
}

/*------------------------------------------------------------*/

bool MenusView::QuitRequested()
{
	BPath	path;

	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append("UI_settings");
		BFile	file(path.Path(), O_RDWR);
		if (file.InitCheck() == B_NO_ERROR) {
			BRect r(Frame());
			file.WriteAttr("_menu_wd_r", B_RAW_TYPE, 0, &r, sizeof(BRect));
		}
	}
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

/*------------------------------------------------------------*/
TMenuItem::TMenuItem(BMessage *msg, long style)
	: BMenuItem("", msg)
{
	msg->AddInt32("style", style);
	fStyle = style;
}

/*------------------------------------------------------------*/
void TMenuItem::GetContentSize(float *w, float *h)
{
	*w = 50;
	*h = 8;
}

/*------------------------------------------------------------*/
void TMenuItem::DrawContent()
{
	BRect		b = Frame();
	BMenu		*parent = Menu();
	ASSERT((parent));
	long		y = (long)((b.top + (b.bottom - b.top) / 2));
	BPoint		loc = parent->PenLocation();

	loc.y = y;

	if (fStyle > 0) {
		loc.x += 10;
		b.right -= 10;
	}
	menu_info minfo;
	get_menu_info(&minfo);

	parent->SetHighColor(shift_color(minfo.background_color, 1.1));
	parent->StrokeLine(loc, BPoint(b.right, loc.y));
	if (fStyle == 2) {
		loc.y++;
		loc.x++;
		b.right--;
		parent->StrokeLine(loc, BPoint(b.right, loc.y));
	}
	parent->SetHighColor(255,255,255);
	loc.y++;
	if (fStyle == 2) {
		loc.x++;
		b.right--;
	}
	parent->StrokeLine(loc, BPoint(b.right, loc.y));
	parent->SetHighColor(0,0,0);
}

/*------------------------------------------------------------*/

void MenusView::InvalMenu(BMenu *menu)
{
	BFont font;

	if (menu->Window() && menu->Window()->Lock()) {
		font.SetFamilyAndStyle(fMenuInfo.f_family, fMenuInfo.f_style);
		font.SetSize(fMenuInfo.font_size);
		menu->SetFont(&font);
		menu->SetViewColor(fMenuInfo.background_color);
		menu->Window()->Unlock();
	} else {
		font.SetFamilyAndStyle(fMenuInfo.f_family, fMenuInfo.f_style);
		font.SetSize(fMenuInfo.font_size);
		menu->SetFont(&font);
		menu->SetViewColor(fMenuInfo.background_color);
	}

	menu->InvalidateLayout();
	menu->Invalidate();

	BMenuItem	*item;
	for (long i = 0; (item = menu->ItemAt(i)) != 0; i++) {
		BMenu *sub = item->Submenu();
		if (sub) {

			// make sure the right font face/style is selected
			FontMenu *fontMenu = dynamic_cast<FontMenu *>(sub);
			if (fontMenu) {
				BFont font;
				font.SetFamilyAndStyle(fMenuInfo.f_family, fMenuInfo.f_style);
				fontMenu->SetCurrent(&font);
			}

			InvalMenu(sub);
		}
	}
}

/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
