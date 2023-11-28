#ifndef MINITEL_CONTENT_H
#define MINITEL_CONTENT_H

#include "Content.h"
#include "Resource.h"

using namespace Wagner;


/*======================== ContentInstance ========================*/

class MinitelContentInstance : public ContentInstance
{
	public:
							MinitelContentInstance	(Content* content,
													 GHandler* handler,
													 const BMessage* msg);
							~MinitelContentInstance	();
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
		virtual	status_t	ContentNotification		(BMessage* msg);
		virtual	void		MouseDown				(BPoint where,
													 const BMessage* event = NULL);
		virtual	void		MouseUp					(BPoint where,
													 const BMessage* event = NULL);
		virtual	void		MouseMoved				(BPoint where,
													 uint32 code,
													 const BMessage* msg,
													 const BMessage* event = NULL);
		virtual	void		KeyDown					(const char* bytes,
													 int32 num_bytes,
													 const BMessage* event = NULL);
		virtual	void		KeyUp					(const char* bytes,
													 int32 num_bytes,
													 const BMessage* event = NULL);
		virtual void		SyncToState				(BMessage* msg);
		virtual status_t	UsurpPredecessor		(ContentInstance* old_instance);
		virtual void		Notification			(BMessage* msg);

		status_t			InitCheck				()
														{ return fResult; };

	private:
		int32				fHeight;
		int32				fWidth;
		status_t			fResult;
		BView*				fMinitel;
		BView*				fView;
};


/*============================ Content ============================*/

class MinitelContent : public Content
{
	public:
							MinitelContent			(void* handle);
		virtual				~MinitelContent			();
		virtual ssize_t		Feed					(const void* buffer,
													 ssize_t buffer_len,
													 bool done = false);
		virtual size_t		GetMemoryUsage			();
		virtual	bool		IsInitialized			();

		status_t			InitCheck				()
														{ return fResult; };

	private:
		virtual status_t	CreateInstance			(ContentInstance** out_instance,
													 GHandler* handler,
													 const BMessage& msg);

		status_t			fResult;
		friend class		MinitelContentInstance;
};


/*========================== Minitel view =========================*/

class Minitel : public BView
{
	public:
							Minitel				(BRect);
};
#endif	/* MINITEL_CONTENT_H */
