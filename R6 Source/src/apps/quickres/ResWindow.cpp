#include "ResWindow.h"
#include "ResSupport.h"
#include "QuickRes.h"
#include "SavePanel.h"
#include "AddWindow.h"

#include <ResourceParser.h>
#include <ResourceEntry.h>

#include <experimental/Order.h>
#include <experimental/DividerControl.h>
#include <controls/ColumnListView.h>
#include <controls/ColumnTypes.h>

#include <Clipboard.h>
#include <MessageRunner.h>

#include <fs_attr.h>

#include <Alert.h>
#include <Box.h>
#include <Button.h>
#include <ListView.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Screen.h>
#include <ScrollView.h>
#include <TextControl.h>

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <NodeInfo.h>
#include <Resources.h>

#include <Autolock.h>
#include <Debug.h>
#include <String.h>

#include <RecentItems.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

enum {
	kInplaceEditor = 'ined',
	kWindowEditor = 'wned',
	kShowEditor = 'shed',
	kCollectionChanged = 'clch'
};

#define SETTING_FRAME			"be:frame"
#define SETTING_SHOWEDITOR		"be:showeditor"
#define SETTING_EDITORHEIGHT	"be:editheight"
#define SETTING_LISTCONFIG		"be:listconfig"

// ---------------------------- ResWindow ----------------------------

enum {
	SOURCE_FORMAT = 0,
	BINARY_FORMAT = 1,
	ATTRIBUTE_FORMAT = 2
};

static const SavePanel::format_def save_formats[] = {
	{ "Source (.rdef)", "text/x-vnd.Be.ResourceDef", SOURCE_FORMAT },
	{ "Binary (.rsrc)", "application/x-be-resource", BINARY_FORMAT },
	{ "Attributes Only", "", ATTRIBUTE_FORMAT },
	{ 0, 0, 0 }
};

enum {
	SPACER = 4
};

ResWindow::ResWindow(WindowRoster *wr, entry_ref *ref, const char *title,
					 window_look /*look*/, window_feel feel,
					 uint32 flags, uint32 workspace)
	: DocWindow(wr, ref, BRect(0, 0, 520, 300),
				title, B_DOCUMENT_WINDOW_LOOK, feel, flags, workspace),
	  fAddons(*this), fReadLocks(0),
	  fWriteEnding(false), fChangedSelection(false),
	  fController(*this, fAddons, BResourceAddonArgs(*this)),
	  fResList(0), fColumns(0), fDivider(0),
	  fRoot(0), fContainer(0),
	  fAddField(0), fShowField(0), fRemoveButton(0), fUndoItem(0), fRedoItem(0),
	  fCutItem(0), fCopyItem(0), fPasteItem(0), fClearItem(0),
	  fAddItem(0), fDupItem(0), fEditInplaceItem(0), fEditWindowItem(0),
	  number(0), fSaveFormat(BINARY_FORMAT), fAttributeSettings(false),
	  fBodyChanged(false),
	  fMergePanel(0)
{
	fController.SetShowAllUndos();
	
	LoadSettings();
	
	// create the menu bar
	BMenuBar	*menubar = new BMenuBar(BRect(0, 0, 0, 0), "menu");

	// File menu
	BMenu *file = new BMenu("File");
	menubar->AddItem(new BMenuItem(file));

	// new menu item
	BMenuItem *nwin = new BMenuItem("New", new BMessage(DOC_APP_NEW_WINDOW), 'N');
	nwin->SetTarget(be_app);
	file->AddItem(nwin);

	// open menu item
	BMenuItem* open = new BMenuItem(BRecentFilesList::NewFileListMenu("Open" B_UTF8_ELLIPSIS,
		NULL, NULL, be_app, 10, false, NULL, "application/x-vnd.Be.QuickRes"), new BMessage(DOC_APP_OPEN));
	open->SetShortcut('O', B_COMMAND_KEY);
	open->SetTarget(be_app);
	file->AddItem(open);

	// merge menu item
	BMenuItem *merge = new BMenuItem("Merge From" B_UTF8_ELLIPSIS, new BMessage(kMergeRequest), 'M');
	merge->SetTarget(this);
	file->AddItem(merge);

	file->AddSeparatorItem();
	
	// save menu item
	BMenuItem *save = new BMenuItem("Save", new BMessage(DOC_WIN_SAVE), 'S');
	save->SetTarget(this);
	file->AddItem(save);

	// save as menu item
	BMenuItem *saveas = new BMenuItem("Save As" B_UTF8_ELLIPSIS, new BMessage(DOC_WIN_SAVE_AS), 0);
	saveas->SetTarget(this);
	file->AddItem(saveas);

	// close menu item
	BMenuItem *close = new BMenuItem("Close", new BMessage(B_QUIT_REQUESTED), 'W');
	close->SetTarget(this);
	file->AddItem(close);

	file->AddSeparatorItem();
	
	// about menu item
	BMenuItem *about = new BMenuItem("About QuickRes", new BMessage(B_ABOUT_REQUESTED));
	about->SetTarget(be_app);
	file->AddItem(about);

	file->AddSeparatorItem();
	
	// quit menu item
	BMenuItem *quit = new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q');
	quit->SetTarget(be_app);
	file->AddItem(quit);

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

	edit->AddSeparatorItem();
	
	// add menu item
	fAddItem = new BMenuItem("New Item...", new BMessage(kAddItem), 'I');
	fAddItem->SetTarget(this);
	edit->AddItem(fAddItem);

	// duplicate menu item
	fDupItem = new BMenuItem("Duplicate Item", new BMessage(kDuplicateItem), 'D');
	fDupItem->SetTarget(this);
	edit->AddItem(fDupItem);

	edit->AddSeparatorItem();
	
	// edit menu item
	fEditInplaceItem = new BMenuItem("Edit In Place",
									 new BMessage(kInplaceEditor), 'E');
	fEditInplaceItem->SetTarget(this);
	edit->AddItem(fEditInplaceItem);

	fEditWindowItem = new BMenuItem("Edit In Window" B_UTF8_ELLIPSIS,
									 new BMessage(kWindowEditor),
									 'E', B_SHIFT_KEY|B_COMMAND_KEY);
	fEditWindowItem->SetTarget(this);
	edit->AddItem(fEditWindowItem);

	// Window menu
	BMenu* windowmenu = wr->WindowMenu(this);
	menubar->AddItem(windowmenu);

	// add menu bar to window
	AddChild(menubar);
	BRect area = Bounds();
	area.top = menubar->Bounds().Height() + 1;

	fController.SetMenus(menubar, edit, windowmenu);
	
	// Add background box
	fRoot = new BView(area, "root", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP);
	fRoot->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(fRoot);

	BRect r = area;
	r.OffsetTo(0, 0);
	r.InsetBy(SPACER, SPACER);

	fColumns = new ColumnContainer(BRect(r.left, r.top, r.left+100, r.top+100),
									"column_container", SPACER,
									B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP);
	
	// resource list area
	fResList = new ResourceListView(BRect(0, 0, 400, 400), "reslist", B_FOLLOW_NONE);
	fResList->SetupColumns(*this, fAddons);
	BMessage listconfig;
	if (fWindowConfiguration.FindMessage(SETTING_LISTCONFIG, &listconfig) == B_OK)
		fResList->LoadState(&listconfig);
	fResList->SetInvocationMessage(new BMessage(kWindowEditor));
	
	BList additems;
	if( fAddons.GetGenerateMenuItems(&additems) == B_OK ) {
		BPopUpMenu* addpop = new BPopUpMenu("New", false, false);
		addpop->AddList(&additems, 0);
		fAddField = new BMenuField(BRect(0, 0, 10, 10), "add", "", addpop, false, B_FOLLOW_NONE);
	}
	
	#if 0
	BPopUpMenu* showpop = new BPopUpMenu("Show: ", true, true);
	showpop->AddItem(new BMenuItem(""));
	BList additems;
	if( fAddons.GetGenerateMenuItems(&additems) == B_OK ) {
		BPopUpMenu* addpop = new BPopUpMenu("New", false, false);
		addpop->AddList(&additems, 0);
		fAddField = new BMenuField(BRect(0, 0, 10, 10), "add", "", addpop,
								   B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	}
	#endif
	
	fRemoveButton = new BButton(BRect(0, 0, 10, 10), "remove", "Delete", new BMessage(B_CLEAR), B_FOLLOW_NONE);
	
	fColumns->AddColumnView(fResList);
	if( fAddField ) fColumns->AddButton(fAddField);
	fColumns->AddButton(fRemoveButton);
	
	fRoot->AddChild(fColumns);
	
	font_height fh;
	fResList->GetFontHeight(&fh);
	BRect resRect(r);
	resRect.bottom = resRect.top + (fh.ascent+fh.descent+fh.leading)*18;
	fColumns->ResizeTo(resRect.Width(), resRect.Height());
	
	fDivider = new DividerControl(BPoint(0, resRect.bottom+1), "divider",
								  new BMessage(kShowEditor), B_HORIZONTAL,
								  B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP);
	fRoot->AddChild(fDivider);
	fDivider->SetTarget(this);
	
	float pw, ph;
	fDivider->GetPreferredSize(&pw, &ph);
	
	// detail/ResEditor area
	fContainer = new ViewWrapper(BRect(fRoot->Bounds().left, resRect.bottom+ph+SPACER,
										fRoot->Bounds().right, fRoot->Bounds().bottom),
								 "edit_container", B_FOLLOW_ALL,
								  B_NAVIGABLE_JUMP);
	fRoot->AddChild(fContainer);

	fController.SetContainer(fContainer);
	
	UpdateControlState();

	// setup notification
	fResList->SetTarget(this);
	fResList->SetSelectionMessage(new BMessage('sele'));
	
	Lock();
	AddCommonFilter(this);
	Unlock();
}

thread_id ResWindow::Run()
{
	bool showeditor;
	if( fWindowConfiguration.FindBool(SETTING_SHOWEDITOR, &showeditor) != B_OK ) {
		showeditor = false;
	}
	
	if( !showeditor ) {
		fDivider->SetValue(0);
		float h = Frame().Height();
		UpdateEditorVisibility();
		ResizeTo(Frame().Width(), h);
	}
	
	float edheight;
	if( fWindowConfiguration.FindFloat(SETTING_EDITORHEIGHT, &edheight) == B_OK ) {
		float cw=0, ch=0;
		fColumns->GetPreferredSize(&cw, &ch);
		if( fDivider->Value() != 0 ) {
			if( edheight > Frame().Height()-fDivider->Frame().Height()-ch-fColumns->Frame().top ) {
				edheight = Frame().Height()-fDivider->Frame().Height()-ch-fColumns->Frame().top;
			}
		} else {
			BScreen s(this);
			if( edheight > s.Frame().Height()-Frame().Height() ) {
				edheight = s.Frame().Height()-Frame().Height();
			}
		}
		float delta = floor( edheight - fContainer->Frame().Height() + .5 );
		fContainer->ResizeBy(0, delta);
		if( fDivider->Value() != 0 ) {
			fContainer->MoveBy(0, -delta);
			fDivider->MoveBy(0, -delta);
			fColumns->ResizeBy(0, -delta);
		}
	}
	
	return inherited::Run();
}

bool ResWindow::QuitRequested()
{
	// First give all editors a chance to apply any cached changes they
	// contain.
	for( int32 i=0; i<fResList->CountRows(); i++ ) {
		ResRow* row = dynamic_cast<ResRow*>(fResList->RowAt(i));
		if( row ) {
			row->ApplyChanges();
			if( row->WindowEditor().IsValid() ) {
				BMessage msg(EDITWIN_APPLYCHANGES);
				BMessage reply;
				// Send apply request, waiting for a reply.  We use a
				// timeout here to work around a bug where SendMessage()
				// will not fail if the target thread exits while waiting
				// for a reply.
				status_t res;
				do {
					res = row->WindowEditor().SendMessage(&msg, &reply,
														  2000*1000, 2000*1000);
				} while (res==B_TIMED_OUT);
			}
		}
	}
	
	fController.ApplyChanges();
	
	// If it doesn't look like there are any changes that need to be saved,
	// then try closing down all external editors right now.  This gives them
	// a chance to apply any changes, if they didn't implement the
	// EDITWIN_APPLYCHANGES request above.
	bool was_dirty = IsDirty();
	if( !was_dirty ) {
		for( int32 i=0; i<fResList->CountRows(); i++ ) {
			ResRow* row = dynamic_cast<ResRow*>(fResList->RowAt(i));
			if( row && row->WindowEditor().IsValid() ) {
				BMessage msg(B_QUIT_REQUESTED);
				msg.AddBool("wants_reply", true);
				BMessage reply;
				// Send quit request, waiting for a reply.  We use a
				// timeout here to work around a bug where SendMessage()
				// will not fail if the target thread exits while waiting
				// for a reply.
				status_t res;
				do {
					res = row->WindowEditor().SendMessage(&msg, &reply,
														  500*1000, 500*1000);
				} while (res==B_TIMED_OUT);
			}
		}
	}
	
	bool result = DocWindow::QuitRequested();
	
	// If we are now shutting down, close all editor windows.
	if( result ) {
		bool prev_dirty = IsDirty();
		
		for( int32 i=0; i<fResList->CountRows(); i++ ) {
			ResRow* row = dynamic_cast<ResRow*>(fResList->RowAt(i));
			if( row ) {
				if( row->WindowEditor().IsValid() ) {
					BMessage msg(B_QUIT_REQUESTED);
					msg.AddBool("wants_reply", true);
					BMessage reply;
					// Send quit request, waiting for a reply.  We use a
					// timeout here to work around a bug where SendMessage()
					// will not fail if the target thread exits while waiting
					// for a reply.
					status_t res;
					do {
						res = row->WindowEditor().SendMessage(&msg, &reply,
															  500*1000, 500*1000);
					} while (res==B_TIMED_OUT);
				}
			}
		}
		
		fController.ForgetEditor();
		fResList->EndEdit();
		
		// At this point, there is a chance that new changes have been
		// applied since the document was saved at the user's request.
		// If that is the case, tell the user about this and abort the
		// quit.
		if( IsDirty() && !prev_dirty ) {
			(new BAlert(B_EMPTY_STRING, "Some open editors had changes that "
										"were not saved.  Select quit again to "
										"continue.",
						"Okay", 0, 0, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
			return false;
		}
		
		if( fAddWindow.IsValid() ) {
			BMessage msg(B_QUIT_REQUESTED);
			fAddWindow.SendMessage(&msg);
		}
	}
	
	return result;
}

void ResWindow::Quit()
{
	fController.ForgetEditor();
	
	SaveSettings();
	
	if( !fContainer->Parent() ) delete fContainer;
	
	// We have to clear out all state that can reference add-ons
	// before being deleted, so that the add-ons are still loaded.
	fResList->Clear();
	fUndo.ForgetUndos();
	fUndo.ForgetRedos();
	
	RemoveCommonFilter(this);
	
	DocWindow::Quit();
}

void ResWindow::MenusBeginning()
{
	fController.UpdateUndoMenus(fUndoItem, fRedoItem);
}

void ResWindow::WindowFrame(BRect *proposed)
{
	BAutolock l(this);
	
	BRect	saved;
	if(fWindowConfiguration.FindRect(SETTING_FRAME, &saved) == B_OK)
	{
		*proposed = saved;
	}
	else
	{
		proposed->right = proposed->left + 550;
		proposed->bottom = proposed->top + 350;
	}
}

void ResWindow::LoadSettings()
{
	ssize_t err = B_OK;
	
	BMessage settings;
	
	entry_ref doc_entry = FileRef();
	if( doc_entry == entry_ref() ) err = B_ERROR;
	
	if( err >= B_OK ) {
		BNode node(&doc_entry);
		err = node.InitCheck();
		attr_info attr;
		if( err >= B_OK ) {
			err = node.GetAttrInfo("be:quickres", &attr);
		}
		void* buf = 0;
		if( err >= B_OK && attr.type == B_MESSAGE_TYPE ) {
			buf = malloc(attr.size);
			err = node.ReadAttr("be:quickres", attr.type, 0, buf, attr.size);
		}
		if( err >= B_OK ) {
			BMemoryIO io(buf, attr.size);
			err = settings.Unflatten(&io);
		}
		free(buf);
	}
	
	if( err < B_OK ) {
		BPath	prefs_name;
		BFile	prefs_file;
		err = find_directory(B_USER_SETTINGS_DIRECTORY, &prefs_name);
		if( err >= B_OK ) err = prefs_name.Append("QuickRes_settings");
		if( err >= B_OK ) err = prefs_file.SetTo(prefs_name.Path(), B_READ_ONLY);
		if( err >= B_OK ) err = settings.Unflatten(&prefs_file);
	}
	
	if( err >= B_OK ) {
		settings.FindMessage("be:document", &fWindowConfiguration);
		BMessage edsettings;
		if( settings.FindMessage("be:editors", &edsettings) == B_OK ) {
			fAddons.SetAllConfigurations(&edsettings);
		}
	}
}

void ResWindow::SaveSettings() const
{
	ResWindow* This = const_cast<ResWindow*>(this);
	This->fWindowConfiguration = BMessage();
	This->fWindowConfiguration.AddRect(SETTING_FRAME, Frame());
	This->fWindowConfiguration.AddBool(SETTING_SHOWEDITOR,
									   fDivider->Value() ? true : false);
	This->fWindowConfiguration.AddFloat(SETTING_EDITORHEIGHT,
										fContainer->Bounds().Height());
	if( fResList ) {
		BMessage listconfig;
		fResList->SaveState(&listconfig);
		This->fWindowConfiguration.AddMessage(SETTING_LISTCONFIG, &listconfig);
	}
	
	BMessage settings;
	settings.AddMessage("be:document", &This->fWindowConfiguration);
	BMessage edsettings;
	if( fAddons.GetAllConfigurations(&edsettings) == B_OK ) {
		settings.AddMessage("be:editors", &edsettings);
	}
	
	entry_ref doc_entry = FileRef();
	if( doc_entry != entry_ref() && fAttributeSettings ) {
		BMallocIO io;
		if( settings.Flatten(&io) >= B_OK ) {
			BNode node(&doc_entry);
			if( node.InitCheck() >= B_OK ) {
				node.WriteAttr("be:quickres", B_MESSAGE_TYPE,
							   0, io.Buffer(), io.BufferLength());
			}
		}
	} else {
		BPath	prefs_name;
		BFile	prefs_file;
		if(find_directory(B_USER_SETTINGS_DIRECTORY, &prefs_name) >= B_OK &&
			prefs_name.Append("QuickRes_settings") >= B_OK &&
			prefs_file.SetTo(prefs_name.Path(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE) >= B_OK)
		{
			settings.Flatten(&prefs_file);
		}
	}
}

void ResWindow::MessageReceived(BMessage *msg)
{
	if( fController.ExecMessageReceived(msg) == B_OK ) return;
	
	switch(msg->what)
	{
		case kMergeRequest :
		{
			RequestMerge();
		} break;
			
		case kMergeResources :
		{
			entry_ref ref;
			msg->FindRef("refs", &ref);

			BEntry e(&ref);
			Merge(&e);
		} break;
		
		case kCollectionChanged:
		{
			SyncResList();
		} break;
		
		case kShowEditor:
		{
			UpdateEditorVisibility();
		} break;
		
		case B_CUT:
		{
			Cut(be_clipboard);
		} break;
		
		case B_COPY:
		{
			Copy(be_clipboard);
		} break;
		
		case B_PASTE:
		{
			Paste(be_clipboard);
		} break;
		
		case kInplaceEditor:
		{
			if( fCurItem.IsValid() ) {
				fResList->BeginEdit(true);
			}
		} break;
		
		case kWindowEditor:
		{
			BRow* r = fResList->CurrentSelection();
			BPoint pt;
			uint32 but;
			fRoot->GetMouse(&pt, &but, false);
			fRoot->ConvertToScreen(&pt);
			while( r ) {
				ResRow* row = dynamic_cast<ResRow*>(r);
				if( row ) {
					if( !row->WindowEditor().IsValid() ) {
						BWindow* win =
							new EditWindow(	BResourceAddonArgs(*this), *row,
											fAddons, pt, Title() );
						win->Show();
						pt.x += 20; pt.y += 20;
						row->SetWindowEditor(BMessenger(win));
					} else {
						BMessage msg(EDITWIN_ACTIVATE);
						row->WindowEditor().SendMessage(&msg);
					}
				}
				r = fResList->CurrentSelection(r);
			}
		} break;
		
		case kFinalChange:
		{
			int32 rowidx;
			PRINT(("Changing row...\n"));
			if( msg->FindInt32("row", &rowidx) == B_OK ) {
				PRINT(("The change is on row #%ld\n", rowidx));
				if( WriteLockCollection() == B_OK ) {
					ResRow* row = dynamic_cast<ResRow*>(fResList->RowAt(rowidx));
					PRINT(("Row object is %p\n", row));
					if( row ) {
						BResourceItem* item = WriteItem(*row);
						PRINT(("Resource item for this row is %p\n", item));
						if( item ) {
							const char* nm = row->Write(item);
							if( nm ) fUndo.SuggestUndoName(nm);
						}
					}
					WriteUnlockCollection();
				}
			}
		} break;
		
		case kAddItem :
		{
			if( !fAddWindow.IsValid() ) {
				BList items;
				if( fAddons.GetGenerateListItems(&items) == B_OK ) {
					BPoint pt;
					uint32 but;
					fRoot->GetMouse(&pt, &but, false);
					fRoot->ConvertToScreen(&pt);
					add_types type = ADD_ANYTHING;
					switch( fSaveFormat ) {
						case SOURCE_FORMAT:
							type = ADD_RESOURCES_ONLY;
							break;
						case BINARY_FORMAT:
							type = ADD_ANYTHING;
							break;
						case ATTRIBUTE_FORMAT:
							type = ADD_ATTRIBUTES_ONLY;
							break;
						default:
							TRESPASS();
					}
					AddWindow* win = new AddWindow(&items, type,
												   BMessenger(this),
												   pt, Title());
					win->Show();
					fAddWindow = BMessenger(win);
				}
			} else {
				BMessage msg(ADDWIN_ACTIVATE);
				fAddWindow.SendMessage(&msg);
			}
		} break;

		case B_GENERATE_RESOURCE :
		{
			if( WriteLockCollection("New Item") == B_OK ) {
				int32 id = 0;
				
				BResourceHandle oldItem = CurrentItem();
				const BResourceItem* it = ReadItem(oldItem);
				
				if( fSaveFormat == ATTRIBUTE_FORMAT ) {
					id = -1;
				} else {
					bool attr;
					if( fSaveFormat != SOURCE_FORMAT
							&& msg->FindBool("be:attribute", &attr) == B_OK && attr ) {
						id = -1;
					} else if( msg->FindInt32("be:id", &id) == B_OK ) {
						// use the given attribute.
					} else {
						const ResEntry* e = dynamic_cast<const ResEntry*>(it);
						printf("ResEntry %p from BResourceItem %p\n", e, it);
						if( fSaveFormat != SOURCE_FORMAT && e && e->IsAttribute() ) {
							id = -1;
						} else {
							if( it ) id = it->ID();
							else id = 1;
						}
					}
				}
				
				const char* name;
				if( msg->FindString("be:item_name", &name) != B_OK ) name = NULL;
				
				BResourceHandle newItem;
				fAddons.GenerateResource(*this, &newItem, msg, id, name);
				WriteUnlockCollection();
			}
		} break;

		case B_CLEAR :
		{
			Clear();
		} break;

		case kBackspaceItem :
		{
			if( WriteLockCollection("Delete Item") == B_OK ) {
				BRow* r = fResList->CurrentSelection(0);
				if( r ) {
					int32 idx = fResList->IndexOf(r);
					ResRow* prev = dynamic_cast<ResRow*>
							(idx > 0 ? fResList->RowAt(idx-1) : 0);
					if( prev ) {
						RemoveItem(*prev);
					}
				}
				WriteUnlockCollection();
			}
			
		} break;

		case kDuplicateItem :
		{
			if( WriteLockCollection("Duplicate Item") == B_OK ) {
				BRow* r = 0;
				while( (r=fResList->CurrentSelection(r)) != 0 ) {
					ResRow* row = dynamic_cast<ResRow*>(r);
					const BResourceItem* it = row ? ReadItem(*row) : 0;
					if( it ) {
						BResourceHandle h;
						AddItem(&h, it->Type(), it->ID(), it->Name(), it->Data(), it->Size(),
								true, B_RENAME_NEW_ITEM);
					}
				}
				WriteUnlockCollection();
			}
		} break;
		
		case 'sele' :
		{
			ResRow* row = dynamic_cast<ResRow*>(fResList->CurrentSelection());
			const ResourceEntry* newItem = 0;
			if( WriteLockCollection() == B_OK ) {
				if( row && fResList->CurrentSelection(row) == 0 ) {
					newItem = dynamic_cast<const ResourceEntry*>(ReadItem(*row));
					SetCurrentItem(*row);
				} else {
					SetCurrentItem(BResourceHandle());
				}
				WriteUnlockCollection();
			}
			UpdateControlState();
			fController.UpdateEditor(BResourceHandle(const_cast<ResourceEntry*>(newItem)));
		} break;

		default :
			DocWindow::MessageReceived(msg);
			break;
	}
}

status_t ResWindow::Load(BEntry *ent, bool merge)
{
	ssize_t err = WriteLockCollection(merge ? "Merge" : "Load");
	if( err != B_OK ) return err;
	
	entry_ref	ref;
	BString		buf;
	BResourceHandle handle;
	type_code	type;
	int32		id;
	size_t		length;
	const char	*op = "reading";
	const char	*name;
	const void	*ptr;
	BString		message;
	BString		filename;
	BString		where;
	
	op = "finding";
	err = ent->GetRef(&ref);
	
	BFile f;
	if( !err ) {
		op = "opening";
		err = f.SetTo(&ref, B_READ_ONLY);
	}
	
	BResources	res;
	if( !err ) {
		op = "parsing";
		err = res.SetTo(&f);
	}
	
	fAttributeSettings = false;
	
	if( (!merge || fSaveFormat != ATTRIBUTE_FORMAT) && err >= B_OK ) {
		res.PreloadResourceType();
		for(int32 i = 0; err >= B_OK && res.GetResourceInfo(i, &type, &id, &name, &length); i++) {
			if((ptr = res.LoadResource(type, id, &length)) != 0) {
				ResEntry* e = new ResEntry(this, type, id, NULL, NULL);
				if( e ) {
					e->ApplyLabel(name);
					e->SetData(ptr, length);
					e->SetModified(merge);
					err = AddEntry(e, false, B_RENAME_NEW_ITEM);
				}
				//err = AddItem(&handle, type, id, name, ptr, length, false);
			}
		}
		if( !merge ) {
			fSaveFormat = BINARY_FORMAT;
			if( err >= B_OK ) {
				BNodeInfo	ni;
				if(ni.SetTo(&f) == B_OK) {
					char type[B_MIME_TYPE_LENGTH];
					if( ni.GetType(type) == B_OK &&
							strcmp(type, "application/x-be-resource") == 0 ) {
						fAttributeSettings = true;
					}
				}
			}
		}
		
	} else if( (!merge || fSaveFormat != ATTRIBUTE_FORMAT) && f.IsReadable() ) {
		// This is not a binary resource file -- try to parse as a text file.
		res.SetTo(NULL);
		
		op = "finding";
		BPath path;
		ResParser parser(*this, merge);
		
		err = path.SetTo(&ref);
		if( err >= B_OK ) {
			op = "opening";
			err = parser.SetTo(path.Path());
		}
		
		if( err >= B_OK ) {
			op = "parsing";
			err = parser.Run();
			if( err ) {
				message = parser.ErrorAt().Message();
				filename = parser.ErrorAt().File();
				where << parser.ErrorAt().Line();
			}
		}
		
		if( !merge ) {
			if( err >= B_OK || parser.NumberRead() > 0 ) {
				fSaveFormat = SOURCE_FORMAT;
				fAttributeSettings = true;
			} else {
				fSaveFormat = ATTRIBUTE_FORMAT;
			}
		}
	}

	if( !merge && fSaveFormat == ATTRIBUTE_FORMAT ) {
		// Read in body of file as a single resource.
		op = "opening";
		err = f.SetTo(&ref, B_READ_ONLY);
		if( err == B_OK ) {
			off_t size = 0;
			err = f.GetSize(&size);
			void* buf = err >= B_OK ? malloc(size) : 0;
			if( buf ) {
				BString mimeSig = "";
				
				BNodeInfo	ni;
				if(ni.SetTo(&f) == B_OK) {
					char type[B_MIME_TYPE_LENGTH];
					if( ni.GetType(type) == B_OK ) {
						mimeSig = type;
					}
				}
				
				err = f.ReadAt(0, buf, size);
				if( err >= B_OK ) {
					ResEntry* e = new ResEntry(this, B_MIME_TYPE, 0,
											   mimeSig.String(), NULL);
					if( e ) {
						e->SetData(buf, size);
						e->SetIsBody(true);
						e->SetModified(merge);
						err = AddEntry(e, false, B_RENAME_NEW_ITEM);
					}
				}
				free(buf);
			}
		}
	}
	
	if( (!merge || fSaveFormat == ATTRIBUTE_FORMAT) &&
			(fSaveFormat == BINARY_FORMAT || fSaveFormat == ATTRIBUTE_FORMAT) ) {
		if( f.InitCheck() >= B_OK ) {
			op = "reading attributes";
			// Read in all attributes.
			f.RewindAttrs();
			char attrname[B_ATTR_NAME_LENGTH];
			while( f.GetNextAttrName(attrname) >= B_OK ) {
				attr_info info;
				err = f.GetAttrInfo(attrname, &info);
				void* buf = err >= B_OK ? malloc(info.size) : 0;
				if( buf ) {
					err = f.ReadAttr(attrname, info.type, 0, buf, info.size);
					if( err >= B_OK ) {
						ResEntry* e = new ResEntry(this, info.type, 0, attrname, NULL);
						if( e ) {
							e->SetData(buf, info.size);
							e->SetIsAttribute(true);
							e->SetModified(merge);
							err = AddEntry(e, false, B_RENAME_NEW_ITEM);
						}
					}
					free(buf);
				}
				if( err < B_OK ) {
					message << "Attribute " << attrname
						<< " couldn't be read: "
						<< strerror(err);
					break;
				}
			}
		}
		err = B_OK;
	}
	
	const bool got_something = (!merge && fItems.size() > 0) ? true : false;
	
	WriteUnlockCollection();
	
	SetDirty(false);
	fBodyChanged = false;
	
	// Special case: If this is the first thing done in the window,
	// remove the load from the undo stack.
	if( fUndo.CountUndos() == 1 ) {
		// TO DO: Should be acquiring a write lock.
		fUndo.ForgetUndos();
	}
	
	if( err < B_OK ) {
		BString str("Error ");
		str += op;
		str += " file";
		if( filename.Length() > 0 ) {
			str += "\n";
			str += filename;
		}
		if( where.Length() > 0 ) {
			str += " at line ";
			str += where;
		}
		str += ":\n";
		if( message.Length() > 0 ) {
			str += message;
		} else {
			str += strerror(err);
		}
		str += ".";
		(new BAlert(Title(), str.String(), "OK",
					NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
	}
	
	if( got_something ) return B_OK;
	
	return err;
}

status_t ResWindow::Merge(BEntry *ent)
{
	status_t err = Load(ent, true);
	if( err == B_OK ) SetDirty(true);
	return err;
}

void ResWindow::Cut(BClipboard* clipboard)
{
	if( WriteLockCollection("Cut") != B_OK ) return;
	
	Copy(clipboard);
	Clear();
	
	WriteUnlockCollection();
}

void ResWindow::Paste(BClipboard* clipboard)
{
	if( WriteLockCollection("Paste") != B_OK ) return;
	
	clipboard->Lock();
	
	BMessage *clip_msg = clipboard->Data();
	if( clip_msg->HasMessage("application/x-vnd.Be.ResourceArchive") ) {
		SetCurrentItem(BResourceHandle());
		status_t err;
		int32 i=0;
		BMessage archive;
		while( (err=clip_msg->FindMessage("application/x-vnd.Be.ResourceArchive",
										  i, &archive)) == B_OK ) {
			ResEntry* entry = new ResEntry(this, &archive);
			if( entry ) {
				entry->SetModified(true);
				err = AddEntry(entry, true);
			}
			i++;
		}
		
	} else {
		fAddons.MessagePaste(*this, clip_msg);
	}
	
	clipboard->Unlock();
	
	WriteUnlockCollection();
}

void ResWindow::Clear()
{
	if( WriteLockCollection("Delete") == B_OK ) {
		BRow* r = 0;
		BRow* next = 0;
		while( (r=fResList->CurrentSelection(r)) != 0 ) {
			if( next == r ) next = 0;
			if( !next ) {
				int32 idx = fResList->IndexOf(r);
				next = fResList->RowAt(idx+1);
			}
			ResRow* row = dynamic_cast<ResRow*>(r);
			if( row ) {
				RemoveItem(*row);
			}
		}
		ResRow* row = dynamic_cast<ResRow*>(next);
		if( row ) SetCurrentItem(*row);
		else SetCurrentItem(BResourceHandle());
		WriteUnlockCollection();
	}
}

status_t ResWindow::Save(BEntry *ent, const BMessage* args)
{
	// Pull out desired format from message, if set.
	if( args ) {
		int32 format;
		if( args->FindInt32("format_id", &format) == B_OK ) {
			if( fSaveFormat == ATTRIBUTE_FORMAT &&
					format != ATTRIBUTE_FORMAT ) {
				int32 sel =
					(new BAlert(B_EMPTY_STRING, "Delete the existing data in this file? "
												"Only the file attributes were "
												"originally read in.",
								"Cancel", "Overwrite", 0,
								B_WIDTH_AS_USUAL, B_OFFSET_SPACING,
								B_WARNING_ALERT))->Go();
				if( sel == 0 ) return B_ERROR;
			}
			fSaveFormat = format;
		}
	}
	
	status_t err = ReadLockCollection();
	if( err ) return err;
	
	if( fSaveFormat == BINARY_FORMAT ) {
		err = SaveBinary(ent);
		if( err == B_OK ) err = SaveAttributes(ent);
	} else if( fSaveFormat == ATTRIBUTE_FORMAT ) {
		err = SaveAttributes(ent);
	} else {
		err = SaveSource(ent);
	}
	
	// Make sure current modification state of all rows is currently
	// displayed.
	
	if( err == B_OK ) {
		SetDirty(false);
		fBodyChanged = false;
	}
	
	ReadUnlockCollection();
	
	// Make sure to update displayed modification state of all rows.
	if( fResList ) {
		BResourceAddonBase acc(BResourceAddonArgs(*this));
		for( int32 i=0; i<fResList->CountRows(); i++ ) {
			ResRow* r = dynamic_cast<ResRow*>(fResList->RowAt(i));
			// This magic number is a gross hack to make it only update
			// the modification state.
			if( r ) UpdateRow(acc, r, 0x10000000, false);
		}
		fResList->Invalidate();
	}
	
	return err;
}

status_t ResWindow::SaveAttributes(BEntry *ent)
{
	BNode		n(ent);
	ssize_t		err = n.InitCheck();
	int32		num_failed = 0;
	ssize_t		first_err = B_OK;
	bool		body_failed = false;
	
	if( err >= B_OK ) {
		// First erase any existing attributes.  This is dangerous, if
		// later write operations fail...  on the other hand, it's easy,
		// and allows us to order the writes for optimal compaction.
		size_t i;
		for (i=0; i<fItems.size(); i++) {
			const BResourceItem* item = ReadItem(fItems[i]);
			const ResEntry* e = dynamic_cast<const ResEntry*>(item);
			if( e && e->IsAttribute() ) {
				n.RemoveAttr(item->Name());
			}
		}
		for (i=0; i<fDeletedAttrs.size(); i++) {
			const BResourceItem* item = ReadItem(fDeletedAttrs[i]);
			const ResEntry* e = dynamic_cast<const ResEntry*>(item);
			if( e && e->IsAttribute() ) {
				n.RemoveAttr(item->Name());
			}
		}
		
		AttributeOrder order(*this, fItems);
		BString buf;
		for( i=0; i<fItems.size(); i++ ) {
			const BResourceItem* item = ReadItem(fItems[order[i]]);
			const ResEntry* e = dynamic_cast<const ResEntry*>(item);
			if( e && e->IsAttribute() ) {
				err = n.WriteAttr(item->Name(), item->Type(), 0,
								  item->Data(), item->Size());
				if( err >= B_OK && e ) e->SetModified(false);
				else {
					num_failed++;
					if( first_err != B_OK ) first_err = err;
				}
			} else if( e && e->IsBody() && fBodyChanged ) {
				// This is the item representing the file data body
				// and it has changed -- rewrite file.
				BFile file(ent, B_READ_WRITE);
				err = file.SetSize(item->Size());
				if( err >= B_OK ) {
					err = file.WriteAt(0, item->Data(), item->Size());
				}
				if( err >= B_OK && e ) e->SetModified(false);
				else {
					num_failed++;
					first_err = err;
					body_failed = true;
				}
			}
		}
	}

	if( first_err < B_OK ) {
		BString str("Error writing file");
		BPath path;
		if( ent->GetPath(&path) == B_OK ) {
			str << " " << path.Path();
		}
		str << (body_failed ? " data:" : " attributes:\n")
			<< strerror(first_err) << ".";
		(new BAlert(Title(), str.String(), "OK",
					NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
	}
	
	return err >= B_OK ? B_OK : err;
}

status_t ResWindow::SaveBinary(BEntry *ent)
{
	BFile		f(ent, B_READ_WRITE);
	BResources	res;
	status_t	err = B_OK;

	// more code ripped from StyledEdit
	// maybe this code should be factored out in DocWindow
	if((err = f.InitCheck()) == B_NO_ERROR)
	{
		mode_t perms = 0;
		f.GetPermissions(&perms);
	
		if((perms & (S_IWUSR | S_IWGRP | S_IWOTH)) == 0)
		{
			(new BAlert(B_EMPTY_STRING, "File is read-only.", "OK"))->Go();
			return err;
		}
	}
	else
	{
		BDirectory dir;
		ent->GetParent(&dir);

		entry_ref ref;
		ent->GetRef(&ref);

		dir.CreateFile(ref.name, &f);

		err = f.InitCheck();
		if( err == B_OK ) {
			BNodeInfo	ni;
			if(ni.SetTo(&f) == B_OK)
				ni.SetType("application/x-be-resource");
		}
	}

	if( err == B_OK && (err=res.SetTo(&f, true)) == B_OK ) {
		SaveOrder order(*this, fItems);
		BString buf;
		for( size_t i=0; err == B_OK && i<fItems.size(); i++ ) {
			const BResourceItem* item = ReadItem(fItems[order[i]]);
			const ResEntry* e = dynamic_cast<const ResEntry*>(item);
			if( item && (!e || !e->IsAttribute()) ) {
				err = res.AddResource(item->Type(), item->ID(),
									  item->Data(), item->Size(),
									  item->CreateLabel(&buf));
			}
		}

		if( err == B_OK ) err = res.Sync();
	}

	if( err == B_OK ) {
		for( size_t i=0; err == B_OK && i<fItems.size(); i++ ) {
			const BResourceItem* item = ReadItem(fItems[i]);
			const ResEntry* e = dynamic_cast<const ResEntry*>(item);
			if( item && (!e || !e->IsAttribute()) ) {
				if( e ) e->SetModified(false);
			}
		}
	}
	
	if( err != B_OK ) {
		BString str("Error writing file");
		BPath path;
		if( ent->GetPath(&path) == B_OK ) {
			str << " " << path.Path();
		}
		str << ":\n" << strerror(err) << ".";
		(new BAlert(Title(), str.String(), "OK",
					NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
	}
	
	return err;
}

status_t ResWindow::SaveSource(BEntry *ent)
{
	ResParser parser(*this, false);
	
	BPath path;
	status_t err = ent->GetPath(&path);
	if( err ) {
		ErrorInfo info;
		info.SetTo(err, "Unable to open file:\n%s", strerror(err));
		parser.Error(info);
	}
	
	if( !err ) err = parser.StartWritingHeader(path.Path());
	
	SaveOrder order(*this, fItems);
	for( size_t i=0; err == B_OK && i<fItems.size(); i++ ) {
		const BResourceItem* item = ReadItem(fItems[order[i]]);
		const ResEntry* e = dynamic_cast<const ResEntry*>(item);
		if( item && (!e || !e->IsAttribute()) ) {
			err = parser.WriteItem(item);
			if( err >= B_OK && e ) e->SetModified(false);
		}
	}

	if( err != B_OK ) {
		BString str("Error writing file");
		if( path.InitCheck() == B_OK ) {
			str << " " << path.Path();
		}
		str << ":\n";
		BString message = parser.ErrorAt(0).Message();
		if( message.Length() > 0 ) {
			str += message;
		} else {
			str += strerror(err);
		}
		str += ".";
		(new BAlert(Title(), str.String(), "OK",
					NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
	}
	
	return err;
}

void ResWindow::Copy(BClipboard* clipboard)
{
	BRow* r = fResList->CurrentSelection();
	if( !r ) return;
	
	if( ReadLockCollection() != B_OK ) return;
	
	clipboard->Lock();
	clipboard->Clear();
	
	BMessage *clip_msg = clipboard->Data();
	status_t err = B_OK;
	while( err == B_OK && r != 0 ) {
		ResRow* row = dynamic_cast<ResRow*>(r);
		const BResourceItem* item = row ? ReadItem(*row) : 0;
		if( item ) {
			BMessage archive;
			err = item->Archive(&archive);
			if( err == B_OK ) {
				err = clip_msg->AddMessage("application/x-vnd.Be.ResourceArchive",
										&archive);
			}
		}
		r = fResList->CurrentSelection(r);
	}
	
	clipboard->Commit();
	clipboard->Unlock();
	
	ReadUnlockCollection();
}

BFilePanel* ResWindow::CreateSavePanel() const
{
	BMessenger self(this);
	SavePanel* panel = new SavePanel(save_formats, &self);
	panel->SetFormatID(fSaveFormat);
	return panel;
}

/***
**** BResourceCollection interface.
***/

status_t ResWindow::GetNextItem(size_t *inout_cookie, BResourceHandle* out_item) const
{
	if( !AssertReadLock("GetNextItem()") ) return B_ERROR;
	
	if( *inout_cookie >= fItems.size() ) {
		*inout_cookie = 0;
		*out_item = BResourceHandle();
		return B_BAD_INDEX;
	}
	
	*out_item = fItems[*inout_cookie];
	(*inout_cookie)++;
	return B_OK;
}

status_t ResWindow::AddEntry(ResEntry* e, bool make_selected,
							 conflict_resolution resolution)
{
	if( !AssertWriteLock("AddEntry()") ) return B_ERROR;
	
	const size_t N = fItems.size();
	BString eName;
	BString buf;
	
	e->CreateLabel(&eName);
	
	// See if there is a conflict.
	size_t conflict;
	for( conflict=0; conflict<N; conflict++ ) {
		const BResourceItem* it = ReadItem(fItems[conflict]);
		const ResEntry* ite = dynamic_cast<const ResEntry*>(it);
		if( ite == 0 ) continue;
		if( e->IsAttribute() || e->IsBody() ) {
			if( ite->IsAttribute() == e->IsAttribute() &&
					ite->IsBody() == e->IsBody() &&
					eName == ite->CreateLabel(&buf) ) {
				break;
			}
		} else if( it->Type() == e->Type() && it->ID() == e->ID() &&
					ite->IsAttribute() == e->IsAttribute() &&
					ite->IsBody() == e->IsBody() ) {
			break;
		}
	}
	
	if( conflict < fItems.size() ) {
		if( resolution == B_ASK_USER ) {
			if( fConflictResolution == kNotAsked ) {
				int32 sel =
					(new BAlert(B_EMPTY_STRING, "The type and ID of an added item "
												"matches an existing item.  How should "
												"this be resolved?",
								"Ask for Each", "Rename New Items",
								"Delete Old Items",
								B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
				if( sel == 0 ) fConflictResolution = kAskEvery;
				else if( sel == 1 ) fConflictResolution = kMoveAll;
				else fConflictResolution = kDeleteAll;
			}
			if( fConflictResolution == kMoveAll ) resolution = B_RENAME_NEW_ITEM;
			else if( fConflictResolution == kDeleteAll ) resolution = B_DELETE_OLD_ITEM;
			else {
				BString str("Duplicate item ");
				BString buf;
				str << BResourceParser::TypeIDToString(e->Type(), e->ID(), &buf)
					<< ".";
				int32 sel =
					(new BAlert(B_EMPTY_STRING, str.String(),
								"Rename New Item", "Delete Old Item",
								0, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
				if( sel == 0 ) resolution = B_RENAME_NEW_ITEM;
				else resolution = B_DELETE_OLD_ITEM;
			}
		}
		if( resolution == B_RENAME_NEW_ITEM ) {
			e->SetID(UniqueIDForType(e->Type(), e->ID()+1));
		} else if( resolution == B_DELETE_OLD_ITEM ) {
			BResourceHandle it(fItems[conflict]);
			RemoveItem(it);
		}
	}
	
	e->ClearChanges();
	CollectionUndoEntry* u = dynamic_cast<CollectionUndoEntry*>(fUndo.LastOperation(this));
	if( !u ) {
		u = new CollectionUndoEntry(*this);
		fUndo.AddOperation(u);
	}
	if( u ) u->AddItem(BResourceHandle(e));
	if( make_selected ) {
		SetCurrentItem(BResourceHandle());
		e->SetMakeSelected(make_selected);
	}
	PerformAdd(BResourceHandle(e));
	return B_OK;
}

status_t ResWindow::AddItem(BResourceHandle* out_item,
					type_code type, int32 id, const char* name,
					const void *data, size_t len, bool make_selected,
					conflict_resolution resolution)
{
	if( !AssertWriteLock("AddItem()") ) return B_ERROR;
	
	bool is_attr = false;
	if( id == -1 ) {
		is_attr = true;
		id = 0;
	}
	ResEntry* e = new ResEntry(this, type, id, NULL, NULL);
	if( is_attr ) {
		e->SetIsAttribute(true);
		e->SetName(name);
	} else {
		e->ApplyLabel(name);
	}
	e->SetData(data, len);
	e->SetModified(true);
	*out_item = BResourceHandle(e);
	return AddEntry(e, make_selected, resolution);
}

status_t ResWindow::FindItem(BResourceHandle* out_item,
							 type_code type, int32 id, const char* name) const
{
	if( !AssertReadLock("FindItem()") ) return B_ERROR;
	
	*out_item = BResourceHandle();
	
	int32 i;
	BString buf;
	const int32 max = fItems.size();
	for(i = 0; i < max; i++) {
		const BResourceItem* it = ReadItem(fItems[i]);
		if( (type == 0 || it->Type() == type) &&
				(id == 0 || it->ID() == id) &&
				(name == 0 || strcmp(it->CreateLabel(&buf), name) == 0) ) {
			*out_item = fItems[i];
			return B_OK;
		}
	}

	return B_NAME_NOT_FOUND;
}

status_t ResWindow::RemoveItem(BResourceHandle& item)
{
	if( !AssertWriteLock("RemoveItem()") ) return B_ERROR;
	
	bool done = PerformRemove(item);
	if( done ) {
		CollectionUndoEntry* u = dynamic_cast<CollectionUndoEntry*>(fUndo.LastOperation(this));
		if( !u ) {
			u = new CollectionUndoEntry(*this);
			fUndo.AddOperation(u);
		}
		if( u ) u->RemoveItem(item);
		return B_OK;
	}
	
	return B_BAD_VALUE;
}

status_t ResWindow::SetCurrentItem(const BResourceHandle& item)
{
	if( !AssertWriteLock("SetCurrentItem()") ) return B_ERROR;
	
	fCurItem = item;
	fChangedSelection = true;
	return B_OK;
}

BResourceHandle ResWindow::CurrentItem() const
{
	if( !AssertReadLock("CurrentItem()") ) return BResourceHandle();
	
	return fCurItem;
}

int32 ResWindow::UniqueID(int32 smallest) const
{
	if( !AssertReadLock("NextID()") ) return B_ERROR;
	
	const int32 max = fItems.size();
	
	bool conflict = true;
	while( conflict ) {
		conflict = false;
		for( int32 i=0; i<max; i++ ) {
			const BResourceItem* it = ReadItem(fItems[i]);
			if( it && smallest == it->ID() ) {
				smallest = it->ID()+1;
				conflict = true;
			}
		}
	}
	
	return smallest;
}

int32 ResWindow::UniqueIDForType(type_code type, int32 smallest) const
{
	if( !AssertReadLock("NextIDForType()") ) return B_ERROR;
	
	const int32 max = fItems.size();
	
	bool conflict = true;
	while( conflict ) {
		conflict = false;
		for( int32 i=0; i<max; i++ ) {
			const BResourceItem* it = ReadItem(fItems[i]);
			if( it && type == it->Type() && smallest == it->ID() ) {
				smallest = it->ID()+1;
				conflict = true;
			}
		}
	}
	
	return smallest;
}

int32 ResWindow::NextID(bool fromCurrent) const
{
	if( !AssertReadLock("NextID()") ) return B_ERROR;
	
	int32 lookID = 1;
	
	BResourceHandle cur = fromCurrent
						? fCurItem
						: BResourceHandle(0);
	if( cur.IsValid() ) {
		const BResourceItem* it = ReadItem(cur);
		if( it ) lookID = it->ID()+1;
	}
	
	return UniqueID(lookID);
}

int32 ResWindow::NextIDForType(type_code type, bool fromCurrent) const
{
	if( !AssertReadLock("NextIDForType()") ) return B_ERROR;
	
	int32 lookID = 1;
	
	BResourceHandle cur = fromCurrent
						? fCurItem
						: BResourceHandle(0);
	if( cur.IsValid() ) {
		const BResourceItem* it = ReadItem(cur);
		if( it ) lookID = it->ID();
	}
	
	return UniqueIDForType(type, lookID);
}

BUndoContext* ResWindow::UndoContext()
{
	if( !AssertWriteLock("UndoContext() [non-const]") ) return 0;
	
	return &fUndo;
}

const BUndoContext* ResWindow::UndoContext() const
{
	if( !AssertReadLock("UndoContext() [const]") ) return 0;
	
	return &fUndo;
}
	
status_t ResWindow::ReadLockCollection() const
{
	ResWindow* This = const_cast<ResWindow*>(this);
	if( This->fDataLock.ReadLock() ) {
		atomic_add(&This->fReadLocks, 1);
		return B_OK;
	}
	return B_ERROR;
}

status_t ResWindow::ReadUnlockCollection() const
{
	ResWindow* This = const_cast<ResWindow*>(this);
	if( This->fDataLock.ReadUnlock() ) {
		atomic_add(&This->fReadLocks, -1);
		return B_OK;
	}
	return B_ERROR;
}

status_t ResWindow::WriteLockCollection(const char* name)
{
	if( fDataLock.WriteLock() ) {
		if( fUndo.UpdateCount() == 0 ) fConflictResolution = kNotAsked;
		fUndo.StartUpdate(name);
		return B_OK;
	}
	
	return B_ERROR;
}

status_t ResWindow::WriteUnlockCollection()
{
	if( fWriteEnding ) {
		// If this is a recursive entry, let the outer level handle
		// everything.
		fUndo.EndUpdate();
		bool ok = fDataLock.WriteUnlock();
		return ok ? B_OK : B_ERROR;
	}
	
	if( fUndo.UpdateCount() == 1 ) {
		fWriteEnding = true;
		
		PRINT(("Reporting changes to %ld items.\n", fChangedItems.size()));
		while( fChangedItems.size() > 0 ) {
			BResourceHandle h = fChangedItems.back();
			fChangedItems.pop_back();
			const BResourceItem* it = ReadItem(h);
			PRINT(("Changes for item %p (%ld, %lx, %s, %s): %lx\n",
					it, it->ID(), it->Type(), it->Name(), it->Symbol(), it->Changes()));
			if( it->Changes() ) {
				fUpdatedItems.push_back(h);
				fUpdatedChanges.push_back(it->Changes());
				ResourceEntry* e = const_cast<ResourceEntry*>(
					dynamic_cast<const ResourceEntry*>(it));
				if( e ) {
					fDataLock.WriteUnlock();
					PRINT(("Reporting change to subscribers.\n"));
					e->ReportChange();
					fDataLock.WriteLock();
				}
			}
		}
		
		fUndo.EndUpdate();
	
		BMessage msg(kCollectionChanged);
		BMessenger(this).SendMessage(&msg);
		fWriteEnding = false;
		
	} else {
	
		fUndo.EndUpdate();
	
	}
	
	status_t err = fDataLock.WriteUnlock() ? B_OK : B_ERROR;
	
	return err;
}

void ResWindow::EntryChanged(ResourceEntry* e)
{
	if( e ) {
		PRINT(("Adding item %p (%ld, %lx, %s, %s) to change list.\n",
				e, e->ID(), e->Type(), e->Name(), e->Symbol()));
		fChangedItems.push_back(BResourceHandle(e));
		SetDirty(true);
		ResEntry* re = dynamic_cast<ResEntry*>(e);
		if( re && re->IsBody() ) fBodyChanged = true;
	}
}

bool ResWindow::AssertReadLock(const char* context) const
{
	if( (fReadLocks <= 0 || !fDataLock.IsReadLocked())
			&& !fDataLock.IsWriteLocked() ) {
		BString msg(context);
		msg << ": Must be called with read or write lock.";
		debugger(msg.String());
		return false;
	}
	return true;
}

bool ResWindow::AssertWriteLock(const char* context) const
{
	if( !fDataLock.IsWriteLocked() ) {
		BString msg(context);
		msg << ": Must be called with write lock.";
		debugger(msg.String());
		return false;
	}
	return true;
}

bool ResWindow::PerformAdd(BResourceHandle item)
{
	fItems.push_back(item);
	const ResEntry* e = dynamic_cast<const ResEntry*>(ReadItem(item));
	if( e && e->IsAttribute() ) {
		size_t i, j;
		for( i=j=0; i<fDeletedAttrs.size(); i++ ) {
			if( j < i ) fDeletedAttrs[j] = fDeletedAttrs[i];
			if( fDeletedAttrs[i] != item ) j++;
		}
		if( i != j ) {
			fDeletedAttrs.resize(j);
		}
	}
	SetDirty(true);
	return true;
}

bool ResWindow::PerformRemove(BResourceHandle item)
{
	size_t i, j;
	for( i=j=0; i<fItems.size(); i++ ) {
		if( j < i ) fItems[j] = fItems[i];
		if( fItems[i] != item ) j++;
	}
	
	const ResEntry* e = dynamic_cast<const ResEntry*>(ReadItem(item));
	if( e && e->IsAttribute() ) {
		fDeletedAttrs.push_back(item);
	}
	
	if( i != j ) {
		fItems.resize(j);
		fRemovedItems.push_back(item);
		SetDirty(true);
		return true;
	}
	
	return false;
}

void ResWindow::SyncResList()
{
	bool sel_changed = false;
	bool added_sel = false;
	size_t i;
	
	BResourceAddonBase acc(BResourceAddonArgs(*this));
		
	vector<ResRow*> tmpRows;
	vector<uint32> tmpChanges;
	
	if( ReadLockCollection() == B_OK ) {
	
		// Take any removed items out of the list view.
		for( i=0; i<fRemovedItems.size(); i++ ) {
			ResourceEntry* e = const_cast<ResourceEntry*>(
					dynamic_cast<const ResourceEntry*>(ReadItem(fRemovedItems[i])));
			ResRow* row = e ? (ResRow*)(e->Owner()) : 0;
			if( row ) {
				if( row->WindowEditor().IsValid() ) {
					BMessage msg(B_QUIT_REQUESTED);
					row->WindowEditor().SendMessage(&msg);
				}
				e->SetOwner(0);
				fResList->RemoveRow(row);
				delete row;
			}
		}
		fRemovedItems.resize(0);
		
		// Find any items that have been newly added.
		for( i=0; i<fItems.size(); i++ ) {
			ResEntry* e = const_cast<ResEntry*>(
					dynamic_cast<const ResEntry*>(ReadItem(fItems[i])));
			ResRow* row = e ? (ResRow*)(e->Owner()) : 0;
			if( !row ) {
				row = new ResRow(e);
				e->SetOwner(row);
				tmpRows.push_back(row);
				tmpChanges.push_back(e->MakeSelected());
				e->SetMakeSelected(false);
			}
		}
		
		ReadUnlockCollection();
	}
	
	// Add any new rows to the list view.  Must be done outside of
	// a read lock, because the creation of mini editors for these
	// items may want to get their own read locks.
	for( i=0; i<tmpRows.size(); i++ ) {
		ResRow* row = tmpRows[i];
		if( row ) {
			fResList->AddRow(row);
			UpdateRow(acc, row, B_RES_ALL_CHANGED);
			if( tmpChanges[i]) {
				if( !added_sel ) {
					fResList->DeselectAll();
					fChangedSelection = false;
				}
				added_sel = true;
				fResList->AddToSelection(row);
			}
		}
	}
	tmpRows.resize(0);
	tmpChanges.resize(0);
	
	ResRow* row = dynamic_cast<ResRow*>(fResList->CurrentSelection());
	const bool multisel = row && fResList->CurrentSelection(row);
	
	if( ReadLockCollection() == B_OK ) {
	
		if( fChangedSelection ) {
			// Check if currently selected item has changed.
			BResourceHandle shown;
			if( row ) shown = *row;
			BResourceHandle new_sel = CurrentItem();
			ResourceEntry* sel_e = const_cast<ResourceEntry*>(
					dynamic_cast<const ResourceEntry*>(ReadItem(new_sel)));
			
			if( sel_e && shown != new_sel ) {
				// If a new item was selected, change the list view to it.
				ResRow* row = (ResRow*)(sel_e->Owner());
				if( row ) {
					fResList->DeselectAll();
					fResList->AddToSelection(row);
				}
				sel_changed = true;
				
			} else if( !sel_e && shown != new_sel && !multisel && !added_sel ) {
				// If an item is no longer selected, and the list view is
				// not currently displaying a multiple selection, then
				// deselect it.	
				fResList->DeselectAll();
				sel_changed = true;
			}
			
			if( added_sel ) {
				//fResList->ScrollToSelection();
			}
		}
		
		// Copy array of changed items.
		for( i=0; i<fUpdatedItems.size(); i++ ) {
			ResEntry* e = const_cast<ResEntry*>(
					dynamic_cast<const ResEntry*>(ReadItem(fUpdatedItems[i])));
			ResRow* row = e ? (ResRow*)(e->Owner()) : 0;
			if( row ) {
				tmpRows.push_back(row);
				tmpChanges.push_back(fUpdatedChanges[i]);
			}
		}
		fUpdatedItems.resize(0);
		fUpdatedChanges.resize(0);
		
		ReadUnlockCollection();
	}
	
	// Update list view with any changed items.
	for( i=0; i<tmpRows.size(); i++ ) {
		ResRow* row = tmpRows[i];
		if( row ) UpdateRow(acc, row, tmpChanges[i]);
	}
	
	fController.UpdateEditor( (row && !multisel)
							  ? *(BResourceHandle*)row
							  : BResourceHandle() );
	if( sel_changed ) UpdateControlState();
	fChangedSelection = false;
}

/***
**** ResWindow private implementation.
***/

void ResWindow::UpdateControlState()
{
	if( ReadLockCollection() == B_OK ) {
		ResRow* sel = dynamic_cast<ResRow*>(fResList->CurrentSelection());
		const bool multi = sel != 0 && fResList->CurrentSelection(sel) != 0;
		if(sel)
		{
			fRemoveButton->SetEnabled(true);
			fDupItem->SetEnabled(true);
			fCutItem->SetEnabled(true);
			fCopyItem->SetEnabled(true);
			fPasteItem->SetEnabled(true);
			fClearItem->SetEnabled(true);
			fEditInplaceItem->SetEnabled(!multi);
			fEditWindowItem->SetEnabled(true);
		}
		else
		{
			fRemoveButton->SetEnabled(false);
			fDupItem->SetEnabled(false);
			fCutItem->SetEnabled(true);
			fCopyItem->SetEnabled(true);
			fPasteItem->SetEnabled(true);
			fClearItem->SetEnabled(true);
			fEditInplaceItem->SetEnabled(false);
			fEditWindowItem->SetEnabled(false);
		}
		ReadUnlockCollection();
	}
}

void ResWindow::SortItems()
{
	// No longer needed with change to column list view.
}

void ResWindow::UpdateRow(const BResourceAddonBase& acc, ResRow* row,
						  uint32 changes, bool force)
{
	// change list item entry
	if( row ) {
		bool changed = row->Update(acc, changes, force);
		if( changed || force ) fResList->UpdateRow(row);
		SortItems();
	}
}

void ResWindow::UpdateEditorVisibility()
{
	bool isShown = fContainer->Parent() ? true : false;
	bool nowShown = fDivider->Value() ? true : false;
	if( isShown != nowShown ) {
		if( nowShown ) {
			fController.ForgetEditor();
			fDivider->SetResizingMode(B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP);
			fColumns->SetResizingMode(B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP);
			ResizeTo(Frame().Width(),
					 (fDivider->Frame().bottom+fContainer->Frame().Height()+fRoot->Frame().top) + SPACER);
			fContainer->ResizeTo(fRoot->Frame().Width(),
								 fContainer->Frame().Height());
			fContainer->MoveTo(0, fDivider->Frame().bottom+SPACER);
			fRoot->AddChild(fContainer);
			fController.UpdateEditor(fCurItem, fController.T_UPDATE_NOW);
		} else {
			fContainer->RemoveSelf();
			ResizeTo(Frame().Width(),
					 fRoot->Frame().top+fDivider->Frame().bottom);
			fDivider->SetResizingMode(B_FOLLOW_LEFT_RIGHT|B_FOLLOW_BOTTOM);
			fColumns->SetResizingMode(B_FOLLOW_ALL);
		}
	}
}
			
void ResWindow::RequestMerge()
{
	BMessenger self(this);

	if(! fMergePanel)
	{
		fMergePanel = new BFilePanel(B_OPEN_PANEL, &self);//, &dirEntry);
		fMergePanel->Window()->SetTitle("Merge From File");
		fMergePanel->SetMessage(new BMessage(kMergeResources));
	}

	fMergePanel->Show();
}
