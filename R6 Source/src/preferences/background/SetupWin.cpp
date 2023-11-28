//*****************************************************************************
//
//	File:		 SetupWin.cpp
//
//	Description: Setup window class for Background preference panel
//
//	Copyright 1996 - 1998, Be Incorporated
//
//*****************************************************************************

#include "SetupWin.h"
#include "PreviewView.h"
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

class MyColorControl : public BColorControl
{
	SetupWin	*win;
public:
	MyColorControl(BPoint leftTop, color_control_layout matrix, float cellSide, const char *name, SetupWin *w)
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
	SetupWin	*win;
public:
	MyPreviewView(BRect frame, const char *name, float ratio, BMessage *badload, SetupWin *w)
	 : PreviewView(frame, name, ratio, badload), win(w)
	{
	}

	virtual void Move(BPoint p)
	{
		PreviewView::Move(p);
		win->ChangePosition(p);
	}
};

SetupWin::SetupWin()
 : BWindow(BRect(0, 0, WX, WY), "Backgrounds", B_TITLED_WINDOW_LOOK,
	B_NORMAL_WINDOW_FEEL, B_NOT_RESIZABLE | B_NOT_ZOOMABLE, B_ALL_WORKSPACES),
	imgpanel(0), dirpanel(0), lastimgpanel(0), lastdirpanel(0)
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

	// adjust window position
	{
		BPoint	pos;
		BRect	screen;
		BScreen	s(this);
		screen = s.Frame();
		if(prefs.FindPoint("pos", &pos) != B_OK || pos.x > screen.right || pos.y > screen.bottom)
			MoveTo((screen.right - screen.left - WX) / 2, (screen.bottom - screen.top - WY) / 2);
		else
			MoveTo(pos);
	}

	// load panels
	BMessenger	me(this);
	BMessage	imgmsg(kLoadImage);
	BMessage	dirmsg(kLoadDir);

	imgpanel = new ImagePanel(&me, 0, &imgmsg, &imgfilter);
	dirpanel = new DirPanel(&me, 0, &dirmsg, &dirfilter);

	const char *path;
	if(prefs.FindString("paneldir", &path) == B_OK)
	{
		BEntry e(path);
		entry_ref dir;
		e.GetRef(&dir);
		imgpanel->SetPanelDirectory(&dir);
	}
	if(prefs.FindString("folderpaneldir", &path) == B_OK)
	{
		BEntry e(path);
		entry_ref dir;
		e.GetRef(&dir);
		dirpanel->SetPanelDirectory(&dir);
	}

	// build user interface
	BBox *back = new BBox(Bounds(), "back", B_FOLLOW_ALL, B_WILL_DRAW, B_PLAIN_BORDER);
	AddChild(back);

	BFont	f(be_plain_font);
	f.SetSize(10);

	// PREVIEW
#define PREVX 120
#define PREVY 90

	BBox	*preview = new BBox(BRect(10, 16, 160, 180), "Preview");
	preview->SetFont(&f);
	preview->SetLabel("Preview");
	back->AddChild(preview);

	float	scale;

	// wrapped in a block to minimize BScreen life
	{
		scale = (float)PREVX / BScreen().Frame().Width();
	}

	PreviewView *prev = new MyPreviewView(BRect(15, 25, 15 + PREVX, 25 + PREVY),
		"preview", scale, new BMessage(kLoadError), this);
	prev->SetTarget(this);
	preview->AddChild(new MockupView("mockup", prev));

	BTextControl *x = new BTextControl(BRect(15, 135, 70, 150), "x", "X:", "", new BMessage(kXChanged));
	x->SetDivider(12);
	preview->AddChild(x);
	BTextControl *y = new BTextControl(BRect(80, 135, 135, 150), "y", "Y:", "", new BMessage(kYChanged));
	y->SetDivider(12);
	preview->AddChild(y);
	x->TextView()->SetMaxBytes(4);
	y->TextView()->SetMaxBytes(4);
	for(int32 ch = 0; ch <= 0xff; ch++)
		if(!isdigit((uchar)ch))
		{
			x->TextView()->DisallowChar((uchar)ch);
			y->TextView()->DisallowChar((uchar)ch);
		}


	// SETTINGS
	BBox	*setbox = new BBox(BRect(170, 10, WX - 10, 180), "settings");
	setbox->SetFont(&f);
	back->AddChild(setbox);
	BPopUpMenu	*popup = new BPopUpMenu("menu");
	BMenuItem	*it;
	popup->AddItem(it = new BMenuItem("All Workspaces", new BMessage(kAllWorkspaces)));
	popup->AddItem(it = new BMenuItem("Current Workspace", new BMessage(kCurrentWorkspace)));
	it->SetMarked(true);
	popup->AddSeparatorItem();
	popup->AddItem(it = new BMenuItem("Default folder", new BMessage(kDefaultFolder)));
	popup->AddItem(it = new BMenuItem("Other folder" B_UTF8_ELLIPSIS, new BMessage(kSelectedFolder)));
	BMenuField *applyto = new BMenuField(BRect(0, 0, 130, 18), "applyto", "", popup, (bool)true);
	popup->SetFont(&f);
	applyto->SetFont(&f);
	applyto->MenuBar()->SetFont(&f);
	applyto->SetDivider(0);
	setbox->SetLabel(applyto);

	BMenuField *image;
	BPopUpMenu *pu = new BPopUpMenu("");
	pu->SetTargetForItems(this);
	setbox->AddChild(image = new BMenuField(BRect(10, 25, 250, 45), "image", "Image:", pu));
	image->SetAlignment(B_ALIGN_RIGHT);
	image->SetDivider(70);

	BMenuField *place;
	pu = new BPopUpMenu("");
	pu->AddItem(new BMenuItem("Manual", new BMessage(kManualPlacement)));
	pu->AddItem(new BMenuItem("Center", new BMessage(kCenterPlacement)));
	pu->AddItem(new BMenuItem("Scale to fit", new BMessage(kScaleToFitPlacement)));
	pu->AddItem(new BMenuItem("Tile", new BMessage(kTilePlacement)));
	pu->SetTargetForItems(this);
	setbox->AddChild(place = new BMenuField(BRect(10, 50, 250, 70), "placement", "Placement:", pu));
	place->SetAlignment(B_ALIGN_RIGHT);
	place->SetDivider(70);

	setbox->AddChild(new BCheckBox(BRect(80, 75, 250, 95),
		"erase", "Icon label background", new BMessage(kEraseText)));

	setbox->AddChild(new MyColorControl(BPoint(10, 110), B_CELLS_32x8, 4, "backcolor", this));
	back->AddChild(new BButton(BRect(10, WY - 35, 90, WY - 10),
		"revert", "Revert", new BMessage(kRevert)));
	back->AddChild(new BButton(BRect(WX - 80, WY - 35, WX - 10, WY - 10),
		"apply", "Apply", new BMessage(kApply)));

	int32 i = 0;
//	// load recent image list
//	const char *imgpath;
//	while(prefs.FindString("recentimage", i++, &imgpath) == B_OK)
//		AddPath(recentimages, imgpath);

	// load recent folder list
	i = 0;
	const char *fldpath;
	while(prefs.FindString("recentfolder", i++, &fldpath) == B_OK)
		AddPath(recentfolders, fldpath);

	BuildFolderMenu();
	ApplyToSelection();
	if(settings.ApplyToCurrentWorkspace() == B_OK)
		ShowSettings();
	else
	{
		(new BAlert("Backgrounds", "Problem loading desktop settings", "OK", 0, 0,
			B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
		be_app->PostMessage(B_QUIT_REQUESTED);
	}
}

SetupWin::~SetupWin()
{
	delete imgpanel;
	delete dirpanel;
}

void SetupWin::WorkspaceActivated(int32 /*workspace*/, bool active)
{
	if(active)
	{
		settings.Revert();
		ShowSettings();
	}
}

// reload image to reflect workspace changes
void SetupWin::ScreenChanged(BRect, color_space)
{
	PreviewView *pv = dynamic_cast<PreviewView *>(FindView("preview"));
	float	scale;

	// wrapped in a block to minimize BScreen life
	{
		scale = (float)PREVX / BScreen().Frame().Width();
	}

	pv->SetScale(scale);

	BPath path = settings.GetImagePath();
	if(path.Path() != 0 && strlen(path.Path()) != 0)
		pv->LoadBitmap(path.Path());
}

void SetupWin::ShowSettings()
{
	PreviewView		*prev = dynamic_cast<PreviewView *>(FindView("preview"));
	BColorControl	*bcc = dynamic_cast<BColorControl *>(FindView("backcolor"));
	BMenuField		*placement = dynamic_cast<BMenuField *>(FindView("placement"));
	BCheckBox		*icon = dynamic_cast<BCheckBox *>(FindView("erase"));

	if(prev && bcc && placement && icon)
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
				prev->Manual(p.x, p.y);
				}
				break;

			case kCentered :
				prev->Center();
				break;

			case kScaledToFit :
				prev->ScaleToFit();
				break;

			case kTiled :
				prev->Tile();
				break;
		}
		ShowXY();

		icon->SetValue(settings.GetImageEraseText());

		// load image
		BPath path = settings.GetImagePath();
		BEntry file;
		if(path.Path() == 0 || strlen(path.Path()) == 0 ||
			file.SetTo(path.Path()) != B_OK || ! file.IsFile() ||
			! prev->LoadBitmap(path.Path()))
		{
			prev->ClearBitmap();
		}

		rgb_color col = settings.GetColor();
		prev->SetLowColor(col);
		prev->SetViewColor(col);
		bcc->SetValue(col);
	}
	CheckEnabled();
	CheckChanges();
}

void SetupWin::BuildImageMenu()
{
	BPath current = settings.GetImagePath();
	BMenuField	*image = dynamic_cast<BMenuField *>(FindView("image"));

	if(image)
	{
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
	}
}

bool SetupWin::QuitRequested()
{
	entry_ref	dir;
	BPath	path;
	BEntry	e;
	int32	count;
	int32	i;

	// regen prefs
	prefs.RemoveName("pos");
	prefs.AddPoint("pos", Frame().LeftTop());

	prefs.RemoveName("paneldir");
	imgpanel->GetPanelDirectory(&dir);
	e.SetTo(&dir);
	e.GetPath(&path);
	prefs.AddString("paneldir", path.Path());

	prefs.RemoveName("folderpaneldir");
	dirpanel->GetPanelDirectory(&dir);
	e.SetTo(&dir);
	e.GetPath(&path);
	prefs.AddString("folderpaneldir", path.Path());

	prefs.RemoveName("recentimage");
//	count = recentimages.CountItems();
//	if(count > 32) count = 32;
//	for(i = 0; i < count; i++)
//	{
//		BPath *curr = (BPath *)recentimages.ItemAt(i);
//		prefs.AddString("recentimage", curr->Path());
//	}

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
		prefs_name.Append("Backgrounds_settings") == B_OK &&
		prefs_file.SetTo(prefs_name.Path(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE) == B_OK)
	{
		prefs.Flatten(&prefs_file);
	}

#if 0
	bool		reallyquit = true;
	status_t	ret;

	if(settings.Changes())
	{
		int32 choice = (new BAlert("Backgrounds", "The settings haven't been applied", "Don't apply", "Cancel", "Apply",
			B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
		switch(choice)
		{
			case 1 :
				reallyquit = false;
				break;

			case 2 :
				if((ret = settings.Apply()) != B_NO_ERROR)
				{
					char buf[256];
					sprintf(buf, "There was an error setting the background attribute: %s", strerror(ret));
					(new BAlert("Backgrounds", buf, "OK", 0, 0, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
				}
				break;
		}
	}

	if(reallyquit)
	{
		be_app->PostMessage(B_QUIT_REQUESTED);
		return BWindow::QuitRequested();
	}
	else
		return false;
#endif

	be_app->PostMessage(B_QUIT_REQUESTED);
	return BWindow::QuitRequested();
}

void SetupWin::MessageReceived(BMessage *msg)
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
					PostMessage(&imgs);
				else
					PostMessage(&firstdir);
			}
			break;

		case B_CANCEL :
			{
			BFilePanel *which;
			if(msg->FindPointer("source", (void **)&which) == B_OK)
			{
				if(which == dirpanel && lastdirpanel)
				{
					// switch folder/workspace selection back to current
					lastdirpanel->SetMarked(true);
				} else if(which == imgpanel && lastimgpanel)
				{
					// switch image setting back to current
					lastimgpanel->SetMarked(true);
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

		case kRevert :
			settings.Revert();
			ShowSettings();
			break;

		case kLoadImage :
			{
			PreviewView *pv = dynamic_cast<PreviewView *>(FindView("preview"));
			if(pv)
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
						entry_ref dir;
						file.GetParent(&direntry);
						direntry.GetRef(&dir);
						imgpanel->SetPanelDirectory(&dir);

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
					if(lastimgpanel)
						lastimgpanel->SetMarked(true);

					beep();
				}
			}
			}
			break;

		case kManualPlacement :
		case kCenterPlacement :
		case kTilePlacement :
		case kScaleToFitPlacement :
			{
			PreviewView		*pv = dynamic_cast<PreviewView *>(FindView("preview"));

			if(pv)
			{
				switch(msg->what)
				{
					case kManualPlacement :
						{
						BPoint p = settings.GetImageOffset();
						pv->Manual(p.x, p.y);
						settings.SetImageMode(kAtOffset);
						}
						break;

					case kCenterPlacement :
						pv->Center();
						settings.SetImageMode(kCentered);
						settings.SetImageOffset(BPoint(0, 0));
						break;

					case kScaleToFitPlacement :
						pv->ScaleToFit();
						settings.SetImageMode(kScaledToFit);
						settings.SetImageOffset(BPoint(0, 0));
						break;

					case kTilePlacement :
						pv->Tile();
						settings.SetImageMode(kTiled);
						settings.SetImageOffset(BPoint(0, 0));
						break;
				}
				pv->Redisplay();
				ShowXY();
				CheckEnabled();
				CheckChanges();
			}
			}
			break;

		case kNoImage :
			{
			PreviewView		*pv = dynamic_cast<PreviewView *>(FindView("preview"));

			if(pv)
				pv->ClearBitmap();

			settings.SetImagePath(BPath());
			BuildImageMenu();
			ImageSelection();
			ShowXY();
			CheckChanges();
			CheckEnabled();
			}
			break;

		case kEraseText :
			{
			BCheckBox *icon = dynamic_cast<BCheckBox *>(FindView("erase"));
			if(icon)
			{
				settings.SetImageEraseText(icon->Value());
				CheckChanges();
			}
			}
			break;

		case kOpenFile :
			imgpanel->Show();
			break;

		case kLoadError :
			(new BAlert("Backgrounds", "There was an error loading the image", "OK", 0, 0,
				B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
			PostMessage(kNoImage);
			break;

		case kAllWorkspaces :
			{
			ApplyToSelection();
			BMenuField	*mf = dynamic_cast<BMenuField *>(FindView("applyto"));
			if(mf)
			{
				mf->MenuBar()->ItemAt(0)->SetLabel("All Workspaces");

				// mark current item
				BMenu	*m = mf->MenuBar()->ItemAt(0)->Submenu();
				int32 count = m->CountItems();
				for(int32 i = 0; i < count; i++)
					m->ItemAt(i)->SetMarked(i == 0);
			}

			settings.ApplyToAllWorkspaces();
			EnablePlacementItems(true);
			ShowSettings();
			ViewAs(MockupView::MOCKUP_MONITOR);
			}
			break;

		case kCurrentWorkspace :
			{
			ApplyToSelection();
			BMenuField	*mf = dynamic_cast<BMenuField *>(FindView("applyto"));
			if(mf)
			{
				mf->MenuBar()->ItemAt(0)->SetLabel("Current Workspace");

				// mark current item
				BMenu	*m = mf->MenuBar()->ItemAt(0)->Submenu();
				int32 count = m->CountItems();
				for(int32 i = 0; i < count; i++)
					m->ItemAt(i)->SetMarked(i == 1);
			}

			settings.ApplyToCurrentWorkspace();
			EnablePlacementItems(true);
			ShowSettings();
			ViewAs(MockupView::MOCKUP_MONITOR);
			}
			break;

		case kDefaultFolder :
			{
			ApplyToSelection();
			BMenuField	*mf = dynamic_cast<BMenuField *>(FindView("applyto"));
			if(mf)
			{
				mf->MenuBar()->ItemAt(0)->SetLabel("Default Folder");

				// mark current item
				BMenu	*m = mf->MenuBar()->ItemAt(0)->Submenu();
				int32 count = m->CountItems();
				for(int32 i = 0; i < count; i++)
					m->ItemAt(i)->SetMarked(i == 3);
			}

			settings.ApplyToDefaultFolder();
			EnablePlacementItems(false);
			ShowSettings();
			ViewAs(MockupView::MOCKUP_FOLDER);
			}
			break;

		case kSelectedFolder :
			dirpanel->Show();
			break;

		case kLoadDir :
			{
			bool		found = false;
			entry_ref	ref;
			BEntry		file;
			BPath		path;
			BPath		desktop;
			int32		i = 0;

			lastdirpanel = 0;

			while(msg->FindRef("refs", i++, &ref) == B_OK)
			{
				file.SetTo(&ref, true);
				if(file.IsDirectory())
				{
					file.GetPath(&path);

					if(find_directory(B_DESKTOP_DIRECTORY, &desktop) == B_OK &&
						path == desktop)
					{
						if(! found)
						{
							found = true;
							PostMessage(kCurrentWorkspace);
						}
						continue;
					}

					AddPath(recentfolders, path);
					BuildFolderMenu();

					if(! found)
					{
						BMessage fmsg(kOpenDir);
						fmsg.AddString("path", path.Path());
						PostMessage(&fmsg);
						found = true;
					}
				}
			}

			if(! found)
			{
				(new BAlert("Backgrounds", "There was an error opening the selected folder", "OK", 0, 0,
					B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
				PostMessage(kDefaultFolder);
			}
			}
			break;

		case kOpenDir :
			{
			ApplyToSelection();
			BMenuField	*mf = dynamic_cast<BMenuField *>(FindView("applyto"));
			const char	*folder;
			entry_ref	ref;

			if(mf &&
				msg->FindString("path", &folder) == B_OK)
			{
				BPath	folderpath(folder);
				BPath	desktop;

				if(find_directory(B_DESKTOP_DIRECTORY, &desktop) == B_OK &&
					folderpath == desktop)
				{
					PostMessage(kCurrentWorkspace);
					break;
				}

				if(settings.ApplyToFolder(&folderpath) == B_OK)
				{
					BMenu	*m = mf->MenuBar()->ItemAt(0)->Submenu();

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

					EnablePlacementItems(false);
					ShowSettings();

					// switch viewing mode
					ViewAs(MockupView::MOCKUP_FOLDER);
				}
				else
				{
					(new BAlert("Backgrounds", "There was an error opening the selected folder", "OK", 0, 0,
						B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
					BuildFolderMenu();
					PostMessage(kDefaultFolder);
				}
			}
			}
			break;

		case kXChanged :
			{
			BTextControl *x = dynamic_cast<BTextControl *>(FindView("x"));
			PreviewView *pv = dynamic_cast<PreviewView *>(FindView("preview"));
			if(x && pv)
			{
				BPoint	p = settings.GetImageOffset();
				int32	xc = atol(x->Text());
				int32	sw = BScreen(this).Frame().IntegerWidth();
				if(xc < 0) xc = 0;
				if(xc > sw) xc = sw;
				BPoint	newp = BPoint(xc, p.y);
				settings.SetImageOffset(newp);
				pv->Manual(newp.x, newp.y);
				pv->Redisplay();
				ChangePosition(newp);
				CheckChanges();
			}
			}
			break;

		case kYChanged :
			{
			BTextControl *y = dynamic_cast<BTextControl *>(FindView("y"));
			PreviewView *pv = dynamic_cast<PreviewView *>(FindView("preview"));
			if(y && pv)
			{
				BPoint	p = settings.GetImageOffset();
				int32	yc = atol(y->Text());
				int32	sh = BScreen(this).Frame().IntegerHeight();
				if(yc < 0) yc = 0;
				if(yc > sh) yc = sh;
				BPoint	newp = BPoint(p.x, yc);
				settings.SetImageOffset(newp);
				pv->Manual(newp.x, newp.y);
				pv->Redisplay();
				ChangePosition(newp);
				CheckChanges();
			}
			}
			break;

		default :
			BWindow::MessageReceived(msg);
			break;
	}
}

void SetupWin::EnablePlacementItems(bool enable)
{
	BMenuField		*placement = dynamic_cast<BMenuField *>(FindView("placement"));
	BColorControl	*bcc = dynamic_cast<BColorControl *>(FindView("backcolor"));

	if(placement && bcc)
	{
		BMenu	*m = placement->MenuBar()->ItemAt(0)->Submenu();
		m->ItemAt(1)->SetEnabled(enable);
		m->ItemAt(2)->SetEnabled(enable);
		bcc->SetEnabled(enable);
	}
}

void SetupWin::CheckEnabled()
{
	BMenuField		*placement = dynamic_cast<BMenuField *>(FindView("placement"));
	BCheckBox		*erase = dynamic_cast<BCheckBox *>(FindView("erase"));
	BTextControl	*x = dynamic_cast<BTextControl *>(FindView("x"));
	BTextControl	*y = dynamic_cast<BTextControl *>(FindView("y"));
	BTextView		*xv = dynamic_cast<BTextView *>(x ? x->ChildAt(0) : 0);
	BTextView		*yv = dynamic_cast<BTextView *>(y ? y->ChildAt(0) : 0);

	if(placement && erase && x && y)
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

void SetupWin::CheckChanges()
{
	BButton	*apply = dynamic_cast<BButton *>(FindView("apply"));
	BButton	*revert = dynamic_cast<BButton *>(FindView("revert"));

	if(apply && revert)
	{
		bool changes = settings.Changes();
		apply->SetEnabled(changes);
		revert->SetEnabled(changes);
	}
	else
		beep();
}

void SetupWin::ChangeColor(rgb_color col)
{
	PreviewView *pv = dynamic_cast<PreviewView *>(FindView("preview"));
	if(pv)
	{
		pv->SetViewColor(col);
		pv->Invalidate();
	}
	settings.SetColor(col);
	CheckChanges();
}

void SetupWin::ChangePosition(BPoint p)
{
	BTextControl	*x = dynamic_cast<BTextControl *>(FindView("x"));
	BTextControl	*y = dynamic_cast<BTextControl *>(FindView("y"));

	if(x && y)
	{
		char	tmp[30];
		sprintf(tmp, "%d", (int)p.x);
		x->SetText(tmp);
		sprintf(tmp, "%d", (int)p.y);
		y->SetText(tmp);
	}
	settings.SetImageOffset(p);
	CheckChanges();
}

void SetupWin::ViewAs(MockupView::mockup_t mode)
{
	MockupView	*v = dynamic_cast<MockupView *>(FindView("mockup"));
	if(v)
		v->SetMode(mode);
}

void SetupWin::ImageSelection()
{
	BMenuField	*image = dynamic_cast<BMenuField *>(FindView("image"));
	if(image)
	{
		BMenu	*m = image->MenuBar()->ItemAt(0)->Submenu();
		lastimgpanel = m->FindMarked();
	}
}

void SetupWin::ApplyToSelection()
{
	BMenuField	*apply = dynamic_cast<BMenuField *>(FindView("applyto"));
	if(apply)
	{
		BMenu	*m = apply->MenuBar()->ItemAt(0)->Submenu();
		lastdirpanel = m->FindMarked();
	}
}

void SetupWin::ShowXY()
{
	BTextControl	*x = dynamic_cast<BTextControl *>(FindView("x"));
	BTextControl	*y = dynamic_cast<BTextControl *>(FindView("y"));

	if(x && y)
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
}

void SetupWin::BuildFolderMenu()
{
	BMenuField	*mf = dynamic_cast<BMenuField *>(FindView("applyto"));
	if(mf)
	{
		BMenu	*m = mf->MenuBar()->ItemAt(0)->Submenu();
		int32	count = m->CountItems();
		char tmp[256];

		for(int32 i = count - 1; i > 4; i--)
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
	}
}

