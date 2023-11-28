//========================================================================
//	MAccessPathsView.h
//	Copyright 1995 - 97 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#ifndef _MACCESSPATHSVIEW_H
#define _MACCESSPATHSVIEW_H

#include "MPreferencesWindow.h"
#include "MPreferencesView.h"
#include "MPrefsStruct.h"
#include "IDEConstants.h"

class String;
class MAccessPathListView;
class MDLOGListView;
class BFilePanel;
class MProjectWindow;
class BRadioButton;
class BListView;
class BCheckBox;

enum AccessPathT
{
	kAbsolutePath,
	kProjectRelativePath,
	kSystemRelativePath,
	kPathToProjectTree,
	kPathToSystemTree
};

struct AccessPathData
{
	void		SwapBigToHost();
	void		SwapHostToBig();

	AccessPathT		pathType;
	bool			recursiveSearch;
	char			pathName[kPathSize];
	char			punused1;
	char			punused2;
	char			punused3;		// pad to 264 bytes
};

enum AccessPathState
{
	aIdle,
	aAdding,
	aChanging,
	aRemoving,
	aDragging
};

typedef MList<AccessPathData*> AccessPathList;


class MAccessPathsView : public MPreferencesView
{
public:
								MAccessPathsView(
									MPreferencesWindow&	inWindow,
									BRect				inFrame);
		virtual					~MAccessPathsView();

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(
									BMessage * inMessage);

	virtual	bool				MessageDropped(	
									BMessage *	aMessage);

	virtual void				DoFactorySettings();

	virtual void				GetData(
									BMessage&	inOutMessage,
									bool		isProxy);
	virtual void				SetData(
									BMessage&	inOutMessage);
	virtual void				LastCall();

	virtual const char *		Title();

	virtual TargetT				Targets();

		void					DoButtonPushed(
									BMessage&	inMessage);
	static void					EmptyList(
									AccessPathList&	inList);

protected:

	AccessPathsPrefs			fOldSettings;
	AccessPathsPrefs			fNewSettings;
	BCheckBox*					fTreater;
	BButton*					fAddDefault;
	BButton*					fAdd;
	BButton*					fChange;
	BButton*					fRemove;
	BRadioButton*				fAbsoluteRB;
	BRadioButton*				fProjectRelRB;
	BRadioButton*				fSystemRelRB;
	MAccessPathListView*		fProjectView;
	MAccessPathListView*		fSystemView;
	AccessPathList				fProjectPathList;		// project path records
	AccessPathList				fSystemPathList;		// system path records
	AccessPathState				fState;
	AccessPathT					fPathType;
	int32						fChangingIndex;
	BFilePanel*					fAddFilePanel;
	MProjectWindow*				fProject;
	
	virtual void				UpdateValues();
	virtual void				ValueChanged();
	virtual void				DoSave();
	virtual void				DoRevert();

	void						EmptyListView(
									BListView&	inList);
	void						AddPathToListView(
									String&				inString,
									AccessPathT			inType,
									MDLOGListView&		inList,
									int32				inIndex = -1,
									bool				inRecursiveSearch = true);
	void						NewAccessPath(
									BMessage&	inMessage);
	void						GetPathForRef(
									entry_ref&		ref,
									String&			outPath,
									AccessPathT&	inOutType);
	bool						PathInList(
									const char * 	inPath,
									MDLOGListView&	inList,
									bool			inShowAlert = true) const;

	void						AdjustPathType();

	void						AddDefaultAccessPath();
	void						RemoveAccessPath();
	void						AddAccessPath();
	void						ChangeAccessPath();
	void						AdjustButtons();
	void						CopyOldPathsToNewPaths();
	void						CopyNewPathsToOldPaths();
	bool						ListsAreDifferent(
									MAccessPathListView&	inView,
									AccessPathList&			inList);

	void						ShowFilePanel(const char* inTitle, 
											  const char* inButtonLabel,
											  entry_ref* inDirectory,
											  AccessPathState inState);
	void						CloseFilePanel();
};

#endif
