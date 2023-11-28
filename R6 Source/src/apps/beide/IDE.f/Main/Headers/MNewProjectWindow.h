//========================================================================
//	MNewProjectWindow.h
//	Copyright 1998 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
// BDS

#ifndef _MNEWPROJECTWINDOW_H
#define _MNEWPROJECTWINDOW_H

#include "CString.h"
#include "MList.h"
#include <Entry.h>
#include <InterfaceKit.h>

class MTriangleListView;

struct StationeryItem
{
	entry_ref		ref;				// ref of folder
	String			projectName;		// name of project file in this folder
	bool			isEmptyProject;		// empty project item?
	bool			hasProject;			// folder has project in it?
	bool			subHasProject;		// subfolder of this one has project in it?
};
//typedef MList<StationeryItem*> StationeryList;

struct StationeryFolderItem
{
								~StationeryFolderItem();
		
	void						AddToView(
									BOutlineListView& 	inView,
									int32				inDepth);


	StationeryItem					fItem;
	MList<StationeryFolderItem*>	fList;
};

class StationeryRow : public BListItem
{
public:
								StationeryRow(
									StationeryItem&		inData,
									uint32 				outlineLevel, 
									bool 				expanded);
									
virtual							~StationeryRow();

bool							IsProject();
			
virtual	void					DrawItem(BView *owner,
										BRect bounds,
										bool complete);
void							SetParentFont(
									BView *owner);
const StationeryItem*			DataItem()
								{
									return &fData;
								}

private:

	StationeryItem		fData;
	BFont				fItalicFont;
	bool				fHasFont;
};


class MNewProjectWindow : public BWindow
{
public:

								MNewProjectWindow();
								~MNewProjectWindow();

		void					MessageReceived(
									BMessage * message);
virtual	void					WindowActivated(
									bool state);
virtual	bool					QuitRequested();

	status_t					GetStationeryFolder(
									entry_ref& inRef);
static	BFilePanel*				CreateProject(
									BMessage*	inMessage);
static	void					SaveProject(
									BMessage*	inMessage);
static	void					CreateNewProject(
									const char *		inNewProjectName,
									const char *		inStationeryName,
									const entry_ref&	inFolderRef,
									const entry_ref&	inStationeryRef,
									bool				inCreateFolder,
									bool				inCreateEmptyProject);

private:

		BOutlineListView*		fListView;
		BCheckBox*				fCreateFolderCB;
		BButton*				fOKButton;

		void					BuildWindow();
		void					BuildListView();
		bool					ExtractInfo(
									BMessage* inMessage);
};

#endif
