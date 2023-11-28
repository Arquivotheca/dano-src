// ---------------------------------------------------------------------------
/*
	CodeGenerationOptionsView.h
	
	Copyright (c) 1998 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			29 October 1998

*/
// ---------------------------------------------------------------------------

#ifndef _CODEGENERATIONOPTIONSVIEW_H
#define _CODEGENERATIONOPTIONSVIEW_H

#include "GCCOptionsView.h"
#include "GCCOptions.h"

class BCheckBox;
class BRect;
class BBox;
class BPopUpMenu;

// ---------------------------------------------------------------------------
//	Class CodeGenerationOptionsView
// ---------------------------------------------------------------------------

class CodeGenerationOptionsView : public GCCOptionsView<CodeGenerationSettings>
{
public:	
						CodeGenerationOptionsView(BRect inFrame,
												  const char* title,
												  const char* messageName,
												  type_code messageType,
												  UpdateType updateType);
	virtual				~CodeGenerationOptionsView();

	// GCCOptionsView overrides
	virtual void	AttachedToWindow();
	virtual void	UpdateValues();

protected:
	virtual void	GetUserTextChanges();
	virtual void	GetUserCheckBoxesChanges();
	virtual void	GetUserPopupChanges();
	
private:
	void			DisableExclusiveBoxes();

	// since the optimization levels are done through the popup, we waste some
	// check box list space here (we have more than we need), but I don't want to
	// worry about order of the enumerations for both allocation here and indexing
	// in the code
	BCheckBox*		fCheckBoxList[kMaxCodeGenerationOptions];
	BPopUpMenu*		fOptimizationPopup;
};

#endif
