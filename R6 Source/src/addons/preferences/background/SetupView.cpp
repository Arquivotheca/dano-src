//*****************************************************************************
//
//	File:		 SetupView.cpp
//
//	Description: Setup view class for Background preference panel
//
//	Copyright 1996 - 1998, Be Incorporated
//
//*****************************************************************************

#include "SetupView.h"
#include "PreviewView.h"
#include "Settings.h"
#include "TrackerDefs.h"
#include "ImagePanel.h"
#include <Application.h>
#include <StringView.h>
#include <Font.h>
#include <Button.h>
#include <CheckBox.h>
#include <Box.h>
#include <MenuBar.h>
#include <RadioButton.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <Rect.h>
#include <Screen.h>
#include <ColorControl.h>
#include <TextControl.h>
#include <FilePanel.h>
#include <File.h>
#include <FindDirectory.h>
#include <Beep.h>
#include <Alert.h>
#include <TranslationUtils.h>
#include <TranslatorRoster.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

const uint32 kCurrentWorkspace = 'wcur';
const uint32 kAllWorkspaces = 'wall';
const uint32 kManualPlacement = 'pman';
const uint32 kCenterPlacement = 'pctr';
const uint32 kScaleToFitPlacement = 'pscl';
const uint32 kTilePlacement = 'ptil';
const uint32 kLoadImage = 'limg';
const uint32 kLoadDir = 'ldir';
const uint32 kNoImage = 'nimg';
const uint32 kOpenFile = 'open';
const uint32 kOpenDir = 'odir';
const uint32 kEraseText = 'etxt';
const uint32 kLoadError = 'lerr';
const uint32 kApply = 'aapl';
const uint32 kDefaults = 'dflt';
const uint32 kRevert = 'revt';
const uint32 kXChanged = 'xxxx';
const uint32 kYChanged = 'yyyy';

void AddPath(BList &list, BPath p);
void RemovePath(BList &list, BPath p);

// Add path unique, sorted by Leaf()
void AddPath(BList &list, BPath p)
{
	int32	count = list.CountItems();
	int32	i;
	for(i = 0; i < count; i++)
	{
		BPath *curr = (BPath *)list.ItemAt(i);

		if(*curr == p)
			return;

		if(strcasecmp(curr->Leaf(), p.Leaf()) < 0)
			continue;

		if(strcasecmp(curr->Path(), p.Path()) < 0)
			continue;

		BPath *dup = new BPath(p);
		list.AddItem(dup, i);
		break;
	}
	if(i == count)
	{
		BPath *dup = new BPath(p);
		list.AddItem(dup);
	}
}

// Remove a path
void RemovePath(BList &list, BPath p)
{
	int32	count = list.CountItems();
	int32	i;
	for(i = 0; i < count; i++)
	{
		BPath *curr = (BPath *)list.ItemAt(i);
		if(p == *curr)
		{
			list.RemoveItem(curr);
			delete curr;
			break;
		}
	}
}

class MyColorControl : public BColorControl
{
	SetupView	*win;
public:
	MyColorControl(BPoint leftTop, color_control_layout matrix, float cellSide, const char *name, SetupView *w)
	 : BColorControl(leftTop, matrix, cellSide, name, 0, false), win(w)
	{
	}

	virtual void SetValue(int32 color)
	{
		BColorControl::SetValue(color);
		win->ChangeColor(ValueAsColor());
	}
};

class MyPreviewView : public PreviewView
{
	SetupView	*win;
public:
	MyPreviewView(BRect frame, const char *name, float ratio, BMessage *badload, SetupView *w)
	 : PreviewView(frame, name, ratio, badload), win(w)
	{
	}

	virtual void Move(BPoint p)
	{
		PreviewView::Move(p);
		win->ChangePosition(p);
	}
};

class MiniScreenView : public BView
{
public:
	MiniScreenView(BRect frame)
	 : BView(frame, "miniscreen", 0, B_WILL_DRAW)
	{
		SetViewColor(B_TRANSPARENT_32_BIT);
	}

	void Draw(BRect)
	{
		BRect r = Bounds();
		rgb_color	back = Parent()->ViewColor();
		rgb_color	shade = { 184, 184, 184, 255 };
		rgb_color	frame = { 156, 156, 156, 255 };
		BeginLineArray(16);
		AddLine(BPoint(2, 0), BPoint(r.right - 2, 0), frame);	// top frame
		AddLine(BPoint(1, 1), BPoint(r.right - 1, 1), frame);	// top frame
		AddLine(BPoint(1, r.bottom - 1), BPoint(r.right - 1, r.bottom - 1), frame);	// bottom frame
		AddLine(BPoint(2, r.bottom), BPoint(r.right - 2, r.bottom), frame);	// bottom frame
		AddLine(BPoint(0, 2), BPoint(0, r.bottom - 2), frame);	// left frame
		AddLine(BPoint(1, 1), BPoint(1, r.bottom - 1), frame);	// left frame
		AddLine(BPoint(r.right, 2), BPoint(r.right, r.bottom - 2), frame);	// right frame
		AddLine(BPoint(r.right - 1, 1), BPoint(r.right - 1, r.bottom - 1), frame);	// right frame
		AddLine(BPoint(0, 1), BPoint(1, 0), shade);	// corner shade
		AddLine(BPoint(0, r.bottom - 1), BPoint(1, r.bottom), shade);	// corner shade
		AddLine(BPoint(r.right - 1, r.bottom), BPoint(r.right, r.bottom - 1), shade);	// corner shade
		AddLine(BPoint(r.right - 1, 0), BPoint(r.right, 1), shade);	// corner shade
		AddLine(BPoint(0, 0), BPoint(0, 0), back);	// corner
		AddLine(BPoint(0, r.bottom), BPoint(0, r.bottom), back);	// corner
		AddLine(BPoint(r.right, 0), BPoint(r.right, 0), back);	// corner
		AddLine(BPoint(r.right, r.bottom), BPoint(r.right, r.bottom), back);	// corner
		EndLineArray();
	}
};


class LineView : public BView
{
public:
	LineView(BRect frame)
	 : BView(frame, "line", 0, B_WILL_DRAW)
	{
	}

	void AttachedToWindow()
	{
		SetViewColor(Parent()->ViewColor());
	}

	void Draw(BRect)
	{
		BRect r = Bounds();
		SetHighColor(156, 156, 156);
		StrokeLine(BPoint(r.left, 10), BPoint(r.right, 10), B_SOLID_HIGH);
		SetHighColor(156, 156, 156);
		StrokeLine(BPoint(r.left, 11), BPoint(r.right, 11), B_SOLID_HIGH);
		SetHighColor(Parent()->ViewColor());
		FillRect(BRect(r.left, 0, r.right, 9), B_SOLID_HIGH);
		FillRect(BRect(r.left, 12, r.right, r.bottom), B_SOLID_HIGH);
	}
};

SetupView::SetupView(PPAddOn *adn)
 : BView(BRect(0, 0, WX, WY), "background", 0, 0),
	imgpanel(0), lastimgpanelitem(0), currentbitmap(0), //pw(0),
	pv(0), erase(0), applyto(0), placement(0), x(0), y(0), bcc(0),
	apply(0), revert(0), image(0), addon(adn)
{
	// load preferences
	BPath	prefs_name;
	BFile	prefs_file;
	if(find_directory(B_USER_SETTINGS_DIRECTORY, &prefs_name) == B_OK &&
		prefs_name.Append("Backgrounds_settings") == B_OK &&
		prefs_file.SetTo(prefs_name.Path(), B_READ_ONLY) == B_OK)
	{
		if(prefs.Unflatten(&prefs_file) != B_OK)
			prefs.MakeEmpty();
	}

	// initialize translation kit
	roster = BTranslatorRoster::Default();

	BuildUI();
	LoadSettings();
}

void SetupView::BuildUI()
{
	// TOP BAR
	LineView	*topbar = new LineView(BRect(0, 0, WX, 24));
	AddChild(topbar);
	BPopUpMenu	*popup = new BPopUpMenu("menu");
	BMenuItem	*it;
	popup->AddItem(it = new BMenuItem("All Workspaces", new BMessage(kAllWorkspaces)));
	popup->AddItem(it = new BMenuItem("Current Workspace", new BMessage(kCurrentWorkspace)));
	it->SetMarked(true);
	BMenuField *applyto = new BMenuField(BRect(5, 0, 130, 18), "applyto", "", popup, (bool)true);
	applyto->SetDivider(0);
	topbar->AddChild(applyto);

	// PREVIEW
#define PREVX 110
#define PREVY 82

	MiniScreenView *miniview = new MiniScreenView(BRect(11, 32, 15 + PREVX, 36 + PREVY));
	PreviewView *prev = new MyPreviewView(BRect(2, 2, 2 + PREVX, 2 + PREVY),
		"preview", (float)PREVX / BScreen().Frame().Width(), new BMessage(kLoadError), this);
	miniview->AddChild(prev);
	AddChild(miniview);

	// SETTINGS
	BMenuField *image;
	BPopUpMenu *pu = new BPopUpMenu("image");
	AddChild(image = new BMenuField(BRect(140, 30, WX, 50), "image", "Desktop image:", pu));
	image->SetAlignment(B_ALIGN_RIGHT);
	image->SetDivider(80);

	BMenuField *place;
	pu = new BPopUpMenu("position");
	pu->AddItem(new BMenuItem("Manual", new BMessage(kManualPlacement)));
	pu->AddItem(new BMenuItem("Center", new BMessage(kCenterPlacement)));
	pu->AddItem(new BMenuItem("Scale to fit", new BMessage(kScaleToFitPlacement)));
	pu->AddItem(new BMenuItem("Tile", new BMessage(kTilePlacement)));
	AddChild(place = new BMenuField(BRect(140, 55, WX, 75), "placement", "Location:", pu));
	place->SetAlignment(B_ALIGN_RIGHT);
	place->SetDivider(80);

	BTextControl *x = new BTextControl(BRect(220, 83, 270, 100), "x", "X:", "", new BMessage(kXChanged));
	x->SetDivider(12);
	AddChild(x);
	BTextControl *y = new BTextControl(BRect(280, 83, 330, 100), "y", "Y:", "", new BMessage(kYChanged));
	y->SetDivider(12);
	AddChild(y);
	x->TextView()->SetMaxBytes(4);
	y->TextView()->SetMaxBytes(4);
	for(int32 ch = 0; ch <= 0xff; ch++)
		if(!isdigit((uchar)ch))
		{
			x->TextView()->DisallowChar((uchar)ch);
			y->TextView()->DisallowChar((uchar)ch);
		}

	AddChild(new BCheckBox(BRect(220, 105, WX, 120),
		"erase", "Mask icon text backgrounds", new BMessage(kEraseText)));

	// COLOR CONTROL
	AddChild(new BStringView(BRect(8, 140, 200, 155), "back", "Background color:", B_FOLLOW_TOP | B_FOLLOW_LEFT, B_WILL_DRAW));
	AddChild(new MyColorControl(BPoint(15, 160), B_CELLS_32x8, 4, "backcolor", this));

	// BOTTOM
	LineView	*bottombar = new LineView(BRect(0, WY - 45, WX, WY - 26));
	AddChild(bottombar);

	AddChild(new BButton(BRect(5, WY - 25, 75, WY),
		"defaults", "Defaults", new BMessage(kDefaults)));
	AddChild(new BButton(BRect(85, WY - 25, 155, WY),
		"revert", "Revert", new BMessage(kRevert)));
	AddChild(new BButton(BRect(WX - 70, WY - 25, WX, WY),
		"apply", "Apply", new BMessage(kApply)));
}

void SetupView::LoadSettings()
{
	// last directories
	const char *path;
	if(prefs.FindString("paneldir", &path) == B_OK)
	{
		BEntry e(path);
		e.GetRef(&lastimgpaneldir);
	}
}

SetupView::~SetupView()
{
	delete currentbitmap;
	delete imgpanel;
}

void SetupView::WorkspaceActivated(int32 /*workspace*/, bool active)
{
	if(active)
	{
		settings.Revert();
		ShowSettings();
	}
}

// reload image to reflect workspace changes
void SetupView::ScreenChanged(BRect, color_space)
{
	float	scale;

	// wrapped in a block to minimize BScreen life
	{
		scale = (float)PREVX / BScreen().Frame().Width();
	}

	pv->SetScale(scale);

	BPath path = settings.GetImagePath();
	if(path.Path() != 0 && strlen(path.Path()) != 0)
		pv->LoadBitmap(path.Path(), &currentbitmap);
}

void SetupView::ShowSettings()
{
	BuildImageMenu();
	ImageSelection();

	int32 mode = settings.GetImageMode();
	placement->MenuBar()->ItemAt(0)->Submenu()->ItemAt(mode)->SetMarked(true);
	switch(mode)
	{
		case kAtOffset :
			{
			BPoint p = settings.GetImageOffset();
			pv->Manual(p.x, p.y);
			}
			break;

		case kCentered :
			pv->Center();
			break;

		case kScaledToFit :
			pv->ScaleToFit();
			break;

		case kTiled :
			pv->Tile();
			break;
	}
	ShowXY();

	erase->SetValue(settings.GetImageEraseText());

	// load image
	BPath path = settings.GetImagePath();
	BEntry file;
	if(path.Path() == 0 || strlen(path.Path()) == 0 ||
		file.SetTo(path.Path()) != B_OK || ! file.IsFile() ||
		! pv->LoadBitmap(path.Path(), &currentbitmap))
	{
		pv->ClearBitmap();
	}

	rgb_color col = settings.GetColor();
	pv->SetLowColor(col);
	pv->SetViewColor(col);
	bcc->SetValue(col);
	CheckEnabled();
	CheckChanges();
}

void SetupView::BuildImageMenu()
{
	BPath current = settings.GetImagePath();
	BMenu		*pu = image->Menu();
	BMenuItem	*bar = image->MenuBar()->ItemAt(0);
	BMenuItem	*it;
	bool		added = false;

	// delete all items
	int32 count = pu->CountItems();
	for(int32 i = count - 1; i >= 0; i--)
	{
		BMenuItem *it = pu->RemoveItem(i);
		delete it;
	}

	uint32	cookie = 0;
	BPath	p;
	while(settings.GetNextPath(&p, &cookie))
		AddPath(recentimages, p);

	// menu item
	pu->AddItem(it = new BMenuItem("None", new BMessage(kNoImage)));
	if(current.Path() == 0 || strlen(current.Path()) == 0)
	{
		it->SetMarked(true);
		bar->SetLabel("None");
	}
	pu->AddSeparatorItem();

	count = recentimages.CountItems();
	for(int32 i = 0; i < count; i++)
	{
		BPath		*curr = (BPath *)recentimages.ItemAt(i);
		BMessage	*msg = new BMessage(kLoadImage);
		BEntry		e(curr->Path());
		entry_ref	ref;
		e.GetRef(&ref);

		msg->AddRef("refs", &ref);
		msg->AddBool("menu", true);
		pu->AddItem(it = new BMenuItem(curr->Leaf(), msg));

		if(*curr == current)
			it->SetMarked(true);
		added = true;
	}

	if(added)
		pu->AddSeparatorItem();
	pu->AddItem(new BMenuItem("Other" B_UTF8_ELLIPSIS, new BMessage(kOpenFile)));

	// the image menu is always rebuilt so we always have to reset the target
	image->MenuBar()->ItemAt(0)->Submenu()->SetTargetForItems(this);
}

void SetupView::AttachedToWindow()
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	pv = dynamic_cast<PreviewView *>(FindView("preview"));
	applyto = dynamic_cast<BMenuField *>(FindView("applyto"));
	placement = dynamic_cast<BMenuField *>(FindView("placement"));
	x = dynamic_cast<BTextControl *>(FindView("x"));
	y = dynamic_cast<BTextControl *>(FindView("y"));
	image = dynamic_cast<BMenuField *>(FindView("image"));
	bcc = dynamic_cast<BColorControl *>(FindView("backcolor"));
	erase = dynamic_cast<BCheckBox *>(FindView("erase"));
	dflt = dynamic_cast<BButton *>(FindView("defaults"));
	apply = dynamic_cast<BButton *>(FindView("apply"));
	revert = dynamic_cast<BButton *>(FindView("revert"));

	settings.Load();
	settings.ApplyToCurrentWorkspace();
	ShowSettings();

	x->SetTarget(this);
	y->SetTarget(this);
	pv->SetTarget(this);
	apply->SetTarget(this);
	revert->SetTarget(this);
	erase->SetTarget(this);
	placement->MenuBar()->ItemAt(0)->Submenu()->SetTargetForItems(this);
	applyto->MenuBar()->ItemAt(0)->Submenu()->SetTargetForItems(this);
	dflt->SetTarget(this);
}

void SetupView::DetachedFromWindow()
{
	entry_ref	dir;
	BPath	path;
	BEntry	e;

	// regen prefs
	prefs.RemoveName("pos");
	prefs.AddPoint("pos", Frame().LeftTop());

	if(imgpanel)
		imgpanel->GetPanelDirectory(&lastimgpaneldir);
	if(e.SetTo(&lastimgpaneldir) == B_OK &&
		e.GetPath(&path) == B_OK)
	{
		prefs.RemoveName("paneldir");
		prefs.AddString("paneldir", path.Path());
	}

	prefs.RemoveName("recentimage");	// this cleans up unneeded R4 preferences
	prefs.RemoveName("recentfolder");	// this cleans up unneeded folder settings

	// save prefs
	BPath	prefs_name;
	BFile	prefs_file;
	if(find_directory(B_USER_SETTINGS_DIRECTORY, &prefs_name) == B_OK &&
		prefs_name.Append("Backgrounds_settings") == B_OK &&
		prefs_file.SetTo(prefs_name.Path(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE) == B_OK)
	{
		prefs.Flatten(&prefs_file);
	}
}

void SetupView::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case B_SIMPLE_DATA :
			if(msg->HasRef("refs"))
			{
				BMessage	imgs(kLoadImage);
				entry_ref	ref;
				int32		i = 0;
				int32		files = 0;

				// preprocess refs, send all files to kLoadImage
				while(msg->FindRef("refs", i++, &ref) == B_OK)
				{
					BEntry	file;
					BPath	path;
					file.SetTo(&ref, true);
					file.GetPath(&path);
					if(file.IsFile())
					{
						status_t		err = B_ERROR;
						translator_info	info;
						const char		*mime = 0;
						char			str[B_MIME_TYPE_LENGTH + 30];

						BFile	imgfile(&ref, O_RDONLY);

						if(B_OK <= imgfile.ReadAttr("BEOS:TYPE", B_STRING_TYPE, 0, str, B_FILE_NAME_LENGTH))
							mime = str;

						// First try to identify the data using the hint, if any
						if(mime)
							err = roster->Identify(&imgfile, 0, &info, 0, mime);

						// If not identified, try without a hint
						if(err)
							err = roster->Identify(&imgfile, 0, &info);

						if(err == B_OK && info.group == B_TRANSLATOR_BITMAP)
						{
							// add the image to the recent list
							AddPath(recentimages, path);

							imgs.AddRef("refs", &ref);
							files++;
						}
					}
				}
				if(files)
					LoadImages(&imgs);
			}
			break;

		case B_CANCEL :
			{
			BFilePanel *which;
			if(msg->FindPointer("source", (void **)&which) == B_OK)
			{
				if(which == imgpanel && lastimgpanelitem)
				{
					// switch image setting back to current
					lastimgpanelitem->SetMarked(true);
				}
			}
			}
			break;

		case B_WORKSPACE_ACTIVATED :
			{
			long space;
			bool act;
			msg->FindInt32("workspace", &space);
			msg->FindBool("active", &act);
			WorkspaceActivated(space, act);
			}
			break;

		case B_SCREEN_CHANGED :
			{
			BRect r;
			color_space mode;
			msg->FindRect("frame", &r);
			msg->FindInt32("mode", (long*) &mode);
			ScreenChanged(r, mode);
			}
			break;

		case 'doit' :
			addon->SwitchToPanel();
			break;

		case kApply :
			{
			status_t ret;
			if((ret = settings.Apply()) != B_NO_ERROR)
			{
				char buf[256];
				sprintf(buf, "There was an error writing the background attribute: %s", strerror(ret));
				(new BAlert("Background", buf, "OK", 0, 0, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
			}
			CheckChanges();
			}
			break;

		case kDefaults :
			settings.Default();
			ShowSettings();
			break;

		case kRevert :
			settings.Revert();
			ShowSettings();
			break;

		case kLoadImage :
			LoadImages(msg);
			break;

		case kManualPlacement :
			{
			BPoint p = settings.GetImageOffset();
			pv->Manual(p.x, p.y);
			settings.SetImageMode(kAtOffset);

			pv->Redisplay();
			ShowXY();
			CheckEnabled();
			CheckChanges();
			}
			break;

		case kCenterPlacement :
			pv->Center();
			settings.SetImageMode(kCentered);
			settings.SetImageOffset(BPoint(0, 0));

			pv->Redisplay();
			ShowXY();
			CheckEnabled();
			CheckChanges();
			break;

		case kTilePlacement :
			pv->Tile();
			settings.SetImageMode(kTiled);
			settings.SetImageOffset(BPoint(0, 0));

			pv->Redisplay();
			ShowXY();
			CheckEnabled();
			CheckChanges();
			break;

		case kScaleToFitPlacement :
			pv->ScaleToFit();
			settings.SetImageMode(kScaledToFit);
			settings.SetImageOffset(BPoint(0, 0));

			pv->Redisplay();
			ShowXY();
			CheckEnabled();
			CheckChanges();
			break;

		case kNoImage :
			NoImage();
			break;

		case kEraseText :
			settings.SetImageEraseText(erase->Value());
			CheckChanges();
			break;

		case kOpenFile :
			if(! imgpanel)
			{
				// load panel
				BMessenger	me(this);
				BMessage	imgmsg(kLoadImage);

				imgpanel = new ImagePanel(&me, 0, &imgmsg, &imgfilter);
				imgpanel->SetPanelDirectory(&lastimgpaneldir);
			}

			imgpanel->Show();
			break;

		case kLoadError :
			(new BAlert("Background", "There was an error loading the image", "OK", 0, 0,
				B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
			NoImage();
			break;

		case kAllWorkspaces :
			AllWorkspaces();
			break;

		case kCurrentWorkspace :
			CurrentWorkspace();
			break;

		case kXChanged :
			{
			BPoint	p = settings.GetImageOffset();
			int32	xc = atol(x->Text());
			int32	sw = BScreen(Window()).Frame().IntegerWidth();
			if(xc < 0) xc = 0;
			if(xc > sw) xc = sw;
			BPoint	newp = BPoint(xc, p.y);
			settings.SetImageOffset(newp);
			pv->Manual(newp.x, newp.y);
			pv->Redisplay();
			ChangePosition(newp);
			CheckChanges();
			}
			break;

		case kYChanged :
			{
			BPoint	p = settings.GetImageOffset();
			int32	yc = atol(y->Text());
			int32	sh = BScreen(Window()).Frame().IntegerHeight();
			if(yc < 0) yc = 0;
			if(yc > sh) yc = sh;
			BPoint	newp = BPoint(p.x, yc);
			settings.SetImageOffset(newp);
			pv->Manual(newp.x, newp.y);
			pv->Redisplay();
			ChangePosition(newp);
			CheckChanges();
			}
			break;

		default :
			BView::MessageReceived(msg);
			break;
	}
}

void SetupView::LoadImages(BMessage *msg)
{
	bool		loaded = false;
	entry_ref	ref;
	int32		i = 0;

	while(msg->FindRef("refs", i++, &ref) == B_OK)
	{
		BEntry file;
		BPath path;
		file.SetTo(&ref, true);
		file.GetPath(&path);

		if(file.IsFile() && ! loaded &&
			pv->LoadBitmap(path.Path(), &currentbitmap))
		{
			// set panel directory
			BEntry direntry;
			file.GetParent(&direntry);
			direntry.GetRef(&lastimgpaneldir);
			if(imgpanel)
				imgpanel->SetPanelDirectory(&lastimgpaneldir);

			settings.SetImagePath(path);
			ShowXY();
			CheckEnabled();
			CheckChanges();
			loaded = true;
		}
	}

	if(loaded)
	{
		BuildImageMenu();
		ImageSelection();
	}
	else
	{
		if(lastimgpanelitem)
			lastimgpanelitem->SetMarked(true);

		beep();
	}
}

void SetupView::NoImage()
{
	pv->ClearBitmap();
	settings.SetImagePath(BPath());
	BuildImageMenu();
	ImageSelection();
	ShowXY();
	CheckChanges();
	CheckEnabled();
}

void SetupView::AllWorkspaces()
{
	applyto->MenuBar()->ItemAt(0)->SetLabel("All Workspaces");

	// mark current item
	BMenu	*m = applyto->MenuBar()->ItemAt(0)->Submenu();
	int32 count = m->CountItems();
	for(int32 i = 0; i < count; i++)
		m->ItemAt(i)->SetMarked(i == 0);

	settings.ApplyToAllWorkspaces();
	EnablePlacementItems(true);
	ShowSettings();
}

void SetupView::CurrentWorkspace()
{
	applyto->MenuBar()->ItemAt(0)->SetLabel("Current Workspace");

	// mark current item
	BMenu	*m = applyto->MenuBar()->ItemAt(0)->Submenu();
	int32 count = m->CountItems();
	for(int32 i = 0; i < count; i++)
		m->ItemAt(i)->SetMarked(i == 1);

	settings.ApplyToCurrentWorkspace();
	EnablePlacementItems(true);
	ShowSettings();
}

void SetupView::EnablePlacementItems(bool enable)
{
	BMenu	*m = placement->MenuBar()->ItemAt(0)->Submenu();
	m->ItemAt(1)->SetEnabled(enable);
	m->ItemAt(2)->SetEnabled(enable);
	bcc->SetEnabled(enable);
}

void SetupView::CheckEnabled()
{
	BTextView		*xv = dynamic_cast<BTextView *>(x ? x->ChildAt(0) : 0);
	BTextView		*yv = dynamic_cast<BTextView *>(y ? y->ChildAt(0) : 0);

	if(xv && yv)
	{
		BPath	p = settings.GetImagePath();

		if(p.Path() == 0 || strlen(p.Path()) == 0)
		{
			placement->SetEnabled(false);
			placement->SetFlags(placement->Flags() & ~B_NAVIGABLE);
			erase->SetEnabled(false);
			x->SetEnabled(false);
			y->SetEnabled(false);
			xv->MakeSelectable(false);
			yv->MakeSelectable(false);
			xv->SetFlags(xv->Flags() & ~B_NAVIGABLE);
			yv->SetFlags(yv->Flags() & ~B_NAVIGABLE);
		}
		else
		{
			int32 mode = settings.GetImageMode();
			placement->SetEnabled(true);
			placement->SetFlags(placement->Flags() | B_NAVIGABLE);
			erase->SetEnabled(true);
			bool enable = mode == kAtOffset;
			x->SetEnabled(enable);
			y->SetEnabled(enable);
			xv->MakeSelectable(enable);
			yv->MakeSelectable(enable);
			xv->SetFlags(enable ? (xv->Flags() | B_NAVIGABLE) : (xv->Flags() & ~B_NAVIGABLE));
			yv->SetFlags(enable ? (yv->Flags() | B_NAVIGABLE) : (yv->Flags() & ~B_NAVIGABLE));
		}
	}
}

void SetupView::CheckChanges()
{
	bool changes = settings.Changes();
	apply->SetEnabled(changes);
	revert->SetEnabled(changes);
}

void SetupView::ChangeColor(rgb_color col)
{
	pv->SetViewColor(col);
	pv->Invalidate();
	settings.SetColor(col);
	CheckChanges();
}

void SetupView::ChangePosition(BPoint p)
{
	char	tmp[30];
	sprintf(tmp, "%d", (int)p.x);
	x->SetText(tmp);
	sprintf(tmp, "%d", (int)p.y);
	y->SetText(tmp);
	settings.SetImageOffset(p);
	CheckChanges();
}

void SetupView::ImageSelection()
{
	BMenu	*m = image->MenuBar()->ItemAt(0)->Submenu();
	lastimgpanelitem = m->FindMarked();
}

void SetupView::ShowXY()
{
	BPath	current = settings.GetImagePath();
	int32	mode = settings.GetImageMode();
	if(mode == kAtOffset && current.Path() && strlen(current.Path()))
	{
		BPoint	p = settings.GetImageOffset();
		char	tmp[30];
		sprintf(tmp, "%d", (int)p.x);
		x->SetText(tmp);
		sprintf(tmp, "%d", (int)p.y);
		y->SetText(tmp);
	}
	else
	{
		x->SetText("");
		y->SetText("");
	}
}
