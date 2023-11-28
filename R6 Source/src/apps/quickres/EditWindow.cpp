#include "EditWindow.h"

#include <ResourceParser.h>
#include <ResourceEntry.h>
#include <UndoContext.h>

#include <MessageRunner.h>
#include <Alert.h>
#include <MenuItem.h>
#include <MenuBar.h>
#include <Screen.h>
#include <ScrollView.h>
#include <Autolock.h>
#include <Debug.h>
#include <String.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

// ---------------------------- EditWindowController ----------------------------

EditWindowController::EditWindowController(BWindow& window, BResourceRoster& roster,
										   BResourceAddonArgs args)
	: BResourceAddonBase(args),
	  fWindow(window), fRoster(roster), fShowAllUndos(false),
	  fContainer(0), fMenuBar(0), fEditMenu(0), fCustomPosition(0),
	  fEditAddon(0), fFullEditor(0),
	  fEditorShow(0), fEditorView(0)
{
}

void EditWindowController::SetContainer(BView* container)
{
	fContainer = container;
}

void EditWindowController::SetMenus(BMenuBar* bar, BMenu* editMenu,
								BMenu* customPosition)
{
	fMenuBar = bar;
	fEditMenu = editMenu;
	fCustomPosition = customPosition;
}

void EditWindowController::SetShowAllUndos(bool state)
{
	fShowAllUndos = state;
}

bool EditWindowController::ShowAllUndos() const
{
	return fShowAllUndos;
}

BList* EditWindowController::GetContext(const BResourceCollection* collection,
										BList* context)
{
	if( ShowAllUndos() ) return 0;
	
	if( fFullEditor ) {
		BResourceHandle h;
		for( int32 i=0;
				collection->GetSubscriptionAt(&h, fFullEditor, i) == B_OK;
				i++ ) {
			const BResourceItem* it = collection->ReadItem(h);
			if( it ) {
				const ResourceEntry* e = dynamic_cast<const ResourceEntry*>(it);
				if( e && e->Owner() ) {
					context->AddItem(const_cast<ResourceEntry*>(e));
				}
			}
		}
	}
	
	return context;
}

status_t EditWindowController::ExecMessageReceived(BMessage *msg)
{
	if (msg->WasDropped()) {
		BResourceCollection* c = WriteLock("Drop");
		if( !c ) return B_ERROR;
		fRoster.MessageDrop(*c, msg);
		WriteUnlock(c);
		return B_OK;
	}
	
	switch(msg->what)
	{
		case EDITWIN_ACTIVATE:
		{
			fWindow.Activate();
		} break;
		
		case EDITWIN_APPLYCHANGES:
		{
			BMessage reply(B_REPLY);
			ApplyChanges(&reply);
		} break;
		
		case B_UNDO:
		{
			PerformUndo();
		} break;
		
		case B_REDO:
		{
			PerformRedo();
		} break;

		case 'edsh' :
		{
			uint32 flags;
			if( msg->FindInt32("flags", (int32*)&flags) != B_OK ) {
				flags = 0;
			}
			ShowEditor(flags);
		} break;
		
		default :
			return B_ERROR;
	}
	
	return B_OK;
}

void EditWindowController::UpdateUndoMenus(BMenuItem* undo, BMenuItem* redo)
{
	const BResourceCollection* c = ReadLock();
	if( !c ) return;
	
	BList local_context;
	BList* show_context = GetContext(c, &local_context);
	
	const BUndoContext* context = c->UndoContext();
	
	if( undo ) {
		BString undoName("Undo");
		if( context && context->CountUndos(show_context) > 0 ) {
			undo->SetEnabled(true);
			const char* nm = context->UndoName(show_context);
			if( nm && *nm ) {
				undoName << " " << nm;
			}
		} else {
			undo->SetEnabled(false);
		}
		undo->SetLabel(undoName.String());
	}
	if( redo ) {
		BString redoName("Redo");
		if( context && context->CountRedos(show_context) > 0 ) {
			redo->SetEnabled(true);
			const char* nm = context->RedoName(show_context);
			if( nm && *nm ) {
				redoName << " " << nm;
			}
		} else {
			redo->SetEnabled(false);
		}
		redo->SetLabel(redoName.String());
	}
	
	ReadUnlock(c);
}

void EditWindowController::PerformUndo()
{
	BResourceCollection* c = WriteLock();
	if( !c ) return;
	
	BList local_context;
	BList* show_context = GetContext(c, &local_context);
	
	BUndoContext* context = c->UndoContext();
	
	if( context ) context->Undo(show_context);
	
	WriteUnlock(c);
}

void EditWindowController::PerformRedo()
{
	BResourceCollection* c = WriteLock();
	if( !c ) return;
	
	BList local_context;
	BList* show_context = GetContext(c, &local_context);
	
	BUndoContext* context = c->UndoContext();
	
	if( context ) context->Redo(show_context);
	
	WriteUnlock(c);
}

bool EditWindowController::ApplyChanges(BMessage* reply)
{
	bool hasChanges = false;
	if( fFullEditor ) {
		hasChanges = fFullEditor->HasOpenChanges();
		if( hasChanges ) {
			hasChanges = fFullEditor->ApplyChanges();
			if( reply ) reply->AddBool("be:changed", hasChanges);
		}
	}
	return hasChanges;
}

void EditWindowController::ForgetEditor()
{
	// save away current configuration.
	if( fEditAddon && fFullEditor && fEditName.Length() > 0 ) {
		BMessage config;
		status_t err = fFullEditor->GetConfiguration(&config);
		if( err == B_OK && !config.IsEmpty() ) {
			fRoster.UpdateConfiguration(fEditAddon, &config);
		}
	}
	
	DeleteMenus();
	
	// flush pane
	if( fEditorView && fContainer ) fContainer->RemoveChild(fEditorView);
	fEditorView = 0;
	if( fFullEditor ) {
		delete fFullEditor;
		fFullEditor = 0;
	}
	
	fEditAddon = 0;
	fEditName = "";
	
	delete fEditorShow;
	fEditorShow = 0;
}

void EditWindowController::UpdateEditor(BResourceHandle item, uint32 flags)
{
	if( !fContainer || !fContainer->Parent() ) {
		ForgetEditor();
		return;
	}
	
	if( fEditItem == item && item.IsValid() ) {
		const BResourceCollection* c = ReadLock();
		if( c ) {
			BString name;
			const BResourceItem* it = c->ReadItem(item);
			BResourceAddon* addon = fRoster.AddonForResource(it, &name);
			ReadUnlock(c);
			if( addon == fEditAddon ) return;
		}
	}
	
	if( item.IsValid() && fEditAddon && fFullEditor ) {
		BResourceAddon* addon = 0;
		const BResourceCollection* c = ReadLock();
		if( c ) {
			BString name;
			const BResourceItem* it = c->ReadItem(item);
			addon = fRoster.AddonForResource(it, &name);
			ReadUnlock(c);
		}
		if( addon == fEditAddon && fFullEditor->Retarget(item) == B_OK ) {
			fEditItem = item;
			return;
		}
	}
	
	ForgetEditor();
	
	fEditItem = item;
	if( fEditItem.IsValid() ) {
		const BResourceCollection* c = ReadLock();
		if( !c ) return;
		
		// find best editor for the resource
		const BResourceItem* it = c->ReadItem(item);
		BString name;
		fEditAddon = fRoster.AddonForResource(it, &name);
		
		ReadUnlock(c);
		
		if( fEditAddon ) {
			BMessage* configuration = 0;
			BMessage msg;
			if( fEditName.Length() > 0 ) {
				if( fRoster.GetConfiguration(fEditAddon, &msg) == B_OK ) {
					configuration = &msg;
				}
			}
			fFullEditor = fEditAddon->MakeFullEditor(BResourceAddonArgs(*this),
													 fEditItem, configuration);
		}
		
		delete fEditorShow;
		fEditorShow = 0;
		if( flags&T_UPDATE_NOW ) ShowEditor(flags);
		else {
			bigtime_t speed;
			get_key_repeat_delay(&speed);
			BMessage msg('edsh');
			msg.AddInt32("flags", flags);
			fEditorShow = new BMessageRunner(BMessenger(&fWindow), &msg, speed);
		}
	}
}

void EditWindowController::ShowEditor(uint32 flags)
{
	delete fEditorShow;
	fEditorShow = 0;
	
	if( !fContainer || !fContainer->Parent() ) return;
	
	fWindow.DisableUpdates();
	fWindow.BeginViewTransaction();
	
	if( fFullEditor && !fEditorView ) {
		// Watch out!  The act of creating the view may change data,
		// causing the editor to change.  If this happens, just bail.
		BFullItemEditor* myEditor = fFullEditor;
		fEditorView = fFullEditor->View();
		if( fEditorView != 0 && myEditor == fFullEditor ) {
			fEditorView->Hide();
			fEditorView->MoveTo(fWindow.Bounds().Width()+1000, 0);
			fContainer->AddChild(fEditorView);
			fEditorView->ResizeTo(fContainer->Bounds().Width()+1,
								  fContainer->Bounds().Height()+1);
			fEditorView->SetResizingMode(B_FOLLOW_ALL);
			if( flags&T_UPDATE_FOCUS ) fEditorView->MakeFocus();
		} else {
			fEditorView = 0;
		}
	}
		
	if( fEditorView ) {
		DeleteMenus();
		
		if( fFullEditor && fMenuBar ) {
			int32 pos = fCustomPosition
					  ? fMenuBar->IndexOf(fCustomPosition)
					  : 0;
			BMenu* menu;
			for( int32 i=0; (menu=fFullEditor->CustomMenu(i)) != 0; i++ ) {
				fEditorMenus.AddItem(menu);
				if( pos ) {
					fMenuBar->AddItem(menu, pos++);
				} else {
					fMenuBar->AddItem(menu);
				}
			}
		}
		
		fEditorView->Show();
		fEditorView->MoveTo(0, 0);
	
		float pw = 0, ph = 0;
		fEditorView->GetPreferredSize(&pw, &ph);
		float dw = pw - fContainer->Bounds().Width()+1;
		float dh = ph - fContainer->Bounds().Height()+1;
		if( dw > 0 || dh > 0 ) {
			if( dw < 0 ) dw = 0;
			if( dh < 0 ) dh = 0;
			fWindow.ResizeBy(dw, dh);
			
			BRect wf(fWindow.Frame());
			float x = wf.left;
			float y = wf.top;
			BRect sf(BScreen(&fWindow).Frame());
			if( (x+wf.Width()) > sf.right ) x = sf.right-wf.Width();
			if( (y+wf.Height()) > sf.bottom ) y = sf.bottom-wf.Height();
			if( x < sf.left ) x = sf.left;
			if( y-20 < sf.top ) y = sf.top+20;
			
			fWindow.MoveTo(x, y);
		}
	}
	
	fWindow.EndViewTransaction();
	fWindow.EnableUpdates();
}

void EditWindowController::DeleteMenus()
{
	BMenuItem* item;
	while( (item=(BMenuItem*)fEditorMenuItems.RemoveItem(
								fEditorMenuItems.CountItems()-1)) != 0 ) {
		if( fEditMenu ) fEditMenu->RemoveItem(item);
	}
	
	BMenu* menu;
	while( (menu=(BMenu*)fEditorMenus.RemoveItem(
							fEditorMenus.CountItems()-1)) != 0 ) {
		if( fMenuBar ) {
			int32 idx = fMenuBar->IndexOf(menu);
			fMenuBar->RemoveItem(idx);
		}
	}
}

// ---------------------------- EditWindow ----------------------------

EditWindow::EditWindow(const BResourceAddonArgs& args, BResourceHandle handle,
			   BResourceRoster& roster,
			   BPoint center, const char* doc_name,
			   window_look look, window_feel feel,
			   uint32 flags, uint32 workspace)
	: BWindow(BRect(center.x-10, center.y-10, center.x+10, center.y+10),
			  doc_name, look, feel, flags, workspace),
	  BResourceAddonBase(args), fController(*this, roster, args),
	  fEditItem(handle), fDocName(doc_name),
	  fContainer(0),
	  fUndoItem(0), fRedoItem(0),
	  fCutItem(0), fCopyItem(0), fPasteItem(0), fClearItem(0)
{
	BResourceCollection* c = WriteLock();
	if( c ) {
		c->Subscribe(fEditItem, this);
		WriteUnlock(c);
	}
	
	UpdateTitle();
	
	// create the menu bar
	BMenuBar	*menubar = new BMenuBar(BRect(0, 0, 0, 0), "menu");

	// File menu
	BMenu *file = new BMenu("Window");
	menubar->AddItem(new BMenuItem(file));

	// close menu item
	BMenuItem *close = new BMenuItem("Close", new BMessage(B_QUIT_REQUESTED), 'W');
	close->SetTarget(this);
	file->AddItem(close);
	//file->AddItem(new BSeparatorItem());

	// Edit menu
	BMenu *edit = new BMenu("Edit");
	menubar->AddItem(new BMenuItem(edit));

	// undo menu item
	fUndoItem = new BMenuItem("Undo", new BMessage(B_UNDO), 'Z');
	fUndoItem->SetTarget(this);
	edit->AddItem(fUndoItem);

	// redo menu item
	fRedoItem = new BMenuItem("Redo", new BMessage(B_REDO), 'Z', B_OPTION_KEY);
	fRedoItem->SetTarget(this);
	edit->AddItem(fRedoItem);

	edit->AddSeparatorItem();
	
	// cut menu item
	fCutItem = new BMenuItem("Cut", new BMessage(B_CUT), 'X');
	fCutItem->SetTarget(0, this);
	edit->AddItem(fCutItem);

	// copy menu item
	fCopyItem = new BMenuItem("Copy", new BMessage(B_COPY), 'C');
	fCopyItem->SetTarget(0, this);
	edit->AddItem(fCopyItem);

	// paste menu item
	fPasteItem = new BMenuItem("Paste", new BMessage(B_PASTE), 'V');
	fPasteItem->SetTarget(0, this);
	edit->AddItem(fPasteItem);

	edit->AddSeparatorItem();
	
	// clear menu item
	fClearItem = new BMenuItem("Clear", new BMessage(B_CLEAR));
	fClearItem->SetTarget(0, this);
	edit->AddItem(fClearItem);

	// select menu item
	BMenuItem* item = new BMenuItem("Select All", new BMessage(B_SELECT_ALL), 'A');
	item->SetTarget(0, this);
	edit->AddItem(item);

	// deselect menu item
	item = new BMenuItem("Deselect", new BMessage(B_DESELECT_ALL));
	item->SetTarget(0, this);
	edit->AddItem(item);

	// add menu bar to window
	AddChild(menubar);
	BRect area = Bounds();
	area.top = menubar->Bounds().Height() + 1;

	fController.SetMenus(menubar, edit, 0);
	
	// Add root view
	BView* root = new BView(area, "root", B_FOLLOW_ALL, 0);
	root->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(root);

	const int SPACER = 0;
	
	BRect r = area;
	r.OffsetTo(0, 0);
	r.InsetBy(SPACER, SPACER);

	// The container view.
	fContainer = new BView(r, "edit_container", B_FOLLOW_ALL, 0);
	fContainer->SetViewColor(root->ViewColor());
	root->AddChild(fContainer);
	
	fController.SetContainer(fContainer);
	
	fController.UpdateEditor(fEditItem,
							 fController.T_UPDATE_NOW|fController.T_UPDATE_FOCUS);
	
	BRect bnd(Bounds());
	float x = center.x - (bnd.Width()/2);
	float y = center.y - (bnd.Height()/2);
	BRect frm(BScreen(this).Frame());
	if( (x+bnd.Width()) > frm.right ) x = frm.right-bnd.Width();
	if( (y+bnd.Height()) > frm.bottom ) y = frm.bottom-bnd.Height();
	if( x < frm.left ) x = frm.left;
	if( y-20 < frm.top ) y = frm.top+20;
	
	MoveTo(x, y);
	
	Lock();
	AddCommonFilter(this);
	Unlock();
}

bool EditWindow::QuitRequested()
{
	fController.ForgetEditor();
	return true;
}

void EditWindow::Quit()
{
	fController.ForgetEditor();
	RemoveCommonFilter(this);
	BWindow::Quit();
}

void EditWindow::MenusBeginning()
{
	fController.UpdateUndoMenus(fUndoItem, fRedoItem);
}

void EditWindow::MessageReceived(BMessage *msg)
{
	if( fController.ExecMessageReceived(msg) == B_OK ) return;
	
	switch(msg->what)
	{
		default :
			BWindow::MessageReceived(msg);
			break;
	}
}

void EditWindow::DataChanged(BResourceHandle& item)
{
	bool type_changed = false;
	bool id_changed = false;
	
	if( item == fEditItem ) {
		const BResourceCollection* c = ReadLock();
		if( c ) {
			const BResourceItem* it = c->ReadItem(item);
			type_changed = ( it && (it->Changes()&B_RES_TYPE_CHANGED) != 0 ) ;
			id_changed = ( it && (it->Changes()&B_RES_ID_CHANGED) != 0 ) ;
			ReadUnlock(c);
		}
	}
	
	if( type_changed ) {
		fController.UpdateEditor(fEditItem,
								 fController.T_UPDATE_NOW|fController.T_UPDATE_FOCUS);
	}
	if( type_changed || id_changed ) {
		UpdateTitle();
	}
}

void EditWindow::UpdateTitle()
{
	const BResourceCollection* c = ReadLock();
	if( c ) {
		const BResourceItem* it = c->ReadItem(fEditItem);
		BString title(fDocName);
		BString buf;
		title << ": " << BResourceParser::TypeIDToString(it->Type(),
														 it->ID(), &buf);
		SetTitle(title.String());
		ReadUnlock(c);
	}
}
