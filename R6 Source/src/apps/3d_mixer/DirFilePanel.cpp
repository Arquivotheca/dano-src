#include "DirFilePanel.h"
#include <NodeInfo.h>
#include <Path.h>
#include <Volume.h>
#include <Window.h>

extern bool CanHandleDir(const entry_ref *ref);


//--------------------------------------------------------

bool
GetFileType(char *filepath,char *filetype)
{
	BNode		node(filepath);
	BNodeInfo	node_info(&node);

	filetype[0] = 0;
	if (node_info.GetType(filetype) == B_NO_ERROR) 
		return true;

	update_mime_info(filepath, 0, 1, 0);
	if (node_info.GetType(filetype) == B_NO_ERROR) 
		return true;

	return false;
}

//--------------------------------------------------------

void
GetFileTypeDescription(char *longmimetype,char *shortmimetype)
{
	BMimeType	mtype(longmimetype);

	if (mtype.GetShortDescription(shortmimetype) != 0)
		shortmimetype[0] = 0;
}

//--------------------------------------------------------

static bool
CheckForSpecialDirs(char *path)
{
	bool	retval = false;
	char	*suffix;

	if (path == NULL)
		return false;
		
	//
	//	here's a bit of a hack
	//	hmmm, what other special places should be here
	//
	suffix = strrchr(path, '/');
	if (suffix == NULL)
		retval = false;
	else if (strcmp(suffix,"/") == 0)
		retval = false;
	else if (strcmp(suffix,"/dev") == 0)
		retval = false;
	else if (strcmp(suffix,"/pipe") == 0)
		retval = false;
	else if (strcmp(suffix,"/Trash") == 0)
		retval = false;
	else
		retval = true;
	//
	//	check and see if its a read only volume
	//
	if (retval) {
		struct stat st;
		
		stat(path,&st);
		
		BVolume v(st.st_dev);
		retval = (v.IsReadOnly() == false);
	}
	
	return retval;
}

//--------------------------------------------------------

void dirname(const char *path, char *newpath)
{
  	char *slash;
  	int length;

  	slash = strrchr (path, '/');
	if (slash == 0){
      	path = ".";
      	length = 1;
	} else {
      while (slash > path && *slash == '/')
        --slash;

      length = slash - path + 1;
    }
  	strncpy (newpath, path, length);
  	newpath[length] = 0;
}

//--------------------------------------------------------

void path_from_entry_ref(const entry_ref *ref,char *p)
{
	BPath  		path;
	BEntry 		entry(ref);
	//
	//	perform validity checks
	//	
	if (entry.InitCheck() == B_NO_ERROR) {
		if (entry.GetPath(&path) == B_NO_ERROR)
			strcpy(p,path.Path());
		else
			p[0] = 0;
	} else
		p[0] = 0;
}

//--------------------------------------------------------

bool TraverseSymLink(entry_ref *ref)
{
	BEntry e(ref,false);
	
	if (e.IsSymLink()) {
		BEntry f(ref,true);
		f.GetRef(ref);
		return true;
	} else {
		return false;
	}
}

//--------------------------------------------------------

bool
CanHandleDir(const entry_ref *ref)
{
	char path[B_PATH_NAME_LENGTH];
	
	entry_ref traversedRef(*ref);
	TraverseSymLink(&traversedRef);
	
	path_from_entry_ref(&traversedRef,path);

	BEntry entry(&traversedRef);
	if (entry.IsDirectory()) {
		if (CheckForSpecialDirs(path))
			return true;
	}
	
	return false;
}

//--------------------------------------------------------

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

//--------------------------------------------------------

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
			float charWidth,minx,miny,maxx,maxy;

			SetButtonLabel(B_DEFAULT_BUTTON,kDefaultBtnString);
			//
			//	Add the new 'Select Parent' button
			//
			charWidth = cancelBtn->StringWidth(kSelectDirBtnString);
			btnrect = cancelBtn->Frame();
			btnrect.right = btnrect.left - 15;
			btnrect.left = btnrect.right - charWidth - 40;
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
			w->GetSizeLimits(&minx,&maxx,&miny,&maxy);
			w->SetSizeLimits(w->Bounds().Width(),maxx,
							 w->Bounds().Height(),maxy);							
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
