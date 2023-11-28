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
	label		labels[] = {{kDEFAULT_FILE_TAB_ORNAMENT,		kFILE_TAB_ORNAMENT},
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
							{kDEFAULT_LIST_EDIT_COLOR,			kLIST_EDIT_COLOR},
							{kDEFAULT_PATH_COLOR,				kPATH_COLOR},
							{kDEFAULT_PATH_TEXT_COLOR,			kPATH_TEXT_COLOR},
							{kDEFAULT_PATH_GREYED_TEXT_COLOR,	kPATH_GREYED_TEXT_COLOR},
							{kDEFAULT_PATH_DROP_COLOR,			kPATH_DROP_COLOR}};
	offset		offsets[]= {{kDEFAULT_TAB_HEIGHT,				kTAB_HEIGHT},
							{kDEFAULT_TAB_SPACER_WIDTH,			kTAB_SPACER_WIDTH},
							{kDEFAULT_LIST_SELECTED_FRAME_WIDTH,kLIST_SELECTED_FRAME_WIDTH},
							{kDEFAULT_LIST_ITEM_HEIGHT,			kLIST_ITEM_HEIGHT},
							{kDEFAULT_LIST_SPACER_HEIGHT,		kLIST_SPACER_HEIGHT},
							{kDEFAULT_PATH_HEIGHT,				kPATH_HEIGHT}};
	flag		flags[]  = {{kDEFAULT_SUPPORT_DRAG,				kSUPPORT_DRAG}};
	icon		icons[]  = {{kTYPE_AUDIO,						kAUDIO_ICON},
							{kTYPE_VIDEO,						kVIDEO_ICON},
							{kTYPE_IMAGE,						kIMAGE_ICON},
							{kTYPE_TEXT,						kTEXT_ICON},
							{kTYPE_OTHER,						kOTHER_ICON},
							{kTYPE_FOLDER_CLOSED,				kFOLDER_CLOSED_ICON},
							{kTYPE_FOLDER_OPENED,				kFOLDER_OPENED_ICON},
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

	fParameters.path_font = kDEFAULT_PATH_FONT;
	fParameters.path_font.SetSize(kDEFAULT_PATH_FONT_SIZE);
	if (msg.FindString(kPATH_FONT, &str) == B_NO_ERROR)
		decode_font(str, &fParameters.path_font);

	fDefaultURL = "file:///";
	if (msg.HasString(kDEFAULT_URL))
		fDefaultURL << msg.FindString(kDEFAULT_URL);
}


/*-----------------------------------------------------------------*/

FileBrowserContentInstance::~FileBrowserContentInstance()
{
	for (int32 id = 0; id < eIconCount; id++)
	{
		if (fParameters.icons[id])
			delete fParameters.icons[id];
	}
}


/*-----------------------------------------------------------------*/

status_t FileBrowserContentInstance::AttachedToView(BView*	view,
													uint32*	/* flags */)
{
	fView = view;
	fView->AddChild(fFileBrowser = new FileBrowserView(BRect(0, 0, fWidth - 1, fHeight - 1),
					&fParameters, fDefaultURL.String()));
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
					int32	height;
					int32	width;
					uint32	flags;
					BView*	view;

					instance->GetSize(&width, &height, &flags);
					fParameters.icons[id] = new BBitmap(BRect(0, 0, width - 1, height - 1), B_RGBA32, true);
					view = new BView(fParameters.icons[id]->Bounds(), "", B_FOLLOW_NONE, 0);
					fParameters.icons[id]->AddChild(view);
					fParameters.icons[id]->Lock();
					view->SetHighColor(B_TRANSPARENT_32_BIT);
					view->FillRect(view->Bounds());
					instance->FrameChanged(fParameters.icons[id]->Bounds(), width, height);
					instance->Draw(view, fParameters.icons[id]->Bounds());
					view->Sync();
					fParameters.icons[id]->Unlock();
					fParameters.icons[id]->RemoveChild(view);
					delete view;
				}
			}
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

	if (strcmp(name, "ShowVolume") == 0)
	{
		BString		url("file://");

		url << args[0].String().String();
		m.what = eDisplayVolume;
		m.AddString("url", url.String());
	}
	else if (strcmp(name, "ShowNetworkVolume") == 0)
	{
		BString		url("ftp://");

		url << args[1].String().String()	// user
			<< ":"
			<< args[2].String().String()	// password
			<< "@"
			<< args[0].String().String()	// host
			<< args[3].String().String();	// path
		m.what = eDisplayVolume;
		m.AddString("url", url.String());
	}
	else if (strcmp(name, "DeleteSelected") == 0)
	{
		m.what = eDeleteItems;
	}
	else if (strcmp(name, "OpenSelected") == 0)
	{
		m.what = eOpenItems;
	}
	else if (strcmp(name, "EditSelected") == 0)
	{
		m.what = eEditItem;
	}
	else if (strcmp(name, "MakeDirectory") == 0)
	{
		m.what = eMakeDirectory;
		m.AddString("name", args[0].String().String());
	}
	else if ((fFileBrowser) && (strcmp(name, "FileList") == 0))
	{
		BMessenger	target(fFileBrowser);

		if (target.IsValid())
		{
			BMessage	reply;
			BMessage	message(eAttachItems);

			message.AddBool("recursive", false);
			if (target.SendMessage(&message, &reply) == B_NO_ERROR)
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
// *ftp debug code*
	else if (strcmp(name, "DisplayOther") == 0)
	{
		m.what = eDisplayVolume;
		m.AddString("url", "ftp://robert:%polic%@208.243.144.33/usr/home/robert");
	}
	else if (strcmp(name, "DisplayText") == 0)
	{
		m.what = eMakeDirectory;
		//m.AddString("name", args[0].String().String());
		m.AddString("name", "New Folder");
	}
// *ftp end debug code*

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
