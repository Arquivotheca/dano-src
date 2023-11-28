#include "RList.h"
// fix lameness in BFilePanel

#ifndef _MFILEPANEL_H
#define _MFILEPANEL_H

class MFilePanel : public BFilePanel
{
public:		
	MFilePanel(file_panel_mode = B_OPEN_PANEL,
				BMessenger* target = NULL,
				entry_ref *start_directory = NULL,
				bool directory_selection = false,
				bool allow_multiple_selection = true,
				uint32 message = 0,
				BRefFilter* = NULL,
				const char *openLabel = NULL,
				bool autoClose = true);

virtual			~MFilePanel();

virtual void	SelectionChanged();	
virtual void	WasHidden();	// closed
virtual void	SendMessage(const BMessenger*, BMessage *); // open/save
private:
	bool							fAutoClose;
	BMessenger						*fTarget;
	
	static BLocker					dataLock;
	static entry_ref				openLoc;
	static entry_ref				saveLoc;
	static BRect					openRect;
	static BRect					saveRect;
	
//	static	RList<BFilePanel *>		panelList;
	
	void					SetWindowRect(BRect);
	
	char					*fOpenLabel;
	static const char		*kOpenLabel;
	bool					fUpdtButton;
};
#endif
