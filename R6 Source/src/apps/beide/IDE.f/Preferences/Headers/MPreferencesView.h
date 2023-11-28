//========================================================================
//	MPreferencesView.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#ifndef _MPREFERENCESVIEW_H
#define _MPREFERENCESVIEW_H

#include <View.h>

#include "MPreferencesWindow.h"
#include "PlugInPreferences.h"
class String;


class MPreferencesView : public BView
{
	friend class MPlugInShepard;
public:
								MPreferencesView(
									MPreferencesWindow&	inWindow,
									BRect				inFrame,
									const char*			inName,
									uint32				inResizeMask = B_FOLLOW_LEFT+ B_FOLLOW_TOP,
									uint32				inFlags = B_WILL_DRAW);
		virtual					~MPreferencesView();

	virtual void				Hide();
	virtual void				LastCall();
	virtual void				DoSave();
	virtual void				DoRevert();
	virtual void				DoFactorySettings();
	virtual	bool				ProjectRequiresUpdate(
									UpdateType inType);

	virtual void				GetData(
									BMessage&	inOutMessage,
									bool		isProxy);
	virtual void				SetData(
									BMessage&	inOutMessage);

	virtual const char *		Title() = 0;
	virtual TargetT				Targets();
	virtual MakeActionT			Actions();

	virtual void				ValueChanged();

	virtual	void				SetDirty(
									bool inDirty = true);
	virtual	bool				IsDirty();

		void					SetPointers(
									void*	inOld,
									void*	inNew,
									int32	inLength)
								{
									fOldSettingsP = inOld;
									fNewSettingsP = inNew;
									fSettingsLength = inLength;
								}

protected:

	virtual void				UpdateValues();

private:

	MPreferencesWindow&			fWindow;
	bool						fDirty;
	void*						fOldSettingsP;
	void*						fNewSettingsP;
	int32						fSettingsLength;
};

#endif
