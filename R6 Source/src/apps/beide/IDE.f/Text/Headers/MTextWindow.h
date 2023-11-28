//========================================================================
//	MTextWindow.h
//	Copyright 1995 - 97 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	Source editting document for Be IDE

#ifndef _MTEXTWINDOW_H
#define _MTEXTWINDOW_H

#include "MPrefsStruct.h"
#include "MIDETextView.h"
#include "MMessageItem.h"
#include "MFormatUtils.h"
#include "MFileUtils.h"

class MProjectWindow;
class MTextInfoView;
class MGoToLineWindow;
class MPopupMenu;
class MTextAddOn;

typedef status_t (*perform_edit_func)(MTextAddOn *addon);

struct edit_add_on {
  image_id    			image;
  perform_edit_func		perform_edit;
  char        			name[B_FILE_NAME_LENGTH];
};

// This is the struct we use for the font prefs attribute
struct FontPrefsData
{
	void		SwapHostToBig();
	void		SwapBigToHost();

	BRect		pFrame;
	int32		pSelStart;
	int32		pSelEnd;
	int32		pTabSize;	
	bool		pAutoIndent;
	uchar		pFlags;
	uchar		punused2;
	uchar		punused3;
	int32		pFontSize;
	font_family	pFontFamily;
	font_style	pFontStyle;
};

const uchar kUsesSyntaxStyles = 0x01;

class MTextWindow : public BWindow
{
public:
		friend class MFindThing;			// needs access to textview

								MTextWindow(
									const char * title);
								MTextWindow(
									BEntry *		file,
									const char *	inEntry); // assumes ownership
								MTextWindow(
									const entry_ref& inRef);
								MTextWindow(
									const char * 	title,
									const char * 	inText,
									size_t			inTextLength);

								~MTextWindow();

virtual	void					DispatchMessage(
									BMessage *message, 
									BHandler *receiver);
virtual	bool					QuitRequested();
virtual	void					WindowActivated(
									bool inActive);
virtual	void					MenusBeginning();
 
virtual	void					MessageReceived(
									BMessage * message);
virtual void					Zoom(	
									BPoint 	rec_position,
									float 	rec_width,
									float 	rec_height);
virtual void					ScreenChanged(
									BRect 			screen_size, 
									color_space 	depth);

		void					ReadTextFile();
		bool					DoSave(
									bool	inSync = true);
		bool					DoSaveAs();
		void					GetDocTitle(
									char * outName);
		bool					IsMatch(
									const entry_ref& ref);
		void					GoToLine(
									long line);

		bool					HasSelection();
		void					PutGoBack();

		void					GetData(
									BMessage&	inOutMessage);
		void					SetData(
									BMessage&	inOutMessage,
									bool		inPermanent = true);

		void					BuildPopupMenu(
									MPopupMenu& 	inPopup);
	
		status_t				WriteToBlock(
									char*	inBlock,
									off_t&	inSize);

		bool					SavePanelIsRunning();
		void					CloseSavePanel();
		void					ShowSavePanel(
									const char *	inTitle,
									const char *	inSaveText);
		bool					SavePanelAddProjectChecked();
		void					AddEntryToProject(const BEntry& entry);
	
		void					ChangeWritableState();

		TextFormatType			GetEOLType()
								{
									return 	fNativeTextFormat;
								}				
		FileWriteAble			WritableState()
								{
									return fWriteableState;
								}
		const char *			Text()
								{
									return fTextView->Text();
								}
		bool					UsesSyntaxColoring()
								{
									return fTextView->UsesSyntaxColoring();
								}
		entry_ref				Ref();
		status_t				GetRef(
									entry_ref&	inoutRef);

static	void					RememberSelections(bool inRemember);
static	long					ScanForEditAddOns();
static	void					ShutDownEditAddOns();
static	void					RunStartupShutdownFunction(image_id id, const char* name);
static	void					SetSaveDirectory(
									BEntry&	inFile);

private:

		MIDETextView *			fTextView;
		BEntry *				fFile;
		BEntry *				fMonitorFile;

		BMenuBar *				fMenuBar;
		BMenu *					fWindowMenu;
		BMenuItem *				fUndoItem;
		BMenuItem *				fRedoItem;
		BMenu*					fSizeMenu;
		BMenu*					fFontMenu;
		BMenu*					fViewMenu;
	
		MGoToLineWindow*		fGoToLineWindow;
		MTextInfoView *			fInfoView;
		BFilePanel*				fSavePanel;
		MProjectWindow*			fProject;			// our project window
		int32					fSelStart;
		int32					fSelEnd;
		
		bool					fProjectIsOpen;		// a project window is open
		bool					fSavingACopyAs;
		bool					fChangeFontInfo;
		FontPrefs				fFontPrefs;
		FontPrefs				fTempFontPrefs;
		bool					fUsingPermanentFontPrefs;
		TextFormatType			fNativeTextFormat;
		BRect					fUserRect;
		bool					fUserState;
		bool					fSavePrefs;
		bool					fIsWritable;
		FileWriteAble			fWriteableState;		// Based on file permissions

static entry_ref				fLastDirectory;
static bool						fLastDirectoryGood;
static BRect					fSavePanelFrame;
static BLocker					fSavePanelLock;
static bool						fRememberSelections;
static MList<edit_add_on *> 	fEditAddOns;
static InfoStructList			fGoBackList;

		void					BuildWindow(
									SuffixType	inSuffix);
		void					FindMenuWillShow();
		void					WritableStateChanged();

		void					SaveRequested(
									entry_ref *directory, 
									const char *name);
		void					AttemptClose();
		bool					AskToClose();

		void					DoSaveACopyAs();
		void					SaveACopyAsRequested(
									entry_ref* inRef, 
									const char *name);
		status_t				WriteDataToFile(
									bool	inSync = true);

		void					DoRevert();

		bool					DoFindNext(
									bool	inForward);
		void					DoEnterFindString(
									bool	inForward = true);
		void					DoFindSelection(
									bool	inForward = true);
		void					DoReplace();
		void					DoReplaceAndFind(
									bool inForward);
		void					DoReplaceAll();

		void					AddSelectionToMessage(
									BMessage&	inMessage);

		void					OpenGoToLineWindow();
		void					DoGoToLine(
									BMessage& inMessage);

		void					InitMTextWindow(
									const char *	inTitle);
		void					SetTextProperties();
		void					DoCompile();
		void					DoSendToProject(
									uint32 inWhat);
		void					HandleProjectClosed(
									BMessage& inMessage);
		void					HandleProjectOpened(
									BMessage& inMessage);
		void					AddToProject();

		void					GetPrefs();
		void					SetPrefs();
		void					GetDefaultPrefs();
		bool					SelectionChanged();
static	bool					SelectionsAreRemembered();

		void					UpdateTextView(
									bool	inPermanent = true);

		void					UpdateViewMenu();
		
		void					UpdateFontData();
		void					ChangeFont(const font_family family, const font_style style);
		void					ChangeFontSize(float size);

		void					OpenSelection();
		void					DoAndyFeature();
		bool					FileExists();
		
		void					FindDefinition();
		void					FindDocumentation();

		void					GoBack();
		bool					CanGoBack();

		BMenu *					BuildEditAddOnMenu(
									const char *	title);
		status_t				PerformEditAddOn(
									BMessage *		message);

		void					ToggleSyntaxColoring();

		// Accessors for the find window
		MIDETextView*			GetTextView()
								{
									return fTextView;
								}

		void					StartMonitor();
};

#endif
