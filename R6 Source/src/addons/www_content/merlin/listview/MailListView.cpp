/*
	MailListView.cpp
*/
#include <Binder.h>
#include "CMessengerAutoLocker.h"
#include <Content.h>
#include <ContentView.h>
#include <ColumnTypes.h>
#include <File.h>
#include <parsedate.h>
#include <stdio.h>
#include <stdlib.h>
#include <TranslatorFormats.h>
#include <www/util.h>
#include "MailListView.h"
#include "MailDebug.h"

using namespace Wagner;

const float kRowHeight = 20.0;
const float kFontSize = 12.0;
static BResourceSet gResources;
static bool gResInitialized = false;
const rgb_color kDeletedColor = { 170, 170, 170, 255};

MailListView::MailListView(BRect frame, BMessage &parameters, atomref<GHandler> target)
	:	BColumnListView(frame, "*MailColumnListView", B_FOLLOW_ALL, B_WILL_DRAW, B_NO_BORDER, false),
		fSelectedRow(NULL),
		fColumnMode(kIncomingMode),
		fTarget(target)
{
	int32 width = atoi(parameters.FindString("status_width"));
	AddColumn(new ImapIconStatusColumn(B_EMPTY_STRING, width, width, width, true), 0);

	width = atoi(parameters.FindString("from_width"));
	AddColumn(new ImapStringColumn("From",width, width, width, B_TRUNCATE_END), 1);

	width = atoi(parameters.FindString("subject_width"));
	AddColumn(new ImapStringColumn("Subject", width, width, width, B_TRUNCATE_END), 2);

	width = atoi(parameters.FindString("date_width"));
	AddColumn(new ImapDateColumn("Date", width, width, width), 3);

	width = atoi(parameters.FindString("attachment_width"));
	AddColumn(new ImapIconStatusColumn(B_EMPTY_STRING, width, width, width, false), 4);

	width = atoi(parameters.FindString("size_width"));
	AddColumn(new ImapSizeColumn("Size", width, width, width), 5);

	// No moving, resizing, popping, or anything else dammit!
	// What do you think this is, a desktop OS or something...
	SetColumnFlags((column_flags)(B_ALLOW_COLUMN_NONE));
	// Set the date column to be the default sorting column
	SetSortColumn(ColumnAt(3), false, false);
	// Clear the hash table
	memset(fEntries, 0, kMimeMessageHashSize * sizeof(MailHashEntry *));
}
 

MailListView::~MailListView()
{
	// The rows are cleared and deleted in ~BColumnListView()
}

void MailListView::AttachedToWindow()
{
	BColumnListView::AttachedToWindow();
	// Make sure the font size is properly set...
	BFont font;
	GetFont(B_FONT_ROW, &font);
	font.SetSize(kFontSize);
	SetFont(B_FONT_ROW, &font);
}

void MailListView::SelectionChanged()
{
	BColumnListView::SelectionChanged();
	enum {
		kNoneSelected,
		kOneSelected,
		kManySelected
	} selection = kNoneSelected;
	
	fSelectedRow = dynamic_cast<MailListItem *>(CurrentSelection());
	if (fSelectedRow != NULL)
		selection = (CurrentSelection(fSelectedRow) != NULL ? kManySelected : kOneSelected);

	BMessage *message = new BMessage(kSelectionChanged);
	message->AddInt32("openarg", selection == kOneSelected ? 1 : 0);
	message->AddInt32("deletearg", selection == kNoneSelected ? 0 : 1);
	fTarget->PostMessage(message);
}

void MailListView::DetachedFromWindow()
{
	BColumnListView::DetachedFromWindow();
}


void MailListView::ItemInvoked()
{
	fTarget->PostMessage(new BMessage(kOpenMessage));
	BColumnListView::ItemInvoked();
}

void MailListView::DeselectAllAndClear()
{
	{	// Scope for autolock, no need to have the
		// window locked while we're clearing the hash list...
		CMessengerAutoLocker _lock(this);
		if (!_lock)
			return;
		// Calling BColumnListView::Clear() is not enough
		// to trigger the SelectionChanged messages, even
		// though it does a DeselectAll, that's why it's
		// manually getting done here.
		BColumnListView::Clear();
		SelectionChanged();
	}
	
	for (int32 i = 0; i < kMimeMessageHashSize; i++) {
		for (MailHashEntry *entry = fEntries[i]; entry != 0; /* Empty loop control */) {
			MailHashEntry *temp = entry->fHashNext;
			delete entry;
			entry = temp;
		}
	}
	memset(fEntries, 0, kMimeMessageHashSize * sizeof(MailHashEntry *));
	
}

void MailListView::SetColumnMode(column_mode mode)
{
	if (fColumnMode != mode) {
		BTitledColumn *column = dynamic_cast<BTitledColumn *>(ColumnAt(1));
		if (column != NULL) {
			CMessengerAutoLocker _lock(this);
			if (!_lock)
				return;
			// This is a hack to get the column to refresh itself!
			column->SetVisible(false);
			column->SetTitle(mode == kIncomingMode ? "From" : "To");
			column->SetVisible(true);
		}
		fColumnMode = mode;
	}
}

MailListItem *MailListView::FindMailListItem(const char *uid)
{
	int32 count = CountRows();
	MailListItem *item = NULL;
	for (int32 i = 0; i < count; i++) {
		item = dynamic_cast<MailListItem *>(RowAt(i));
		if (item && (strcmp(item->GetUID(), uid) == 0))
			break;
	}
	return item;
}

void MailListView::SyncList(SummaryContainer *container, uint32 event)
{
	// When called, the container should already be locked
	SummaryIterator iterator(container);
	// Check for new messages in summary container or flags that have changed.
	if (event & SummaryContainer::kEntryAddedEvent) {
		for (iterator.First(); !iterator.IsDone(); iterator.Next()) {
			// Get the next message...
			MimeMessage *message = iterator.CurrentItem()->Message();
			if (!ContainsMessage(message)) {
				if (LockLooper()) {
					AddRow(new MailListItem(message, fColumnMode));
					AddEntry(new MailHashEntry(message));
					UnlockLooper();
				}
			}
		}
	}
	
	// Check for messages that are no longer in the summary container
	if (event & SummaryContainer::kEntryRemovedEvent) {
		for (uint8 i = 0; i < kMimeMessageHashSize; i++) {
			MailHashEntry *entry = 0;
			for (entry = fEntries[i]; entry != 0; /* Empty loop control*/ ) {
				// Save the next entry in a temp variable in case we delete this entry
				MailHashEntry *temp = entry->fHashNext;
				// See if this entry is in the summary container
				bool contains = (container->FindEntry(entry->fMessage->GetUid()) != NULL);
				if (!contains) {
					// Remove entry from the list view
					if (LockLooper()) {
						MailListItem *item = FindMailListItem(entry->fMessage->GetUid());
						if (item != NULL)
							RemoveRow(item);
						RemoveEntry(entry);
						UnlockLooper();
					}
				}
				entry = temp;
			}
		}
	}
	
	// Refresh those rows who's flags have been updated.
	if (event & SummaryContainer::kEntryModifiedEvent) {
		for (iterator.First(); !iterator.IsDone(); iterator.Next()) {
			// If the entry is marked as dirty, then refresh it in the list...
			SummaryEntry *sentry = iterator.CurrentItem();
			if (sentry->IsDirty()) {
				if (LockLooper()) {
					MailListItem *item = FindMailListItem(sentry->Message()->GetUid());
					if (item != NULL) {
						item->ReloadFields();
						UpdateRow(item);
					}
					UnlockLooper();
				}
			}
		}
	}
}

void MailListView::AddEntry(MailHashEntry *entry)
{
	ASSERT(entry != NULL);

	MailHashEntry **entryBucket = &fEntries[HashString(entry->Message()->GetUid()) % kMimeMessageHashSize];
	entry->fHashPrevious = entryBucket;
	entry->fHashNext = *entryBucket;
	*entryBucket = entry;
	if (entry->fHashNext != 0)
		entry->fHashNext->fHashPrevious = &entry->fHashNext;
}

void MailListView::RemoveEntry(MailHashEntry *entry)
{
	ASSERT(entry != NULL);
	
	*entry->fHashPrevious = entry->fHashNext;
	if (entry->fHashNext)
		entry->fHashNext->fHashPrevious = entry->fHashPrevious;

	delete entry;
}

MailHashEntry* MailListView::FindEntry(const MimeMessage *message)
{
	MailHashEntry **entryBucket = &fEntries[HashString(message->GetUid()) % kMimeMessageHashSize];
	MailHashEntry *entry = 0;
	for (entry = *entryBucket; entry != 0; entry = entry->fHashNext) {
		if (strcmp(entry->Message()->GetUid(), message->GetUid()) == 0) {
			break;
		}
	}
	return entry;
}

inline bool MailListView::ContainsMessage(const MimeMessage *message)
{
	return (FindEntry(message) != 0);
}

// --------------------------------------------------------------------
//								MailHashEntry
// #pragma mark -
// --------------------------------------------------------------------
MailHashEntry::MailHashEntry()
	:	fMessage(NULL)
{

}


MailHashEntry::MailHashEntry(MimeMessage *message)
	:	fMessage(message)
{
	fMessage->AcquireReference();
}


MailHashEntry::~MailHashEntry()
{
	fMessage->ReleaseReference();
}

const MimeMessage *MailHashEntry::Message() const
{
	return fMessage;
}

MimeMessage *MailHashEntry::Message()
{
	return fMessage;
}


// --------------------------------------------------------------------
//								MailListItem
// #pragma mark -
// --------------------------------------------------------------------
MailListItem::MailListItem(MimeMessage *mime, column_mode mode)
	:	BRow(kRowHeight),
		fMessage(mime),
		fMode(mode)
{
	ASSERT(fMessage != NULL);
	
	fMessage->AcquireReference();
	
	SetField(new ImapIconStatusField(kMailIcon, fMessage), 0);
	if (fMode == kIncomingMode) {
		fAddress = fMessage->GetFrom();
		SetField(new ImapStringField(fMessage->GetFromName(), fMessage), 1);
	} else if (fMode == kOutgoingMode) {
		fAddress = fMessage->GetRecipient();
		if (strcmp(fMessage->GetRecipientName(), B_EMPTY_STRING) == 0)
			SetField(new ImapStringField(fAddress.String(), fMessage), 1);
		else
			SetField(new ImapStringField(fMessage->GetRecipientName(), fMessage), 1);
	}
	SetField(new ImapStringField(fMessage->GetSubject(), fMessage), 2);

	time_t timeDate = parsedate(fMessage->GetDate(), -1);
	SetField(new ImapDateField(&timeDate, fMessage), 3);

	SetField(new ImapIconStatusField(kAttachmentIcon, fMessage), 4);
	SetField(new ImapSizeField(fMessage), 5);
}

MailListItem::~MailListItem()
{
	fMessage->ReleaseReference();
}

void MailListItem::ReloadFields()
{
	BStringField *field = dynamic_cast<BStringField *>(GetField(1));
	if (fMode == kIncomingMode) {
		fAddress = fMessage->GetFrom();
		field->SetString(fMessage->GetFromName());
	} else if (fMode == kOutgoingMode) {
		fAddress = fMessage->GetRecipient();
		if (strcmp(fMessage->GetRecipientName(), B_EMPTY_STRING) == 0)
			field->SetString(fMessage->GetRecipient());
		else
			field->SetString(fMessage->GetRecipientName());
	}

	field = dynamic_cast<BStringField *>(GetField(2));
	field->SetString(fMessage->GetSubject());
	
}

bool MailListItem::HasAttachments() const
{
	return fMessage->IsMultipartMixed();
}

const char *MailListItem::GetUID() const
{
	return fMessage->GetUid();
}

const char *MailListItem::GetMailbox() const
{
	return fMessage->GetMailbox();
}

const char *MailListItem::GetDate() const
{
	return fMessage->GetDate();
}

const char *MailListItem::GetSubject() const
{
	return fMessage->GetSubject();
}

const char *MailListItem::GetAddress() const
{
	return fAddress.String();
}

// --------------------------------------------------------------------
//								MailField
// #pragma mark -
// --------------------------------------------------------------------
MailField::MailField(MimeMessage *mime)
	:	fMessage(mime)
{
	ASSERT(fMessage != NULL);
	
	fMessage->AcquireReference();
}
	
MailField::~MailField()
{
	ASSERT(fMessage != NULL);
	
	fMessage->ReleaseReference();
}

uint32 MailField::Flags()
{
	return fMessage->GetFlags();
}

// --------------------------------------------------------------------
//								ImapStringColumn
// #pragma mark -
// --------------------------------------------------------------------
ImapStringColumn::ImapStringColumn(const char *title, float width, float minWidth,
										float maxWidth, uint32 truncate)
	:	BTitledColumn(title, width, minWidth, maxWidth),
		fTruncate(truncate)
{

}

void ImapStringColumn::DrawField(BField *field, BRect rect, BView *parent)
{
	ImapStringField *stringField = (ImapStringField*) field;
	BFont font = (stringField->Flags() & kMessageSeen) ? *be_plain_font : *be_bold_font;
	font.SetSize(kFontSize);
		
	if (rect.Width() != stringField->Width()) {
		const char *string = stringField->String();
		BString	tmp;
		font.GetTruncatedStrings(&string, 1, fTruncate, rect.Width() - 10, &tmp);
		stringField->SetClippedString(tmp.String());
		stringField->SetWidth(rect.Width());
	}
	DrawFieldProperly(rect, parent, stringField->ClippedString(), &font, stringField->Flags());
}

int ImapStringColumn::CompareFields(BField *field1, BField *field2)
{
	return (strcasecmp(((ImapStringField*)field1)->String(),(((ImapStringField*)field2)->String())));
}

// --------------------------------------------------------------------
//								ImapStringField
// #pragma mark -
// --------------------------------------------------------------------
ImapStringField::ImapStringField(const char *string, MimeMessage *mime)
	:	BStringField(string),
		MailField(mime)
{

}

// --------------------------------------------------------------------
//								ImapDateColumn
// #pragma mark -
// --------------------------------------------------------------------
ImapDateColumn::ImapDateColumn(const char *title, float width, float minWidth, float maxWidth)
	:	BTitledColumn(title, width, minWidth, maxWidth)
{

}

void ImapDateColumn::DrawField(BField *field, BRect rect, BView *parent)
{
	ImapDateField *dateField = dynamic_cast<ImapDateField *>(field);

	bool newMessage = !(dateField->Flags() & kMessageSeen);
	BFont font = (newMessage ? *be_bold_font : *be_plain_font);
	font.SetSize(kFontSize);
	
	if (dateField->Width() != rect.Width()) {
		BString date;
		BString	tmp;
		CalculateRelativeTime(rect.Width() - 8, &date, dateField->UnixTime(), newMessage);
		const char *in = date.String();
		font.GetTruncatedStrings(&in, 1, B_TRUNCATE_END, rect.Width() - 8, &tmp);
		dateField->SetClippedString(tmp.String());
	}
	dateField->SetWidth(rect.Width());
	DrawFieldProperly(rect, parent, dateField->ClippedString(), &font, dateField->Flags());
}

void ImapDateColumn::CalculateRelativeTime(float width, BString *out, time_t when, bool newMessage)
{
	char date[256];
	// Find out the UNIX time for the start of "today"
	time_t today = time(NULL);
	time_t midnight;
	strftime(date, 256, "%a %b %d 00:00:01", localtime(&today));
	midnight = parsedate(date, -1);
	
	// Message date is sometime today, so just show the time...
	if( (when > midnight) && ((midnight + (when - midnight)) < (midnight + kSecondsPerDay)))
	{
		if(kUse24hour)
			strftime(date,256,"Today, %H:%M",localtime(&when));
		else
			strftime(date,256,"Today, %I:%M %p",localtime(&when));
		*out = date;
	}
	// Maybe message was within the past seven days, then show weekday
	else if( (when < midnight) && ( when > (midnight - (kSecondsPerDay * 6))) )
	{
		if(kUse24hour)
			strftime(date,256,"%A, %H:%M",localtime(&when));
		else
			strftime(date,256,"%A, %I:%M %p",localtime(&when));
		*out = date;
	}
	// Otherwise just show date
	else
		DateToString(width, out, when, newMessage);
}

void ImapDateColumn::DateToString(float width,BString *output,time_t time, bool newMessage)
{
	const BFont *font = (newMessage ? be_bold_font : be_plain_font);
	char *buffer = output->LockBuffer(256);
	if (buffer == NULL)
		return;
		
	for(;;)
	{
		// Wednesday, July 01 1998, 11:30 PM
		if(kUse24hour)
			strftime(buffer,256,"%A, %B %d %Y, %H:%M", localtime(&time));
		else
			strftime(buffer,256,"%A, %B %d %Y, %I:%M %p", localtime(&time));
		if(font->StringWidth(buffer) < width) break;
		// Wednesday, Jul 01 1998, 11:30 PM
		if(kUse24hour)
			strftime(buffer,256,"%A, %b %d %Y, %H:%M", localtime(&time));
		else
			strftime(buffer,256,"%A, %b %d %Y, %I:%M %p", localtime(&time));
		if(font->StringWidth(buffer) < width) break;
		// Wed, July 01 1998, 11:30 PM
		if(kUse24hour)
			strftime(buffer,256,"%a, %B %d %Y, %H:%M", localtime(&time));
		else
			strftime(buffer,256,"%a, %B %d %Y, %I:%M %p", localtime(&time));
		if(font->StringWidth(buffer) < width) break;
		// Wed, Jul 01 1998, 11:30 PM
		if(kUse24hour)
			strftime(buffer,256,"%a, %b %d %Y, %H:%M", localtime(&time));
		else
			strftime(buffer,256,"%a, %b %d %Y, %I:%M %p", localtime(&time));
		if(font->StringWidth(buffer) < width) break;
		// Jul 01 1998, 11:30 PM
		if(kUse24hour)
			strftime(buffer,256,"%b %d %Y, %H:%M", localtime(&time));
		else
			strftime(buffer,256,"%b %d %Y, %I:%M %p", localtime(&time));
		if(font->StringWidth(buffer) < width) break;
		// 07/01/98, 11:30 PM
		if(kUse24hour)
			strftime(buffer,256,"%m/%d/%y, %H:%M", localtime(&time));
		else
			strftime(buffer,256,"%m/%d/%y, %I:%M %p", localtime(&time));
		if(font->StringWidth(buffer) < width) break;
		// 07/01/98
		strftime(buffer,256,"%m/%d/%y",localtime(&time));
		break;
	}
	output->UnlockBuffer();
}

int ImapDateColumn::CompareFields(BField *field1, BField *field2)
{
	return ((ImapDateField*) field1)->Seconds() - ((ImapDateField*) field2)->Seconds();
}

// --------------------------------------------------------------------
//								ImapDateField
// #pragma mark -
// --------------------------------------------------------------------
ImapDateField::ImapDateField(time_t *t, MimeMessage *mime)
	:	BDateField(t),
		MailField(mime)
{

}

// --------------------------------------------------------------------
//								ImapIconStatusColumn
// #pragma mark -
// --------------------------------------------------------------------
ImapIconStatusColumn::ImapIconStatusColumn(const char *title, float width, float minWidth, float maxWidth, bool flag)
	:	BTitledColumn(title, width, minWidth, maxWidth),
		fFlagColumn(flag)
{

}

void ImapIconStatusColumn::DrawField(BField *field, BRect rect, BView *parent)
{
	ImapIconStatusField *iconStatusField = dynamic_cast<ImapIconStatusField *>(field);
	iconStatusField->UpdateBitmap();
	const BBitmap *bitmap = iconStatusField->Bitmap();
	if(bitmap != NULL) {
		// Set drawing mode to support proper transparency in PNG's
		parent->SetDrawingMode(B_OP_ALPHA);
		parent->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		float top = ((rect.Height() - iconStatusField->BitmapHeight()) / 2) + rect.top;
		parent->DrawBitmap(bitmap, BPoint(rect.left, top));
	}
	DrawFieldProperly(rect, parent, NULL, NULL, iconStatusField->Flags());
}

int ImapIconStatusColumn::CompareFields(BField *field1, BField *field2)
{
	ImapIconStatusField *leftField = dynamic_cast<ImapIconStatusField *>(field1);
	ImapIconStatusField *rightField = dynamic_cast<ImapIconStatusField *>(field2);

	return (fFlagColumn ?
			(int)(leftField->Flags()) - (int)(rightField->Flags()) :
			(int)(leftField->HasAttachment()) - (int)(rightField->HasAttachment()));
}

// --------------------------------------------------------------------
//								ImapIconStatusField
// #pragma mark -
// --------------------------------------------------------------------
ImapIconStatusField::ImapIconStatusField(icon_type itype, MimeMessage *mime)
	: 	MailField(mime),
		fIconType(itype),
		fBitmap(NULL),
		fBitmapHeight(0)
{
	// Set up the icon bitmap.
	if (fIconType == kAttachmentIcon) {
		if (!fMessage->IsMultipartMixed() )
			fIconType = kNoIcon;
	} else if (mail_author(fMessage->GetSender())) {
		fIconType = kScootIcon;
	}

	UpdateBitmap(true);

	if(fBitmap)
		fBitmapHeight = fBitmap->Bounds().Height();
}

const BBitmap *ImapIconStatusField::Bitmap() const
{
	return fBitmap;
} 

float ImapIconStatusField::BitmapHeight() const
{
	return fBitmapHeight;
}

void ImapIconStatusField::UpdateBitmap(bool force = false)
{
	const char *icon_name = NULL;
	uint32 flags = Flags();
	if (force || (flags != fRecentFlags)) {
		fRecentFlags = flags;
		switch (fIconType) {
			case kMailIcon:
				icon_name = (flags & kMessageDeleted ? "trash_disabled.png" :
								(flags & kMessageSeen ? "mail_read.png" : "mail_new.png"));
				break;
			case kAttachmentIcon:
				icon_name = (flags & kMessageDeleted ? "attachment_disabled.png" : "attachment.png");
				break;
			case kScootIcon:
				icon_name = "scoot.png";
			case kNoIcon:
			default:
				break;
		}
	}
	if (icon_name) // only not null if we should change the icon
		fBitmap = const_cast<BBitmap *>(ResourceBitmap(icon_name));
}

bool ImapIconStatusField::HasAttachment()
{
	return fMessage->IsMultipartMixed();
}

// --------------------------------------------------------------------
//								ImapSizeColumn
// #pragma mark -
// --------------------------------------------------------------------
ImapSizeColumn::ImapSizeColumn(const char *title, float width, float minWidth, float maxWidth)
	:	BTitledColumn(title, width, minWidth, maxWidth)
{

}

void ImapSizeColumn::DrawField(BField *field, BRect rect, BView *parent)
{
	ImapSizeField *sizeField = (ImapSizeField*) field;
	BFont font = (sizeField->Flags() & kMessageSeen) ? *be_plain_font : *be_bold_font;
	font.SetSize(kFontSize);
	
	if (rect.Width() != sizeField->Width()) {
		uint32 size = sizeField->Size();
		char	formatted[256];

		//char *formatted = sizeField->clippedString.LockBuffer(256);
		if (size > 1024 * 1024 * 1024)
			sprintf(formatted, "%.2f Gb", (double) size / (1024 * 1024 * 1024));
		else if (size > 1024 * 1024)
			sprintf(formatted, "%.2f Mb", (double) size / (1024 * 1024));	
		else if (size > 1024)
			sprintf(formatted, "%.2f kb", (double) size / 1024);
		else
			sprintf(formatted, "%ld bytes", size);
		//sizeField->clippedString.UnlockBuffer();
		sizeField->SetClippedString(formatted);
		sizeField->SetWidth(rect.Width());
	}
	DrawFieldProperly(rect, parent, sizeField->ClippedString(), &font, sizeField->Flags());
}

int ImapSizeColumn::CompareFields(BField *field1, BField *field2)
{
	return ((ImapSizeField*) field1)->Size() - ((ImapSizeField*) field2)->Size();
}

// --------------------------------------------------------------------
//								ImapSizeField
// #pragma mark -
// --------------------------------------------------------------------
ImapSizeField::ImapSizeField(MimeMessage *mime)
	:	BStringField(""), MailField(mime)
{

}

uint32 ImapSizeField::Size()
{
	return fMessage->GetSize();
}

// --------------------------------------------------------------------
//								Resources
// #pragma mark -
// --------------------------------------------------------------------
const BBitmap* ResourceBitmap(const char* name)
{
	return Resources().FindBitmap(B_PNG_FORMAT, name);
}

BResourceSet& Resources()
{
	if( gResInitialized ) return gResources;

	gResources.AddEnvDirectory("${/service/web/macros/RESOURCES}/Mail/images",
								"/boot/custom/resources/en/Mail/images");
	gResInitialized = true;
	return gResources;
}

// --------------------------------------------------------------------
//								DrawField
// #pragma mark -
// --------------------------------------------------------------------
void DrawFieldProperly(BRect rect, BView *parent, const char *text, const BFont *font, uint32 flags)
{
	rgb_color originalColor = parent->HighColor();
	rgb_color ourColor = flags & kMessageDeleted ? kDeletedColor : originalColor;

	BFont oldFont;
	parent->GetFont(&oldFont);
	if(text != NULL) {
		parent->SetFont(font);
		parent->SetHighColor(ourColor);
		parent->DrawString(text);
	}
	if(flags & kMessageDeleted) {
		parent->SetHighColor(kDeletedColor);
		parent->StrokeLine(BPoint(rect.left, rect.top + (rect.Height() / 2) + 1), BPoint(rect.right, rect.top + (rect.Height() / 2) + 1));
	}
		
	parent->SetFont(&oldFont);
	parent->SetHighColor(originalColor);
}


