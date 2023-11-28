//========================================================================
//	MPlugInShepard.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#ifndef _MPLUGINSHEPARD_H
#define _MPLUGINSHEPARD_H

#include "MPreferencesView.h"

class MPlugInPrefsView;


class MPlugInShepard : public MPreferencesView
{
public:
								MPlugInShepard(
									MPreferencesWindow&	inWindow,
									MPlugInPrefsView&	inView,
									BRect				inFrame,
									const char*			inName);
		virtual					~MPlugInShepard();

	virtual void				Show();
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

	virtual const char *		Title();

	virtual TargetT				Targets();
	virtual void				ValueChanged();
	virtual void				UpdateValues();
	virtual void				SetDirty(
									bool	inDirty);
	virtual bool				IsDirty();
	void						AdjustDirtyState();

private:

	MPlugInPrefsView&		fView;
	bool						fPointersAreGood;
};

#endif
