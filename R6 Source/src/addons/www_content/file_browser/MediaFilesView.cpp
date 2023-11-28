/*-----------------------------------------------------------------*/
//
//	File:		MediaFilesView.cpp
//
//	Written by:	Robert Polic
//
//	Copyright 2000, Be Incorporated
//
/*-----------------------------------------------------------------*/

#include "MediaFilesView.h"

#include <ScrollBar.h>
#include <Window.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define kEDIT_ICON_X	2
#define kEDIT_ICON_Y	1


//========================================================================

/* global */
extern int32			cistrncmp				(const char*, const char*, int32);


/*=================================================================*/

MediaFilesView::MediaFilesView(BRect rect, drawing_parameters* parameters)
 : FilesView		(rect, "MediaFilesView", parameters),
   fResizing		(eMediaNone),
   fSort			(eMediaTitleAscending)
{
	float	width = rect.Width() - (20 + B_V_SCROLL_BAR_WIDTH);

	fOffsets.ornament = 0;
	fOffsets.title = 20;
	fOffsets.artist = 20 + (int32)(width * .3000);
	fOffsets.album = 20 + (int32)(width * .5400);
	fOffsets.track = 20 + (int32)(width * .7800);
	fOffsets.time = 20 + (int32)(width * .8900);
	fOffsets.controls = 20 + (int32)(width * 1.000);
}


/*-----------------------------------------------------------------*/

void MediaFilesView::AddFile(BString* path, BString* name, const char* type,
							 time_t /* modified */, off_t /* size */, PROTOCOL p)
{
	bool		is_mp3 = false;
	id3_tags*	tags = new id3_tags();

	tags->title = *name;
	if ((p == eLocal) && (cistrncmp(type, "audio/x-mpeg", 12) == 0))
	{
		is_mp3 = true;
		ReadID3Tags(path->String(), tags);
	}

	InsertItem(List(), new TMediaListItem(List(), path, name, type, p, tags, is_mp3,
										  &fOffsets, fParameters, List()->fOffscreen,
										  List()->fOffscreenView, this));
}


/*-----------------------------------------------------------------*/

void MediaFilesView::Draw(BRect where)
{
	FilesView::Draw(where);
	DrawTabs();
}


/*-----------------------------------------------------------------*/

void MediaFilesView::ItemChanged(FileListItem* item)
{
	List()->RemoveItem(item);
	List()->Select(InsertItem(List(), dynamic_cast<TMediaListItem*>(item)));
	List()->ScrollToSelection();
}


/*-----------------------------------------------------------------*/

void MediaFilesView::MouseDown(BPoint where)
{
	BRect	r = Bounds();

	r.bottom = fParameters->offsets[eTabHeight];
	r.left = fOffsets.title;
	r.right = fOffsets.artist - 1;
	if (r.Contains(where))
	{
		if (where.x > r.right - 10)
			Resize(eMediaTitle, where, r.right);
		else if (fSort == eMediaTitleAscending)
			Sort(eMediaTitleDescending);
		else
			Sort(eMediaTitleAscending);
	}
	else
	{
		r.left = fOffsets.artist;
		r.right = fOffsets.album - 1;
		if (r.Contains(where))
		{
			if (where.x < r.left + 2)
				Resize(eMediaTitle, where, r.left - 1);
			else if (where.x > r.right - 10)
				Resize(eMediaArtist, where, r.right);
			else if (fSort == eMediaArtistAscending)
				Sort(eMediaArtistDescending);
			else
				Sort(eMediaArtistAscending);
		}
		else
		{
			r.left = fOffsets.album;
			r.right = fOffsets.track - 1;
			if (r.Contains(where))
			{
				if (where.x < r.left + 2)
					Resize(eMediaArtist, where, r.left - 1);
				else if (where.x > r.right - 10)
					Resize(eMediaAlbum, where, r.right);
				else if (fSort == eMediaAlbumAscending)
					Sort(eMediaAlbumDescending);
				else
					Sort(eMediaAlbumAscending);
			}
			else
			{
				r.left = fOffsets.track;
				r.right = fOffsets.time - 1;
				if (r.Contains(where))
				{
					if (where.x < r.left + 2)
						Resize(eMediaAlbum, where, r.left - 1);
					else if (where.x > r.right - 10)
						Resize(eMediaTrack, where, r.right);
					else if (fSort == eMediaTrackAscending)
						Sort(eMediaTrackDescending);
					else
						Sort(eMediaTrackAscending);
				}
				else
				{
					r.left = fOffsets.time;
					r.right = fOffsets.controls - 1;
					if (r.Contains(where))
					{
						if (where.x < r.left + 2)
							Resize(eMediaTrack, where, r.left - 1);
						else if (fSort == eMediaTimeAscending)
							Sort(eMediaTimeDescending);
						else
							Sort(eMediaTimeAscending);
					}
				}
			}
		}
	}
}


/*-----------------------------------------------------------------*/

void MediaFilesView::MouseMoved(BPoint /* where */, uint32 transit, const BMessage*)
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
					case eMediaTitle:
						if (x < fOffsets.title + min_width)
							x = fOffsets.title + min_width;
						if (x > fOffsets.album - min_width)
							x = fOffsets.album - min_width;
						if (x != fOffsets.artist)
							SetPosition(x);
						break;

					case eMediaArtist:
						if (x < fOffsets.artist + min_width)
							x = fOffsets.artist + min_width;
						if (x > fOffsets.track - min_width)
							x = fOffsets.track - min_width;
						if (x != fOffsets.album)
							SetPosition(x);
						break;

					case eMediaAlbum:
						if (x < fOffsets.album + min_width)
							x = fOffsets.album + min_width;
						if (x > fOffsets.time - min_width)
							x = fOffsets.time - min_width;
						if (x != fOffsets.track)
							SetPosition(x);
						break;

					case eMediaTrack:
						if (x < fOffsets.track + min_width)
							x = fOffsets.track + min_width;
						if (x > fOffsets.controls - min_width)
							x = fOffsets.controls - min_width;
						if (x != fOffsets.time)
							SetPosition(x);
						break;

					default:
						break;
				}
			}
			else
				fResizing = eMediaNone;
		}
	}
}


/*-----------------------------------------------------------------*/

void MediaFilesView::MouseUp(BPoint /* where */)
{
	fResizing = eMediaNone;
}


/*-----------------------------------------------------------------*/

void MediaFilesView::DrawTabs()
{
	int32	sort;

	if (fSort == eMediaOrnamentAscending)
		sort = 1;
	else if (fSort == eMediaOrnamentDescending)
		sort = -1;
	else
		sort = 0;
	DrawTab(0, fOffsets.title - 1, fParameters->labels[eMediaTabOrnament], sort, false);

	if (fSort == eMediaTitleAscending)
		sort = 1;
	else if (fSort == eMediaTitleDescending)
		sort = -1;
	else
		sort = 0;
	DrawTab(fOffsets.title, fOffsets.artist - 1, fParameters->labels[eMediaTabTitle], sort);

	if (fSort == eMediaArtistAscending)
		sort = 1;
	else if (fSort == eMediaArtistDescending)
		sort = -1;
	else
		sort = 0;
	DrawTab(fOffsets.artist, fOffsets.album - 1, fParameters->labels[eMediaTabArtist], sort);

	if (fSort == eMediaAlbumAscending)
		sort = 1;
	else if (fSort == eMediaAlbumDescending)
		sort = -1;
	else
		sort = 0;
	DrawTab(fOffsets.album, fOffsets.track - 1, fParameters->labels[eMediaTabAlbum], sort);

	if (fSort == eMediaTrackAscending)
		sort = 1;
	else if (fSort == eMediaTrackDescending)
		sort = -1;
	else
		sort = 0;
	DrawTab(fOffsets.track, fOffsets.time - 1, fParameters->labels[eMediaTabTrack], sort);

	if (fSort == eMediaTimeAscending)
		sort = 1;
	else if (fSort == eMediaTimeDescending)
		sort = -1;
	else
		sort = 0;
	DrawTab(fOffsets.time, fOffsets.controls - 1, fParameters->labels[eMediaTabTime], sort);

	DrawTab(fOffsets.controls, Bounds().right, fParameters->labels[eMediaTabControls], 0, false);

	DrawBitmap(fOffscreen, BPoint(0, 0));
}


/*-----------------------------------------------------------------*/

int32 MediaFilesView::InsertItem(FileListView* list, TMediaListItem* new_item)
{
	int32				index = 0;
	int32				insert = -1;
	TMediaListItem*		item;

	while ((insert == -1) && ((item = (TMediaListItem*)list->ItemAt(index)) != NULL))
	{
		switch (fSort)
		{
			case eMediaTitleAscending:
				if (ICompare(*(new_item->Title()), *(item->Title())) <= 0)
					insert = index;
				break;

			case eMediaTitleDescending:
				if (ICompare(*(new_item->Title()), *(item->Title())) >=0)
					insert = index;
				break;

			case eMediaArtistAscending:
				if (ICompare(*(new_item->Artist()), *(item->Artist())) <= 0)
					insert = index;
				break;

			case eMediaArtistDescending:
				if (ICompare(*(new_item->Artist()), *(item->Artist())) >= 0)
					insert = index;
				break;

			case eMediaAlbumAscending:
				if (((new_item->Track()) &&
					(ICompare(*(new_item->Album()), *(item->Album())) == 0) &&
					(new_item->Track() < item->Track())) ||
					(ICompare(*(new_item->Album()), *(item->Album())) <= 0))
					insert = index;
				break;

			case eMediaAlbumDescending:
				if (((new_item->Track()) &&
					(ICompare(*(new_item->Album()), *(item->Album())) == 0) &&
					(new_item->Track() > item->Track())) ||
					(ICompare(*(new_item->Album()), *(item->Album())) >= 0))
					insert = index;
				break;

			case eMediaTimeAscending:
				if (new_item->Time() < item->Time())
					insert = index;
				break;

			case eMediaTimeDescending:
				if (new_item->Time() > item->Time())
					insert = index;
				break;

			case eMediaTrackAscending:
				if (new_item->Track() < item->Track())
					insert = index;
				break;

			case eMediaTrackDescending:
				if (new_item->Track() > item->Track())
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

void MediaFilesView::Resize(eMediaFields field, BPoint start, float position)
{
	fResizing = field;
	fStart = start;
	fOriginalPosition = position;
}


/*-----------------------------------------------------------------*/

void MediaFilesView::SetPosition(float x)
{
	BRect	r;

	r.top = 0;
	r.bottom = fParameters->offsets[eTabHeight];

	switch (fResizing)
	{
		case eMediaTitle:
			r.left = fOffsets.title;
			r.right = fOffsets.album - 1;
			fOffsets.artist = x;
			Draw(r);
			r.top = List()->Bounds().top;
			r.bottom = List()->Bounds().bottom;
			List()->Draw(r);
			break;

		case eMediaArtist:
			r.left = fOffsets.artist;
			r.right = fOffsets.track - 1;
			fOffsets.album = x;
			Draw(r);
			r.top = List()->Bounds().top;
			r.bottom = List()->Bounds().bottom;
			List()->Draw(r);
			break;

		case eMediaAlbum:
			r.left = fOffsets.album;
			r.right = fOffsets.time - 1;
			fOffsets.track = x;
			Draw(r);
			r.top = List()->Bounds().top;
			r.bottom = List()->Bounds().bottom;
			List()->Draw(r);
			break;

		case eMediaTrack:
			r.left = fOffsets.track;
			r.right = fOffsets.time - 1;
			fOffsets.time = x;
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

void MediaFilesView::Sort(eMediaSortFields sort)
{
	int32			index = 0;
	TMediaListItem*	item;
	FileListView*	list = new FileListView(BRect(0, 0, 0, 0), fParameters);

	fSort = sort;
	DrawTabs();
	while (((item = dynamic_cast<TMediaListItem*>(List()->ItemAt(index))) != NULL))
	{
		InsertItem(list, item);
		index++;
	}

	index = 0;
	List()->MakeEmpty();
	while (((item = dynamic_cast<TMediaListItem*>(list->ItemAt(index))) != NULL))
	{
		List()->AddItem(item);
		index ++;
	}
	list->MakeEmpty();
	delete list;	
}


//====================================================================

TMediaListItem::TMediaListItem(BView* view, BString* path, BString* name, const char* type,
							   PROTOCOL p, id3_tags* tags, bool is_mp3, media_field_offsets* offsets,
							   drawing_parameters* parameters, BBitmap* offscreen,
							   BView* offview, FilesView* files_view)
 : FileListItem		(path, name, type, p, parameters, files_view),
   fIsMP3			(is_mp3),
   fOffscreen		(offscreen),
   fEditControls	(NULL),
   fOffscreenView	(offview),
   fView			(view),
   fOffsets			(offsets),
   fTags			(tags)
{
	for (int32 loop = 0; loop < eMediaEditorCount; loop++)
		fEditor[loop] = NULL;
}


//--------------------------------------------------------------------

TMediaListItem::~TMediaListItem()
{
	RemoveEditor();
	delete(fTags);
}


//--------------------------------------------------------------------

void TMediaListItem::DrawItem(BView *view, BRect rect, bool /* complete */)
{
	char		str[64];
	const char*	src[1];
	char*		result[1];
	int32		loop;
	float		y;
	BRect		r = rect;
	font_height	finfo;

	r.OffsetTo(0, 0);
	fOffscreen->Lock();

	if (IsSelected())
	{
		if (fEditor[0])
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

	if ((IsSelected()) && (!fEditor[0]))
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

	if (!fEditor[0])
	{
		fParameters->list_font.GetHeight(&finfo);
		fOffscreenView->SetFont(&fParameters->list_font);
	
		y = r.top + ((r.Height() - (finfo.ascent + finfo.descent + finfo.leading)) / 2) +
						(finfo.ascent + finfo.descent) - 2;
	
		fOffscreenView->MovePenTo(fOffsets->title + 10, y);
		src[0] = fTags->title.String();
		result[0] = (char*)malloc(strlen(src[0]) + 16);
		fParameters->list_font.GetTruncatedStrings(src, 1, B_TRUNCATE_MIDDLE, fOffsets->artist - fOffsets->title - 20, result);
		fOffscreenView->DrawString(result[0]);
		free(result[0]);
	
		fOffscreenView->MovePenTo(fOffsets->artist + 10, y);
		src[0] = fTags->artist.String();
		result[0] = (char*)malloc(strlen(src[0]) + 16);
		fParameters->list_font.GetTruncatedStrings(src, 1, B_TRUNCATE_MIDDLE, fOffsets->album - fOffsets->artist - 20, result);
		fOffscreenView->DrawString(result[0]);
		free(result[0]);
	
		fOffscreenView->MovePenTo(fOffsets->album + 10, y);
		src[0] = fTags->album.String();
		result[0] = (char*)malloc(strlen(src[0]) + 16);
		fParameters->list_font.GetTruncatedStrings(src, 1, B_TRUNCATE_MIDDLE, fOffsets->track - fOffsets->album - 20, result);
		fOffscreenView->DrawString(result[0]);
		free(result[0]);
	
		if (fTags->track)
		{
			sprintf(str, "%d", (int)fTags->track);
			src[0] = str;
			result[0] = (char*)malloc(strlen(src[0]) + 16);
			fParameters->list_font.GetTruncatedStrings(src, 1, B_TRUNCATE_MIDDLE, fOffsets->time - fOffsets->track - 20, result);
			fOffscreenView->MovePenTo(fOffsets->time - 10 - fParameters->list_font.StringWidth(result[0]), y);
			fOffscreenView->DrawString(result[0]);
			free(result[0]);
		}
	
		if (fTags->length)
		{
			sprintf(str, "%d:%.2d", (int)((fTags->length / 1000) / 60), (int)((fTags->length / 1000) % 60));
			src[0] = str;
			result[0] = (char*)malloc(strlen(src[0]) + 16);
			fParameters->list_font.GetTruncatedStrings(src, 1, B_TRUNCATE_MIDDLE, fOffsets->controls - fOffsets->time - 20, result);
			fOffscreenView->MovePenTo(fOffsets->controls - 10 - fParameters->list_font.StringWidth(result[0]), y);
			fOffscreenView->DrawString(result[0]);
			free(result[0]);
		}
	}

	fOffscreenView->Sync();
	fOffscreen->Unlock();
	view->DrawBitmap(fOffscreen, r, rect);

	int32	icon = eAudioIcon;

	if ((cistrncmp(Type(), "video/", 6) == 0) && (fParameters->icons[eVideoIcon]))
		icon = eVideoIcon;

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

void TMediaListItem::EditorCallBack(BMessage* msg)
{
	switch (msg->what)
	{
		case eDetachedFromWindow:
			RemoveEditor();
			break;

		case eEditAccepted:
			Edit(NULL, BRect(0, 0, 0, 0));
			break;

		case eEditCanceled:
			RemoveEditor();
			break;

		case 'ETLE':
			fView->RemoveChild(fEditor[eMediaEditorTitle]);
			delete fEditor[eMediaEditorTitle];
			fEditor[eMediaEditorTitle] = NULL;
			RemoveControls();
			break;

		case 'EART':
			fView->RemoveChild(fEditor[eMediaEditorArtist]);
			delete fEditor[eMediaEditorArtist];
			fEditor[eMediaEditorArtist] = NULL;
			RemoveControls();
			break;

		case 'EABM':
			fView->RemoveChild(fEditor[eMediaEditorAlbum]);
			delete fEditor[eMediaEditorAlbum];
			fEditor[eMediaEditorAlbum] = NULL;
			RemoveControls();
			break;

		case 'ETRK':
			fView->RemoveChild(fEditor[eMediaEditorTrack]);
			delete fEditor[eMediaEditorTrack];
			fEditor[eMediaEditorTrack] = NULL;
			RemoveControls();
			break;
	}
}


//--------------------------------------------------------------------

void TMediaListItem::Edit(BView* /* view */, BRect rect)
{
	if (!fEditor[eMediaEditorTitle])
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

		if (fIsMP3)
		{
			float	squeeze = min_c((fOffsets->controls - tmp) / fOffsets->time, 1);
			char	str[32];
			BString	track;

			r.left = fOffsets->title + 3;
			r.right = (fOffsets->artist - 5) * squeeze;
			fView->AddChild(fEditor[eMediaEditorTitle] = new Editor(this, r, rect, &fTags->title, fParameters, !fIsMP3, 'ETLE'));

			r.left = r.right + 8;
			r.right = r.left + ((fOffsets->album - fOffsets->artist) - 8) * squeeze;
			fView->AddChild(fEditor[eMediaEditorArtist] = new Editor(this, r, rect, &fTags->artist, fParameters, false, 'EART'));

			r.left = r.right + 8;
			r.right = r.left + ((fOffsets->track - fOffsets->album) - 8) * squeeze;
			fView->AddChild(fEditor[eMediaEditorAlbum] = new Editor(this, r, rect, &fTags->album, fParameters, false, 'EABM'));

			r.left = r.right + 8;
			r.right = r.left + ((fOffsets->time - fOffsets->track) - 8) * squeeze;
			sprintf(str, "%d", (int)fTags->track);
			track = str;
			fView->AddChild(fEditor[eMediaEditorTrack] = new Editor(this, r, rect, &track, fParameters, false, 'ETRK'));
		}
		else
		{
			r.left = fOffsets->title + 3;
			r.right = fOffsets->controls - tmp;
			fView->AddChild(fEditor[eMediaEditorTitle] = new Editor(this, r, rect, &fTags->title, fParameters, !fIsMP3, 'ETLE'));
		}

		r.right = fOffsets->controls;
		r.left = r.right - tmp;
		fView->AddChild(fEditControls = new EditControls(this, r, rect, fParameters));

		fEditor[eMediaEditorTitle]->MakeFocus(true);
	}
	else
	{
		if (fIsMP3)
		{
			int32	track;

			track = strtol(fEditor[eMediaEditorTrack]->Text(), NULL, 0);
			if ((strcmp(fTags->artist.String(), fEditor[eMediaEditorArtist]->Text()) != 0) ||
				(strcmp(fTags->album.String(), fEditor[eMediaEditorAlbum]->Text()) != 0) ||
				(strcmp(fTags->title.String(), fEditor[eMediaEditorTitle]->Text()) != 0) ||
				(fTags->track != track))
			{
				id3_tags	backup = *fTags;

				fTags->artist = fEditor[eMediaEditorArtist]->Text();
				fTags->album = fEditor[eMediaEditorAlbum]->Text();
				fTags->title = fEditor[eMediaEditorTitle]->Text();
				fTags->track = track;
				if (WriteIDTags(Path(), fTags) != B_NO_ERROR)
					*fTags = backup;
				else
					fFilesView->ItemChanged(this);
			}
		}
		else
		{
			if (Rename(fEditor[eMediaEditorTitle]->Text()) == B_NO_ERROR)
			{
				fTags->title = Leaf();
				fFilesView->ItemChanged(this);
			}
		}
		for (int32 loop = 0; loop < eMediaEditorCount; loop++)
		{
			if (fEditor[loop])
				fView->Window()->PostMessage(eEditAccepted, fEditor[loop]);
		}
	}
}


//--------------------------------------------------------------------

void TMediaListItem::RemoveControls()
{
	if (fEditControls)
	{
		fView->RemoveChild(fEditControls);
		delete fEditControls;
		fEditControls = NULL;
	}
}


//--------------------------------------------------------------------

void TMediaListItem::RemoveEditor()
{
	for (int32 loop = 0; loop < eMediaEditorCount; loop++)
	{
		if (fEditor[loop])
		{
			fView->RemoveChild(fEditor[loop]);
			delete fEditor[loop];
			fEditor[loop] = NULL;
		}
	}
	RemoveControls();
}
