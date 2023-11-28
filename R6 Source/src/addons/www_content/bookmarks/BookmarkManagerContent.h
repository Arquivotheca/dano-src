// BookmarkManagerContent.h -- BeIA bookmarks
// by Tinic Uro, additional coding by Allen Brunson

#ifndef _BookmarkManagerContent_h
#define _BookmarkManagerContent_h

#include <Binder.h>
#include <Content.h>
#include <experimental/ResourceSet.h>
#include <Locker.h>
#include "TListView.h"

using namespace Wagner;
using namespace BExperimental;

static const char *kContentSignature = "text/x-vnd.Be.Bookmarks";

class BookmarkContent : public Content
{
	public:
								BookmarkContent(void *handle, const char *mime);
		virtual 				~BookmarkContent();

		virtual size_t 			GetMemoryUsage();
		virtual ssize_t 		Feed(const void *buffer, ssize_t bufferLen, bool done=false);

	private:
		virtual status_t 		CreateInstance(ContentInstance **outInstance, GHandler *handler, const BMessage &attributes);
		BString fMimeType;
};

class BookmarkContentInstance : public ContentInstance, public BinderNode
{
    //*
    //***  Private defines
    //*
    
    private:
    
    typedef get_status_t                           // A list of these
     (BookmarkContentInstance::*PROPERTYPROC)      //  is used to carry out
     (property& outProperty,                       //  JavaScript calls
     const property_list& inArgs);
    
    struct PROPERTYDATA                            // Info on one property
      {
        const char*   name;                        // Property name
        PROPERTYPROC  proc;                        // Procedure to call
      };
      
	public:
								BookmarkContentInstance(BookmarkContent *parent, GHandler *h, const BString &mime, const BMessage &attributes);
		virtual 				~BookmarkContentInstance();

		status_t				OpenProperties(void **cookie, void *copyCookie);
		status_t				NextProperty(void *cookie, char *nameBuffer, int32 *length);
		status_t				CloseProperties(void *cookie);

		get_status_t			ReadProperty(const char *name, property &outProperty, const property_list &inArgs);

		virtual status_t 		AttachedToView(BView *view, uint32 *contentFlags);
		virtual status_t 		DetachedFromView();
		virtual	status_t 		GetSize(int32 *x, int32 *y, uint32 *outResizeFlags);
		virtual status_t 		FrameChanged(BRect newFrame, int32 fullWidth, int32 fullHeight);
		virtual	status_t		HandleMessage(BMessage *message);
		virtual void			Cleanup();
		
				void			MarkDirty();

	private:
	
		void					CorrectItems();
		
		void					CreateBookmark(const char* title, const char* url);
		void					CreateFolder(const char *title);
		bool					DuplicateEntry(const char* title, const char* url);
		void					OpenURL(const char *url, bool is_secure);
		void					HandleInvoke();
		void					DeleteSelectedEntries();
		void					PurgeDeleted(BRow *parent = 0, bool forcetagged = false);

		void					Save();
		void					Load();

    get_status_t     pf_GetFoldersFunc(            // Get all folder names
                      property& outProperty, 
                      const property_list& inArgs);
    
    get_status_t     pf_selectionBookmarksCount(   // Selected bookmark count
                      property& outProperty, 
                      const property_list& inArgs);
                      
    get_status_t     pf_selectionHasFullFolders(   // At least one folder
                      property& outProperty,       //  that contains bookmarks
                      const property_list& inArgs);
    
    get_status_t     pf_MoveSelectedBookmarks(     // Move selected to folder
                      property& outProperty, 
                      const property_list& inArgs);


		BinderNode::property     fBinderRoot;    
		TListView 				*fBookmarkList;
		BView					*fCurrentView;
};


/****************************************************************************/
/*                                                                          */
/***  Bookmarks classes                                                   ***/
/*                                                                          */
/****************************************************************************


Stuff to do
-----------

Items that are selected need to change to some contrasting color, like
black or something.

Tinic says that people have reported that sometimes the string contents
of bookmarks and the contact manager go blank sometimes.  Probably a bug
in TListView that needs to be found and fixed.

BUG BUG BUG: Tinic only adds the "isFolder" property to folders, NOT to
bookmarks!  I need to fix this but first I must check to see if it will
break anything else.

*/

#endif                                         // End BookmarkManagerContent.h

