#include "DirFilePanel.h"

#include <Window.h>

extern bool CanHandleDir(const entry_ref *ref);

bool
TDirFilter::Filter(const entry_ref* e, BNode* n, struct stat* s, const char* mimetype)
{
	if (strcmp("application/x-vnd.Be-directory",mimetype) == 0)
		return true;
	else if (strcmp("application/x-vnd.Be-volume",mimetype) == 0)
		return true;
	else if (strcmp("application/x-vnd.Be-symlink",mimetype) == 0)
		return CanHandleDir(e);
	else
		return false;
}

static void
SetTruncatedButtonName(BButton *button,const char *name,const char *defaultname)
{
	BRect rect = button->Bounds();
	char buffer[256];

	sprintf(buffer,"Select '%s'",name);
	//
	//	check against the button's width
	//
	if (button->StringWidth(buffer) <= rect.Width()) {
		// set if in the bounds of the btn
		button->SetLabel(buffer);
		return;
	}
	//
	//	else use the font method to truncate the string
	//
	BFont font;
	button->GetFont(&font);

	char truncBuffer[256];
	const char *srcStr[1];
	char *results[1];
	srcStr[0] = &buffer[0];
	results[0] = &truncBuffer[0];
    font.GetTruncatedStrings(srcStr, 1, B_TRUNCATE_END, rect.Width() - 10, results);
	button->SetLabel(truncBuffer);
}
			
//--------------------------------------------------------------------

TDirFilePanel::TDirFilePanel(BMessenger* target, entry_ref *start_directory,
	BMessage *openmsg, BRefFilter* filter)
	: BFilePanel(B_OPEN_PANEL, target, start_directory, B_DIRECTORY_NODE,
		false, openmsg, filter, true, true)
{
	BWindow	*w;
	//
	//	Get the window from the FilePanel
	//	so we can find the btns
	//
	w = Window();
	if (w->Lock()) {
		BRect btnrect;
		//
		//	find the default btn and other btns to modify them
		//	
		BView *v = w->ChildAt(0);
		BButton *defaultBtn = (BButton*)v->FindView("default button");
		BButton *cancelBtn = (BButton*)v->FindView("cancel button");
	
		ASSERT((cancelBtn==NULL) || (defaultBtn==NULL));
		if (cancelBtn && defaultBtn) {
			BView *parentview;
			float charWidth;

			SetButtonLabel(B_DEFAULT_BUTTON,kDefaultBtnString);
			//
			//	Add the new 'Select Parent' button
			//
			charWidth = cancelBtn->StringWidth(kSelectDirBtnString);
			btnrect = cancelBtn->Frame();
			btnrect.right = btnrect.left - 15;
			btnrect.left = btnrect.right - charWidth - 40; /* was 40 */
			fCurrentDirBtn = new BButton(btnrect, "current dir button",
				kSelectDirBtnString, new BMessage('slct'),
				B_FOLLOW_RIGHT + B_FOLLOW_BOTTOM);
			//
			//	Set its target and add it to the parent
			//	
			fCurrentDirBtn->SetTarget(*target);			
			parentview = defaultBtn->Parent();
			parentview->AddChild(fCurrentDirBtn);

			//
			//	Reset the size limits so that the new btn will not
			//	be obscured
			//		
			float minx,miny,maxx,maxy;
			w->GetSizeLimits(&minx,&maxx,&miny,&maxy);
			
			/* AJH -- this assumes that the ordering of buttons will not change */
			/* and that the BFilePanel will not obscure its own buttons vertically */
			float buttons_width  = defaultBtn->Frame().right - fCurrentDirBtn->Frame().left;

			if (buttons_width > minx)
				minx = buttons_width + 40;
		
			w->SetSizeLimits(minx,maxx,miny,maxy);							

			//
			//	if a null start directory was passed in
			//	retrieve the default directory (/boot/home)
			//
			BEntry	entry(start_directory);
			
			if (!entry.Exists()) {
				entry_ref ref;
				GetPanelDirectory(&ref);
				entry.SetTo(&ref);
			}
			
			ASSERT(entry.InitCheck() == B_NO_ERROR);
			if (entry.IsDirectory() && entry.InitCheck() == B_NO_ERROR) {
				char dirname[B_FILE_NAME_LENGTH];
				//
				//	Set the parent dir btn's name
				//
				entry_ref currRef;
				entry.GetName(dirname);
				entry.GetRef(&currRef);
				SetTruncatedButtonName(fCurrentDirBtn,dirname,kSelectDirBtnString);
				//
				//	set the message for the new btn
				//	this will get reset when the user selects a new item
				//
				BMessage *msg = new BMessage('slct');
				msg->AddRef("refs",&currRef);
				fCurrentDirBtn->SetMessage(msg);
			}
		}
		
		w->Unlock();
	
	}
}

TDirFilePanel::~TDirFilePanel()
{
}

//
//	Something changed in the filepanel
//	update the 'select parent btn' name
//	update the ref for the message with the new parent
//
void
TDirFilePanel::SelectionChanged()
{
	BWindow *wind;
	
	wind = Window();
	
	if (wind->Lock()) {
		char dirname[B_FILE_NAME_LENGTH];
		entry_ref currRef;

		//
		//	Get the current directory, set the select dir btn
		//
		GetPanelDirectory(&currRef);
		strcpy(dirname,currRef.name);
		//
		//	set the btns name to reflect a change
		//
		SetTruncatedButtonName(fCurrentDirBtn,dirname,kSelectDirBtnString);
		//
		//	modify the btn's msg
		//
		BMessage *msg = new BMessage('slct');
		msg->AddRef("refs",&currRef);
		fCurrentDirBtn->SetMessage(msg);
		
		wind->Unlock();
	}
}
