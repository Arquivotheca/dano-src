//========================================================================
//	MTargetView.h
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#ifndef _MTARGETVIEW_H
#define _MTARGETVIEW_H

#include "MPreferencesWindow.h"
#include "MPreferencesView.h"
#include "MTargetTypes.h"
#include "MPrefsStruct.h"

class String;
class MTargetListView;
class BPopUpMenu;
class BTextView;


typedef MList<TargetRec*> TargetList;
//typedef MList<MPlugInLinker*> LinkerList;



class MTargetView : public MPreferencesView
{
public:
								MTargetView(
									MPreferencesWindow&	inWindow,
									BRect				inFrame);
		virtual					~MTargetView();

		void					MessageReceived(
									BMessage * inMessage);

	virtual void				GetData(
									BMessage&	inOutMessage,
									bool		isProxy);
	virtual void				SetData(
									BMessage&	inOutMessage);
	virtual	void				AttachedToWindow();

	virtual void				GetTitle(
									String&	inString);
	virtual const char *		Title();
	virtual TargetT				Targets();
	virtual	bool				ProjectRequiresUpdate(
									UpdateType inType);
	virtual void				DoRevert();
	virtual void				DoSave();

	void						SetupTools(
									MList<char*>&	inList);
	const char *				CurrentLinkerName()
								{
									return fNewSettings.pLinkerName;
								}

	static void					EmptyList(
									TargetList&	inList);

private:

	BPopUpMenu*					fTargetPopup;
	BPopUpMenu*					fFlagsPopup;
	BPopUpMenu*					fToolsPopup;
	MTargetListView*			fListView;
	BTextView*					fFileTypeBox;
	BTextView*					fFileExtensionBox;
	BButton*					fAddButton;
	BButton*					fChangeButton;
	BButton*					fRemoveButton;
	TargetList					fOldTargets;
	TargetRec					fCurrentRec;
	TargetPrefs					fNewSettings;
	TargetPrefs					fOldSettings;
	MPlugInLinker*				fLinker;

	virtual void				DoFactorySettings();
	virtual void				ValueChanged();

	void						HandleFlagsPopup(
									BMessage&	inMessage);
	void						TargetPopupChanged(
									BMessage&	inMessage);
	void						SetTargetPopup();
	void						UpdateButtons();
	void						SelectionChanged();
	void						AddNewRecord();
	void						ChangeRecord();
	void						RemoveRecord();
	bool						FieldsChanged();
	void						FieldsToRecord(
									TargetRec&	inRec);
	void						ClearCurrentRecord();
	void						ResetFields();

	void						CopyNewRecsToOld();
	void						CopyOldRecsToNew();
	void						AddRecToListView(
									TargetRec*		inRec);
	bool						ListsAreDifferent(
									MTargetListView&	inView,
									TargetList&			inList);
	bool						RecordExists(
									TargetRec&		inRec);
};

#endif
