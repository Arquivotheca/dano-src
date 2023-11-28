//--------------------------------------------------------------------
//	
//	EditTextView.h
//
//	Written by: Robert Polic
//	
//	Copyright 1995 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef EDIT_TEXT_VIEW_H
#define EDIT_TEXT_VIEW_H

#include <Application.h>
#include <List.h>
#include <Looper.h>
#include <Rect.h>
#include <TextView.h>
#include <View.h>

enum edit_messages {KILL_EDIT_SAVE = 1000, KILL_EDIT_NOSAVE, KILL_TITLE_SAVE,
					PREV_FIELD, NEXT_FIELD, SAVE_DATA};


//====================================================================

class TEditTextView : public BTextView {
private:

bool			fFlag;

public:
				TEditTextView(BRect, BRect, bool flag = FALSE);
virtual	void	AttachedToWindow(void);
virtual void	KeyDown(const char*, int32);
};
#endif
