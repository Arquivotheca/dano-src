/*
	MerlinContent.h
*/
#ifndef _MERLINCONTENT_H
#define _MERLINCONTENT_H
#include <Binder.h>
#include <Content.h>
#include <experimental/ResourceSet.h>
#include <IMAP.h>
#include <Locker.h>
#include <MimeMessage.h>
#include "MailListView.h"
#include "MailListObserver.h"
#include "CEditorView.h"

using namespace Wagner;
using namespace BExperimental;

static const char *kContentListViewSignature = "application/x-vnd.Be.merlin-listView";
static const char *kContentComposeSignature = "application/x-vnd.Be.merlin-composeEditor";

enum content_mode {
	kUnknownMode = -1,
	kListMode = 0,
	kComposeMode = 1,
	kReplyMode = 2,
	kForwardMode = 3,
	kDraftsMode = 4
};

enum {
	kSelectMailbox = 'slec',
	kBuildCompose = 'refw',
	kSendMessage = 'send'
};

class MerlinContent : public Content
{
	public:
								MerlinContent(void *handle, const char *mime);

		virtual size_t 			GetMemoryUsage();
		virtual ssize_t 		Feed(const void *buffer, ssize_t bufferLen, bool done=false);

	protected:
		virtual 				~MerlinContent();

	private:
		virtual status_t 		CreateInstance(ContentInstance **outInstance, GHandler *handler, const BMessage &attributes);

		BString fMimeType;
};

class MerlinContentInstance : public ContentInstance, public BinderNode
{
	public:
								MerlinContentInstance(MerlinContent *parent, GHandler *h, const BString &mime, const BMessage &attributes);
		virtual					~MerlinContentInstance();
						
		virtual status_t 		AttachedToView(BView *view, uint32 *contentFlags);
		virtual status_t 		DetachedFromView();
		virtual	status_t 		GetSize(int32 *x, int32 *y, uint32 *outResizeFlags);
		virtual status_t 		FrameChanged(BRect newFrame, int32 fullWidth, int32 fullHeight);
		virtual void			Cleanup();
		virtual	status_t		HandleMessage(BMessage *message);
		
								// Binder virtuals
		virtual put_status_t	WriteProperty(const char *name, const property &inProperty);
		virtual get_status_t	ReadProperty(const char *name, property &outProperty, const property_list &inArgs = empty_arg_list);

		const char *			GetLoadedMailbox() const;

	protected: 
								// LiveConnect functions calls available in list mode
		virtual void			LC_SelectMailbox(property &outProperty, const property_list &inArgs);
		virtual void			LC_GetHeaderInfo(property &outProperty, const property_list &inArgs);
		virtual void			LC_OpenNext(property &outProperty, const property_list &inArgs, int32 delta);
		virtual void			LC_DeleteMessage(property &outProperty, const property_list &inArgs);
		virtual void			LC_DeleteSelectedMessages(property &outProperty, const property_list &inArgs);
		virtual void			LC_ExpungeMailbox(property &outProperty, const property_list &inArgs);

								// LiveConnect functions calls available in editor mode
		virtual void			LC_SendMessage(property &outProperty, const property_list &inArgs);
		virtual void			LC_SaveMessage(property &outProperty, const property_list &inArgs, bool saveToState);
		virtual void			LC_SetSelectionStyle(property &outProperty, const property_list &inArgs);
		virtual void			LC_SetSelectionSize(property &outProperty, const property_list &inArgs);
		virtual void			LC_SetSelectionColor(property &outProperty, const property_list &inArgs);
		virtual void			LC_ClearAll(property &outProperty, const property_list &inArgs);
		virtual void			LC_UpdateAttachmentList(property &outProperty, const property_list &inArgs, bool addFile);
		virtual void			LC_RemoveAllAttachments(property &outProperty, const property_list &inArgs);

	private:

		status_t				SelectMailboxWorker(const char *mailbox);
		status_t				SyncListView(uint32 event);
		status_t				BuildComposeWorker(const char *mailbox, const char *uid, int32 thePoliceMan);

		void					GetIndexInfo(BString &uid, int32 &current);
		void					GetParsedDate(const char *rawDate, BString &outDate, const char *format);
		status_t				ParseRecipients(BMessage *args, const char *recipients, StringBuffer *problem);
		void					BuildAttachmentList(atom<BinderContainer> &node, const MimeMessage *message, const MessagePart *part, int32 &propCount);
		
		BMessage fEmbedAttributes;
		BView *fCurrentView;
		MailListView *fMailList;
		MailListObserver *fMailListObserver;
		CEditorView *fEditorView;
		BString fMailbox;
		BString fUsername;
		BString fDraftUid;
		content_mode fContentMode;
		volatile int32 fComposeMasterKey;
		volatile int32 fBusyCount;
		volatile bool fSending;
		BLocker fTextViewLocker;

		BinderNode::property fStateNode;
};

#endif

/* End of MerlinContent.h */
