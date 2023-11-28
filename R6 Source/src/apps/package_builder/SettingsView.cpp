#include <Be.h>
// SettingsView.cpp

#include "SettingsView.h"
#include "PackMessages.h"
#include "SmallPopup.h"
#include "NameDialog.h"
#include "Attributes.h"

#include "RList.h"
#include "PackApplication.h"
#include "PackWindow.h"
#include "PackListView.h"
#include "FArchiveItem.h"

#include "GroupsWindow.h"
#include "DestinationWindow.h"
//#include "CondMessages.h"

#include "Replace.h"

#include "Util.h"
#include "MyDebug.h"
#undef DEBUG
#define DEBUG 0

enum {
	M_AUTO_SET = 	'auto'
};

struct reploption kReplaceOptions[] = {
	{ "Always ask user",R_ASK_USER },
	{ "Never replace",R_NEVER_REPLACE},
	{ "Rename existing item",R_RENAME},
	{ "Ask user if newer version", R_ASK_VERSION_NEWER},
	{ "Ask user if newer creation date", R_ASK_CREATION_NEWER},
	{ "Ask user if newer modification date", R_ASK_MODIFICATION_NEWER},
	{ "Replace if newer version",R_REPLACE_VERSION_NEWER},
	{ "Replace if newer creation date",R_REPLACE_CREATION_NEWER},
	{ "Replace if newer modification date",R_REPLACE_MODIFICATION_NEWER},
	{ "Merge with existing folder",R_MERGE_FOLDER},
	// {"Always Replace",R_ALWAYS_REPLACE}, ??
	{ "Install only if item exists",R_INSTALL_IF_EXISTS}
};

SettingsView::SettingsView(BRect bounds,
	const char *name, ulong resizeFlags, ulong flags)
	: BBox(bounds,name,resizeFlags,flags), mergeOptionIndex(0)
{
}

SettingsView::~SettingsView()
{
	//mcheckall();
}

void SettingsView::AttachedToWindow()
{
	BBox::AttachedToWindow();
	
	SetFont(be_plain_font);
	SetLowColor(light_gray_background);
	SetViewColor(light_gray_background);
	/////////////////////////////////////////////////////////////////
	
	/////////////////////////////////////////////////////////////////
	
	BRect r;
	BRect rect = Bounds();
	rect.InsetBy(10,10);
	rect.top += 4;
	rect.bottom = rect.top + 16;
	rect.right = rect.left + 64;
		
	groupsPopup = new SmallPopUpMenu("Groups");
	groupsMenu = new BMenuField(rect,"Groups",B_EMPTY_STRING,groupsPopup);
	groupsMenu->SetEnabled(FALSE);
	groupsMenu->SetDivider(0);
	groupsMenu->MenuBar()->SetFont(be_plain_font);
	AddChild(groupsMenu);
	groupsMenu->MenuBar()->SetFlags(groupsMenu->MenuBar()->Flags() | B_FOLLOW_ALL);
	//////////////////// add existing names here!!!


	/////////////////////////////////////////////////////////////////
	
	BRect sr = rect;
	sr.left = sr.right + 12;
	sr.right = Bounds().right - 5;

	BStringView *grpStrView = new BStringView(sr,"names",B_EMPTY_STRING);
	AddChild(grpStrView);
	grpStrView->SetViewColor(light_gray_background);
	grpStrView->SetFont(be_plain_font);
	
	/////////////////////////////////////////////////////////////////

	sr = rect;
		
	sr.top = sr.bottom + 7;
	sr.bottom = sr.top + 16;
	sr.right = Bounds().right - 5;
	
	destPopup = new SmallPopUpMenu("Destination",FALSE,FALSE);
	
	destMenu = new BMenuField(sr,"Destination","Destination:",
								destPopup);
	AddChild(destMenu);
	destMenu->SetDivider(68);
	destMenu->SetEnabled(FALSE);
	destMenu->SetFont(be_plain_font);
	destMenu->MenuBar()->SetFont(be_plain_font);
	destMenu->MenuBar()->SetFlags(destMenu->MenuBar()->Flags() | B_FOLLOW_ALL);
	/////////////////////////////////////////////////////////////////////
	sr.top = sr.bottom + 7;
	sr.bottom = sr.top + 16;
				
	replPopup = new SmallPopUpMenu("Replace",FALSE,TRUE);
	
	replMenu = new BMenuField(sr,"Replace","Replacement:",replPopup);
	
	AddChild(replMenu);
	replMenu->SetDivider(68);
	replMenu->SetEnabled(FALSE);
	replMenu->SetFont(be_plain_font);
	replMenu->MenuBar()->SetFont(be_plain_font);
	replMenu->MenuBar()->SetFlags(replMenu->MenuBar()->Flags() | B_FOLLOW_ALL);

	//////////////////////////////////////////////////////////////////////
	BMenuItem *mitem;
	
	for(long i = 0; i < R_END_REPLACE; i++) {
		BMessage	*m = new BMessage(M_REPL_SELECTED);
		m->AddInt32("repl",kReplaceOptions[i].code);
		if (kReplaceOptions[i].code == R_MERGE_FOLDER) {
			mergeOptionIndex = i;
		}
		mitem = new BMenuItem(kReplaceOptions[i].name,m);
		
		mitem->SetTarget(this);
		mitem->SetMarked(FALSE);
		replPopup->AddItem(mitem);
	}
	
	
#if CONDITIONAL_INSTALL
	BPopUpMenu *pop;
	
	///////////////////////////////////////
	sr.top = sr.bottom + 7;
	sr.bottom = sr.top + 16;
	r = sr;
	r.right = r.left + 60;
	r.OffsetBy(0,2);
	
	// screwed up ... supposed to be checkbox here
	
	ifMenu = new BMenuField(r,"ififnot",B_EMPTY_STRING,pop,
							B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	AddChild(ifMenu);
	ifMenu->SetDivider(0);
	ifMenu->SetEnabled(false);
	
	pop = new BPopUpMenu(B_EMPTY_STRING,true);
	pop->AddItem(new BMenuItem("PowerPC",new BMessage(M_COND_SELECTED)));
	pop->AddItem(new BMenuItem("Intel",new BMessage(M_COND_SELECTED)));
	pop->AddItem(new BMenuItem("Found v1.0",new BMessage(M_COND_SELECTED)));
	pop->SetTargetForItems(this);
	
	r.left = r.right;
	r.right = sr.right;
	condMenu = new BMenuField(r,"conditions",B_EMPTY_STRING,pop,
							B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	AddChild(condMenu);
	condMenu->SetDivider(0);
	condMenu->SetEnabled(false);
#endif
	BStringView *sv;

	sr.top = sr.bottom + 7;
	sr.bottom = sr.top + 16;
	r = sr;
	sr.right = sr.left + 90;
	
	archPopup = new SmallPopUpMenu("Architectures");
	archMenu = new BMenuField(sr,"Architectures",B_EMPTY_STRING,archPopup);
	archMenu->SetEnabled(false);
	archMenu->SetDivider(0);
	archMenu->MenuBar()->SetFont(be_plain_font);
	AddChild(archMenu);
	archMenu->MenuBar()->SetFlags(archMenu->MenuBar()->Flags() | B_FOLLOW_ALL);
	
	r.left = sr.right;
	sv = new BStringView(r,"platform",B_EMPTY_STRING);
	sv->SetViewColor(ViewColor());
	AddChild(sv);
	
	///////////////////////////////////////
	sr.left += 1;
	sr.top = sr.bottom + 7;
	sr.bottom = sr.top + 16;
	r = sr;

	sr.right = sr.left + 64;
	sv = new BStringView(sr,"libs","Libraries:");
	sv->SetViewColor(ViewColor());
	sv->SetHighColor(disabled_color);
	AddChild(sv);
	sv->SetFont(be_plain_font);
	sv->Hide();
	
	r.left = sr.right;
	sv = new BStringView(r,"deps",B_EMPTY_STRING);
	sv->SetViewColor(ViewColor());
	AddChild(sv);
	sv->Hide();
}

void SettingsView::Draw(BRect up)
{
	BBox::Draw(up);
}

void SettingsView::SetupAttributes()
{
	PackWindow *wind = (PackWindow *)(Window());
	BMenuItem *mitem;
	
	/////////////////// do groups popup menu ///////////////////
	
	GroupList *gl = wind->attrib.groupList;
	long count = gl->viewList->CountItems();
	for (long i = 0; i < count; i++)
	{
		IndexItem *ind = gl->viewList->ItemAt(i);
		if (ind->index == -1) {
			groupsPopup->AddSeparatorItem();
		}
		else {
			mitem = new BMenuItem(gl->masterList->ItemAt(ind->index)->name,
									new BMessage(M_GROUP_SELECTED));
			mitem->SetTarget(this);
			mitem->SetMarked(FALSE);
			groupsPopup->AddItem(mitem);
		}
	}
	groupsPopup->AddSeparatorItem();
	
	///////////////////// add commands ////////////////////////////
	
	mitem = new BMenuItem("New Group...",new BMessage(M_NEW_GROUP));
	mitem->SetTarget(this);
	mitem->SetEnabled(TRUE);
	groupsPopup->AddItem(mitem);
	
	mitem = new BMenuItem("Edit Groups...",new BMessage(M_EDIT_GROUPS));
	mitem->SetTarget(this);
	mitem->SetEnabled(TRUE);
	groupsPopup->AddItem(mitem);
	

	/////////////// dest Popup, add default items ////////////

	BMessage *destMsg;
	
	destMsg = new BMessage(M_DEST_SELECTED);
	destMsg->AddInt32("dest",D_INSTALL_FOLDER);
	destMsg->AddBool("custom",FALSE);

	mitem = new BMenuItem("Install Folder",destMsg);
	///////////
	mitem->SetTarget(this);
	mitem->SetMarked(FALSE);
	destPopup->AddItem(mitem);
	
	destMsg = new BMessage(M_DEST_SELECTED);
	destMsg->AddInt32("dest",D_PARENT_FOLDER);
	destMsg->AddBool("custom",FALSE);
	
	mitem = new BMenuItem("Parent Folder",destMsg);
	///////////////
	
	mitem->SetTarget(this);
	mitem->SetEnabled(FALSE); // only enabled when there is a parent folder
	mitem->SetMarked(FALSE);
	destPopup->AddItem(mitem);
	
	/////////////// add default dest items /////////////////////////////
	
	destPopup->AddSeparatorItem();
	RList<DestItem *> *dl = wind->attrib.defaultDestList;
	count = dl->CountItems();
	for (long i = 0; i < count; i++)
	{
		destMsg = new BMessage(M_DEST_SELECTED);
		destMsg->AddInt32("dest",i);
		//destMsg->Add
		destMsg->AddBool("custom",false);
		
		const char *title = dl->ItemAt(i)->findName;
		if (!title) title = dl->ItemAt(i)->path;
		mitem = new BMenuItem(title,destMsg);
		mitem->SetTarget(this);
		mitem->SetMarked(false);
		destPopup->AddItem(mitem);
	}
	destPopup->AddSeparatorItem();

	// offset to start of custom items
	custStartIndex = destPopup->CountItems();
	////////////// add custom dest items //////////////////////////////
	dl = wind->attrib.customDestList;
	count = dl->CountItems();
	
	for (destCount = 0; destCount < count; destCount++)
	{
		destMsg = new BMessage(M_DEST_SELECTED);
		
		// "dest" should be extracted from index
		// destMsg->AddInt32("dest",destCount);
		destMsg->AddBool("custom",true);
		mitem = new BMenuItem(dl->ItemAt(destCount)->path,destMsg);
		mitem->SetTarget(this);
		mitem->SetMarked(FALSE);
		destPopup->AddItem(mitem);
	}

	if (destCount > 0)
		destPopup->AddSeparatorItem();
	
	///////////////// add extra commands ///////////////////////////////
	mitem = new BMenuItem("Add Custom Path...",new BMessage(M_NEW_DEST));
	mitem->SetTarget(this);
	destPopup->AddItem(mitem);

	mitem = new BMenuItem("Edit Custom Paths...",new BMessage(M_EDIT_DEST));
	mitem->SetTarget(this);
	destPopup->AddItem(mitem);	
	lastMarked = 0;
	
	//////////////// setup architectures ///////////////////////////
	BMessage *archSel;
	archSel = new BMessage(M_ARCH_SELECTED);
	archPopup->AddItem(new BMenuItem("PowerPC",archSel));
	archSel = new BMessage(M_ARCH_SELECTED);
	archPopup->AddItem(new BMenuItem("x86",archSel));
//	archSel = new BMessage(M_ARCH_SELECTED);
//	archPopup->AddItem(new BMenuItem("Hitachi SH",archSel));
	archPopup->AddSeparatorItem();
	archSel = new BMessage(M_ARCH_SELECTED);
	archPopup->AddItem(new BMenuItem("Any Architecture",archSel));
	archPopup->SetRadioMode(true);
	archPopup->SetTargetForItems(this);
}	

/* message contents
	"dest" -- destination index
	"custom" -- is destination custom
	"bitmap" -- selected groups*
	"repl" -- replacement option
	"folder" -- is this a folder
	"deps" -- dependency string
*/
void SettingsView::ItemsSelected(BMessage *msg)
{
	// items selected/deselected in interface
	PackWindow *wind = (PackWindow *)Window();
	long count;
	bool enable = msg->HasInt32("bitmap");

	// check if there is any selection
	if (groupsMenu->IsEnabled() != enable) {
		// we changed from no selection to selection or visa versa
		groupsMenu->SetEnabled(enable);
		destMenu->SetEnabled(enable);
		replMenu->SetEnabled(enable);
		
#if CONDITIONAL_INSTALL		
		installCb->SetEnabled(enable);
#endif
		archMenu->SetEnabled(enable);
		
		BStringView *sv = (BStringView *)FindView("libs");
		if (enable) sv->SetHighColor(0,0,0);
		else sv->SetHighColor(disabled_color);
		sv->Invalidate();
	}
	if (!enable) {
		((BStringView *)FindView("names"))->SetText(B_EMPTY_STRING);
		((BStringView *)FindView("deps"))->SetText(B_EMPTY_STRING);
		((BStringView *)FindView("platform"))->SetText(B_EMPTY_STRING);		
		// disable
#if CONDITIONAL_INSTALL	
		ifMenu->SetEnabled(enable);
		condMenu->SetEnabled(enable);
#endif

		return;
	}
	// we have a valid selection!
	
	GroupList *gList = wind->attrib.groupList;
	
	// first uncheck all menu items in the groups popup menu
	if (groupsPopup->CountItems() - 4 != gList->viewList->CountItems() - 1)
		doError("Incorrect menu count.");
	
	
	// display group names
	const long GR_LEN = 120;
	
	// hack
	char nam[GR_LEN];
	char *to = nam;
	char *fr;
	char *limit = to+GR_LEN;
	
	count = gList->masterList->CountItems();
	
	*to = '\0';
	currentBitmap = msg->FindInt32("bitmap");
	PRINT(("    current bitmap is %o\n",currentBitmap));
	for (int i = 0; i < count && i < 32; i++) {

		long ind = gList->ViewIndexFor(i);
		
		if ((1 << i) & currentBitmap) {
			fr = gList->masterList->ItemAt(i)->name;
			PRINT(("group %s is set, viewindex %d\n",fr,ind));
			while(*fr && to < limit)
				*to++ = *fr++;
			if (to < limit-1) {
				strcpy(to,", ");
				to += 2;
			}
			else {
				// no more chars left in the buffer
				doError("No more space left in buffer");
				break;
			}
			
			// reverse lookup
			groupsPopup->ItemAt(ind)->SetMarked(TRUE);
		}
		else {
			fr = gList->masterList->ItemAt(i)->name;
			PRINT(("group %s is NOT set, viewindex %d\n",fr,ind));
			groupsPopup->ItemAt(ind)->SetMarked(FALSE);
		}
	}
	if (to > nam+1) {
		// some chars were copied, including comma and space
		*(to-2) = '\0';
	}
	
	((BStringView *)FindView("names"))->SetText(nam);
	
	/////////////////////////////////////////////////////////
	////// if the message is merely for updating the group names
	////// we break out here
	
	if (! msg->HasInt32("dest"))
		return;
	////////////////////////////////////////////////////////
		
	long index = msg->FindInt32("dest");
	bool custom = msg->FindBool("custom");
	
	PRINT(("Settings view got DESTINATION %d  CUSTOM %d\n",index,custom));
	
	if (index == D_NO_DEST) {
		destMenu->MenuBar()->ItemAt(0)->SetLabel(B_EMPTY_STRING);
		destPopup->ItemAt(lastMarked)->SetMarked(FALSE);
		return;
	}
	else if (index == D_INSTALL_FOLDER) {
		index = 0;
	}
	else if (index == D_PARENT_FOLDER) {
		index = 1;
	}
	else {
		if (custom)
			index += custStartIndex;
		else
			index += 3;
	}
	PRINT(("Settings view got INDEX %d\n",index));
	
	BMenuItem *item = destPopup->ItemAt(index);
	destPopup->ItemAt(lastMarked)->SetMarked(FALSE);
	item->SetMarked(TRUE);
	lastMarked = index;
	destMenu->MenuBar()->ItemAt(0)->SetLabel(item->Label());
	
	///////////////////////////////////////////////////////
	
	index = msg->FindInt32("replindex");
	bool clearReplaceMenu = false;
	
	if (index == -1) {
		clearReplaceMenu = true;
	} else {
		// if in compatibility mode, make sure that an illegal value is not marked
		PackApp *app = dynamic_cast<PackApp *>(be_app);
		if (app && app->CompatibleMode()) {
			if (!IsCompatibleReplacementOption(1, kReplaceOptions[index].code)) {
				clearReplaceMenu = true;
			}
		}
	}

	if (clearReplaceMenu) {
		long count = replPopup->CountItems();
		for (long i = 0; i < count; i++)
			replPopup->ItemAt(i)->SetMarked(FALSE);
		replMenu->MenuBar()->ItemAt(0)->SetLabel(B_EMPTY_STRING);
	} else {
		replPopup->ItemAt(index)->SetMarked(TRUE);
	}

	replMenu->SetEnabled(!msg->FindBool("script"));
	
	bool folderSel = msg->FindBool("folder");
	// enable or disable the "merge folders" option depending on whether or not
	// the item is a folder
	BMenuItem *mergeItem = replPopup->ItemAt(mergeOptionIndex);
	if (mergeItem != NULL) {
		mergeItem->SetEnabled(folderSel);
	}

	// support multiple platforms per file?? Yes!! need to implement
	{
		int32 arch = msg->FindInt32("platform");
		const char *archText = B_EMPTY_STRING;
		if (arch == 0xFFFFFFFF) {
			archText = "Mixed";
			archPopup->ItemAt(archPopup->CountItems() - 1)->SetMarked(true);	
		} else {
			for(int i = 0; arch; i++) {
				if (arch & 1) {
					archText = archPopup->ItemAt(i)->Label();
					archPopup->ItemAt(i)->SetMarked(true);
				}
				arch = arch >> 1;
			}
		}
		((BStringView *)FindView("platform"))->SetText(archText);
	}
	//else {
	//	archText = B_EMPTY_STRING;
	//	BMenuItem *it = archPopup->FindMarked();
	//	if (it) it->SetMarked(false);
	//}
	
	
	
	
#if 0	// support multiple archs per file?? Yes!! need to implement
	char *dst = nam;
	limit = dst + GR_LEN;

	count = archPopup->CountItems();
	for (int i = 0; i < count && i < 32; i++) {
		if ((1 << i) & archs) {
			const char *src = archPopup->ItemAt(i)->Label();
			while(*src && dst < limit)
				*dst++ = *src++;
			if (dst < limit-2) {
				strcpy(dst,", ");
				dst += 2;
			}
			else {
				break;
			}
			archPopup->ItemAt(i)->SetMarked(true);
		}
		else {
			archPopup->ItemAt(i)->SetMarked(false);
		}
	}
	if (dst > nam+1) {
		// some chars were copied, including comma and space
		// null out the last value
		*(dst-2) = '\0';
	}
	((BStringView *)FindView("platform"))->SetText(nam);
#endif	
	////////////
	
	((BStringView *)FindView("deps"))->SetText(msg->FindString("depends"));
}

#if CONDITIONAL_INSTALL
void SettingsView::ConditionSet(bool state)
{
	BMessage	cond(M_COND_SELECTED);
	BMenuField *f;
		
	if (state) {
		f = (BMenuField *)FindView("ififnot");
		BMenu *men = f->Menu();
		cond.AddInt32("conditional", men->IndexOf(men->FindMarked()));
		f = (BMenuField *)FindView("conditions");
		men = f->Menu();
		cond.AddInt32("condition",men->IndexOf(men->FindMarked()));
	}
	else {
		cond.AddInt32("conditional",-1);
		cond.AddInt32("condition",0);
	}
#if DEBUG
	cond.PrintToStream();
#endif

	Looper()->PostMessage(&cond,Window()->FindView("listing"));
}
#endif

void SettingsView::EnableDisableReplacementOptions(uint16 version)
{
	BMenuItem *replItem;
	int32 i = 0;
	while ((replItem = replPopup->ItemAt(i++)) != NULL) {
		BMessage *msg = replItem->Message();
		int32 code = (msg && msg->HasInt32("repl")) ? msg->FindInt32("repl") : -1;
		bool isCompatible = IsCompatibleReplacementOption(version, code);
		// don't touch the merge folder option, because it is enable/disabled
		// elsewhere, and it is always compatible 
		if (code != R_MERGE_FOLDER) {
			replItem->SetEnabled(isCompatible);
		}
	}
}


void SettingsView::MessageReceived(BMessage *msg)
{
#if DEBUG
	msg->PrintToStream();
#endif

	PackWindow *wind = (PackWindow *)Window();
	GroupList *groupList;
	int32 index;
	
	switch(msg->what) {
#if CONDITIONAL_INSTALL
		case M_INSTALL_COND: {
			BControl *p;
			bool	value;
			if (msg->FindPointer("source",(void **)&p))
				break;
			value = p->Value();
			
			BMenuField *f;
			f = (BMenuField *)FindView("ififnot");
			f->SetEnabled(value);
			f = (BMenuField *)FindView("conditions");
			f->SetEnabled(value);
			
			ConditionSet(value);
			break;
		}
		case M_INSTALL_IF: {
			ConditionSet(true);
			break;
		}
		case M_COND_SELECTED: {
			ConditionSet(true);			
			break;
		}
		case M_NEW_COND: {
			// update the menu with the new name
			break;
		}
		case M_REMOVE_COND: {
			// remove the index from the menu
			
			// reset deleted condition
			// Looper()->PostMessage(condDel,wind->FindView("listing"));
			
			break;
		}
#endif
		case M_ARCH_SELECTED: {
			if (msg->FindInt32("index",&index) != B_NO_ERROR)
				break;
			
			int32 arch;
			if (index == archPopup->CountItems()-1) {
				arch = 0xFFFFFFFF;
				((BStringView *)FindView("platform"))->SetText("Mixed");
			}
			else {
				arch = 1 << index;
				((BStringView *)FindView("platform"))->SetText(
					archPopup->ItemAt(index)->Label());
			}
			msg->AddInt32("platform",arch);
			Looper()->PostMessage(msg,wind->FindView("listing"));
			break;
		}
		case M_DEST_SELECTED: {
			if (msg->FindInt32("index",&index) == B_NO_ERROR) {
				BMenuItem *item = destPopup->ItemAt(index);
				destPopup->ItemAt(lastMarked)->SetMarked(FALSE);
				item->SetMarked(TRUE);
				lastMarked = index;
				destMenu->MenuBar()->ItemAt(0)->SetLabel(item->Label());
			
				// apply to selected files
				Looper()->DetachCurrentMessage();
				
				if (msg->FindBool("custom")) {
					if (msg->HasInt32("dest")) {
						doError("Custom destination shouldn't have index!");
					}
					
					msg->AddInt32("dest",index - custStartIndex);
				}
				Looper()->PostMessage(msg,wind->FindView("listing"));
			}
			
			break;
		}
		case M_NEW_DEST: {
			// a new desintation was created
			wind->attribDirty = TRUE;
			
			const char *pathN;
			long count;
			ulong type;
			
			msg->GetInfo("text",&type,&count);
			if (count > 0) {
				// the text either came from the edit window or a dialog
				while(count--) {
					BMessage *destMsg = new BMessage(M_DEST_SELECTED);
					
					if (destCount == 0) {
						PRINT(("addING SEPARATOR ITEM\n"));
						// add extra separator if this is the first one
						long loc = destPopup->CountItems()-3;
						destPopup->AddItem(new BSeparatorItem(),loc);
					}
					// destMsg->AddInt32("dest",destCount++);
					destCount++;
					destMsg->AddBool("custom",TRUE);
					pathN = msg->FindString("text");
					BMenuItem *mitem = new BMenuItem(pathN,destMsg);
					mitem->SetTarget(this);
					mitem->SetMarked(FALSE);
					long loc = destPopup->CountItems()-3;
					
					destPopup->AddItem(mitem,loc);
				}				
				if (msg->HasBool("fromdialog")) {
					// send it on to the edit window (if it exists)
					PRINT(("SENDING IT ONTO THE WINDOW\n"));
			
					// race condition here where we could miss adding items
					// to the list
					// add item manually
					const char *newPath = msg->FindString("text");
					wind->attrib.customDestList->AddItem(new DestItem(newPath));
					
					if ( wind->childWindows[PackWindow::kDestWind].IsValid()) {
						// Looper()->DetachCurrentMessage();
						// window suddenly becomes invalid -- item isn't added
						// we should get a reply???
						wind->childWindows[PackWindow::kDestWind].SendMessage(M_ITEMS_UPDATED);
					}

					
					// if this failed we need to add it to the viewlist manually!!
			
				}
			}
			else {
				BMessage *ndMsg = new BMessage(M_NEW_DEST);
				ndMsg->AddBool("fromdialog",TRUE);
				NameDialog *nd = new NameDialog(BRect(0,0,280,80),"Pathname:",
					"newpathname",ndMsg,this);
			}
			break;
		}
		case M_NEW_FINDITEM: {
			wind->attribDirty = TRUE;
			
			BMessage *destMsg = new BMessage(M_DEST_SELECTED);
			// add item to the menu			
			if (destCount == 0) {
				long loc = destPopup->CountItems()-3;
				destPopup->AddItem(new BSeparatorItem(),loc);
			}
			// destMsg->AddInt32("dest",destCount++);
			destCount++;
			destMsg->AddBool("custom",TRUE);
			const char *queryName = msg->FindString("text");
			BMenuItem *mitem = new BMenuItem(queryName,destMsg);
			mitem->SetTarget(this);
			mitem->SetMarked(FALSE);
			long loc = destPopup->CountItems()-3;
			
			destPopup->AddItem(mitem,loc);
			
			
			if (msg->HasBool("fromapp")) {
				// the real item doesn exist (ie wasn't created in the window)
				DestList	*destList = wind->attrib.customDestList;
				
				const char *sig = msg->FindString("signature");
				long size = msg->FindInt32("size");
			
				PRINT(("NEW signature %s  size %d\n",sig,size));
				
				destList->lock.Lock();
				destList->AddItem(new FindItem(queryName,size,sig));
				
				if (msg->HasPointer("arcitem")) {
					ArchiveItem *it;
					msg->FindPointer("arcitem",reinterpret_cast<void **>(&it));
					it->destination = destList->CountItems()-1;
					it->customDest = TRUE;
				}
				destList->lock.Unlock();
				
				// send an update message to the window so it redraws its list
				
				if (wind->childWindows[PackWindow::kDestWind].IsValid())
					wind->childWindows[PackWindow::kDestWind].SendMessage(M_ITEMS_UPDATED);
			}	
			break;
		}
		case M_EDIT_DEST:
			// bring up the edit window
			if (! wind->childWindows[PackWindow::kDestWind].IsValid() ) {
				PRINT(("making dest window\n"));
				
				DestinationWindow *destWindow =
					new DestinationWindow("Edit Destinations",wind->attrib.customDestList,wind);
				wind->childWindows[PackWindow::kDestWind] = BMessenger(destWindow);
				destWindow->settingsViewMessenger = BMessenger(this);
			}
			else {
				// a small hack to do activate
				wind->childWindows[PackWindow::kDestWind].SendMessage(M_DO_ACTIVATE);
			}
			break;
		case M_NAME_DEST: {
			wind->attribDirty = TRUE;
			
			index = msg->FindInt32("index");
			index += custStartIndex;
			destPopup->ItemAt(index)->SetLabel(msg->FindString("destname"));
			break;
		}
		case M_REMOVE_DEST: {
			wind->attribDirty = TRUE;
			// remove a custom destination from the menu
			// reset all files with references to this destination
		
			// handle multiple removals eventually
			long oldIndex = msg->FindInt32("index");
			
			PRINT(("rEMOVING index is %d\n",oldIndex));
			
			long itemIndex = oldIndex + custStartIndex;
			
			destPopup->RemoveItem(itemIndex);
			destCount--;
			if (destCount == 0) {
				// get rid of extra separator
				destPopup->RemoveItem(itemIndex);
			}
			
			PackList *plv = (PackList *)(wind->FindView("listing"));
			plv->treeLock.Lock();
			plv->toplevel->DestDeleted(oldIndex);
			plv->treeLock.Unlock();
			plv->SelectionSet();
			break;
		}
		case M_REORDER_DEST: {
			wind->attribDirty = TRUE;
			
			long oldI = msg->FindInt32("oldindex");
			long newI = msg->FindInt32("newindex");
			if (oldI == newI)
				break;
	
			BMenuItem *mi = destPopup->RemoveItem(oldI + custStartIndex);
			// separator line is between indexes so if we are moving an item
			// UP in the list, the final new index will be one greater than
			// that reported by the list widget
			if (newI < oldI)
				newI++;
			destPopup->AddItem(mi,newI + custStartIndex);
			
			PackList *plv = (PackList *)(wind->FindView("listing"));
			plv->treeLock.Lock();
			plv->toplevel->DestMoved(oldI,newI);
			plv->treeLock.Unlock();
			plv->SelectionSet();
			break;
		}
		case M_NEW_GROUP: {
			PRINT(("got new groups message\n"));
			
			ulong type;
			long  count;
			// do checking for too many groups
			wind->attribDirty = TRUE;
			
			const char *gName;
			
			msg->GetInfo("text",&type,&count);
			if (count > 0) {
				// the text either came from the edit window or a dialog
				groupList = wind->attrib.groupList;
				while(count--) {
					if (groupList->masterList->CountItems() >= 32) {
						doError("Sorry, no more groups can be added");
						break;
					}
					gName = msg->FindString("text",count);
					BMenuItem *item = new BMenuItem(gName,new BMessage(M_GROUP_SELECTED));
					item->SetTarget(this);
					item->SetMarked(FALSE);
					groupsPopup->AddItem(item,groupsPopup->CountItems() - 3);
					if (msg->HasBool("fromdialog")) {	
						groupList->AddGroup(gName);
					}
				}
				if (msg->HasBool("fromdialog")) {
					// send it on to the edit window (if it exists)
					PRINT(("SENDING IT ONTO THE WINDOW\n"));
					if (wind->childWindows[PackWindow::kGroupsWind].IsValid()) {
						Looper()->DetachCurrentMessage();
						wind->childWindows[PackWindow::kGroupsWind].SendMessage(msg);
					}
				}
			}
			else {
				BMessage *ndMsg = new BMessage(M_NEW_GROUP);
				ndMsg->AddBool("fromdialog",TRUE);
				NameDialog *nd = new NameDialog(BRect(0,0,280,80),"Group Name:",
					"New Group",ndMsg,this);
			}
			break;
		}
		case M_NEW_SEPARATOR: {
			wind->attribDirty = TRUE;
			
			groupsPopup->AddItem(new BSeparatorItem(),groupsPopup->CountItems() - 3);
			break;
		}
		case M_NAME_GROUP: {
			wind->attribDirty = TRUE;
			
			long vIndex = msg->FindInt32("viewindex");
			groupsPopup->ItemAt(vIndex)->SetLabel(msg->FindString("groupname"));
			break;
		}
		case M_REORDER_GROUP: {
			wind->attribDirty = TRUE;
			
			int32 oldI = msg->FindInt32("oldindex");
			int32 newI = msg->FindInt32("newindex");
			if (oldI == newI)
				break;
	
			BMenuItem *mi = groupsPopup->RemoveItem(oldI);
			if (newI < oldI)
				newI++;
			groupsPopup->AddItem(mi,newI);
			
			break;
		}
		case M_EDIT_GROUPS: {
			if (! (wind->childWindows[PackWindow::kGroupsWind].IsValid()) ) {
				PRINT(("making groups window\n"));
				
				GroupsWindow *groupsWindow =
					new GroupsWindow("Edit Groups",wind->attrib.groupList,wind);
				wind->childWindows[PackWindow::kGroupsWind] = BMessenger(groupsWindow);
				groupsWindow->settingsViewMessenger = BMessenger(this);
			}
			else {
				// a small hack to do activate
				wind->childWindows[PackWindow::kGroupsWind].SendMessage(M_DO_ACTIVATE);
			}
			break;
		}
		case M_GROUP_SELECTED: {
			// item chosen from menu
			BMessage *cur = Looper()->CurrentMessage();
			index = cur->FindInt32("index");
			
			// check whether selected or deselected
			
			BMenuItem *item = groupsPopup->ItemAt(index);
			
			bool marked = item->IsMarked();
			
			item->SetMarked(!marked);
			
			PRINT(("group selected: menuindex %d itemmarked %d was %d\n",index,item->IsMarked(),marked));
			
			// transform to master group list
				// currently groups can't be removed, removal will require fixing all bitmaps
			groupList = wind->attrib.groupList;

			// CRASH here when group is created from popup
			index = groupList->viewList->ItemAt(index)->index;
			
			// compute bitmap
			ulong bitmap = 1 << index;
			
			// apply to selected files
			BMessage grpSel(M_GROUP_SELECTED);
			grpSel.AddBool("mark",item->IsMarked());
			grpSel.AddInt32("bitmap",bitmap);
			
			Looper()->PostMessage(&grpSel,wind->FindView("listing"));
			
			// need to update displayed string!
			// best to compute from current mask
			BMessage upMsg(M_ITEMS_SELECTED);
			if (!item->IsMarked()) {
				bitmap = ~bitmap;
				upMsg.AddInt32("bitmap",currentBitmap & bitmap);
			}
			else {
				upMsg.AddInt32("bitmap",currentBitmap | bitmap);
			}
			
			Looper()->PostMessage(&upMsg,this);
					
			break;
		}
		case M_REMOVE_GROUP: {
			wind->attribDirty = TRUE;
			
			long viewInd = msg->FindInt32("viewindex");
			long mastInd = msg->FindInt32("masterindex");
			ulong lowMask,highMask;
			
			PRINT(("popup menu request to remove item %d\n",viewInd));
			
			groupsPopup->RemoveItem(viewInd);
			if (mastInd != -1) {
				// fix up the bitmaps for ALL archive items
				
				lowMask = 1;
				for (long i = 0; i < mastInd; i++) {
					lowMask = lowMask << 1;
					lowMask += 1;
				}
				highMask = ~lowMask;
				lowMask = lowMask >> 1; // we dont wan't the deleted item
				
				PackList *plv = (PackList *)wind->FindView("listing");
				
				plv->toplevel->GroupDeleted(lowMask,highMask);
				
				// be sure to update names displayed for the selected groups
				/****
					group = (group & lowMask) | ((group & highMask) >> 1)
				****/
				
			}
			break;
		}
		case M_ITEMS_SELECTED: {
			ItemsSelected(msg);
			break;
		}
		case M_CHANGE_FOLDER_LEVEL: {
			long level = msg->FindInt32("index");
			
			// PRINT(("SETTINGS got folder level %d\n",level));
			if (level == 0) {
				destPopup->ItemAt(1)->SetEnabled(FALSE);
			}
			else {
				destPopup->ItemAt(1)->SetEnabled(TRUE);
			}
			break;
		}
		case M_REPL_SELECTED: {
			Looper()->DetachCurrentMessage();
			Looper()->PostMessage(msg,Window()->FindView("listing"));
			break;
		}
/******
		case M_AUTO_SET:
			BCheckBox *cb = (BCheckBox *)FindView("autocompress");
			PRINT(("check box value %d\n",cb->Value()));
			wind->autoCompress = cb->Value();
			break;
*******/
		default:
			break;
	}

}
