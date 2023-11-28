/*-----------------------------------------------------------------*/
//
//	File:		FileBrowserView.cpp
//
//	Written by:	Robert Polic
//
//	Copyright 2001, Be Incorporated
//
/*-----------------------------------------------------------------*/

#include <Application.h>
#include <Bitmap.h>
#include <Directory.h>
#include <Entry.h>
#include <Font.h>
#include <FtpProtocol.h>
#include <fs_attr.h>
#include <InterfaceDefs.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Roster.h>
#include <Path.h>
#include <String.h>
#include <StringBuffer.h>
#include <Window.h>

#include <stdio.h>

#include "FileBrowserView.h"

#define kFOLDER_MARGIN	2
#define kSLASH			"/ "

// uncomment to require dbl-clicking folder to open
//#define kDOUBLE_CLICK

//====================================================================

/* local */
static status_t		auto_scroller			(void*);
static status_t		file_processor			(void*);
static void			copy_item				(BEntry*, BDirectory*, void*, ssize_t);
static void			delete_item				(const char*, bool);
static status_t		list_entries			(URL*, BMessage*);
static int32		parse_buffer			(char*, ssize_t, URL*, BMessage*);

/* global */
void				send_busy_status		(const char*, const char*, bool);
void				send_error_message		(const char*, status_t);
void				send_selection_count	(int32);
extern int32		cistrncmp				(const char*, const char*, int32);


//====================================================================

FileBrowserView::FileBrowserView(BRect rect, drawing_parameters* parameters, const char* url)
	:BView(rect, "*file_browser_view", B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED | B_FRAME_EVENTS),
	fQuit(false),
	fAutoScroll(0),
	fOffscreen(NULL),
	fParameters(parameters),
	fClicked(NULL),
	fNodeList(NULL)
{
	// cache some interesting widths
	fWidths[eSlashWidth] = parameters->path_font.StringWidth(kSLASH);
	fWidths[eEllipsisWidth] = parameters->path_font.StringWidth(B_UTF8_ELLIPSIS);
	fWidths[eLeftRightMarginWidth] = parameters->path_font.StringWidth("M");
	fWidths[eFolderWidth] = 0;
	fParameters->path_font.GetHeight(&fFontInfo);

	fURL.SetTo(url);
	//fURL.SetTo("ftp://polic:SnabToph@ftp.scruznet.com/users/polic/private");

	SetEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
	Show();
}


//--------------------------------------------------------------------

FileBrowserView::~FileBrowserView()
{
	status_t	result;

	fQuit = true;
	wait_for_thread(fAutoScroller, &result);

	delete fOffscreen;
	DeleteNodeList(fNodeList);
}


//--------------------------------------------------------------------

void FileBrowserView::AttachedToWindow()
{
	BRect		r = Bounds();

	// construct the list view
	r.top = fParameters->offsets[ePathHeight] + 1;
	AddChild(fFileListView = new FileListView(r, fParameters, this));

	// offsreen view where all drawing for the path occurs
	fPathBounds = Bounds();
	fPathBounds.bottom = fParameters->offsets[ePathHeight];
	fOffscreen = new BBitmap(fPathBounds, B_RGBA32, true);
	fPathView = new BView(fPathBounds, "path_view", B_FOLLOW_NONE, 0);
	fOffscreen->AddChild(fPathView);
	fOffscreen->Lock();
	fPathView->SetFont(&fParameters->path_font);
	fOffscreen->Unlock();

	SetViewColor(B_TRANSPARENT_32_BIT);
	SetPath();
	fFileListView->MakeFocus(true);

	resume_thread(fAutoScroller = spawn_thread((status_t (*)(void *))auto_scroller,
				  "auto_scroller", B_DISPLAY_PRIORITY, this));
}


//--------------------------------------------------------------------

void FileBrowserView::Draw(BRect /* where */)
{
	fOffscreen->Lock();
	DrawBitmapAsync(fOffscreen);
	fOffscreen->Unlock();
}


//--------------------------------------------------------------------

void FileBrowserView::MessageReceived(BMessage* msg)
{
	switch (msg->what)
	{
		case 'DATA':
			{
				const char*		target = NULL;
				int32			index = 0;
				BMessage*		list = new BMessage();
				BPoint			where;
				BString			dst;
				entry_ref		ref;
				node*			item = fNodeList;

				list->AddInt32("function", eCopyItems);
				msg->FindPoint("_drop_point_", &where);
				where = ConvertFromScreen(where);
				while (item)
				{
					dst.Append("/");
					dst.Append(item->name);
					if ((where.x >= item->offset) && (where.x <= item->offset + item->total_width))
					{
						target = dst.String();
						break;
					}
					item = item->child;
				}
				if (target == NULL)
					break;

				list->AddString("volume", target);

				while (msg->FindRef("refs", index, &ref) == B_NO_ERROR)
				{
					BPath	path(&ref);

					list->AddString("path", path.Path());
					index++;
				}
				if (index)
					resume_thread(spawn_thread((status_t (*)(void *))file_processor,
					  "file_processor", B_DISPLAY_PRIORITY, list));
			}
			break;

		case eOpenParent:
			OpenParent();
			break;

		case eDisplayVolume:
			fURL.SetTo(msg->FindString("url"));
			SetPath();
			break;

		case eOpenItems:
			{
				FileListItem*	item = dynamic_cast<FileListItem*>(fFileListView->FocusRow());

				if (item)
					OpenItem(item);
			}
			break;

		case eEditItem:
			fFileListView->Edit();
			break;

		case eMessageDropped:
			{
				BMessage*	list = new BMessage(*msg);

				list->AddInt32("function", eCopyItems);
				resume_thread(spawn_thread((status_t (*)(void *))file_processor,
				  "file_processor", B_DISPLAY_PRIORITY, list));
			}
			break;

		case eCopyItems:
			{
				BMessage*	list = BuildItemList();

				list->AddInt32("function", eCopyItems);
				list->AddString("volume", msg->FindString("volume"));
				resume_thread(spawn_thread((status_t (*)(void *))file_processor,
				  "file_processor", B_DISPLAY_PRIORITY, list));
			}
			break;

		case eAttachItems:
			{
				BMessage*	list = BuildItemList(false, true);

				msg->SendReply(list);
				delete list;
			}
			break;

		case eDeleteItems:
			{
				BMessage*	list = BuildItemList(true);

				send_selection_count(0);
				list->AddInt32("function", eDeleteItems);
				list->AddString("volume", fNodeList->name.String());
				resume_thread(spawn_thread((status_t (*)(void *))file_processor,
				  "file_processor", B_DISPLAY_PRIORITY, list));
			}
			break;

		case eMakeDirectory:
			fFileListView->MakeDirectory(msg->FindString("name"));
			break;

		default:
			BView::MessageReceived(msg);
	}
}


//--------------------------------------------------------------------

void FileBrowserView::MouseDown(BPoint where)
{
	BString	new_path;
	node*	item = fNodeList;

	if (fPathBounds.Contains(where))
	{
		while (item)
		{
			new_path.Append("/");
			new_path.Append(item->name);
			if ((where.x >= item->offset) && (where.x <= item->offset + item->total_width))
			{
#ifdef kDOUBLE_CLICK
				if ((fClicked == item) && (Window()->CurrentMessage()->FindInt32("clicks") > 1))
#endif
				{
					if ((item->child) || ((!item->child) && (item->greyed)))
					{
						if ((!item->greyed) && (item->child) && (item->child->greyed))
						{
						}
						else
						{
							NewURL(&new_path, item->selection, item->position);
							item->greyed = false;
							while (item->child)
							{
								item->child->greyed = true;
								item = item->child;
							}
							DrawPath();
						}
					}
				}
#ifdef kDOUBLE_CLICK
				else
					fClicked = item;
#endif
				return;
			}
			item->greyed = false;
			item = item->child;
		}
#ifdef kDOUBLE_CLICK
		fClicked = NULL;
#endif
	}
}


//--------------------------------------------------------------------

void FileBrowserView::MouseMoved(BPoint where, uint32 code, const BMessage* msg)
{
	if (Window()->IsActive())
	{
		int32	index = 0;
		node*	item = fNodeList;

		if (fPathBounds.Contains(where))
		{
			if ((code == B_ENTERED_VIEW) || (code == B_INSIDE_VIEW))
			{
				while (item)
				{
					if ((where.x >= item->offset) && (where.x <= item->offset + item->total_width))
					{
						DrawPath(index, true);
						break;
					}
					index++;
					item = item->child;
				}
				if (!item)
					DrawPath();
			}
		}
		else if ((fAutoScroll) || ((msg) && (msg->what == 'DATA')))
		{
			BPoint	p = ConvertToScreen(where);
			BRect	r = fFileListView->ScrollView()->Frame();

			if (code == B_EXITED_VIEW)
				DrawPath();
			fFileListView->ConvertToScreen(&r);
			if ((p.x >= r.left) && (p.y <= r.right))
			{
				if (p.y < r.top)
					fAutoScroll = (int32)(r.top - p.y) * -1;
				else if (p.y > r.bottom)
					fAutoScroll = (int32)(p.y - r.bottom);
				else
				{
					BRow*	focus = fFileListView->FocusRow();
					BRow*	new_focus;

					p = fFileListView->ScrollView()->ConvertFromScreen(p);
					new_focus = fFileListView->RowAt(p);

					if ((new_focus) && (new_focus != focus))
					{
						fFileListView->SetFocusRow(new_focus);
						fFileListView->DeselectAll();
						fFileListView->AddToSelection(new_focus);
					}
					fAutoScroll = 0;
				}
			}
			else
				fAutoScroll = 0;
		}
		else if (code == B_EXITED_VIEW)
			DrawPath();
	}
}


//--------------------------------------------------------------------

void FileBrowserView::MouseUp(BPoint where)
{
	fAutoScroll = 0;
	BView::MouseUp(where);
}


//--------------------------------------------------------------------

void FileBrowserView::AutoScroller()
{
	while (!fQuit)
	{
		if (fAutoScroll)
		{
			status_t	lock;

			lock = Window()->LockWithTimeout(500000);
			if (lock != B_TIMED_OUT)
			{
				BPoint	p(1, 0);
				BRow*	focus = fFileListView->FocusRow();
				BRow*	new_focus;

				fFileListView->ScrollView()->ScrollBy(0, fAutoScroll);
				if (fAutoScroll < 0)
					p.y = fFileListView->ScrollView()->Bounds().top + 1;
				else
					p.y = fFileListView->ScrollView()->Bounds().bottom - 1;

				new_focus = fFileListView->RowAt(p);
				if ((!new_focus) && (fAutoScroll > 0))
					new_focus = fFileListView->RowAt(fFileListView->CountRows() - 1);
				if ((new_focus) && (new_focus != focus))
				{
					fFileListView->SetFocusRow(new_focus);
					fFileListView->DeselectAll();
					fFileListView->AddToSelection(new_focus);
				}
			}
			if (lock == B_OK)
				Window()->Unlock();
		}
		snooze(100000);
	}
}


//--------------------------------------------------------------------

void FileBrowserView::AddDirectoryEntries(const char* parent, BMessage* reply)
{
	URL		url(parent);

	if (cistrncmp("file", url.GetScheme(), 4) == 0)
	{
		char*	p = (char*)malloc(strlen(url.GetPath()) + 1);

		url.GetUnescapedPath(p, strlen(url.GetPath()) + 1);
		BEntry		entry(p);
		BDirectory	dir(&entry);

		while (dir.GetNextEntry(&entry) == B_NO_ERROR)
		{
			if (entry.IsDirectory())
			{
				BPath	path;
				BString	s;

				entry.GetPath(&path);
				s << url.GetScheme() << "://" << path.Path();
				AddDirectoryEntries(s.String(), reply);
			}
			else
			{
				BNode		node(&entry);
				BNodeInfo	node_info(&node);
				BString		s;
				off_t		size;

				node.GetSize(&size);

				reply->AddString("url", parent);
				reply->AddString("path", p);
				//reply->AddString("mime", item->MimeType());
				reply->AddInt64("size", size);
				reply->AddBool("is_directory", false);
			}
		}
		free(p);
	}
	else
	{
	}
}


//--------------------------------------------------------------------

BMessage* FileBrowserView::BuildItemList(bool remove, bool recursive)
{
	BMessage*		reply = new BMessage();
	FileListItem*	item = NULL;

	while ((item = dynamic_cast<FileListItem*>(fFileListView->CurrentSelection(item))))
	{
		char*	u = item->GetURL();

		if ((item->IsDirectory()) && (recursive))
			AddDirectoryEntries(u, reply);
		else
		{
			reply->AddString("url", u);
			reply->AddString("path", item->Path());
			reply->AddString("mime", item->MimeType());
			reply->AddInt64("size", item->Size());
			reply->AddBool("is_directory", item->IsDirectory());
		}
		free(u);

		if (remove)
		{
			fFileListView->RemoveRow(item);
			delete item;
			item = NULL;
		}
	}
	return reply;
}


//--------------------------------------------------------------------

void FileBrowserView::BuildNodeList(const char* start)
{
	DeleteNodeList(fNodeList);
	fNodeList = NULL;

	BPath	path(start);

	while ((strlen(path.Leaf())) && (path.InitCheck() == B_NO_ERROR))
	{
		fNodeList = new node(NULL, fNodeList, path.Leaf());
		fNodeList->name_width = fParameters->path_font.StringWidth(fNodeList->name.String());
		path.GetParent(&path);
	}
}


//--------------------------------------------------------------------

void FileBrowserView::DeleteNodeList(node* start)
{
	node*	item = start;

	while (item)
	{
		node*	child = item->child;

		delete item;
		item = child;
	}
}


//--------------------------------------------------------------------

void FileBrowserView::DrawFolder(BView* view, bool grey)
{
	if (fParameters->icons[eFolderOpenedIcon])
	{
		BPoint	pen = view->PenLocation();
		BRect	r = fParameters->icons[eFolderOpenedIcon]->Bounds();

		view->PushState();
		view->SetDrawingMode(B_OP_ALPHA);
		if (grey)
		{
			view->SetHighColor(0, 0, 0, 32);
			view->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
		}
		view->DrawBitmap(fParameters->icons[eFolderOpenedIcon],
						 BPoint(pen.x, (view->Bounds().Height() - r.Height()) / 2));
		view->PopState();
		view->MovePenTo(pen.x + fWidths[eFolderWidth], pen.y);
	}
}


//--------------------------------------------------------------------

void FileBrowserView::DrawPath(int32 must_show_item, bool hilite)
{
	float		y;
	int32		index = must_show_item;
	node*		item = fNodeList;
	BRect		r;

	fOffscreen->Lock();
	r = fPathView->Bounds();
	fPathView->SetHighColor(fParameters->colors[ePathColor]);
	fPathView->FillRect(r);

	if (index == -1)
	{
		while (item)
		{
			if (item->greyed)
			{
				must_show_item = index;
				break;
			}
			index++;
			item = item->child;
		}
		item = fNodeList;
	}
	
	if ((fWidths[eFolderWidth] == 0) && (fParameters->icons[eFolderOpenedIcon]))
		fWidths[eFolderWidth] = fParameters->icons[eFolderOpenedIcon]->Bounds().Width() + kFOLDER_MARGIN + kFOLDER_MARGIN;

	Fit(r.Width() - (2 * fWidths[eLeftRightMarginWidth]), must_show_item);

	y = r.top + ((r.Height() - (fFontInfo.ascent + fFontInfo.descent + fFontInfo.leading)) / 2) +
				(fFontInfo.ascent + fFontInfo.descent) - 2;
	fPathView->SetLowColor(fParameters->colors[ePathColor]);
	fPathView->MovePenTo(fWidths[eLeftRightMarginWidth], y);

	index = 0;
	while (item)
	{
		bool	grey = false;
		BPoint	pen = fPathView->PenLocation();
		item->offset = pen.x;

		if ((index == must_show_item) && (hilite))
			fPathView->SetHighColor(fParameters->colors[ePathDropColor]);
		else if (item->greyed)
		{
			fPathView->SetHighColor(fParameters->colors[ePathGreyedTextColor]);
			grey = true;
		}
		else
			fPathView->SetHighColor(fParameters->colors[ePathTextColor]);
		index++;

		switch (item->draw)
		{
			case eDrawIconNameSlash:
				DrawFolder(fPathView, grey);
				pen = fPathView->PenLocation();
				fPathView->DrawString(item->name.String());
#ifndef kDOUBLE_CLICK
				fPathView->StrokeLine(BPoint(pen.x, y + 2), BPoint(pen.x + item->name_width, y + 2));
#endif
				fPathView->MovePenTo(pen.x + item->name_width, pen.y);
				fPathView->DrawString(kSLASH);
				item->total_width = fWidths[eFolderWidth] + item->name_width + fWidths[eSlashWidth];
				break;

			case eDrawIconEllipsisSlash:
				DrawFolder(fPathView, grey);
				pen = fPathView->PenLocation();
				fPathView->DrawString(B_UTF8_ELLIPSIS);
#ifndef kDOUBLE_CLICK
				fPathView->StrokeLine(BPoint(pen.x, y + 2), BPoint(pen.x + fWidths[eEllipsisWidth], y + 2));
#endif
				fPathView->MovePenTo(pen.x + fWidths[eEllipsisWidth], pen.y);
				fPathView->DrawString(kSLASH);
				item->total_width = fWidths[eFolderWidth] + fWidths[eEllipsisWidth] + fWidths[eSlashWidth];
				break;

			case eDrawIconSlash:
				DrawFolder(fPathView, grey);
				fPathView->DrawString(kSLASH);
				item->total_width = fWidths[eFolderWidth] + fWidths[eSlashWidth];
				break;

			case eDrawIcon:
				DrawFolder(fPathView, grey);
				item->total_width = fWidths[eFolderWidth];
				break;

			case eDrawSlash:
				fPathView->DrawString(kSLASH);
				item->total_width = fWidths[eSlashWidth];
				break;

			case eDrawNone:
				break;
		}
		item = item->child;
	}

	fPathView->SetHighColor(fParameters->colors[eListSelectedFrameColor]);
	fPathView->StrokeLine(BPoint(r.left, r.bottom), BPoint(r.right, r.bottom));
	fPathView->Sync();
	fOffscreen->Unlock();
	Invalidate(Bounds());
}


//--------------------------------------------------------------------

void FileBrowserView::Fit(float max_width, int32 must_show_item)
{
	float	width = 0;
	node*	item = fNodeList;

	while (item)
	{
		width += fWidths[eFolderWidth] + item->name_width + fWidths[eSlashWidth];
		item->draw = eDrawIconNameSlash;
		item = item->child;
	}

	if (width > max_width)		// doesn't fit, try showing first, must_show and and as many
								// that can fit on the end in full and icon/ellipsis/slash
								// for all others
	{
		int32	index = 0;
		node*	last_item = NULL;

		width = 0;
		item = fNodeList;
		while (item)
		{
			if ((index == 0) || (index == must_show_item) || (item->child == NULL))
			{
				width += fWidths[eFolderWidth] + item->name_width + fWidths[eSlashWidth];
				item->draw = eDrawIconNameSlash;
			}
			else
			{
				width += fWidths[eFolderWidth] +  fWidths[eEllipsisWidth] + fWidths[eSlashWidth];
				item->draw = eDrawIconEllipsisSlash;
			}
			last_item = item;
			item = item->child;
			index++;
		}
		if (width < max_width)	// fits, see if we can show more items at the end
		{
			item = last_item->parent;
			while (item)
			{
				if (width + (item->name_width - fWidths[eEllipsisWidth]) < max_width)
				{
					item->draw = eDrawIconNameSlash;
					width += (item->name_width - fWidths[eEllipsisWidth]);
				}
				else
					break;
				item = item->parent;
			}
		}
		else					// still doesn't fit, try showing must_show and as many
								// that can fit on the end (same as above except for the
								// first item)
		{
			index = 0;
			width = 0;
			item = fNodeList;
			while (item)
			{
				if ((index == must_show_item) || (item->child == NULL))
				{
					width += fWidths[eFolderWidth] + item->name_width + fWidths[eSlashWidth];
					item->draw = eDrawIconNameSlash;
				}
				else
				{
					width += fWidths[eFolderWidth] +  fWidths[eEllipsisWidth] + fWidths[eSlashWidth];
					item->draw = eDrawIconEllipsisSlash;
				}
				last_item = item;
				item = item->child;
				index++;
			}
			if (width < max_width)	// fits, see if we can show more items at the end
			{
				item = last_item->parent;
				while (item)
				{
					if (width + (item->name_width - fWidths[eEllipsisWidth]) < max_width)
					{
						item->draw = eDrawIconNameSlash;
						width += (item->name_width - fWidths[eEllipsisWidth]);
					}
					else
						break;
					item = item->parent;
				}
			}
			else				// still doesn't fit, just show icons for everything but
								// must_show item
			{
				index = 0;
				width = 0;
				item = fNodeList;
				while (item)
				{
					if ((index == must_show_item) || (item->child == NULL))
					{
						width += fWidths[eFolderWidth] + item->name_width + fWidths[eSlashWidth];
						item->draw = eDrawIconNameSlash;
					}
					else
					{
						width += fWidths[eFolderWidth];
						item->draw = eDrawIcon;
					}
					last_item = item;
					item = item->child;
					index++;
				}
				if (width < max_width)	// fits, see if we can show more items at the end
				{
					item = last_item->parent;
					while (item)
					{
						if (width + (item->name_width + fWidths[eSlashWidth]) < max_width)
						{
							item->draw = eDrawIconNameSlash;
							width += (item->name_width + fWidths[eSlashWidth]);
						}
						else
							break;
						item = item->parent;
					}
				}
			}
		}
	}
}


//--------------------------------------------------------------------

void FileBrowserView::NewURL(BString* path, int32 selected, BPoint position)
{
	BString	url(fURL.GetScheme());

	url << "://";
	if (cistrncmp("ftp", fURL.GetScheme(), 3) == 0)
		url << fURL.GetUserName() << ":"
			<< fURL.GetPassword() << "@"
			<< fURL.GetHostName();
	url << path->String();
	fURL.SetTo(url.String());
	fFileListView->SetURL(&fURL, selected, position);
}


//--------------------------------------------------------------------

void FileBrowserView::OpenItem(FileListItem* item)
{
	if (item->IsDirectory())
	{
		BString	new_path(item->Path());
		node*	nodes = fNodeList;

		while (nodes)
		{
			if (nodes->greyed)
			{
				nodes = nodes->parent;
				DeleteNodeList(nodes->child);
				break;
			}
			else if (nodes->child)
				nodes = nodes->child;
			else
				break;
		}
		if (fFileListView->FocusRow())
			nodes->selection = fFileListView->IndexOf(fFileListView->FocusRow());
		nodes->position = fFileListView->ScrollView()->Bounds().LeftTop();
		nodes = new node(nodes, NULL, item->Leaf());
		nodes->name_width = fParameters->path_font.StringWidth(nodes->name.String());
		DrawPath();
		// notify list view
		NewURL(&new_path);
	}
	else
	{
		char*		url = item->GetURL();
		BMessage	open;

		open.what = OpenWith(item->MimeType());
		open.AddString("url", url);
		free(url);
		open.AddInt32("groupid", 0);
		be_app->PostMessage(&open);
	}
}


//--------------------------------------------------------------------

void FileBrowserView::OpenParent()
{
	BString	new_path;
	node*	item = fNodeList;

	while (item)
	{
		if ((!item->child) || ((item->child) && (item->child->greyed)))
		{
			node*	parent = item->parent;
			if (!parent)
				break;
			item->greyed = true;
			DrawPath();
			NewURL(&new_path, parent->selection, parent->position);
			break;
		}
		new_path.Append("/");
		new_path.Append(item->name);
		item = item->child;
	}
}


//--------------------------------------------------------------------

struct open_table
{
	char*		mime_type;
	uint32		open_with;
};


uint32 FileBrowserView::OpenWith(const char* type)
{
	open_table	table[] =
				{
					{"audio/mid",		'dock'},
					{"audio/midi",		'dock'},
					{"audio/x-midi",	'dock'},
					{"audio/rmf",		'dock'},
					{"audio/x-rmf",		'dock'},
					{"audio/x-aiff",	'dock'},
					{"audio/aiff",		'dock'},
					{"audio/wav",		'dock'},
					{"audio/x-wav",		'dock'},
					{"audio/mpeg",		'dock'},
					{"audio/x-mpeg",	'dock'},
					{"audio/x-mpegurl",	'dock'},
					{"audio/x-scpls",	'dock'},
					{"audio/basic",		'dock'},
					{"message/rfc822",	'oeml'},
					{"",				'ourl'}
				};
	int32		index = 0;
	int32		len;

	while (1)
	{
		len = strlen(table[index].mime_type);
		if ((!len) || (cistrncmp(type, table[index].mime_type, len) == 0))
			break;
		index++;
	}
	return table[index].open_with;
}


//--------------------------------------------------------------------

void FileBrowserView::SetPath()
{
	size_t path_buf_length = strlen(fURL.GetPath()) + 1;
	char*	path = (char*)malloc(path_buf_length);

	fURL.GetUnescapedPath(path, path_buf_length);
	if (strlen(path))
		BuildNodeList(path);
	free(path);
	DrawPath();
	// notify list view
	fFileListView->SetURL(&fURL);
}


//====================================================================

status_t auto_scroller(void* data)
{
	((FileBrowserView*)data)->AutoScroller();
	return B_NO_ERROR;
}


//====================================================================

status_t file_processor(void* data)
{
	const char*	volume;
	int32		index = 0;
	BMessage*	list = (BMessage*)data;

	volume = list->FindString("volume");
	switch (list->FindInt32("function"))
	{
		case eDeleteItems:
			{
				send_busy_status("deleting", volume, true);
				while (list->HasString("url", index))
				{
					delete_item(list->FindString("url", index), 
								list->FindBool("is_directory", index));
					index++;
				}
				send_busy_status("deleting", volume, false);
			}
			break;

		case eCopyItems:
			{
				BEntry	entry(volume);

				if (entry.InitCheck() == B_NO_ERROR)
				{
					BDirectory	target(&entry);

					if (target.InitCheck() == B_NO_ERROR)
					{
						void*	buffer;
						ssize_t	buffer_len = 64 * 1024;

						send_busy_status("copying", volume, true);
						while ((buffer = malloc(buffer_len)) == NULL)
							buffer_len = buffer_len >> 2;
						while (list->HasString("path", index))
						{
							BEntry	entry(list->FindString("path", index), true);

							copy_item(&entry, &target, buffer, buffer_len);
							index++;
						}
						free(buffer);
						send_busy_status("copying", volume, false);
					}
					else
						send_error_message(volume, target.InitCheck());
				}
				else
					send_error_message(volume, entry.InitCheck());
			}
			break;
	}
	delete list;
	return B_NO_ERROR;
}


//--------------------------------------------------------------------

void copy_item(BEntry* source, BDirectory* target, void* buffer, ssize_t buffer_len)
{
	BEntry		dst_entry;
	BPath		path(source);
	status_t	result = B_NO_ERROR;
	node_ref	dst_ref;
	node_ref	src_ref;

	target->GetEntry(&dst_entry);
	dst_entry.GetNodeRef(&dst_ref);
	source->GetNodeRef(&src_ref);

	// if copying to the same device, move instead
	if (dst_ref.device == src_ref.device)
	{
		if ((result = source->MoveTo(target)) != B_NO_ERROR)
			send_error_message(path.Path(), result);
	}
	// if it's a directory, create it and copy recursively
	else if (source->IsDirectory())
	{
		BDirectory	dir;

		if ((result = target->CreateDirectory(path.Leaf(), &dir)) == B_NO_ERROR)
		{
			BDirectory	src(source);
			BEntry		item;

			while (src.GetNextEntry(&item) == B_NO_ERROR)
				copy_item(&item, &dir, buffer, buffer_len);
		}
		else if (result == B_FILE_EXISTS)
		{
			// what do we do?
		}
		if (result != B_NO_ERROR)
			send_error_message(path.Path(), result);
	}
	else if (source->IsFile())
	{
		BFile	dst_file;

		if ((result = target->CreateFile(path.Leaf(), &dst_file, true)) == B_NO_ERROR)
		{
			BFile	src_file(source, B_READ_ONLY);

			if ((result = src_file.InitCheck()) == B_NO_ERROR)
			{
				ssize_t		read = buffer_len;

				// copy data
				while ((read == buffer_len) && ((read = src_file.Read(buffer, buffer_len)) > 0))
				{
					if ((result = dst_file.Write(buffer, read)) < 0)
						break;
				}
				// copy attributes
				if ((read >= 0) && (result >= 0))
				{
					char			name[B_FILE_NAME_LENGTH];
					attr_info		info;

					while (src_file.GetNextAttrName(name) == B_NO_ERROR)
					{
						src_file.GetAttrInfo(name, &info);
						off_t	chunk = min_c(info.size, buffer_len);

						while (chunk)
						{
							src_file.ReadAttr(name, info.type, 0, buffer, chunk);
							dst_file.WriteAttr(name, info.type, 0, buffer, chunk);
							if (info.size < chunk)
								chunk = info.size;
							info.size -= chunk;
						}
					}
				}

				if (read < 0)
					result = read;
				if (result < 0)
					send_error_message(path.Path(), result);
			}
			else
				send_error_message(path.Path(), result);
		}
		else if (result == B_FILE_EXISTS)
		{
			// what do we do?
		}
		if (result < B_NO_ERROR)
			send_error_message(path.Path(), result);
	}
	else
	{
		// symlink, what do we do?
	}
}


//--------------------------------------------------------------------

void delete_item(const char* u, bool is_directory)
{
	URL		url(u);

	if (cistrncmp("file", url.GetScheme(), 4) == 0)
	{
		size_t path_buf_length = strlen(url.GetPath()) + 1;
		char*	p = (char*)malloc(path_buf_length);

		url.GetUnescapedPath(p, path_buf_length);
		BEntry	entry(p);
		free(p);

		if (entry.InitCheck() == B_NO_ERROR)
		{
			status_t	result = B_NO_ERROR;

			if (is_directory)
			{
				BDirectory	dir(&entry);
				BEntry		item;

				while (dir.GetNextEntry(&item) == B_NO_ERROR)
				{
					BPath	path;
					BString	s;

					item.GetPath(&path);
					s << url.GetScheme() << "://" << path.Path();
					delete_item(s.String(), item.IsDirectory());
				}
			}

			if ((result = entry.Remove()) != B_NO_ERROR)
			{
				BPath		path(&entry);

				send_error_message(path.Path(), result);
			}
		}
	}
	else
	{
		if (is_directory)
		{
			BMessage	entries;

			if (list_entries(&url, &entries) == B_NO_ERROR)
			{
				int32	index = 0;

				while (entries.HasString("url", index))
				{
					delete_item(entries.FindString("url", index), 
								entries.FindBool("is_directory", index));
					index++;
				}
			}
			delete_item(u, false);	// this should delete the directory itself
		}
		else
		{
			size_t path_buf_length = strlen(url.GetPath()) + 1;
			char*			p = (char*)malloc(path_buf_length);
			BMessage		errors;
			BPath			path;
			BPath			par;
			FtpProtocol*	ftp = NULL;
			status_t		result;

			url.GetUnescapedPath(p, path_buf_length);
			path.SetTo(p);
			path.GetParent(&par);
			URL parent(url, par.Path());

			if ((ftp = dynamic_cast<FtpProtocol*>(Protocol::InstantiateProtocol(url.GetScheme()))) != NULL)
			{
				if ((result = ftp->OpenRW(parent, URL(), &errors, 0, true)) == B_NO_ERROR)
				{
					if ((result = ftp->Delete(path.Leaf())) != B_NO_ERROR)
						send_error_message(p, result);
				}
				else
					send_error_message(p, result);
				delete ftp;
			}
			free(p);
		}
	}
}


//--------------------------------------------------------------------

status_t list_entries(URL* url, BMessage* msg)
{
	FtpProtocol*		ftp_protocol;
	status_t			result = B_NO_ERROR;

	if ((ftp_protocol = dynamic_cast<FtpProtocol*>(Protocol::InstantiateProtocol(url->GetScheme()))) != NULL)
	{
		BMessage	errors;
		status_t	result;

		if ((result = ftp_protocol->OpenRW(*url, URL(), &errors, 0)) == B_NO_ERROR)
		{
			char*	buffer;
			int32	offset = 0;
			size_t	size = ftp_protocol->GetContentLength();
			ssize_t	read;

			while ((buffer = (char*)malloc(size + 1)) < 0)
				size /= 2;
			buffer[size] = 0;

			while ((read = ftp_protocol->Read(&buffer[offset], size - offset)) > 0)
				offset = parse_buffer(buffer, read, url, msg);
			free(buffer);
			result = B_NO_ERROR;
		}
		delete ftp_protocol;
	}
	else
		result = (status_t)ftp_protocol;
	return result;
}

//--------------------------------------------------------------------

int32 parse_buffer(char* buffer, ssize_t len, URL* url, BMessage* msg)
{
	char*			last_line = buffer;
	char*			line = buffer;
	StringBuffer	u;

	u << *url;

	while (1)
	{
		char		type;
		BString		name;
		BString		path;
		BString		tmp;

		last_line = line;

		if (!line[0])
			return 0;

		tmp = line;
		line += tmp.Length() + 1;
		if (line > (buffer + len))
			break;

		name = line;
		line += name.Length() + 1;
		if (line > (buffer + len))
			break;

		if ((line + sizeof(int32) + sizeof(tm) + sizeof(type)) > (buffer + len))
			break;

		line += sizeof(int32);
		line += sizeof(tm);
		type = *line;
		line++;

		path << u.String() << "/" << name;
		msg->AddString("url", path.String());
		msg->AddBool("is_directory", type == 'd');
	}
	memcpy(buffer, last_line, (buffer + len) - last_line);
	return((buffer + len) - last_line);
}

//====================================================================

void send_busy_status(const char* action, const char* path, bool state)
{
	BMessage	busy('fbBZ');

	busy.AddString("action", action);
	busy.AddString("volume", path);
	busy.AddBool("busy", state);
	be_app->PostMessage(&busy);
}


//--------------------------------------------------------------------

void send_error_message(const char* path, status_t status)
{
	BMessage	error('fbER');

	error.AddString("file", path);
	error.AddString("message", strerror(status));
	error.AddInt32("error code", status);
	be_app->PostMessage(&error);
}


//--------------------------------------------------------------------

void send_selection_count(int32 items)
{
	BMessage	count('fbSL');

	count.AddInt32("items", items);
	be_app->PostMessage(&count);
}
