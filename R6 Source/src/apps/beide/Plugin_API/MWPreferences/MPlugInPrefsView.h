//========================================================================
//	MPlugInPrefsView.h
//	Copyright 1996 - 97 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MPLUGINPREFSVIEW_H
#define _MPLUGINPREFSVIEW_H

#include <View.h>
#include "PlugInPreferences.h"
class BMessage;

class MPlugInPrefsView : public BView
{

public:

								MPlugInPrefsView(
									BRect			inFrame,
									const char*		inName,
									ulong			inResizeMask,
									ulong			inFlags)
								: BView(inFrame, inName, inResizeMask, inFlags),
								fDirty(FALSE)
								{
								}

	virtual const char *		Title() = 0;
	virtual TargetT				Targets() = 0;

	virtual void				GetData(
									BMessage&	inOutMessage) = 0;
	virtual void				SetData(
									BMessage&	inOutMessage) = 0;

	virtual void				GetPointers(
									void*&	outOld,
									void*&	outNew,
									long&	outLength) = 0;
	virtual void				DoSave() = 0;
	virtual void				DoRevert() = 0;
	virtual void				DoFactorySettings() = 0;
	virtual void				UpdateValues() = 0;
	virtual void				ValueChanged() = 0;

	virtual	bool				FilterKeyDown(
									ulong aKey) = 0;	
	virtual	bool				ProjectRequiresUpdate(
									UpdateType inType) = 0;
									
	void						SetDirty(
									bool inDirty = true)
								{
									fDirty = inDirty;
								}

	bool						IsDirty()
								{
									return fDirty;
								}
private:

	bool						fDirty;
};

#endif
