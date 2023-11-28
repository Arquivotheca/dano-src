//========================================================================
//	MMessageView.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
// BDS

#include <string.h>

#include "MMessageView.h"
#include "MMessageItem.h"
#include "BeIDEComm.h"
#include "IDEConstants.h"
#include "IDEMessages.h"
#include "ProjectCommands.h"
#include "ErrorMessage.h"

#include <Message.h>
#include <Window.h>

const float kMessageMargin = 3.0;

// ---------------------------------------------------------------------------
//		MMessageView
// ---------------------------------------------------------------------------
//	Constructor

MMessageView::MMessageView(
	BRect			area,
	const char *	name,
	uint32			resize,
	uint32			flags)
	:	MListView(
		area,
		name,
		resize,
		flags)
{
	fErrors = 0;
	fWarnings = 0;
	fInfos = 0;
	SetMultiSelect(false);
}

// ---------------------------------------------------------------------------
//		MMessageView
// ---------------------------------------------------------------------------
//	Destructor

MMessageView::~MMessageView()
{
	ClearMessages();
}

// ---------------------------------------------------------------------------
//		MessageReceived
// ---------------------------------------------------------------------------

void
MMessageView::MessageReceived(
	BMessage*	inMessage)
{
	int32		row = -1;
	
	switch (inMessage->what)
	{
		case cmd_PrevMessage:
			SelectPrev();
			if (FirstSelected(row))
				InvokeRow(row);
			break;

		case cmd_NextMessage:
			SelectNext();
			if (FirstSelected(row))
				InvokeRow(row);
			break;
		
		default:
			MListView::MessageReceived(inMessage);
			break;
	}
}

// ---------------------------------------------------------------------------
//		DrawRow
// ---------------------------------------------------------------------------

void
MMessageView::DrawRow(
	int32 		/* inRow */,
	void * 		inData,
	BRect 		inFrame,
	BRect		/*inIntersection*/)
{
	ASSERT(inData);

	if (inData)
		((MMessageItem*)inData)->Draw(inFrame, *this);
}

// ---------------------------------------------------------------------------
//		AddNewMessage
// ---------------------------------------------------------------------------
//	An error or warning has been received from the compiler or linker.

void
MMessageView::AddNewMessage(BMessage& inMessage, MessagesType visibleTypes)
{
	ErrorNotificationMessage* error;
	ssize_t length;
	MessagesType myType;
	
	if (B_NO_ERROR == inMessage.FindData(kErrorMessageName, kErrorType, (const void**) &error, &length))
	{
		MMessageItem*			msg = nil;
		
		if (error->isWarning)
		{
			ASSERT(! error->hasErrorRef || error->errorRef.warning);

			// It's a warning
			myType = kWarnings;
			fWarnings++;
			if (error->hasErrorRef)
				msg	= new MWarningMessage(*error);
			else
				msg	= new MWarningMessage(error->errorMessage);
		}
		else
		{
			ASSERT(! error->hasErrorRef || ! error->errorRef.warning);

			// It's an error
			myType = kErrors;
			fErrors++;
			if (error->hasErrorRef)
				msg	= new MErrorMessage(*error);
			else
				msg	= new MErrorMessage(error->errorMessage);
		}

		// Add the new message to the window at the end
		if (msg)
		{
			if (visibleTypes == kErrorsAndWarnings || myType == visibleTypes || visibleTypes == kAllMessages) {
				AddMessageAtEnd(msg);
			}
			fMessageList.AddItem(msg);
		}
	}
}

// ---------------------------------------------------------------------------
//		AddMessageAtEnd
// ---------------------------------------------------------------------------
// Add the new message to the window at the end

void
MMessageView::AddMessageAtEnd(
	MMessageItem * inMessage)
{
	// If this is the first message, and the window is hidden,
	// open it up so the user can see the messages.
	if (this->CountRows() == 0 && Window()->IsHidden()) {
		Window()->PostMessage(msgShowAndActivate);
	}

	float	lineHeight = fFontHeight.ascent + fFontHeight.descent + fFontHeight.leading;
	float	cellHeight = lineHeight * inMessage->GetLineCount() + kMessageMargin;
	
	if (cellHeight < kMessageItemMinHeight)
		cellHeight = kMessageItemMinHeight + kMessageMargin;
	
	InsertRow(CountRows(), inMessage, cellHeight);
}

// ---------------------------------------------------------------------------
//		AddNewInfo
// ---------------------------------------------------------------------------
//	An info message has come from the find window.

void
MMessageView::AddNewInfo(BMessage& inMessage, MessagesType visibleTypes)
{
	ssize_t			len;
	InfoStruct* 	infoStruct;
	int32			index = 0;

	while (B_NO_ERROR == inMessage.FindData(kInfoStruct, kInfoType, index++, (const void**) &infoStruct, &len))
	{
		MInfoMessage*	newInfo = new MInfoMessage(*infoStruct, inMessage.what);
		if (visibleTypes == kInfos || visibleTypes == kAllMessages) {
			AddMessageAtEnd(newInfo);
		}
		fMessageList.AddItem(newInfo);
		
		fInfos++;
	}
}

// ---------------------------------------------------------------------------

void
MMessageView::AddNewDocInfo(BMessage& inMessage, MessagesType visibleTypes)
{
	// An info (actually documentation lookup info) has come from documentation lookup
	// Add the list to the window
	ssize_t len;
	DocumentationLookupEntry* docInfo;
	int32 index = 0;

	while (inMessage.FindData(kDocInfo, kInfoType, index++, (const void**) &docInfo, &len) == B_OK) {
		MDocumentationMessage* newMessage = new MDocumentationMessage(*docInfo);
		if (visibleTypes == kInfos || visibleTypes == kAllMessages) {
			AddMessageAtEnd(newMessage);
		}
		fMessageList.AddItem(newMessage);
		fInfos++;
	}
}

// ---------------------------------------------------------------------------
//		AddErrors
// ---------------------------------------------------------------------------
//	An errors message has come from a non-ideAware compiler.

void
MMessageView::AddErrors(BMessage& inMessage, MessagesType visibleTypes)
{
	ErrorNotificationMessage notification;
	type_code type;
	int32 count;
	MessagesType myType;
	
	if (B_NO_ERROR == inMessage.GetInfo(kPlugInErrorName, &type, &count))
	{
		for (int32 i = 0; i < count; i++)
		{
			ssize_t			len;
			ErrorMessage*	error;

			if (B_NO_ERROR == inMessage.FindData(kPlugInErrorName, type, i, (const void**) &error, &len))
			{
				MMessageItem*	msg = nil;
				
				if (error->isWarning)
				{
					// It's a warning
					fWarnings++;
					myType = kWarnings;
					if (error->textonly)
						msg	= new MWarningMessage(error->errorMessage);
					else
					{
						notification.isWarning = true;
						notification.hasErrorRef = true;
						notification.errorRef.linenumber = error->linenumber;
						notification.errorRef.offset = error->offset;
						notification.errorRef.length = error->length;
						notification.errorRef.synclen = error->synclen;
						notification.errorRef.syncoffset = error->syncoffset;
						notification.errorRef.errorlength = error->errorlength;
						notification.errorRef.erroroffset = error->erroroffset;
						memcpy(notification.errorRef.sync, error->sync, 32);
						strncpy(notification.errorMessage, error->errorMessage, MAX_ERROR_LENGTH);
						int32	len = min(strlen(error->filename), (size_t) MAX_ERROR_PATH_LENGTH-1);
						strncpy(&notification.errorRef.errorfile.name[1], error->filename, len);
						notification.errorRef.errorfile.name[0] = len;
	
						msg	= new MWarningMessage(notification);
					}
				}
				else
				{
					// It's an error
					fErrors++;
					myType = kErrors;
					if (error->textonly)
						msg	= new MErrorMessage(error->errorMessage);
					else
					{
						notification.isWarning = false;
						notification.hasErrorRef = true;
						notification.errorRef.linenumber = error->linenumber;
						notification.errorRef.offset = error->offset;
						notification.errorRef.length = error->length;
						notification.errorRef.synclen = error->synclen;
						notification.errorRef.syncoffset = error->syncoffset;
						notification.errorRef.errorlength = error->errorlength;
						notification.errorRef.erroroffset = error->erroroffset;
						memcpy(notification.errorRef.sync, error->sync, 32);
						strncpy(notification.errorMessage, error->errorMessage, MAX_ERROR_LENGTH);
						int32	len = min(strlen(error->filename), (size_t) MAX_ERROR_PATH_LENGTH-1);
						strncpy(&notification.errorRef.errorfile.name[1], error->filename, len);
						notification.errorRef.errorfile.name[0] = len;
	
						msg	= new MErrorMessage(notification);
					}
				}
	
				// Add the new message to the window at the end
				if (msg)
				{
					if (visibleTypes == kErrorsAndWarnings || visibleTypes == myType || visibleTypes == kAllMessages) {
						AddMessageAtEnd(msg);
					}
					fMessageList.AddItem(msg);
				}
			}
		}
	}
}

// ---------------------------------------------------------------------------
//		Update
// ---------------------------------------------------------------------------
//	Hide or show messages of the specified type.  inType specifies those
//	messages to be visible.

void
MMessageView::Update(
	MessagesType inType)
{
	bool			showInfos = inType == kInfos;
	bool			showWarnings = inType == kWarnings || inType == kErrorsAndWarnings;
	bool			showErrors = inType == kErrors || inType == kErrorsAndWarnings;
	MMessageItem*	item;
	MMessageItem*	typeditem;
	BList&			list = *GetList();
	int32			rangeTop = -1;
	int32			rangeBottom = -1;
	int32			i = 0;

	while (fMessageList.GetNthItem(item, i++))
	{
		typeditem = nil;

		// Is it supposed to be visible?
		if (showInfos)
			typeditem = dynamic_cast<MInfoMessage*>(item);
		if (showWarnings && !typeditem)
			typeditem = dynamic_cast<MWarningMessage*>(item);
		if (showErrors && !typeditem)
			typeditem = dynamic_cast<MErrorMessage*>(item);
		
		// Add it if it's not there and is supposed to be or
		// remove it if it's there and shouldn't be
		int32		index = list.IndexOf(item);
		
		if (index == -1 && typeditem != nil)
			AddMessageAtEnd(item);
		else 
		if (index >= 0 && typeditem == nil)
		{
			if (rangeTop < 0)			// first to be hidden
			{
				rangeTop = index;
				rangeBottom = index;
			}
			else
			if (index == rangeTop + 1)	// extend range
			{
				rangeTop = index;
			}
			else						// end of the range so hide em
			{
				RemoveRows(rangeBottom, rangeTop - rangeBottom + 1);
				// Our current index can shift in the RemoveRows if
				// we deleted rows previous to where we are.
				// Get it again...
				 if (index > rangeBottom) {
					index = list.IndexOf(item);
				}
				rangeTop = index;
				rangeBottom = index;
			}
		}
	}

	// hide the last range
	if (rangeTop >= 0)
	{
		RemoveRows(rangeBottom, rangeTop - rangeBottom + 1);	
	}
}

// ---------------------------------------------------------------------------

void
MMessageView::ClearMessages()
{
	MMessageItem*	item;
	BList&			list = *GetList();
	int32			rangeTop = -1;
	int32			rangeBottom = -1;
	
	int32	i = fMessageList.CountItems() - 1;
	while (fMessageList.GetNthItem(item, i--))
	{
		fMessageList.RemoveItem(item);
		int32	index = list.IndexOf(item);

		if (index >= 0)
		{
			if (rangeTop < 0)
			{
				rangeTop = index;
				rangeBottom = index;
			}
			else
			if (index == rangeBottom - 1)
			{
				rangeBottom = index;
			}
			else
			{
				DeleteRows(rangeBottom, rangeTop - rangeBottom + 1);
				// Our current index can shift in the RemoveRows if
				// we deleted rows previous to where we are.
				// Get it again...
			 	if (index > rangeBottom) {
					index = list.IndexOf(item);
				}
				rangeTop = index;
				rangeBottom = index;
			}
		}
	}

	if (rangeTop >= 0)
	{
		DeleteRows(rangeBottom, rangeTop - rangeBottom + 1);	
	}

	fInfos = 0;
	fWarnings = 0;
	fErrors = 0;
}

// ---------------------------------------------------------------------------
//		HideMessages
// ---------------------------------------------------------------------------

void
MMessageView::HideMessages(
	MessagesType inType)
{
	bool			hideInfos = inType == kInfos || inType == kAllMessages;
	bool			hideWarnings = inType == kErrorsAndWarnings || inType == kAllMessages;;
	bool			hideErrors = hideWarnings;
	MMessageItem*	item;
	MMessageItem*	typeditem;
	BList&			list = *GetList();
	int32			rangeTop = -1;
	int32			rangeBottom = -1;

	for (int32 i = list.CountItems() - 1; i >= 0; i--)
	{
		item = (MMessageItem*) list.ItemAt(i);
		typeditem = nil;

		// Is it supposed to be hidden?
		if (hideInfos)
			typeditem = dynamic_cast<MInfoMessage*>(item);
		if (hideWarnings && !typeditem)
			typeditem = dynamic_cast<MWarningMessage*>(item);
		if (hideErrors && !typeditem)
			typeditem = dynamic_cast<MErrorMessage*>(item);
		
		if (typeditem != nil)
		{
			if (rangeTop < 0)			// first to be hidden
			{
				rangeTop = i;
				rangeBottom = i;
			}
			else
			if (i == rangeBottom - 1)	// extend range
			{
				rangeBottom = i;
			}
			else						// end of the range so hide em
			{
				RemoveRows(rangeBottom, rangeTop - rangeBottom + 1);
				rangeTop = i;
				rangeBottom = i;
			}
		}
	}
	
	// hide the last range
	if (rangeTop >= 0)
	{
		RemoveRows(rangeBottom, rangeTop - rangeBottom + 1);	
	}
}

// ---------------------------------------------------------------------------
//		DeleteItem
// ---------------------------------------------------------------------------

void
MMessageView::DeleteItem(
	void* inItem)
{
	delete (MMessageItem*) inItem;
}

// ---------------------------------------------------------------------------
//		InvokeRow
// ---------------------------------------------------------------------------

void
MMessageView::InvokeRow(
	int32 	row)
{
	ASSERT(row >=0 && row < CountRows());
	MMessageItem * 		item = (MMessageItem *) GetList()->ItemAt(row);
	
	if (item)
		item->Invoke();
}

// ---------------------------------------------------------------------------
//		CopyAllText
// ---------------------------------------------------------------------------
//	Return all the text in the window, delimited by newlines, in the String
//	object.

void
MMessageView::CopyAllText(
	String&		inText)
{
	int32		count = CountRows();
	BList*		list = GetList();
	int32		i;
	int32		textLen = 0;

	// How much text is there?
	for (i = 0; i < count; i++)
	{
		MMessageItem * 		item = (MMessageItem *) list->ItemAt(i);
		textLen += item->TextLength();
	}
	
	// Copy it all to a buffer
	char *		text = new char[textLen + count];
	char *		text1 = text;
	int32		len;

	for (i = 0; i < count; i++)
	{
		MMessageItem * 		item = (MMessageItem *) list->ItemAt(i);
		len = item->TextLength();

		memcpy(text1, item->Text(), len);
		text1 += len;
		*text1 = '\n';
		text1++;
	}

	// Copy the buffer to the String object
	len = text1 - text - 1;

	inText.Set(text, len);
	
	delete [] text;
}

// ---------------------------------------------------------------------------
//		AdjustAllRowHeights
// ---------------------------------------------------------------------------
//	Adjust the heights of all rows after the font or fontsize change.

void
MMessageView::AdjustAllRowHeights()
{
	const float		lineHeight = fFontHeight.ascent + fFontHeight.descent + fFontHeight.leading;
	int32			count = CountRows();
	BList*			list = GetList();

	for (int32 i = 0; i < count; i++)
	{
		MMessageItem * 		item = (MMessageItem *) list->ItemAt(i);
		
		float	cellHeight = lineHeight * item->GetLineCount() + kMessageMargin;
		
		if (cellHeight < kMessageItemMinHeight)
			cellHeight = kMessageItemMinHeight + kMessageMargin;

		SetRowHeight(i, cellHeight);		// This is a bit slow with a lot of message items
	}
}

