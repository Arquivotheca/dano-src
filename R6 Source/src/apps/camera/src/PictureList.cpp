/*
	PictureList.cpp
	Implementation.
*/

#include <stdio.h>
#include <string.h>
#include <Bitmap.h>
#include <ScrollBar.h>
#include <Window.h>
#include "appconfig.h"
#include "PictureList.h"

#include "thumb.h" // bitmap for un-initialized thumbnail images

#ifndef MIN
#define MIN(a,b)				(((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b)				(((a) > (b)) ? (a) : (b))
#endif

PictureList::PictureList(BRect r, const char *name) :
	BView(r, name, B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS)
{
	SetViewUIColor(B_UI_DOCUMENT_BACKGROUND_COLOR);
	fSelected = fHasThumbnail = NULL;
	fDragging = false;
	fExistingFilenames = false;
}

PictureList::~PictureList()
{
	Empty();
}

void PictureList::Empty()
{
	int32	i;
	char	*str;
	BBitmap	*bmp;

	for (i = 0; i < fNameList.CountItems(); i++)
	{
		str = (char *)(fNameList.RemoveItem(i));
		delete [] str;
		i--;
	}
	fExistingFilenames = false;
	for (i = 0; i < fImgList.CountItems(); i++)
	{
		bmp = (BBitmap *)(fImgList.RemoveItem(i));
		delete bmp;
		i--;
	}
	if (fSelected)
		delete [] fSelected;
	fSelected = NULL;
	if (fHasThumbnail)
		delete [] fHasThumbnail;
	fHasThumbnail = NULL;

	BScrollBar	*sb = ScrollBar(B_VERTICAL);
	if (sb != NULL)
	{
		sb->SetSteps(8, 64);
		sb->SetRange(0, 0);
		sb->SetValue(0);
		sb->SetProportion(1.0f);
	}
	Invalidate();
}

void PictureList::CreateNewList(int32 length)
{
	int32	i;
	char	*str;
	BBitmap	*img;

	fSelected = new bool[length];
	fHasThumbnail = new bool[length];
	for (i = 0; i < length; i++)
	{
		if(fExistingFilenames == false) {
			str = new char[32];
			sprintf(str, STR_PICTURENAME, i + 1);
			fNameList.AddItem(str);
		}
		img = new BBitmap(BRect(0, 0, TWIDTH - 1, THEIGHT - 1), B_RGB32, true);
		img->AddChild(new BView(img->Bounds(), "thumbView", B_FOLLOW_ALL, 0));
		img->SetBits(ThumbnailBitmap, TWIDTH * THEIGHT, 0, B_CMAP8);
		fImgList.AddItem(img);
		fSelected[i] = fHasThumbnail[i] = false;
	}

	FixupScrollbar();
	Invalidate();
}

void PictureList::AddFileName(char *name)
{
	if(name == NULL) return;	

	fNameList.AddItem(name);

	fExistingFilenames = true;
}

int32 PictureList::CountPictures()
{
	return fNameList.CountItems();
}

int32 PictureList::CountSelection()
{
	int32	i, n = fNameList.CountItems(), tot = 0;

	for (i = 0; i < n; i++)
	{
		if (fSelected[i])
			tot++;
	}
	return tot;
}

bool PictureList::IsSelected(int32 item)
{
	if (item < fNameList.CountItems())
		return fSelected[item];
	else
		return false;
}

void PictureList::Select(int32 item)
{
	if (item < fNameList.CountItems())
		fSelected[item] = true;
}

bool PictureList::HasThumbnail(int32 item)
{
	if (item < fNameList.CountItems())
		return fHasThumbnail[item];
	else
		return false;
}

char *PictureList::PictureName(int32 item)
{
	if (item < fNameList.CountItems())
		return (char *)(fNameList.ItemAt(item));
	else
		return NULL;
}

void PictureList::DeletePicture(int32 item)
{
	if (item >= fNameList.CountItems())
		return; // not a valid index number

	int32	i;
	char	*str;
	BBitmap	*bmp;

	str = (char *)(fNameList.RemoveItem(item));
	delete [] str;
	bmp = (BBitmap *)(fImgList.RemoveItem(item));
	delete bmp;
	for (i = item; i < fNameList.CountItems(); i++)
	{
		fSelected[i] = fSelected[i + 1];
		fHasThumbnail[i] = fHasThumbnail[i + 1];
	}
	FixupScrollbar();
	Invalidate();
}

void PictureList::SetThumbnail(int32 item, BBitmap *thumbnail)
{
	if (item >= fNameList.CountItems())
		return; // not a valid index number

	BBitmap		*img = (BBitmap *)(fImgList.ItemAt(item));
	BView		*view = img->FindView("thumbView");
	BRect		r = img->Bounds();
	float		aspect;
	rgb_color	black = ui_color(B_SHADOW_COLOR);
	rgb_color	white = ui_color(B_SHINE_COLOR);
	rgb_color	dark = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_2_TINT);

	img->Lock();
	view->SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	view->FillRect(r);
	r = thumbnail->Bounds();
	aspect = r.Width() / r.Height();
	if (aspect > 1.0f)
	{
		r.left = 0; r.right = TWIDTH - 1;
		r.top = THEIGHT / 2.0f - ((TWIDTH - 1) / aspect) / 2.0f;
		r.bottom = THEIGHT / 2.0f + ((TWIDTH - 1) / aspect) / 2.0f;
	}
	else
	{
		r.top = 0; r.bottom = THEIGHT - 1;
		r.left = TWIDTH / 2.0f - ((THEIGHT - 1) * aspect) / 2.0f;
		r.right = TWIDTH / 2.0f + ((THEIGHT - 1) * aspect) / 2.0f;
	}
	view->DrawBitmap(thumbnail, r);
	r = img->Bounds();
	view->SetHighColor(black);
	view->StrokeRect(r);
	r.InsetBy(1, 1);
	view->BeginLineArray(4);
	view->AddLine(r.LeftTop(), r.RightTop(), white);
	view->AddLine(r.LeftTop(), r.LeftBottom(), white);
	view->AddLine(r.RightTop(), r.RightBottom(), dark);
	view->AddLine(r.LeftBottom(), r.RightBottom(), dark);
	view->EndLineArray();
	view->Sync();
	img->Unlock();

	fHasThumbnail[item] = true;
	Invalidate();
}

void PictureList::Draw(BRect updateRect)
{
	int32		i, n, numCols;
	BRect		baseR, r;
	BPoint		pt;
	char		*str;
	rgb_color	black = { 0, 0, 0, 255 };

	r = Bounds();
	numCols = (int32)(r.Width() + 4.0f) / ITEM_WIDTH;
	baseR.Set(0, 0, TWIDTH, THEIGHT);
	baseR.OffsetBy(BUFFER_WIDTH, BUFFER_WIDTH);
	n = CountPictures();
	for (i = 0; i < n; i++)
	{
		if (i > 0 && (i % numCols) == 0)
			baseR.OffsetBy(0, ITEM_HEIGHT);
		r = baseR;
		r.OffsetBy(ITEM_WIDTH * (float)(i % numCols), 0);
		DrawBitmap((BBitmap *)(fImgList.ItemAt(i)), r.LeftTop());
		pt.Set(r.left + (r.right - r.left) / 2.0f, r.bottom + BUFFER_WIDTH * 2);
		str = (char *)(fNameList.ItemAt(i));
		pt.x -= be_plain_font->StringWidth(str) / 2.0f;
		DrawString(str, pt);
		if (fSelected[i])
		{
			r.bottom += BUFFER_WIDTH * 2.5;
			r.right += (r.right - r.left) * 0.05;
			r.left -= (r.right - r.left) * 0.05;
			SetHighColor(black);
			SetDrawingMode(B_OP_BLEND);
			FillRect(r);
			SetDrawingMode(B_OP_COPY);
		}
	}
}

void PictureList::FrameResized(float width, float height)
{
	Invalidate(Bounds());
	FixupScrollbar();
}

void PictureList::MouseDown(BPoint point)
{
	int32		i;
	int32		id = Pick(point);
	int32		n = CountPictures();
	BMessage	*msg = Window()->CurrentMessage();

	if (n < 1)
		return;

	fDragSel = false;
	if (id >= 0)
	{ // clicked on a picture
		if (modifiers() & B_SHIFT_KEY)
			fSelected[id] = !fSelected[id];
		else
		{
			if (!fSelected[id])
			{
				for (i = 0; i < n; i++)
					fSelected[i] = false;
				fSelected[id] = true;
			}
			else if (msg->FindInt32("clicks", &i) == B_NO_ERROR)
			{
				if (i > 1)
					Window()->PostMessage(MSG_SAVE);
			}
		}
	}
	else
	{ // clicked on background
		if (!(modifiers() & B_SHIFT_KEY))
		{
			for (i = 0; i < n; i++)
				fSelected[i] = false;
		}
		fDragSel = true;
		fPreDragSel = new bool[n];
		for (i = 0; i < n; i++)
			fPreDragSel[i] = fSelected[i];
		BeginRectTracking(BRect(point, point), B_TRACK_RECT_CORNER);
	}
	fDragging = true;
	fDragStartPt = point;
	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS | B_NO_POINTER_HISTORY);
	Window()->PostMessage(MSG_UPDATESEL);
	Invalidate();
}

void PictureList::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	if (!fDragging)
		return;

	int32	i, n = CountPictures(), numCols;

	if (fDragSel)
	{ // drag selection rect
		bool	selChange = false;
		BRect	baseR, r;
		BRect	selectR;
	
		selectR.Set(MIN(point.x, fDragStartPt.x), MIN(point.y, fDragStartPt.y),
					MAX(point.x, fDragStartPt.x), MAX(point.y, fDragStartPt.y));
		r = Bounds();
		numCols = (int32)(r.Width() + 4.0f) / ITEM_WIDTH;
		baseR.Set(0, 0, TWIDTH, THEIGHT);
		baseR.OffsetBy(BUFFER_WIDTH, BUFFER_WIDTH);
		for (i = 0; i < n; i++)
		{
			if (i > 0 && (i % numCols) == 0)
				baseR.OffsetBy(0, ITEM_HEIGHT);
			r = baseR;
			r.OffsetBy(ITEM_WIDTH * (float)(i % numCols), 0);
			if (r.Intersects(selectR))
			{
				if (fPreDragSel[i] == fSelected[i])
				{
					fSelected[i] = !fPreDragSel[i];
					selChange = true;
				}
			}
			else
			{
				if (fPreDragSel[i] != fSelected[i])
				{
					fSelected[i] = fPreDragSel[i];
					selChange = true;
				}
			}
		}
		if (selChange)
			Invalidate();
	}
	else
	{ // drag an image
		float	dist = (point.x - fDragStartPt.x) * (point.x - fDragStartPt.x) +
						(point.y - fDragStartPt.y) * (point.y - fDragStartPt.y);

		if (dist > 8.0f)
		{
			int32	id = 0, numSel = 0;

			for (i = 0; i < n; i++)
			{
				if (fSelected[i])
				{
					id = i;
					numSel++;
				}
			}

			BMessage	msg(B_SIMPLE_DATA);
			// types
			msg.AddString("be:types", B_FILE_MIME_TYPE);
			msg.AddString("be:filetypes", "image/jpeg");
			// actions
			msg.AddInt32("be:actions", B_COPY_TARGET);
			msg.AddInt32("be:actions", B_TRASH_TARGET);
			msg.AddString("be:clip_name", "Camera Image");

			if (numSel == 1)
			{
				BBitmap		*sourceBmp = (BBitmap *)(fImgList.ItemAt(id));
				BBitmap		*dragBmp = new BBitmap(sourceBmp->Bounds(), sourceBmp->ColorSpace());
				BRect		dragRect = PictureBounds(id);

				memcpy(dragBmp->Bits(), sourceBmp->Bits(), sourceBmp->BitsLength());
				DragMessage(&msg, dragBmp, B_OP_BLEND,
							BPoint(fDragStartPt.x - dragRect.left, fDragStartPt.y - dragRect.top),
							Window());
			}
			else if (numSel > 1)
			{
				DragMessage(&msg, Bounds(), Window());
			}
		}
	}
}

void PictureList::MouseUp(BPoint point)
{
	if (fDragging)
	{
		fDragging = false;
		if (fDragSel)
		{
			delete [] fPreDragSel;
			EndRectTracking();
		}
		Window()->PostMessage(MSG_UPDATESEL);
	}
}

void PictureList::FixupScrollbar()
{
	int32		numCols, numRows, n = CountPictures();
	float		maxScroll, proportion;
	BScrollBar	*sb = ScrollBar(B_VERTICAL);
	BRect		r = Bounds();

	if (n == 0)
	{
		sb->SetSteps(8, 64);
		sb->SetRange(0, 0);
		sb->SetValue(0);
		sb->SetProportion(1.0f);
		return;
	}

	numCols = (int32)(r.Width() + 4.0f) / ITEM_WIDTH;
	numRows = n / numCols;
	if (numRows * numCols < n)
		numRows++;
	if (sb != NULL)
	{
		proportion = (r.Height() / (float)ITEM_HEIGHT) / (float)numRows;
		sb->SetSteps(r.Height() / 8.0f, r.Height() / 2.0f);
		maxScroll = (numRows * ITEM_HEIGHT + BUFFER_WIDTH) - r.Height();
		if (maxScroll < 0.0f)
			maxScroll = 0.0f;
		sb->SetRange(0, maxScroll);
		sb->SetProportion(proportion);
	}
}

int32 PictureList::Pick(BPoint point)
{ // returns the id of the image clicked on, or -1 if none
	int32		i, n, numCols;
	BRect		baseR, r;

	r = Bounds();
	numCols = (int32)(r.Width() + 4.0f) / ITEM_WIDTH;
	baseR.Set(0, 0, TWIDTH, THEIGHT);
	baseR.OffsetBy(BUFFER_WIDTH, BUFFER_WIDTH);
	n = CountPictures();
	for (i = 0; i < n; i++)
	{
		if (i > 0 && (i % numCols) == 0)
			baseR.OffsetBy(0, ITEM_HEIGHT);
		r = baseR;
		r.OffsetBy(ITEM_WIDTH * (float)(i % numCols), 0);
		if (r.Contains(point))
			return i;
	}
	return -1;
}

BRect PictureList::PictureBounds(int32 id)
{ // returns the BRect occupied by a given image
	// it goes through all the images, a bit wasteful, but
	// there can't be too many images anyhow
	int32		i, n, numCols;
	BRect		baseR, r;

	r = Bounds();
	numCols = (int32)(r.Width() + 4.0f) / ITEM_WIDTH;
	baseR.Set(0, 0, TWIDTH, THEIGHT);
	baseR.OffsetBy(BUFFER_WIDTH, BUFFER_WIDTH);
	n = CountPictures();
	for (i = 0; i < n; i++)
	{
		if (i > 0 && (i % numCols) == 0)
			baseR.OffsetBy(0, ITEM_HEIGHT);
		r = baseR;
		r.OffsetBy(ITEM_WIDTH * (float)(i % numCols), 0);
		if (i == id)
			break;
	}
	return r;
}
