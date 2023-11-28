 #ifndef CONFIG_WINDOW_H
#define CONFIG_WINDOW_H


#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <FilePanel.h>
#include <Font.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <ScrollView.h>
#include <StringView.h>
#include <TextControl.h>
#include <Window.h>
#include <View.h>

#include "DirFilePanel.h"

#include "build_prefs.h"

class TXpandoMatic;
class TDirFilePanel;
class TDirFilter;

enum {
  M_File = 'mbr1',
  M_Edit = 'mbr2'
};

enum {
	msg_choose_src = 128,
	msg_src_chosen,
	msg_choose_dest,
	msg_dest_chosen,
	msg_expand,
	msg_expand_complete,
	msg_list_complete,
	msg_src_update,
	msg_dest_update,
	msg_toggle_list,
	msg_close_status,
	msg_update_ref,
	msg_open_new_ref,
	msg_wind_loc,
	msg_kill,
	msg_pause_resume
};

#define kConfigWindWidth 		640.0
#define kConfigMinWindWidth 	450.0
#define kConfigMaxWindWidth		1024.0
#define kConfigMinWindHt		101.0 	//91.0
#define kConfigWindHeight 		335.0	//325.0
#define kConfigWindXLoc 		50.0
#define kConfigWindYLoc 		50.0

class TFileFilter : public BRefFilter {
public:
	bool Filter(const entry_ref* e, BNode* n, struct stat* s, const char* mimetype);
};

class TTextControl : public BTextControl {
public:
			TTextControl(BRect frame,
						const char *name,
						const char *label, 
						const char *initial_text, 
						BMessage *message,
						uint32 rmask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
						uint32 flags = B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS);
			~TTextControl();
			
			void 	FrameResized(float new_width, float new_height);
private:
			float	fWidth;
};

class TXpandoWindow : public BWindow {
public:
			TXpandoWindow(entry_ref *ref, BPoint loc);			
	  		~TXpandoWindow(void);
  
			void 	FrameResized(float w,float h);
			void	MessageReceived(BMessage *msg);
			bool	QuitRequested();
			
	virtual void 	MenusBeginning();
			void 	AddMenuBar(void);
			void 	AddParts(void);
			
			void 	InitWindow(entry_ref *ref);
			void 	SetWindowLocAndSize(BPoint loc);
			
			void	AutoExtractDest();
			void	DestFromPrefs();
			
			void	SetSource(entry_ref *ref);
			bool	SourceRef(entry_ref *ref);
			bool	SourcePath(char *path);
			bool	SourcePathUpdate(entry_ref *ref);
			bool	CheckSource(void);
			
			void	SetDest(entry_ref *ref);
			bool	DestRef(entry_ref *ref);
			bool	DestPath(char *path);
			bool	DestPathUpdate(entry_ref *ref);
			bool	CheckDest(void);

			void 	DisableControls(void);
			void 	EnableControls(void);
			
#if _BUILD31_
			void	PauseResumeProcess();
			bool	StopProcessing();
#endif
			
			void	SyncListControls(void);
			void	DisableListControls(void);
			void	EnableListControls(void);
			void	ToggleListing();
			void	HideList(void);
			void	ShowList(void);
			void	BuildList(void);
			void	ListComplete(int32);
			void	SetList(char *text);
			void 	AddToList(char *line);
			bool	IsListShowing(void) { return fListShowing; }
			
			void	SetStatus(char *status);
			
			void 	ChooseSource(void);
			void 	ChooseDestination(void);
			
			void 	EnableDisableButtons(void);
			void 	ExpandFile(void);
			void 	HandleExpansionCompletion(int32 result);
			void	ExpandComplete(BMessage *msg);
			
			void	UpdateRefs(entry_ref *ref);
			
			void	HandleRefDrop(BMessage *msg);
			
			void	HandleSourceUpdate(entry_ref *ref);
			void	HandleDestUpdate(entry_ref *ref);
			
#if _BUILD31_ ==  0
			bool	IsBusy();
#endif
			
			bool 	Prefs_AutoExtract();
			bool 	Prefs_QuitWhenDone();
			short 	Prefs_DestSetting();
			void 	Prefs_DestRef(entry_ref *ref);
			bool 	Prefs_OpenDest();
			bool 	Prefs_ShowContents();

			void 	UpdateWindowLoc();
private:
#if _BUILD31_ ==  0
	bool			fIsBusy;
	bool			fWhichAction;
#endif
	TFileFilter 	fFileFilter;
	BFilePanel      *fSourceFilePanel;
	entry_ref		fSrcRef;
	
	TDirFilter		fDirFilter;
	TDirFilePanel  	*fDestFilePanel;
	entry_ref		fDestRef;

 	BMenuBar 		*fMenuBar;
 	BBox			*fBackdrop;

	BButton			*fSrcBtn;
	TTextControl	*fSrcFld;
	bool			fSrcSet;
	
	BButton			*fDestBtn;
	TTextControl	*fDestFld;
	bool			fDestSet;
	
	BButton			*fExpandBtn;
	BStringView		*fStatusFld;
	BCheckBox		*fShowListBtn;
	bool			fListShowing;
	bool			fRebuildList;
	
	BScrollView		*fContentsScroller;
	BTextView		*fContentsFld;
	
	TXpandoMatic 	*fXpandoMatic;	
};

#endif
