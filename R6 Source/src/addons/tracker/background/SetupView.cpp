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
#include "MockupView.h"
#include "Settings.h"
#include "TrackerDefs.h"
#include "ImagePanel.h"
#include "DirPanel.h"
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
#include <Roster.h>
#include <Screen.h>
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

const uint32 kDefaultFolder = 'dfld';
const uint32 kSelectedFolder = 'sfld';
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
const uint32 kDefault = 'dflt';
const uint32 kApply = 'aapl';
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

SetupView::SetupView()
 : BBox(BRect(0, 0, WX, WY), "background", B_FOLLOW_ALL, B_WILL_DRAW, B_PLAIN_BORDER),
	imgpanel(0), dirpanel(0), lastimgpanelitem(0), lastdirpanelitem(0),
	pv(0), erase(0), applyto(0), x(0), y(0), placement(0),
	apply(0), revert(0), dflt(0), image(0)
{
	// load preferences
	BPath	prefs_name;
	BFile	prefs_file;
	if(find_directory(B_USER_SETTINGS_DIRECTORY, &prefs_name) == B_OK &&
		prefs_name.Append("FolderBackground_settings") == B_OK &&
		prefs_file.SetTo(prefs_name.Path(), B_READ_ONLY) == B_OK)
	{
		if(prefs.Unflatten(&prefs_file) != B_OK)
			prefs.MakeEmpty();
	}

	// initialize translation kit
	roster = BTranslatorRoster::Default();

	settings.ApplyToDefaultFolder();
	BuildUI();
	LoadSettings();
}

void SetupView::BuildUI()
{
	// build user interface
	LineView	*topbar = new LineView(BRect(10, 0, WX - 10, 24));
	AddChild(topbar);

	BFont	f(be_plain_font);
	f.SetSize(10);

	BPopUpMenu	*popup = new BPopUpMenu("menu");
	BMenuItem	*it;
	popup->AddItem(it = new BMenuItem("Default folder", new BMessage(kDefaultFolder)));
	it->SetMarked(true);
	popup->AddItem(it = new BMenuItem("Other folder" B_UTF8_ELLIPSIS, new BMessage(kSelectedFolder)));
	BMenuField *applyto = new BMenuField(BRect(5, 0, 130, 18), "applyto", "", popup, (bool)true);
	popup->SetFont(&f);
	applyto->SetFont(&f);
	applyto->MenuBar()->SetFont(&f);
	applyto->SetDivider(0);
	topbar->AddChild(applyto);

	// PREVIEW
#define PREVX 120
#define PREVY 90
	float scale = (float)PREVX / BScreen().Frame().Width();

	PreviewView *prev = new MyPreviewView(BRect(20, 38, 20 + PREVX, 38 + PREVY),
		"preview", scale, new BMessage(kLoadError), this);
	AddChild(new MockupView("mockup", prev));

	BMenuField *image;
	BPopUpMenu *pu = new BPopUpMenu("");
	AddChild(image = new BMenuField(BRect(160, 30, WX, 50), "image", "Folder image:", pu));
	image->SetAlignment(B_ALIGN_RIGHT);
	image->SetDivider(80);

	BMenuField *place;
	pu = new BPopUpMenu("");
	pu->AddItem(new BMenuItem("Manual", new BMessage(kManualPlacement)));
	pu->AddItem(new BMenuItem("Tile", new BMessage(kTilePlacement)));
	AddChild(place = new BMenuField(BRect(160, 55, WX, 75), "placement", "Location:", pu));
	place->SetAlignment(B_ALIGN_RIGHT);
	place->SetDivider(80);

	BTextControl *x = new BTextControl(BRect(240, 83, 290, 100), "x", "X:", "", new BMessage(kXChanged));
	x->SetDivider(12);
	AddChild(x);
	BTextControl *y = new BTextControl(BRect(300, 83, 350, 100), "y", "Y:", "", new BMessage(kYChanged));
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

	AddChild(new BCheckBox(BRect(240, 105, WX, 120),
		"erase", "Mask icon text backgrounds", new BMessage(kEraseText)));

	// build user interface
	LineView	*bottombar = new LineView(BRect(10, WY - 54, WX - 10, WY - 44));
	AddChild(bottombar);

	AddChild(new BButton(BRect(10, WY - 35, 80, WY - 10),
		"dflt", "Defaults", new BMessage(kDefault)));
	AddChild(new BButton(BRect(90, WY - 35, 160, WY - 10),
		"revert", "Revert", new BMessage(kRevert)));
	AddChild(new BButton(BRect(WX - 80, WY - 35, WX - 10, WY - 10),
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
	if(prefs.FindString("folderpaneldir", &path) == B_OK)
	{
		BEntry e(path);
		e.GetRef(&lastdirpaneldir);
	}

	// load recent folder list
	int32 i = 0;
	const char *fldpath;
	while(prefs.FindString("recentfolder", i++, &fldpath) == B_OK)
		AddPath(recentfolders, fldpath);
}

SetupView::~SetupView()
{
	delete imgpanel;
	delete dirpanel;
}

void SetupView::ShowSettings()
{
	BuildImageMenu();
	ImageSelection();

	int32 mode = settings.GetImageMode();
	placement->MenuBar()->ItemAt(0)->Submenu()->ItemAt(mode ? 1 : 0)->SetMarked(true);
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
		! pv->LoadBitmap(path.Path()))
	{
		pv->ClearBitmap();
	}

	rgb_color col = { 255, 255, 255, 255 };
	pv->SetLowColor(col);
	pv->SetViewColor(col);
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

	const char *nonetext = settings.IsApplyToFolder() ? "Default" : "None";

	// menu item
	pu->AddItem(it = new BMenuItem(nonetext, new BMessage(kNoImage)));
	if(current.Path() == 0 || strlen(current.Path()) == 0)
	{
		it->SetMarked(true);
		bar->SetLabel(nonetext);
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
}

void SetupView::AllAttached()
{
	pv = dynamic_cast<PreviewView *>(FindView("preview"));
	applyto = dynamic_cast<BMenuField *>(FindView("applyto"));
	placement = dynamic_cast<BMenuField *>(FindView("placement"));
	image = dynamic_cast<BMenuField *>(FindView("image"));
	erase = dynamic_cast<BCheckBox *>(FindView("erase"));
	x = dynamic_cast<BTextControl *>(FindView("x"));
	y = dynamic_cast<BTextControl *>(FindView("y"));
	apply = dynamic_cast<BButton *>(FindView("apply"));
	revert = dynamic_cast<BButton *>(FindView("revert"));
	dflt = dynamic_cast<BButton *>(FindView("dflt"));

	BuildFolderMenu();
	ApplyToSelection();
	ShowSettings();

	pv->SetTarget(this);
	x->SetTarget(this);
	y->SetTarget(this);
	apply->SetTarget(this);
	revert->SetTarget(this);
	dflt->SetTarget(this);
	erase->SetTarget(this);
	placement->MenuBar()->ItemAt(0)->Submenu()->SetTargetForItems(this);
	applyto->MenuBar()->ItemAt(0)->Submenu()->SetTargetForItems(this);
}

void SetupView::DetachedFromWindow()
{
	entry_ref	dir;
	BPath	path;
	BEntry	e;
	int32	count;
	int32	i;

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

	if(dirpanel)
		dirpanel->GetPanelDirectory(&lastdirpaneldir);
	if(e.SetTo(&lastdirpaneldir) == B_OK &&
		e.GetPath(&path) == B_OK)
	{
		prefs.RemoveName("folderpaneldir");
		prefs.AddString("folderpaneldir", path.Path());
	}

	prefs.RemoveName("recentimage");	// this cleans up unneeded R4 preferences
	prefs.RemoveName("recentfolder");
	count = recentfolders.CountItems();
	if(count > 32) count = 32;
	for(i = 0; i < count; i++)
	{
		BPath *curr = (BPath *)recentfolders.ItemAt(i);
		prefs.AddString("recentfolder", curr->Path());
	}

	// save prefs
	BPath	prefs_name;
	BFile	prefs_file;
	if(find_directory(B_USER_SETTINGS_DIRECTORY, &prefs_name) == B_OK &&
		prefs_name.Append("FolderBackground_settings") == B_OK &&
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
				BMessage	firstdir(kOpenDir);
				entry_ref	ref;
				int32		i = 0;
				int32		files = 0;
				int32		dirs = 0;

				// preprocess refs, send all files to kLoadImage
				// send first folder to kOpenDir (if no images)
				// queue all folders
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
					else
					{
						if(dirs++ == 0)
							firstdir.AddString("path", path.Path());
						BPath desktop;
						if(find_directory(B_DESKTOP_DIRECTORY, &desktop) != B_OK ||
							path != desktop)
							AddPath(recentfolders, path);
					}
				}
				if(dirs)
					BuildFolderMenu();
				if(files)
					LoadImages(&imgs);
				else
					OpenDir(&firstdir);
			}
			break;

		case B_CANCEL :
			{
			BFilePanel *which;
			if(msg->FindPointer("source", (void **)&which) == B_OK)
			{
				if(which == dirpanel && lastdirpanelitem)
				{
					// switch folder/workspace selection back to current
					lastdirpanelitem->SetMarked(true);
				} else if(which == imgpanel && lastimgpanelitem)
				{
					// switch image setting back to current
					lastimgpanelitem->SetMarked(true);
				}
			}
			}
			break;

		case kApply :
			{
			status_t ret;
			if((ret = settings.Apply()) != B_NO_ERROR)
			{
				char buf[256];
				sprintf(buf, "There was an error writing the background attribute: %s", strerror(ret));
				(new BAlert("Backgrounds", buf, "OK", 0, 0, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
			}
			CheckChanges();
			}
			break;

		case kDefault :
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
			(new BAlert("Backgrounds", "There was an error loading the image", "OK", 0, 0,
				B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
			NoImage();
			break;

		case kDefaultFolder :
			DefaultFolder();
			break;

		case kSelectedFolder :
			if(! dirpanel)
			{
				// load panel
				BMessenger	me(this);
				BMessage	dirmsg(kLoadDir);

				dirpanel = new DirPanel(&me, 0, &dirmsg, &dirfilter);
				dirpanel->SetPanelDirectory(&lastdirpaneldir);
			}
			dirpanel->Show();
			break;

		case kLoadDir :
			LoadDir(msg);
			break;

		case kOpenDir :
			OpenDir(msg);
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
			BBox::MessageReceived(msg);
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
			pv->LoadBitmap(path.Path()))
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

void SetupView::DefaultFolder()
{
	ApplyToSelection();
	applyto->MenuBar()->ItemAt(0)->SetLabel("Default Folder");

	// mark current item
	BMenu	*m = applyto->MenuBar()->ItemAt(0)->Submenu();
	int32 count = m->CountItems();
	for(int32 i = 0; i < count; i++)
		m->ItemAt(i)->SetMarked(i == 0);

	settings.ApplyToDefaultFolder();
	ShowSettings();
}

void SetupView::OpenDir(BMessage *msg)
{
	ApplyToSelection();
	const char	*folder;
	entry_ref	ref;

	if(msg->FindString("path", &folder) == B_OK)
	{
		BPath	folderpath(folder);
		BPath	desktop;

		if(find_directory(B_DESKTOP_DIRECTORY, &desktop) == B_OK &&
			folderpath == desktop)
		{
			BMessage panel('doit');
			panel.AddString("be:panel", "background");
			be_roster->Launch("application/x-vnd.Be-BACK", &panel);
			be_app->PostMessage(B_QUIT_REQUESTED);
			return;
		}
		else
		{
			if(settings.ApplyToFolder(&folderpath) == B_OK)
			{
				BMenu	*m = applyto->MenuBar()->ItemAt(0)->Submenu();
	
				int32 count = m->CountItems();
				for(int32 i = 0; i < count; i++)
				{
					BMenuItem *it = m->ItemAt(i);
					const char *itpath;
					if(it->Message() &&
						it->Message()->FindString("path", &itpath) == B_OK &&
						strcmp(itpath, folder) == 0)
						it->SetMarked(true);
				}

				ShowSettings();
			}
			else
			{
				(new BAlert("Backgrounds", "There was an error opening the selected folder", "OK", 0, 0,
					B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
				BuildFolderMenu();
				DefaultFolder();
			}
		}
	}
}

void SetupView::LoadDir(BMessage *msg)
{
	bool		found = false;
	entry_ref	ref;
	BEntry		file;
	BPath		path;
	BPath		desktop;
	int32		i = 0;

	lastdirpanelitem = 0;

	while(msg->FindRef("refs", i++, &ref) == B_OK)
	{
		file.SetTo(&ref, true);
		if(file.IsDirectory())
		{
			file.GetPath(&path);

			if(find_directory(B_DESKTOP_DIRECTORY, &desktop) == B_OK &&
				path == desktop)
			{
				BMessage panel('open');
				panel.AddString("be:panel", "background");
				be_roster->Launch("application/x-vnd.Be-BACK", &panel);
				be_app->PostMessage(B_QUIT_REQUESTED);
				return;
			}

			AddPath(recentfolders, path);
			BuildFolderMenu();

			if(! found)
			{
				BMessage fmsg(kOpenDir);
				fmsg.AddString("path", path.Path());
				OpenDir(&fmsg);
				found = true;
			}
		}
	}

	if(! found)
	{
		(new BAlert("Backgrounds", "There was an error opening the selected folder", "OK", 0, 0,
			B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
		DefaultFolder();
	}
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

void SetupView::ApplyToSelection()
{
	BMenu	*m = applyto->MenuBar()->ItemAt(0)->Submenu();
	lastdirpanelitem = m->FindMarked();
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

void SetupView::BuildFolderMenu()
{
	BMenu	*m = applyto->MenuBar()->ItemAt(0)->Submenu();
	int32	count = m->CountItems();
	char tmp[256];

	for(int32 i = count - 1; i > 1; i--)
	{
		BMenuItem *it = m->RemoveItem(i);
		delete it;
	}

	count = recentfolders.CountItems();
	if(count)
	{
		m->AddSeparatorItem();
		for(int32 i = 0; i < count; i++)
		{
			BPath		*curr = (BPath *)recentfolders.ItemAt(i);
			BMessage	*msg = new BMessage(kOpenDir);
			BEntry e(curr->Path());
			if(e.IsDirectory())
			{
				msg->AddString("path", curr->Path());
				sprintf(tmp, "Folder: %s", curr->Leaf());
				m->AddItem(new BMenuItem(tmp, msg));
			}
		}
	}

	applyto->MenuBar()->ItemAt(0)->Submenu()->SetTargetForItems(this);
}

