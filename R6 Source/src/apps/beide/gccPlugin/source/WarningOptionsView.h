// ---------------------------------------------------------------------------
/*
	WarningOptionsView.h
	
	Copyright (c) 1998 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			29 October 1998

*/
// ---------------------------------------------------------------------------

#ifndef _WARNINGOPTIONSVIEW_H
#define _WARNINGOPTIONSVIEW_H

#include "GCCOptionsView.h"
#include "GCCOptions.h"

class BCheckBox;
class BRect;
class BBox;
class BPopUpMenu;

// ---------------------------------------------------------------------------
//	Class WarningOptionsView
// ---------------------------------------------------------------------------

class WarningOptionsView : public GCCOptionsView<WarningSettings>
{
public:	
						WarningOptionsView(BRect inFrame,
										    const char* title,
										    const char* messageName,
										    type_code messageType,
										    UpdateType updateType);
	virtual				~WarningOptionsView();

	// GCCOptionsView overrides
	virtual void	AttachedToWindow();
	virtual void	UpdateValues();

protected:
	virtual void	GetUserTextChanges();
	virtual void	GetUserCheckBoxesChanges();
	virtual void	GetUserPopupChanges();
	
private:
	BCheckBox*		fCheckBoxList[kMaxWarningOptions];
	BPopUpMenu*		fWarningPopup;
};

// ---------------------------------------------------------------------------
//	Class CommonWarningOptionsView
// ---------------------------------------------------------------------------

class CommonWarningOptionsView : public GCCOptionsView<CommonWarningSettings>
{
public:	
						CommonWarningOptionsView(BRect inFrame,
											     const char* title,
											     const char* messageName,
											     type_code messageType,
											     UpdateType updateType);
	virtual				~CommonWarningOptionsView();

	// GCCOptionsView overrides
	virtual void	AttachedToWindow();
	virtual void	UpdateValues();

protected:
	virtual void	GetUserTextChanges();
	virtual void	GetUserCheckBoxesChanges();
	virtual void	GetUserPopupChanges();

private:
	void			DisableExclusiveBoxes();
	
private:
	BCheckBox*		fCheckBoxList[kMaxCommonWarningOptions];
};

#endif
