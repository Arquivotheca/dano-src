// ---------------------------------------------------------------------------
/*
	LanguageOptionsView.h
	
	Copyright (c) 1998 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			28 October 1998

*/
// ---------------------------------------------------------------------------

#ifndef _LANGUAGEOPTIONSVIEW_H
#define _LANGUAGEOPTIONSVIEW_H

#include "GCCOptionsView.h"
#include "GCCOptions.h"

class BCheckBox;
class BRect;
class BBox;
class BPopUpMenu;

// ---------------------------------------------------------------------------
//	Class LanguageOptionsView
// ---------------------------------------------------------------------------

class LanguageOptionsView : public GCCOptionsView<LanguageSettings>
{
public:	
						LanguageOptionsView(BRect inFrame,
										    const char* title,
										    const char* messageName,
										    type_code messageType,
										    UpdateType updateType);
	virtual				~LanguageOptionsView();

	// GCCOptionsView overrides
	virtual void	AttachedToWindow();
	virtual void	UpdateValues();

protected:
	virtual void	GetUserTextChanges();
	virtual void	GetUserCheckBoxesChanges();
	virtual void	GetUserPopupChanges();
	
private:
	BCheckBox*		fCheckBoxList[kMaxLanguageOptions];
	BPopUpMenu*		fLanguagePopup;
};

#endif
