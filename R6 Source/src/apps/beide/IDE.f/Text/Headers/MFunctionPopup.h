//========================================================================
//	MFunctionPopup.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MFUNCTIONPOPUP_H
#define _MFUNCTIONPOPUP_H

#include "MPopupMenu.h"
#include "MFunctionParser.h"

class MIDETextView;


class MFunctionPopup : public MPopupMenu
{
public:
								MFunctionPopup(
									MIDETextView&		inTextView,
									const BWindow&		inWindow,
									bool				inOpposite);
								~MFunctionPopup();

	void						AttachedToWindow();

	void						SelectAFunction(
									BMenuItem* inItem);
	static void					SortAllPopups(
									bool	inSort);

private:

		MIDETextView&		fTextView;
		MFunctionParser		fParser;

static bool					sSort;

	void						BuildPopUpText(	
									bool inSort);
};

#endif
