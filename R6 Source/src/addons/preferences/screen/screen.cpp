#include "screen.h"
#include "screen_utils.h"
#include "TimedAlert.h"
#include "ConfirmWindow.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <Alert.h>
#include <Button.h>
#include <Debug.h>
#include <CheckBox.h>
#include <FindDirectory.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <Message.h>
#include <MessageFilter.h>
#include <Path.h>
#include <Resources.h>
#include <Screen.h>
#include <StringView.h>

enum {
	MMV_SET_MODE = 'mvSM',
	MMV_RESTORE_MODE
};

bool gShowPoofDialog;

TRefreshSlider::TRefreshSlider(BRect frame, BMessage* message)
	: BSlider(frame, "slider", "Refresh Rate", message, 0, 999, B_BLOCK_THUMB)
{
	fKeyBuffer[0] = fKeyBuffer[1] = fKeyBuffer[2] = 0;	
}

void TRefreshSlider::SetRate(float rate)
{
	SetValue((int32)(((rate - fBottom) / (fTop - fBottom)) * 1000. + 0.5));
}

void TRefreshSlider::SetFreqs(float bottom, float top)
{
	char str1[16], str2[16];
	
	fTop = top; fBottom = bottom;

	sprintf(str1, "%.1f", bottom);
	sprintf(str2, "%.1f", top);
	SetLimitLabels(str1, str2);
}

char *TRefreshSlider::UpdateText() const
{
	char *s = const_cast<char *>(str);
	sprintf(s, "%.1f", Rate());
	return s;
}

void 
TRefreshSlider::KeyDown(const char *bytes, int32 numBytes)
{
	//	do a simple type ahead
	if (isdigit(bytes[0])){	
		if (fLastTime > system_time()) {
			fKeyBuffer[0] = fKeyBuffer[1];			
			fKeyBuffer[1] = bytes[0];
		} else {	// start over
			fKeyBuffer[0] = '0';
			fKeyBuffer[1] = bytes[0];
		}
		fKeyBuffer[2] = 0;

		int32 v = (atoi(fKeyBuffer) - (int32)fBottom) * 10;
		int32	minimum, maximum;
		GetLimits(&minimum, &maximum);
		if (v >= minimum && v <= maximum) {
			SetValue(v);
			Invoke(Message());
		}
		fLastTime = system_time() + 500000;
		return;
	}

	BSlider::KeyDown(bytes, numBytes);
}

void TRefreshSlider::SetValue(int32 v)
{
	int32	minimum, maximum;
	GetLimits(&minimum, &maximum);
	if (v == Value() || v < minimum || v > maximum)
		return;
		
	BSlider::SetValue(v);	
}

float TRefreshSlider::Rate() const
{
	return ((float)Value() / 1000.) * (fTop - fBottom) + fBottom;
}

//****************************************************************************************
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

//************************************************************************************

static int
SettingsFileRef(const char* fileName, char* path, bool create)
{
	BPath	filePath;
	if (find_directory (B_USER_SETTINGS_DIRECTORY, &filePath) == B_OK) {
		int ref = -1;
		
		filePath.Append(fileName);
		
		ref = open(filePath.Path(), O_RDWR);
		
		if (ref < 0 && create) {
			ref = creat(filePath.Path(), 0777);
			if (ref < 0) 
				goto FAIL;
		} 
		strcpy(path, filePath.Path());
		
		return ref;
	}
	
FAIL:
	path[0] = 0;

	return -1;
}

// ************************************************************************** //

const int32 msg_default_resolution 		= 'dres';
const int32 msg_default_refreshrate 	= 'dref';
const int32 msg_default_position 		= 'dpos';

const int32 msg_revert 					= 'rvrt';
const int32 msg_defaults 				= 'dflt';
const int32 msg_save_settings 			= 'sset';
const int32 msg_use_settings 			= 'uset';
const int32 msg_use_all 				= 'usal';

const int32 msg_ws_all 					= 'wsal';

const char* const kSettingsFile = "Screen_data";

// ************************************************************************** //

const int32 kViewWidth = 400;
const int32 kViewHeight = 330;

TBox::TBox()
 : BView(BRect(0, 0, kViewWidth, kViewHeight), "box", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW)
{
	advancedmode = false;
	gShowPoofDialog = true;
	GetPrefs();

	// initialize screen settings
	int32			wscount = count_workspaces();
	BScreen			s;
	s.GetMode(&current);
	for(int32 i = 0; i < wscount; i++)
		s.GetMode(i, &initial[i]);
	fAllWorkspaces = false;
	AddParts();	

	//	add shortcuts for
	//		default resolution
	//		default refresh rate
	//		default monitor position
	//		workspace count dialog
//	Window()->AddShortcut('D', B_COMMAND_KEY, new BMessage(msg_default_resolution), this);
//	Window()->AddShortcut('R', B_COMMAND_KEY, new BMessage(msg_default_refreshrate), this);
//	Window()->AddShortcut('P', B_COMMAND_KEY, new BMessage(msg_default_position), this);
//	Window()->AddShortcut('D', B_SHIFT_KEY, new BMessage(msg_defaults), this);
//	Window()->AddShortcut('R', B_SHIFT_KEY, new BMessage(msg_revert), this);
//	Window()->AddShortcut('S', B_COMMAND_KEY, new BMessage(msg_save_settings), this);
//	Window()->AddShortcut('U', B_COMMAND_KEY, new BMessage(msg_use_settings), this);
//	Window()->AddShortcut('U', B_SHIFT_KEY, new BMessage(msg_use_all), this);
}

TBox::~TBox()
{
	SetPrefs();
}

void TBox::GetPrefs()
{
	char	path[B_PATH_NAME_LENGTH];
	int		fileRef = SettingsFileRef(kSettingsFile, path, false);
	
	if(fileRef >= 0)
	{
		BPoint	unusedpt;
		bool	unusedbool;

		read(fileRef, &unusedpt, sizeof(BPoint));	// window pos
		read(fileRef, &unusedbool, sizeof(bool));	// applyCustomNow
		if(read(fileRef, &gShowPoofDialog, sizeof(bool)) != sizeof(bool))
			gShowPoofDialog = true;
		
		close(fileRef);
	}
}

void TBox::SetPrefs()
{
	char path[B_PATH_NAME_LENGTH];

	int fileRef = SettingsFileRef(kSettingsFile, path, true);
	if (fileRef >= 0) {
		BPoint loc(0, 0);	// window pos
		write(fileRef, &loc, sizeof(BPoint));
		bool flag = false;	// applyCustomNow
		write(fileRef, &flag, sizeof(bool));
		write(fileRef, &gShowPoofDialog, sizeof(bool));
		
		close(fileRef);
	}				
}

void TBox::AttachedToWindow()
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fTargetMenu->SetTargetForItems(this);
	fApplyBtn->SetTarget(this);
	fRevertBtn->SetTarget(this);
	fDefaultsBtn->SetTarget(this);
	up->SetTarget(this);
	down->SetTarget(this);
	left->SetTarget(this);
	right->SetTarget(this);
	vsync->SetTarget(this);
	hsync->SetTarget(this);
	green->SetTarget(this);
	fRefreshRateSlider->SetTarget(this);

	for(int32 i = 0; i < MM_COLUMN_COUNT; i++)
		col[i]->SetTargetForItems(this);
}

void TBox::MessageReceived(BMessage* msg)
{
	switch(msg->what)
	{
		case B_WORKSPACE_ACTIVATED :
			{
			long space;
			bool act;
			msg->FindInt32("workspace", &space);
			msg->FindBool("active", &act);
			WorkspaceActivated(space, act);
			TweakEasy();
			}
			break;

		case MM_ITEM_SELECTED :
			{
			size_t column;
			msg->FindInt32("type", (int32 *)&column);
			column = mm.ColumnOf((ColumnType)column);
			size_t item;
			msg->FindInt32("item", (int32 *)&item);

			// load best match predicates with old display mode
			Predicate *p[MM_COLUMN_COUNT];
			p[0] = new DisplayShapeClosest(&current);
			p[1] = new VirtualShapeClosest(&current);
			p[2] = new PixelConfigClosest(&current);
			p[3] = new RefreshRateClosest(&current);
			p[4] = new OtherParamsClosest(&current);

			// new display mode
			current = mm.ModeList()[mm.MenuItems(column)->second[item]];
			mm.Select(column, item);

			const display_mode *dm = mm.ModeList();

			size_t count;
			BMenuItem *mitem;
			while(++column < MM_COLUMN_COUNT)
			{
				int32 closest = 0;

				// empty the current menu
				col[column]->RemoveItems(0, col[column]->CountItems(), true);

				// recreate the menu choices
				count = mm.ItemsInColumn(column);
				for(size_t j = 0; j < count; j++)
				{
					msg = new BMessage(MM_ITEM_SELECTED);
					msg->AddInt32("type", mm.TypeOf(column));
					msg->AddInt32("item", j);
					mitem = new BMenuItem(mm.LabelForColumnItem(column, j), msg);

					if(p[column] && (*p[column])(dm + mm.MenuItems(column)->second[j]))
						closest = j;

					col[column]->AddItem(mitem);
				}

				col[column]->ItemAt(closest)->SetMarked(true);
				menu[column]->SetEnabled(count > 1);
				col[column]->SetTargetForItems(this);

				// update state
				mm.Select(column, closest);
				current = mm.ModeList()[mm.MenuItems(column)->second[closest]];
			}
			delete p[0];
			delete p[1];
			delete p[2];
			delete p[3];
			delete p[4];
			CheckChanges();
			TweakEasy();
			UpdateAdvancedControls();
			}
			break;

		case MMV_SET_MODE :
			{
			if(fAllWorkspaces)
			{
				BAlert *a = new BAlert("Confirm Mode Change","Change all workspaces? This action cannot be reverted.",
					"Okay", "Cancel", NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT );
			
				if(a->Go() == 1)
					break;
			}

			if(gShowPoofDialog && current.timing.pixel_clock > 90000L)
			{
				TPoofAlert	*a = new TPoofAlert();
				if(a->Go() == 1)
					break;
				else
					//	disable the dialog from here on
					gShowPoofDialog = !a->Skip();
			}

			BScreen s(Window());
			display_mode previous;
			s.GetMode(&previous);
printf("old mode\n");
dump_mode(&previous);

printf("new mode\n");
dump_mode(&current);

			if(s.SetMode(&current, false) == B_OK)
			{
				TTimedAlert *okcancel = new TTimedAlert(10LL * 1000LL * 1000LL, 0, "Confirm Mode Change", "Do you wish to keep these settings?\nSettings will be reverted in %ld seconds", "Revert", "Keep", 0, B_WIDTH_AS_USUAL, B_OFFSET_SPACING);
				okcancel->SetFeel(B_MODAL_ALL_WINDOW_FEEL);
				okcancel->SetFlags(Flags() | 0x00100000);
				okcancel->SetShortcut(0, B_ESCAPE);
				okcancel->SetShortcut(1, 13);
				if(okcancel->Go() == 1)
				{
					if(fAllWorkspaces)
					{
						int32	wscount = count_workspaces();
						for(int32 i = 0; i < wscount; i++)
							s.SetMode(i, &current, true);
					}
					else
						s.SetMode(&current, true);
				}
				else
					// restore previous mode
					s.SetMode(&previous);
			}

			CheckChanges();
			TweakEasy();
			}
			break;

		case 'up  ' :
			{
			display_mode *dm = TweakAdvanced();
			dm->timing.v_sync_start -= 1;
			dm->timing.v_sync_end -= 1;
			display_mode low = *dm;
			display_mode high = *dm;
			low.flags = low.timing.flags = 0;
			high.flags = high.timing.flags = 0xffffffff;
			BScreen s;
			if(s.ProposeMode(dm, &low, &high) != B_ERROR)
			{
				s.SetMode(dm);
				current = *dm;
			}
			UpdateControls();
			UpdateAdvancedControls();
			CheckChanges();
			}
			break;

		case 'down' :
			{
			display_mode *dm = TweakAdvanced();
			dm->timing.v_sync_start += 1;
			dm->timing.v_sync_end += 1;
			display_mode low = *dm;
			display_mode high = *dm;
			low.flags = low.timing.flags = 0;
			high.flags = high.timing.flags = 0xffffffff;
			BScreen s;
			if(s.ProposeMode(dm, &low, &high) != B_ERROR)
			{
				s.SetMode(dm);
				current = *dm;
			}
			UpdateControls();
			UpdateAdvancedControls();
			CheckChanges();
			}
			break;

		case 'left' :
			{
			display_mode *dm = TweakAdvanced();
			dm->timing.h_sync_start += 8;
			dm->timing.h_sync_end += 8;
			display_mode low = *dm;
			display_mode high = *dm;
			low.flags = low.timing.flags = 0;
			high.flags = high.timing.flags = 0xffffffff;
			BScreen s;
			if(s.ProposeMode(dm, &low, &high) != B_ERROR)
			{
				s.SetMode(dm);
				current = *dm;
			}
			UpdateControls();
			UpdateAdvancedControls();
			CheckChanges();
			}
			break;

		case 'rght' :
			{
			display_mode *dm = TweakAdvanced();
			dm->timing.h_sync_start -= 8;
			dm->timing.h_sync_end -= 8;
			display_mode low = *dm;
			display_mode high = *dm;
			low.flags = low.timing.flags = 0;
			high.flags = high.timing.flags = 0xffffffff;
			BScreen s;
			if(s.ProposeMode(dm, &low, &high) != B_ERROR)
			{
				s.SetMode(dm);
				current = *dm;
			}
			UpdateControls();
			UpdateAdvancedControls();
			CheckChanges();
			}
			break;

		case msg_revert:					// cmd-shift R
			TweakEasy();
			Revert();
			break;

		case msg_defaults:					// cmd-shift D
			TweakEasy();
			Defaults();
			break;			
		
		//	workspace
		case msg_ws_all:					// all or current ws
			{
			int32 value;
			if(msg->FindInt32("workspace", &value) == B_OK)
			{
				fAllWorkspaces = value ? true : false;
				CheckChanges();
			}
			}
			break;

		default:
			BView::MessageReceived(msg);
			break;
	}
}

void TBox::WorkspaceActivated(int32 , bool state)
{
	if(state)
	{
		BScreen().GetMode(&current);
		UpdateControls();
		UpdateAdvancedControls();
		CheckChanges();
	}
}

void TBox::AddParts()
{
	float wx = Bounds().Width();
	float wy = Bounds().Height();

	// TOP BAR
	LineView	*topbar = new LineView(BRect(0, 0, wx, 24));
	AddChild(topbar);
	fTargetMenu = new BPopUpMenu("menu");
	BMenuItem	*it;
	BMessage	*msg;
	msg = new BMessage(msg_ws_all);
	msg->AddInt32("workspace", 1);
	fTargetMenu->AddItem(it = new BMenuItem("All Workspaces", msg));
	msg = new BMessage(msg_ws_all);
	msg->AddInt32("workspace", 0);
	fTargetMenu->AddItem(it = new BMenuItem("Current Workspace", msg));
	it->SetMarked(true);
	BMenuField *fTargetBtn;
	fTargetBtn = new BMenuField(BRect(5, 0, 130, 18), "target", "", fTargetMenu, (bool)true);
	fTargetBtn->SetDivider(0);
	topbar->AddChild(fTargetBtn);

	// MINI SCREEN
#define PREVX 110
#define PREVY 82

	MiniScreenView *miniview = new MiniScreenView(BRect(11, 32, 15 + PREVX, 36 + PREVY));
	miniscreen = new BView(BRect(2, 2, 2 + PREVX, 2 + PREVY), "screen", 0, 0);
	miniview->AddChild(miniscreen);
	AddChild(miniview);

	mm.RepopulateModeList();
	BRect r(140, 30, Bounds().Width(), 50);

	const display_mode *dm = mm.ModeList();
#if 0
	size_t	count = mm.ModeCount();
	for(size_t i = 0; i < count; i++)
		dump_mode(dm++);
#endif

	for(size_t i = 0; i < MM_COLUMN_COUNT; i++)
	{
		size_t count = mm.ItemsInColumn(i);
		col[i] = new BPopUpMenu("");
		for(size_t j = 0; j < count; j++)
		{
			msg = new BMessage(MM_ITEM_SELECTED);
			msg->AddInt32("type", mm.TypeOf(i));
			msg->AddInt32("item", j);
			BMenuItem *mitem = new BMenuItem(mm.LabelForColumnItem(i, j), msg);

			Predicate *p = 0;

			switch(mm.TypeOf(i))
			{
				case DisplayShape:
					p = new DisplayShapePredicate(&current);
					break;
				case VirtualShape:
					p = new VirtualShapePredicate(&current);
					break;
				case RefreshRate:
					p = new RefreshRatePredicate(&current);
					break;
				case PixelConfig:
					p = new PixelConfigPredicate(&current);
					break;
				case OtherParams:
					p = new OtherParamsPredicate(&current);
					break;
				default :
					p = 0;
					break;
			}

			if(p && (*p)(dm + mm.MenuItems(i)->second[j]))
			{
				mitem->SetMarked(true);
				mm.Select(i, j);
			}

			delete p;
				
			col[i]->AddItem(mitem);
		}
		const char *s = mm.HeaderForColumn(i);
		menu[i] = new BMenuField(r, s, s, col[i]);
		menu[i]->SetEnabled(count > 1);
		menu[i]->SetAlignment(B_ALIGN_RIGHT);
		menu[i]->SetDivider(80);
		AddChild(menu[i]);
		r.OffsetBy(0, r.Height() + 3);
	}

	fRefreshRateSlider = new TRefreshSlider(BRect(5, 140, 190, 190), new BMessage('refr'));
	fRefreshRateSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fRefreshRateSlider->SetHashMarkCount(10);
	AddChild(fRefreshRateSlider);

	hsync = new BCheckBox(BRect(5, 195, 190, 210), "hsync", "Positive horizontal SYNC", new BMessage('hsyn'));
	AddChild(hsync);
	vsync = new BCheckBox(BRect(5, 215, 190, 230), "vsync", "Positive vertical SYNC", new BMessage('vsyn'));
	AddChild(vsync);
	green = new BCheckBox(BRect(200, 215, 380, 230), "green", "Sync on green", new BMessage('gren'));
	AddChild(green);

	up = new BButton(BRect(260, 150, 320, 170), "up", "Up", new BMessage('up  '));
	AddChild(up);
	down = new BButton(BRect(260, 180, 320, 200), "down", "Down", new BMessage('down'));
	AddChild(down);
	left = new BButton(BRect(200, 165, 250, 185), "left", "Left", new BMessage('left'));
	AddChild(left);
	right = new BButton(BRect(330, 165, 380, 185), "right", "Right", new BMessage('rght'));
	AddChild(right);

	// BOTTOM
	LineView	*bottombar = new LineView(BRect(0, wy - 45, wx, wy - 26));
	AddChild(bottombar);

	AddChild(fDefaultsBtn = new BButton(BRect(5, wy - 25, 75, wy),
		"defaults", "Defaults", new BMessage(msg_defaults)));
	AddChild(fRevertBtn = new BButton(BRect(85, wy - 25, 155, wy),
		"revert", "Revert", new BMessage(msg_revert)));
	AddChild(fApplyBtn = new BButton(BRect(wx - 70, wy - 25, wx, wy),
		"apply", "Apply", new BMessage(MMV_SET_MODE)));

	UpdateControls();
	UpdateAdvancedControls();
	CheckChanges();
}

void TBox::UpdateControls()
{
	// put current resolution through menus
	const display_mode *dm = mm.ModeList();
	bool found;
	for(size_t i = 0; i < MM_COLUMN_COUNT; i++)
	{
		found = false;

		// empty the current menu
		col[i]->RemoveItems(0, col[i]->CountItems(), true);

		// recreate the menu choices
		size_t count = mm.ItemsInColumn(i);
		for(size_t j = 0; j < count; j++)
		{
			BMessage *msg = new BMessage(MM_ITEM_SELECTED);
			msg->AddInt32("type", mm.TypeOf(i));
			msg->AddInt32("item", j);
			BMenuItem *mitem = new BMenuItem(mm.LabelForColumnItem(i, j), msg);

			Predicate *p = 0;

			switch(mm.TypeOf(i))
			{
				case DisplayShape:
					p = new DisplayShapePredicate(&current);
					break;
				case VirtualShape:
					p = new VirtualShapePredicate(&current);
					break;
				case RefreshRate:
					p = new RefreshRatePredicate(&current);
					break;
				case PixelConfig:
					p = new PixelConfigPredicate(&current);
					break;
				case OtherParams:
					p = new OtherParamsPredicate(&current);
					break;
				default :
					p = 0;
					break;
			}

			if(p && (*p)(dm + mm.MenuItems(i)->second[j]))
			{
				mitem->SetMarked(true);
				mm.Select(i, j);
				found = true;
			}

			delete p;

			col[i]->AddItem(mitem);
		}
		if(! found)
		{
			col[i]->ItemAt(0)->SetMarked(true);
			mm.Select(i, 0);
		}
		menu[i]->SetEnabled(count > 1);
		col[i]->SetTargetForItems(this);
	}

	// 	Mini Screen thing
//	miniscreen->SetResolution(fResolution);
	miniscreen->SetViewColor(BScreen().DesktopColor());
}

void TBox::UpdateAdvancedControls()
{
	// selected
	display_mode current = mm.ModeList()[mm.MenuItems(MM_COLUMN_COUNT - 1)->second[mm.Selected(MM_COLUMN_COUNT - 1)]];

	// set advanced settings stuff
	uint32	low, high;
	BScreen().GetPixelClockLimits(&current, &low, &high);
	double min_refresh_rate = (low * 1000.0) / (double)(current.timing.h_total * current.timing.v_total);
	double max_refresh_rate = (high * 1000.0) / (double)(current.timing.h_total * current.timing.v_total);
	if(max_refresh_rate > 180)
		max_refresh_rate = 180;
	fRefreshRateSlider->SetFreqs(min_refresh_rate, max_refresh_rate);
	fRefreshRateSlider->SetRate(rate_from_display_mode(&current));
}

display_mode *TBox::TweakAdvanced()
{
	if(! advancedmode)
	{
		// an advanced mode parameter has been set, let's add a control for it
		advancedmode = true;
		advancedentry = mm.AddEntry();
		*advancedentry = current;
	}

	return advancedentry;
}

void TBox::TweakEasy()
{
	advancedmode = false;

	// TODO remove advanced mode if it's not different
}

void TBox::CheckChanges()
{
	display_mode	m[32];
	int32			currentws = current_workspace();
	int32			wscount = count_workspaces();
	BScreen			s;
	bool			changed;

	for(int32 i = 0; i < wscount; i++)
		s.GetMode(i, &m[i]);

	if(fAllWorkspaces)
	{
		changed = false;
		for(int32 i = 0; i < wscount; i++)
		{
			if(! modes_match(&m[i], &current))
			{
				changed = true;
				break;
			}
		}
	}
	else
		changed = ! modes_match(&m[currentws], &current);

	if(changed)
		fApplyBtn->SetEnabled(true);
	else
		fApplyBtn->SetEnabled(false);

	if(! modes_match(&current, &initial[currentws]))
		fRevertBtn->SetEnabled(true);
	else
		fRevertBtn->SetEnabled(false);
}

void TBox::Revert()
{
	memcpy(&current, &initial[current_workspace()], sizeof(display_mode));
	UpdateControls();
	UpdateAdvancedControls();
	CheckChanges();
}

void TBox::Defaults()
{
	// pick the first item in the list
	memcpy(&current, mm.ModeList(), sizeof(display_mode));
	UpdateControls();
	UpdateAdvancedControls();
	CheckChanges();
}
