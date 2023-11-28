//--------------------------------------------------------------------
//	
//	Expander.cpp
//
//	Written by: Robert Chinn
//	
//	Copyright 1994-97 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include "Expander.h"

//====================================================================

int main(void)
{
  TExpanderApp	*app;

  app = new TExpanderApp();
  app->Run();
  
  delete app;
  return B_NO_ERROR;
}

//====================================================================

TExpanderApp::TExpanderApp(void)
		 :BApplication("application/x-vnd.Be-EXPD")
{
	InitApp();
	GetPrefs();
	fAutoExtractRun = fAutoExtract;

	/* load user-extended action table */
	BPath path;

	find_directory(B_USER_CONFIG_DIRECTORY, &path);
	path.Append("etc/expander.rules");
	load_action_table(path.Path());
}

void
TExpanderApp::InitApp()
{
	fAutoExtract = false;
	fQuitWhenDone = false;
	fDestSetting = kDestFromSrc;
	fDestRef = entry_ref();
	fOpenDest = false;
	fShowContents = false;

	fStatusWind = NULL;
	fStatusIsShowing = false;
	
	fGotRefs = false;
	
	fWindowLoc.x = 50; fWindowLoc.y = 50;
	fHOffset = 0;
}

//--------------------------------------------------------------------

TExpanderApp::~TExpanderApp(void)
{
	free_action_table();
}

//--------------------------------------------------------------------

void TExpanderApp::DoAbout(void)
{
	char msg[2048];
	
	sprintf(msg,"Expand-O-Matic\n\nExtract your tar, gzip and zip files.\n\n"
			"  Brought to you by:\n  Dominic, Hiroshi, Peter, Pavel and "
			"Robert.");
	
	(new BAlert("About Expand-O-Matic", msg, "OK"))->Go();
}

//--------------------------------------------------------------------

void TExpanderApp::ArgvReceived(int32 argc, char** argv)
{
	char		str[256];
  	int			i=1;
  	BEntry		entry;
  	entry_ref	ref;
  
  	for (i = 1; i < argc; i++) {
    	entry.SetTo(argv[i]);
    	if (entry.InitCheck() == B_NO_ERROR) {
	      	entry.GetRef(&ref);
			if (HandleTypeForRef(&ref)) {
				fGotRefs = true;
				TXpandoWindow *w=CheckForDupRef(&ref);
				
				if (w)
					w->Activate();
				else
					NewRef(&ref);

			}
    }
    else {
     	 sprintf(str, "Error: Could not open '%s'", argv[i]);
      	(new BAlert("", str, "Sorry"))->Go();
    }
	}
}

//--------------------------------------------------------------------

//
//	enter here when dropped on in Tracker
//
void TExpanderApp::RefsReceived(BMessage *msg)
{
	int32		 	count;
	int32		 	i=0;
	uint32	 	type;
	entry_ref		ref;
	
	if (msg->GetInfo("refs", &type, &count) == B_NO_ERROR) {
		for (i = 0; i < count; i++) {
			msg->FindRef("refs", i, &ref);
			if (HandleTypeForRef(&ref)) {
				fGotRefs = true;
				TXpandoWindow *w=CheckForDupRef(&ref);
				if (w) {
					PRINT(("activating existing window\n"));
					w->Activate();
				} else {
					PRINT(("new window\n"));
					NewRef(&ref);
				}

			}
		}
	}
	
}

//--------------------------------------------------------------------

//
//		if we received a d&d of a valid file and the auto flag is set
//		then the file will be expanded and the app will quit
//		else open an xpando window and let the user take over
//
void TExpanderApp::ReadyToRun()
{
	//
	//	nothing was dropped, no msgs received
	//	open an empty xpando window
	//
	if (!fGotRefs) {
		fAutoExtractRun = false;
		NewXpandoWindow(NULL);
	}
}

//--------------------------------------------------------------------

void TExpanderApp::MessageReceived(BMessage *msg)
{
	BApplication::MessageReceived(msg);
	
	switch(msg->what) {
		case B_ABOUT_REQUESTED:
			DoAbout();
			break;
			
		case B_SIMPLE_DATA:
	  	case B_REFS_RECEIVED:
			//
			//	will probably get this from an xpando window
			//	if multiple icons are dropped in a window
			//	open new xpando windows for each ref
			//
			RefsReceived(msg);
			break;
	
		case msg_open_new_ref:
			{
				uint32    type;
				int32     i, count;
				entry_ref ref;
				
				if (msg->GetInfo("refs", &type, &count) != B_NO_ERROR)
					break;

				for(i=0; i < count; i++) {
					msg->FindRef("refs", i, &ref);
					NewXpandoWindow(&ref);
				}
			}
			break;
	    	
		case msg_open_prefs:
			ShowPreferences();
			break;
		case msg_update_prefs:
			UpdatePrefs(msg);
			break;
			
		case msg_toggle_status:
			ToggleStatusWind();
			break;
			
		default:
			BApplication::MessageReceived(msg);
	}
}

//--------------------------------------------------------------------

bool TExpanderApp::QuitRequested(void)
{
	if (!inherited::QuitRequested())
		return false;
	
	if (CountWindows() > 1)
		return false;
	//
	//	update the preferences
	//
	SetPrefs();
	
	return true;
}

//--------------------------------------------------------------------

TXpandoWindow *TExpanderApp::CheckForDupRef(entry_ref *newref)
{
	int32 wCount = be_app->CountWindows();
	TXpandoWindow *t=NULL;
	
	if (wCount > 0) {
		for (int32 indx=0 ; indx<wCount ; indx++) {
			t = dynamic_cast<TXpandoWindow*>(be_app->WindowAt(indx));
			
			entry_ref ref;
			
			if (t && t->SourceRef(&ref)) {
				if (ref == *newref) {
					return t;
				}
			}
			
			if (t && t->DestRef(&ref)) {
				if (ref == *newref) {
					return t;
				}
			}
		}
		
	}
	return NULL;
}

TXpandoWindow* TExpanderApp::FindFrontWindow(void)
{
	int32 wCount = be_app->CountWindows();
	TXpandoWindow *t=NULL;
	
	if (wCount > 0) {
		for (int32 indx=0 ; indx<wCount ; indx++) {
			t = (TXpandoWindow*)be_app->WindowAt(indx);
			//
			//	see if the window thinks its the front window
			//
			if (t->IsFront())
				break;
		}
		
		return t;
	} else
		return NULL;
}

void TExpanderApp::NewRef(entry_ref *ref)
{
	int32 wCount = be_app->CountWindows();
	//
	//	if a window (xpander window) is showing update the
	//	front most with the new ref
	//	else show a new xpando window
	//
	if (wCount > 0) {
		//
		//	launched, but with an empty xpando window
		//	find it and fill it
		//
		if (fGotRefs == false) {
			fGotRefs = true;
			TXpandoWindow *w;			
			w = FindFrontWindow();
			if (w != NULL)
				w->UpdateRefs(ref);
			
		} else {
			//
			//	launched, but the windows are filled
			//	create a new xpando window and fill it
			//
			NewXpandoWindow(ref);
		}
	} else {
		//
		//	now windows yet, just launched
		//	create one with the ref
		//
		NewXpandoWindow(ref);
	}
}

//
//		auto expansion wrapper
//		invoked from a d&d when the auto flag is set
//
void TExpanderApp::ProcessRef(entry_ref *ref)
{
	BEntry entry = BEntry(ref);
	struct stat st;
	status_t result = 0;
	char msg[256];
	
	entry.GetStat(&st);	
	
	if (S_ISDIR(st.st_mode) == false) {
		char 			filepath[B_PATH_NAME_LENGTH],filedir[B_PATH_NAME_LENGTH];
		entry_ref	 	destref;
		
		path_from_entry_ref(ref,filepath);
		dirname(filepath,filedir);
		get_ref_for_path(filedir,&destref);

		ShowStatus("Expanding file...");
		
		if (HandleTypeForPath(filepath)) {
			//!!!!!result = ProcessFile(filepath,filetype,filedir,fExtractTar);
			
			if (result != B_NO_ERROR) {
				sprintf(msg,"Unable to process file (%li)",result);
				ShowStatus(msg);
			} else if (result == B_NO_ERROR)
				ShowStatus("Done");
		} else {
			sprintf(msg,"File type not handled for file %s",filepath);
			ShowStatus(msg);
		}
	}
}

//--------------------------------------------------------------------

const BRect kWindowFrame(0, 0, kConfigMinWindWidth, kConfigMinWindHt);
const float kDefaultWindXLoc = 50.0;
const float kDefaultWindYLoc = 50.0;

BPoint
TExpanderApp::NextWindowLoc()
{
	BScreen screen(B_MAIN_SCREEN_ID);

	BRect frame = kWindowFrame;
	frame.OffsetTo(fWindowLoc.x,fWindowLoc.y);

	if (!(screen.Frame().Contains(frame))) {
		// wont fit on screen start over from top left
		fHOffset += 10;
		frame = kWindowFrame;
		frame.OffsetTo(kDefaultWindXLoc,kDefaultWindYLoc);
		frame.OffsetBy(fHOffset,0);
	}
		
	fWindowLoc = frame.LeftTop() + BPoint(10,10);

	return frame.LeftTop();
}

//
//		create a new xpando window
//
void TExpanderApp::NewXpandoWindow(entry_ref *ref)
{
	new TXpandoWindow(ref, NextWindowLoc());
}

//--------------------------------------------------------------------

//
//		drop zone window wrappers
//
void TExpanderApp::InitStatus(void)
{
	fStatusWind = new TExpanderStatusWindow();
}

bool TExpanderApp::StatusIsShowing(void)
{
	return fStatusIsShowing;
}

void TExpanderApp::ShowStatus(char *statusMsg)
{
	if (fStatusIsShowing == false)
		InitStatus();
		
	fStatusWind->Show();
	fStatusIsShowing = true;
	fStatusWind->SetMsg(statusMsg);
}

void TExpanderApp::HideStatus(void)
{
	fStatusWind->Hide();
	fStatusIsShowing = false;
}

void TExpanderApp::ToggleStatusWind(void)
{
	if (StatusIsShowing()) {
		HideStatus();
	} else {
		ShowStatus("");
	}
}

//--------------------------------------------------------------------

const char *EXPANDER_SETTINGS = "Expander_Settings";
const char kVersion = 1;

void TExpanderApp::GetPrefs(void)
{
	BPath path;
	fDestSetting = kDestFromSrc;
	fOpenDest = true;
	fShowContents = false;
	fAutoExtract = false;

	if (find_directory (B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		int	ref;

		path.Append(EXPANDER_SETTINGS);
		ref = open(path.Path(), O_RDONLY);
		if (ref >= 0) {
			char version=0;
			read(ref,&version,sizeof(char));
			if (version == kVersion) {
				read(ref, (char *)&(fAutoExtract), sizeof(bool));
				read(ref, (char *)&(fQuitWhenDone), sizeof(bool));
				read(ref, (char *)&(fDestSetting), sizeof(short));
				
				int32 len=0;
				char name[B_FILE_NAME_LENGTH];
				ino_t directory;
				dev_t device;
				read(ref,&device,sizeof(dev_t));
				read(ref,&directory,sizeof(ino_t));
				read(ref,&len,sizeof(int32));
				if (len > 0) {
					read(ref,name,len);
					name[len] = 0;
					fDestRef = entry_ref(device,directory,name);
				}
				
				read(ref, (char *)&(fOpenDest), sizeof(bool));
				read(ref, (char *)&(fShowContents), sizeof(bool));
				
				read(ref,&fWindowLoc,sizeof(BPoint));
			}
			close(ref);
		}
	}
}

void TExpanderApp::SetPrefs(void)
{
	BPath		path;

	if (find_directory (B_USER_SETTINGS_DIRECTORY, &path, true) == B_OK) {
		int			ref;
	
		path.Append(EXPANDER_SETTINGS);
		ref = creat(path.Path(), 0777);
		if (ref >= 0) {
			char version=kVersion;
			write(ref,&version,sizeof(char));
			write(ref, (char *)&(fAutoExtract), sizeof(bool));
			write(ref, (char *)&(fQuitWhenDone), sizeof(bool));
			write(ref, (char *)&(fDestSetting), sizeof(short));
			
			write(ref,&(fDestRef.device),sizeof(dev_t));
			write(ref,&(fDestRef.directory),sizeof(ino_t));
			int32 len=0;
			if (fDestRef.name) {
				len=strlen(fDestRef.name);
				write(ref,&len,sizeof(int32));
				write(ref,fDestRef.name,len);
			} else
				write(ref,&len,sizeof(int32));
			
			write(ref, (char *)&(fOpenDest), sizeof(bool));
			write(ref, (char *)&(fShowContents), sizeof(bool));
			write(ref,&fWindowLoc,sizeof(BPoint));

			close(ref);
		}
	}
}

void TExpanderApp::UpdatePrefs(BMessage *msg)
{
	msg->FindBool("autoExtract",&fAutoExtract);
	msg->FindBool("quitWhenDone",&fQuitWhenDone);
	msg->FindInt16("dest",&fDestSetting);

	if (fDestSetting == kCustomDest) {
		char *name;
		dev_t device;
		ino_t directory;
		msg->FindString("name", (const char**)&name);
		msg->FindInt32("device",&device);
		msg->FindInt64("directory", (int64*) &directory);

		fDestRef = entry_ref(device,directory,name);
	}
	msg->FindBool("open",&fOpenDest);
	msg->FindBool("show",&fShowContents);
}

void TExpanderApp::ShowPreferences(void)
{
	TPrefsWindow *prefs = new TPrefsWindow(fAutoExtract, fQuitWhenDone, fDestSetting, &fDestRef,
		fOpenDest,fShowContents);
	prefs->Show();
}

bool 
TExpanderApp::AutoExtractRun() const
{
	return fAutoExtractRun && fGotRefs;
}

bool
TExpanderApp::QuitWhenDone() const
{
	return fQuitWhenDone;
}

short
TExpanderApp::DestSetting() const
{
	return fDestSetting;
}

void
TExpanderApp::DestRef(entry_ref *ref)
{
	*ref = fDestRef;
}

bool
TExpanderApp::OpenDest() const
{
	return fOpenDest;
}

bool
TExpanderApp::ShowContents() const
{
	return fShowContents;
}

void
TExpanderApp::SetWindowLoc(BPoint p)
{
	fWindowLoc = p;
}
