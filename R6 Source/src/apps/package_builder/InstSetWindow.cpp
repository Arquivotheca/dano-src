#include <Be.h>
// InstSetWindow.cpp
#include "InstSetWindow.h"
#include "PackMessages.h"
#include "PackListView.h"
#include "STextField.h"
#include "TCheckBox.h"
#include "MFilePanel.h"

#include <Path.h>

#include "Util.h"
#include "PackWindow.h"

#include "FArchiveItem.h"

#include "MyDebug.h"


InstSetWindow::InstSetWindow(const char *title, PackWindow *parW)
	: 	ChildWindow(BRect(0,0,360,370),title,B_TITLED_WINDOW,
			B_NOT_RESIZABLE | B_NOT_ZOOMABLE,parW)
{
	Lock();
	pw = (PackWindow *)parW;

	AddChild(new InstSetView(Bounds(),"setview",pw));
	parentList = (PackList *)(pw->FindView("listing"));	
		
	Show();
	Unlock();
}

InstSetWindow::~InstSetWindow()
{
}


bool InstSetWindow::QuitRequested()
{
	if (Dirty()) {
		SetDescription();
		SetInstallFolder();
		SetHelp();
	}
	return ChildWindow::QuitRequested();
}

void InstSetWindow::WindowActivated(bool state)
{
	PRINT(("Window activated %s\n",state ? "true" : "false"));
	// fix this bullshit!!!!!
	if (!state && Dirty()) {
		// the window is deactivating and it is dirty so do a save
		SetDescription();
		SetInstallFolder();
		SetHelp();
	}
	ChildWindow::WindowActivated(state);
}

void InstSetWindow::SetDescription()
{
	STextField *gv = (STextField *)FindView("description");
	free( pw->attrib.descriptionText );
	char *dText = (char *)malloc((gv->TextLength()+1)*sizeof(char));
	
	strcpy(dText,gv->Text());
	
	pw->attrib.descriptionText = dText;
	pw->attribDirty = TRUE;
	
	SetDirty(FALSE);
}


void InstSetWindow::SetInstallFolder()
{
	// Deadlock here!!
	// so we cache this value
	ArchiveFolderItem *top = parentList->toplevel;
	const char *t = ((BTextControl *)FindView("foldertext"))->Text();
	
	free(top->name);
	top->name = strdup(t);
	pw->attribDirty = TRUE;

	PRINT(("foldername set\n"));
	SetDirty(FALSE);
}

void InstSetWindow::SetHelp()
{
	STextField	*pHelp = (STextField *)FindView("pkghelp");
	free( pw->attrib.packageHelpText );

	char *dText = (char *)malloc((pHelp->TextLength()+1)*sizeof(char));
	strcpy(dText,pHelp->Text());
	
	pw->attrib.packageHelpText = dText;
	pw->attribDirty = TRUE;
	
	SetDirty(FALSE);
}

void InstSetWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case M_INFO_MODIFIED: {
			SetDirty(TRUE);			// a text view was modified
			PRINT(("window is DIRTY\n"));
			break;
		}
		case M_DO_LICENSE: {
			// pw->Lock();
			bool state;			// respond to checkbox click
			state = !pw->attrib.showLicense;
			pw->attrib.showLicense = state;

			// record_ref lRef = pw->attrib.licenseFile;
			if (pw->attrib.showLicense)
				PostMessage(M_SEL_LICENSE);

			pw->attribDirty = TRUE;
			
			// pw->Unlock();
			break;
		}
		case M_DO_INSTALLFOLDER: {
			// pw->Lock();
			bool state;			// respond to checkbox click
			state = !pw->attrib.doInstallFolder;
			pw->attrib.doInstallFolder = state;
			pw->attribDirty = TRUE;
			// pw->Unlock();
			break;
		}
		case M_DO_FOLDERPOPUP: {
			bool state;			// respond to checkbox click
			state = !pw->attrib.doFolderPopup;
			pw->attrib.doFolderPopup = state;
			pw->attribDirty = TRUE;
			break;
		}
		case M_DO_PACKAGEHELP: {
			bool state;
			state = !pw->attrib.doPackageHelp;
			pw->attrib.doPackageHelp = state;
			pw->attribDirty = TRUE;
			break;
		}
		case M_DO_ABORTSCRIPT: {
			pw->attrib.abortScript = !pw->attrib.abortScript;
			pw->attribDirty = TRUE;
			break;
		}
		case M_SEL_LICENSE: {
			if (msg->HasRef("refs")) {
				// set the license
				entry_ref	ref;
				msg->FindRef("refs",0,&ref);
				
				((BStringView *)FindView("filename"))->SetText(ref.name);
			
				//pw->Lock();
				BPath	p;
				BEntry	e(&ref);
				e.GetPath(&p);
				pw->attrib.licenseFile = p;
				
				pw->attribDirty = TRUE;
				// pw->Unlock();
			}
			else {
				if (!TryActivate(licPanelMsngr)) {				
					BFilePanel *panel = new MFilePanel(B_OPEN_PANEL,
													new BMessenger(this),
													NULL,
													false,
													true,
													M_SEL_LICENSE,
													NULL,
													"Select");
													
					licPanelMsngr = BMessenger(panel->Window());
					panel->Show();
				}
			}
			break;
		}
		case M_SET_DESCRIPTION: {
			SetDescription();
			break;
		}
		/**  // SetHelp called while cleaning window
		case M_SET_PACKAGEHELP:
			SetHelp()
			break;
		**/
		case M_INST_FOLDER: {
			SetInstallFolder();
			break;
		}
	}
}

///////////////////////////////////////////////////////////

InstSetView::InstSetView(BRect frame,const char *name,PackWindow *parW)
	: BView(frame,name,B_FOLLOW_ALL,B_WILL_DRAW),
	pw(parW)
{
	SetViewColor(light_gray_background);
	BRect r = Bounds();

	const long CWindowInset = 10;
	const long CCheckBoxInset = 18;
	
	r.InsetBy(CWindowInset,CWindowInset);
	/////////////////// create install folder checkbox ///////////////////
	r.bottom = r.top + 14;
	
	TCheckBox *doFolder = new TCheckBox(r,"dofolder","Create Install Folder",
								new BMessage(M_DO_INSTALLFOLDER));
	AddChild(doFolder);
	
	/////////////////// create folder name text control ///////////////////
	
	r.top = r.bottom + 6;
	r.bottom = r.top + 16;
	r.left += CCheckBoxInset;
	// get folder name
	// description
	// licenseRef
	char *topname = ((PackList *)parW->FindView("listing"))->toplevel->name;
	
	inFolderText = new BTextControl(r,"foldertext",
			"Install Folder:",topname,new BMessage(M_INST_FOLDER));
	inFolderText->SetModificationMessage(new BMessage(M_INFO_MODIFIED));
	
	inFolderText->SetDivider(100);
	AddChild(inFolderText);


	doFolder->AddSlave(inFolderText);
	if (pw->attrib.doInstallFolder)
		doFolder->SetValue(B_CONTROL_ON);
	else
		doFolder->SetValue(B_CONTROL_OFF);
	
	// unindent rect
	r.left -= CCheckBoxInset;
	
	
	r.top = r.bottom + 8;
	r.bottom = r.top + 14;
	
	BCheckBox *cb;
	cb = new BCheckBox(r,"dopopup","Display Folder Selection Menu",
								new BMessage(M_DO_FOLDERPOPUP));
	AddChild(cb);
	cb->SetValue(pw->attrib.doFolderPopup ? B_CONTROL_ON : B_CONTROL_OFF);
	
	r.OffsetBy(0,18);
	cb = new BCheckBox(r,"doscript","Abort install on non-zero script exits",
								new BMessage(M_DO_ABORTSCRIPT));
	AddChild(cb);
	cb->SetValue(pw->attrib.abortScript);
	
	/////////////////// package description text field ///////////////////
	
	r.top = r.bottom + 8;
	r.bottom = r.top + 100;
	desc = new STextField(r,"description","Install Description:",B_EMPTY_STRING,
					new BMessage(M_SET_DESCRIPTION), 
					B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
					B_WILL_DRAW | B_NAVIGABLE,
					TRUE);
	desc->SetModificationMessage(new BMessage(M_INFO_MODIFIED));
	AddChild(desc);
	
	/////////////////// package help checkbox ///////////////////

	r.top = r.bottom + 12;
	r.bottom = r.top + 14;
	// r.right = Bounds().Width()/2.0 - 3.0;
	
	TCheckBox *doPkgHelp = new TCheckBox(r,"dopkghelp","Display Package Help",
								new BMessage(M_DO_PACKAGEHELP));
	AddChild(doPkgHelp);
	
	/*
	/////////////////// groups help checkbox ///////////////////
	
	BRect rr = r;
	rr.left  = rr.right + 3.0;
	rr.right = Bounds().right - CWindowInset;
	
	TCheckBox *doGrpHelp = new TCheckBox(rr,"dogrphelp","Display Groups Help",
								new BMessage(M_DO_GROUPSHELP));
	AddChild(doGrpHelp);
	*/
	
	/////////////////// package help text field ///////////////////
	r.top = r.bottom + 6;
	r.bottom = r.top + 80;
	r.left += CCheckBoxInset;
	
	pkgHelp = new STextField(r,"pkghelp","Help Text:",B_EMPTY_STRING,
					new BMessage('    '),
					B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
					B_WILL_DRAW | B_NAVIGABLE,
					TRUE);
	pkgHelp->SetModificationMessage(new BMessage(M_INFO_MODIFIED));
	AddChild(pkgHelp);

	doPkgHelp->AddSlave(pkgHelp);

	/*
	/////////////////// groups help text field ///////////////////
	rr = r;
	rr.left = rr.right + 3.0 + CCheckBoxInset;
	rr.right = Bounds().right - CWindowInset;
	
	grpsHelp = new STextField(rr,"grpshelp","Help Text:",B_EMPTY_STRING,
					new BMessage('    '),
					B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
					B_WILL_DRAW | B_NAVIGABLE,
					TRUE);
	grpsHelp->SetModificationMessage(new BMessage(M_INFO_MODIFIED));
	AddChild(grpsHelp);

	doGrpHelp->AddSlave(grpsHelp);
	*/
	
	/////
	r.left -= CCheckBoxInset;
	
	/////////////////// display license checkbox ///////////////////
	
	r.top = r.bottom + 16;
	r.bottom = r.top + 16;
	r.right = Bounds().right - CWindowInset;
	
	TCheckBox *doLicense = new TCheckBox(r,"dolicense","Display Text at Installer Open",
								new BMessage(M_DO_LICENSE));
	AddChild(doLicense);

	/////////////////// select file button ///////////////////
	
	r.top = r.bottom + 6;
	r.bottom = r.top + 18;
	r.left += CCheckBoxInset;
	r.right = r.left + 82;
	
	BButton *selectFile = new BButton(r,"selfile","Select File...",new BMessage(M_SEL_LICENSE));
	AddChild(selectFile);

	doLicense->AddSlave(selectFile);
	
	/////////////////// filename string view ///////////////////
		
	r.left = r.right + 12;
	r.right = Bounds().right - 8;
	
	lFilename = new BStringView(r,"filename",B_EMPTY_STRING);
	lFilename->SetViewColor(light_gray_background);
	AddChild(lFilename);

	BPath *lPath = &pw->attrib.licenseFile;
	
	BEntry lic(lPath->Path());
	if (lic.InitCheck() == B_NO_ERROR) {
		lFilename->SetText(lPath->Leaf());
	}
}

void InstSetView::AllAttached()
{
	BView::AllAttached();
		
	desc->SetText(pw->attrib.descriptionText);
	desc->SetTarget(Window());
	
	PackWindow *pw = ((InstSetWindow *)Window())->pw;
	BControl *cntl = (BControl *)FindView("dopkghelp");
	cntl->SetValue(pw->attrib.doPackageHelp);
	pkgHelp->SetText(pw->attrib.packageHelpText);

	/**
	cntl = (BControl *)FindView("dogrphelp");
	cntl->SetValue(pw->attrib.doGroupHelp);
	grpsHelp->SetText(pw->attrib.groupHelpText);
	**/
	
	cntl = (BControl *)FindView("dolicense");
	cntl->SetValue(pw->attrib.showLicense);
	
	MakeFocus();
}
