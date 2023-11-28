//==================================================================
//	MTextView.h
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//==================================================================

#ifndef _MTEXTVIEW_H
#define _MTEXTVIEW_H

#include <TextView.h>

const ulong msgTextChanged = 'TXCh';


class MTextView : public BTextView
{
public:
								MTextView(
									BRect			inFrame,
									const char*		inName,
									ulong 			resizeMask = B_FOLLOW_ALL,
									ulong 			flags = B_PULSE_NEEDED | B_WILL_DRAW);
								~MTextView();

virtual	void					MessageReceived(
									BMessage *message);
virtual	void					KeyDown(
									const char *	inBytes, 
									int32 			inNumBytes);
virtual void					Cut(
									BClipboard *clip);
virtual	void					Paste(
									BClipboard *clip);

	void						SetDirty(
									bool inDirty = true);
	long						SetTarget(
									BHandler*	inTarget);
	void						SetMessage(
									BMessage*	inMessage);

	long						GetValue();

	static BRect				GetTextRect(const BRect & area);

protected:

	bool						fDirty;
	BMessage*					fMessage;

private:

	BHandler*					fTarget;
	BLooper*					fOurLooper;
	bool						fIBeam;

	bool						OneCharSelected();
	void						SetCursor();
};

#endif
