//========================================================================
//	MPEFView.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#ifndef _MPEFVIEW_H
#define _MPEFVIEW_H

#include "MPlugInPrefsView.h"
#include "MPrefsStruct.h"

//class String;
//class MTextView;
class BPopUpMenu;

class MPEFView : public MPlugInPrefsView
{
public:
								MPEFView(
									BRect		inFrame);
		virtual					~MPEFView();

		void					MessageReceived(
									BMessage * inMessage);
	virtual	void				AttachedToWindow();

	virtual const char *		Title();
	virtual TargetT				Targets();
	virtual void				GetData(
									BMessage&	inOutMessage);
	virtual void				SetData(
									BMessage&	inOutMessage);

	virtual void				GetPointers(
									void*&	outOld,
									void*&	outNew,
									long&	outLength);
	virtual void				DoSave();
	virtual void				DoRevert();
	virtual void				DoFactorySettings();
	virtual void				ValueChanged();
	virtual	bool				FilterKeyDown(
									ulong aKey);	
	virtual	bool				ProjectRequiresUpdate(
									UpdateType inType);

private:

	BPopUpMenu*					fExportSymbolsPopup;
	PEFPrefs					fOldSettings;
	PEFPrefs					fNewSettings;

	void						UpdateValues();
	void						ExtractInfo();
};

#endif
