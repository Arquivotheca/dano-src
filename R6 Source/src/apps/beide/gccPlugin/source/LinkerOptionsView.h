// ---------------------------------------------------------------------------
/*
	LinkerOptionsView.h
	
	Copyright (c) 1998 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			29 October 1998

*/
// ---------------------------------------------------------------------------

#ifndef _LINKEROPTIONSVIEW_H
#define _LINKEROPTIONSVIEW_H

#include "GCCOptionsView.h"
#include "GCCOptions.h"

class BCheckBox;
class BRect;
class BBox;

// ---------------------------------------------------------------------------
//	Class LinkerOptionsView
// ---------------------------------------------------------------------------

class LinkerOptionsView : public GCCOptionsView<LinkerSettings>
{
public:	
						LinkerOptionsView(BRect inFrame,
										  const char* title,
										  const char* messageName,
										  type_code messageType,
										  UpdateType updateType);
	virtual				~LinkerOptionsView();

	// GCCOptionsView overrides
	virtual void	AttachedToWindow();
	virtual void	UpdateValues();

protected:
	virtual void	GetUserTextChanges();
	virtual void	GetUserCheckBoxesChanges();
	virtual void	GetUserPopupChanges();
	
private:
	BCheckBox*		fCheckBoxList[kMaxLinkerOptions];
};

#endif
