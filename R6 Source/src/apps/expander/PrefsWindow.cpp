#include "PrefsWindow.h"

#define kPrefsWindWidth			325
#define kPrefsWindHeight		305
#define kPrefsWindXLoc 			50.0
#define kPrefsWindYLoc 			50.0

#define BTN_HEIGHT 20
#define BTN_WIDTH 75

TPrefsWindow::TPrefsWindow(bool autoExtract, bool quitWhenDone,
	short destsetting, entry_ref *ref, bool opendest, bool showcontents)
  : BWindow(	BRect(0,0,kPrefsWindWidth,kPrefsWindHeight), "Expander Preferences",
		B_MODAL_WINDOW ,
		B_FRAME_EVENTS | B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_WILL_DRAW,
		B_CURRENT_WORKSPACE)
{
	fDestFilePanel = NULL;
	fSaveData=false;
	fDestRef = entry_ref(ref->device,ref->directory,ref->name);
	
	BScreen screen(B_MAIN_SCREEN_ID);
	float x = screen.Frame().Width()/2 - kPrefsWindWidth/2;
	float y = screen.Frame().Height()/2 - kPrefsWindHeight/2;
	
	MoveTo(x,y);

	AddParts(autoExtract, quitWhenDone, destsetting, opendest, showcontents);	
}

TPrefsWindow::~TPrefsWindow(void)
{
}

void TPrefsWindow::MessageReceived(BMessage *msg)
{
	inherited::MessageReceived(msg);
	switch(msg->what) {
		case msg_no_dest:
		case msg_extract_dest:
			fSelectDestBtn->SetEnabled(false);
			fCustomDestFld->SetEnabled(false);
			break;
		case msg_custom_dest:
			fSelectDestBtn->SetEnabled(true);
			fCustomDestFld->SetEnabled(true);
			(fCustomDestFld->TextView())->MakeEditable(false);
			break;
			
		case msg_select_dest:
			ChooseDestination();
			break;
		case 'slct':						// select parent btn
			fDestFilePanel->Hide();			// hide filepanel, then pass it on
		case msg_dest_chosen:				// folder picked in file panel
			{
				msg->FindRef("refs",&fDestRef);
				BPath p;
				BEntry e(&fDestRef);
				
				e.GetPath(&p);
				fCustomDestFld->SetText(p.Path());
				
				if (fDestFilePanel) {
					delete fDestFilePanel;
					fDestFilePanel = NULL;
				}
			}
			break;
			
		case msg_okay_prefs:
			fSaveData = true;
			PostMessage(B_QUIT_REQUESTED);
			break;
		case msg_cancel_prefs:
			fSaveData = false;
			PostMessage(B_QUIT_REQUESTED);
			break;
			
		default:
			be_app->PostMessage(msg);
	}
}
		
void
TPrefsWindow::FrameResized(float w,float h)
{
	//printf("%f, %f\n",w,h);
	BWindow::FrameResized( w, h);
}

bool TPrefsWindow::QuitRequested()
{
	if (fSaveData) {
		BMessage msg(msg_update_prefs);
		
		msg.AddBool("autoExtract",fAutoExtractBtn->Value());
		msg.AddBool("quitWhenDone",fQuitAfterExpandBtn->Value());
		short val = 0;
		if (fNoDestBtn->Value())
			val = kNoDest;
		else if (fDestFromSrcBtn->Value())
			val = kDestFromSrc;
		else if (fUseCustomBtn->Value()) {
			val = kCustomDest;
			if (fDestRef.name) {
				msg.AddInt32("device",fDestRef.device);
				msg.AddInt64("directory",fDestRef.directory);
				msg.AddString("name",fDestRef.name);
			}
		}
		msg.AddInt16("dest",val);
		msg.AddBool("open",fOpenDestBtn->Value());
		msg.AddBool("show",fShowContentsBtn->Value());
		
		be_app->PostMessage(&msg);
	}	
	return true;
}

void
TPrefsWindow::AddParts(bool autoExtract, bool quitWhenDone,
	short destsetting, bool opendest, bool showcontents)
{
	BRect r(Bounds());
	r.InsetBy(-1, -1);
	fBackdrop = new BBox(r,"", B_FOLLOW_ALL);
	fBackdrop->SetFont(be_plain_font);
	AddChild(fBackdrop);

	BBox *box = new BBox(BRect(12, 10, kPrefsWindWidth-10, kPrefsWindHeight-BTN_HEIGHT-34),
		"box", B_FOLLOW_ALL, B_WILL_DRAW);
	box->SetLabel("Expander Preferences");
	box->SetFont(be_bold_font);
	box->SetFontSize(12);
	fBackdrop->AddChild(box);
	
	float w = box->Bounds().Width();
	//
	//	Extraction
	//
	float btnWidth = fBackdrop->StringWidth("Close window when done expanding") + 20;

	r.Set(15,25,w-10,37);
	fExtractionFld = new BStringView(r,"e fld","Expansion:", B_FOLLOW_NONE, B_WILL_DRAW);
	box->AddChild(fExtractionFld);

	r.left = r.left + 10; r.right = r.left + btnWidth;
	r.top = r.bottom + 5;
	r.bottom = r.top + 15;	
	fAutoExtractBtn = new BCheckBox( r,"autoExpand","Automatically expand files",
		new BMessage(msg_update_auto_extract), B_FOLLOW_NONE);
	fAutoExtractBtn->SetValue(autoExtract);
	box->AddChild(fAutoExtractBtn);

	r.right = r.left + btnWidth;
	r.top = r.bottom + 2;
	r.bottom = r.top + 15;	
	fQuitAfterExpandBtn = new BCheckBox( r,"autoExpand","Close window when done expanding",
		new BMessage(msg_update_quit_when_done), B_FOLLOW_NONE);
	fQuitAfterExpandBtn->SetValue(quitWhenDone);
	box->AddChild(fQuitAfterExpandBtn);
	//
	//	Destination
	//	
	r.left = r.left - 10;
	r.top = r.bottom + 15;
	r.bottom = r.top + 15;	
	fDestinationFld = new BStringView(r,"e fld","Destination Folder:", B_FOLLOW_NONE,
		B_WILL_DRAW);
	box->AddChild(fDestinationFld);

	btnWidth = fBackdrop->StringWidth("Leave destination folder path empty") + 20;
	r.left = r.left + 10;  r.right = r.left + btnWidth;
	r.top = r.bottom + 5;  r.bottom = r.top + 15;
	fNoDestBtn = new BRadioButton( r, "no dest", "Leave destination folder path empty",
		new BMessage(msg_no_dest));
	fNoDestBtn->SetValue(destsetting==kNoDest);
	box->AddChild(fNoDestBtn);
	
	btnWidth = fBackdrop->StringWidth("Same directory as source (archive) file") + 20;
	r.right = r.left + btnWidth;
	r.top = r.bottom + 2;  r.bottom = r.top + 15;
	fDestFromSrcBtn = new BRadioButton( r, "dest from src",
		"Same directory as source (archive) file", new BMessage(msg_extract_dest));
	fDestFromSrcBtn->SetValue(destsetting==kDestFromSrc);
	box->AddChild(fDestFromSrcBtn);
	
	btnWidth = fBackdrop->StringWidth("Use:") + 20;
	r.right = r.left + btnWidth;
	r.top = r.bottom + 2;  r.bottom = r.top + 15;
	fUseCustomBtn = new BRadioButton( r, "custom dest", "Use:",
		new BMessage(msg_custom_dest));
	fUseCustomBtn->SetValue(destsetting==kCustomDest);
	box->AddChild(fUseCustomBtn);

	btnWidth = fBackdrop->StringWidth("Select") + 20;
	r.top -= 4; r.bottom -= 4;
	r.right = w - 15;
	r.left = r.right - btnWidth;
	fSelectDestBtn = new BButton( r, "sel dest", "Select",
		new BMessage(msg_select_dest));
	fSelectDestBtn->SetEnabled(destsetting==kCustomDest);
	box->AddChild(fSelectDestBtn);
	
	// !!!!!  need to truncate path for display
	r.top += 4; r.bottom += 4;
	r.right = r.left - 10;
	r.left = fUseCustomBtn->Frame().right + 5;
	BPath p;
	BEntry e(&fDestRef);
	e.GetPath(&p);	
	fCustomDestFld = new BTextControl(r, "custom", "", p.Path(), NULL,
		B_FOLLOW_NONE,B_WILL_DRAW);
	fCustomDestFld->SetEnabled(destsetting==kCustomDest);
	fCustomDestFld->SetDivider(0.0);
	box->AddChild(fCustomDestFld);
	(fCustomDestFld->TextView())->MakeEditable(false);
	//
	//	Other
	//
	btnWidth = fBackdrop->StringWidth("Open destination folder after extraction") + 30;
	r.left = 15; r.right = r.left + btnWidth;
	r.top = r.bottom + 15;
	r.bottom = r.top + 15;	
	fOtherFld = new BStringView(r,"o fld","Other:", B_FOLLOW_NONE, B_WILL_DRAW);
	box->AddChild(fOtherFld);

	r.left = r.left + 10;
	r.top = r.bottom + 5;
	r.bottom = r.top + 15;
	fOpenDestBtn = new BCheckBox(	r,"openDestination","Open destination folder after extraction",
									new BMessage(msg_update_open_dest),
									B_FOLLOW_NONE);
	fOpenDestBtn->SetValue(opendest);
	box->AddChild(fOpenDestBtn);

	r.top = r.bottom + 2;
	r.bottom = r.top + 15;
	fShowContentsBtn = new BCheckBox(	r,"list btn","Automatically show contents listing",
									new BMessage(msg_update_show_contents),
									B_FOLLOW_NONE);
	fShowContentsBtn->SetValue(showcontents);
	box->AddChild(fShowContentsBtn);
	
	//
	//	Buttons
	//
	r.Set(	Bounds().Width()-BTN_WIDTH-13,	Bounds().Height()-BTN_HEIGHT-19,
		  	Bounds().Width()-13,			Bounds().Height()-19);
	fOkayBtn = new BButton(r,"","OK",new BMessage(msg_okay_prefs),B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);

	r.right = r.left - 14;
	r.left = r.right - BTN_WIDTH;
	fCancelBtn = new BButton(r,"","Cancel",new BMessage(msg_cancel_prefs),B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	fBackdrop->AddChild(fCancelBtn);
	
	fBackdrop->AddChild(fOkayBtn);
	SetDefaultButton(fOkayBtn);
}

void
TPrefsWindow::ChooseDestination(void)
{
	if (fDestFilePanel)
		delete fDestFilePanel;
		
		BMessage	*msg;
		
		msg = new BMessage(msg_dest_chosen);	
		fDestFilePanel = new TDirFilePanel(new BMessenger(NULL,this),&fDestRef,msg,
			&fDirFilter);
			
		float offset = 10;
	  	(fDestFilePanel->Window())->MoveTo(Frame().left+offset,Frame().top+ offset);

		fDestFilePanel->Show();	
}

