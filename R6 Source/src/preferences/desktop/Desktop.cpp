//--------------------------------------------------------------------
//	
//	Desktop.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1995 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <stdio.h>
#ifndef _OS_H
#include <OS.h>
#endif
#ifndef DESKTOP_H
#include "Desktop.h"
#endif
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

bool			lava_desk = FALSE;
TDesktopView	*desktop_view;

//====================================================================

int main()
{	
	TDesktopApplication	*myApplication;

	myApplication = new TDesktopApplication();
	myApplication->Run();

	delete myApplication;
	return B_NO_ERROR;
}

//--------------------------------------------------------------------

TDesktopApplication::TDesktopApplication()
		  :BApplication('DESK')
{
	int				ref;
	BMenuItem		*item;
	BRect			r;
	TDesktopWindow	*myDesktopWindow;
	screen_info 	sInfo;

	get_screen_info(0, &sInfo);

	fMenu = new BPopUpMenu("Desktop", FALSE, FALSE);
	item = new BMenuItem("About Desktop...", new BMessage(B_ABOUT_REQUESTED));
	fMenu->AddItem(item);

	item = new BMenuItem("Lava Desk", new BMessage(LAVA_DESK), 'L');
	fMenu->AddItem(item);

	fMenu->AddSeparatorItem();

	item = new BMenuItem("Quit Desktop", new BMessage(B_QUIT_REQUESTED), 'Q');
	fMenu->AddItem(item);

	SetMainMenu(fMenu);

	if ((ref = open("/boot/system/settings/Desktop_data", 0)) >= 0) {
		read(ref, &r, sizeof(BRect));
		close(ref);
	}
	if ((ref < 0) || (!sInfo.frame.Contains(r.LeftTop())))
		r.Set(BROWSER_WIND, TITLE_BAR_HEIGHT,
			  BROWSER_WIND + (CELL_WIDTH * CELLS_WIDE) + (WIND_BORDER * 2),
			  TITLE_BAR_HEIGHT + (CELL_HEIGHT * CELLS_TALL) + (WIND_BORDER * 2) + 1 + (3 * 13));
	myDesktopWindow = new TDesktopWindow(r, "Desktop");
	myDesktopWindow->Show();
}

//--------------------------------------------------------------------

void TDesktopApplication::AboutRequested()
{
	char				about[32] = "...by Robert Polic";
	BAlert				*myAlert;

	myAlert = new BAlert("",about,"Big Deal");
	myAlert->Go();
}

//--------------------------------------------------------------------

void TDesktopApplication::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case LAVA_DESK:
		case LAVA_DESK_QUIT:
			if (lava_desk) {
				lava_desk = FALSE;
				desktop_view->Window()->Lock();
				desktop_view->SetColor(fCurrentColor, FALSE);
				desktop_view->Window()->Unlock();
				if (msg->what == LAVA_DESK_QUIT)
					be_app->PostMessage(B_QUIT_REQUESTED);
			}
			else {
				fCurrentColor = desktop_view->fTheColor;
				lava_desk = TRUE;
			}
			BMenuItem *item = fMenu->FindItem("Lava Desk");
			if (!item)
				return;
			item->SetMarked(lava_desk);
			break;

		default:
			break;
	}
}


//====================================================================

TDesktopWindow::TDesktopWindow(BRect rect, char *title)
	          :BWindow(rect, title, B_TITLED_WINDOW, B_WILL_FLOAT)
{
	BRect	r;

	r = Frame();
	r.OffsetTo(0, 0);
	desktop_view = new TDesktopView(r, "DesktopView");
	desktop_view->FrameResized(rect.Width() + 1, rect.Height() + 1);
	AddChild(desktop_view);
	SetSizeLimits(50, 32767, 69 + 1 + (3 * 13), 32767);
}

//--------------------------------------------------------------------

bool TDesktopWindow::QuitRequested()
{
	int		ref;
	BRect	r;

	if (lava_desk) {
		be_app->PostMessage(LAVA_DESK_QUIT);
		return FALSE;
	}
	else {
		r = Frame();
		mkdir("/boot/system/settings", 0777);
		if ((ref = creat("/boot/system/settings/Desktop_data", 0777)) >= 0) {
			write(ref, &r, sizeof(BRect));
			close(ref);
		}

		be_app->PostMessage(B_QUIT_REQUESTED);
		return TRUE;
	}
}


//====================================================================

TDesktopView::TDesktopView(BRect rect, char *title)
	   :BView(rect, title, B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE)
{
}

//--------------------------------------------------------------------

TDesktopView::~TDesktopView()
{
	long	result;

	fDie = TRUE;
	wait_for_thread(fThreadID, &result);
}

//--------------------------------------------------------------------

void TDesktopView::AttachedToWindow()
{
	fTheColor = index_for_color(desktop_color());
	SetFontName("Erich");
	SetFontSize(12);

	fDie = FALSE;
	fThreadID = spawn_thread(this->LavaThread, "LAVA", B_DISPLAY_PRIORITY, this);
	resume_thread(fThreadID);
}

//--------------------------------------------------------------------

void TDesktopView::Draw(BRect where)
{
	long		i, j, colorIndex = 0;
	BRect		colorRect;
	long		textSize;
	BRegion		*clip;
	rgb_color	c;

	clip = new BRegion();
	GetClippingRegion(clip);

	c.red = 0;
	c.green = 0;
	c.blue = 0;
	BeginLineArray(fCellsWide + fCellsTall + 2);
	for (i = 0; i <= fCellsWide; i += 1)
		AddLine(BPoint(i * fCellWidth + fHorMargin, fVerMargin),
			    BPoint(i * fCellWidth + fHorMargin, fCellsTall * fCellHeight + fVerMargin),c);
	for (i = 0; i <= fCellsTall; i += 1)
		AddLine(BPoint(fHorMargin, i * fCellHeight + fVerMargin),
			    BPoint(fCellsWide * fCellWidth + fHorMargin, i * fCellHeight + fVerMargin),c);
	EndLineArray();
	
	for (i = 0; i < fCellsTall; i += 1)
		for (j = 0; j < fCellsWide; j += 1) {
			colorRect.Set(fHorMargin + 1 + j * fCellWidth,
						  fVerMargin + 1 + i * fCellHeight,
						  fHorMargin - 1 + fCellWidth + j * fCellWidth,
						  fVerMargin - 1 + fCellHeight + i * fCellHeight);
			if (clip->Intersects(colorRect)) {
				SetHighColor(system_colors()->color_list[colorIndex]);
				FillRect(colorRect);
			}
			colorIndex += 1;
		}

	SetHighColor(0, 0, 0);
	MovePenTo(BPoint(fHorMargin, fCellsTall * fCellHeight + fVerMargin + (13 * 1)));
	DrawString("Color:");
	textSize = StringWidth("Front: MMM  ") + fHorMargin;
	MovePenTo(BPoint(textSize, fCellsTall * fCellHeight + fVerMargin + (13 * 1)));
	DrawString("Red:");
	MovePenTo(BPoint(textSize, fCellsTall * fCellHeight + fVerMargin + (13 * 2)));
	DrawString("Green:");
	MovePenTo(BPoint(textSize, fCellsTall * fCellHeight + fVerMargin + (13 * 3)));
	DrawString("Blue:");

	SetColor(fTheColor, FALSE);
	delete clip;
}

//--------------------------------------------------------------------

void TDesktopView::MouseDown(BPoint thePoint)
{
	long		x,y;
	uchar		currentSel, newSel, lastSel;
	ulong		buttons;
	BPoint		where;

	lastSel = currentSel = fTheColor;

	GetMouse(&where, &buttons);

	do {
		GetMouse(&where, &buttons);
		x = where.x;
		y = where.y;
		x = (x - fHorMargin) / fCellWidth;
		y = (y - fVerMargin) / fCellHeight;
		if (((x >= 0) && (x < fCellsWide)) && ((y >= 0) && (y < fCellsTall)) &&
			((where.x > fHorMargin) && (where.y > fVerMargin)))
			newSel = (y * fCellsWide) + x;
		else
			newSel = currentSel;

		if (newSel != lastSel) {
			SetColor(newSel, TRUE);
			lastSel = newSel;
		}
		snooze(50000);
	} while(buttons);
}

//--------------------------------------------------------------------

void TDesktopView::FrameResized(float width, float height)
{
	short	x;
	short	y;

	x = width;
	y = height - (1 + (3 * 13));

	if (x / y >= 2) {
		fCellsWide = 32;
		fCellsTall = 8;
	}
	else if (y / x >= 2) {
		fCellsWide = 8;
		fCellsTall = 32;
	}
	else {
		fCellsWide = 16;
		fCellsTall = 16;
	}
	x -= 1 + 2 * WIND_BORDER;
	y -= 1 + 2 * WIND_BORDER;
	fCellWidth = x / fCellsWide;
	fHorMargin = WIND_BORDER + (x % fCellsWide) / 2;
	fCellHeight = y / fCellsTall;
	fVerMargin = WIND_BORDER + (y % fCellsTall) / 2;
}

//--------------------------------------------------------------------

void TDesktopView::SetColor(uchar colorID, bool saveIt)
{
	char		colorValue[256];
	int			ref;
	long		x,y;
	long		offsetLen;
	long		colorWidth;
	BRect		colorRect;
	rgb_color	theColor;

	x = fTheColor % fCellsWide;
	y = fTheColor / fCellsWide;
	colorRect.Set(x * fCellWidth + fHorMargin,
				  y * fCellHeight + fVerMargin,
				  x * fCellWidth + fHorMargin + fCellWidth,
				  y * fCellHeight + fVerMargin + fCellHeight);
	SetHighColor(0, 0, 0);
	StrokeRect(colorRect);

	x = colorID % fCellsWide;
	y = colorID / fCellsWide;
	colorRect.Set(x * fCellWidth + fHorMargin,
				  y * fCellHeight + fVerMargin,
				  x * fCellWidth + fHorMargin + fCellWidth,
				  y * fCellHeight + fVerMargin + fCellHeight);
	SetHighColor(255, 255, 255);
	StrokeRect(colorRect);
	SetHighColor(0, 0, 0);

	if (colorID != fTheColor) {
		fTheColor = colorID;
		if (saveIt) {
			mkdir("/boot/system/settings", 0777);
			if ((ref = open("/boot/system/settings/Desktop_settings", 0)) < 0)
				ref = creat("/boot/system/settings/Desktop_settings", 0777);
			if (ref >= 0)
				close(ref);
		}
		set_desktop_color(system_colors()->color_list[fTheColor], saveIt);
	}

	theColor = system_colors()->color_list[colorID];
	offsetLen = StringWidth("Color: ") + fHorMargin;
	colorWidth = StringWidth("MMM ");
	colorRect.Set(offsetLen,
				  fCellsTall * fCellHeight + fVerMargin + 1,
				  offsetLen + colorWidth,
				  fCellsTall * fCellHeight + fVerMargin + (13 * 3));
	FillRect(colorRect, B_SOLID_LOW);

	MovePenTo(BPoint(offsetLen, fCellsTall * fCellHeight + fVerMargin + (13 * 1)));
	sprintf(colorValue, "%d", colorID);
	DrawString(colorValue);
	offsetLen += colorWidth + StringWidth("  Green: ");
	colorRect.left = offsetLen;
	colorRect.right = offsetLen + colorWidth;
	FillRect(colorRect, B_SOLID_LOW);
	MovePenTo(BPoint(offsetLen, fCellsTall * fCellHeight + fVerMargin + (13 * 1)));
	sprintf(colorValue, "%d", theColor.red);
	DrawString(colorValue);
	MovePenTo(BPoint(offsetLen, fCellsTall * fCellHeight + fVerMargin + (13 * 2)));
	sprintf(colorValue, "%d", theColor.green);
	DrawString(colorValue);
	MovePenTo(BPoint(offsetLen, fCellsTall * fCellHeight + fVerMargin + (13 * 3)));
	sprintf(colorValue, "%d", theColor.blue);
	DrawString(colorValue);
}

//--------------------------------------------------------------------

long TDesktopView::LavaThread(void *arg)
{
	TDesktopView	*myView = (TDesktopView *)arg;
	bigtime_t		c_time;

	c_time = system_time() + 50000;
	while (!myView->fDie) {
		if ((lava_desk) && (c_time < system_time())) {
			c_time = system_time() + 50000;
			myView->Window()->Lock();
			myView->SetColor(desktop_view->fTheColor + 1, FALSE);
			myView->Window()->Unlock();
		}
		snooze(50000);
	}
	return B_NO_ERROR;
}
