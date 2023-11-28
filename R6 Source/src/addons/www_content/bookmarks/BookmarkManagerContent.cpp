// BookmarkManagerContent.cpp -- BeIA bookmarks by Tinic Uro, additional
// coding by Allen Brunson

#include <Autolock.h>
#include <ContentView.h>
#include <ScrollBar.h>
#include <stdio.h>
#include <Application.h>
#include <FindDirectory.h>
#include <Window.h>
#include "BookmarkManagerContent.h"

// #define DEBUG_OUTPUT   // Un-comment for printf() debug output

static const bigtime_t kSaveTimeout = 10*1000000LL;
static const uint32 kInvokeMessage = 'INVO';
static const uint32 kEditEndMessage = 'ENDe';
static const uint32 kSelectionMessage = 'SELE';
static const int32 kLeftOffset = -2;
static const int32 kRightOffset = 10;

int32 kPropertyCount = 12;
static const char *kPropertyList [] = {
	"CreateBookmarkFunc",
	"CreateFolderFunc",
	"DeleteSelectedEntriesFunc",
	"editMode",
	"EditSelectedEntryFunc",
	"GetFoldersFunc",
	"GoToFunc",
	"MoveSelectedBookmarks",
	"PurgeDeletedFunc",
	"selectionBookmarksCount",
	"selectionCount",
	"selectionHasFullFolders"
};

// Do not depend on this structure only, there is
// hardcoded stuff in THIS file too...
static struct t_column_info cinfo [] = 
{
 {COLUMN_ICON, "", 32, 0, "favorite.png", "favorite.png", "favorite.png", "favorite.png", 0, 0 },
 {COLUMN_STRINGFIELD, "Title", 220, "title", {0}, {0}, {0}, {0}, 0, 0 },
 {COLUMN_STRINGFIELD, "URL", 220, "URL", {0}, {0}, {0}, {0}, 0, 0 },
 {COLUMN_COUNTERFIELD, "", 24, "", {0}, {0}, {0}, {0}, 0, 0},
 {COLUMN_BITMAPBUTTON, "", 60, 0, "cancel_edit_up.png", "cancel_edit_over.png", "cancel_edit_down.png", {0}, 'CANC', 0},
 {COLUMN_BITMAPBUTTON, "", 60, 0, "approve_edit_up.png", "approve_edit_over.png", "approve_edit_down.png", {0}, 'OKAY', 0},
 {COLUMN_END, {0}, 0, {0}, {0}, {0}, {0}, {0}, 0, 0}
};

BResourceSet& Resources()
{
	static BResourceSet gResources;
	static bool gResInitialized = false;
	if( gResInitialized )
		return gResources;
	gResources.AddEnvDirectory("${/service/web/macros/RESOURCES}/Bookmarks/images", "/boot/custom/resources/en/Bookmarks/images");
	gResInitialized = true;
	return gResources;
}

static void UpdateFolderIcon(TListItem *item)
{
	TButtonField *field = (TButtonField *)item->GetField(0);
	field->isFolder = true;
	field->SetStatic(item->IsExpanded() ? "folder_open.png" : "folder_closed.png");
}

class BookmarkListView : public TListView
{
	public:
					BookmarkListView(BinderNode::property &root, BRect frame, const BMessage &parameters, t_column_info *cinfo):TListView(root, frame, parameters, cinfo)
					{
						fRootProp = BinderNode::Root() ["user"] ["~"] ["bookmarks"];
					}

		void		MessageDropped(BMessage *msg, BPoint point)
					{
						if(LockLooper())
						{
							TListItem *src;
							if(msg->FindPointer("row",(void **)&src)==B_OK)
							{
								if(src)
								{
									TListItem *dst = (TListItem *)RowAt(point);
									HandleDragDrop(dst,src);
								}
							}
							UnlockLooper();
						}
					}

		void		HandleDragDrop(TListItem *dst, TListItem *src)
					{
						if(src)
						{
							DeselectAll();
							
							if(dst && !dst->Expandable()) {
								bool isVisible=0;
								if(!FindParent(dst,(BRow **)&dst,&isVisible))
									dst = 0;
							}
								
							// We only support dragging URLs into folders
							if((!dst || dst->Expandable()) && !src->Expandable())
							{
								// Get parent item of source
								// We need that to delete the property.
								bool isVisible=0;
								TListItem *srcparent=0;
								if(!FindParent(src,(BRow **)&srcparent,&isVisible))
									srcparent = 0;
								if(srcparent != dst)
								{
									BinderNode::property srcprop = src->Property();
									if(srcprop.IsObject())
									{
										BinderNode::property newprop = srcprop->Copy(true);
										if(newprop.IsObject())
										{
											BString bidstr = src->IDStr();
											const char *idstr = bidstr.LockBuffer(bidstr.Length()+1);
										
											// Place item in new node position
											TListItem *item = 0;
											if(dst) {
												dst->Property() [idstr] = newprop;
												item = new TListItem(this,fRootProp,idstr,cinfo);
											}
											else {
												fRootProp [idstr] = newprop;
												item = new TListItem(this,fRootProp,idstr,cinfo);
											}
											AddRow(item,dst);
							
											// Reset intendation
											TButtonField *field = (TButtonField *)item->GetField(0);
											if(dst)
												field->SetOffset(BPoint(kRightOffset,0));
											else
												field->SetOffset(BPoint(kLeftOffset,0));
								
											// Remove the row itself
											RemoveRow(src);
											delete src;
										
											// Now delete BinderContainer entry
											if(srcparent)
												srcparent->Property() [idstr] = BinderNode::property::undefined;
											else
												fRootProp [idstr] = BinderNode::property::undefined;
		
											bidstr.UnlockBuffer();
											AddToSelection(item);
											SetFocusRow(item);
										}
									}
								}
							}
						}
					}
					
	private:

		BinderNode::property fRootProp;	
};

// -----------------------------------------------------------------------------------------
//									BookmarkContentInstance
// -----------------------------------------------------------------------------------------

BookmarkContentInstance::BookmarkContentInstance(BookmarkContent *parent, GHandler *h, const BString &/*mime*/, const BMessage &attributes):ContentInstance(parent, h)
{
	fBinderRoot = BinderNode::Root() ["user"] ["~"] ["bookmarks"];
	
	fBookmarkList = new BookmarkListView(fBinderRoot,
	 BRect(0, 0, 600, 350),attributes,cinfo);
	
	fBookmarkList->SetHandler(this,kInvokeMessage,kEditEndMessage);
	fBookmarkList->SetSelectionMessage(new BMessage(kSelectionMessage));
	
	fBookmarkList->SetDragable();
	fBookmarkList->SetLatchWidth(0);
}

BookmarkContentInstance::~BookmarkContentInstance()
{
}

void BookmarkContentInstance::CorrectItems()
{
	for(int32 c=0;c<fBookmarkList->CountRows();c++)
	{
		TListItem *item = dynamic_cast<TListItem *>(fBookmarkList->RowAt(c));
		if(item)
		{
			{
				TButtonField *field = (TButtonField *)item->GetField(0);
				if(field) field->SetOffset(BPoint(kLeftOffset,0));
			}
			BinderNode::property prop = item->Property()["isFolder"];
			if(prop.IsNumber() && prop.Number()==1)
			{
				{
					TStringField *field = (TStringField *)item->GetField(2);
					if(field) field->SetEditable(false);
				}
				item->SetExpandable();
				UpdateFolderIcon(item);
				for (int32 d=0;d<fBookmarkList->CountRows(item);d++)
				{
					TListItem *child = dynamic_cast<TListItem*>(fBookmarkList->RowAt(d,item));
					if(child)
					{
						TButtonField *field = (TButtonField *)child->GetField(0);
						if(field) field->SetOffset(BPoint(kRightOffset,0));
					}
				}
			}
		}
	}
}

status_t BookmarkContentInstance::OpenProperties(void **cookie, void *copyCookie)
{
	int32 *index = new int32;
	*index = 0;
	if (copyCookie)
		*index = *((int32 *)copyCookie);
	*cookie = index;
	return B_OK;
}

status_t BookmarkContentInstance::NextProperty(void *cookie, char *nameBuffer, int32 *length)
{
	int32 *index = (int32 *)cookie;
	if (*index >= kPropertyCount)
		return ENOENT;
	const char *item = kPropertyList[(*index)++];
	strncpy(nameBuffer, item, *length);
	*length = strlen(item);
	return B_OK;
}

status_t BookmarkContentInstance::CloseProperties(void *cookie)
{
	int32 *index = (int32 *)cookie;
	delete index;
	return B_OK;
}


/****************************************************************************/
/*                                                                          */
/***  BookmarkContentInstance::pf_GetFoldersFunc()                        ***/
/*                                                                          */
/****************************************************************************

This function gets the name of the Nth folder.                              */

get_status_t BookmarkContentInstance::             // Begin pf_GetFolders..()
 pf_GetFoldersFunc(property& outProperty,
 const property_list& inArgs)
  {
    int32        count;                            // Total items
    int32        i, j;                             // Loop counters
    TListItem*   item = NULL;                      // Next item from list
    int32        num;                              // Number caller wants
    
    num = (int32) inArgs[0].Number();              // Get first argument
    count = fBookmarkList->CountRows();            // Total item count
			  
    if (num < 0) num = 0;                          // Not too small
    if (num > 16000) num = 16000;                  // And not too big
			  
	BinderNode::property prop;
    for (i = 0, j = 0; i < count; i++)             // Loop to find item
      {
        item = (TListItem*)fBookmarkList->RowAt(i);// Get next row item
        if (!item) break;                          // Stop at list end

		prop = item->Property() ["isFolder"];
		if(!prop.IsNumber() || prop.Number()==0)
			continue;
		prop = BinderNode::property::undefined;
			      
        if (num == j)                              // If it's the one wanted
          {
			prop = item->Property() ["title"];
            break;                                 // Stop the loop
          }
			        
        j++;                                       // To next folder
      }
			    
    if (prop.IsString())                           // If we found a name
      outProperty = property(prop);       		   // Use it
    else                                           // Didn't find a name
      outProperty = property::undefined;           // Nothing there
			  
    return B_OK;                                   // Success
  }                                                // End pf_GetFolders..()


/****************************************************************************/
/*                                                                          */
/***  BookmarkContentInstance::pf_MoveSelectedBookmarks()                 ***/
/*                                                                          */
/****************************************************************************

This function moves all currently selected bookmarks to the named folder
given.  If the folder name isn't given then the bookmarks are moved to
the root level.                                                             */

get_status_t BookmarkContentInstance::             // Begin pf_MoveSelected..()
 pf_MoveSelectedBookmarks(property& /*outProperty*/,
 const property_list& /*inArgs*/)
  {
  
    /*
    RemoveRow(src);
    
      {
        TButtonField *field = (TButtonField *)src->GetField(0);
        field->SetOffset(BPoint(!dst ? kLeftOffset : kRightOffset,0));
      }
	
    AddRow(src,dst);
    _root->MarkDirty();
    */
    
    return B_OK;                                   // Success
  }                                                // End pf_MoveSelected..()


/****************************************************************************/
/*                                                                          */
/***  BookmarkContentInstance::pf_selectionBookmarksCount()               ***/
/*                                                                          */
/****************************************************************************

This function returns the number of currently selected *bookmarks*, not
including folders.  It must take into account the fact that if a folder is
selected then the items it contains are "selected," in a sense.             */

get_status_t BookmarkContentInstance::             // Begin pf_selection..()
 pf_selectionBookmarksCount(property& outProperty,
 const property_list& inArgs)
  {
    bool                  isFolder;                // Is this one a folder?
    TListItem*            item = NULL;             // Next item from list
	BinderNode::property  prop;                    // Property to loop with
    int32                 total;                   // Total to return
    
    (void)inArgs;                                  // Parameter not used
	
    for (total = 0; ; )                            // Loop for selected
      {
        item = (TListItem*) fBookmarkList->CurrentSelection(item);
        if (!item) break;                          // Exit at list end
				    
		prop = item->Property() ["isFolder"];      // Get folder property
        isFolder = (prop.IsNumber() && prop.Number() >= 1);
        
        if (isFolder)                              // A folder
          total += fBookmarkList->CountRows(item); // Add all children
        else                                       // Not a folder
          total++;                                 // One more selected
      }
				
    outProperty = property((int)total);            // That's how many
    return B_OK;                                   // Success
  }                                                // End pf_selection..()


/****************************************************************************/
/*                                                                          */
/***  BookmarkContentInstance::pf_selectionHasFullFolders()               ***/
/*                                                                          */
/****************************************************************************

This function returns true if the current selection contains folders that
contain bookmarks, false if not.                                            */

get_status_t BookmarkContentInstance::             // Begin pf_selection..()
 pf_selectionHasFullFolders(property& outProperty,
 const property_list& inArgs)
  {
    bool                  isFolder;                // Is this one a folder?
    TListItem*            item = NULL;             // Next item from list
	BinderNode::property  prop;                    // Property to loop with
	bool                  rval = false;            // Value to return
    
    (void)inArgs;                                  // Parameter not used
	
    while (true)                                   // Loop for selected
      {
        item = (TListItem*) fBookmarkList->CurrentSelection(item);
        if (!item) break;                          // Exit at list end
				    
		prop = item->Property() ["isFolder"];      // Get folder property
        isFolder = (prop.IsNumber() && prop.Number() >= 1);
        
        if (!isFolder) continue;                   // If not a folder
        
        if (fBookmarkList->CountRows(item) >= 1)   // If there's children
          {
            rval = true;                           // There's yer answer
            break;                                 // Stop the loop
          }
      }
				
    if (!rval)                                     // Nope
      outProperty = property("false");             // Return this string
    else                                           // Yep
      outProperty = property("true");              // Return this string
      
    return B_OK;                                   // Success
  }                                                // End pf_selection..()


// Vanilla dispatcher for our functions
get_status_t BookmarkContentInstance::ReadProperty(const char *name, property &outProperty, const property_list &inArgs)
{
	get_status_t  rval;
	
	if(strcmp(name,"editMode")==0)
    {
    	char  str[10];
    	
		if (!fBookmarkList->LockLooper()) return B_ERROR;
		
    	if (fBookmarkList->InEditMode())
    	  strcpy(str, "true");
    	else
    	  strcpy(str, "false");  
    	
		outProperty = property(str);
		fBookmarkList->UnlockLooper();
		return B_OK;
    }
			
	if(!fBookmarkList->InEditMode())
	{
		if(fBookmarkList->LockLooper())
		{
			if(strcmp(name,"CreateBookmarkFunc")==0)
			{
				BString title = inArgs[0].String();
				BString url   = inArgs[1].String();
				
				if (DuplicateEntry(title.String(), url.String()))
				  {
				    outProperty = property("false");
				    goto createEnd;
				  }
				
				CreateBookmark(title.String(), url.String());
				
				createEnd:
				fBookmarkList->UnlockLooper();
				return B_OK;
			}
			else if(strcmp(name,"CreateFolderFunc")==0)
			{
				BString title = inArgs[0].String();

				#ifdef DEBUG_OUTPUT
				puts(title.String());
				#endif

				CreateFolder(title.LockBuffer(title.Length()+1));
				title.UnlockBuffer();
				fBookmarkList->UnlockLooper();
				return B_OK;
			}
			else if(strcmp(name,"DeleteSelectedEntriesFunc")==0)
			{
				DeleteSelectedEntries();
				fBookmarkList->UnlockLooper();
				return B_OK;
			}
			else if(strcmp(name,"EditSelectedEntryFunc")==0)
			{
				fBookmarkList->BeginEdit();
				fBookmarkList->UnlockLooper();
				NotifyListeners(B_PROPERTY_CHANGED,"editMode");
				
				#ifdef DEBUG_OUTPUT
				printf("Starting bookmark edit mode\n");
				#endif
				
	 			return B_OK;
			}
			else if(strcmp(name,"GetFoldersFunc")==0)
			{
				rval = pf_GetFoldersFunc(outProperty, inArgs);
				fBookmarkList->UnlockLooper();
	 			return rval;
			}
			else if(strcmp(name,"GoToFunc")==0)
			{
				HandleInvoke();
			}
			else if(strcmp(name,"MoveSelectedBookmarks")==0)
			{
			    rval = pf_MoveSelectedBookmarks(outProperty, inArgs);
				fBookmarkList->UnlockLooper();
				return rval;
			}
			else if(strcmp(name,"PurgeDeletedFunc")==0)
			{
				PurgeDeleted();
				fBookmarkList->UnlockLooper();
				return B_OK;
			}
			else if(strcmp(name,"selectionCount")==0)
			{
				int32 count=0;
				for(TListItem *item=0;(item = (TListItem *)fBookmarkList->CurrentSelection(item))!=0;count++) {}
				
				outProperty = property((int)count);
				fBookmarkList->UnlockLooper();
				return B_OK;
			}
			else if(strcmp(name,"selectionBookmarksCount")==0)
			{
			  rval = pf_selectionBookmarksCount(outProperty, inArgs);
			   
			  fBookmarkList->UnlockLooper();
			  return rval; 
			}
			else if (strcmp(name,"selectionHasFullFolders")==0)
			{
			  rval = pf_selectionHasFullFolders(outProperty, inArgs);
			  
			  fBookmarkList->UnlockLooper();
			  return rval;
			}
			
			fBookmarkList->UnlockLooper();
		}
	}
	return BinderNode::ReadProperty(name,outProperty,inArgs);
}

void BookmarkContentInstance::CreateBookmark(const char *title, const char *url)
{
	if(fBookmarkList->LockLooper())
	{
		// Create unique ID, sort of 
		static int32 idcnt = 0;
		int id = (time(NULL)&0xFFFFFF00) + (idcnt++);
		char idstr[16];
		sprintf(idstr, "B%08x", id);

		// Add bookmark
		BinderNode::property newprop;
		fBinderRoot->GetProperty("+bookmark",newprop);
		if(newprop.IsObject())
		{
			puts("Add bookmark");
			fBinderRoot [idstr] = newprop;

			newprop ["title"] = title;
			newprop ["URL"] = url;
			newprop ["isSecure"] = (int)false;
			newprop ["isDeletable"] = (int)true;

			TListItem *item = 0;
			fBookmarkList->DeselectAll();
			fBookmarkList->AddRow(item = new TListItem(fBookmarkList,fBinderRoot,idstr,cinfo));
			TButtonField *field = (TButtonField *)item->GetField(0);
			field->SetOffset(BPoint(kLeftOffset,0));

			// Select new item
			fBookmarkList->AddToSelection(item);
			fBookmarkList->SetFocusRow(item);
			fBookmarkList->SelectionChanged();
		}
		fBookmarkList->UnlockLooper();
	}
}

// This only allows creating folders on
// the first level like specified in
// the spec.
void BookmarkContentInstance::CreateFolder(const char *title)
{
	if(fBookmarkList->LockLooper())
	{
		// Create unique ID, sort of 
		static int32 idcnt = 0;
		int id = (time(NULL)&0xFFFFFF00) + (idcnt++);
		char idstr[16];
		sprintf(idstr, "F%08x", id);

		// This is probaly the slowest way to do it...
		char realtitle[284];
		sprintf(realtitle,"%s",title);

		if(strlen(title)<256)
		{
			for(int number=0;;)
			{
				TListItem *item = 0;
				bool found = false;
				for(int32 c=0;(item=(TListItem *)fBookmarkList->RowAt(c))!=0;c++)
				{
					BinderNode::property prop;
					prop = item->Property() ["title"];
					if(prop.IsString())
						if(prop.String() == realtitle)
							found = true;
				}
				if(!found)
					break;
				sprintf(realtitle,"%s %d",title,++number);
			}
		}

		// Add bookmark
		BinderNode::property newprop;
		fBinderRoot->GetProperty("+bookmark",newprop);
		if(newprop.IsObject())
		{
			fBinderRoot [idstr] = newprop;
			newprop ["title"] = title;
			newprop ["isFolder"] = (int)true;
			newprop ["isSecure"] = (int)false;
			newprop ["isDeletable"] = (int)true;

			TListItem *item = 0;
			fBookmarkList->DeselectAll();
			fBookmarkList->AddRow(item = new TListItem(fBookmarkList,fBinderRoot,idstr,cinfo));
		
			{
				TButtonField *field = (TButtonField *)item->GetField(0);
				field->SetOffset(BPoint(kLeftOffset,0));
			}
			// Folder have no editable URL
			{
				TStringField *field = (TStringField *)item->GetField(2);
				field->SetEditable(false);
			}
		
			item->SetExpandable();
			UpdateFolderIcon(item);

			// Select new item
			fBookmarkList->AddToSelection(item);
			fBookmarkList->SetFocusRow(item);
			fBookmarkList->SelectionChanged();
		}
		fBookmarkList->UnlockLooper();
	}
}

bool BookmarkContentInstance::DuplicateEntry(const char* title,
 const char* url)
{
	uint32                count = fBookmarkList->CountRows();
	uint32                i;
	TListItem*            item;
	BinderNode::property  prop;
	BString               str;
	
	for (i = 0; i < count; i++)
	  {
	    item = (TListItem*) fBookmarkList->RowAt(i);
	    if (!item) {ASSERT(false); continue;}
	    
		prop = item->Property() ["isFolder"];
		
		if (prop.IsNumber() && prop.Number() != 0)  // If it's a folder
			continue;                               // Ignore it
				    
	    str = item->Property() ["title"].String().String();
	    if (strcasecmp(str.String(), title)) continue;
	    
	    str = item->Property() ["URL"].String().String();
	    if (strcasecmp(str.String(), url)) continue;
	  
	    return true;   // Duplicate found!
	  }
	
	return false;    // All checked, no duplicates
}

void BookmarkContentInstance::HandleInvoke()
{
	if(fBookmarkList->LockLooper())
	{
		// We only take into account the first selected item
		// This might have weird consequences
		TListItem *item = (TListItem *)fBookmarkList->CurrentSelection(0);
		if(item)
		{
			// If folder, expand or collapse
			if(item->Expandable())
			{
				fBookmarkList->ExpandOrCollapse(item,!item->IsExpanded());
				UpdateFolderIcon(item);
			}
			// if URL tell application to go to new URL
			else
			{
				BinderNode::property prop;
				prop = item->Property() ["URL"];
				if(prop.IsString())
				{
					BString urlstr = prop.String();
					if(urlstr.Length()>0)
					{
						bool issecure = false;	

						BinderNode::property prop;
						prop = item->Property() ["isSecure"];
						if(prop.IsNumber())
							issecure = prop.Number();

						OpenURL(urlstr.LockBuffer(urlstr.Length()+1),issecure);
						urlstr.UnlockBuffer();
					}
				}
			}
		}
		fBookmarkList->UnlockLooper();
	}
}

void BookmarkContentInstance::DeleteSelectedEntries()
{
	for(TListItem *row=0;(row=(TListItem *)fBookmarkList->CurrentSelection(row))!=0;)
	{
		bool isDeletable = false;	

		BinderNode::property prop;
		prop = row->Property() ["isDeletable"];
		if(prop.IsNumber() && prop.Number()==1)
			isDeletable = prop.Number();
		else if(!prop.IsNumber())
			isDeletable = true;
			
	
		if(!isDeletable)
			continue;
			
		row->Tag();
	}
}

void BookmarkContentInstance::PurgeDeleted(BRow *parent, bool forcetagged)
{
  	for(int32 c=0;c<fBookmarkList->CountRows(parent);c++)	
	{
		TListItem *item = (TListItem *)fBookmarkList->RowAt(c,parent);
		PurgeDeleted(item,item->IsTagged());
		if(item->IsTagged() || forcetagged)
		{
			fBookmarkList->RemoveRow(item);
			item->ParentProperty()[item->IDStr().String()] = BinderNode::property::undefined;
			delete item;
			c--;
		}
	}
	
	fBookmarkList->Invalidate();  // This will update the scrollbar
}

void BookmarkContentInstance::OpenURL(const char *url, bool is_secure)
{
	BMessage openURL('ourl');
	openURL.AddString("url", url);
	openURL.AddString("caller", "bookmarks");
	if (is_secure) openURL.AddInt32("security_zone", 0);
	be_app->PostMessage(&openURL);
}

status_t BookmarkContentInstance::AttachedToView(BView *inView, uint32 */*contentFlags*/)
{
	if (fBookmarkList)
	{
		inView->AddChild(fBookmarkList);
		BRect rect = FrameInParent();
		fBookmarkList->MoveTo(rect.LeftTop());
		fBookmarkList->ResizeTo(rect.Width(), rect.Height());	
		CorrectItems();
	}
	return B_OK;
}

status_t BookmarkContentInstance::DetachedFromView()
{
	if (fBookmarkList)
	{
		fBookmarkList->RemoveSelf();
		delete fBookmarkList;
		fBookmarkList = 0;
	}
	return B_OK;
}

status_t BookmarkContentInstance::GetSize(int32 *x, int32 *y, uint32 * outResizeFlags)
{
	*x = 632;
	*y = 300;	
	*outResizeFlags = STRETCH_VERTICAL | STRETCH_HORIZONTAL;
	return B_OK;
} 

status_t BookmarkContentInstance::FrameChanged(BRect newFrame, int32 fullWidth, int32 fullHeight)
{
	if (fBookmarkList)
	{	
		if (fBookmarkList->Window())	
		{
			if (fBookmarkList->Window()->Lock())
			{
				fBookmarkList->ResizeTo(newFrame.Width(), newFrame.Height());
				BPoint newLoc = newFrame.LeftTop();
				BView *parent = fBookmarkList->Parent();
				if (parent)
					newLoc -= parent->Bounds().LeftTop();
				fBookmarkList->MoveTo(newLoc);
				fBookmarkList->Window()->Unlock();
			}
			return ContentInstance::FrameChanged(newFrame, fullWidth, fullHeight);
		}
	}		
	return B_ERROR;
}

status_t BookmarkContentInstance::HandleMessage(BMessage *message)
{
	status_t result = B_OK;
	switch (message->what)
	{
		case	kEditEndMessage:  // Edit mode is over
				NotifyListeners(B_PROPERTY_CHANGED,"editMode");
				#ifdef DEBUG_OUTPUT
				printf("Ending bookmark edit mode\n");
				#endif
				break;
		case	kInvokeMessage:
				HandleInvoke();
				break;
		case	kSelectionMessage:
				NotifyListeners(B_PROPERTY_CHANGED,"selectionCount");
				NotifyListeners(B_PROPERTY_CHANGED,"selectionBookmarksCount");
				break;		
		default:
				result = BinderNode::HandleMessage(message);
				break;
	}
	return result;
}

void BookmarkContentInstance::Cleanup()
{
	BinderNode::Cleanup();
	ContentInstance::Cleanup();
}

// -----------------------------------------------------------------------------------------
//										BookmarkContent
// -----------------------------------------------------------------------------------------

BookmarkContent::BookmarkContent(void *handle, const char *mime)
	:	Content(handle),
		fMimeType(mime)
{
}

BookmarkContent::~BookmarkContent()
{

}

size_t BookmarkContent::GetMemoryUsage()
{
	return 0;
}

ssize_t BookmarkContent::Feed(const void */*buffer*/, ssize_t /*count*/, bool /*done*/)
{
	return 0;
}

status_t BookmarkContent::CreateInstance(ContentInstance **outInstance, GHandler *handler, const BMessage &attributes)
{
	*outInstance = new BookmarkContentInstance(this, handler, fMimeType, attributes);
	return B_OK;
}

// -----------------------------------------------------------------------------------------
//										BookmarkContentFactory
// -----------------------------------------------------------------------------------------

class BookmarkContentFactory : public ContentFactory
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
			return new BookmarkContent(handle, mime);
		}
};

extern "C" _EXPORT ContentFactory* make_nth_content(int32 n, image_id, uint32, ...)
{
	if (n == 0) return new BookmarkContentFactory;
	return 0;
}
