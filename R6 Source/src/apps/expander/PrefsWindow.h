#include <stdio.h>

#include <Application.h>
#include <CheckBox.h>
#include <Box.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <RadioButton.h>
#include <ScrollView.h>
#include <StringView.h>
#include <TextControl.h>
#include <Window.h>
#include <View.h>

#include "Expander.h"
#include "DirFilePanel.h"

#ifndef PREFS_WINDOW_H
#define PREFS_WINDOW_H

enum {
	msg_update_grab_dest = 999,
	msg_update_open_dest,
	msg_update_show_contents,
	msg_update_auto_extract,
	msg_update_quit_when_done,
	msg_no_dest,
	msg_extract_dest,
	msg_custom_dest,
	msg_select_dest,
	msg_open_prefs,
	msg_update_prefs,
	msg_okay_prefs,
	msg_cancel_prefs
};

class TPrefsWindow : public BWindow {
public:

		TPrefsWindow(bool autoExtract, bool quitWhenDone,
			short destsetting, entry_ref *ref, bool opendest, bool showcontents);
			
 		~TPrefsWindow(void);
  
		void	MessageReceived(BMessage *msg);
		void 	FrameResized(float w,float h);

		bool	QuitRequested();
		
		void 	AddParts(bool autoExtract, bool quitWhenDone,
					short destsetting, bool opendest, bool showcontents);
			
		void 	ChooseDestination(void);
private:
	bool			fSaveData;
	
	BBox			*fBackdrop;

	BStringView		*fTitleFld;
	//
	//	Extraction
	//
	BStringView		*fExtractionFld;
	BCheckBox		*fAutoExtractBtn;
	BCheckBox		*fQuitAfterExpandBtn;
	//
	//	Destination
	//
	BStringView		*fDestinationFld;
	BRadioButton	*fNoDestBtn;
	BRadioButton	*fDestFromSrcBtn;
	BRadioButton	*fUseCustomBtn;
	BTextControl	*fCustomDestFld;
	BButton			*fSelectDestBtn;
	//
	//	Other
	//
	BStringView 	*fOtherFld;
	BCheckBox		*fOpenDestBtn;
	BCheckBox		*fShowContentsBtn;
	
	BButton			*fCancelBtn;
	BButton			*fOkayBtn;

	TDirFilter		fDirFilter;
	TDirFilePanel 	*fDestFilePanel;
	entry_ref		fDestRef;
};

#endif
