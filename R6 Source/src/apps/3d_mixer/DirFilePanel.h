#ifndef DIR_FILE_PANEL_H
#define DIR_FILE_PANEL_H

#include <string.h>
#include <stdlib.h>
#include <Debug.h>
#include <Button.h>
#include <FilePanel.h>

#define kDefaultBtnString "Select"
#define kSelectDirBtnString "Select Current Directory"

class TDirFilter : public BRefFilter {
public:
	bool Filter(const entry_ref* e, BNode* n, struct stat* s, const char* mimetype);
};

class TDirFilePanel : public BFilePanel {
public:
			TDirFilePanel(BMessenger* target = NULL,
						  entry_ref *start_directory = NULL,
						  BMessage *openmsg=NULL,
						  BRefFilter* filter=NULL);
			~TDirFilePanel();
			
			void SelectionChanged();
private:
			BButton		*fCurrentDirBtn;			
};

#endif
