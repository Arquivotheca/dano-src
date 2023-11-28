/*-----------------------------------------------------------------*/
//
//	File:		ImageTextFilesView.cpp
//
//	Written by:	Robert Polic
//
//	Copyright 2000, Be Incorporated
//
/*-----------------------------------------------------------------*/

#include "ImageTextFilesView.h"

#include <ScrollBar.h>
#include <Window.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const int64 	kKB_SIZE				= 1024;
const int64 	kMB_SIZE				= 1048576;
const int64 	kGB_SIZE				= 1073741824;
const int64 	kTB_SIZE				= kGB_SIZE * kKB_SIZE;

#define kEDIT_ICON_X	2
#define kEDIT_ICON_Y	1


//========================================================================

/* global */
extern int32			cistrncmp				(const char*, const char*, int32);


/*=================================================================*/

ImageTextFilesView::ImageTextFilesView(BRect rect, drawing_parameters* parameters, VIEW_TYPE type)
 : FilesView		(rect, "ImageTextFilesView", parameters),
   fResizing		(eFileNone),
   fSort			(eFileNameAscending),
   fViewType		(type)
{
	float	width = rect.Width() - (20 + B_V_SCROLL_BAR_WIDTH);

	fOffsets.ornament = 0;
	fOffsets.name = 20;
	fOffsets.modified = 20 + (int32)(width * .4000);
	fOffsets.size = 20 + (int32)(width * .8000);
	fOffsets.controls = 20 + (int32)(width * 1.0000);
}


/*-----------------------------------------------------------------*/

void ImageTextFilesView::AddFile(BString* path, BString* name, const char* type,
								 time_t modified, off_t size, PROTOCOL p)
{
	InsertItem(List(), new TFileListItem(List(), path, name, type, p, modified, size, &fOffsets,
										 fParameters, List()->fOffscreen, List()->fOffscreenView,
										 this, fViewType));
}


/*-----------------------------------------------------------------*/

void ImageTextFilesView::Draw(BRect where)
{
	FilesView::Draw(where);
	DrawTabs();
}


/*-----------------------------------------------------------------*/

void ImageTextFilesView::ItemChanged(FileListItem* item)
{
	List()->RemoveItem(item);
	List()->Select(InsertItem(List(), dynamic_cast<TFileListItem*>(item)));
	List()->ScrollToSelection();
}


/*-----------------------------------------------------------------*/

void ImageTextFilesView::MouseDown(BPoint where)
{
	BRect	r = Bounds();

	r.bottom = fParameters->offsets[eTabHeight];
	r.left = fOffsets.name;
	r.right = fOffsets.modified - 1;
	if (r.Contains(where))
	{
		if (where.x > r.right - 10)
			Resize(eFileName, where, r.right);
		else if (fSort == eFileNameAscending)
			Sort(eFileNameDescending);
		else
			Sort(eFileNameAscending);
	}
	else
	{
		r.left = fOffsets.modified;
		r.right = fOffsets.size - 1;
		if (r.Contains(where))
		{
			if (where.x < r.left + 2)
				Resize(eFileName, where, r.left - 1);
			else if (where.x > r.right - 10)
				Resize(eFileModification, where, r.right);
			else if (fSort == eFileModificationAscending)
				Sort(eFileModificationDescending);
			else
				Sort(eFileModificationAscending);
		}
		else
		{
			r.left = fOffsets.size;
			r.right = fOffsets.controls - 1;
			if (r.Contains(where))
			{
				if (where.x < r.left + 2)
					Resize(eFileModification, where, r.left - 1);
				else if (fSort == eFileSizeAscending)
					Sort(eFileSizeDescending);
				else
					Sort(eFileSizeAscending);
			}
		}
	}
}


/*-----------------------------------------------------------------*/

void ImageTextFilesView::MouseMoved(BPoint /* where */, uint32 transit, const BMessage*)
{
	if (fResizing)
	{
		if ((transit == B_EXITED_VIEW) || (transit == B_OUTSIDE_VIEW))
			SetPosition(fOriginalPosition);
		else
		{
			float	min_width = fParameters->tab_font.StringWidth(B_UTF8_ELLIPSIS) + 22;
			uint32	buttons;
			BPoint	mouse;

			GetMouse(&mouse, &buttons);
			if (buttons)
			{
				float	x = fOriginalPosition + mouse.x - fStart.x;

				switch (fResizing)
				{
					case eFileName:
						if (x < fOffsets.name + min_width)
							x = fOffsets.name + min_width;
						if (x > fOffsets.size - min_width)
							x = fOffsets.size - min_width;
						if (x != fOffsets.modified)
							SetPosition(x);
						break;

					case eFileModification:
						if (x < fOffsets.modified + min_width)
							x = fOffsets.modified + min_width;
						if (x > fOffsets.controls - min_width)
							x = fOffsets.controls - min_width;
						if (x != fOffsets.size)
							SetPosition(x);
						break;

					default:
						break;
				}
			}
			else
				fResizing = eFileNone;
		}
	}
}


/*-----------------------------------------------------------------*/

void ImageTextFilesView::MouseUp(BPoint /* where */)
{
	fResizing = eFileNone;
}


/*-----------------------------------------------------------------*/

void ImageTextFilesView::DrawTabs()
{
	int32	sort;

	if (fSort == eFileOrnamentAscending)
		sort = 1;
	else if (fSort == eFileOrnamentDescending)
		sort = -1;
	else
		sort = 0;
	DrawTab(0, fOffsets.name - 1, fParameters->labels[eFileTabOrnament], sort, false);

	if (fSort == eFileNameAscending)
		sort = 1;
	else if (fSort == eFileNameDescending)
		sort = -1;
	else
		sort = 0;
	DrawTab(fOffsets.name, fOffsets.modified - 1, fParameters->labels[eFileTabName], sort);

	if (fSort == eFileModificationAscending)
		sort = 1;
	else if (fSort == eFileModificationDescending)
		sort = -1;
	else
		sort = 0;
	DrawTab(fOffsets.modified, fOffsets.size - 1, fParameters->labels[eFileTabModified], sort);

	if (fSort == eFileSizeAscending)
		sort = 1;
	else if (fSort == eFileSizeDescending)
		sort = -1;
	else
		sort = 0;
	DrawTab(fOffsets.size, fOffsets.controls - 1, fParameters->labels[eFileTabSize], sort);

	DrawTab(fOffsets.controls, Bounds().right, fParameters->labels[eFileTabControls], 0, false);

	DrawBitmap(fOffscreen, BPoint(0, 0));
}


/*-----------------------------------------------------------------*/

int32 ImageTextFilesView::InsertItem(FileListView* list, TFileListItem* new_item)
{
	int32				index = 0;
	int32				insert = -1;
	TFileListItem*		item;

	while ((insert == -1) && ((item = dynamic_cast<TFileListItem*>(list->ItemAt(index))) != NULL))
	{
		switch (fSort)
		{
			case eFileNameAscending:
				if (ICompare(*(new_item->Name()), *(item->Name())) <= 0)
					insert = index;
				break;

			case eFileNameDescending:
				if (ICompare(*(new_item->Name()), *(item->Name())) >= 0)
					insert = index;
				break;

			case eFileModificationAscending:
				if (new_item->Modified() < item->Modified())
					insert = index;
				break;

			case eFileModificationDescending:
				if (new_item->Modified() > item->Modified())
					insert = index;
				break;

			case eFileSizeAscending:
				if (new_item->Size() < item->Size())
					insert = index;
				break;

			case eFileSizeDescending:
				if (new_item->Size() > item->Size())
					insert = index;
				break;

			default:
				break;
		}
		index++;
	}

	if (insert == -1)
		insert = index;
	list->AddItem(new_item, insert);
	return insert;
}


/*-----------------------------------------------------------------*/

void ImageTextFilesView::Resize(eFileFields field, BPoint start, float position)
{
	fResizing = field;
	fStart = start;
	fOriginalPosition = position;
}


/*-----------------------------------------------------------------*/

void ImageTextFilesView::SetPosition(float x)
{
	BRect	r;

	r.top = 0;
	r.bottom = fParameters->offsets[eTabHeight];

	switch (fResizing)
	{
		case eFileName:
			r.left = fOffsets.name;
			r.right = fOffsets.size - 1;
			fOffsets.modified = x;
			Draw(r);
			r.top = List()->Bounds().top;
			r.bottom = List()->Bounds().bottom;
			List()->Draw(r);
			break;

		case eFileModification:
			r.left = fOffsets.modified;
			r.right = fOffsets.controls - 1;
			fOffsets.size = x;
			Draw(r);
			r.top = List()->Bounds().top;
			r.bottom = List()->Bounds().bottom;
			List()->Draw(r);
			break;

		default:
			break;
	}
}


/*-----------------------------------------------------------------*/

void ImageTextFilesView::Sort(eFileSortFields sort)
{
	int32			index = 0;
	TFileListItem*	item;
	FileListView*	list = new FileListView(BRect(0, 0, 0, 0), fParameters);

	fSort = sort;
	DrawTabs();
	while (((item = dynamic_cast<TFileListItem*>(List()->ItemAt(index))) != NULL))
	{
		InsertItem(list, item);
		index++;
	}

	index = 0;
	List()->MakeEmpty();
	while (((item = dynamic_cast<TFileListItem*>(list->ItemAt(index))) != NULL))
	{
		List()->AddItem(item);
		index ++;
	}
	list->MakeEmpty();
	delete list;	
}


//====================================================================

TFileListItem::TFileListItem(BView* view, BString* path, BString* name, const char* type,
							 PROTOCOL p, const time_t modified, const off_t size,
							 file_field_offsets* offsets, drawing_parameters* parameters,
							 BBitmap* offscreen, BView* offview, FilesView* files_view,
							 VIEW_TYPE view_type)
 : FileListItem		(path, name, type, p, parameters, files_view),
   fOffscreen		(offscreen),
   fEditControls	(NULL),
   fOffscreenView	(offview),
   fView			(view),
   fEditor			(NULL),
   fViewType		(view_type)
{
	fName = Leaf();
	fModified = modified;
	fSize = size;
	fOffsets = offsets;
}


//--------------------------------------------------------------------

TFileListItem::~TFileListItem()
{
	RemoveEditor();
}


//--------------------------------------------------------------------

const char *kTIME_FORMATS[] = {
	"%A, %B %d %Y, %I:%M:%S %p",	// Monday, July 09 1997, 05:08:15 PM
	"%a, %b %d %Y, %I:%M:%S %p",	// Mon, Jul 09 1997, 05:08:15 PM
	"%a, %b %d %Y, %I:%M %p",		// Mon, Jul 09 1997, 05:08 PM
	"%b %d %Y, %I:%M %p",			// Jul 09 1997, 05:08 PM
	"%m/%d/%y, %I:%M %p",			// 07/09/97, 05:08 PM
	"%m/%d/%y",						// 07/09/97
	NULL
};

const char *kSIZE_FORMATS[] = {
	"%.2f %s",
	"%.1f %s",
	"%.f %s",
	"%.f%s",
	0
};

void TFileListItem::DrawItem(BView *view, BRect rect, bool /* complete */)
{
	char		str[256];
	const char*	src[1];
	char*		result[1];
	int32		loop;
	float		width = 0;
	float		y;
	int32		index;
	BRect		r = rect;
	font_height	finfo;

	r.OffsetTo(0, 0);
	fOffscreen->Lock();

	if (IsSelected())
	{
		if (fEditor)
		{
			fOffscreenView->SetHighColor(fParameters->colors[eListEditColor]);
			fOffscreenView->SetLowColor(fParameters->colors[eListEditColor]);
		}
		else
		{
			fOffscreenView->SetHighColor(fParameters->colors[eListSelectedColor]);
			fOffscreenView->SetLowColor(fParameters->colors[eListSelectedColor]);
		}
	}
	else
	{
		// seems like a good place to remove the editor
		RemoveEditor();
		if (fRollOver)
		{
			fOffscreenView->SetHighColor(fParameters->colors[eListRolloverColor]);
			fOffscreenView->SetLowColor(fParameters->colors[eListRolloverColor]);
		}
		else
		{
			fOffscreenView->SetHighColor(fParameters->colors[eListColor]);
			fOffscreenView->SetLowColor(fParameters->colors[eListColor]);
		}
	}
	fOffscreenView->FillRect(r);

	fOffscreenView->SetHighColor(fParameters->colors[eListSeperatorColor]);
	for (loop = 0; loop < fParameters->offsets[eListSpacerHeight]; loop++)
		fOffscreenView->StrokeLine(BPoint(r.right, r.bottom - loop), BPoint(r.left, r.bottom - loop));

	if ((IsSelected()) && (!fEditor))
	{
		BRect	tmp = r;

		fOffscreenView->SetHighColor(fParameters->colors[eListSelectedFrameColor]);
		fOffscreenView->SetPenSize(fParameters->offsets[eListSelectedFrameWidth]);
		tmp.bottom -= fParameters->offsets[eListSelectedFrameWidth];
		fOffscreenView->StrokeRect(tmp);
		fOffscreenView->SetPenSize(1);
	}

	if (IsSelected())
		fOffscreenView->SetHighColor(fParameters->colors[eListSelectedTextColor]);
	else
		fOffscreenView->SetHighColor(fParameters->colors[eListTextColor]);

	if (!fEditor)
	{
		fParameters->list_font.GetHeight(&finfo);
		fOffscreenView->SetFont(&fParameters->list_font);
	
		y = r.top + ((r.Height() - (finfo.ascent + finfo.descent + finfo.leading)) / 2) +
						(finfo.ascent + finfo.descent) - 2;
		fOffscreenView->MovePenTo(fOffsets->name + 10, y);
		src[0] = fName.String();
		result[0] = (char*)malloc(strlen(src[0]) + 16);
		fParameters->list_font.GetTruncatedStrings(src, 1, B_TRUNCATE_MIDDLE, fOffsets->modified - fOffsets->name - 20, result);
		fOffscreenView->DrawString(result[0]);
		free(result[0]);
	
		time_t	time = fModified;
		tm		time_data;
	
		fOffscreenView->MovePenTo(fOffsets->modified + 10, y);
		localtime_r(&time, &time_data);
		for (index = 0; ; index++)
		{
			if (!kTIME_FORMATS[index])
				break;
			strftime(str, 256, kTIME_FORMATS[index], &time_data);
			width = fParameters->list_font.StringWidth(str);
			if (width <= fOffsets->size - fOffsets->modified - 20)
				break;
		}
		if (width > fOffsets->size - fOffsets->modified - 20)
		{
			src[0] = str;
			result[0] = (char*)malloc(strlen(src[0]) + 16);
			fParameters->list_font.GetTruncatedStrings(src, 1, B_TRUNCATE_MIDDLE, fOffsets->size - fOffsets->modified - 20, result);
			fOffscreenView->DrawString(result[0]);
			free(result[0]);
		}
		else
			fOffscreenView->DrawString(str);
	
		width = fOffsets->controls - fOffsets->size - 20;
		if (fSize < kKB_SIZE)
		{
			sprintf(str, "%Ld bytes", fSize);
			if (fParameters->list_font.StringWidth(str) > width)
				sprintf(str, "%Ld B", fSize);
		}
		else
		{
			const char*	suffix;
			float 		float_value;
			if (fSize >= kTB_SIZE)
			{
				suffix = "TB";
				float_value = (float)fSize / kTB_SIZE;
			}
			else if (fSize >= kGB_SIZE)
			{
				suffix = "GB";
				float_value = (float)fSize / kGB_SIZE;
			}
			else if (fSize >= kMB_SIZE)
			{
				suffix = "MB";
				float_value = (float)fSize / kMB_SIZE;
			}
			else
			{
				suffix = "KB";
				float_value = (float)fSize / kKB_SIZE;
			}
	
			for (index = 0; ; index++) {
				if (!kSIZE_FORMATS[index])
					break;
	
				sprintf(str, kSIZE_FORMATS[index], float_value, suffix);
	
				// strip off an insignificant zero so we don't get readings
				// such as 1.00
				char *period = 0;
				for (char *tmp = str; *tmp; tmp++)
				{
					if (*tmp == '.')
						period = tmp;
				}
				if (period && period[1] && period[2] == '0')
					// move the rest of the string over the insignificant zero
					for (char *tmp = &period[2]; *tmp; tmp++)
						*tmp = tmp[1];
				if (fParameters->list_font.StringWidth(str) <= width)
					break;
			}
		}
		src[0] = str;
		result[0] = (char*)malloc(strlen(src[0]) + 16);
		fParameters->list_font.GetTruncatedStrings(src, 1, B_TRUNCATE_MIDDLE, width, result);
		fOffscreenView->MovePenTo(fOffsets->controls - 10 - fParameters->list_font.StringWidth(result[0]), y);
		fOffscreenView->DrawString(result[0]);
		free(result[0]);
	}

	fOffscreenView->Sync();
	fOffscreen->Unlock();
	view->DrawBitmap(fOffscreen, r, rect);

	int32	icon = eImageIcon;

	if (ViewType() == eOtherView)
		icon = eOtherIcon;
	else if (ViewType() == eTextView)
	{
		icon = eTextIcon;

		if (cistrncmp(Type(), "text/html", 9) == 0)
			icon = eHTMLIcon;
		else if (cistrncmp(Type(), "message/rfc822", 14) == 0)
			icon = eEmailIcon;
	}

	if (fParameters->icons[icon])
	{
		int32	height = 0;
		int32	width;
		uint32	flags;
		BRect	png = rect;

		fParameters->icons[icon]->GetSize(&width, &height, &flags);

		png.left += kEDIT_ICON_X;
		png.right = png.left + width - 1;
		png.top += (int)((r.Height() - height) / 2);
		png.bottom = png.top + height - 1;

		fParameters->icons[icon]->FrameChanged(png, width, height);
		fParameters->icons[icon]->Draw(view, png);
	}
}


//--------------------------------------------------------------------

void TFileListItem::EditorCallBack(BMessage* msg)
{
	switch (msg->what)
	{
		case eDetachedFromWindow:
			RemoveEditor();
			break;

		case eEditAccepted:
			Edit(NULL, BRect(0, 0, 0, 0));
			RemoveEditor();
			break;

		case eEditCanceled:
			RemoveEditor();
			break;
	}
}


//--------------------------------------------------------------------

void TFileListItem::Edit(BView* /* view */, BRect rect)
{
	if (!fEditor)
	{
		int32	cancel_width = 0;
		int32	ok_width = 0;
		int32	tmp;
		uint32	flags;
		BRect	r = rect;

		if (fParameters->icons[eEditAcceptUpIcon])
			fParameters->icons[eEditAcceptUpIcon]->GetSize(&ok_width, &tmp, &flags);
		if (fParameters->icons[eEditRejectUpIcon])
			fParameters->icons[eEditRejectUpIcon]->GetSize(&cancel_width, &tmp, &flags);
		tmp = 9 + ok_width + 8 + cancel_width + 8;

		r.left = fOffsets->name + 3;
		r.right = fOffsets->controls - tmp;

		fView->AddChild(fEditor = new Editor(this, r, rect, &fName, fParameters, true));

		fEditor->MakeFocus(true);

		r.right = fOffsets->controls;
		r.left = r.right - tmp;
		fView->AddChild(fEditControls = new EditControls(this, r, rect, fParameters));
	}
	else
	{
		if (Rename(fEditor->Text()) == B_NO_ERROR)
		{
			fName = Leaf();
			fFilesView->ItemChanged(this);
		}
		fView->Window()->PostMessage(eEditAccepted, fEditor);
	}
}


//--------------------------------------------------------------------

void TFileListItem::RemoveEditor()
{
	if (fEditor)
	{
		fView->RemoveChild(fEditor);
		delete fEditor;
		fEditor = NULL;
	}
	if (fEditControls)
	{
		fView->RemoveChild(fEditControls);
		delete fEditControls;
		fEditControls = NULL;
	}
}
