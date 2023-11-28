#include <stdio.h>

#include <Debug.h>
#include <experimental/ResourceSet.h>
#include <TranslatorFormats.h>
#include <View.h>
#include <ViewTransaction.h>
#include <Window.h>
#include <util.h>

#include "FileBrowserContent.h"
#include "FileBrowserView.h"
#include "Parameters.h"
#include "ResourceCache.h"

//#ifndef NDEBUG
	#define ddprintf		printf
//#else
//	#define ddprintf		if (1) {} else printf
//#endif


struct label
{
	char*		default_label;
	char*		msg_name;
};

struct color
{
	rgb_color	default_color;
	char*		msg_name;
};

struct offset
{
	int32		default_offset;
	char*		msg_name;
};

struct flag
{
	bool		default_value;
	char*		msg_name;
};

struct icon
{
	char*		default_icon;
	char*		msg_name;
};


//========================================================================

/* global */
extern int32		cistrncmp(const char*, const char*, int32);


/*======================== ContentInstance ========================*/

FileBrowserContentInstance::FileBrowserContentInstance(Content*			content,
													   GHandler*		handler,
													   const BMessage&	msg)
 : ContentInstance	(content, handler),
   BinderNode		(),
   fHeight			(457),
   fWidth			(400),
   fFileBrowser		(NULL),
   fView			(NULL)
{
	const char*	str;
	int32		loop;
							/* Default value					Message name */
	label		labels[] = {{kDEFAULT_MEDIA_TAB_ORNAMENT,		kMEDIA_TAB_ORNAMENT},
							{kDEFAULT_MEDIA_TAB_TITLE,			kMEDIA_TAB_TITLE},
							{kDEFAULT_MEDIA_TAB_ARTIST,			kMEDIA_TAB_ARTIST},
							{kDEFAULT_MEDIA_TAB_ALBUM,			kMEDIA_TAB_ALBUM},
							{kDEFAULT_MEDIA_TAB_TIME,			kMEDIA_TAB_TIME},
							{kDEFAULT_MEDIA_TAB_TRACK,			kMEDIA_TAB_TRACK},
							{kDEFAULT_MEDIA_TAB_CONTROLS,		kMEDIA_TAB_CONTROLS},
							{kDEFAULT_FILE_TAB_ORNAMENT,		kFILE_TAB_ORNAMENT},
							{kDEFAULT_FILE_TAB_NAME,			kFILE_TAB_NAME},
							{kDEFAULT_FILE_TAB_MODIFIED,		kFILE_TAB_MODIFIED},
							{kDEFAULT_FILE_TAB_SIZE,			kFILE_TAB_SIZE},
							{kDEFAULT_FILE_TAB_CONTROLS,		kFILE_TAB_CONTROLS}};
	color		colors[] = {{kDEFAULT_TAB_COLOR,				kTAB_COLOR},
							{kDEFAULT_TAB_SEPERATOR_COLOR,		kTAB_SEPERATOR_COLOR},
							{kDEFAULT_TAB_TEXT_COLOR,			kTAB_TEXT_COLOR},
							{kDEFAULT_TAB_SORT_COLOR,			kTAB_SORT_COLOR},
							{kDEFAULT_TAB_SORT_TEXT_COLOR,		kTAB_SORT_TEXT_COLOR},
							{kDEFAULT_LIST_COLOR,				kLIST_COLOR},
							{kDEFAULT_LIST_SEPERATOR_COLOR,		kLIST_SEPERATOR_COLOR},
							{kDEFAULT_LIST_SELECTED_COLOR,		kLIST_SELECTED_COLOR},
							{kDEFAULT_LIST_TEXT_COLOR,			kLIST_TEXT_COLOR},
							{kDEFAULT_LIST_SELECTED_TEXT_COLOR,	kLIST_SELECTED_TEXT_COLOR},
							{kDEFAULT_LIST_SELECTED_FRAME_COLOR,kLIST_SELECTED_FRAME_COLOR},
							{kDEFAULT_LIST_ROLLOVER_COLOR,		kLIST_ROLLOVER_COLOR},
							{kDEFAULT_LIST_EDIT_COLOR,			kLIST_EDIT_COLOR}};
	offset		offsets[]= {{kDEFAULT_TAB_HEIGHT,				kTAB_HEIGHT},
							{kDEFAULT_TAB_SPACER_WIDTH,			kTAB_SPACER_WIDTH},
							{kDEFAULT_LIST_SELECTED_FRAME_WIDTH,kLIST_SELECTED_FRAME_WIDTH},
							{kDEFAULT_LIST_ITEM_HEIGHT,			kLIST_ITEM_HEIGHT},
							{kDEFAULT_LIST_SPACER_HEIGHT,		kLIST_SPACER_HEIGHT}};
	flag		flags[]  = {{kDEFAULT_SUPPORT_DRAG,				kSUPPORT_DRAG},
							{kDEFAULT_SHOW_TRASHED_FILES,		kSHOW_TRASHED_FILES}};
	icon		icons[]  = {{kTYPE_AUDIO,						kAUDIO_ICON},
							{kTYPE_VIDEO,						kVIDEO_ICON},
							{kTYPE_IMAGE,						kIMAGE_ICON},
							{kTYPE_TEXT,						kTEXT_ICON},
							{kTYPE_HTML,						kHTML_ICON},
							{kTYPE_MAIL,						kMAIL_ICON},
							{kTYPE_OTHER,						kOTHER_ICON},
							{kAPPROVE_EDIT_UP,					kEDIT_ACCEPT_UP_ICON},
							{kAPPROVE_EDIT_OVER,				kEDIT_ACCEPT_OVER_ICON},
							{kAPPROVE_EDIT_DOWN,				kEDIT_ACCEPT_DOWN_ICON},
							{kCANCEL_EDIT_UP,					kEDIT_REJECT_UP_ICON},
							{kCANCEL_EDIT_OVER,					kEDIT_REJECT_OVER_ICON},
							{kCANCEL_EDIT_DOWN,					kEDIT_REJECT_DOWN_ICON},
							{kSORT_ARROW,						kSORT_ASCENDING_ICON},
							{NULL,								kSORT_DESCENDING_ICON}};

	if (msg.FindString("width", &str) == B_NO_ERROR)
		fWidth = strtol(str, NULL, 0);
	if (msg.FindString("height", &str) == B_NO_ERROR)
		fHeight = strtol(str, NULL, 0);

	for (loop = 0; loop < eLabelCount; loop++)
	{
		fParameters.labels[loop].SetTo(labels[loop].default_label);
		if (msg.HasString(labels[loop].msg_name))
			fParameters.labels[loop].SetTo(msg.FindString(labels[loop].msg_name));
	}

	for (loop = 0; loop <eColorCount; loop++)
	{
		fParameters.colors[loop] = colors[loop].default_color;
		if (msg.HasString(colors[loop].msg_name))
			fParameters.colors[loop] = decode_color(msg.FindString(colors[loop].msg_name));
	}

	for (loop = 0; loop < eOffsetsCount; loop++)
	{
		fParameters.offsets[loop] = offsets[loop].default_offset;
		if (msg.HasInt32(offsets[loop].msg_name))
			msg.FindInt32(offsets[loop].msg_name, &fParameters.offsets[loop]);
	}

	for (loop = 0; loop < eFlagsCount; loop++)
	{
		fParameters.flags[loop] = flags[loop].default_value;
		if (msg.FindString(flags[loop].msg_name, &str) == B_NO_ERROR)
			fParameters.flags[loop] = (cistrncmp(str, "true", 4) == 0);
	}

	for (loop = 0; loop < eIconCount; loop++)
	{
		URL	url;

		fParameters.icons[loop] = NULL;
		if (msg.HasString(icons[loop].msg_name))
			url.SetTo(msg.FindString(icons[loop].msg_name));
		else if (icons[loop].default_icon)
			url.SetTo(icons[loop].default_icon);
		else
			continue;

		resourceCache.NewContentInstance(url, loop, this, 0, BMessage(),
									securityManager.GetGroupID(url), 0, (const char*)NULL);
	}
	
	fParameters.tab_font = kDEFAULT_TAB_FONT;
	fParameters.tab_font.SetSize(kDEFAULT_TAB_FONT_SIZE);
	if (msg.FindString(kTAB_FONT, &str) == B_NO_ERROR)
		decode_font(str, &fParameters.tab_font);

	fParameters.list_font = kDEFAULT_LIST_FONT;
	fParameters.list_font.SetSize(kDEFAULT_LIST_FONT_SIZE);
	if (msg.FindString(kLIST_FONT, &str) == B_NO_ERROR)
		decode_font(str, &fParameters.list_font);

	fDefaultVolume = "";
	if (msg.HasString(kDEFAULT_VOLUME))
		fDefaultVolume.SetTo(msg.FindString(kDEFAULT_VOLUME));

	fDefaultView = "";
	if (msg.HasString(kDEFAULT_VIEW))
		fDefaultView.SetTo(msg.FindString(kDEFAULT_VIEW));
}


/*-----------------------------------------------------------------*/

FileBrowserContentInstance::~FileBrowserContentInstance()
{
	for (int32 id = 0; id < eIconCount; id++)
	{
		if (fParameters.icons[id])
			fParameters.icons[id]->Release();
	}
}


/*-----------------------------------------------------------------*/

status_t FileBrowserContentInstance::AttachedToView(BView*	view,
													uint32*	/* flags */)
{
	fView = view;
	fView->AddChild(fFileBrowser = new FileBrowserView(BRect(0, 0, fWidth - 1, fHeight - 1),
					&fParameters, &fDefaultVolume, &fDefaultView));
	return B_NO_ERROR;
}


/*-----------------------------------------------------------------*/

status_t FileBrowserContentInstance::DetachedFromView()
{
	if ((fView) && (fFileBrowser))
	{
		fView->RemoveChild(fFileBrowser);
		delete fFileBrowser;
		fFileBrowser = NULL;
	}

	return B_NO_ERROR;
}


/*-----------------------------------------------------------------*/

status_t FileBrowserContentInstance::Draw(BView*	/* view */,
										  BRect		/* exposed */)
{
	return B_NO_ERROR;
}


/*-----------------------------------------------------------------*/

status_t FileBrowserContentInstance::FrameChanged(BRect	new_frame,
												  int32	full_width,
												  int32	full_height)
{
	if (fFileBrowser)
	{
		ViewTransaction	trans;

		if (new_frame != fFileBrowser->Frame())
			fView->Invalidate(fFileBrowser->Frame());
		trans.Move(fFileBrowser, new_frame.left, new_frame.top);
		trans.Resize(fFileBrowser, new_frame.Width(), new_frame.Height());
		trans.Commit();
	}
	fWidth = full_width;
	fHeight = full_height;

	return B_NO_ERROR;
}


/*-----------------------------------------------------------------*/

status_t FileBrowserContentInstance::GetSize(int32*		width,
											 int32*		height,
											 uint32*	/* resize_flags */)
{
	*width = fWidth;
	*height = fHeight;

	return B_NO_ERROR;
}


/*-----------------------------------------------------------------*/

status_t FileBrowserContentInstance::HandleMessage(BMessage* msg)
{
	switch (msg->what)
	{
		case NEW_CONTENT_INSTANCE:
			{
				int32					id = -1;
				atom<ContentInstance>	instance;

				msg->FindInt32("id", &id);
				msg->FindAtom("instance",instance);
				if ((id >= 0) && (id < eIconCount))
				{
					fParameters.icons[id] = instance;
					instance->Acquire();
				}
			}
			break;

		case 'fbSI':	/* display I-Drive */
		case 'fbDM':	/* display media */
		case 'fbDI':	/* display images */
		case 'fbDT':	/* display text */
		case 'fbDO':	/* display other */
		case 'fbSV':	/* show volume */
		case 'fbRV':	/* remove volume */
		case 'fbCP':	/* copy the selected to target */
		case 'fbDL':	/* delete the selected files */
		case 'fbOP':	/* open selected files */
		case 'fbED':	/* edit selected files */
		case 'fbAD':	/* file added */
			if (fFileBrowser)
				fFileBrowser->Window()->PostMessage(msg, fFileBrowser);
			break;

		default:
			return GHandler::HandleMessage(msg);
	}
	return B_NO_ERROR;
}

// ----------------------------------------------------------------------
// BinderNode implementation
// ----------------------------------------------------------------------

status_t FileBrowserContentInstance::OpenProperties(void**	cookie,
													void*	copyCookie)
{
	PRINT(("FileBrowserContentInstance::OpenProperties\n"));
	int32 *index = new int32;
	*index = 0;
	if (copyCookie)
		*index = *((int32 *)copyCookie);

	*cookie = index;
	return B_OK;
}


/*-----------------------------------------------------------------*/

status_t FileBrowserContentInstance::NextProperty(void*		cookie,
												  char*		nameBuf,
												  int32*	len)
{
	PRINT(("FileBrowserContentInstance::NextProperty\n"));
	int32 *index = (int32 *)cookie;
	if (*index >= (int32)((sizeof(kBinderProperties) / sizeof(char const*))))
		return ENOENT;

	const char *item = kBinderProperties[(*index)++];
	strncpy(nameBuf, item, *len);
	*len = strlen(item);
	return B_OK;
}


/*-----------------------------------------------------------------*/

status_t FileBrowserContentInstance::CloseProperties(void*	cookie)
{
	PRINT(("FileBrowserContentInstance::CloseProperties\n"));
	int32 *index = (int32 *)cookie;
	delete index;
	return B_OK;
}


/*-----------------------------------------------------------------*/

put_status_t FileBrowserContentInstance::WriteProperty(const char*		/* name */,
													   const property&	/* prop */)
{
	PRINT(("FileBrowserContentInstance::WriteProperty\n"));
	return EPERM;
}


/*-----------------------------------------------------------------*/

get_status_t FileBrowserContentInstance::ReadProperty(const char*			name,
													  property&				prop,
													  const property_list&	args = empty_arg_list)
{
	PRINT(("FileBrowserContentInstance::ReadProperty(\"%s\")\n", name));

	// Functions
	BMessage	m((uint32)0);

	if (strcmp(name, "DisplayMedia") == 0)
	{
		m.what = 'fbDM';
	}
	else if (strcmp(name, "DisplayImages") == 0)
	{
		m.what = 'fbDI';
	}
	else if (strcmp(name, "DisplayText") == 0)
	{
		m.what = 'fbDT';
	}
	else if (strcmp(name, "DisplayOther") == 0)
	{
		m.what = 'fbDO';
	}
	else if (strcmp(name, "ShowVolume") == 0)
	{
		m.what = 'fbSV';
		m.AddString("mounted_at", args[0].String().String());
	}
	else if (strcmp(name, "RemoveVolume") == 0)
	{
		m.what = 'fbRV';
	}
	else if (strcmp(name, "ShowNetworkVolume") == 0)
	{
		m.what = 'fbSI';
		m.AddString("domain", args[0].String().String());
		m.AddString("user", args[1].String().String());
		m.AddString("password", args[2].String().String());
		m.AddString("path", args[3].String().String());
	}
	else if (strcmp(name, "CopySelectedToLocal") == 0)
	{
		m.what = 'fbCP';
		m.AddString("mounted_at", args[0].String().String());
	}
	else if (strcmp(name, "CopySelectedToNetwork") == 0)
	{
		m.what = 'fbCP';
		m.AddString("domain", args[0].String().String());
		m.AddString("user", args[1].String().String());
		m.AddString("password", args[2].String().String());
		m.AddString("path", args[3].String().String());
	}
	else if (strcmp(name, "DeleteSelected") == 0)
	{
		m.what = 'fbDL';
	}
	else if (strcmp(name, "OpenSelected") == 0)
	{
		m.what = 'fbOP';
	}
	else if (strcmp(name, "EditSelected") == 0)
	{
		m.what = 'fbED';
	}
	else if (strcmp(name, "FileAdded") == 0)
	{
		m.what = 'fbAD';
		m.AddString("path", args[0].String().String());
	}
	else if ((fFileBrowser) && (strcmp(name, "FileList") == 0))
	{
		BMessenger	target(fFileBrowser);

		if (target.IsValid())
		{
			BMessage	reply;

			if (target.SendMessage('fbFL', &reply) == B_NO_ERROR)
			{
				int32				index = 0;
				BinderContainer*	result;

				result = new BinderContainer();

				while (reply.HasString("path", index))
				{
					const char*			path;
					const char* 		mime;
					off_t				size = 0;
					BinderContainer*	item;
					BString				name("file_");

					reply.FindString("path", index, &path);
					reply.FindString("mime", index, &mime);
					reply.FindInt64("size", index, &size);
					item = new BinderContainer();
					item->AddProperty("path", path);
					item->AddProperty("mime", mime);
					item->AddProperty("size", size);
					name << index;
					result->AddProperty(name.String(), item);
					index++;
				}
				prop = result;
				return B_OK;
			}
		}
	}
	else if ((fFileBrowser) && (strcmp(name, "editMode") == 0))
	{
		BMessenger	target(fFileBrowser);

		if (target.IsValid())
		{
			BMessage	reply;

			if (target.SendMessage('fbEM', &reply) == B_NO_ERROR)
			{
				prop = property(reply.FindString("edit_mode"));
				return B_OK;
			}
		}
	}

	if (m.what)
	{
		if (fFileBrowser)
			fFileBrowser->Window()->PostMessage(&m, fFileBrowser);
		return B_OK;
	}

	// Exposed data

	PRINT(("FileBrowserContentInstance::ReadProperty: no match\n"));
	return ENOENT;
}
// ----------------------------------------------------------------------
// end of BinderNode implementation
// ----------------------------------------------------------------------


/*============================ Content ============================*/

FileBrowserContent::FileBrowserContent(void* handle)
 : Content(handle)
{
}


/*-----------------------------------------------------------------*/

FileBrowserContent::~FileBrowserContent()
{
}


/*-----------------------------------------------------------------*/

status_t FileBrowserContent::CreateInstance(ContentInstance**	out_instance,
											GHandler*			handler,
											const BMessage&		msg)
{
	*out_instance = new FileBrowserContentInstance(this, handler, msg);
	return B_NO_ERROR;
}


/*-----------------------------------------------------------------*/

ssize_t FileBrowserContent::Feed(const void*	/* buffer */,
								 ssize_t		/* count */,
								 bool			/* done */)
{
	return 0;
}


/*-----------------------------------------------------------------*/

size_t FileBrowserContent::GetMemoryUsage()
{
	return sizeof(FileBrowserView);
}


/*-----------------------------------------------------------------*/

bool FileBrowserContent::IsInitialized()
{
	return true;
}


//====================== FileBrowserContentFactory ======================

class FileBrowserContentFactory : public ContentFactory
{
public:
	virtual void GetIdentifiers(BMessage* into)
	{
		 /*
		 ** BE AWARE: Any changes you make to these identifiers should
		 ** also be made in the 'addattr' command in the makefile.
		 */
		into->AddString(S_CONTENT_MIME_TYPES, "application/x-vnd.Be.FileBrowser");

		into->AddString(S_CONTENT_PLUGIN_IDS, "File Browser");
		into->AddString(S_CONTENT_PLUGIN_DESCRIPTION, "File Browser");
	}
	
	virtual Content* CreateContent(void*		handle,
								   const char*	/* mime */,
								   const char*	/* extension */)
	{
		return new FileBrowserContent(handle);
	}
};


/*-----------------------------------------------------------------*/

extern "C" _EXPORT ContentFactory* make_nth_content(int32		n,
													image_id	/* you */,
													uint32		/* flags */,
													...)
{
	if (n == 0)
		return new FileBrowserContentFactory;
	return 0;
}
