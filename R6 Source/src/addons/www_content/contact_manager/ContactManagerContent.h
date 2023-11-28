/*
	ContactManagerContent.h
*/

#ifndef _MagicBookContent_h
#define _MagicBookContent_h

#include <Binder.h>
#include <Content.h>
#include <experimental/ResourceSet.h>
#include <Locker.h>
#include "TListView.h"

using namespace Wagner;
using namespace BExperimental;

class MagicBookContent : public Content
{
	public:
								MagicBookContent(void *handle, const char *mime);
		virtual 				~MagicBookContent(void);

		virtual size_t 			GetMemoryUsage(void);
		virtual ssize_t 		Feed(const void *buffer, ssize_t bufferLen, bool done=false);

	private:

		virtual status_t 		CreateInstance(ContentInstance **outInstance, GHandler *handler, const BMessage &attributes);
		BString fMimeType;
};

class MagicBookContentInstance : public ContentInstance, public BinderNode
{
	public:
								MagicBookContentInstance(MagicBookContent *parent, GHandler *h, const BString &mime, const BMessage &attributes);
		virtual 				~MagicBookContentInstance();

		status_t				OpenProperties(void **cookie, void *copyCookie);
		status_t				NextProperty(void *cookie, char *nameBuffer, int32 *length);
		status_t				CloseProperties(void *cookie);

		get_status_t			ReadProperty(const char *name, property &outProperty, const property_list &inArgs);

		virtual status_t 		AttachedToView(BView *view, uint32 *contentFlags);
		virtual status_t 		DetachedFromView();
		virtual	status_t 		GetSize(int32 *x, int32 *y, uint32 *outResizeFlags);
		virtual status_t 		FrameChanged(BRect newFrame, int32 fullWidth, int32 fullHeight);
		virtual	status_t		HandleMessage(BMessage *message);
		virtual void			Cleanup(void);

	private:
	
		status_t 				CreateEntry(property& out, const property_list& inArgs);
		void					DeleteIfBrandNew(const BMessage* msg);
		bool					DuplicateEntry(const property_list& inArgs);
		
		BinderNode::property	 fBinderRoot;	
		TListView 				*fAddressList;
		const char				*sortcolumn;
};

#endif                /* End of MagicBookContent.h */

