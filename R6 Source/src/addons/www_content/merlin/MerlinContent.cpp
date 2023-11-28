/*
	MerlinContent.cpp
*/
#include <Autolock.h>
#include <Base64ToRawAdapter.h>
#include <ConcatenateAdapter.h>
#include <ContentView.h>
#include <ctype.h>
#include <HTMLMailToUtf8Adapter.h>
#include <Mime.h>
#include <parsedate.h>
#include <QPToPlainAdapter.h>
#include <stdio.h>
#include <TextToUtf8Adapter.h>
#include <TranslatorFormats.h>
#include <Window.h>
#include "MerlinContent.h"
#include "CMessengerAutoLocker.h"
#include "PartContainer.h"
#include "PostOffice.h"
#include "www/util.h"
#include "RawToBase64Adapter.h"
#include "Rfc822MessageAdapter.h"
#include "MailDebug.h"

// -----------------------------------------------------------------------------------------
//									MerlinContentInstance
// -----------------------------------------------------------------------------------------

MerlinContentInstance::MerlinContentInstance(MerlinContent *parent, GHandler *h, const BString &mime, const BMessage &attributes)
	:	ContentInstance(parent, h),
		fEmbedAttributes(attributes),
		fCurrentView(NULL),
		fMailList(NULL),
		fMailListObserver(NULL),
		fEditorView(NULL),
		fMailbox(B_EMPTY_STRING),
		fUsername(B_EMPTY_STRING),
		fDraftUid(B_EMPTY_STRING),
		fContentMode(kUnknownMode),
		fComposeMasterKey(0),
		fBusyCount(0),
		fSending(false),
		fTextViewLocker("TextViewLocker")
{
	MDB(MailDebug md);
	// Extract which content mode we need to be in based on the mime signature that was
	// passed to us...
	if (mime.ICompare(kContentListViewSignature) == 0) {
		fContentMode = kListMode;
	} else if (mime.ICompare(kContentComposeSignature) == 0) {
		BString mode;
		attributes.FindString("mode", &mode);
		if (mode.ICompare("reply") == 0)
			fContentMode = kReplyMode;
		else if (mode.ICompare("forward") == 0)
			fContentMode = kForwardMode;
		else if (mode.ICompare("draft") == 0)
			fContentMode = kDraftsMode;
		else
			fContentMode = kComposeMode;
	} else {
		DEBUGGER("MerlinContent: 'mime' type was unknown in constructor.");
	}
	
	// Create the appropriate view.
	switch (fContentMode) {
		case kListMode: {	
			fCurrentView = (fMailList = new MailListView(BRect(0, 0, 642, 600), fEmbedAttributes, this));
			fMailListObserver = new MailListObserver(this);
			break;
		}
		case kReplyMode:
		case kForwardMode:
		case kDraftsMode:
		case kComposeMode: {
			fCurrentView = (fEditorView = new CEditorView(BRect(0, 0, 642, 600)));
			break;
		}
		default: {
			DEBUGGER("MerlinContent: 'fContentMode' was unknown in constructor.");
			break;
		}
	} // end of switch ()

	// BinderNode that represents the state saving node
	fStateNode = BinderNode::Root()["service"]["web"]["state"];
	// Current username
	fUsername = BinderNode::Root()["user"]["~"]["name"].String();
}

MerlinContentInstance::~MerlinContentInstance()
{
	MDB(MailDebug md);
}

status_t MerlinContentInstance::AttachedToView(BView *inView, uint32 *contentFlags)
{
	MDB(MailDebug md);
	(void)contentFlags;
	if (fCurrentView != NULL) {
		inView->AddChild(fCurrentView);
		BRect rect = FrameInParent();
		fCurrentView->MoveTo(rect.LeftTop());
		fCurrentView->ResizeTo(rect.Width(), rect.Height());	
	}

	switch (fContentMode) {
		case kListMode: {
			BMessage args(kSelectMailbox);
			args.AddString("mailbox", fEmbedAttributes.FindString("mailbox"));
			PostMessage(args);
			break;
		}
		case kReplyMode:
		case kForwardMode:
		case kDraftsMode: {
			BMessage args(kBuildCompose);
			args.AddString("uid", fEmbedAttributes.FindString("uid"));
			args.AddString("mailbox", fEmbedAttributes.FindString("mailbox"));
			PostMessage(args);
			break;
		}
		default:
			break;
	}
	// Restore compose view's state.
	if (fContentMode != kListMode) {
		fStateNode = BinderNode::Root()["service"]["web"]["state"];
		if (!fStateNode.IsUndefined()) {
			BinderNode::property traProp = fStateNode["mail:compose:text_run_array"];
			BinderNode::property bodyProp = fStateNode["mail:compose:body"];
			
			// No body? Then don't bother with the text_run
			if (bodyProp.IsString()) {
				// Check for a valid text_run
				BMallocIO buffer;
				if (traProp.IsString())
					Base64ToRawAdapter::Decode(static_cast<const void *>(traProp.String().String()), traProp.String().Length(), &buffer);
				const text_run_array *textArray = buffer.BufferLength() > 0 ? static_cast<const text_run_array *>(buffer.Buffer()) : NULL;
				fEditorView->TextView()->SetText(bodyProp.String().String(), textArray);
			}
			// Move insertion point to the end.
			fEditorView->TextView()->Select(fEditorView->TextView()->TextLength(), fEditorView->TextView()->TextLength());
		}
	}
	return B_OK;
}

status_t MerlinContentInstance::DetachedFromView()
{
	fComposeMasterKey++;
	
	if (fCurrentView != NULL) {
		fCurrentView->RemoveSelf();
		delete fCurrentView;
		delete fMailListObserver;
		fCurrentView = NULL;
	}

	return B_OK;
}

status_t MerlinContentInstance::GetSize(int32 *x, int32 *y, uint32 * outResizeFlags)
{
	*x = 642; 	// Doesn't really matter, but a rough aproximation
	*y = 350;	// Doesn't really matter	
	*outResizeFlags = STRETCH_VERTICAL | STRETCH_HORIZONTAL;
	return B_OK;
} 

status_t MerlinContentInstance::FrameChanged(BRect newFrame, int32 fullWidth, int32 fullHeight)
{
	if (fCurrentView == NULL)
		return B_ERROR;
	
	if (fCurrentView->Window()) {
		if (fCurrentView->Window()->Lock()) {
			fCurrentView->ResizeTo(newFrame.Width(), newFrame.Height());

			BPoint newLoc = newFrame.LeftTop();
			BView *parent = fCurrentView->Parent();
			if (parent != NULL)
				newLoc -= parent->Bounds().LeftTop();
			fCurrentView->MoveTo(newLoc);
			fCurrentView->Window()->Unlock();
		}
	}

	return ContentInstance::FrameChanged(newFrame, fullWidth, fullHeight);
} 

void MerlinContentInstance::Cleanup()
{
	ContentInstance::Cleanup();
	BinderNode::Cleanup();
}

status_t MerlinContentInstance::HandleMessage(BMessage *message)
{
	status_t result = B_OK;
	switch (message->what) {
		case kSelectMailbox: {
			const char *mailbox = NULL;
			if (message->FindString("mailbox", &mailbox) == B_OK)
				SelectMailboxWorker(mailbox);
			break;
		}
		case kSyncList: {
			uint32 event = SummaryContainer::kUnknownEvent;
			message->FindInt32("event", (int32 *)&event);
			SyncListView(event);
			break;
		}
		case kBuildCompose: {
			int32 privateKey = fComposeMasterKey;
			if (fTextViewLocker.Lock()) {
				privateKey = ++fComposeMasterKey;
				fTextViewLocker.Unlock();
			}
			ResumeScheduling();
			fEmbedAttributes.PrintToStream();
			BuildComposeWorker(fEmbedAttributes.FindString("mailbox"), fEmbedAttributes.FindString("uid"), privateKey);
			break;
		}
		case kSendMessage: {
			fSending = true;
			fEditorView->SendMessage(message);
			fSending = false;
			break;
		}
		case kSelectionChanged: {
			int32 value;
			message->FindInt32("openarg", &value);
			BinderNode::property arg1 = value;
			message->FindInt32("deletearg", &value);
			BinderNode::property arg2 = value;
			
			BinderNode::property prop = dynamic_cast<BinderNode *>(Handler());
			prop["window"]["EnableOpenButton"](&arg1, NULL);
			prop["window"]["EnableSaveButton"](&arg1, NULL);
			prop["window"]["EnableDeleteButton"](&arg2, NULL);
			break;
		}
		case kOpenMessage: {
			BinderNode::property prop = dynamic_cast<BinderNode *>(Handler());
			prop["window"]["OpenMessage"]();
			break;
		}
		default: {
			result = BinderNode::HandleMessage(message);
			break;
		}
	}
	return result;
}

get_status_t MerlinContentInstance::ReadProperty(const char *name, property &outProperty, const property_list &inArgs)
{
	MDB(MailDebug md);
	DB(md.Print("Name = '%s'\n", name));
	
	outProperty.Undefine();

	if (fContentMode == kListMode) {
		// List view mode
		if (strcmp(name, "LoadMailbox") == 0)
			LC_SelectMailbox(outProperty, inArgs); 
		else if (strcmp(name, "GetHeaderInfo") == 0)
			LC_GetHeaderInfo(outProperty, inArgs);
		else if (strcmp(name, "OpenNext") == 0)
			LC_OpenNext(outProperty, inArgs, 1);
		else if (strcmp(name, "OpenPrevious") == 0)
			LC_OpenNext(outProperty, inArgs, -1);
		else if (strcmp(name, "DeleteMessage") == 0)
			LC_DeleteMessage(outProperty, inArgs);
		else if (strcmp(name, "DeleteSelectedMessages") == 0)
			LC_DeleteSelectedMessages(outProperty, inArgs);
		else if (strcmp(name, "ExpungeMailbox") == 0)
			LC_ExpungeMailbox(outProperty, inArgs);
		else {
			DB(md.Print("Unknown read property\n"));
		}
	} else if (fContentMode > kListMode) {
		// Must be in editor mode
		if (strcmp(name, "SendMessage") == 0)
			LC_SendMessage(outProperty, inArgs);
		else if (strcmp(name, "SetSelectionStyle") == 0)
			LC_SetSelectionStyle(outProperty, inArgs);
		else if (strcmp(name, "SetSelectionSize") == 0)
			LC_SetSelectionSize(outProperty, inArgs);
		else if (strcmp(name, "SetSelectionColor") == 0)
			LC_SetSelectionColor(outProperty, inArgs);
		else if (strcmp(name, "AddAttachment") == 0)
			LC_UpdateAttachmentList(outProperty, inArgs, true);
		else if (strcmp(name, "RemoveAttachment") == 0)
			LC_UpdateAttachmentList(outProperty, inArgs, false);
		else if (strcmp(name, "RemoveAllAttachments") == 0)
			LC_RemoveAllAttachments(outProperty, inArgs);
		else if (strcmp(name, "SaveState") == 0)
			LC_SaveMessage(outProperty, inArgs, true);
		else if (strcmp(name, "SaveDraft") == 0)
			LC_SaveMessage(outProperty, inArgs, false);
		else if (strcmp(name, "ClearAll") == 0)
			LC_ClearAll(outProperty, inArgs);
		else {
			DB(md.Print("Unknown read property\n"));
		}
	} else {
		// Unknown mode
		TRESPASS();
		return B_ERROR;
	}
	return B_OK;
}

put_status_t MerlinContentInstance::WriteProperty(const char *, const property &)
{
	return EPERM;
}

const char *MerlinContentInstance::GetLoadedMailbox() const
{
	return fMailbox.String();
}

void MerlinContentInstance::LC_SelectMailbox(property &outProperty, const property_list &inArgs)
{
	MDB(MailDebug md);
	
	atom<BinderContainer> node = new BinderContainer;
	const char *result = "B_OK";
	
	if (inArgs.Count() > 0) {
		// Remove all old messages...
		DequeueMessage(kSelectMailbox);
		// Send a new one..
		BMessage *message = new BMessage(kSelectMailbox);
		message->AddString("mailbox", inArgs[0].String().String());
		// Let GHandler take care of this...
		PostMessage(message);
	} else {
		result = "B_ERROR: LoadMailbox, mailbox name not found in argument list.";
	}
	
	node->AddProperty("result", result);
	outProperty = static_cast<BinderNode *>(node);
}

void MerlinContentInstance::LC_GetHeaderInfo(property &outProperty, const property_list &inArgs)
{
	MDB(MailDebug md);
	atom<BinderContainer> node = new BinderContainer;
	const char *result = "B_OK";
	
	// If two arguments were passed in, then the second argument represents the URL
	// of a message that we're trying to open. (Presumably a file on a memory stick
	// or something...)
	if (inArgs.Count() < 2) {
		MailListItem *selectedItem = dynamic_cast<MailListItem *>(fMailList->CurrentSelection());
		if (selectedItem != NULL) {
			// Special case the drafts mailbox... Eeew
			if (strcasecmp(selectedItem->GetMailbox(), "Drafts") == 0) {
				node->AddProperty("uid", selectedItem->GetSubject()); // Yes, this line is correct
				node->AddProperty("subject", selectedItem->GetSubject());
				node->AddProperty("from", selectedItem->GetAddress());
				BString escaped;
				escape_for_html(&escaped, selectedItem->GetAddress());
				node->AddProperty("from_escaped", escaped.String());
				node->AddProperty("mailbox", selectedItem->GetMailbox());
			} else {
				node->AddProperty("uid", selectedItem->GetUID());
				BString escaped;
				node->AddProperty("mailbox", selectedItem->GetMailbox());
				escape_for_html(&escaped, selectedItem->GetSubject());
				node->AddProperty("subject", escaped.String());
				node->AddProperty("from", selectedItem->GetAddress());
				escape_for_html(&escaped, selectedItem->GetAddress());
				node->AddProperty("from_escaped", escaped.String());
				BString date;
				if (inArgs.Count() == 1)
					GetParsedDate(selectedItem->GetDate(), date, inArgs[0].String().String());
				else
					GetParsedDate(selectedItem->GetDate(), date, NULL);
				node->AddProperty("date", date.String());
				// Extract all the attachments for this message...
				node->AddProperty("has_attachments", (int)(selectedItem->HasAttachments()));
				MimeMessage *message = PostOffice::MailMan()->FindMessage(selectedItem->GetUID());
				int32 propCount = 0;
				atom<BinderContainer> attachmentNode = new BinderContainer;
				if (message != NULL)
					BuildAttachmentList(attachmentNode, message, message->GetRoot(), propCount);
				node->AddProperty("attachments", static_cast<BinderNode *>(attachmentNode));
				// Extract the index information.
				int32 current = 0;
				BString uid = selectedItem->GetUID();
				GetIndexInfo(uid, current);
	
				node->AddProperty("nextIndex", (int)min_c(current + 1, fMailList->CountRows() - 1));
				node->AddProperty("prevIndex", (int)max_c(current - 1, -1));
				node->AddProperty("currentIndex", (int)current);
				node->AddProperty("listCount", (int)fMailList->CountRows());
			}
		} else {
			result = "B_ERROR: There was no focus row.";
		}
	} else if (inArgs.Count() == 2) {
		PostOffice::MailMan()->ParseRfc822Header(inArgs[1].String().String(), node);
		node->AddProperty("nextIndex", (int)0);
		node->AddProperty("prevIndex", (int)-1);
		node->AddProperty("currentIndex", (int)0);
		node->AddProperty("listCount", (int)1);
	} else {
		result = "B_ERROR: Invalid argument count";
	}
	node->AddProperty("result", result);
	outProperty = static_cast<BinderNode *>(node);
}

void MerlinContentInstance::LC_OpenNext(property &outProperty, const property_list &inArgs, int32 delta)
{
	MDB(MailDebug md);
	atom<BinderContainer> node = new BinderContainer;
	const char *result = "B_OK";
	int32 current = 0;
	BString uid = inArgs.Count() > 0 ? inArgs[0].String() : B_EMPTY_STRING;
	GetIndexInfo(uid, current);

	current += delta;
	// Check for out of bounds...
	if (current < 0)
		current = 0;
	else if (current >= fMailList->CountRows())
		current = fMailList->CountRows() - 1;

	BRow *row = fMailList->RowAt(current);
	if (row != NULL) {
		CMessengerAutoLocker messengerAutoLock(fMailList);
		if (!messengerAutoLock) {
			result = "B_ERROR";
		} else {
			fMailList->DeselectAll();
			fMailList->SetFocusRow(row, true);
			fMailList->ItemInvoked();
		}
	} else {
		result = "B_ERROR";
	}

	node->AddProperty("result", result);
	outProperty = static_cast<BinderNode *>(node);
}

void MerlinContentInstance::LC_DeleteMessage(property &outProperty, const property_list &inArgs)
{
	MDB(MailDebug md);
	atom<BinderContainer> node = new BinderContainer;
	const char *result = "B_ERROR";
		
	if (inArgs.Count() == 2) {
		MimeMessage *message = PostOffice::MailMan()->FindMessage(inArgs[1].String().String());
		if (message != NULL) {
			for (;;) {
				bool turnOn = !(kMessageDeleted & message->GetFlags());
				PostOffice::MailMan()->SetFlags(inArgs[1].String().String(), kMessageDeleted, turnOn);
				CMessengerAutoLocker messengerAutoLock(fMailList);
				if (!messengerAutoLock) {
					result = "B_ERROR";
					break;
				}

				MailListItem *item = fMailList->FindMailListItem(inArgs[1].String().String());
				if (item != NULL)
					fMailList->UpdateRow(item);
				break;
			}
		}
	} else {
		result = "B_ERROR";
	}
	node->AddProperty("result", result);
	outProperty = static_cast<BinderNode *>(node);
}

void MerlinContentInstance::LC_DeleteSelectedMessages(property &outProperty, const property_list &)
{
	MDB(MailDebug md);
	atom<BinderContainer> node = new BinderContainer;
	const char *result = "B_OK";
	
	MailListItem *selectedItem = dynamic_cast<MailListItem *>(fMailList->CurrentSelection());
	while (selectedItem != NULL) {
		MimeMessage *message = PostOffice::MailMan()->FindMessage(selectedItem->GetUID());
		if (message != NULL) {
			for (;;) {
				bool turnOn = !(kMessageDeleted & message->GetFlags());
				PostOffice::MailMan()->SetFlags(selectedItem->GetUID(), kMessageDeleted, turnOn);
				CMessengerAutoLocker messengerAutoLock(fMailList);
				if (!messengerAutoLock) {
					result = "B_ERROR";
					break;
				}
				fMailList->UpdateRow(selectedItem);
				break;
			}
		}
		selectedItem = dynamic_cast<MailListItem *>(fMailList->CurrentSelection(selectedItem));
	}
	
	node->AddProperty("result", result);
	outProperty = static_cast<BinderNode *>(node);
}

void MerlinContentInstance::LC_ExpungeMailbox(property &outProperty, const property_list &)
{
	MDB(MailDebug md);
	atom<BinderContainer> node = new BinderContainer;
	node->AddProperty("result", "B_OK");
	outProperty = static_cast<BinderNode *>(node);
}

void MerlinContentInstance::LC_SendMessage(property &outProperty, const property_list &inArgs)
{
	MDB(MailDebug md);
	atom<BinderContainer> node = new BinderContainer;
	const char *result = "B_OK";
	if (!fSending) {
		BMessage args(kSendMessage);
		args.AddString("subject", inArgs[0].String().String());
		// I need to do this here so that I can send a response back to Javascript.
		StringBuffer problem;
		if (ParseRecipients(&args, inArgs[1].String().String(), &problem) != B_OK) {
			// There was a problem parsing the recipients. Report this back to
			// Javascript and don't send the message.
			result = "B_ERROR";
			node->AddProperty("notoriousBIG", problem.String());
			args.PrintToStream();
		}  else {
			PostMessage(args);
			args.PrintToStream();
		}
	}
	node->AddProperty("result", result);
	outProperty = static_cast<BinderNode *>(node);
}

void MerlinContentInstance::LC_SaveMessage(property &outProperty, const property_list &inArgs, bool justState)
{
	outProperty.Undefine();
	const char *result = "B_OK";
	// Extract the text_run_array...
	text_run_array *runArray = fEditorView->TextView()->RunArray(0, fEditorView->TextView()->TextLength());
	int32 runSize = sizeof(text_run_array) + (runArray->count * sizeof(text_run));
	BString encodedRunArray;
	RawToBase64Adapter::Encode((void *)runArray, runSize, &encodedRunArray);
	free(runArray); // runArray is malloc'd inside TextView
	
	fStateNode["mail:compose:body"] = fEditorView->TextView()->Text();
	fStateNode["mail:compose:text_run_array"] = encodedRunArray.String();

	if (!justState) {
		// Saving to a text file also...
		if (PostOffice::MailMan()->SaveDraft(inArgs, fDraftUid.String(), fEditorView->TextView()->Text(), encodedRunArray) != B_OK)
			result = "B_ERROR: Error saving draft, post office had problems.";
	}
	
	// We're done...
	atom<BinderContainer> node = new BinderContainer;
	node->AddProperty("result", result);
	outProperty = static_cast<BinderNode *>(node);
}

void MerlinContentInstance::LC_SetSelectionStyle(property &outProperty, const property_list &inArgs)
{
	MDB(MailDebug md);
	atom<BinderContainer> node = new BinderContainer;
	const char *result = "B_OK";
	if (inArgs.Count() == 1) {
		text_style style = (inArgs[0].String().ICompare("bold") == 0) ? kBoldStyle : kPlainStyle;
		CMessengerAutoLocker messengerAutoLock(fEditorView);
		if (!messengerAutoLock)
			return;

		fEditorView->SetSelectedTextStyle(style);
	} else
		result = "B_ERROR: Invalid argument count to SetSelectionStyle().";

	node->AddProperty("result", "B_OK");
	outProperty = static_cast<BinderNode *>(node);
}

void MerlinContentInstance::LC_SetSelectionSize(property &outProperty, const property_list &inArgs)
{
	MDB(MailDebug md);
	atom<BinderContainer> node = new BinderContainer;
	const char *result = "B_OK";
	if (inArgs.Count() == 1) {
		CMessengerAutoLocker messengerAutoLock(fEditorView);
		if (!messengerAutoLock)
			return;
		fEditorView->SetSelectedTextSize(atoi(inArgs[0].String().String()));
	} else
		result = "B_ERROR: Invalid argument count to SetSelectionSize().";

	node->AddProperty("result", "B_OK");
	outProperty = static_cast<BinderNode *>(node);
}

void MerlinContentInstance::LC_SetSelectionColor(property &outProperty, const property_list &inArgs)
{
	MDB(MailDebug md);
	atom<BinderContainer> node = new BinderContainer;
	const char *result = "B_OK";
	if (inArgs.Count() == 3) {
		rgb_color color;
		color.red = atoi(inArgs[0].String().String());
		color.green = atoi(inArgs[1].String().String());
		color.blue = atoi(inArgs[2].String().String());
		CMessengerAutoLocker messengerAutoLock(fEditorView);
		if (!messengerAutoLock)
			return;
		fEditorView->SetSelectedTextColor(&color);
	} else
		result = "B_ERROR: Invalid argument count to SetSelectionColor(). Expected 3 args: red, green, blue";

	node->AddProperty("result", result);
	outProperty = static_cast<BinderNode *>(node);
}

void MerlinContentInstance::LC_ClearAll(property &outProperty, const property_list &)
{
	outProperty.Undefine();
	CMessengerAutoLocker messengerAutoLock(fEditorView);
	if (!messengerAutoLock)
		return;

	fEditorView->TextView()->SetText("");
	BFont font;
	rgb_color black = {0, 0, 0, 255};
	fEditorView->TextView()->GetFontAndColor(0, &font);
	fEditorView->TextView()->SetFontAndColor(&font, B_FONT_ALL, &black);
	fDraftUid = B_EMPTY_STRING;
	atom<BinderContainer> node = new BinderContainer;
	node->AddProperty("result", "B_OK");
	outProperty = static_cast<BinderNode *>(node);
}

void MerlinContentInstance::LC_UpdateAttachmentList(property &outProperty, const property_list &inArgs, bool addFile)
{
	atom<BinderContainer> node = new BinderContainer;
	const char *result = "B_OK";
	if (addFile) {
		if (inArgs.Count() != 2)
			result = "B_ERROR";
		else
			fEditorView->AddAttachment(inArgs[0].String().String(), inArgs[1].String().String());
	} else {
		if (inArgs.Count() < 1)
			result = "B_ERROR";
		else
			fEditorView->RemoveAttachment(inArgs[0].String().String());
	}
	
	node->AddProperty("result", result);
	outProperty = static_cast<BinderNode *>(node);
}

void MerlinContentInstance::LC_RemoveAllAttachments(property &outProperty, const property_list &inArgs)
{
	atom<BinderContainer> node = new BinderContainer;
	ASSERT(fEditorView != NULL);
	fEditorView->RemoveAllAttachments();
	node->AddProperty("result", "B_OK");
	outProperty = static_cast<BinderNode *>(node);
}

status_t MerlinContentInstance::SelectMailboxWorker(const char *mailbox)
{
	MDB(MailDebug md);
	ASSERT(mailbox != NULL);
	
	// If this mailbox is already selected, no need to change anything
	BString username = BinderNode::Root()["user"]["~"]["name"].String();
	if ((fMailbox == mailbox) && (fUsername == username))
		return B_OK;
	
	status_t result = B_OK;
	column_mode mode = kIncomingMode;
	
	// Set the column mode...
	if ((strcmp(mailbox, "Drafts") == 0) || (strcmp(mailbox, "Sent Items") == 0)) {
		mode = kOutgoingMode;
	}
	// Shutdown the current summary
	SummaryContainer *container = PostOffice::MailMan()->LoadedSummaryContainer();
	if (container != NULL) {
		container->Lock();
		container->Shutdown();
		container->Unlock();
	}
	// Clear out the current list
	fMailList->DeselectAllAndClear();
	// Set the column mode
	fMailList->SetColumnMode(mode);
	// Set this as our new mailbox and update username
	fMailbox = mailbox;
	fUsername = username;
	// Select mailbox. Note, even if this fails, we still want fMailbox to point to
	// this mailbox because the UI thinks the mailbox is loaded. (And we need this
	// so that the ObservationCallback stuff will work properly)
	PostOffice::MailMan()->SelectMailbox(fUsername.String(), mailbox, fMailListObserver);
	return result;
}

status_t MerlinContentInstance::SyncListView(uint32 event)
{
	SummaryContainer *container = PostOffice::MailMan()->LoadedSummaryContainer();
	if (container != NULL) {
		container->Lock();
		fMailList->SyncList(container, event);
		fMailListObserver->fDirty = true;
		container->Unlock();
	}
	return B_OK;		
}

status_t MerlinContentInstance::BuildComposeWorker(const char *mailbox, const char *uid, int32 privateKey)
{
	MDB(MailDebug md);
	ASSERT((mailbox != NULL) && (uid != NULL));
	status_t result = B_OK;
	BDataIO *stream = NULL;
	
	// ** Special case the drafts folder... Eeew
	if (strcasecmp(mailbox, "Drafts") == 0) {
		fDraftUid = uid;

		PartContainer container("Drafts", uid, "0");
		BFile *file = dynamic_cast<BFile *>(PostOffice::MailMan()->FetchSection(container));

		ASSERT(file != NULL);
		if (file == NULL)
			return B_ERROR;
		
		stream = file;
		fEditorView->StreamTextBody(stream, privateKey, &fComposeMasterKey, &fTextViewLocker, fContentMode);
		BString attribute;
		file->ReadAttrString("beia_mail:encodedRunArray", &attribute);
		BMallocIO buffer;
		Base64ToRawAdapter::Decode(static_cast<const void *>(attribute.String()), attribute.Length(), &buffer);
		const text_run_array *textArray = buffer.BufferLength() > 0 ? static_cast<const text_run_array *>(buffer.Buffer()) : NULL;
		CMessengerAutoLocker _lock(fEditorView);
		fEditorView->TextView()->SetRunArray(0, fEditorView->TextView()->TextLength(), textArray);
	} else {
		BString contentType = "text/plain";
		if (strcmp(uid, "*be:eml") == 0) {
			const char *path = NULL;
			if ((fEmbedAttributes.FindString("emlurl", &path) != B_OK) || (strcmp(path, "") == 0))
				return B_ERROR;
			stream = new Rfc822MessageAdapter(path);
		} else {
			MimeMessage *message = PostOffice::MailMan()->FindMessage(uid);
			if (message == NULL)
				return B_ERROR;
			// Ideally we'd like to reply to the text/plain part of the message. If there isn't
			// a text/plain part, then we'll get the next best thing (probably text/html)
			MessagePart *part = message->GetMainDoc("text/plain");
			if (part == NULL)
				return B_ERROR;
			// Get the content type
			contentType = part->type;
			// Create a part container for fetching
			PartContainer container(mailbox, uid, part->id.String());
			container.SetContentTypePtr(&contentType);
			container.SetSize(part->size);
			// Fetch the section
			BDataIO *source = PostOffice::MailMan()->FetchSection(container);
	
			ASSERT(source != NULL);
			if (source == NULL)
				return B_ERROR;
			// Line-up the adapters for doing conversion
			stream = source;
			// First, convert from the source encoding to a raw format.
			if (part->encoding.ICompare("base64") == 0)
				stream = new Base64ToRawAdapter(source);
			else if (part->encoding.ICompare("quoted-printable") == 0)
				stream = new QPToPlainAdapter(source, MessagePart::CharsetConversion(part->characterSet));
		}
		
		// Perform higher level conversions
		if (contentType.ICompare("text/", 5) == 0) {
			ConcatenateAdapter *merged = new ConcatenateAdapter;
			if (contentType.ICompare("text/plain") == 0)
				merged->AddStream(new TextToUtf8Adapter(stream));
			else if (contentType.ICompare("text/html") == 0)
				merged->AddStream(new HTMLMailToUtf8Adapter(stream));
			stream = merged;
		}
		fEditorView->StreamTextBody(stream, privateKey, &fComposeMasterKey, &fTextViewLocker, fContentMode);
	}
	delete stream;
	return result;
}

void MerlinContentInstance::GetIndexInfo(BString &uid, int32 &current)
{
	BRow *item = fMailList->CurrentSelection();
	if ((item == NULL) && (uid != B_EMPTY_STRING)) {
		// Nothing selected, probably because we got restored from a saved state
		// and immediately went to a display message frame. In this case, the UI
		// should tell us which UID they are currently looking at. We'll try and
		// find that UID in the list, select it, then go from there...
		for (int32 i = 0; i < fMailList->CountRows(); i++) {
			MailListItem *listItem = dynamic_cast<MailListItem *>(fMailList->RowAt(i));
			if (uid.Compare(listItem->GetUID()) == 0) {
				item = listItem;
				break;
			}
		}
	}
	// Should have a valid item now..
	if (item != NULL) {
		current = fMailList->IndexOf(item);
		if (current < 0) {
			// Couldn't find it, try selecting the first row..
			current = 0;
		}
	}
}

void MerlinContentInstance::GetParsedDate(const char *rawDate, BString &outDate, const char *format)
{
	time_t ptime = parsedate(rawDate, -1);
	char *buffer = outDate.LockBuffer(512);
	if (format != NULL) {
		// Format here is defined in Javascript
		strftime(buffer, 512, format, localtime(&ptime));
	} else {
		// Format here is 01/16/2000, 11:30 PM
		strftime(buffer, 512, "%m/%d/%Y, %I:%M %p", localtime(&ptime));
	}
	outDate.UnlockBuffer();
}

status_t MerlinContentInstance::ParseRecipients(BMessage *args, const char *recipients, StringBuffer *problem)
{
	MDB(MailDebug md);
	status_t result = B_OK;
	const char *ptr = recipients;
	// Extract each recipient
	while ((*ptr != '\0')) {
		// Skip leading whitespace
		while (isspace(*ptr))
			ptr++;
		// Extract up to delimeter
		bool fetched = false;
		bool inQuote = false;
		StringBuffer entry;
		while (!fetched) {
			if ((*ptr == '\0') || ((*ptr == ';' || *ptr == ',') && (!inQuote))) {
				fetched = true;
				if (*ptr != '\0')
					ptr++;
				break;
			}

			if (*ptr == '\"')
				inQuote = !inQuote;

			entry << *ptr++;
		}
		// If there is a '<', then assume that the address is in brackets
		// and everything else is the users name. Otherwise, assume that
		// everything is the address.
		const char *buffer = entry.String();
		bool hasName = (strchr(buffer, '<') != NULL);
		StringBuffer field;
		if (hasName) {
			// Remove leading whitespace
			while (isspace(*buffer))
				buffer++;
			int quoteCount = 0;
			while (*buffer != '<') {
				// Skip quotes
				if (*buffer == '\"') {
					quoteCount++;		
					buffer++;
					continue;
				}
				// Skip whitespace after last quote
				if ((quoteCount >= 2) && (isspace(*buffer))) {
					buffer++;
					continue;
				}
				field << *buffer++;
			}
			// Skip bracket
			buffer++;
			field.Clear();
		}
		// Always add this to keep the BMessage fields balanced.
		args->AddString("recipient_name", field.String());
		// Extract address, take everthing
		const char kInvalidChar[] = { 	'(', ')', '<', '>', ',',
										';', ':', '\\', '\"','[',
										']', ' ', '\t', '\n' };
		while ((*buffer != '\0') && (*buffer != '>')){
			if (strchr(kInvalidChar, *buffer) == NULL)
				field << *buffer;
			else {
				result = B_ERROR;
				*problem = field;
				break;
			}
			buffer++;
		}
		args->AddString("recipient_address", field.String());
	}
	return result;
}

void MerlinContentInstance::BuildAttachmentList(atom<BinderContainer> &node, const MimeMessage *message, const MessagePart *part, int32 &propCount)
{
	MDB(MailDebug md);
	if (part == 0)
		return;

	BMimeType attachmentType(part->type.String());
	BMessage extensions;
	BString extensionString("");
	
	if (attachmentType.GetFileExtensions(&extensions) == B_OK)
		if (extensions.FindString("extensions", &extensionString) == B_OK)
			extensionString.Prepend(".");

	const MessagePart *body = (const_cast<MimeMessage *>(message))->GetMainDoc("text/html");
	if (part->isContainer) {
		bool showPart = true;
		if (part->containerType == MessagePart::kRelated) {
			// If this is related to the content, don't show it (for example, inlined
			// parts of HTML mail).
			for (int32 i = 0; i < part->subParts.CountItems(); i++) {
				MessagePart *subPart = reinterpret_cast<MessagePart*>(part->subParts.ItemAt(i));
				if (subPart->isContainer || subPart == body) {
					showPart = false;
					break;
				}
			}
		}

		if (part->containerType == MessagePart::kAlternative) {
			// There can be only one...
			MessagePart *preferred = 0;
			for(int32 i = 0; i < part->subParts.CountItems(); i++) {
				MessagePart *subPart = reinterpret_cast<MessagePart*>(part->subParts.ItemAt(i));
				if (subPart == body) {
					preferred = 0;
					break;
				}
				
				if (!preferred)	// For now, just pick the first one.
					preferred = subPart;
			}
			
			if (preferred)
				BuildAttachmentList(node, message, preferred, propCount);
		} else if (showPart) {
			for (int32 i = 0; i < part->subParts.CountItems(); i++)
				BuildAttachmentList(node, message, static_cast<MessagePart*>(part->subParts.ItemAt(i)), propCount);
		}
	} else if ((part != body) /* && (part->disposition != MessagePart::kInline) */ // Yahoo Mail sends attachments as inlined!
				&& (part->type.ICompare("application/x-be_attribute") != 0)) {
		atom<BinderContainer> attachment = new BinderContainer;
		attachment->AddProperty("type", part->type.String());
		attachment->AddProperty("mailbox", message->GetMailbox());		
		attachment->AddProperty("uid", message->GetUid());
		attachment->AddProperty("msgpart", part->id.String());
		attachment->AddProperty("name", part->name.String());
		attachment->AddProperty("size", part->size);
		// The name needs to be unique...
		BString propName("attachment_");
		propName << propCount++;
		node->AddProperty(propName.String(), static_cast<BinderNode *>(attachment));
	}
}

// -----------------------------------------------------------------------------------------
//										MerlinContent
// -----------------------------------------------------------------------------------------

MerlinContent::MerlinContent(void *handle, const char *mime)
	:	Content(handle),
		fMimeType(mime)
{
	// empty body
}


MerlinContent::~MerlinContent()
{
	// empty body
}

size_t MerlinContent::GetMemoryUsage()
{
	return 0;
}

ssize_t MerlinContent::Feed(const void *, ssize_t , bool )
{
	return 0;
}

status_t MerlinContent::CreateInstance(ContentInstance **outInstance, GHandler *handler, const BMessage &attributes)
{

	*outInstance = new MerlinContentInstance(this, handler, fMimeType, attributes);
	return B_OK;
}

// -----------------------------------------------------------------------------------------
//										MerlinContentFactory
// -----------------------------------------------------------------------------------------

class MerlinContentFactory : public ContentFactory
{
	public:
		virtual void GetIdentifiers(BMessage *into)
		{
			 /*
			 ** BE AWARE: Any changes you make to these identifiers should
			 ** also be made in the 'addattr' command in the makefile.
			 */
			into->AddString(S_CONTENT_MIME_TYPES, kContentComposeSignature);
			into->AddString(S_CONTENT_MIME_TYPES, kContentListViewSignature);

			into->AddString(S_CONTENT_PLUGIN_IDS, "BeIA Mail List View");
			into->AddString(S_CONTENT_PLUGIN_DESCRIPTION, "BeIA Mail List View 1.0");
		}
	
		virtual Content* CreateContent(void *handle, const char *mime, const char *extension)
		{
			(void)mime;
			(void)extension;
			return new MerlinContent(handle, mime);
		}
};

extern "C" _EXPORT ContentFactory* make_nth_content(int32 n, image_id , uint32 , ...)
{
	if (n == 0)
		return new MerlinContentFactory;
	return 0;
}
