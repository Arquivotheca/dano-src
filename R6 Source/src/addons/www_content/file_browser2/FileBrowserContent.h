#ifndef FILE_BROWSER_CONTENT_H
#define FILE_BROWSER_CONTENT_H

#include "Binder.h"
#include "Content.h"
#include "Resource.h"
#include "FileListView.h"

using namespace Wagner;

static char const *kBinderProperties[] = { "foo" };


/*======================== ContentInstance ========================*/

class FileBrowserContentInstance : public ContentInstance,
								   public BinderNode
{
	public:
							FileBrowserContentInstance	(Content* content,
													 GHandler* handler,
													 const BMessage& msg);
							~FileBrowserContentInstance	();
		virtual	status_t	AttachedToView			(BView* view,
													 uint32* flags);
		virtual	status_t	DetachedFromView		();
		virtual status_t	Draw					(BView* into,
													 BRect exposed);
		virtual	status_t	FrameChanged			(BRect new_frame,
													 int32 full_width,
													 int32 full_height);
		virtual status_t	GetSize					(int32* width,
													 int32* height,
													 uint32* resize_flags);
		virtual	status_t	HandleMessage			(BMessage* msg);
		virtual	void		Cleanup					()
														{
															ContentInstance::Cleanup();
															BinderNode::Cleanup();
														};

		virtual	status_t 	OpenProperties			(void** cookie,
													 void* copyCookie);
		virtual	status_t 	NextProperty			(void* cookie,
													 char* nameBuf,
													 int32* len);
		virtual	status_t 	CloseProperties			(void* cookie);
		virtual	put_status_t WriteProperty			(const char* name,
													 const property& prop);
		virtual	get_status_t ReadProperty			(const char*name,
													 property& prop,
													 const property_list& args = empty_arg_list);

	private:
		int32				fHeight;
		int32				fWidth;
		BView*				fFileBrowser;
		BView*				fView;
		drawing_parameters	fParameters;
		BString				fDefaultURL;
};


/*============================ Content ============================*/

class FileBrowserContent : public Content
{
	public:
							FileBrowserContent		(void* handle);
		virtual				~FileBrowserContent		();
		virtual ssize_t		Feed					(const void* buffer,
													 ssize_t buffer_len,
													 bool done = false);
		virtual size_t		GetMemoryUsage			();
		virtual	bool		IsInitialized			();

	private:
		virtual status_t	CreateInstance			(ContentInstance** out_instance,
													 GHandler* handler,
													 const BMessage& msg);

		friend class		FileBrowserContentInstance;
};
#endif	/* FILE_BROWSER_CONTENT_H */
