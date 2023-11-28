// InstallView.cpp
#include <ScrollView.h>
#include <Box.h>
#include <MenuField.h>
#include <string.h>
#include <Button.h>
#include <FilePanel.h>
#include <NodeMonitor.h>
#include "InstallView.h"
#include "InstallWindow.h"
#include "InstallMessages.h"
#include "IGroupsListView.h"
#include "IMenuBar.h"
#include "IArchiveItem.h"
#include "MyVolumeQuery.h"
#include "PackAttrib.h"
#include "LabelView.h"
#include <Path.h>
#include <PopUpMenu.h>

#include "Util.h"
#include "HelpButton.h"
#include "SettingsManager.h"

#include "MyDebug.h"

//#include "NiceBox.h"
#include "IconMenuItem.h"
#include "FSIcons.h"

extern SettingsManager *gSettings;

static char *defaultFolders[] =
{
	"apps",
	"home",
	"home/Desktop"
};


InstallView::InstallView(BRect frame,
				const char *name,
				ulong resizeMask,
				ulong flags,GroupList *grps,char *description)
	: BView(frame,name,resizeMask,flags)
{
	groupList = grps;
	dText = description;
}

InstallView::~InstallView()
{
}

void InstallView::AttachedToWindow()
{
	BView::AttachedToWindow();
	
#if USING_HELP
	const long CHelpBtnSize = 18;
	HelpButton::InitPicture(this);
#endif

	//////////////////////////
	
	BRect r = Bounds();	
	///////////// package description text view ///////////
	
	r.InsetBy(8,2);
	r.bottom = r.top + VIEW_TOP_FRAME;	
	
	LabelView *infoView = new LabelView(r,dText);
	AddChild(infoView);	

	InstallWindow *wind = (InstallWindow *)Window();
#if USING_HELP	
	//////////// package help button ///////////////
	if (wind->attr->packageHelpText) {
		PRINT(("ading package help button\n"));
		
		PRINT(("help string %s, len %d\n", wind->attr->packageHelpText,
							strlen(wind->attr->packageHelpText)));
		BRect rr = r;
		rr.right = Bounds().right;
		rr.InsetBy(6,4);

		rr.top = rr.bottom - CHelpBtnSize;
		rr.left = rr.right - CHelpBtnSize;
		
		HelpButton *pkgHelpBtn =
				new HelpButton(rr,B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT,
								"Package Help",
								wind->attr->packageHelpText);
		AddChild(pkgHelpBtn);

		// make room for button
		infoView->ResizeBy(rr.left - infoView->Frame().right,0);
		BRect textRect = infoView->Bounds();
		textRect.InsetBy(2,2);
		
		infoView->SetTextRect(textRect);
	}
#endif
	
	///////////////////// groups list view //////////////////////////
	PRINT(("groups list view\n"));
	
	r.left += 8;
	r.top = r.bottom + 30;
	r.bottom = Bounds().bottom - VIEW_BOTTOM_FRAME - 10;
	r.right = r.left + Bounds().Width()*0.4;
	
	IGroupsListView *gv = new IGroupsListView(r,groupList,this);
	if (gv->CountItems() > 0)
		gv->SelectItem(0L);

	BScrollView *scroller = new BScrollView("scroller",gv,B_FOLLOW_ALL,0,FALSE,TRUE);
	AddChild(scroller);

	gv->MakeFocus(TRUE);

	BRect labelR = r;
	labelR.left += 6;
	labelR.top -= 21;
	labelR.bottom = labelR.top + 12;
	
	BStringView *listLabel = new BStringView(labelR,"listlabel","Groups",B_FOLLOW_LEFT,B_WILL_DRAW);
	AddChild(listLabel);
	listLabel->SetViewColor(light_gray_background);
	listLabel->SetLowColor(light_gray_background);
	listLabel->SetFont(be_plain_font);
	//listLabel->SetFontSize(9);

	//////////////////////// group description box ///////////////////////
	
	BRect boxR = r;
	boxR.left = boxR.right + 28;
	boxR.right = Bounds().right - 8;
	boxR.top -= 20;
	boxR.bottom += 3;
	
	BBox *descBox = new BBox(boxR,"descbox",B_FOLLOW_RIGHT | B_FOLLOW_TOP,B_WILL_DRAW);
	AddChild(descBox);
	descBox->SetViewColor(light_gray_background);
	descBox->SetLowColor(light_gray_background);
	descBox->SetLabel("Group Information");
	
	/////////////////// group description text view //////////////
	r = boxR;
	r.OffsetTo(0,0);
	r.InsetBy(3,16);
	r.bottom -= 30;
	
	descText = new LabelView(r,B_EMPTY_STRING);
	descBox->AddChild(descText);
	
#if USING_HELP
	//////////// groups help button ///////////////

	PRINT(("groups help button\n"));
	r = boxR;
	r.OffsetTo(0,0);
	r.InsetBy(4,4);
	
	r.top = r.bottom - CHelpBtnSize;
	r.left = r.right - CHelpBtnSize;
	
	grpsHelpBtn = new HelpButton(r,B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT,
							"Groups Help",
							B_EMPTY_STRING);
	descBox->AddChild(grpsHelpBtn);
	grpsHelpBtn->BView::Hide();
#endif
	 
	//////////////////// size of install string view ///////////////////////////
	r = boxR;
	r.OffsetTo(0,0);
	r.InsetBy(3,0);
	
	r.bottom -= 18;
	r.top = r.bottom - 12;
	
	sizeText = new BStringView(r,"sizetext",B_EMPTY_STRING,B_FOLLOW_LEFT,B_WILL_DRAW);
	descBox->AddChild(sizeText);
	sizeText->SetViewColor(light_gray_background);
	sizeText->SetLowColor(light_gray_background);

	/////////////////// space on volume sting view //////////////
	r.top = r.bottom + 4;
	r.bottom = r.top + 12;
	r.right -= 20;
	
	spaceFree = new BStringView(r,"spacefree",B_EMPTY_STRING,B_FOLLOW_LEFT,B_WILL_DRAW);
	descBox->AddChild(spaceFree);
	spaceFree->SetViewColor(light_gray_background);
	spaceFree->SetLowColor(light_gray_background);
	
	/////////////////// Volume popup menu ////////////////////////////
	r = Bounds();
	r.left += 8;
	r.top = r.bottom - VIEW_BOTTOM_FRAME + 6;
	r.bottom = r.top + 22;
	r.right = r.left + 75 + VOLUME_MENU_MAX;
	
	volPopup = new BPopUpMenu("volumes");
	volPopup->SetFont(be_plain_font);
	
	BMenuField *volMenu = new BMenuField(r,"Volume","Install on",volPopup,B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	volMenu->SetDivider(VOL_DIVIDER);
	volMenu->SetFont(be_plain_font);
	AddChild(volMenu);
	volMBar = volMenu->MenuBar();
	volMBar->SetFont(be_plain_font);
	volMenu->ResizeBy(0,4);

	///////////////////////// volumes ////////////////////////
	VRoster.StartWatching(BMessenger(this));
	
	BVolume curVolume;
	VRoster.GetBootVolume(&curVolume);
	long bootVolID = curVolume.Device();
	
	VRoster.Rewind();
	long bootVolIndex;
	long index = 0;
	while(TRUE) {
		int32 err;
		err = VRoster.GetNextVolume(&curVolume);
		if (err < B_NO_ERROR)
			break;
		if (!curVolume.IsPersistent())
			continue;
		BBitmap *rootSIcon = SIconOfVolume(&curVolume);

		/// getting volume id
		BMessage *volMsg = new BMessage(M_VOL_SELECTED);
		volMsg->AddInt32("volume_id",curVolume.Device());
		if (curVolume.Device() == bootVolID)
			bootVolIndex = index;
		
		char name[B_FILE_NAME_LENGTH];
		curVolume.GetName(name);

		IconMenuItem *mitem = new IconMenuItem(name,rootSIcon,volMsg);
		mitem->SetTarget(this);
		volPopup->AddItem(mitem);
		
		index++;
	}
	//////////////// Set default volume info //////////////
									
	IconMenuItem *bootItem = (IconMenuItem *)volPopup->ItemAt(bootVolIndex);
	bootItem->SetMarked(TRUE);
	bootItem->MyInvoke();
	
	// hack to remove existing menu item and allow us to add an icon item
	volSuperItem = new IconMenuItem(bootItem->GetIcon(),volPopup);
	volMBar->RemoveItem(0L);
	volMBar->AddItem(volSuperItem);
	
	///////////////////// folders menu //////////////////////////
	r.left = r.right + 4;
	r.right = r.left + 20 + FOLDER_MENU_MAX;
	
	foldPopup = new BPopUpMenu("folders",FALSE,FALSE);
	foldPopup->SetFont(be_plain_font);

	BMenuField *foldMenu = new BMenuField(r,"Folder","in",foldPopup,B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	foldMenu->SetDivider(FOLD_DIVIDER);
	foldMenu->SetFont(be_plain_font);
	AddChild(foldMenu);
	foldMBar = foldMenu->MenuBar();
	foldMBar->SetFont(be_plain_font);
	// compensate for larger menu size due to icons
	foldMenu->ResizeBy(0,4);
	
	foldSuperItem = new IconMenuItem(gGenericFolderSIcon,foldPopup);
	foldMBar->RemoveItem(0L);
	foldMBar->AddItem(foldSuperItem);
	
	
	SetFolderPopup();
	/// select default folder
	prevFolder = 0;
	
	if (!wind->attr->doFolderPopup) {
		foldMenu->Hide();
		wind->dirName = strdup(".");
	}
	else {
		//////////////// Prime selection of default install folder ////
	
#if (!SEA_INSTALLER)
		if (gSettings->data.FindBool("install/usepath")) {
			BMessage defFolder(M_FOLDER_SELECTED);
			BEntry	ent(gSettings->data.FindString("install/path"));
			if (ent.InitCheck() >= B_NO_ERROR) {
				entry_ref	r;
				ent.GetRef(&r);
				defFolder.AddRef("refs",&r);
				Looper()->PostMessage(&defFolder,this);
			}
		}
		else
#endif
			SelectDefaultFolder();
	}
	
	///////////////////////////////////////////////
	/// create install button and set default /////
	r = Bounds();
	r.InsetBy(16,12);
	r.left = r.right - 75;
	r.top = r.bottom - 20;
	
	BButton *installBtn = new BButton(r,"installbutton","Begin",new BMessage(M_DO_INSTALL),
							B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	AddChild(installBtn);
	installBtn->MakeDefault(TRUE);

	//////////////// Prime selection of default groups ///////////////
	
	gv->SelectionSet();
}

void InstallView::SetFolderPopup()
{
	for(long i = 0; i < nel(defaultFolders); i++) {
		char *dirName = defaultFolders[i];
		//directory_which w = defaultFolders[i];
		//BPath path;
		//find_directory(w,&path,false,vol);
		
		IconMenuItem *mitem = new IconMenuItem(dirName,gGenericFolderSIcon,
					new BMessage(M_FOLDER_SELECTED),FALSE);
		mitem->SetTarget(this);
		foldPopup->AddItem(mitem);
	}
	foldPopup->AddSeparatorItem();
	BMenuItem *mitem = new BMenuItem("Other...",new BMessage(M_FOLDER_SELECTED));
	mitem->SetTarget(this);
	foldPopup->AddItem(mitem);
}

void InstallView::Draw(BRect update)
{
	BView::Draw(update);

	BRect b = Bounds();	
	DrawHSeparator(0,b.right, VIEW_TOP_FRAME+4, this);
}

void	InstallView::SelectDefaultFolder()
{
	IconMenuItem *it = (IconMenuItem *)foldPopup->ItemAt(0);
	it->SetMarked(TRUE);
	it->MyInvoke();
}

void	InstallView::SelectDefaultVolume()
{
	IconMenuItem *it = (IconMenuItem *)volPopup->ItemAt(0);
	it->SetMarked(TRUE);
	it->MyInvoke();
}

void	InstallView::MessageReceived(BMessage *msg)
{
	PRINT(("Install view got messsage\n"));
	#if (DEBUG)
		msg->PrintToStream();
	#endif
	switch(msg->what) {
		case M_VOL_SELECTED: {
			PRINT(("Volume selected message\n"));
			// volume selected in menu
			InstallWindow *wind = (InstallWindow *)Window();
			long volID = msg->FindInt32("volume_id");
			// if volume changed and custom destination
			// reset to default destination
			// (unless the volumes were the same!)
			if (wind->volumeID != volID && wind->custom) {
				SelectDefaultFolder();
			}
			
			wind->volumeID = volID;

			// set menu bar
			long menuIndex = msg->FindInt32("index");
			IconMenuItem *vit = (IconMenuItem *)volPopup->ItemAt(menuIndex);
			volSuperItem->SetIcon(vit->GetIcon());
			volMBar->Invalidate();
			
			allocBlockSize = 1024;
			
			PRINT(("volID is %d\n",volID));	
			Looper()->PostMessage(M_VOL_INFO,this);
			
			break;
		}
		case M_VOL_INFO: {
			InstallWindow *wind = (InstallWindow *)Window();
			BVolume	theVol(wind->volumeID);
			off_t bytes = theVol.FreeBytes();
			// PRINT(("*************** \nspace on volume is %f MB\n",bytes/(1024.0*1024.0) + 0.05));
			
			char buf[B_FILE_NAME_LENGTH + 24];
			char vname[B_FILE_NAME_LENGTH];
			theVol.GetName(vname);
			if (bytes < 1024*1024)
				sprintf(buf,"%d K free on \"%s\"",(int)((float)bytes/1024.0),vname);
			else
				sprintf(buf,"%.1f MB free on \"%s\"",(double)bytes/(1024.0*1024.0),vname);
			
			spaceFree->SetText(buf);
			// update so 
			wind->UpdateIfNeeded();
			
			if (!wind->CheckPossibleInstall(theVol)) {			
			};
			
			break;
		}
		case M_FOLDER_SELECTED: {
			InstallWindow *wind = (InstallWindow *)Window();
			if (msg->HasRef("refs")) {
				// custom folder selected in menu
				// close the file panel
				// hope these aren't nested!
				
				msg->FindRef("refs",&wind->instDirRef);
				
				BEntry	fold(&wind->instDirRef);
				if ((fold.InitCheck() < B_NO_ERROR) || !fold.IsDirectory())
					break;

				wind->custom = TRUE;
				
				// fixup menu
				foldPopup->ItemAt(prevFolder)->SetMarked(FALSE);
				foldPopup->ItemAt(foldPopup->CountItems()-1)->SetMarked(TRUE);
				prevFolder = foldPopup->CountItems()-1;

				// char buf[B_FILE_NAME_LENGTH];
				BPath path;
				fold.GetPath(&path);
				foldMBar->ItemAt(0)->SetLabel(path.Path());

				// make sure the correct volume is selected
				long vID;
				{
					// works for mount points at any location?
					BDirectory	foldDir(&fold);
					BVolume v;
					fold.GetVolume(&v);
					vID = v.Device();
				}
				/** this doesn't work for top level mnt points **/
				//else
				//	vID = wind->instDirRef.device;
				long i;
				for (i = volPopup->CountItems()-1; i >= 0; i--) {
					IconMenuItem *mit = (IconMenuItem *)volPopup->ItemAt(i);
					BMessage *itemMsg = mit->Message();
					if (itemMsg->FindInt32("volume_id") == vID) {
						wind->volumeID = vID;
						mit->SetMarked(TRUE);
						mit->MyInvoke();
						break;
					}
				}
				if (i == -1) {
					doError("Volume not found!");
					SelectDefaultFolder();
				}
				break;
			}
			long itemInd = msg->FindInt32("index");
			if (itemInd == foldPopup->CountItems()-1) {
				// do other
				PRINT(("calling run save panel\n"));
				
				// hope these aren't nested
				BFilePanel *pan = new BFilePanel(B_OPEN_PANEL,
							new BMessenger(this),
							NULL,
							B_DIRECTORY_NODE,
							false,
							new BMessage(msg->what));
				pan->Show();
			}
			else {
				// default folder selected
				foldPopup->ItemAt(prevFolder)->SetMarked(FALSE);
				foldPopup->ItemAt(itemInd)->SetMarked(TRUE);
				foldMBar->ItemAt(0)->SetLabel(foldPopup->ItemAt(itemInd)->Label());
				prevFolder = itemInd;
				const char *dname = foldPopup->ItemAt(itemInd)->Label();

				free(wind->dirName);					
				wind->dirName = strdup(dname);
				wind->custom = FALSE;
			}
			break;
		}
		case B_NODE_MONITOR: {
			switch(msg->FindInt32("opcode")) {
				case B_DEVICE_MOUNTED: {
					PRINT(("volume mounted\n"));
					// add to the menu
					BVolume curVolume(msg->FindInt32("new device"));
			
					BBitmap *rootSIcon = SIconOfVolume(&curVolume);
					
					/// getting volume id
					BMessage *volMsg = new BMessage(M_VOL_SELECTED);
					volMsg->AddInt32("volume_id",curVolume.Device());
					
					char name[B_FILE_NAME_LENGTH];
					PRINT(("getting volume name..."));
					curVolume.GetName(name);
					PRINT(("got it\n"));			
					IconMenuItem *mitem = new IconMenuItem(name,rootSIcon,volMsg);
					mitem->SetTarget(this);
					volPopup->AddItem(mitem);
					break;
				}
				case B_DEVICE_UNMOUNTED: {
					PRINT(("volume was unmounted\n"));					
					// scan through the menu getting the messages
					// if the volume id is the same, then remove it
					dev_t devid = msg->FindInt32("device");
					for (long i = volPopup->CountItems()-1; i >= 0; i--) {
						IconMenuItem *mit = (IconMenuItem *)volPopup->ItemAt(i);
						BMessage *itemMsg = mit->Message();
						if (devid == itemMsg->FindInt32("volume_id")) {
							volPopup->RemoveItem(i);
							// if the item unmounted was currently selected
							if (mit->IsMarked()) {
								volSuperItem->SetIcon(NULL);  // hack
								// send a message to select a default volume
								// (there is always at least one volume!)
								SelectDefaultVolume();
								// SelectDefaultFolder();
							}
							// this destructor deletes the icon?
							delete mit; // defer since icon is in use by super item?
							break;
						}
					}
					break;
				}
			}
			break;
		}
		case M_ITEMS_SELECTED: {
			InstallWindow *w = (InstallWindow *)Window();
			
			if (msg->FindInt32("groups") == 0) {
				((BButton *)FindView("installbutton"))->SetEnabled(FALSE);
				sizeText->SetText(B_EMPTY_STRING);
				descText->SetText(B_EMPTY_STRING);
#if USING_HELP
				grpsHelpBtn->Hide();
#endif
			}	
			else {
				// set description text
				descText->SetText(msg->FindString("description"));
			
#if USING_HELP
				// set help text
				long	err;
				char *txt;
				err = msg->FindPointer("helptext",(void **)&txt);
				if (err >= B_NO_ERROR && txt) {
					grpsHelpBtn->Show();
					grpsHelpBtn->SetHelpText(txt);
				}
				else {
					grpsHelpBtn->Hide();
					//
				}
#endif			
				ArchiveFolderItem *top = w->topfolder;
				
				FileEntry	*entryList = w->entryList;
				long		numEntries = w->entryCount;
				
				ulong selGrps = w->selectedGroups = msg->FindInt32("groups");
				ulong selPlatforms = w->selectedPlatforms;
				long numItems = 0;
				long bytes = 0;

//#if (!SEA_INSTALLER)
				if (w->lowMemMode || !top) {
					for (FileEntry *entry = entryList+numEntries-1; entry >= entryList; entry--) {
						if ((entry->groups & selGrps) &&
							(entry->platform & selPlatforms))
						{
							numItems++;
							long sz = entry->size;
							long rem = sz % allocBlockSize;
							bytes += sz + (rem ? allocBlockSize - rem : 0);
						}
					}
				}
//				else if (top) {
//#endif
//					top->GetGroupInfo(msg->FindInt32("groups"),numItems,bytes,allocBlockSize);
//#if (!SEA_INSTALLER)
//				}
				else {
					doError(errNOTOPLEVEL);
				}
//#endif
				char buf[80];
				if (bytes < 1024)
					sprintf(buf,"%d files   %d bytes selected",numItems,bytes);
				else if (bytes < 1024*1024)
					sprintf(buf,"%d files    %d K selected",numItems,bytes/1024);
				else
					sprintf(buf,"%d files    %.1f MB selected",
						numItems,(float)bytes/(1024.0*1024.0) + 0.05);
				
				w->groupBytes = bytes;
				w->itemCount = numItems;
				sizeText->SetText(buf);
				((BButton *)FindView("installbutton"))->SetEnabled(TRUE);
			}
			break;
		}
		default:
			break;
	}
}
