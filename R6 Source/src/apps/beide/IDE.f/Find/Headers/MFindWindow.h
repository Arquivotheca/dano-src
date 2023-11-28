//========================================================================
//	MFindWindow.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	The Find window.
//	BDS

#ifndef _MFINDWINDOW_H
#define _MFINDWINDOW_H

#include "MSourceFileList.h"
#include "MFindThing.h"
#include "MFindData.h"
#include "MFileSet.h"

#include <AppDefs.h>
#include <Locker.h>
#include <MessageFilter.h>
#include <Window.h>

class MTextWindow;
class MIDETextView;
class MTextView;
class MMultiFileListView;
class MProjectWindow;
class MMultiFindThread;
class MSaveFileSetWindow;
class MRemoveFileSetWindow;
class MPictureMenuBar;
class MPictureButton;
class MFindWindow;

class BCheckBox;
class BPictureButton;
class BPopUpMenu;
class BMenuField;
class BMenu;

class FindWindowDropFilter : public BMessageFilter
{
public:
							FindWindowDropFilter(
								MFindWindow&	inWindow)
							: BMessageFilter(B_DROPPED_DELIVERY,B_ANY_SOURCE, B_SIMPLE_DATA),
							fFindWindow(inWindow){}

virtual	filter_result		Filter(
								BMessage *message, 
								BHandler **target);

private:
		MFindWindow&		fFindWindow;
};


class MFindWindow : public BWindow
{
	friend class MFindThing;
	friend class FindWindowDropFilter;
public:
								MFindWindow();
		virtual					~MFindWindow();

virtual	void					MessageReceived(
									BMessage * inMessage);
virtual	void					WindowActivated(
									bool inActive);
virtual	void					DispatchMessage(
									BMessage *message, 
									BHandler *receiver);
virtual	bool					QuitRequested();
virtual	void					Show();
virtual	void					Hide();

		bool					DoFindNext(
									MIDETextView& 	inTextView,
									bool			inForward);
		bool					DoFindSelection(
									MIDETextView& 	inTextView,
									const char *	inText,
									int32			inLength,
									bool			inForward);

		bool					DoReplace(
									MIDETextView& 	inTextView);
		void					DoReplaceAndFind(
									MIDETextView& 	inTextView,
									bool			inForward);
		void					DoReplaceAll(
									MIDETextView& 	inTextView);

		bool					HasFindString();
		bool					CanFindInNextFile();
		bool					CanReplaceAll();


	static void					TextWindowClosed(
									bool	inPostMesage = true);
	static void					RemovePreferences();
	static	MFindWindow&		GetFindWindow();

private:

		MTextView*				fFindStringBox;
		MTextView*				fReplaceStringBox;
		BButton*				fFindButton;
		BButton*				fReplaceButton;
		BButton*				fReplaceAndFindButton;
		BButton*				fReplaceAllButton;
		BCheckBox*				fBatchCB;
		BCheckBox*				fWrapCB;
		BCheckBox*				fIgnoreCaseCB;
		BCheckBox*				fEntireWordCB;
		BCheckBox*				fRegexpCB;

		MMultiFileListView*		fListView;
		BCheckBox*				fSourcesCB;
		BCheckBox*				fSystemHeadersCB;
		BCheckBox*				fProjectHeadersCB;
		BCheckBox*				fOtherCB;
		BCheckBox*				fStopAtEOFCB;
		BPictureButton*			fMultFindButton;
		BPictureButton*			fTriangleButton;
		BButton*				fOtherButton;

		MTextWindow*			fTextWindow;
		BMenu*					fFindRecentStrings;
		BMenu*					fReplaceRecentStrings;
		BPopUpMenu*				fFileSetsPopup;
		BPopUpMenu*				fProjectPopup;
		BMenuField*				fFileSetsMenu;
		BMenuField*				fProjectMenu;
		MSaveFileSetWindow*		fFileSetWindow;
		MRemoveFileSetWindow*	fRemoveSetWindow;
		bool					fMultiVisible;
		bool					fMultiFinding;
		bool					fTextWindowHasSelection;
		bool					fHasSources;
		bool					fHasSystemHeaders;
		bool					fHasProjectHeaders;
		MSourceFileList			fSourcesList;
		MSourceFileList			fSysHeadersList;
		MSourceFileList			fProjectHeadersList;
		MSourceFileList			fOtherList;
		MSourceFileSet			fAllList;

		// item number in list matches item number in fProjectPopup
		MList<MProjectWindow*> 	fProjectList;
		int32					fSelectedProjectIndex;
		
		String					fFindString;
		String					fReplaceString;
		
		MFindThing				fFinder;
		FindData				fData;
		MFileSetKeeper			fSetKeeper;
		BLocker					fDataLock;
		bool					fWindowIsBuilt;
	
		static MFindWindow*		sFindWindow;

		struct FindDataT
		{
			// all bools, doesn't need to swap
			bool		sBatch;
			bool		sWrap;
			bool		sIgnoreCase;
			bool		sEntireWord;
			bool		sRegexp;
			// added after d2
			bool		sMultiVisible;
			bool		sStopAtEOF;
		};

		void					BuildWindow();
		void					GetPrefs();
		void					SetPrefs();

		void					CheckBoxChanged();
		void					UpdateButtons();
	
		void					SetUpFind();
		bool					CanFind();

		bool					FindNext(
									const char* inText, 
									int32 		inTextLen, 
									int32 		inOffset, 
									int32& 		outOffset, 
									int32& 		outLength,
									bool		inWrap);

		void					DoBatchFind();

		void					DoMultiFileFind();
		void					MultiFileFindDone(
									bool	inFound);
		void					CancelMultiFindThread();
		void					PseudoLock();
		void					PseudoUnlock();

		void					SetFindString(
									BMessage&	inMessage);
		void					SetFindString(
									const char*	inText,
									int32		inLength,
									bool		inForward);
		void					FindSelection(
									BMessage&	inMessage);
		void					SetMultiFileFind(
									bool	inMulti);
		void					ResetMultiFind();
		void					ToggleWindowSize();

		void					ModifyFileList(
									uint32	inKind);
		void					EmptyList(
									MSourceFileList&	inList);
		void					AddToAllList(
									MSourceFileList&	inList);
		void					RemoveFromAllList(
									MSourceFileList&	inList);
		void					DeleteSelectionInListBox();
		void					UpdateListBox();
		void					AdjustOtherCheckBox();
		void					UpdateRecentStrings(BMenu* inPopup);
		void					StringChanged(
									BMessage&	inMessage);
		bool					AddToOtherList(
									BMessage&	inMessage);
		bool					AddFolderToOtherList(
									entry_ref&		inRef);

		void					DoFileSet(
									BMessage& inMessage);
		void					SaveFileSet(
									bool			isProjectList,
									const char *	inName);
		void					SwitchFileSet(
									BMessage& inMessage);
		void					RemoveFileSet(
									BMessage& inMessage);
		void					UpdateProjectSets();
		void					CloseFileSetWindows();

		void					UpdateFileSetMenu();

		void					ProjectOpened(MProjectWindow* inWindow);
		void					ProjectClosed(MProjectWindow* inWindow);
		void					ProjectSelected(int32 index);
		void					HandleProjectSelected(BMessage& inMessage);
		
		bool					EqualText(
									const char* 	inString,
									const char* 	inText,
									bool 			inCaseInsensitive);

		bool					IsWhiteSpace(
									char 	inChar);
	
		int32					LineOf(
									const char* 	inText,
									int32			offset,
									int32			oldLineNumber,
									int32&			oldOffset);

		int32					LineEndOffset(
									const char*		inText,
									int32			inOffset,
									int32			inTextLen);

		void					BuildExpressionHelper(BMenu& inMenu, bool isFind);
		void					InsertRegularExpression(BMessage& inMessage);

		bool					LockData();
		void					UnlockData();

	static void 				HandleRegExpError(
									short inCode);
};


inline bool
MFindWindow::LockData()
{
	return fDataLock.Lock();
}
inline void
MFindWindow::UnlockData()
{
	fDataLock.Unlock();
}

#endif
