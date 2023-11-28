/*-----------------------------------------------------------------*/
//
//	File:		FileListView.cpp
//
//	Written by:	Robert Polic
//
//	Copyright 2000, Be Incorporated
//
/*-----------------------------------------------------------------*/

#include "FileListView.h"

#include <Application.h>
#include <Bitmap.h>
#include <Directory.h>
#include <fs_attr.h>
#include <FtpProtocol.h>
#include <NodeInfo.h>
#include <ScrollView.h>
#include <Window.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define kDRAG_SLOP			  4
#define kSELECT				'mSEL'


//========================================================================

/* local */
static status_t		file_processor			(void*);
static status_t		copy_file				(BFile*, BFile*);
static status_t		create_file				(const char*, const char*, BFile*);
static bool			use_dock				(FileListItem*);
static bool			use_email				(FileListItem*);

/* global */
extern void			send_busy				(bool, const char*, const char*);
extern int32		cistrncmp				(const char*, const char*, int32);



/*=================================================================*/

FilesView::FilesView(BRect rect, const char* name, drawing_parameters* parameters)
 : BView			(rect, name, B_FOLLOW_ALL, B_WILL_DRAW),
   fParameters		(parameters),
   fList			(NULL)
{
	BRect		r(Bounds());

	r.bottom = fParameters->offsets[eTabHeight];
	fOffscreen = new BBitmap(r, B_RGBA32, true);
	fOffscreenView = new BView(r, "", B_FOLLOW_NONE, 0);
	fOffscreen->AddChild(fOffscreenView);

	r = Bounds();
	r.top += fParameters->offsets[eTabHeight];
	r.right -= B_V_SCROLL_BAR_WIDTH + 1;
	fList = new FileListView(r, fParameters);
	AddChild(new BScrollView("FileScrollView", fList, B_FOLLOW_ALL,
							 B_WILL_DRAW, false, true, B_PLAIN_BORDER));
}


/*-----------------------------------------------------------------*/

FilesView::~FilesView()
{
	MakeEmpty();
}


/*-----------------------------------------------------------------*/

void FilesView::Draw(BRect /* where */)
{
}


/*-----------------------------------------------------------------*/

void FilesView::MakeEmpty()
{
	if (fList)
	{
		FileListItem*		item;

		while ((item = dynamic_cast<FileListItem*>(fList->ItemAt(0))) != NULL)
		{
			fList->RemoveItem(item);
			delete(item);
		}
	}
}


/*-----------------------------------------------------------------*/

void FilesView::CopyFiles(const char* target)
{
	bool			first = true;
	int32			index;
	int32			selection = 0;
	BMessage*		copy_msg = new BMessage('fbCP');
	FileListItem*	item;

	send_busy(true, "copying", target);

	copy_msg->AddString("mounted_at", target);
	while ((index = fList->CurrentSelection(selection)) >= 0)
	{
		if ((item = dynamic_cast<FileListItem*>(fList->ItemAt(index))) != NULL)
		{
			if (first)
			{
				copy_msg->AddInt32("protocol", (int32)item->Protocol());
				first = false;
			}
			copy_msg->AddString("file", item->Path());
		}
		selection++;
	}
	if (selection)
		resume_thread(spawn_thread((status_t (*)(void *))file_processor,
				  "file_processor", B_DISPLAY_PRIORITY, copy_msg));
	else
		send_busy(false, "copying", target);
}


/*-----------------------------------------------------------------*/

void FilesView::DeleteFiles()
{
	bool			first = true;
	int32			index;
	BMessage*		delete_msg = new BMessage('fbDL');
	FileListItem*	item;

	send_busy(true, "deleting", "");

	while ((index = fList->CurrentSelection(0)) >= 0)
	{
		if ((item = dynamic_cast<FileListItem*>(fList->ItemAt(index))) != NULL)
		{
			if (first)
			{
				delete_msg->AddInt32("protocol", (int32)item->Protocol());
				first = false;
			}
			delete_msg->AddString("file", item->Path());
			fList->RemoveItem(item);
			delete(item);
		}
	}

	resume_thread(spawn_thread((status_t (*)(void *))file_processor,
				  "file_processor", B_DISPLAY_PRIORITY, delete_msg));
}


/*-----------------------------------------------------------------*/

void FilesView::EditFiles()
{
	int32			index;

	if ((index = fList->CurrentSelection(0)) >= 0)
	{
		FileListItem*	item;

		if ((item = dynamic_cast<FileListItem*>(fList->ItemAt(index))) != NULL)
		{
			item->Edit(this, fList->ItemFrame(index));
			//item->DrawItem(fList, fList->ItemFrame(index), true);
			fList->Invalidate(fList->ItemFrame(index));
		}
	}
}


/*-----------------------------------------------------------------*/

bool FilesView::Editing()
{
	bool	result = false;
	int32	index;

	if ((index = fList->CurrentSelection(0)) >= 0)
	{
		FileListItem*	item;

		if ((item = dynamic_cast<FileListItem*>(fList->ItemAt(index))) != NULL)
			result = item->Editing();
	}
	return result;
}


/*-----------------------------------------------------------------*/

void FilesView::GetFiles(BMessage* reply)
{
	int32			index;
	int32			selection = 0;
	FileListItem*	item;

	while ((index = fList->CurrentSelection(selection)) >= 0)
	{
		if ((item = dynamic_cast<FileListItem*>(fList->ItemAt(index))) != NULL)
		{
			off_t	size = 0;
			BEntry	entry(item->Path());

			reply->AddString("path", item->Path());
			reply->AddString("mime", item->Type());
			if (entry.InitCheck() == B_NO_ERROR)
				entry.GetSize(&size);
			reply->AddInt64("size", size);
		}
		selection++;
	}
}


/*-----------------------------------------------------------------*/

void FilesView::OpenFiles()
{
	int32			index;
	int32			selection = 0;
	FileListItem*	item;

	while ((index = fList->CurrentSelection(selection)) >= 0)
	{
		if ((item = dynamic_cast<FileListItem*>(fList->ItemAt(index))) != NULL)
		{
			BMessage	openURL;
			BString		url(item->Scheme());

			if (use_dock(item))
				openURL.what = 'dock';
			else if (use_email(item))
				openURL.what = 'oeml';
			else
				openURL.what = 'ourl';
			url << item->Path();
			openURL.AddString("url", url.String());
			openURL.AddInt32("groupid", 0);
			be_app->PostMessage(&openURL);
		}
		selection++;
	}
}


/*-----------------------------------------------------------------*/

void FilesView::DrawTab(float left, float right, BString label, int32 sorted, bool draw_right_edge)
{
	const char*	src[1];
	char*		result[1];
	BRect		r(left, 0, right, fParameters->offsets[eTabHeight] - 2);
	font_height	finfo;

	fOffscreen->Lock();
	(sorted) ?
		fOffscreenView->SetHighColor(fParameters->colors[eTabSortColor])
	  :
		fOffscreenView->SetHighColor(fParameters->colors[eTabColor]);

	fOffscreenView->FillRect(r);
	fOffscreenView->SetHighColor(fParameters->colors[eTabSeperatorColor]);
	fOffscreenView->StrokeLine(BPoint(r.left, r.bottom), BPoint(r.right, r.bottom));
	if (draw_right_edge)
	{
		int32	loop;

		for (loop = 0; loop < fParameters->offsets[eTabSpacerWidth]; loop++)
			fOffscreenView->StrokeLine(BPoint(r.right - loop, r.top), BPoint(r.right - loop, r.bottom));
	}

	if (sorted)
	{
		fOffscreenView->SetHighColor(fParameters->colors[eTabSortTextColor]);
		fOffscreenView->SetLowColor(fParameters->colors[eTabSortColor]);
	}
	else
	{
		fOffscreenView->SetHighColor(fParameters->colors[eTabTextColor]);
		fOffscreenView->SetLowColor(fParameters->colors[eTabColor]);
	}

	fParameters->tab_font.GetHeight(&finfo);
	fOffscreenView->SetFont(&fParameters->tab_font);
	fOffscreenView->MovePenTo(left + 10, (((fParameters->offsets[eTabHeight] - 2) - (finfo.ascent + finfo.descent + finfo.leading)) / 2) +
					(finfo.ascent + finfo.descent) - 2);
	src[0] = label.String();
	result[0] = (char*)malloc(strlen(src[0]) + 16);
	fParameters->tab_font.GetTruncatedStrings(src, 1, B_TRUNCATE_MIDDLE, right - left - 20, result);
	fOffscreenView->DrawString(result[0]);

	if (sorted)
	{
		int32	icon = eSortDescendingIcon;

		if (sorted == 1)
			icon = eSortAscendingIcon;

		if (0) //fParameters->icons[icon])
		{
			int32	height = 0;
			int32	width;
			uint32	flags;
			BRect	png = r;

			fParameters->icons[icon]->GetSize(&width, &height, &flags);

			png.left = fOffscreenView->PenLocation().x + 4;
			png.right = png.left + width - 1;
			png.top += (int)((Bounds().Height() - height) / 2);
			png.bottom = png.top + height - 1;

			if (png.right < right - 3)
			{
				fParameters->icons[icon]->FrameChanged(png, width, height);
				fParameters->icons[icon]->Draw(fOffscreenView, png);
			}
		}
		else
		{
			BPoint	location = fOffscreenView->PenLocation();

			if (location.x + 4 + 7 < right - 3)
			{
				location.y = (int)((fOffscreenView->Bounds().Height() - 7) / 2);
				if (sorted == 1)
				{
					fOffscreenView->StrokeLine(BPoint(location.x + 4 + 3, location.y + 0),
											   BPoint(location.x + 4 + 3, location.y + 0));
					fOffscreenView->StrokeLine(BPoint(location.x + 4 + 2, location.y + 2),
											   BPoint(location.x + 4 + 4, location.y + 2));
					fOffscreenView->StrokeLine(BPoint(location.x + 4 + 1, location.y + 4),
											   BPoint(location.x + 4 + 5, location.y + 4));
					fOffscreenView->StrokeLine(BPoint(location.x + 4 + 0, location.y + 6),
											   BPoint(location.x + 4 + 6, location.y + 6));
				}
				else
				{
					fOffscreenView->StrokeLine(BPoint(location.x + 4 + 3, location.y + 6),
											   BPoint(location.x + 4 + 3, location.y + 6));
					fOffscreenView->StrokeLine(BPoint(location.x + 4 + 2, location.y + 4),
											   BPoint(location.x + 4 + 4, location.y + 4));
					fOffscreenView->StrokeLine(BPoint(location.x + 4 + 1, location.y + 2),
											   BPoint(location.x + 4 + 5, location.y + 2));
					fOffscreenView->StrokeLine(BPoint(location.x + 4 + 0, location.y + 0),
											   BPoint(location.x + 4 + 6, location.y + 0));
				}
			}
		}
	}

	free(result[0]);

	fOffscreenView->Sync();
	fOffscreen->Unlock();
}


//====================================================================

FileListView::FileListView(BRect rect, drawing_parameters* parameters)
 : BListView		(rect, "FileListView", B_MULTIPLE_SELECTION_LIST, B_FOLLOW_ALL, B_WILL_DRAW),
   fParameters		(parameters),
   fDeselect		(false),
   fDragging		(false),
   fRolledOverItem	(NULL)
{
	BRect		r = Bounds();

	fOffscreen = new BBitmap(r, B_RGBA32, true);
	fOffscreenView = new BView(r, "", B_FOLLOW_NONE, 0);
	fOffscreen->AddChild(fOffscreenView);

	SetViewColor(B_TRANSPARENT_COLOR);
}


//--------------------------------------------------------------------

FileListView::~FileListView()
{
	delete fOffscreen;
}


/*-----------------------------------------------------------------*/

void FileListView::AllAttached()
{
	SetSelectionMessage(new BMessage(kSELECT));
	SetTarget(this);
	SetButtonState();
}


//--------------------------------------------------------------------

void FileListView::Draw(BRect where)
{
	int32 items = CountItems();
	BRect	r = Bounds() & where;

	if (items)
		r.top = ItemFrame(items - 1).bottom + 1;

	if (r.IsValid())
	{
		SetHighColor(fParameters->colors[eListColor]);
		FillRect(r);
	}
	BListView::Draw(where);
}


//--------------------------------------------------------------------

void FileListView::DetachedFromWindow()
{
	FileListItem*	item;

	item = dynamic_cast<FileListItem *>(ItemAt(CurrentSelection()));
	if (item)
	{
		BMessage	msg(eDetachedFromWindow);

		item->EditorCallBack(&msg);
	}
}


//--------------------------------------------------------------------

void FileListView::MessageReceived(BMessage* msg)
{
	switch (msg->what)
	{
		case kSELECT:
			SetButtonState();
			break;

		default:
			BListView::MessageReceived(msg);
	}
}


//--------------------------------------------------------------------

void FileListView::MouseDown(BPoint where)
{
	int32	index = IndexOf(where);

	MakeFocus(true);
	fDeselect = false;
	if (index >= 0)
	{
		FileListItem*	item;

		item = dynamic_cast<FileListItem *>(ItemAt(index));
		if (item)
		{
			bool	selected = IsItemSelected(index);
			uint32	mods;

			Window()->CurrentMessage()->FindInt32("modifiers", (int32*)&mods);
			if ((selected) && !(mods & B_CONTROL_KEY))
			{
				int32 clicks;

				Window()->CurrentMessage()->FindInt32("clicks", &clicks);

				if (clicks > 1)
				{
					BMessage	openURL;
					BString		url(item->Scheme());

					if (use_dock(item))
						openURL.what = 'dock';
					else if (use_email(item))
						openURL.what = 'oeml';
					else
						openURL.what = 'ourl';
					url << item->Path();
					openURL.AddString("url", url.String());
					openURL.AddInt32("groupid", 0);
					be_app->PostMessage(&openURL);
				}
				else
				{
					if (!(mods & B_SHIFT_KEY))
					{
						fDeselect = true;
						fSelectedItem = index;
					}
					fStart = where;
					fDragging = true;
				}
			}
			else if (mods & B_SHIFT_KEY)
			{
				int32	first = CurrentSelection();

				if (selected)
				{
					DeselectAll();
					first = index;
				}
				else if (first == -1)
					first = 0;
				else if (first > index)
				{
					int32	tmp = index;
					index = first;
					first = tmp;
				}
				for (int32 loop = first; loop <= index; loop++)
					Select(loop, true);
			}
			else if (mods & B_CONTROL_KEY)
			{
				if (selected)
					Deselect(index);
				else
					Select(index, true);
			}
			else
				Select(index);
  		}
	}
}


/*-----------------------------------------------------------------*/

void FileListView::MouseMoved(BPoint where, uint32 /* transit */, const BMessage*)
{
	if ((fDragging) &&
		((abs((int)(fStart.x - where.x)) > kDRAG_SLOP) ||
		(abs((int)(fStart.y - where.y)) > kDRAG_SLOP)))
	{
		int32			index = CurrentSelection();
		FileListItem*	item;

		item = dynamic_cast<FileListItem *>(ItemAt(index));
		if ((fParameters->flags[eSupportDrag]) && (item))
		{
			bool		first = true;
			int32		icon = eOtherIcon;
			int32		selection = 0;
			const char*	mime = item->Type();
			BBitmap*	drag_bits;
			BMessage	drag_msg('DRAG');
			BView*		offscreen_view;
			BRect		icon_rect(0, 0, 15, 15);

			if (cistrncmp(mime, "audio/", 6) == 0)
				icon = eAudioIcon;
			else if (cistrncmp(mime, "video/", 6) == 0)
				icon = eVideoIcon;
			else if (cistrncmp(mime, "image/", 6) == 0)
				icon = eImageIcon;
			else if (cistrncmp(mime, "text/html", 7) == 0)
				icon = eHTMLIcon;
			else if (cistrncmp(mime, "text/", 5) == 0)
				icon = eTextIcon;
			else if (cistrncmp(mime, "message/rfc822", 14) == 0)
				icon = eEmailIcon;

			drag_bits = new BBitmap(icon_rect, B_RGBA32, true);
			offscreen_view = new BView(drag_bits->Bounds(), "", B_FOLLOW_NONE, 0);
			drag_bits->AddChild(offscreen_view);

			drag_bits->Lock();
			offscreen_view->SetHighColor(0, 0, 0, 0);
			offscreen_view->FillRect(offscreen_view->Bounds());
			offscreen_view->SetDrawingMode(B_OP_ALPHA);
			offscreen_view->SetHighColor(0, 0, 0, 128);
			offscreen_view->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);
			if (fParameters->icons[icon])
			{
				fParameters->icons[icon]->FrameChanged(icon_rect, icon_rect.Width(), icon_rect.Height());
				fParameters->icons[icon]->Draw(offscreen_view, icon_rect);
			}
			drag_bits->Unlock();

			while ((index = CurrentSelection(selection)) >= 0)
			{
				if ((item = dynamic_cast<FileListItem*>(ItemAt(index))) != NULL)
				{
					if (first)
					{
						drag_msg.AddInt32("protocol", (int32)item->Protocol());
						first = false;
					}
					drag_msg.AddString("file", item->Path());
				}
				selection++;
			}
			DragMessage(&drag_msg, drag_bits, B_OP_ALPHA,
						BPoint(drag_bits->Bounds().Height() / 2,
							   drag_bits->Bounds().Width() / 2));
		}
		fDragging = false;
	}
	else if (0) //((Window()->IsActive()) && (IsFocus()) && (!fDragging) && (transit == B_INSIDE_VIEW))
	{
		int32	index = IndexOf(where);

		if (index >= 0)
		{
			FileListItem*	item;

			item = dynamic_cast<FileListItem *>(ItemAt(index));
			if (item != fRolledOverItem)
			{
				if (fRolledOverItem)
					fRolledOverItem->RollOver(false);
				fRolledOverItem = item;
				fRolledOverItem->RollOver(true);
				Invalidate(Bounds());
			}
		}
	}
}


//--------------------------------------------------------------------

void FileListView::MouseUp(BPoint /* where */)
{
	if (fDragging)
	{
		fDragging = false;

		if (fDeselect)
			DeselectExcept(fSelectedItem, fSelectedItem);
	}
	fDeselect = false;
}


//--------------------------------------------------------------------

void FileListView::SetButtonState()
{
	int32		state = 0;
	BMessage	msg('fbSL');

	if (CurrentSelection() != -1)
	{
		if (CurrentSelection(1)  != -1)
			state = 2;
		else
			state = 1;
	}

	msg.AddInt32("items", state);
	be_app->PostMessage(&msg);
}


//====================================================================

FileListItem::FileListItem(BString* path, BString* name, const char* type,
						   PROTOCOL p, drawing_parameters* parameters,
						   FilesView* view)
 : BListItem	(),
   fParameters	(parameters),
   fFilesView	(view),
   fRollOver	(false),
   fProtocol	(p)
{
	fPath = *path;
	fName = *name;
	fFileType = type;

	SetHeight(fParameters->offsets[eListItemHeight]);
}


//--------------------------------------------------------------------

void FileListItem::EditorCallBack(BMessage*)
{
}


//--------------------------------------------------------------------

void FileListItem::RollOver(bool in)
{
	fRollOver = in;
}


//--------------------------------------------------------------------

const char* FileListItem::Scheme()
{
	switch (fProtocol)
	{
		case eLocal:	return "file://";
		case eFTP:		return "ftp://";
		case eHTTP:		return "http://";
	}
	return "";
}


//--------------------------------------------------------------------

status_t FileListItem::Rename(const char* name)
{
	BEntry		entry(Path());
	status_t	result = B_NO_ERROR;

	if (strlen(name) == 0)
		return B_ERROR;
	if (strcmp(Leaf(), name) == 0)
		return B_ERROR;

	switch (fProtocol)
	{
		case eLocal:
			if ((result = entry.Rename(name)) == B_NO_ERROR)
			{
				BPath	path;
		
				entry.GetPath(&path);
				fPath = path.Path();
				fName = name;
			}
			break;

		case eFTP:
			{
				FtpProtocol*	ftp;
				BString			url(Scheme());
				URL				u;

				url.Append(Path(), strlen(Path()) - strlen(Leaf()) - 1);
				u = url.String();
				if ((ftp = dynamic_cast<FtpProtocol*>(Protocol::InstantiateProtocol(u.GetScheme()))) != NULL)
				{
					BMessage	errors;

					if ((result = ftp->OpenRW(u, URL(), &errors, 0, true)) == B_NO_ERROR)
					{
						if ((result = ftp->Rename(Leaf(), name)) == B_NO_ERROR)
						{
							fPath.Truncate(fPath.Length() - strlen(Leaf()));
							fPath.Append(name);
							fName = name;
						}
					}
					delete ftp;
				}
				else
					result = B_ERROR;
			}
			break;

		case eHTTP:
			break;
	}

	if (result != B_NO_ERROR)
	{
		BMessage	error('fbER');

		error.AddString("file", Path());
		error.AddString("message", strerror(result));
		error.AddInt32("error code", result);
		be_app->PostMessage(&error);
	}
	return result;
}


//--------------------------------------------------------------------

void FileListItem::Update(BView* owner, const BFont* font)
{
	BListItem::Update(owner, font);
	SetHeight(fParameters->offsets[eListItemHeight]);
}


/*=================================================================*/

status_t file_processor(void* data)
{
	BMessage*	msg = (BMessage*)data;
	BMessage	busy('fbBZ');
	status_t	result = B_NO_ERROR;

	switch (msg->what)
	{
		// Delete files
		case 'fbDL':
			{
				const char*	path;
				int32		index = 0;
				PROTOCOL	protocol;

				msg->FindInt32("protocol", (int32*)&protocol);
				switch (protocol)
				{
					case eLocal:	// delete local files
						while (msg->FindString("file", index, &path) == B_NO_ERROR)
						{
							BEntry	entry(path);

							if (entry.InitCheck() == B_NO_ERROR)
							{
								if ((result = entry.Remove()) != B_NO_ERROR)
								{
									BMessage	error('fbER');

									error.AddString("file", path);
									error.AddString("message", strerror(result));
									error.AddInt32("error code", result);
									be_app->PostMessage(&error);
								}
							}
							index++;
						}
						break;

					case eFTP:		// delete ftp files
						{
							FtpProtocol*	ftp = NULL;
							BMessage		errors;
							BString			leaf;
							BString			p;
							URL				u;

							while (msg->FindString("file", index, &path) == B_NO_ERROR)
							{
								BString			url("ftp://");

								p = path;
								int32	len = p.Length();
								int32	offset = p.FindLast("/");
								p.CopyInto(leaf, len - (len - offset) + 1,  len - offset + 1);
								p.Truncate(len - (leaf.Length() + 1));
								url.Append(p.String());
								u = url.String();
								if (!index)
								{
									if ((ftp = dynamic_cast<FtpProtocol*>(Protocol::InstantiateProtocol(u.GetScheme()))) == NULL)
										break;
								}

								if ((result = ftp->OpenRW(u, URL(), &errors, 0, true)) == B_NO_ERROR)
								{
									if ((result = ftp->Delete(leaf.String())) != B_NO_ERROR)
									{
										BMessage	error('fbER');
	
										error.AddString("file", path);
										error.AddString("message", strerror(result));
										error.AddInt32("error code", result);
										be_app->PostMessage(&error);
									}
								}
								index++;
							}
							delete ftp;
						}
						break;

					case eHTTP:		// delete http files
						break;
				}

				busy.AddString("action", "deleting");
				busy.AddString("volume", "");
			}
			break;

		// Copying files
		case 'fbCP':
			{
				const char*	volume;
				const char*	path;
				int32		index = 0;
				PROTOCOL	protocol;

				msg->FindString("mounted_at", &volume);
				msg->FindInt32("protocol", (int32*)&protocol);

				switch (protocol)
				{
					case eLocal:	// copy local file to target
						{
							FtpProtocol*	ftp = NULL;
							URL				u;

							while (msg->FindString("file", index, &path) == B_NO_ERROR)
							{
								BFile		src(path, B_READ_ONLY);

								if ((result = src.InitCheck()) == B_NO_ERROR)
								{
									BPath		name(path);

									// check to see if we're copying to ftp
									if (strncmp(volume, "ftp://", 6) == 0)
									{
										if (!ftp)
										{
											BMessage	errors;

											u = volume;
											if ((ftp = dynamic_cast<FtpProtocol*>(Protocol::InstantiateProtocol(u.GetScheme()))) == NULL)
												break;
											if ((result = ftp->OpenRW(u, URL(), &errors, 0, true)) != B_NO_ERROR)
											{
												delete ftp;
												ftp = NULL;
												break;
											}
										}
										if (ftp)
										{
											if ((result = ftp->SaveFileToFtp(&src, name.Leaf())) != B_NO_ERROR)
												break;
										}
									}
									else	// copying to a local file
									{
										BFile		dst;

										if ((result = create_file(volume, name.Leaf(), &dst)) == B_NO_ERROR)
										{
											if ((result = copy_file(&src, &dst)) != B_NO_ERROR)
												break;
										}
									}
								}
								index++;
							}
							delete ftp;
						}
						break;

					case eFTP:		// copy ftp file to local target
						{
							FtpProtocol*	ftp = NULL;
							BMessage		errors;
							BString			leaf;
							BString			p;
							URL				u;

							while (msg->FindString("file", index, &path) == B_NO_ERROR)
							{
								BFile			dst;
								BString			url("ftp://");

								p = path;
								int32	len = p.Length();
								int32	offset = p.FindLast("/");
								p.CopyInto(leaf, len - (len - offset) + 1,  len - offset + 1);
								p.Truncate(len - (leaf.Length() + 1));
								url.Append(p.String());
								u = url.String();
								if (!index)
								{
									if ((ftp = dynamic_cast<FtpProtocol*>(Protocol::InstantiateProtocol(u.GetScheme()))) == NULL)
										break;
								}

								if ((result = ftp->OpenRW(u, URL(), &errors, 0, true)) == B_NO_ERROR)
								{
									if ((result = create_file(volume, leaf.String(), &dst)) == B_NO_ERROR)
									{
										if ((result = ftp->SaveFileToLocal(&dst, leaf.String())) != B_NO_ERROR)
											break;
									}
								}
								index++;
							}
							delete ftp;			
						}
						break;

					case eHTTP:		// copy http file
						{
						}
						break;
				}

				if (result != B_NO_ERROR)
				{
					BMessage	error('fbER');

					error.AddString("file", path);
					error.AddString("message", strerror(result));
					error.AddInt32("error code", result);
					be_app->PostMessage(&error);
				}
				busy.AddString("action", "copying");
				busy.AddString("volume", volume);
			}
			break;
	}

	busy.AddBool("busy", false);
	be_app->PostMessage(&busy);

	delete msg;
	return B_NO_ERROR;
}


//--------------------------------------------------------------------

status_t copy_file(BFile* src, BFile* dst)
{
	char		attr[B_FILE_NAME_LENGTH];
	void*		buffer;
	ssize_t		read = 0;
	ssize_t		size = 0;
	attr_info	info;
	status_t	result;
	BNodeInfo	dst_node;
	BNodeInfo	src_node;

	read = size = 16 * 1024;
	buffer = malloc(size);

	/* copy file contents */
	while ((read == size) && ((read = src->Read(buffer, size)) > 0))
	{
		if ((result = dst->Write(buffer, read)) < 0)
			goto done;
	}

	/* copy attributes */
	while (src->GetNextAttrName(attr) == B_NO_ERROR)
	{
		src->GetAttrInfo(attr, &info);
		if (info.size > size)
		{
			size = info.size;
			buffer = realloc(buffer, size);
		}
		src->ReadAttr(attr, info.type, 0, buffer, info.size);
		dst->WriteAttr(attr, info.type, 0, buffer, info.size);
	}

	/* set file type */
	if ((src_node.SetTo(src) == B_NO_ERROR) && (dst_node.SetTo(dst)))
	{
		src_node.GetType(attr);
		dst_node.SetType(attr);
	}
	result = B_NO_ERROR;

done:;
	free(buffer);
	return result;
}


//--------------------------------------------------------------------

status_t create_file(const char* volume, const char* target, BFile* file)
{
	int32		index = 1;
	BDirectory	dir(volume);
	BString		name(target);
	BString		extension("");
	BString		file_name(target);
	status_t	result;

	if ((strlen(target) > 4) && (target[strlen(target) - 4] == '.'))
	{
		name.Truncate(strlen(target) - 4);
		extension.SetTo(&target[strlen(target) - 4]);
	}

	if ((result = dir.InitCheck()) == B_NO_ERROR)
	{
		while ((result = dir.CreateFile(file_name.String(), file, true)) != B_ERROR)
		{
			if (result != B_FILE_EXISTS)
				break;
			file_name.SetTo("");
			file_name << name << index++ << extension;
		}
	}
	return result;
}


//--------------------------------------------------------------------

bool use_dock(FileListItem* item)
{
	char*		dock_items[] = {"audio/mid",
								"audio/midi",
								"audio/x-midi",
								"audio/rmf",
								"audio/x-rmf",
								"audio/x-aiff",
								"audio/aiff",
								"audio/wav",
								"audio/x-wav",
								"audio/mpeg",
								"audio/x-mpeg",
								"audio/x-mpegurl",
								"audio/x-scpls",
								"audio/basic",
								""};
	const char*	type = item->Type();
	int32		index = 0;
	int32		len;

	while ((len = strlen(dock_items[index])))
	{
		if (cistrncmp(type, dock_items[index], len) == 0)
			return true;
		index++;
	}
	return false;
}


//--------------------------------------------------------------------

bool use_email(FileListItem* item)
{
	return (cistrncmp(item->Type(), "message/rfc822", 14) == 0);
}
