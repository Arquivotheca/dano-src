/*-----------------------------------------------------------------*/
//
//	File:		FileBrowserView.cpp
//
//	Written by:	Robert Polic
//
//	Copyright 2000, Be Incorporated
//
/*-----------------------------------------------------------------*/

#include <Application.h>
#include <Debug.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <fs_info.h>
#include <Node.h>
#include <NodeInfo.h>
#include <URL.h>
#include <Window.h>
#include "FileBrowserView.h"


//========================================================================

/* local */
static status_t		volume_processor		(void*);
static status_t		ftp_processor			(void*);
static void			process_directory		(BDirectory*, FileBrowserView*, fs_info*);
static void			process_ftp				(URL, FileBrowserView*);
static int32		parse_ftp				(URL, char*, ssize_t, FileBrowserView*);
static file_type	get_type				(const char*);
static void			install_file			(FileBrowserView*, const char*, BString*,
											 BString*, time_t, off_t, PROTOCOL);
static void			sniff_file				(extension_map*, const char*, char*);
static bool			is_trash				(fs_info*, BPath*);

/* global */
int32				cistrncmp				(const char*, const char*, int32);
void				send_busy				(bool, const char*, const char*);


//========================================================================

FileBrowserView::FileBrowserView(BRect rect, drawing_parameters* parameters,
								 BString* default_volume, BString* default_view)
 : BView			(rect, "*FileBrowserView", B_FOLLOW_ALL, 0),
   fParameters		(parameters),
   fQuit			(false),
   fDefaultVolume	(*default_volume),
   fDefaultView		(*default_view),
   fExtensionMap	(NULL),
   fCurrentView		(NULL),
   fFtpProtocol		(NULL)
{
	fProcessingSem = create_sem(1, "file_processing");
	Show();
}


/*-----------------------------------------------------------------*/

FileBrowserView::~FileBrowserView()
{
	KillProcessingThread();
	delete_sem(fProcessingSem);

	if (fCurrentView)
		RemoveChild(fCurrentView);
	delete fMediaFiles;
	delete fImageFiles;
	delete fTextFiles;
	delete fOtherFiles;
	delete fExtensionMap;
}


/*-----------------------------------------------------------------*/

void FileBrowserView::AllAttached()
{
	BRect	r = Bounds();

	LoadExtensionMap();

	fMediaFiles = new MediaFilesView(r, fParameters);
	fImageFiles = new ImageTextFilesView(r, fParameters, eImageView);
	fTextFiles = new ImageTextFilesView(r, fParameters, eTextView);
	fOtherFiles = new ImageTextFilesView(r, fParameters, eOtherView);

	if (fDefaultView.Length())
	{
		if (ICompare(fDefaultView, "media") == 0)
			fCurrentView = fMediaFiles;
		else if (ICompare(fDefaultView, "image") == 0)
			fCurrentView = fImageFiles;
		else if (ICompare(fDefaultView, "text") == 0)
			fCurrentView = fTextFiles;
		else
			fCurrentView = fOtherFiles;
	}
	else
		fCurrentView = fMediaFiles;
	AddChild(fCurrentView);

	if (fDefaultVolume.Length())
	{
		BMessage	msg('fbSV');

		msg.AddString("mounted_at", fDefaultVolume.String());
		MessageReceived(&msg);
	}

	fCurrentView->List()->MakeFocus(true);
}


/*-----------------------------------------------------------------*/

void FileBrowserView::MessageReceived(BMessage* msg)
{
	FilesView*	view = NULL;

	switch (msg->what)
	{
		case 'fbSV':	/* show volume */
			{
				const char*		mounted_at;

				if ((msg->FindString("mounted_at", &mounted_at) == B_NO_ERROR) &&
					(strcmp(mounted_at, MountedAt()) != 0))
				{
					KillProcessingThread();

					fMediaFiles->MakeEmpty();
					fImageFiles->MakeEmpty();
					fTextFiles->MakeEmpty();
					fOtherFiles->MakeEmpty();

					fDomain = "";
					fUser = "";
					fPassword = "";
					fPath = "";

					fMountedAt = mounted_at;
					fCurrentView->List()->MakeFocus(true);

					acquire_sem(fProcessingSem);
					resume_thread(spawn_thread((status_t (*)(void *))volume_processor,
							  "volume_processor", B_DISPLAY_PRIORITY, this));
				}
			}
			break;

		case 'fbSI':	/* show I-Drive */
			{
				const char*		domain = NULL;
				const char*		user = NULL;
				const char*		password = NULL;
				const char*		path = NULL;

				msg->FindString("domain", &domain);
				msg->FindString("user", &user);
				msg->FindString("password", &password);
				msg->FindString("path", &path);

				if (domain)
				{
					BString		url("ftp://");

					KillProcessingThread();

					fMediaFiles->MakeEmpty();
					fImageFiles->MakeEmpty();
					fTextFiles->MakeEmpty();
					fOtherFiles->MakeEmpty();
					fMountedAt = "";
					fCurrentView->List()->MakeFocus(true);

					fDomain = domain;
					(user) ? fUser = user : fUser = "";
					(password) ? fPassword = password : fPassword = "";
					(path) ? fPath = path : fPath = "";

					if ((strlen(user)) && (strlen(password)))
						url << fUser << ":" << fPassword << "@" << fDomain << fPath;
					else if (strlen(user))
						url << fUser << "@" << fDomain << fPath;
					else
						url << fDomain << fPath;

					fURL = url.String();

					if ((fFtpProtocol = dynamic_cast<FtpProtocol*>(Protocol::InstantiateProtocol(fURL.GetScheme()))) != NULL)
					{
						acquire_sem(fProcessingSem);
						resume_thread(spawn_thread((status_t (*)(void *))ftp_processor,
								  "ftp_processor", B_DISPLAY_PRIORITY, this));
					}
					else
					{
						BMessage	error('fbER');

						error.AddString("domain", fDomain.String());
						error.AddString("message", strerror((status_t)fFtpProtocol));
						error.AddInt32("error code", (status_t)fFtpProtocol);
						be_app->PostMessage(&error);
						fFtpProtocol = NULL;
					}
				}
			}
			break;

		case 'fbRV':	/* remove currently displayed volume */
			KillProcessingThread();

			fMediaFiles->MakeEmpty();
			fImageFiles->MakeEmpty();
			fTextFiles->MakeEmpty();
			fOtherFiles->MakeEmpty();

			fMountedAt = "";
			fDomain = "";
			fUser = "";
			fPassword = "";
			fPath = "";
			break;

		case 'fbDM':	/* display media */
			if (fCurrentView != fMediaFiles)
				view = fMediaFiles;
			break;

		case 'fbDI':	/* display images */
			if (fCurrentView != fImageFiles)
				view = fImageFiles;
			break;

		case 'fbDT':	/* display text */
			if (fCurrentView != fTextFiles)
				view = fTextFiles;
			break;

		case 'fbDO':	/* display other */
			if (fCurrentView != fOtherFiles)
				view = fOtherFiles;
			break;

		case 'fbCP':	/* copy selection to destination */
			if (fCurrentView)
			{
				const char*		target = NULL;
				BString			url("ftp://");

				if (msg->HasString("mounted_at"))
					msg->FindString("mounted_at", &target);
				else if (msg->HasString("domain"))
				{
					const char*		domain = NULL;
					const char*		user = NULL;
					const char*		password = NULL;
					const char*		path = NULL;

					msg->FindString("domain", &domain);
					msg->FindString("user", &user);
					msg->FindString("password", &password);
					msg->FindString("path", &path);

					if ((strlen(user)) && (strlen(password)))
						url << user << ":" << password << "@" << domain << path;
					else if (strlen(user))
						url << user << "@" << domain << path;
					else
						url << domain << path;
					target = url.String();
				}
				if (target)
					fCurrentView->CopyFiles(target);
			}
			break;

		case 'fbDL':	/* delete selected files */
			if (fCurrentView)
				fCurrentView->DeleteFiles();
			break;

		case 'fbOP':	/* open selected files */
			if (fCurrentView)
				fCurrentView->OpenFiles();
			break;

		case 'fbED':	/* edit selected file */
			if (fCurrentView)
				fCurrentView->EditFiles();
			break;

		case 'fbAD':	/* file added */
			{
				if (fCurrentView)
				{
					const char*	name;

					if (msg->FindString("path", &name) == B_NO_ERROR)
					{
						if (strncmp(MountedAt(), name, strlen(MountedAt())) == 0)
						{
							BPath	path(name);

							if (path.InitCheck() == B_NO_ERROR)
							{
								char		mime[B_FILE_NAME_LENGTH];
								BEntry		entry(name);
								BNode		node(&entry);
								BNodeInfo	node_info(&node);
								BString		file_name;
								BString		file_path;
								time_t		time;
								off_t		size;

								mime[0] = 0;
								if ((node_info.GetType(mime) != B_NO_ERROR) || (strlen(mime) == 0))
									sniff_file(ExtensionMap(), path.Leaf(), mime);

								node.GetModificationTime(&time);
								node.GetSize(&size);
								file_name = path.Leaf();
								file_path = name;
								install_file(this, mime, &file_path, &file_name, time, size, eLocal);
							}
						}
					}
				}
			}
			break;

		case 'fbFL':	/* return list of selected files */
			if (fCurrentView)
			{
				BMessage	reply;

				fCurrentView->GetFiles(&reply);
				msg->SendReply(&reply);
			}
			break;

		case 'fbEM':	/* edit mode? */
			if (fCurrentView)
			{
				BMessage	reply;

				reply.AddString("edit_mode", (fCurrentView->Editing()) ? "true" : "false");
				msg->SendReply(&reply);
			}
			break;

		default:
			BView::MessageReceived(msg);
	}

	if (view)
	{
		RemoveChild(fCurrentView);
		fCurrentView = view;
		AddChild(fCurrentView);
		fCurrentView->List()->MakeFocus(true);
	}
}


/*-----------------------------------------------------------------*/

void FileBrowserView::KillProcessingThread()
{
	fQuit = true;
	while (acquire_sem_etc(fProcessingSem, 1, B_RELATIVE_TIMEOUT, 0) != B_NO_ERROR)
		snooze(10000);
	delete fFtpProtocol;
	fFtpProtocol = NULL;
	release_sem(fProcessingSem);
	fQuit = false;
}


/*-----------------------------------------------------------------*/

void FileBrowserView::LoadExtensionMap()
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
		char*	default_map[] = {".aifc",	"audio/x-aifc",
								 ".aiff",	"audio/aiff",
								 ".mp2",	"audio/x-mpeg",
								 ".mp3",	"audio/x-mpeg",
								 ".ra",		"audio/vnd.rn-realaudio",
								 ".rpm",	"audio/x-pn-realaudio-plugin",
								 ".ram",	"audio/x-pn-realaudio",
								 ".rm",		"application/vnd.rn-realmedia",
								 ".rn",		"application/vnd.rn-realmedia",
								 ".riff",	"audio/riff",
								 ".au",		"audio/basic",
								 ".swf",	"application/x-shockwave-flash",
							//	 ".wav",	"audio/wav",

								 ".rv",		"video/vnd.rn-realvideo",
							//	 ".mpg",	"video/mpeg",
							//	 ".mpe",	"video/mpeg",
							//	 ".avi",	"video/x-msvideo",
							//	 ".qt",		"video/quicktime",
							//	 ".mov",	"video/quicktime",
							//	 ".viv",	"video/vnd.vivo",
							//	 ".vivo",	"video/vnd.vivo",

								 ".gif",	"image/gif",
								 ".jpeg",	"image/jgep",
								 ".jpg",	"image/jpeg",
								 ".png",	"image/png",
							//	 ".bmp".	"image/bmp",

								 ".txt",	"text/plain",
								 ".html",	"text/html",
								 ".htm",	"text/html",
								 ".pdf",	"application/pdf",

								 ".eml",	"message/rfc822",

								 "",		""};
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


/*=================================================================*/

status_t volume_processor(void* data)
{
	FileBrowserView*	view = (FileBrowserView*)data;
	BDirectory			dir(view->MountedAt());
	fs_info				info;
	node_ref			node;

	send_busy(true, "load volume", view->MountedAt());

	dir.GetNodeRef(&node);
	fs_stat_dev(node.device, &info);
	process_directory(&dir, view, &info);
	release_sem(view->ProcessingSem());

	send_busy(false, "load volume", view->MountedAt());
	return B_NO_ERROR;
}


/*=================================================================*/

status_t ftp_processor(void* data)
{
	FileBrowserView*	view = (FileBrowserView*)data;

	send_busy(true, "load i-drive", "");
	process_ftp(view->GetURL(), view);
	release_sem(view->ProcessingSem());
	send_busy(false, "load i-drive", "");
	return B_NO_ERROR;
}


/*-----------------------------------------------------------------*/

void process_directory(BDirectory* dir, FileBrowserView* view, fs_info* info)
{
	if (dir->InitCheck() == B_NO_ERROR)
	{
		BEntry		entry;

		while ((view->Quit() == false) && (dir->GetNextEntry(&entry) == B_NO_ERROR))
		{
			BPath		path;

			entry.GetPath(&path);
			if (entry.IsDirectory())
			{
				if ((view->fParameters->flags[eShowTrashedFiles] == false) &&
					(is_trash(info, &path)))
				{
				}
				else
				{
					BDirectory	new_dir(&entry);

					process_directory(&new_dir, view, info);
				}
			}
			else if (entry.IsFile())
			{
				char		name[B_FILE_NAME_LENGTH];
				char		mime[B_FILE_NAME_LENGTH];
				BNode		node(&entry);
				BNodeInfo	node_info(&node);
				BString		file_name;
				BString		file_path;
				time_t		time;
				off_t		size;

				entry.GetName(name);
				mime[0] = 0;
				if ((node_info.GetType(mime) != B_NO_ERROR) || (strlen(mime) == 0))
					sniff_file(view->ExtensionMap(), name, mime);

				node.GetModificationTime(&time);
				node.GetSize(&size);
				file_name = name;
				file_path = path.Path();
				install_file(view, mime, &file_path, &file_name, time, size, eLocal);
			}
			else
			{
				/* ignore symlinks */
			}
		}
		snooze(1000);
	}
}


/*-----------------------------------------------------------------*/

void process_ftp(URL url, FileBrowserView* view)
{
	BMessage	errors;
	status_t	result;

	if ((result = view->FTP()->OpenRW(url, URL(), &errors, 0)) == B_NO_ERROR)
	{
		char*	buffer;
		int32	offset = 0;
		size_t	size = view->FTP()->GetContentLength();
		ssize_t	read;

		while ((buffer = (char*)malloc(size + 1)) < 0)
			size /= 2;
		buffer[size] = 0;

		while ((read = view->FTP()->Read(&buffer[offset], size - offset)) > 0)
			offset = parse_ftp(url, buffer, read, view);
		free(buffer);
	}
	else
	{
		BMessage	error('fbER');

		error.AddString("domain", view->Domain());
		error.AddString("message", strerror(result));
		error.AddInt32("error code", result);
		error.AddMessage("message", &errors);
		be_app->PostMessage(&error);
	}

}


/*-----------------------------------------------------------------*/

int32 parse_ftp(URL u, char* buffer, ssize_t len, FileBrowserView* view)
{
	char*	last_line = buffer;
	char*	line = buffer;
	BString	url;

	if ((strlen(view->User())) && (strlen(view->Password())))
		url << view->User() << ":" << view->Password() << "@" << view->Domain();
	else if (strlen(view->User()))
		url << view->User() << "@" << view->Domain();
	else
		url << view->Domain();

	while (view->Quit() == false)
	{
		char		mime[B_FILE_NAME_LENGTH];
		char		type;
		BString		name;
		BString		path;
		BString		tmp;
		int32		size;
		tm			tm_struct;
		time_t		time;

		last_line = line;

		if (!line[0])
			return 0;

		tmp = line;
		path = url;
		path << tmp;
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
				break;

			case 'd':	// directory - parse it
				{
					char*	previous_buffer;

					if ((previous_buffer = (char*)malloc(len)) > 0)
					{
						// save everything
						memcpy(previous_buffer, buffer, len);
						URL			previous_url(u);
						off_t		previous_position = view->FTP()->Position();
						BMessage	errors;
						BString		new_url;

						// process new directory
						new_url << "ftp://" << path;
						u = new_url.String();
						process_ftp(u, view);

						// restore everything
						memcpy(buffer, previous_buffer, len);
						u = previous_url;
						view->FTP()->OpenRW(u, URL(), &errors, 0);
						view->FTP()->Seek(previous_position, SEEK_SET);
					}
					else
						break;
				}
				break;

			case 'f':	// file
				sniff_file(view->ExtensionMap(), name.String(), mime);
				install_file(view, mime, &path, &name, time, size, eFTP);
				break;
		}
		snooze(1000);
	}
	memcpy(buffer, last_line, (buffer + len) - last_line);
	return((buffer + len) - last_line);
}


/*-----------------------------------------------------------------*/

file_type get_type(const char* mime)
{
	if (cistrncmp(mime, "audio/", 6) == 0)
		return eMediaFile;
	if (cistrncmp(mime, "video/quicktime", 15) == 0)
		return eOtherFile;
	if (cistrncmp(mime, "video/", 6) == 0)
		return eMediaFile;
	if (cistrncmp(mime, "application/vnd.rn-realmedia", 28) == 0)
		return eMediaFile;
	if (cistrncmp(mime, "application/x-shockwave-flash", 29) == 0)
		return eMediaFile;
	if (cistrncmp(mime, "image/", 6) == 0)
		return eImageFile;
	if (cistrncmp(mime, "text/", 5) == 0)
		return eTextFile;
	if (cistrncmp(mime, "message/rfc822", 14) == 0)
		return eTextFile;
	if (cistrncmp(mime, "application/pdf", 15) == 0)
		return eTextFile;
	return eOtherFile;
}


/*-----------------------------------------------------------------*/

void install_file(FileBrowserView* view, const char* mime, BString* path,
				  BString* name, time_t time, off_t size, PROTOCOL p)
{
	file_type	type;
	FilesView*	list = NULL;

	type = get_type(mime);
	switch (type)
	{
		case eMediaFile:
			list = view->MediaFiles();
			break;

		case eImageFile:
			list = view->ImageFiles();
			break;

		case eTextFile:
			list = view->TextFiles();
			break;

		case eOtherFile:
			list = view->OtherFiles();
			break;
	}

	if (list)
	{
		status_t	lock;

		lock = view->Window()->LockWithTimeout(1000000);
		if (lock != B_TIMED_OUT)
			list->AddFile(path, name, mime, time, size, p);
		if (lock == B_OK)
			view->Window()->Unlock();
	}
}


/*-----------------------------------------------------------------*/

void sniff_file(extension_map* map, const char* name, char* mime)
{
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


/*-----------------------------------------------------------------*/

bool is_trash(fs_info* info, BPath* path)
{
	const char*	dir = path->Path();
	bool		result = false;

	dir = strstr(&dir[1], "/");
	if (dir)
	{
		if ((strcmp(info->fsh_name, "dos") == 0) &&
			(strcmp(dir, "/RECYCLED") == 0))
			result = true;
		else if (((strcmp(info->fsh_name, "cfs") == 0) ||
				 (strcmp(info->fsh_name, "bfs") == 0)) &&
				 (strcmp(dir, "/home/Desktop/Trash") == 0))
			result = true;
	}
	return result;
}


/*-----------------------------------------------------------------*/

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


/*=================================================================*/

void send_busy(bool busy, const char* action, const char* volume)
{
	BMessage	msg('fbBZ');

	msg.AddBool("busy", busy);
	msg.AddString("action", action);
	msg.AddString("volume", volume);
	be_app->PostMessage(&msg);
}


/*=================================================================*/

#if 0

void FileBrowserView::LoadGenreMap()
{
	BPath		path;
	genre_map*	next = fGenreMap;

	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, true) == B_NO_ERROR)
	{
		BDirectory	dir(path.Path());
		BEntry		map;

		if (dir.FindEntry(kGENRE_MAP_NAME, &map) == B_NO_ERROR)
		{
			BFile	file(&map, O_RDONLY);

			if (file.InitCheck() == B_NO_ERROR)
			{
				char*	contents;
				char*	index;
				int32	length = 0;
				off_t	size;

				file.GetSize(&size);
				contents = (char*)malloc(size);
				file.Read(contents, size);
				index = contents;
				while (size)
				{
					genre_map*	map;

					if (*index == '\n')
					{
						*index = 0;

						map = new genre_map(index - length);
						*next = map;
						next = &map->next;
					}
					else
						length++;
					size--;
					index++;
				}
				free(contents);
			}
		}
	}

	/* if no map, something failed above, build a default map */
	if (!fGenreMap)
	{
		char*	default_map[] = {	"Blues",			/*   0 */
									"Classic Rock",		/*   1 */
									"Country",			/*   2 */
									"Dance",			/*   3 */
									"Disco",			/*   4 */
									"Funk",				/*   5 */
									"Grunge",			/*   6 */
									"Hip-Hop",			/*   7 */
									"Jazz",				/*   8 */
									"Metal",			/*   9 */
									"New Age",			/*  10 */
									"Oldies",			/*  11 */
									"Other",			/*  12 */
									"Pop",				/*  13 */
									"R&B",				/*  14 */
									"Rap",				/*  15 */
									"Reggae",			/*  16 */
									"Rock",				/*  17 */
									"Techno",			/*  18 */
									"Industrial",		/*  19 */
									"Alternative",		/*  20 */
									"Ska",				/*  21 */
									"Death Metal",		/*  22 */
									"Pranks",			/*  23 */
									"Soundtrack",		/*  24 */
									"Euro-Techno",		/*  25 */
									"Ambient",			/*  26 */
									"Trip-Hop",			/*  27 */
									"Vocal",			/*  28 */
									"Jazz+Funk",		/*  29 */
									"Fusion",			/*  30 */
									"Trance",			/*  31 */
									"Classical",		/*  32 */
									"Instrumental",		/*  33 */
									"Acid",				/*  34 */
									"House",			/*  35 */
									"Game",				/*  36 */
									"Sound Clip",		/*  37 */
									"Gospel",			/*  38 */
									"Noise",			/*  39 */
									"AlternRock",		/*  40 */
									"Bass",				/*  41 */
									"Soul",				/*  42 */
									"Punk",				/*  43 */
									"Space",			/*  44 */
									"Meditative",		/*  45 */
									"Instrumental Pop",	/*  46 */
									"Instrumental Rock",/*  47 */
									"Ethnic",			/*  48 */
									"Gothic",			/*  49 */
									"Darkwave",			/*  50 */
									"Techno-Industrial",/*  51 */
									"Electronic",		/*  52 */
									"Pop-Folk",			/*  53 */
									"Eurodance",		/*  54 */
									"Dream",			/*  55 */
									"Southern Rock",	/*  56 */
									"Comedy",			/*  57 */
									"Cult",				/*  58 */
									"Gangsta",			/*  59 */
									"Top 40",			/*  60 */
									"Christian Rap",	/*  61 */
									"Pop/Funk",			/*  62 */
									"Jungle",			/*  63 */
									"Native American",	/*  64 */
									"Cabaret",			/*  65 */
									"New Wave",			/*  66 */
									"Psychadelic",		/*  67 */
									"Rave",				/*  68 */
									"Showtunes",		/*  69 */
									"Trailer",			/*  70 */
									"Lo-Fi",			/*  71 */
									"Tribal",			/*  72 */
									"Acid Punk",		/*  73 */
									"Acid Jazz",		/*  74 */
									"Polka",			/*  75 */
									"Retro",			/*  76 */
									"Musical",			/*  77 */
									"Rock & Roll",		/*  78 */
									"Hard Rock",		/*  79 */
									"Folk",				/*  80 */
									"Folk-Rock",		/*  81 */
									"National Folk",	/*  82 */
									"Swing",			/*  83 */
									"Fast Fusion",		/*  84 */
									"Bebob",			/*  85 */
									"Latin",			/*  86 */
									"Revival",			/*  87 */
									"Celtic",			/*  88 */
									"Bluegrass",		/*  89 */
									"Avantgarde",		/*  90 */
									"Gothic Rock",		/*  91 */
									"Progressive Rock",	/*  92 */
									"Psychedelic Rock",	/*  93 */
									"Symphonic Rock",	/*  94 */
									"Slow Rock",		/*  95 */
									"Big Band",			/*  96 */
									"Chorus",			/*  97 */
									"Easy Listening",	/*  98 */
									"Acoustic",			/*  99 */
									"Humour",			/* 100 */
									"Speech",			/* 101 */
									"Chanson",			/* 102 */
									"Opera",			/* 103 */
									"Chamber Music",	/* 104 */
									"Sonata",			/* 105 */
									"Symphony",			/* 106 */
									"Booty Bass",		/* 107 */
									"Primus",			/* 108 */
									"Porn Groove",		/* 109 */
									"Satire",			/* 110 */
									"Slow Jam",			/* 111 */
									"Club",				/* 112 */
									"Tango",			/* 113 */
									"Samba",			/* 114 */
									"Folklore",			/* 115 */
									"Ballad",			/* 116 */
									"Power Ballad",		/* 117 */
									"Rhythmic Soul",	/* 118 */
									"Freestyle",		/* 119 */
									"Duet",				/* 120 */
									"Punk Rock",		/* 121 */
									"Drum Solo",		/* 122 */
									"A capella",		/* 123 */
									"Euro-House",		/* 124 */
									"Dance Hall",		/* 125 */
									""};
		int32 	index = 0;

		while (strlen(default_map[index]))
		{
			genre_map*	map;

			map = new genre_map(default_map[index]);
			*next = map;
			next = &map->next;
			index += 1;
		}
	}
}
#endif
