//==================================================================
//	MTextView.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//==================================================================

#ifndef _MTEXTVIEW_H
#define _MTEXTVIEW_H

#include <TextView.h>
#include <StringView.h>
#include "MTargeter.h"

const uint32 msgTextChanged			= 'TXCh';


class MTextView : 	public BTextView,
					public MTargeter
{
public:
								MTextView(
									BRect			inFrame,
									const char*		inName,
									uint32 			resizeMask = B_FOLLOW_ALL_SIDES,
									uint32 			flags = B_PULSE_NEEDED | B_WILL_DRAW | B_NAVIGABLE);
								~MTextView();

virtual	void					MessageReceived(
									BMessage *message);
virtual	void					KeyDown(
									const char *bytes, 
									int32 		numBytes);
virtual void					Cut(
									BClipboard *clip);
virtual	void					Paste(
									BClipboard *clip);
virtual	void					MakeFocus(
									bool focusState = TRUE);

	void						SetDirty(
									bool inDirty = true);
	bool						Dirty();

	int32						GetValue();

	static BRect				GetTextRect(const BRect & area);

private:

	bool						fDirty;

	bool						OneCharSelected();
	int32						SelectionSize();
};

inline bool MTextView::Dirty()
{
	return fDirty;
}

#endif
