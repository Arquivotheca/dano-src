//========================================================================
//	MMessageView.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
// BDS

#ifndef _MMESSAGEVIEW_H
#define _MMESSAGEVIEW_H

#include "MListView.h"
#include "MList.h"

class MErrorMessage;
class MMessageItem;
class String;

enum MessagesType 
{
	kNoMessages,
	kAllMessages,
	kErrorsAndWarnings,
	kErrors,
	kWarnings,
	kInfos
};


class MMessageView : public MListView
{
public:

								MMessageView(
									BRect			area,
									const char* 	name = NULL,
									uint32 			resize = B_FOLLOW_ALL_SIDES,
									uint32 			flags = B_WILL_DRAW | B_FRAME_EVENTS);
								~MMessageView();

virtual	void					MessageReceived(BMessage *message);

		void					AddNewMessage(BMessage& inMessage, MessagesType visibleTypes);
		void					AddNewInfo(BMessage& inMessage, MessagesType visibleTypes);
		void					AddErrors(BMessage& inMessage, MessagesType visibleTypes);
		void					AddNewDocInfo(BMessage& inMessage, MessagesType visibleTypes);

		void					AddMessageAtEnd(MMessageItem* inMessage);

		void					ClearMessages();
		void					HideMessages(MessagesType inType);
		void					Update(MessagesType inType);

virtual	void					InvokeRow(int32 row);
		void					CopyAllText(String&		inText);
		void					AdjustAllRowHeights();

		int32					Errors()
								{
									return fErrors;
								}
		int32					Warnings()
								{
									return fWarnings;
								}
		int32					Infos()
								{
									return fInfos;
								}

protected:

		int32					fErrors;
		int32					fWarnings;
		int32					fInfos;
		MList<MMessageItem*>	fMessageList;

virtual	void					DrawRow(
									int32 	index,
									void * 	data,
									BRect 	rowArea,
									BRect 	intersectionRect);

virtual	void					DeleteItem(
									void* 	inItem);
};

#endif
