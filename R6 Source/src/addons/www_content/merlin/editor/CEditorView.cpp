/*
	CEditorView.cpp
*/
#include <Autolock.h>
#include <Binder.h>
#include "CEditorView.h"
#include "CMessengerAutoLocker.h"
#include "PostOffice.h"
#include "SendMessageContainer.h"
#include <StreamIO.h>

using namespace Wagner;

// ** Note that the '*' in front of the name is important!
CEditorView::CEditorView(BRect frame)
	:	BView(frame, "*EditorView", B_FOLLOW_ALL, B_WILL_DRAW)
{
	BRect rect(Bounds());
	rect.right -= B_V_SCROLL_BAR_WIDTH;
	fTextView = new CTextView(rect, "fTextView", rect);
	AddChild(new BScrollView("scroller", fTextView, B_FOLLOW_ALL_SIDES,	0, false, true, B_NO_BORDER));
	fTextView->DisallowChar(B_INSERT);
}

CEditorView::~CEditorView()
{
	int32 count = fAttachmentList.CountItems() - 1;
	for (int32 i = count; i >= 0; i--)
		delete static_cast<attachment_container *>(fAttachmentList.RemoveItem(i));
}

status_t CEditorView::SendMessage(BMessage *args)
{
	SendMessageContainer *container = new SendMessageContainer;
	container->SetUser(BinderNode::Root()["user"]["~"]["name"].String().String());
	container->SetSubject(args->FindString("subject"));
	// This needs to parse this field, what are the parsing rules?
	const char *name;
	const char *address;
	int32 idx = 0;
	while (	(args->FindString("recipient_name", idx, &name) == B_OK) &&
			(args->FindString("recipient_address", idx, &address) == B_OK)) {
		if (strcmp(address, "") != 0)
			container->AddRecipient(address, name);
		idx++;
	}
	// Add attachments
	for (int32 i = 0; i < fAttachmentList.CountItems(); i++) {
		attachment_container *attachment = static_cast<attachment_container *>(fAttachmentList.ItemAt(i));
		container->AddAttachment(attachment);
	}
	// Container assumes ownership of text run
	text_run_array *runArray = fTextView->RunArray(0, fTextView->TextLength());
	container->SetBodyText(fTextView->Text(), runArray);
	// SendMailProxy assumes ownership of container
	PostOffice::MailMan()->SendMessage(container);
	return B_OK;
}

status_t CEditorView::AddAttachment(const char *path, const char *name)
{
	int32 count = fAttachmentList.CountItems();
	for (int32 i = 0; i < count; i++) {
		attachment_container *item = static_cast<attachment_container *>(fAttachmentList.ItemAt(i));
		if (item->fPath.Compare(path) == 0)
			return B_ERROR;
	}
	// Not already in list, add it
	fAttachmentList.AddItem(new attachment_container(path, name));
	return B_OK;
}

status_t CEditorView::RemoveAttachment(const char *path)
{
	int32 count = fAttachmentList.CountItems();
	for (int32 i = 0; i < count; i++) {
		attachment_container *item = static_cast<attachment_container *>(fAttachmentList.ItemAt(i));
		if (item->fPath.Compare(path) == 0) {
			fAttachmentList.RemoveItem(i);
			delete item;
			return B_OK;
		}
	}
	// Not really an error, but for some reason this file was not an attachment.
	return B_ERROR;
}

status_t CEditorView::RemoveAllAttachments()
{
	int32 index = fAttachmentList.CountItems() - 1;
	while (index >= 0)
		delete (static_cast<attachment_container *>(fAttachmentList.RemoveItem(index--)));

	return B_OK;
}

status_t CEditorView::StreamTextBody(BDataIO *stream, int32 thePoliceMan, volatile int32 *textViewPolice,
										BLocker *locker, int32 mode)
{
	// ** Dude, this function is totally lame... ** Kenny
	StringBuffer insertBuffer;
	switch (mode) {
		case 2:
			insertBuffer << "\n-------- In reply to:\n\n> ";
			break;
		case 3:
			insertBuffer = "\n-------- Forwarded Message:\n\n";
			break;
		default:
			break;
	}

	CMessengerAutoLocker _lock(this);
	if (!_lock)
		return B_ERROR;
	BAutolock tempAutoLock(*locker);
	if(thePoliceMan != *textViewPolice)
		return B_ERROR;
	// Clear text view.
	fTextView->SetText("");

	char buffer[1024 + 1];
	ssize_t read;
	bool insertedHeader = false;
	while((read = stream->Read(buffer, 1024)) > 0) {
		BAutolock autoLock(*locker);
		if(thePoliceMan != *textViewPolice)
			break;
		buffer[max_c(read, 0)] = '\0';
		// Clear insert buffer
		if (insertedHeader)
			insertBuffer.Clear();
		else
			insertedHeader = true;
		// In reply mode, add '>' brackets..
		if(mode == 2) {
			for(int32 i = 0; i < read; i++) {
				insertBuffer << buffer[i];
				if(buffer[i] == '\n')
					insertBuffer << "> ";
			}
		} else {
			insertBuffer << buffer;
		}
		fTextView->Insert(insertBuffer.String(), insertBuffer.Length());
	}
	return B_OK;
}

CTextView *CEditorView::TextView()
{
	return fTextView;
}

void CEditorView::SetSelectedTextColor(const rgb_color *color)
{
	fTextView->SetFontAndColor(NULL, 0, color);
}

void CEditorView::SetSelectedTextSize(int delta)
{
	BFont font;
	uint32 properties;
	fTextView->GetFontAndColor(&font, &properties);
	font.SetSize(font.Size() + delta);
	
	if(delta == 0)
		font.SetSize(14.0);
	else {
		if (font.Size() > 48)
			font.SetSize(48);
		else if(font.Size() < 8)
			font.SetSize(8);
	}
	
	fTextView->SetFontAndColor(&font, B_FONT_SIZE);
}

void CEditorView::SetSelectedTextStyle(text_style style)
{
	BFont font = (style == kBoldStyle ? be_bold_font : be_plain_font);
	BFont currentFont;
	uint32 properties;
	fTextView->GetFontAndColor(&currentFont, &properties);
	font.SetSize(currentFont.Size());
	
	fTextView->SetFontAndColor(&font);	
}


/* End of CEditorView.cpp */

