#include <signal.h>
#include <stdio.h>
#include <Path.h>
#include <fs_info.h>

#include "utils.h"

#include "Expander.h"
#include "XpandoWindow.h"

bool
TFileFilter::Filter(const entry_ref* e, BNode* n, struct stat* s, const char* mimetype)
{
	if (strcmp("application/x-vnd.Be-directory",mimetype) == 0)
		return true;
	else if (strcmp("application/x-vnd.Be-volume",mimetype) == 0)
		return true;
	else if (strcmp("application/x-vnd.Be-symlink",mimetype) == 0)
		//
		//	resolve the symlink and see if it points
		//	to anything we care about
		//
		return CanHandleFile(e);
	else
		//
		//	check all other files to see if we handle them
		//
		return CanHandleFile(e);
}

//--------------------------------------------------------------------

TTextControl::TTextControl(BRect frame,
							const char *name,
							const char *label, 
							const char *initial_text, 
							BMessage *message,
							uint32 rmask,
							uint32 flags)
	: BTextControl(frame,name,label,initial_text,message,rmask,flags)
{
	fWidth = Bounds().Width();
}

TTextControl::~TTextControl()
{
}
			
void TTextControl::FrameResized(float new_width, float new_height)
{
	BTextControl::FrameResized(new_width,new_height);
	
	if (fWidth == new_width)
		return;
		
	Invalidate();
	Window()->UpdateIfNeeded();
	
	fWidth = new_width;
}
				
const char* const kResumeStr = "Resume";
const char* const kPauseStr = "Pause";

//--------------------------------------------------------------------

TXpandoWindow::TXpandoWindow(entry_ref *ref,BPoint loc)
  : BWindow(	BRect(0,0,1,1), "Expand-O-Matic", B_TITLED_WINDOW,
		B_FRAME_EVENTS | B_WILL_DRAW | B_NOT_ZOOMABLE,
		B_CURRENT_WORKSPACE)
{
	SetWindowLocAndSize(loc);
	InitWindow(ref);
	Show();
}

void
TXpandoWindow::SetWindowLocAndSize(BPoint loc)
{
	MoveTo(loc);
	ResizeTo(kConfigMinWindWidth,kConfigWindHeight);
}

void
TXpandoWindow::InitWindow(entry_ref *ref)
{
	fXpandoMatic = new TXpandoMatic();
	
#if _BUILD31_ ==  0
	fIsBusy = false;
	fWhichAction = true;
#endif	

	fSourceFilePanel = NULL;
	fDestFilePanel = NULL;

	fSrcSet = false;
	fDestSet = false;
	fSrcRef = entry_ref();	
	fDestRef = entry_ref();

	AddMenuBar();
	AddParts();
	
	fListShowing = Prefs_ShowContents();
		
	if (ref != NULL) {
		//
		//	we received a ref, update the window, set the controls, etc.
		//
		UpdateRefs(ref);
	} else {
		//
		//	no ref, but the user might want the list showing anyhow
		//
		if (fListShowing && fSrcSet)
			ShowList();
		else 
			HideList();
	
		EnableDisableButtons();
	}
}

TXpandoWindow::~TXpandoWindow(void)
{
	if (fSourceFilePanel != NULL) {
		delete fSourceFilePanel;
		fSourceFilePanel = NULL;
	}
		
	if (fDestFilePanel != NULL) {
		delete fDestFilePanel;
		fDestFilePanel = NULL;
	}

	delete fXpandoMatic;

	if (be_app->CountWindows() <= 1)
		be_app->PostMessage(B_QUIT_REQUESTED);
}

void TXpandoWindow::FrameResized(float w,float h)
{
	BRect rect = fContentsFld->Bounds();
	rect.OffsetTo(0,0);
	rect.InsetBy(2,2);
	
	fContentsFld->SetTextRect(rect);
}


void TXpandoWindow::MessageReceived(BMessage *msg)
{
	entry_ref ref;

	BWindow::MessageReceived(msg);
			
	switch (msg->what) {
		case B_CANCEL:         /* have to delete the file panel objects */
			int32 old_what;

			if (msg->FindInt32("old_what", &old_what) == B_NO_ERROR) {
				if (old_what == msg_src_chosen && fSourceFilePanel) {
					delete fSourceFilePanel;
					fSourceFilePanel = NULL;
				} else if (old_what == msg_dest_chosen && fDestFilePanel) {
					delete fDestFilePanel;
					fDestFilePanel = NULL;
				}
			}
			break;
			
		case B_SIMPLE_DATA:
			HandleRefDrop(msg);
			break;
		case msg_choose_src:			// invoke file panel
			ChooseSource();
			break;
		case msg_src_chosen:			// file picked from file panel
			msg->FindRef("refs",&ref);
			HandleSourceUpdate(&ref);
			break;
		case msg_src_update:			// path modified in fld
			SourcePathUpdate(&ref);
			HandleSourceUpdate(&ref);
			break;
			
		case msg_choose_dest:			// invoke file panel
			ChooseDestination();
			break;
		case 'slct':					// select parent btn
			fDestFilePanel->Hide();		// hide filepanel, then pass it on
		case msg_dest_chosen:			// folder picked in file panel
			msg->FindRef("refs",&ref);
			HandleDestUpdate(&ref);
			break;
		case msg_dest_update:			// path is edited in fld
			DestPathUpdate(&ref);
			HandleDestUpdate(&ref);
			break;
			
		case msg_expand:
			ExpandFile();
			break;
		case msg_expand_complete:
			ExpandComplete(msg);
			break;
		case msg_list_complete:
			ListComplete(B_OK);
			break;
		case msg_toggle_list:
			ToggleListing();
			break;
			
#if _BUILD31_
		case msg_pause_resume:
			PauseResumeProcess();
			break;
			
		case msg_kill:
			StopProcessing();
			break;
#endif

		case msg_update_ref:
			break;

	  default:
			be_app->MessageReceived(msg);
  }
}

bool TXpandoWindow::QuitRequested()
{	
#if _BUILD31_
	if (!StopProcessing())		// just in case something is still trying to run
		return false;
#else
	if (IsBusy()) {
		What("Sorry, can't close or quit while expanding or listing.");
		return false; 
	}
#endif
	
	UpdateWindowLoc();
		
	return true;
}

static BMenu *BuildFileMenu(void)
{
	BMenu *m;
	BMenuItem *mitem;
	
	m = new BMenu("File");
	
	mitem = new BMenuItem("About Expander",new BMessage(B_ABOUT_REQUESTED));
	m->AddItem(mitem);
	
	m->AddSeparatorItem();
	
	mitem = new BMenuItem("Set Source...",new BMessage(msg_choose_src),'O');
	m->AddItem(mitem);
	
	mitem = new BMenuItem("Set Destination...",new BMessage(msg_choose_dest),'D');
	m->AddItem(mitem);
	
	m->AddSeparatorItem();
	
	mitem = new BMenuItem("Expand",new BMessage(msg_expand),'E');
	m->AddItem(mitem);
	
	mitem = new BMenuItem("Show Contents",new BMessage(msg_toggle_list),'L');
	m->AddItem(mitem);
	
	m->AddSeparatorItem();
	
#if _BUILD31_
#if 0
	mitem = new BMenuItem(kPauseStr, new BMessage(msg_pause_resume));
	m->AddItem(mitem);
#endif
	mitem = new BMenuItem("Stop",new BMessage(msg_kill),'K');
	m->AddItem(mitem);
	m->AddSeparatorItem();
#endif

	mitem = new BMenuItem("Close", new BMessage(B_CLOSE_REQUESTED),'W');
	m->AddItem(mitem);
	
	return m;
}

static BMenu *BuildEditMenu(void)
{
  	BMenu *m;
  	BMenuItem *mitem;
	
  	m = new BMenu("Edit");
	
#if 0
	mitem = new BMenuItem("Undo",new BMessage(B_UNDO), 'Z');
	mitem->SetEnabled(false);
	m->AddItem(mitem);
	
	m->AddSeparatorItem();	
	
	mitem = new BMenuItem("Cut",new BMessage(B_CUT), 'X');
	mitem->SetEnabled(false);
	m->AddItem(mitem);
	mitem = new BMenuItem("Copy",new BMessage(B_COPY), 'C');
	mitem->SetEnabled(false);
	m->AddItem(mitem);
	mitem = new BMenuItem("Paste",new BMessage(B_PASTE), 'V');
	mitem->SetEnabled(false);
	m->AddItem(mitem);
	
	m->AddSeparatorItem();	
#endif
	
	mitem = new BMenuItem("Preferences...",new BMessage(msg_open_prefs),'P');
  	m->AddItem(mitem);
	
  	return m;
}

void TXpandoWindow::AddMenuBar(void)
{
  	BMenu		*mitem;
  
  	fMenuBar = new BMenuBar(BRect(0,0,Bounds().right,0),"",
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
		B_ITEMS_IN_ROW, true);
  
	mitem = BuildFileMenu();
	fMenuBar->AddItem(mitem);
	
	mitem = BuildEditMenu();
	fMenuBar->AddItem(mitem);
	
//	AddShortcut( 'P', B_SHIFT_KEY, new BMessage(msg_pause_resume));
	
	AddChild(fMenuBar);
}

void TXpandoWindow::AddParts(void)
{
	BRect visibleRect,r;
	float w = Bounds().Width();
	
	visibleRect.Set(0, fMenuBar->Bounds().Height()+1, Bounds().Width()+1, Bounds().Height());
	fBackdrop = new BBox(visibleRect, "bg", B_FOLLOW_ALL, B_WILL_DRAW, B_PLAIN_BORDER);
	AddChild(fBackdrop);
	
//	r.Set(10,5,75,23);
	r.Set(10,10,75,28);
	fSrcBtn = new BButton(r,"Source btn","Source",new BMessage(msg_choose_src));
	fBackdrop->AddChild(fSrcBtn);
	
	r.Set(80,r.top+2,w-10,r.bottom);
	fSrcFld = new TTextControl(	r,"source fld","","", new BMessage(msg_src_update),
		B_FOLLOW_LEFT_RIGHT);
	fSrcFld->SetDivider(0.0);
	fBackdrop->AddChild(fSrcFld);

//	r.Set(10,33,75,51);
	r.Set(10,38,75,56);
	fDestBtn = new BButton(r,"Dest btn","Destination",new BMessage(msg_choose_dest));
	fBackdrop->AddChild(fDestBtn);
	
	r.Set(80,r.top+2,w-10,r.bottom);
	fDestFld = new TTextControl(r, "dest fld", "", "", new BMessage(msg_dest_update),
		B_FOLLOW_LEFT_RIGHT);
	fDestFld->SetDivider(0.0);
	fBackdrop->AddChild(fDestFld);

//	r.Set(10,61,75,79);
	r.Set(10,66,75,84);
	fExpandBtn = new BButton(r,"expand btn","Expand",new BMessage(msg_expand));
	fBackdrop->AddChild(fExpandBtn);
	
	r.Set(84,61+6,w-110,81);
	fStatusFld = new BStringView(r,"","",B_FOLLOW_LEFT_RIGHT,B_WILL_DRAW);
//	fStatusFld->SetViewColor(203,203,203);
	fBackdrop->AddChild(fStatusFld);	
	
	r.Set(w-105,65,w-10,79);
	fShowListBtn = new BCheckBox(	r,"list btn","Show Contents",
		new BMessage(msg_toggle_list), B_FOLLOW_RIGHT);
	fBackdrop->AddChild(fShowListBtn);	
	
//	r.Set(10,94,w-10-B_V_SCROLL_BAR_WIDTH,300);
	r.Set(10,99,w-10-B_V_SCROLL_BAR_WIDTH,305);
	fContentsFld = new BTextView(	r,"", BRect(0,0,r.Width()-B_V_SCROLL_BAR_WIDTH,r.Height()),
		be_fixed_font, NULL, B_FOLLOW_ALL,B_WILL_DRAW);
	fContentsFld->MakeEditable(false);
	fContentsScroller = new BScrollView("",fContentsFld,B_FOLLOW_ALL,0,false,true);
	fBackdrop->AddChild(fContentsScroller);
}

//--------------------------------------------------------------------

static bool EntryExists(entry_ref *ref)
{
	BEntry e(ref);
	
	if (e.InitCheck() != B_NO_ERROR) {
		return false;
	} else {
		return e.Exists();
	}
}

static bool PathExists(const char *path)
{
	entry_ref ref;
	
	if (get_ref_for_path(path,&ref) == B_NO_ERROR)
		return (EntryExists(&ref));
	else
		return false;
}

//--------------------------------------------------------------------

void
TXpandoWindow::AutoExtractDest()
{
	//
	// 	extract the path to the source file and
	//	use it for the destination
	//
	char destpath[B_PATH_NAME_LENGTH];
	entry_ref destRef;
	
	if (fSrcFld->Text() && strlen(fSrcFld->Text()) > 0) {
		dirname(fSrcFld->Text(), destpath);

		if (get_ref_for_path(destpath,&destRef) == B_NO_ERROR)
			SetDest(&destRef);
	}
}

void
TXpandoWindow::DestFromPrefs()
{
	entry_ref ref;
	
	Prefs_DestRef(&ref);
	if (ref.name)
		SetDest(&ref);
}

void TXpandoWindow::SetSource(entry_ref *ref)
{
	if (Lock()) {
		fSrcSet = true;		

		BPath  	path;
		BEntry 	entry(ref);	
		char 	p[B_PATH_NAME_LENGTH];
		
		entry.GetPath(&path);
		strcpy(p,path.Path());
		fSrcFld->SetText(p);
		
		//	if we are auto extracting
		//	grab the path based on the pref setting
		if (Prefs_AutoExtract() && Prefs_DestSetting() == kNoDest) {
			// 	if the pref setting is no path
			//	default to /boot/home
			entry_ref destRef;
			if (get_ref_for_path("/boot/home",&destRef) == B_NO_ERROR)
				SetDest(&destRef);
		} else if (/*Prefs_AutoExtract() ||*/ Prefs_DestSetting() == kDestFromSrc)
			AutoExtractDest();
		else if (Prefs_DestSetting() == kCustomDest) {
			//	if there is a path it should be set
			DestFromPrefs();
			if (Prefs_AutoExtract() && !CheckDest()) {
				// 	if there is not a path and this is an auto extract
				//	set the path to /boot/home (default)
				entry_ref destRef;
				if (get_ref_for_path("/boot/home",&destRef) == B_NO_ERROR)
					SetDest(&destRef);
			}
		}
			
		Unlock();
	}
}

bool TXpandoWindow::SourceRef(entry_ref *ref) 
{ 
	if (fSrcSet && CheckSource()) {
		if (get_ref_for_path(fSrcFld->Text(),ref) == B_NO_ERROR)
			return true;
		else
			return false;
	} else {
		ref = NULL;
		return false;
	}
}

bool TXpandoWindow::SourcePath(char *path)
{
	if (fSrcSet && CheckSource()) {
		strcpy(path,fSrcFld->Text());
		return true;
	} else {
		path[0] = 0;
		return false;
	}
}

//
//	new path was entered, need a corresponding ref
//
bool TXpandoWindow::SourcePathUpdate(entry_ref *ref)
{
	if (CheckSource()) {
		get_ref_for_path(fSrcFld->Text(),ref);
		return true;
	} else {
		ref = NULL;
		return false;
	}
}

bool TXpandoWindow::CheckSource(void)
{
	return PathExists(fSrcFld->Text());
}

//--------------------------------------------------------------------

void TXpandoWindow::SetDest(entry_ref *ref)
{
	if (Lock()) {
		fDestSet = true;

		BPath  		path;
		BEntry 		entry(ref);
	
		entry.GetPath(&path);
		fDestFld->SetText(path.Path());

		Unlock();
	}
}

bool TXpandoWindow::DestRef(entry_ref *ref) 
{ 
	if (fDestSet && CheckDest()) {
		if (get_ref_for_path(fDestFld->Text(),ref) == B_NO_ERROR)
			return true;
		else
			return false;
	} else {
		ref = NULL;
		return false;
	}
}

bool TXpandoWindow::DestPath(char *path)
{
	if (fDestSet && CheckDest()) {		
		strcpy(path,fDestFld->Text());
		return true;
	} else {
		path[0] = 0;
		return false;
	}
}

//
//	new path was entered, need a corresponding ref
//
bool TXpandoWindow::DestPathUpdate(entry_ref *ref)
{
	if (CheckDest()) {
		get_ref_for_path(fDestFld->Text(),ref);
		return true;
	} else {
		ref = NULL;
		return false;
	}
}

bool TXpandoWindow::CheckDest()
{
	bool retval = PathExists(fDestFld->Text());
	
	return retval;
}

//--------------------------------------------------------------------


void TXpandoWindow::MenusBeginning()
{
	BMenuItem *m;
	
#if _BUILD31_

	m = fMenuBar->FindItem("Set Source...");
	if (m)
		m->SetEnabled(!fXpandoMatic->Processing());
	
	m = fMenuBar->FindItem("Set Destination...");
	if (m)
		m->SetEnabled(!fXpandoMatic->Processing());

	m = fMenuBar->FindItem("Show Contents");
	if (!m) 
		m = fMenuBar->FindItem("Hide Contents");
	if (m)
		m->SetEnabled(!(fXpandoMatic->ProcessAction() == kExpandingAction &&
			fXpandoMatic->Processing()) && fSrcSet);
	
	m = fMenuBar->FindItem("Expand");
	if (m)
		m->SetEnabled(fSrcSet && fDestSet && !fXpandoMatic->Processing());
		
	bool f = fXpandoMatic->Processing();
	m = fMenuBar->FindItem("Pause");
	if (m)
		m->SetEnabled(f);

	m = fMenuBar->FindItem("Stop");
	if (m)
		m->SetEnabled(fXpandoMatic->Processing());
	
	// Edit menu		
	m = fMenuBar->FindItem("Preferences...");
	if (m)
		m->SetEnabled(!fXpandoMatic->Processing());
	
#else
	m = fMenuBar->FindItem("Show Contents");
	if (m)
		m->SetEnabled(fSrcSet && fDestSet && !IsBusy());
		
	m = fMenuBar->FindItem("Expand");
	if (m)
		m->SetEnabled(fSrcSet && fDestSet && !IsBusy());
		
#endif
}

void TXpandoWindow::EnableDisableButtons(void)
{
#if _BUILD31_
	fSrcBtn->SetEnabled(!fXpandoMatic->Processing());
	fDestBtn->SetEnabled(!fXpandoMatic->Processing());
	
	if (fXpandoMatic->Processing()) {
		 
		if (fXpandoMatic->ProcessAction() == kExpandingAction){
			fExpandBtn->SetLabel("Stop");
			fExpandBtn->SetMessage(new BMessage(msg_kill));
			fShowListBtn->SetEnabled(false);
		} else {
//			fShowListBtn->SetMessage(new BMessage(msg_kill));
			fExpandBtn->SetEnabled(false);
		}
		
	} else {	// no action happening, src, dest may be set - default func
		fShowListBtn->SetEnabled(fSrcSet);
		fShowListBtn->SetMessage(new BMessage(msg_toggle_list));

		fExpandBtn->SetEnabled(fSrcSet && fDestSet);
		fExpandBtn->SetLabel("Expand");
		fExpandBtn->SetMessage(new BMessage(msg_expand));

	}
#else
	fSrcBtn->SetEnabled(!IsBusy());
	fDestBtn->SetEnabled(!IsBusy());

	fExpandBtn->SetEnabled(fSrcSet && fDestSet && !IsBusy());
	fMenuBar->SetEnabled(!IsBusy());

	fShowListBtn->SetEnabled(!IsBusy());
#endif
}

#if _BUILD31_
void
TXpandoWindow::PauseResumeProcess()
{
	BMenuItem* mitem;
	
	if (fXpandoMatic->ProcessingPaused()) {
		mitem = fMenuBar->FindItem(kResumeStr);
		if (mitem)
			mitem->SetLabel(kPauseStr);
		fXpandoMatic->ResumeProcessing();
	} else if (fXpandoMatic->Processing()) {
		mitem = fMenuBar->FindItem(kPauseStr);
		mitem->SetLabel(kResumeStr);
		fXpandoMatic->PauseProcessing();
	}
}

const char* const kStopExpandMsg = "Are you sure you want to stop expanding this archive? The expanded items may not be complete.";
bool
TXpandoWindow::StopProcessing()
{
	// no pausing, just yet
/*	if (fXpandoMatic->ProcessingPaused()) {
		BMenuItem* mitem = fMenuBar->FindItem(kResumeStr);
		if (mitem)
			mitem->SetLabel(kPauseStr);
		fXpandoMatic->ResumeProcessing();
	}*/
	
	if (fXpandoMatic->Processing()) {
		
		if (fXpandoMatic->ProcessAction() == kExpandingAction) {

			fXpandoMatic->PauseProcessing();
			
			BAlert* alert = new BAlert("", kStopExpandMsg, "Stop", "Continue", NULL,
				B_WIDTH_FROM_WIDEST, B_WARNING_ALERT);
			int32 result = alert->Go();
			
			// the threads need to be started up again,
			// even if they are to be killed
			fXpandoMatic->ResumeProcessing();
			if (result == 1) {
				return false;
			}
		}
	
		fXpandoMatic->StopProcessing();
#if _BUILD31_
		if (fXpandoMatic->ProcessAction() == kListingAction) {
#else
		if (fWhichAction) {
#endif
			ListComplete(1);
		} else {
			HandleExpansionCompletion(1);
		}
	}
	
	return true;
}
#endif

void
TXpandoWindow::ToggleListing()
{
			if (fListShowing) {
#if _BUILD31_
				StopProcessing();
#endif
				HideList();
			} else {
				ShowList();
				if (fRebuildList)
					BuildList();
			}
}

void TXpandoWindow::HideList(void)
{
//	if (Lock()) {
		BMenuItem *m = fMenuBar->FindItem("Hide Contents");
		if (m)
			m->SetLabel("Show Contents");
	
		float h = kConfigMinWindHt + fMenuBar->Bounds().Height();
		SetSizeLimits(kConfigMinWindWidth,kConfigMaxWindWidth,h,h);
		ResizeTo(Bounds().Width(),h);
		fListShowing = false;
		fShowListBtn->SetValue(false);
		SetStatus("");
//		Unlock();
//	}
}

void TXpandoWindow::ShowList(void)
{
	BMenuItem *m = fMenuBar->FindItem("Show Contents");
	if (m)
		m->SetLabel("Hide Contents");
	
	fContentsFld->MakeSelectable(true);

	SetSizeLimits(kConfigMinWindWidth,9999,kConfigWindHeight,9999);
	ResizeTo(Bounds().Width(),kConfigWindHeight);
	fShowListBtn->SetValue(true);
	fListShowing = true;
}

void TXpandoWindow::BuildList(void)
{
	if (Lock()) {
		if (fSrcSet == false) {
			SetStatus("");
		} else {
			char 	filetype[255];
			char	srcpath[B_PATH_NAME_LENGTH];
			char	destpath[B_PATH_NAME_LENGTH];
			char	msg[1024];
	
			if (SourcePath(srcpath)) {
				char *filename;							
				
				filename = strrchr(srcpath,'/');		
				if (filename == NULL)					
					filename = srcpath;					
				else if (filename[0] == '/')			
					filename++;							
					
#if _BUILD31_ == 0
				fIsBusy = true;
				fWhichAction = true;
#endif

				DestPath(destpath);
				
				sprintf(msg,"Creating listing for %s",filename);
				SetStatus(msg);
				
				SetList("");
		
				GetFileType(srcpath,filetype);
		
				fXpandoMatic->SetSrc(srcpath);
				fXpandoMatic->SetFileType(filetype);
				fXpandoMatic->SetDest(destpath);
				fXpandoMatic->SetParent(this);
				
				fContentsFld->MakeSelectable(false);
				
				fXpandoMatic->List();
				EnableDisableButtons();
			} else {
				SetList("");	// clear the list
				SetStatus("");	// clear the status fld
				ShowFileError(srcpath);
			}
		}		
		Unlock();
	}
}

void TXpandoWindow::ListComplete(int32 result)
{
#if _BUILD31_ == 0
	fIsBusy = false;
#endif
	fContentsFld->MakeSelectable(true);
	
	EnableDisableButtons();
	
	if (result != B_OK){
		fRebuildList = true;
		SetStatus("The listing was stopped and may not be complete.");
	} else {
		//	if the listing completed normally,
		//	cache it so we don't have to do it again
		fRebuildList = false;
		SetStatus("");
	}
}

void TXpandoWindow::AddToList(char *line)
{
	if (Lock()) {
		fContentsFld->GoToLine(fContentsFld->CountLines() + 1);
		fContentsFld->Insert(line);
		fContentsFld->ScrollToSelection();
		Unlock();
	}
}

void TXpandoWindow::SetList(char *text)
{
	if (Lock()) {
		fContentsFld->SetText(text);
		Unlock();
	}
}

//--------------------------------------------------------------------

void TXpandoWindow::SetStatus(char *status)
{
	if (Lock()) {
		fStatusFld->SetText(status);
		Unlock();
	}
}

//--------------------------------------------------------------------

void TXpandoWindow::ChooseSource(void)
{
	if (!fSourceFilePanel) {
		BMessage	*msg;

		msg = new BMessage(msg_src_chosen);
		fSourceFilePanel = new BFilePanel(B_OPEN_PANEL,new BMessenger(NULL,this),
			&fSrcRef,0,false,msg,&fFileFilter,true,true);

		float offset = (fDestFilePanel!=NULL) ? 20 : 10;
	  	(fSourceFilePanel->Window())->MoveTo(Frame().left+offset,Frame().top+ offset);
	}

	fSourceFilePanel->Show();
}

void TXpandoWindow::ChooseDestination(void)
{
	if (!fDestFilePanel) {
		BMessage	*msg;
		
		msg = new BMessage(msg_dest_chosen);	
		fDestFilePanel = new TDirFilePanel(new BMessenger(NULL,this),&fDestRef,msg,
			&fDirFilter);
			
		float offset = (fSourceFilePanel!=NULL) ? 20 : 10;
	  	(fDestFilePanel->Window())->MoveTo(Frame().left+offset,Frame().top+ offset);
	}

	fDestFilePanel->Show();
}

//--------------------------------------------------------------------

#define DO_MEM_CHECK 1

static bool
SpaceAvailable(const char* src_archive, const char* dest_path)
{
//
//	need to change from showing the default error dialogs
//	and instead show more contextual error dialogs
//
	bool retval = true;
	
#if DO_MEM_CHECK	
	off_t 		s;
	entry_ref	ref;
	BEntry 		e(src_archive);
	
	e.GetSize(&s);
	
	BVolume v(dev_for_path(dest_path));
	
	if (v.FreeBytes() < s) {
		char vname[1024],msg[1024];
		v.GetName(vname);
		
		sprintf(msg,"The volume '%s' has %Li bytes free, %Li bytes (or more) necessary for expansion.",
			vname,v.FreeBytes(),s);
		What(msg);
		retval = false;
	}
#endif

	return retval;
}

void TXpandoWindow::ExpandFile(void)
{
	if (fSrcSet == false) {
		SetStatus(""); 
	} else if (fDestSet == false)
		SetStatus("");
	else {
		char	srcpath[B_PATH_NAME_LENGTH], destpath[B_PATH_NAME_LENGTH];
		char 	filetype[256];
		char 	msg[B_PATH_NAME_LENGTH];
			
		if (SourcePath(srcpath) == false) {
			ShowFileError(srcpath);
			return;
		}
		
		if (DestPath(destpath) == false) {
			ShowDirError(destpath);
			return;
		}
		
		entry_ref destref;
		get_ref_for_path(destpath,&destref);
		if (!HandleTypeForRef(&destref)) {
			ShowDirError(destpath);
			return;
		}
		
		if (SpaceAvailable( srcpath, destpath)) {
			
#if _BUILD31_ == 0
			fIsBusy = true;
			fWhichAction = false;
#endif
			
			GetFileType(srcpath,filetype);
			
			char *filename;							
			
			filename = strrchr(srcpath,'/');		
			if (filename == NULL)					
				filename = srcpath;					
			else if (filename[0] == '/')
				filename++;							
	
			sprintf(msg,"Expanding file %s",filename);								
			SetStatus(msg);
	
			fXpandoMatic->SetSrc(srcpath);
			fXpandoMatic->SetFileType(filetype);
			fXpandoMatic->SetDest(destpath);
			fXpandoMatic->SetParent(this);
		
			fXpandoMatic->Expand();
		
			EnableDisableButtons();			
		}
	}
}

void
TXpandoWindow::HandleExpansionCompletion(int32 result)
{
#if _BUILD31_ == 0
	fIsBusy = false;
#endif

	EnableDisableButtons();
	
	if (result == B_NO_ERROR) {
		SetStatus("File expanded");			
				
		if (Prefs_OpenDest()) {
			BMessenger tracker("application/x-vnd.Be-TRAK");
			BMessage openmsg(B_REFS_RECEIVED);
			entry_ref entry;
			
			DestRef(&entry);
			openmsg.AddRef("refs", &entry);
			
			tracker.SendMessage(&openmsg);
		}
		// should we quit automatically
		if (Prefs_AutoExtract() || Prefs_QuitWhenDone())
			PostMessage(B_QUIT_REQUESTED);
#if _BUILD31_
	} else if (result == 1) {
		SetStatus("");
//		(new BAlert("", "The extraction of the archive was stopped.\nThe archive contents may not be intact.", "Ok"))->Go();
#endif
	} else {
		SetStatus("");
		char m[1024];
		sprintf(m,"A problem occurred while expanding file (%s)."
			"The file may be bad or the expansion rule may be incorrect.",
			strerror(result));
		(new BAlert("", m, "Ok"))->Go();
	}
}

void TXpandoWindow::ExpandComplete(BMessage *msg)
{
	long result = B_NO_ERROR;
	
	msg->FindInt32("result",&result);
	HandleExpansionCompletion(result);
}

//--------------------------------------------------------------------

void TXpandoWindow::UpdateRefs(entry_ref *ref)
{
	if (Lock()) {
		BEntry entry(ref,true);
		
		if (HandleTypeForRef(ref)) {
			TXpandoWindow *w=((TExpanderApp*)be_app)->CheckForDupRef(ref);
			if (w)
				w->Activate();
			else {
				if (entry.IsDirectory()) {
					SetStatus(""); 							
					SetDest(ref);
				} else {
						SetStatus(""); 					
						SetList("");
						SetSource(ref);
						fRebuildList = true;
						
						if (Prefs_AutoExtract()) {
							// 	call to setsource will grab the destination
							//	if fAutoExpand is set true
							HideList();
							ExpandFile();
						} else if (Prefs_ShowContents() || fListShowing) {
							ShowList();
							if (fRebuildList)
								BuildList();
						} else
							HideList();
				}
				EnableDisableButtons();
				Activate();
			}
		} else {
			char path[B_PATH_NAME_LENGTH];
		
			path_from_entry_ref(ref,path);
			if (entry.IsDirectory())
				ShowDirError(path);
			else
				ShowFileError(path);
		}

		Unlock();
	}
}

//--------------------------------------------------------------------

void TXpandoWindow::HandleRefDrop(BMessage *msg)
{
	int32	count;
	uint32	type;

	if (msg->GetInfo("refs", &type, &count) == B_NO_ERROR) {
#if _BUILD31_
		if (fXpandoMatic->Processing()) {
#else
		if (IsBusy()) {
#endif
			//
			// expanding or listing, pass the new ref to the app
			//
			entry_ref ref;
			BMessage newmsg = *msg;
			
			newmsg.what = msg_open_new_ref;
			be_app->PostMessage(&newmsg);
		} else if (count == 1) {
			//
			// 	change the current ref to this one
			//
			entry_ref ref;
			
			msg->FindRef("refs", 0, &ref);
			UpdateRefs(&ref);
		} else if (count > 1) {		
			//
			//	more than one ref, pass it to the app
			//
			be_app->PostMessage(msg);
		}
	}
}

//--------------------------------------------------------------------

void
TXpandoWindow::HandleDestUpdate(entry_ref *ref)
{
	if (CanHandleDir(ref)) {
		UpdateRefs(ref);
		fDestRef = *ref;
	} else {
		char path[B_PATH_NAME_LENGTH];
	
		path_from_entry_ref(ref,path);
		ShowDirError(path);
	}
}

void
TXpandoWindow::HandleSourceUpdate(entry_ref *ref)
{
	if (CanHandleFile(ref)) {
		UpdateRefs(ref);
		//
		//	get the parent of the file and set the
		//	ref so that the next time through we will
		//	open the file panel in that directory
		//
		entry_ref parent_ref;
		BEntry e(ref),parent_entry;
		
		e.GetParent(&parent_entry);	
		if (parent_entry.GetRef(&parent_ref) == B_NO_ERROR)
			fSrcRef = parent_ref;
		else
			fSrcRef = entry_ref();
	} else {
		char path[B_PATH_NAME_LENGTH];
	
		path_from_entry_ref(ref,path);
		ShowFileError(path);
	}
}

//--------------------------------------------------------------------

#if _BUILD31_ == 0
bool
TXpandoWindow::IsBusy()
{
	return fIsBusy;
}
#endif

//--------------------------------------------------------------------

bool
TXpandoWindow::Prefs_AutoExtract()
{
	TExpanderApp *app = dynamic_cast<TExpanderApp *>(be_app);
	ASSERT(app);
	
	return app->AutoExtractRun();
}

bool
TXpandoWindow::Prefs_QuitWhenDone()
{
	TExpanderApp *app = dynamic_cast<TExpanderApp *>(be_app);
	ASSERT(app);
	
	return app->QuitWhenDone();
}

short
TXpandoWindow::Prefs_DestSetting()
{
	TExpanderApp *app = dynamic_cast<TExpanderApp *>(be_app);
	ASSERT(app);
	
	return app->DestSetting();
}

void
TXpandoWindow::Prefs_DestRef(entry_ref *ref)
{
	TExpanderApp *app = dynamic_cast<TExpanderApp *>(be_app);
	ASSERT(app);
	
	app->DestRef(ref);
}

bool
TXpandoWindow::Prefs_OpenDest()
{
	TExpanderApp *app = dynamic_cast<TExpanderApp *>(be_app);
	ASSERT(app);
	
	return app->OpenDest();
}

bool
TXpandoWindow::Prefs_ShowContents()
{
	TExpanderApp *app = dynamic_cast<TExpanderApp *>(be_app);
	ASSERT(app);
	
	return app->ShowContents();
}

void
TXpandoWindow::UpdateWindowLoc()
{
	TExpanderApp *app = dynamic_cast<TExpanderApp *>(be_app);
	ASSERT(app);
	
	app->SetWindowLoc(Frame().LeftTop());
}
