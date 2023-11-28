/*-----------------------------------------------------------------*/
//
//	File:		FileListView.cpp
//
//	Written by:	Robert Polic
//
//	Copyright 2001, Be Incorporated
//
/*-----------------------------------------------------------------*/

#include <Application.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <FtpProtocol.h>
#include <InterfaceDefs.h>
#include <NodeInfo.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <ScrollBar.h>
#include <StringBuffer.h>
#include <Window.h>

#include <stdio.h>

#include "FileBrowserView.h"
#include "FileListView.h"

#define kTEXT_MARGIN	 8
#define kDRAG_SLOP		 4
#define kICON_WIDTH		20


//====================================================================

/* local */
static status_t		path_processor			(void*);

/* global */
int32				cistrncmp				(const char*, const char*, int32);

extern void			send_busy_status		(const char*, const char*, bool);
extern void			send_error_message		(const char*, status_t);
extern void			send_selection_count	(int32);


//====================================================================

FileListView::FileListView(BRect rect, drawing_parameters* parameters, BHandler* target)
	:BColumnListView(rect, "list_view", B_FOLLOW_ALL, B_WILL_DRAW, B_NO_BORDER, false),
	fQuit(false),
	fTarget(target),
	fEditor(NULL),
	fControls(NULL),
	fEditingItem(NULL),
	fParameters(parameters),
	fExtensionMap(NULL)
{
	float			width = rect.Width() - kICON_WIDTH - B_V_SCROLL_BAR_WIDTH;
	int32			min;
	BColumn*		column;

	min = max_c((int)(fParameters->list_font.StringWidth(B_UTF8_ELLIPSIS) + (2 * kTEXT_MARGIN)),
				(int)(fParameters->tab_font.StringWidth(B_UTF8_ELLIPSIS) + (2 * kTEXT_MARGIN))) - 2;
	SetSortingEnabled(true);
	AddColumn(column = new BBitmapColumn("", kICON_WIDTH, kICON_WIDTH, kICON_WIDTH, B_ALIGN_CENTER), eIconColumn);
	column->SetShowHeading(false);
	AddColumn(column = new BStringColumn("Name", (int32)(width * .3500), min, width, B_TRUNCATE_MIDDLE), eNameColumn);
	SetSortColumn(column, false, true);
	AddColumn(new BDateColumn("Modified", (int32)(width * .5000), min, width), eModifiedColumn);
	AddColumn(new SizeColumn("Size", (int32)(width * .1500), min, width), eSizeColumn);

	SetColumnFlags(B_ALLOW_COLUMN_RESIZE);

	SetFont(B_FONT_ROW, &fParameters->list_font);
	SetFont(B_FONT_HEADER, &fParameters->tab_font);

	fProcessingSem = create_sem(1, "file_processing");
	AddFilter(new FileListFilter(this));
}


//--------------------------------------------------------------------

FileListView::~FileListView()
{
	KillProcessingThread();
	delete_sem(fProcessingSem);
	delete fExtensionMap;
}


//--------------------------------------------------------------------

void FileListView::AllAttached()
{
	LoadExtensionMap();
	SetViewColor(B_TRANSPARENT_32_BIT);
	BColumnListView::AllAttached();

	SetTarget(this);
	SetSelectionMessage(new BMessage(eItemSelected));
	SetInvocationMessage(new BMessage(eOpenItems));
}


/*-----------------------------------------------------------------*/

bool FileListView::InitiateDrag(BPoint where, bool was_selected)
{
	if (was_selected)
	{
		BRect			bounds = ScrollView()->Bounds();
		float			max_y = 0;
		float			min_y = bounds.bottom;
		int32			items = 0;
		BMessage		drag_msg('DATA');
		BRect			item_rect;
		FileListItem*	item = NULL;

		// build list of selected items
		while ((item = dynamic_cast<FileListItem*>(CurrentSelection(item))))
		{
			BEntry		entry(item->Path());

			if (entry.InitCheck() == B_NO_ERROR)
			{
				entry_ref	ref;

				entry.GetRef(&ref);
				drag_msg.AddRef("refs", &ref);

				GetRowRect(item, &item_rect);
				if (item_rect.top < min_y)
					min_y = item_rect.top;
				if (item_rect.bottom > max_y)
					max_y = item_rect.bottom;

				items++;
			}
		}

		if (items)
		{
			float			icon_width;
			float			name_width;
			BColumn*		column;
			BRect			drag_rect;

			column = ColumnAt(eIconColumn);
			icon_width = column->Width();
			column = ColumnAt(eNameColumn);
			name_width = column->Width();
			drag_rect.Set(0, min_y, icon_width + name_width, max_y);

			// if items are visible, drag an image of them
			if ((min_y >= bounds.top) && (max_y <= bounds.bottom))
			{
				BBitmap*	drag_bits;
				BFont		font = fParameters->list_font;
				BView*		offscreen_view;
				font_height	finfo;

				font.GetHeight(&finfo);

				drag_rect.OffsetTo(0, 0);
				drag_bits = new BBitmap(drag_rect, B_RGBA32, true);
				offscreen_view = new BView(drag_rect, "", B_FOLLOW_NONE, 0);
				drag_bits->AddChild(offscreen_view);

				drag_bits->Lock();
				offscreen_view->SetHighColor(0, 0, 0, 0);
				offscreen_view->FillRect(drag_rect);
				offscreen_view->SetDrawingMode(B_OP_ALPHA);
				offscreen_view->SetHighColor(0, 0, 0, 192);
				offscreen_view->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
				offscreen_view->SetFont(&font);

				item = NULL;
				while ((item = dynamic_cast<FileListItem*>(CurrentSelection(item))))
				{
					const BBitmap*	icon = item->Bitmap();

					if (icon)
					{
						float	x;
						float	y;

						GetRowRect(item, &item_rect);
						x = (icon_width - icon->Bounds().Width()) / 2;
						y = item_rect.top - min_y + ((item->Height() - icon->Bounds().Height()) / 2);
						offscreen_view->DrawBitmap(icon, BPoint(x, y));

						y = item_rect.top - min_y +  + ((item_rect.Height() - (finfo.ascent + finfo.descent + finfo.leading)) / 2) +
										(finfo.ascent + finfo.descent) - 2;
						offscreen_view->MovePenTo(icon_width + 1 + kTEXT_MARGIN, y);
						offscreen_view->DrawString(item->Leaf());
					}
				}
				offscreen_view->Sync();
				drag_bits->Unlock();
				DragMessage(&drag_msg, drag_bits, B_OP_ALPHA,
							BPoint(where.x,
								   where.y - min_y));
			}
			// drag a rect instead
			else
			{
				drag_rect.top = max_c(drag_rect.top, bounds.top);
				drag_rect.bottom = min_c(drag_rect.bottom, bounds.bottom);
				DragMessage(&drag_msg, drag_rect);
			}
		}
		return true;
	}
	return false;
}


//--------------------------------------------------------------------

void FileListView::MessageDropped(BMessage* msg, BPoint point)
{
	if (msg->what == 'DATA')
	{
		size_t path_buf_length = strlen(fURL->GetPath()) + 1;
		char*			p = (char*)malloc(path_buf_length);
		const char*		target;
		int32			index = 0;
		BMessage		drop(eMessageDropped);
		FileListItem*	item = dynamic_cast<FileListItem*>(RowAt(point));;
		entry_ref		ref;

		fURL->GetUnescapedPath(p, path_buf_length);
		target = p;
		if (item)
		{
			BEntry	entry(item->Path());

			if (entry.IsDirectory())
				target = item->Path();
		}
		drop.AddString("volume", target);

		while (msg->FindRef("refs", index, &ref) == B_NO_ERROR)
		{
			BPath	path(&ref);

			drop.AddString("path", path.Path());
			index++;
		}
		if (index)
			Looper()->PostMessage(&drop, fTarget);
		free(p);
	}
}


//--------------------------------------------------------------------

void FileListView::MessageReceived(BMessage* msg)
{
	switch (msg->what)
	{
		case B_NODE_MONITOR:
			{
				int32			opcode = msg->FindInt32("opcode");
				FileListItem	*item;
				node_ref		n_ref;

				msg->FindInt32("device", &n_ref.device);
				msg->FindInt64("node", &n_ref.node);

				switch (opcode)
				{
					case B_ENTRY_CREATED:
						if (!(item = FindItem(n_ref)))
						{
							const char*	name;
							entry_ref	ref;
							BEntry		entry;

							msg->FindString("name", &name);
							msg->FindInt64("directory", &ref.directory);
							ref.device = n_ref.device;
							ref.set_name(name);
							entry.SetTo(&ref);
							if (entry.InitCheck() == B_NO_ERROR)
								AddItem(&entry);
						}
						break;

					case B_ENTRY_MOVED:
						if ((item = FindItem(n_ref)))
						{
							int64	from_dir;
							int64	to_dir;

							if (fEditingItem == item)
								RemoveEditor();

							msg->FindInt64("from directory", &from_dir);
							msg->FindInt64("to directory", &to_dir);
							if (from_dir == to_dir)	// rename
							{
								bool		select = ((FocusRow() == item));
								const char*	name;
								BString		mime(item->MimeType());
								BEntry		entry;
								BPath		path;
								entry_ref	ref;
								off_t		size = item->Size();
								time_t		modified = item->Modified();
								NODE_TYPE	type = item->NodeType();

								msg->FindString("name", &name);
								msg->FindInt64("to directory", &ref.directory);
								ref.device = n_ref.device;
								ref.set_name(name);
								entry.SetTo(&ref);

								RemoveRow(item);
								delete item;

								entry.GetPath(&path);
								item = AddItem(path.Leaf(), mime.String(), type, modified, size, n_ref);
								if (select)
								{
									BRect		bounds = ScrollView()->Bounds();
									BRect		r;

									GetRowRect(item, &r);
									if (!(bounds.Contains(r)))
									{
										if (r.top < bounds.top)
											ScrollView()->ScrollTo(bounds.left, r.top);
										else
											ScrollView()->ScrollTo(bounds.left, r.bottom - bounds.Height());
									}
									SetFocusRow(item, true);
								}
							}
							else	// moved out of current directory
							{
								RemoveRow(item);
								delete item;
							}
						}
						else	// moved into current directory
						{
							const char*	name;
							BEntry		entry;
							entry_ref	ref;

							msg->FindString("name", &name);
							msg->FindInt64("to directory", &ref.directory);
							ref.device = n_ref.device;
							ref.set_name(name);
							entry.SetTo(&ref);
							if (entry.InitCheck() == B_NO_ERROR)
								AddItem(&entry);
						}
						break;

					case B_ENTRY_REMOVED:
						if ((item = FindItem(n_ref)))
						{
							if (fEditingItem == item)
								RemoveEditor();
							RemoveRow(item);
							delete item;
						}
						break;
				}
			}
			break;

		case kEDIT_COMPLETE:
			{
				bool	result;

				if ((msg->FindBool("accept", &result) == B_NO_ERROR) &&
					(result))
				{
					FileListItem*	item;
					if ((item = dynamic_cast<FileListItem*>(FocusRow())))
						Rename(item, fEditor->Text());
				}

				RemoveEditor();
			}
			break;

		case eItemSelected:
			{
				int32			count = 0;
				FileListItem*	item = NULL;

				while ((item = dynamic_cast<FileListItem*>(CurrentSelection(item))))
					count++;
				send_selection_count(count);
			}
			break;

		case eOpenItems:
			Looper()->PostMessage(eOpenItems, fTarget);
			break;

		case eOpenParent:
			Looper()->PostMessage(eOpenParent, fTarget);
			break;

		case eSelectAll:
			{
				int32			index = 0;
				FileListItem*	item;

				while ((item = dynamic_cast<FileListItem*>(RowAt(index++))))
					AddToSelection(item);
				send_selection_count(index - 1);
			}
			break;

		default:
			BColumnListView::MessageReceived(msg);
	}
}


//--------------------------------------------------------------------

void FileListView::Edit()
{
	BRect			r;
	BView*			view = ScrollView();
	FileListItem*	item;

	if (fEditor)
		RemoveEditor();
	else if (((item = dynamic_cast<FileListItem*>(FocusRow()))) &&
		(GetRowRect(item, &r)))
	{
		int32		cancel_width = 0;
		int32		ok_width = 0;
		int32		tmp;
		BColumn*	column;
		BRect		bounds = view->Bounds();

		if (!(bounds.Contains(r)))
		{
			if (r.top < bounds.top)
				view->ScrollTo(bounds.left, r.top);
			else
				view->ScrollTo(bounds.left, r.bottom - bounds.Height());
		}
		SetEditMode(true);
		view->Invalidate(r);

		if (fParameters->icons[eEditAcceptUpIcon])
			ok_width = (int32)(fParameters->icons[eEditAcceptUpIcon]->Bounds().Width());
		if (fParameters->icons[eEditRejectUpIcon])
			cancel_width = (int32)(fParameters->icons[eEditRejectUpIcon]->Bounds().Width());
		tmp = 8 + ok_width + 8 + cancel_width + 8;

		column = ColumnAt(eIconColumn);
		r.left = column->Width() + 3;
		r.right = ScrollView()->Bounds().right - tmp;
		view->AddChild(fEditor = new Editor(r, item->Leaf(), this, fParameters));

		r.left = r.right;
		r.right = ScrollView()->Bounds().right;
		r.bottom--;
		view->AddChild(fControls = new EditControls(r, this, fParameters));

		fEditingItem = item;
		fEditor->MakeFocus(true);
	}
}


//--------------------------------------------------------------------

void FileListView::MakeDirectory(const char* dir_name)
{
	size_t path_buf_length = strlen(fURL->GetPath()) + 1;
	char*			p = (char*)malloc(path_buf_length);
	int32			index = 1;
	BString			name(dir_name);
	FileListItem*	item = NULL;

	fURL->GetUnescapedPath(p, path_buf_length);
	if (cistrncmp("file", fURL->GetScheme(), 4) == 0)
	{
		BDirectory	dir;
		BDirectory	parent(p);
		status_t	result;

		while ((result = parent.CreateDirectory(name.String(), &dir)) == B_FILE_EXISTS)
		{
			name = "";
			name << dir_name << " " << ++index;
		}
		if (result == B_NO_ERROR)
		{
			BEntry	entry;

			dir.GetEntry(&entry);
			item = AddItem(&entry);
		}
		else
		{
			BString	path;

			path << p << "/" << name;
			send_error_message(path.String(), result);
		}
	}
	else
	{
		FtpProtocol*		ftp_protocol;

		if ((ftp_protocol = dynamic_cast<FtpProtocol*>(Protocol::InstantiateProtocol(fURL->GetScheme()))) != NULL)
		{
			BMessage	errors;
			status_t	result;

			if ((result = ftp_protocol->OpenRW(*fURL, URL(), &errors, 0)) == B_NO_ERROR)
			{
				while ((result = ftp_protocol->MakeDirectory(name.String())) == B_FILE_EXISTS)
				{
					name = "";
					name << dir_name << " " << ++index;
				}
				if (result == B_NO_ERROR)
				{
					node_ref	n_ref;

					item = AddItem(name.String(), "application/x-vnd.Be-directory", eDirectory,
						time(NULL), 0, n_ref);
				}
				else
				{
					BString	path;

					path << p << "/" << name;
					send_error_message(path.String(), result);
				}
			}
			else
				send_error_message(p, result);
			delete ftp_protocol;
		}
		else
			send_error_message(p, (status_t)ftp_protocol);
	}
	free(p);

	if (item)
	{
		DeselectAll();
		AddToSelection(item);
		SetFocusRow(item);
		Edit();
	}
}


//--------------------------------------------------------------------

status_t FileListView::PathProcessor()
{
	size_t path_buf_length = strlen(fURL->GetPath()) + 1;
	char*	p = (char*)malloc(path_buf_length);

	fURL->GetUnescapedPath(p, path_buf_length);
	send_busy_status("load path", p, true);
	if (cistrncmp("file", fURL->GetScheme(), 4) == 0)
	{
		BDirectory	dir(p);
		node_ref	n_ref;

		if (dir.InitCheck() == B_NO_ERROR)
		{
			BEntry		entry;

			while ((fQuit == false) && (dir.GetNextEntry(&entry) == B_NO_ERROR))
			{
				AddItem(&entry);
				snooze(1000);
			}
		}
		dir.GetNodeRef(&n_ref);
		watch_node(&n_ref, B_WATCH_DIRECTORY, this, Looper());
	}
	else
	{
		FtpProtocol*		ftp_protocol;

		if ((ftp_protocol = dynamic_cast<FtpProtocol*>(Protocol::InstantiateProtocol(fURL->GetScheme()))) != NULL)
		{
			BMessage	errors;
			status_t	result;

			if ((result = ftp_protocol->OpenRW(*fURL, URL(), &errors, 0)) == B_NO_ERROR)
			{
				char*	buffer;
				int32	offset = 0;
				size_t	size = ftp_protocol->GetContentLength();
				ssize_t	read;

				while ((buffer = (char*)malloc(size + 1)) < 0)
					size /= 2;
				buffer[size] = 0;

				while ((read = ftp_protocol->Read(&buffer[offset], size - offset)) > 0)
					offset = ParseFTPBuffer(buffer, read);
				free(buffer);
			}
			else
				send_error_message(p, result);
			delete ftp_protocol;
		}
		else
			send_error_message(p, (status_t)ftp_protocol);
	}

	if (!fQuit)
	{
		status_t	lock;

		lock = Window()->LockWithTimeout(1000000);
		if (lock != B_TIMED_OUT)
		{
			ScrollView()->ScrollTo(fPosition);
			if (RowAt(fSelectedItem))
			{
				AddToSelection(RowAt(fSelectedItem));
				SetFocusRow(fSelectedItem);
			}
			else
				send_selection_count(0);
		}
		else
			send_selection_count(0);
		if (lock == B_OK)
			Window()->Unlock();
	}

	release_sem(fProcessingSem);
	send_busy_status("load path", p, false);
	free(p);
	return B_NO_ERROR;
}


//--------------------------------------------------------------------

void FileListView::SetURL(URL* url, int32 selected_item, BPoint position)
{
	fSelectedItem = selected_item;
	fPosition = position;
	KillProcessingThread();
	fURL = url;
	acquire_sem(fProcessingSem);
	resume_thread(spawn_thread((status_t (*)(void *))path_processor,
				  "path_processor", B_DISPLAY_PRIORITY, this));
}


//--------------------------------------------------------------------

FileListItem* FileListView::AddItem(const char* leaf, const char* mime, NODE_TYPE type,
									time_t time, off_t size, node_ref node)
{
	int32			count = 10;
	FileListItem*	item = NULL;
	status_t		lock = B_TIMED_OUT;

	while ((!fQuit) && (lock == B_TIMED_OUT) && (count))
	{
		lock = Window()->LockWithTimeout(50000);
		count--;
	}
	if (lock != B_TIMED_OUT)
		AddRow(item = new FileListItem(fURL, leaf, mime, type, time, size, node, fParameters));
	if (lock == B_OK)
		Window()->Unlock();
	return item;
}


//--------------------------------------------------------------------

FileListItem* FileListView::AddItem(BEntry* entry)
{
	BNode			node(entry);
	BNodeInfo		node_info(&node);
	BPath			path;
	BString			file_path;
	FileListItem*	item = NULL;
	node_ref		n_ref;
	off_t			size;
	time_t			time;

	entry->GetPath(&path);
	node.GetModificationTime(&time);
	node.GetSize(&size);
	if (entry->IsSymLink())
	{
		entry_ref	ref;

		entry->GetRef(&ref);
		entry->SetTo(&ref, true);
	}

	entry->GetNodeRef(&n_ref);
	if (entry->IsDirectory())
		item = AddItem(path.Leaf(), "application/x-vnd.Be-directory", eDirectory, time, size, n_ref);
	else if (entry->IsFile())
	{
		char		mime[B_FILE_NAME_LENGTH] = "";
		char		name[B_FILE_NAME_LENGTH];

		entry->GetName(name);
		if ((node_info.GetType(mime) != B_NO_ERROR) || (strlen(mime) == 0))
		{
			char		name[B_FILE_NAME_LENGTH];

			SniffFile(name, mime);
		}
		item = AddItem(path.Leaf(), mime, eFile, time, size, n_ref);
	}
	return item;
}


//--------------------------------------------------------------------

FileListItem* FileListView::FindItem(node_ref ref)
{
	int32			index = 0;
	FileListItem*	item;

	while ((item = dynamic_cast<FileListItem*>(RowAt(index++))) &&
		   (item->NodeRef() != ref));
	return item;
}


//--------------------------------------------------------------------

void FileListView::KillProcessingThread()
{
	FileListItem*	item;

	stop_watching(this, Looper());
	RemoveEditor();

	fQuit = true;
	while (acquire_sem_etc(fProcessingSem, 1, B_RELATIVE_TIMEOUT, 0) != B_NO_ERROR)
		snooze(10000);
	release_sem(fProcessingSem);

	while ((item = dynamic_cast<FileListItem*>(RowAt(0))) != NULL)
	{
		RemoveRow(item);
		delete(item);
	}
	ScrollView()->ScrollTo(0, 0);
	fQuit = false;
}


//--------------------------------------------------------------------

void FileListView::LoadExtensionMap()
{
	BPath		path;

	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, true) == B_NO_ERROR)
	{
		BDirectory	dir(path.Path());
		BEntry		map;

		if (dir.FindEntry(kEXTENSION_MAP_NAME, &map) == B_NO_ERROR)
		{
			BFile	file(&map, O_RDONLY);

			if (file.InitCheck() == B_NO_ERROR)
			{
				bool	extension = true;
				char*	contents;
				char*	extension_index = NULL;
				char*	index;
				int32	extension_length = 0;
				int32	mime_length = 0;
				off_t	size;

				file.GetSize(&size);
				contents = (char*)malloc(size);
				file.Read(contents, size);
				index = contents;
				while (size)
				{
					extension_map*	map;

					if (*index == '\t')
					{
						extension_index = index - extension_length;
						*index = 0;
						mime_length = 0;
						extension = false;
					}
					else if (*index == '\n')
					{
						*index = 0;

						map = new extension_map(extension_index, index - mime_length);
						map->next = fExtensionMap;
						fExtensionMap = map;

						extension_length = 0;
						extension = true;
					}
					else if (extension)
						extension_length++;
					else
						mime_length++;
					size--;
					index++;
				}
				free(contents);
			}
		}
	}

	/* if no map, something failed above, build a default map */
	if (!fExtensionMap)
	{
		char*	default_map[] =
				{
					".aifc",	"audio/x-aifc",
					".aiff",	"audio/aiff",
					".mp2",		"audio/x-mpeg",
					".mp3",		"audio/x-mpeg",
					".ra",		"audio/vnd.rn-realaudio",
					".rpm",		"audio/x-pn-realaudio-plugin",
					".ram",		"audio/x-pn-realaudio",
					".rm",		"application/vnd.rn-realmedia",
					".rn",		"application/vnd.rn-realmedia",
					".riff",	"audio/riff",
					".au",		"audio/basic",
					".swf",		"application/x-shockwave-flash",
				//	".wav",		"audio/wav",

					".rv",		"video/vnd.rn-realvideo",
				//	".mpg",		"video/mpeg",
				//	".mpe",		"video/mpeg",
				//	".avi",		"video/x-msvideo",
				//	".qt",		"video/quicktime",
				//	".mov",		"video/quicktime",
				//	".viv",		"video/vnd.vivo",
				//	".vivo",	"video/vnd.vivo",

					".gif",		"image/gif",
					".jpeg",	"image/jgep",
					".jpg",		"image/jpeg",
					".png",		"image/png",
				//	".bmp".		"image/bmp",

					".txt",		"text/plain",
					".html",	"text/html",
					".htm",		"text/html",
				//	".pdf",		"application/pdf",

					"",			""
				};
		int32 	index = 0;

		while (strlen(default_map[index]))
		{
			extension_map*	map;

			map = new extension_map(default_map[index], default_map[index + 1]);
			map->next = fExtensionMap;
			fExtensionMap = map;
			index += 2;
		}
	}
}


//--------------------------------------------------------------------

int32 FileListView::ParseFTPBuffer(char* buffer, ssize_t len)
{
	char*	last_line = buffer;
	char*	line = buffer;

	while (!fQuit)
	{
		char		mime[B_FILE_NAME_LENGTH];
		char		type;
		BString		name;
		BString		tmp;
		int32		size;
		node_ref	n_ref;
		tm			tm_struct;
		time_t		time;

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

		if ((line + sizeof(size) + sizeof(tm_struct) + sizeof(type)) > (buffer + len))
			break;

		memcpy(&size, line, sizeof(int32));
		line += sizeof(int32);
		memcpy(&tm_struct, line, sizeof(tm_struct));
		time = mktime(&tm_struct);
		line += sizeof(tm_struct);
		type = *line;
		line++;

		switch (type)
		{
			case 'l':	// link - ignore
				SniffFile(name.String(), mime);
				AddItem(name.String(), mime, eSymLink, time, size, n_ref);
				break;

			case 'd':	// directory
				AddItem(name.String(), "application/x-vnd.Be-directory", eDirectory,
						time, size, n_ref);
				break;

			case 'f':	// file
				SniffFile(name.String(), mime);
				AddItem(name.String(), mime, eFile, time, size, n_ref);
				break;
		}
		snooze(1000);
	}
	memcpy(buffer, last_line, (buffer + len) - last_line);
	return((buffer + len) - last_line);
}


//--------------------------------------------------------------------

void FileListView::RemoveEditor()
{
	if (fEditor)
	{
		BView*			view = ScrollView();

		view->RemoveChild(fEditor);
		delete fEditor;
		fEditor = NULL;

		view->RemoveChild(fControls);
		delete fControls;
		fControls = NULL;

		fEditingItem = NULL;
		SetEditMode(false);
		view->Invalidate(view->Bounds());
		MakeFocus(true);
	}
}


//--------------------------------------------------------------------

void FileListView::Rename(FileListItem* item, const char* new_name)
{
	if ((strlen(new_name)) &&
		(strcmp(item->Leaf(), new_name) != 0))
	{
		if (cistrncmp("file", fURL->GetScheme(), 4) == 0)
		{
			BEntry		entry(item->Path());
			status_t	result;

			if ((result = entry.Rename(fEditor->Text())) != B_NO_ERROR)
				send_error_message(item->Path(), result);
		}
		else
		{
			FtpProtocol*	ftp;

			if ((ftp = dynamic_cast<FtpProtocol*>(Protocol::InstantiateProtocol(fURL->GetScheme()))) != NULL)
			{
				BMessage	errors;
				status_t	result;

				if ((result = ftp->OpenRW(*fURL, URL(), &errors, 0, true)) == B_NO_ERROR)
				{
					if ((result = ftp->Rename(item->Leaf(), new_name)) == B_NO_ERROR)
					{
						bool			select = ((FocusRow() == item));
						BString			mime(item->MimeType());
						off_t			size = item->Size();
						time_t			modified = item->Modified();
						NODE_TYPE		type = item->NodeType();
						FileListItem*	new_item;
						node_ref		n_ref;

						RemoveRow(item);
						delete item;

						new_item = AddItem(new_name, mime.String(), type, modified, size, n_ref);
						if (select)
						{
							BRect		bounds = ScrollView()->Bounds();
							BRect		r;

							GetRowRect(new_item, &r);
							if (!(bounds.Contains(r)))
							{
								if (r.top < bounds.top)
									ScrollView()->ScrollTo(bounds.left, r.top);
								else
									ScrollView()->ScrollTo(bounds.left, r.bottom - bounds.Height());
							}
							SetFocusRow(new_item, true);
						}
					}
					else
						send_error_message(item->Path(), result);
				}
				delete ftp;
			}
			else
				send_error_message(item->Path(), (status_t)ftp);
		}
	}
}


//--------------------------------------------------------------------

void FileListView::SniffFile(const char* name, char* mime)
{
	extension_map*	map = fExtensionMap;

	mime[0] = 0;
	while (map)
	{
		uint32	len = strlen(map->extension);

		if (strlen(name) > len)
		{
			if (cistrncmp(map->extension, &name[strlen(name) - len], len) == 0)
			{
				strcpy(mime, map->mime_type);
				break;
			}
		}
		map = map->next;
	}
}


//====================================================================

status_t path_processor(void* data)
{
	return (((FileListView*)data)->PathProcessor());
}


//========================================================================

FileListFilter::FileListFilter(BHandler* target)
	:BMessageFilter(B_KEY_DOWN),
	fTarget(target)
{
}


/*-----------------------------------------------------------------*/

filter_result FileListFilter::Filter(BMessage* msg, BHandler** /* target */)
{
	uint32			modifiers;
	filter_result	result = B_DISPATCH_MESSAGE;

	if ((msg->FindInt32("modifiers", (int32*)&modifiers) == B_NO_ERROR) &&
		(modifiers & B_COMMAND_KEY))
	{
		uchar 	key = 0;

		if (msg->FindInt8("byte", (int8*)&key) == B_NO_ERROR)
		{
			BMessage	m;

			switch (key)
			{
				case B_UP_ARROW:
				case B_LEFT_ARROW:
					Looper()->PostMessage(eOpenParent, fTarget);
					result = B_SKIP_MESSAGE;
					break;

				case B_DOWN_ARROW:
				case B_RIGHT_ARROW:
					Looper()->PostMessage(eOpenItems, fTarget);
					result = B_SKIP_MESSAGE;
					break;

				case 'A':
				case 'a':
					Looper()->PostMessage(eSelectAll, fTarget);
					result = B_SKIP_MESSAGE;
					break;
			}
		}
	}
	return result;
}


//====================================================================

FileListItem::FileListItem(URL* url, const char* leaf, const char* mime, NODE_TYPE type,
						   time_t modified, off_t size, node_ref ref, drawing_parameters* parameters)
	:BRow(parameters->offsets[eListItemHeight]),
	fNodeRef(ref),
	fSize(size),
	fModified(modified),
	fLeaf(leaf),
	fMime(mime),
	fNodeType(type)
{
	size_t path_buf_length = strlen(fURL.GetPath()) + 1;
	char*	p = (char*)malloc(path_buf_length);
	int32	icon = eOtherIcon;

	url->GetUnescapedPath(p, path_buf_length);
	fPath.SetTo(p);
 	fPath << "/" << fLeaf;
	fURL.SetTo(*url, fPath.String());

	// determine icon type
	if (cistrncmp(mime, "audio/", 6) == 0)
		icon = eAudioIcon;
	else if (cistrncmp(mime, "video/", 6) == 0)
		icon = eVideoIcon;
	else if (cistrncmp(mime, "image/", 6) == 0)
		icon = eImageIcon;
	else if (cistrncmp(mime, "text/", 5) == 0)
		icon = eTextIcon;
	else if (cistrncmp(mime, "application/x-vnd.Be-directory", 30) == 0)
		icon = eFolderClosedIcon;
	SetField(fBitmap = new BBitmapField(parameters->icons[icon]), eIconColumn);

	SetField(new BStringField(fLeaf.String()), eNameColumn);

	SetField(new BDateField(&modified), eModifiedColumn);

	SetField(new SizeField(size, (fNodeType == eFile)), eSizeColumn);
}


/*-----------------------------------------------------------------*/

char* FileListItem::GetURL()
{
	char*			result;
	StringBuffer	url;

	url << fURL;
	result = (char*)malloc(strlen(url.String()) + 1);
	strcpy(result, url.String());
	return result;
}


//====================================================================

SizeColumn::SizeColumn(const char* title, float width, float min_width,
					   float max_width, alignment align)
	:BSizeColumn(title, width, min_width, max_width, align)
{
}


//--------------------------------------------------------------------

void SizeColumn::DrawField(BField* field, BRect r, BView* view)
{
	if (((SizeField*)field)->File())
		BSizeColumn::DrawField(field, r, view);
	else
		DrawString("--", view, r);
}


//====================================================================

SizeField::SizeField(uint32 size, bool file)
	:BSizeField(size),
	fFile(file)
{
}


//====================================================================

int32 cistrncmp(const char* str1, const char* str2, int32 max)
{
	char		c1;
	char		c2;
	int32		loop;

	for (loop = 0; loop < max; loop++)
	{
		c1 = *str1++;
		if ((c1 >= 'A') && (c1 <= 'Z'))
			c1 += ('a' - 'A');
		c2 = *str2++;
		if ((c2 >= 'A') && (c2 <= 'Z'))
			c2 += ('a' - 'A');
		if (c1 < c2)
			return -1;
		else if ((c1 > c2) || (!c2))
			return 1;
	}
	return 0;
}
