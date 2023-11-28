
#include <Autolock.h>
#include <ContentView.h>
#include <ScrollBar.h>
#include <stdio.h>
#include <FindDirectory.h>
#include <File.h>
#include <Window.h>
#include "ContactManagerContent.h"

// #define DEBUG_OUTPUT  // un-comment for printf() output

static const uint32 kInvokeMessage = 'INVO';
static const uint32 kEndEditMessage = 'EnDe';
static const uint32 kSelectionMessage ='SELE';

static char* kContentSignature = "application/x-vnd.Be.ContactManager";

int32 kPropertyCount = 11;
static const char *kPropertyList [] = {
	"addressStatus",
	"CreateEntryFunc",
	"DeleteSelectedEntriesFunc",
	"EditEntryFunc",
	"editMode",
	"EditSelectedEntryFunc",
	"EntryCountFunc",
	"GetEntryFunc",
	"GetSelectedEntriesFunc",
	"PurgeDeletedFunc",
	"selectionCount"
};

static struct t_column_info cinfo [] = 
{
	{COLUMN_ICON, "", 24, 0, "address.png", "address.png", "address.png", "address.png", 0, 0 },
	{COLUMN_STRINGFIELD, "First", 130, "firstName", {0}, {0}, {0}, {0}, 0, 0 },
	{COLUMN_STRINGFIELD, "Last", 130, "lastName", {0}, {0}, {0}, {0}, 0, 0 },
	{COLUMN_STRINGFIELD, "Email", 194, "email", {0}, {0}, {0}, {0}, 0, 0 },
    {COLUMN_COUNTERFIELD, "", 12, "", {0}, {0}, {0}, {0}, 0, 0},
	{COLUMN_BITMAPBUTTON, "", 60, 0, "cancel_edit_up.png", "cancel_edit_over.png", "cancel_edit_down.png", {0}, 'CANC', 0 },
	{COLUMN_BITMAPBUTTON, "", 60, 0, "approve_edit_up.png", "approve_edit_over.png", "approve_edit_down.png", {0}, 'OKAY', 0 },
	{COLUMN_END, {0}, 0, 0, {0}, {0}, {0}, {0}, 0, 0 } 
};

BResourceSet& Resources(void)
{
	static BResourceSet gResources;
	static bool gResInitialized = false;
	if( gResInitialized )
		return gResources;
	gResources.AddEnvDirectory("${/service/web/macros/RESOURCES}/Mail/images", "/boot/custom/resources/en/Mail/images");
	gResInitialized = true;
	return gResources;
}

MagicBookContentInstance::MagicBookContentInstance(MagicBookContent *parent, GHandler *h, const BString &/*mime*/, const BMessage &attributes):ContentInstance(parent, h)
{ 
	fBinderRoot = BinderNode::Root() ["user"] ["~"] ["addressbook"];
	
	fAddressList = new TListView(fBinderRoot,
	 BRect(0, 0, 600, 350), attributes, cinfo);
	
	fAddressList->SetHandler(this,kInvokeMessage,kEndEditMessage);
	fAddressList->SetSelectionMessage(new BMessage(kSelectionMessage));
}

MagicBookContentInstance::~MagicBookContentInstance()
{
}

status_t MagicBookContentInstance::OpenProperties(void **cookie, void *copyCookie)
{
	int32 *index = new int32;
	*index = 0;
	if (copyCookie)
		*index = *((int32 *)copyCookie);
	*cookie = index;
	return B_OK;
}

status_t MagicBookContentInstance::NextProperty(void *cookie, char *nameBuffer, int32 *length)
{
	int32 *index = (int32 *)cookie;
	if (*index >= kPropertyCount)
		return ENOENT;
	const char *item = kPropertyList[(*index)++];
	strncpy(nameBuffer, item, *length);
	*length = strlen(item);
	return B_OK;
}

status_t MagicBookContentInstance::CloseProperties(void *cookie)
{
	int32 *index = (int32 *)cookie;
	delete index;
	return B_OK;
}

status_t MagicBookContentInstance::CreateEntry(property& out,
 const property_list& inArgs)
{
	static  int32 idcnt = 0;
	atom<BinderContainer> node = new BinderContainer;
	
	// Create unique id, sort of...
	int id = (time(NULL)&0xFFFFFF00) + (idcnt++);
	char idstr[16];
	sprintf(idstr, "A%08x", id);
	
	BinderNode::property newprop;
	fBinderRoot->GetProperty("+address",newprop);
	
	if (!newprop.IsObject()) return B_ERROR;
	
	fBinderRoot [idstr] = newprop;	
	newprop ["nickname"] = inArgs[0].String().String();
	newprop ["firstName"] = inArgs[1].String().String();
	newprop ["lastName"] = inArgs[2].String().String();
	newprop ["email"] = inArgs[3].String().String();
	newprop ["isDeletable"] = 1;
	
	TListItem* row = 0;
	fAddressList->DeselectAll();
	fAddressList->AddRow(row = new TListItem(fAddressList,fBinderRoot,idstr,cinfo));
	row->SetBrandNew(true);
	fAddressList->AddToSelection(row);
	fAddressList->SetFocusRow(row);
	fAddressList->SelectionChanged();
	
	node->AddProperty("result","B_OK");
	node->AddProperty("id",(const char *)idstr);
	node->AddProperty("identification",idstr);
					
	out = property(node);
	fAddressList->Refresh();
					
	return B_OK;
}

bool MagicBookContentInstance::DuplicateEntry(const property_list& inArgs)
{
//	BString     nickname  = inArgs[0].String().String();
	BString     firstname = inArgs[1].String().String();
	BString     lastname  = inArgs[2].String().String();
	BString     email     = inArgs[3].String().String();
	BString     str;
	uint32      i;
	uint32      count = fAddressList->CountRows();
	TListItem*  item;
	
	for (i = 0; i< count; i++)
	{
		item = (TListItem*) fAddressList->RowAt(i);
		if (!item) {ASSERT(false); continue;}
		
	//	str = item->Property()["nickname"].String().String();
	//	if (strcasecmp(str.String(), nickname.String())) continue;
		
		str = item->Property()["firstName"].String().String();
		if (strcasecmp(str.String(), firstname.String())) continue;
		
		str = item->Property()["lastName"].String().String();
		if (strcasecmp(str.String(), lastname.String())) continue;
		
		str = item->Property()["email"].String().String();
		if (strcasecmp(str.String(), email.String())) continue;
		
		return true;  // Duplicate found!
	}
		
	return false;  // All checked, no duplicates
}

get_status_t MagicBookContentInstance::ReadProperty(const char *name, property &outProperty, const property_list &inArgs)
{
	if(fAddressList)
	{
		if (strcmp(name,"editMode")==0)
		{
			char  str[10];
			
			if (!fAddressList->LockLooper()) return B_ERROR;
			
			if (fAddressList->InEditMode())
			  strcpy(str, "true");
			else
			  strcpy(str, "false");
			  
		    outProperty = property(str);
			fAddressList->UnlockLooper();
			return B_OK;
		}
		
		if(!fAddressList->InEditMode() && fAddressList->LockLooper())
		{
			if(strcmp(name,"CreateEntryFunc")==0) 
			{
				status_t  status = B_OK;
				
				if (DuplicateEntry(inArgs))
				{
					outProperty = property("duplicate");
				}
				else if (fAddressList->CountRows() >= 100)
				{
					outProperty = property("full");
				}
				else
				{
					status = CreateEntry(outProperty, inArgs);
				}	
				
				fAddressList->UnlockLooper();
				return status;
			}
			else if(strcmp(name,"DeleteSelectedEntriesFunc")==0) 
			{
				for(TListItem *row=0;(row=(TListItem *)fAddressList->CurrentSelection(row))!=0;)
				{
					bool isDeletable = true;
	
					BinderNode::property prop;
					if(row->Property()->GetProperty("isDeletable",prop)==B_OK)
						if(prop.Number() == 0)
							isDeletable = false;
	
					if(!isDeletable)
						continue;
						
					row->Tag();
				}
				fAddressList->UnlockLooper();
	
				return B_OK;
			}
			else if(strcmp(name,"EditEntryFunc")==0) 
			{
				atom<BinderContainer> node = new BinderContainer;
				for(int32 c=0;c<fAddressList->CountRows();c++)
				{
					TListItem *item = (TListItem *)fAddressList->RowAt(c);
					if(item->IDStr() == inArgs[0].String())
					{
						fAddressList->DeselectAll();
						fAddressList->AddToSelection(item);
						fAddressList->SetFocusRow(item);
						fAddressList->SelectionChanged();
						fAddressList->BeginEdit();
						
						#ifdef DEBUG_OUTPUT
						printf("Contact edit mode begin\n");
						#endif
	
						node->AddProperty("result", "B_OK");
						outProperty = property(node);
						fAddressList->UnlockLooper();
	
						return B_OK;
					}
				}
				node->AddProperty("result", "B_ERROR: ID not found");
				outProperty = property(node);
				fAddressList->UnlockLooper();
	
				return B_OK;
			}
			else if(strcmp(name,"EditSelectedEntryFunc")==0) 
			{
				fAddressList->BeginEdit();
				#ifdef DEBUG_OUTPUT
				printf("Contact edit mode begin\n");
				#endif
				fAddressList->UnlockLooper();
				NotifyListeners(B_PROPERTY_CHANGED,"editMode");
	 			return B_OK;
			}
			else if(strcmp(name,"EntryCountFunc")==0) 
			{
				outProperty = property((int)fAddressList->CountRows());
				fAddressList->UnlockLooper();
				return B_OK;
			}
			else if(strcmp(name,"GetEntryFunc")==0) 
			{
				atom<BinderContainer> node = new BinderContainer;
				for(int32 c=0;c<fAddressList->CountRows();c++)
				{
					TListItem *item = (TListItem *)fAddressList->RowAt(c);
					if(item->IDStr() == inArgs[0].String())
					{
						node->AddProperty("nickname",item->Property()["nickname"]);
						node->AddProperty("firstName",item->Property()["firstName"]);
						node->AddProperty("lastName",item->Property()["lastName"]);
						node->AddProperty("email",item->Property()["email"]);
						node->AddProperty("isDeletable",item->Property()["isDeletable"]);
						node->AddProperty("result", "B_OK");
						outProperty = property(node);
						fAddressList->UnlockLooper();
						return B_OK;
					}
				}
				node->AddProperty("result", "B_ERROR: ID not found");
				outProperty = property(node);
				fAddressList->UnlockLooper();
				return B_OK;
			}
			else if(strcmp(name,"GetSelectedEntriesFunc")==0)
			{
				atom<BinderContainer> node = new BinderContainer;
				TListItem *item = 0;
				for(int32 c=0;(item = (TListItem *)fAddressList->CurrentSelection(item))!=0;c++)
				{
					if(c==atoi(inArgs[0].String().String()))
					{
						node->AddProperty("nickname",item->Property()["nickname"]);
						node->AddProperty("firstName",item->Property()["firstName"]);
						node->AddProperty("lastName",item->Property()["lastName"]);
						node->AddProperty("email",item->Property()["email"]);
						node->AddProperty("isDeletable",item->Property()["isDeletable"]);
						node->AddProperty("result", "B_OK");
						outProperty = property(node);
						fAddressList->UnlockLooper();
						return B_OK;
					}
				}
				node->AddProperty("result","B_ERROR: Illegal index");
				outProperty = property(node);
				fAddressList->UnlockLooper();
				return B_OK;
			}
			else if(strcmp(name,"PurgeDeletedFunc")==0)
			{
				fAddressList->DeselectAll();
				for(int32 c=0;c<fAddressList->CountRows();c++)	
				{
					if(((TListItem *)fAddressList->RowAt(c))->IsTagged())
					{
						TListItem *item = (TListItem *)fAddressList->RowAt(c);
						fAddressList->RemoveRow(item);
						item->RootProperty()[item->IDStr().String()] = BinderNode::property::undefined;
						delete item;
						c--;
					}
				}
				fAddressList->UnlockLooper();
				return B_OK;
			}
			else if(strcmp(name,"addressStatus")==0)
			{
				atom<BinderContainer> node = new BinderContainer;
				node->AddProperty("result", "B_OK");
				outProperty = property(node);
				fAddressList->UnlockLooper();
				return B_OK;
			}
			else if(strcmp(name,"selectionCount")==0)
			{
				int32 count = 0;
				for(TListItem *item=0;(item = (TListItem *)fAddressList->CurrentSelection(item))!=0;count++) {}
			    outProperty = property((int)count);
				fAddressList->UnlockLooper();
				return B_OK;
			}
			fAddressList->UnlockLooper();
		}
		else
		{
			atom<BinderContainer> node = new BinderContainer;
			node->AddProperty("result", "B_ERROR: Plugin in edit mode");
			outProperty = property(node);
			return B_OK;
		}
	}
	return BinderNode::ReadProperty(name,outProperty,inArgs);
}

status_t MagicBookContentInstance::AttachedToView(BView *inView, uint32 */*contentFlags*/)
{
	if (fAddressList)
	{
		inView->AddChild(fAddressList);
		BRect rect = FrameInParent();
		fAddressList->MoveTo(rect.LeftTop());
		fAddressList->ResizeTo(rect.Width(), rect.Height());	
	}
	return B_OK;
}

status_t MagicBookContentInstance::DetachedFromView()
{
	if (fAddressList)
	{
		fAddressList->RemoveSelf();
		delete fAddressList;
		fAddressList = 0;
	}
	return B_OK;
}

// The idea behind this rather hideous thing is that when you are
// editing an item that has just been created and then the user presses
// the "Cancel" button, certain idiots think the right thing is for
// the new item to go away.  So now it does, but that's STOOOPIT.

void MagicBookContentInstance::DeleteIfBrandNew(const BMessage* msg)
{
	bool		accept;
	TListItem*  item;
	status_t	status;
	
	item = (TListItem*) fAddressList->CurrentSelection(0);
	if (!item) return;
	if (!item->IsBrandNew()) return;
	
	item->SetBrandNew(false);   // First edit is over, no longer brand new
	
	status = msg->FindBool("accept", &accept);
	if (status != B_OK || accept) return;
	
	if (!fAddressList->LockLooper()) return;
	fAddressList->RemoveRow(item);
	fAddressList->UnlockLooper();
	
	item->RootProperty()[item->IDStr().String()] = 
	 BinderNode::property::undefined;
	
	delete item;
	item = NULL;
}

status_t MagicBookContentInstance::GetSize(int32 *x, int32 *y, uint32 * outResizeFlags)
{
	*x = 642; 	// Doesn't really matter, but a rough aproximation
	*y = 350;	// Doesn't really matter	
	*outResizeFlags = STRETCH_VERTICAL | STRETCH_HORIZONTAL;
	return B_OK;
} 

status_t MagicBookContentInstance::FrameChanged(BRect newFrame, int32 fullWidth, int32 fullHeight)
{
	if (fAddressList)
	{	
		if (fAddressList->Window())	
		{
			if (fAddressList->Window()->Lock())
			{
				fAddressList->ResizeTo(newFrame.Width(), newFrame.Height());
	
				BPoint newLoc = newFrame.LeftTop();
				BView *parent = fAddressList->Parent();
				if (parent)
					newLoc -= parent->Bounds().LeftTop();
				fAddressList->MoveTo(newLoc);
				fAddressList->Window()->Unlock();
			}
		}
	}		

	return ContentInstance::FrameChanged(newFrame, fullWidth, fullHeight);
}

status_t MagicBookContentInstance::HandleMessage(BMessage *message)
{
	status_t  result = B_OK;
	
	switch (message->what)
	{
		case	kEndEditMessage:  // Edit mode is over
				NotifyListeners(B_PROPERTY_CHANGED,"editMode");
				DeleteIfBrandNew(message);
				#ifdef DEBUG_OUTPUT
				printf("Contact edit mode ended\n");
				#endif
				break;
		case	kInvokeMessage:
				NotifyListeners(B_PROPERTY_CHANGED,"addressStatus");
				break;
		case	kSelectionMessage:
				NotifyListeners(B_PROPERTY_CHANGED,"selectionCount");
				break;		
		default:
				result = BinderNode::HandleMessage(message);
				break;
	}
	return result;
}

void MagicBookContentInstance::Cleanup()
{
	BinderNode::Cleanup();
	ContentInstance::Cleanup();
}

MagicBookContent::MagicBookContent(void *handle, const char *mime)
	:	Content(handle),
		fMimeType(mime)
{
}

MagicBookContent::~MagicBookContent()
{

}

size_t MagicBookContent::GetMemoryUsage()
{
	return 0;
}

ssize_t MagicBookContent::Feed(const void */*buffer*/, ssize_t /*count*/, bool /*done*/)
{
	return 0;
}

status_t MagicBookContent::CreateInstance(ContentInstance **outInstance, GHandler *handler, const BMessage &attributes)
{
	*outInstance = new MagicBookContentInstance(this, handler, fMimeType, attributes);
	return B_OK;
}

class MagicBookContentFactory : public ContentFactory
{
	public:
		void GetIdentifiers(BMessage *into)
		{
			into->AddString(S_CONTENT_MIME_TYPES, kContentSignature);
		}
	
		Content* CreateContent(void *handle, const char *mime, const char *extension)
		{
			(void)mime;
			(void)extension;
			return new MagicBookContent(handle, mime);
		}
};

extern "C" _EXPORT ContentFactory* make_nth_content(int32 n, image_id, uint32, ...)
{
	if (n == 0) return new MagicBookContentFactory;
	return 0;
}


