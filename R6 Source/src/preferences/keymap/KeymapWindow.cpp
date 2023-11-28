//#define DEBUG 1
//-------------------------------------------------------------------
//	
//	KeymapWindow.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1994-95 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include "KeymapWindow.h"

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include <Button.h>
#include <ClassInfo.h>
#include <Debug.h>
#include <Path.h>
#include <FilePanel.h>
#include <Directory.h> 
#include <FindDirectory.h>
#include <Volume.h>
#include <fs_attr.h>
#include <VolumeRoster.h>
#include <Entry.h>
#include <ScrollView.h>
#include <StringView.h>
#include <Directory.h>
#include <shared_fonts.h>
#include <Box.h>
#include <byteorder.h>

#include "KeymapView.h"

const uint32 SYSTEM_LIST_SELECT_MSG = 'SSEL';
const int32 USER_LIST_SELECT_MSG = 'USEL';
const int32 REVERT_MSG = 'RVRT';
const int32 SANE_BUFFER_SIZE = 5000;

extern unsigned char	hand_cursor;

const BRect             kSystemLabelRect(10, 18, 85, 33);
const BRect             kSystemListViewRect(12, 35, SIDEPANELWIDTH - 18 - 2*WINDBORDER , 105);
const BRect             kUserLabelRect(10, 113, 85, 128);
const BRect             kUserListViewRect(12, 130, SIDEPANELWIDTH - 18 - 2*WINDBORDER, 200);

// Button Stuff
const char*             kButtonBoxName = "ButtonBox";
const float             kButtonWidth = 50;
const float             kButtonSpacing = 5;

const char*             kRevertButtonLbl = "Revert";
const char*             kUseButtonLbl = "Use";

const char*             kSidePanelName = "Maps";
const char*             kSystemLabelText = "System";
const char*             kUserLabelText = "User";

const char*             kSystemListViewLbl = "System ListView";
const char*             kUserListViewLbl = "User ListView";
const char*             kSystemScrollerLbl = "System Scroller";
const char*             kUserScrollerLbl = "User Scroller";
const char*             kbackViewName = "Side Panel View";
const char*             kUserDefaultKeymapFileName = "/Key_map";
const char*             kKeymapSettingsDirectory = "Keymap";
const char*             kDefaultKeymapName = "(Current)";
char*                   gDefaultKeymapFilePath = NULL;
const rgb_color         kViewGray = {200, 200, 200,0};
//====================================================================

TKeymapWindow	*keymapWind;

TKeymapWindow::TKeymapWindow(BEntry* entry, bool readOnly, BRect rect, char *title)
	: BWindow(rect, title, B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE) 
{
	float		height;
	BRect		textRect;
	BRect		tempRect;
	BRect         winRect;
	entry_ref     ref;
	BPath         path;
	BMessenger    self(this);
	
	keymapWind = this;
	fHaveFile = false;
	fOpenPanel = NULL;
	
	fKeyMap = (key_map*)malloc(sizeof(key_map));
	
	// Create the save panel.
	fSavePanel = new BFilePanel(B_SAVE_PANEL, &self);
	fOpenPanel = new BFilePanel(B_OPEN_PANEL);
	if (GetSettingsPath(&path) == B_NO_ERROR) {
		BDirectory dir;
		if (dir.SetTo(path.Path()) == B_NO_ERROR) {
			//printf("Setting directory\n");
			fSavePanel->SetPanelDirectory(&dir);
			fOpenPanel->SetPanelDirectory(&dir);
		}
	}
	
	// Pick some arbitrary starting size for buf, to be
	// used when saving.
	fKeyMapBufferSize = 3000;
	fKeyMapBuffer = (char*)malloc(3000);
	
	BuildMenuBar();	
	
	height = fMenuBar->Bounds().bottom + 1;
	ResizeBy(0, height - KEYSMENU);
	Unlock();
	
	winRect =  Bounds();
	winRect.InsetBy(-1, -1);
	fBackView = new BBox(winRect);
	AddChild(fBackView);
	
	// Add the buttons
	AddButtons();
	
	// Add the keymap view.
	tempRect.Set(0+SIDEPANELWIDTH, height, WINDBORDER + KEYMAPWIDTH+ SIDEPANELWIDTH,
	height + WINDBORDER + KEYMAPHEIGHT );
	fKeymapView = new TKeymapView(tempRect, "KeymapView");
	fKeymapView->SetExtKeyMap(&eKeyMap);
	
	Lock();
	fBackView->AddChild(fKeymapView);
	
	//Bounds().PrintToStream();
	//WAA - delete these
	tempRect.Set(EDITLEFT, EDITTOP, EDITRIGHT, EDITBOTTOM);
	textRect.Set(1, 1, (EDITRIGHT - EDITLEFT) - 1, EDITBOTTOM - EDITTOP + 1);
	//fKeymapView->fTextView = new TEditTextView(fKeymapView, tempRect, "", textRect, B_FOLLOW_NONE, 0);
	//fKeymapView->AddChild(fKeymapView->fTextView);
	Unlock();
	
	// Add the side panel stuff.
	BRect sidePanelRect = winRect;
	sidePanelRect.right = SIDEPANELWIDTH;
	sidePanelRect.top = height + WINDBORDER + 2;
	sidePanelRect.bottom -= WINDBORDER;
	sidePanelRect.left = WINDBORDER;
	BBox* sidePanelBox = new BBox(sidePanelRect, kSidePanelName);
	sidePanelBox->SetLabel(kSidePanelName);
	// Add label for first listbox.
	BStringView* systemLabel = new BStringView(kSystemLabelRect, kSystemLabelText, kSystemLabelText);
	sidePanelBox->AddChild(systemLabel);
	
	BStringView* userLabel = new BStringView(kUserLabelRect, kUserLabelText, kUserLabelText);
	sidePanelBox->AddChild(userLabel);
	
	fSystemListView = new BListView(kSystemListViewRect, kSystemListViewLbl);
	BScrollView* scroller = new BScrollView(kSystemScrollerLbl, fSystemListView, 
	B_FOLLOW_LEFT| B_FOLLOW_TOP,
	0, false, true); 
	fSystemListView->SetSelectionMessage(new BMessage(SYSTEM_LIST_SELECT_MSG));
	sidePanelBox->AddChild(scroller);
	
	// Add the user list of keymap files.
	fUserListView = new BListView(kUserListViewRect, kUserListViewLbl);
	fUserListView->SetSelectionMessage(new BMessage(USER_LIST_SELECT_MSG));
	scroller = new BScrollView(kUserScrollerLbl, fUserListView, 
	B_FOLLOW_LEFT| B_FOLLOW_TOP,
	0, false, true); 
	sidePanelBox->AddChild(scroller);
	
	fBackView->AddChild(sidePanelBox);
	
	// Create the global that holds the path to the user's default keymap file.
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_NO_ERROR) {
		int   pathLen = strlen(path.Path());
		int   totalLen = pathLen + strlen(kUserDefaultKeymapFileName) + 1;
		gDefaultKeymapFilePath = (char*)malloc(totalLen);
	
		strcpy(gDefaultKeymapFilePath, path.Path());
		strcpy(gDefaultKeymapFilePath+pathLen, kUserDefaultKeymapFileName);
		gDefaultKeymapFilePath[totalLen-1] = '\0';
		//printf("user map file is %s\n", gDefaultKeymapFilePath); 
	}
	
	fDefaultFileEntry = new BEntry(gDefaultKeymapFilePath);
	
	if (entry == NULL)
		entry = fDefaultFileEntry;
	
	if (entry->GetRef(&ref) == B_NO_ERROR)
		OpenFile(ref, readOnly);
	
	AddKeymaps();
	cutItem->SetTarget(fKeymapView->fTextView);
	copyItem->SetTarget(fKeymapView->fTextView);
	pasteItem->SetTarget(fKeymapView->fTextView);
	
	SetPulseRate(100);
}

//--------------------------------------------------------------------

TKeymapWindow::~TKeymapWindow()
{
	if (fKeyMapBuffer)
		free(fKeyMapBuffer);
}

void 
TKeymapWindow::AddButtons()
{
	BRect winRect = Bounds();
	BRect r;
	
	r.right = winRect.right - WINDBORDER;
	r.bottom = winRect.bottom - WINDBORDER;
	float width = be_plain_font->StringWidth(kUseButtonLbl);
	if (width < 75) width = 75;
	r.left = r.right - width;
	r.top = r.bottom - 24;
	BButton *useButton = new BButton(r, kUseButtonLbl, kUseButtonLbl,
		new BMessage(MENU_SET_SYS));
	fBackView->AddChild(useButton);
	
	r.right = r.left - 10;
	width = be_plain_font->StringWidth(kRevertButtonLbl);
	if (width < 75) width = 75;
	r.left = r.right - width;
	BButton *revertButton = new BButton(r, kRevertButtonLbl, kRevertButtonLbl,
		new BMessage(REVERT_MSG));
	fBackView->AddChild(revertButton);
}

void
TKeymapWindow::BuildMenuBar()
{
  font_family	fontName;
  uint32	fontFlags;
  uint32	fontStatus;
  BMenu*	theMenu;
  BRect         tempRect;
  int32		numFonts;
  long		loop;

  tempRect.Set(0, 0, 32767, KEYSMENU);
  // Create the menu bar.
  fMenuBar = new BMenuBar(tempRect, "MB");
  fMenuBar->SetBorder(B_BORDER_FRAME);
  
  be_plain_font->GetFamilyAndStyle(&fFontName, 0L);
  
  // Create the File menu
  theMenu = new BMenu("File");
  fMenuBar->AddItem(theMenu);
#if 0
  BMenuItem* item = new BMenuItem("About Keymap...", new BMessage(B_ABOUT_REQUESTED));
  theMenu->AddItem(item);
  item->SetTarget(be_app);
#endif
  theMenu->AddItem(new BMenuItem("Open...", new BMessage(MENU_OPEN), 'O'));
  theMenu->AddItem(new BSeparatorItem());
  theMenu->AddItem(new BMenuItem("Save", new BMessage(MENU_SAVE), 'S'));
  theMenu->AddItem(new BMenuItem("Save As...", new BMessage(MENU_SAVE_AS)));
  theMenu->AddItem(new BSeparatorItem());
  theMenu->AddItem(new BMenuItem("Quit",new BMessage(B_QUIT_REQUESTED), 'Q'));
  SetMenuEnable(MENU_SAVE, false);
  
  // Create the Edit Menu
  theMenu = new BMenu("Edit");
  fMenuBar->AddItem(theMenu);
  
  theMenu->AddItem(new BMenuItem("Undo", new BMessage(MENU_UNDO), 'Z'));
  theMenu->AddItem(new BSeparatorItem());
  theMenu->AddItem(cutItem = new BMenuItem("Cut", new BMessage(B_CUT), 'X'));
  theMenu->AddItem(copyItem = new BMenuItem("Copy", new BMessage(B_COPY), 'C'));
  theMenu->AddItem(pasteItem = new BMenuItem("Paste", new BMessage(B_PASTE), 'V'));
  theMenu->AddItem(new BMenuItem("Clear", new BMessage(MENU_CLEAR)));
  theMenu->AddItem(new BSeparatorItem());
  theMenu->AddItem(new BMenuItem("Select All", new BMessage(MENU_SELECT_ALL), 'A'));
  SetMenuEnable(MENU_UNDO, false);
  
  // Setup the menu item that contains all the fonts
  theMenu = new BMenu("Font");
  theMenu->SetRadioMode(true);
  fMenuBar->AddItem(theMenu);
  numFonts = count_font_families();
  for (loop = 0; loop < numFonts; loop++) 
    {
      fontStatus = get_font_family(loop, &fontName, &fontFlags);
      theMenu->AddItem(new BMenuItem(fontName, new BMessage(MENU_FONT)));
    }
  
  fMenuBar->FindItem(fFontName)->SetMarked(true);	
  
  /*
    theMenu = new BMenu("Symbol Set");
    theMenu->SetRadioMode(true);
    fMenuBar->AddItem(theMenu);
    numFonts = count_symbol_sets();
    for (loop = 0; loop < numFonts; loop++) {
    get_symbol_set_name(loop, (symbol_set_name*)&fontName);
    theMenu->AddItem(new BMenuItem(fontName, new BMessage(MENU_SYMBOL)));
    }
    fMenuBar->FindItem(fSymbolSet)->SetMarked(true);
    */
  
  
  Lock();
  AddChild(fMenuBar);
}
//--------------------------------------------------------------------

bool TKeymapWindow::FilterMessageDropped(BMessage* drop, BPoint ,
	BView **target)
{
	if ((*target == fKeymapView->fTextView) && (drop->what == B_KEY_DOWN)) {
		if (drop->HasData("text", B_ASCII_TYPE)) {
			ssize_t	len;
			char	*text;
			drop->FindData((const char*)"text", (type_code)B_ASCII_TYPE,(const void**) &text, &len);
			fKeymapView->fTextView->Insert(text, len);
			return false;
		}
	}

	return true;
}

//--------------------------------------------------------
void
TKeymapWindow::Save(BFile *mapFile)
{
	int32   buffer_size;

	PackKeyMap();
	PRINT(("TKeymapWindow::Save - ENTER\n"));
#if 0
	// Create a copy of the current key_map structure
	memcpy(&kMap, fKeymap, sizeof(struct key_map));

	// Write out the offset tables one by one,
	// and adjust the offsets in our copy of the key_map

	mapFile->Seek(sizeof(struct key_map), SEEK_SET);

	// Write a 16-bit 0 byte to start for easy reference
	mapFile->Write(&zero, 2);
	offset = 2;

	// Now seek back to the beginning and write out the modified
	// key_map structure

	mapFile->Seek(0, SEEK_SET);

	mapFile->Write(&kMap.version, sizeof(kMap.version));
	mapFile->Write(&kMap.caps_key, sizeof(kMap.caps_key));
	mapFile->Write(&kMap.scroll_key, sizeof(kMap.scroll_key));
	mapFile->Write(&kMap.num_key, sizeof(kMap.num_key));
	mapFile->Write(&kMap.left_shift_key, sizeof(uint32));
	mapFile->Write(&kMap.right_shift_key, sizeof(uint32));
	mapFile->Write(&kMap.left_command_key, sizeof(uint32));
	mapFile->Write(&kMap.right_command_key, sizeof(uint32));
	mapFile->Write(&kMap.left_control_key, sizeof(uint32));
	mapFile->Write(&kMap.right_control_key, sizeof(uint32));
	mapFile->Write(&kMap.left_option_key, sizeof(uint32));
	mapFile->Write(&kMap.right_option_key, sizeof(uint32));
	mapFile->Write(&kMap.menu_key, sizeof(uint32));
	mapFile->Write(&kMap.lock_settings,sizeof(uint32));


	mapFile->Write(&kMap.control_map, sizeof(kMap.control_map));
	mapFile->Write(&kMap.option_caps_shift_map, sizeof(kMap.option_caps_shift_map));
	mapFile->Write(&kMap.option_caps_map, sizeof(kMap.option_caps_map));
	mapFile->Write(&kMap.option_shift_map, sizeof(kMap.option_shift_map));
	mapFile->Write(&kMap.option_map, sizeof(kMap.option_map));
	mapFile->Write(&kMap.caps_shift_map, sizeof(kMap.caps_shift_map));
	mapFile->Write(&kMap.caps_map, sizeof(kMap.caps_map));
	mapFile->Write(&kMap.shift_map, sizeof(kMap.shift_map));
	mapFile->Write(&kMap.normal_map, sizeof(kMap.normal_map));
	mapFile->Write(&kMap.acute_dead_key, sizeof(kMap.acute_dead_key));
	mapFile->Write(&kMap.grave_dead_key, sizeof(kMap.grave_dead_key));
	mapFile->Write(&kMap.circumflex_dead_key, sizeof(kMap.circumflex_dead_key));
	mapFile->Write(&kMap.dieresis_dead_key, sizeof(kMap.dieresis_dead_key));
	mapFile->Write(&kMap.tilde_dead_key, sizeof(kMap.tilde_dead_key));
	mapFile->Write(&kMap.acute_tables, sizeof(kMap.acute_tables));
	mapFile->Write(&kMap.grave_tables, sizeof(kMap.grave_tables));
	mapFile->Write(&kMap.circumflex_tables, sizeof(kMap.circumflex_tables));
	mapFile->Write(&kMap.dieresis_tables, sizeof(kMap.dieresis_tables));
	mapFile->Write(&kMap.tilde_tables, sizeof(kMap.tilde_tables));
#endif
	mapFile->Seek(0, SEEK_SET);
	mapFile->Write(fKeyMap, sizeof(struct key_map));
	// Write out the buffer.
	PRINT(("TKeymapWindow::Save - writing buffer, length %ld\n", fKeyMapBufferPos));
	buffer_size = B_HOST_TO_BENDIAN_INT32(fKeyMapBufferPos);
	mapFile->Write(&buffer_size, sizeof(int32));
	mapFile->Write(fKeyMapBuffer, fKeyMapBufferPos);
	AddKeymaps();
}

//--------------------------------------------------------------------

void TKeymapWindow::MessageReceived(BMessage* theMessage)
{
  BMenuItem	*item;
  BRect		tempRect;
  
  PRINT(("TKeymapWindow::MessageReceived - ENTER\n"));
  
  switch(theMessage->what) 
    {
    case MENU_OPEN:
      if (SaveChanges())
	{
	  // Don't delete, it's done automatically
	  fOpenPanel->Show();
	} 
      break;
      
    case B_SAVE_REQUESTED:
      {
	PRINT(("KeymapWindow::MessageReceived - SaveRequested\n"));
	entry_ref dir;
	theMessage->FindRef("directory", &dir);
	
	const char *name = NULL;
	theMessage->FindString("name", &name);
	
	SaveRequested(&dir, name);
      }
    break;
    
    case MENU_SAVE:
      if (fHaveFile) 
	{
	  PackKeyMap();
	  Save(fMapFile);
	  fKeymapView->fDirty = false;
	  SetMenuEnable(MENU_SAVE, false);
	} 
      else
	{
	  fSavePanel->Show();
	}
      break;
    case MENU_SAVE_AS:
      {
	BMessenger self(this);
	BPath      path;
	
	//printf("Doing save as\n");

	fSavePanel->Show();
	break;
      }
    case MENU_SET_SYS:
      {
	BFile file;
	
	if (file.SetTo(gDefaultKeymapFilePath, O_RDWR) == B_NO_ERROR)
	  {
	    Save(&file);
	    fKeymapView->fDirty = false;
	    fSystemListView->DeselectAll();
	    fUserListView->Select(0);
	    _restore_key_map_();
	  }
	else
	  BusyFile();
      }
    break;

    case MENU_QUIT:
      Close();
      break;
      
    case MENU_UNDO:
      fKeymapView->Undo();
      break;
    case MENU_CLEAR:
      fKeymapView->fTextView->Delete();
      break;
    case MENU_SELECT_ALL:
      fKeymapView->fTextView->Select(0, fKeymapView->fTextView->TextLength());
      break;
      
    case MENU_FONT:
      void *ptr;
      BArchivable	*oo;
      theMessage->FindPointer("source", &ptr);
      oo = (BArchivable *) ptr;
      item = cast_as(oo,  BMenuItem);
      ASSERT(item);
      if (strcmp(fFontName, item->Label())) 
	{
	  strcpy(fFontName, item->Label());
	  // WAA - keymapView->fTextView->SetFontName(fFontName);
	  //	keymapView->fTextView->SetFont(be_plain_font);
	  tempRect.Set(1, 1, (EDITRIGHT - EDITLEFT) - 1, EDITBOTTOM - EDITTOP + 1);
	  //	keymapView->fTextView->BTextView::Draw(tempRect);
	  fKeymapView->ReDraw();
	}
      break;
    case REVERT_MSG:
      //printf("Got revert message\n");
      if (fMapFile != NULL) 
	{
	  fMapFile->Seek(0, SEEK_SET);
	  OpenFile(fMapFile, fKeymapView->fReadOnly);
	}
      //else printf("fMapFile NULL\n");
      break;
    case SYSTEM_LIST_SELECT_MSG:
    case USER_LIST_SELECT_MSG:
      {
	bool        readOnly = false;
	int32       selection;
	BList*      list = &fUserEntryList;
	BListView*  listView = fUserListView;
	BListView*  otherView = fSystemListView;

	//printf("Got listbox click\n");
	if (theMessage->what == SYSTEM_LIST_SELECT_MSG)
	  {
	    readOnly = true;
	    list = &fSystemEntryList;
	    listView = fSystemListView;
	    otherView = fUserListView;
	  }
	selection = listView->CurrentSelection();
	if (selection > -1 && ((selection != fLastSelection) ||
	    (listView != fLastView)))
	  {
	    BEntry*    entry = (BEntry*)list->ItemAt(selection);
	    entry_ref  ref;
	    int32      otherSel = otherView->CurrentSelection();
	    
	    //printf("Selection is valid, %d in %d\n", selection, list->CountItems());
	    if (otherSel != -1)
	      {
		//printf("Deselecting other\n");
		otherView->DeselectAll();
		//printf("Done deselecting\n");
	      }
	    if (entry->GetRef(&ref) == B_NO_ERROR)
	      {
		//printf("Opening file\n");
	       if (OpenFile(ref, readOnly))
		 {
		   fLastView = listView;
		   fOtherView =  otherView;  
		   fLastSelection = selection;
		 }
	      }
	    else 
	      {
		fLastView->Select(fLastSelection);
	      }
	  }
      }
      break;

    default:
      //printf("TKeymapWindow::MessageReceived: %d\n", theMessage->what);
      BWindow::MessageReceived(theMessage);
      break;
    }
}

//--------------------------------------------------------------------

bool TKeymapWindow::QuitRequested()
{
	if (!SaveChanges()) {
		return false;
	}
	
	BRect r(Frame());
	BPath  path;
	BPoint win_pos(r.LeftTop());
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, true) == B_NO_ERROR) {
		if (path.Append("Keymap_Data") == B_NO_ERROR) {
			int	 ref;
			if ((ref = creat(path.Path(), 0777)) >= 0) {
				write(ref, &win_pos, sizeof(BPoint));
				close(ref);
			}
		}
	}
	
	delete fSavePanel;
	delete fOpenPanel;
	
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

//--------------------------------------------------------------------

void TKeymapWindow::WindowActivated(bool activFlag)
{
	if ((!activFlag) && (fKeymapView))
		be_app->SetCursor(B_HAND_CURSOR);
}

//--------------------------------------------------------------------

void TKeymapWindow::BusyFile()
{
	char	busyFile[48] = "Sorry: That file is busy right now!";
	BAlert	*myAlert;

	myAlert = new BAlert("", busyFile, "Bummer");
	myAlert->Go();
	if (fKeymapView)
		fKeymapView->Pulse();
}

//--------------------------------------------------------------------

void TKeymapWindow::CloseFile(bool SaveChanges)
{
	if (fHaveFile) {
		if (SaveChanges) 
			Save(fMapFile);
		fHaveFile = false;
	}
	fKeymapView->fDirty = false;
	SetMenuEnable(MENU_SAVE, false);
	fKeymapView->fUndoFlag = false;
	SetMenuEnable(MENU_UNDO, false);
}

//--------------------------------------------------------------------

void TKeymapWindow::SaveRequested(entry_ref *directory, const char* name)
{
  BFile			*theFile = NULL;
  BDirectory		theDir(directory);
  
  PRINT(("TKeymapWindow::SaveRequested - ENTER\n"));
  
  theFile = new BFile();
  if (theDir.CreateFile(name, theFile) == B_NO_ERROR) 
    {
      //theFile->SetTypeAndApp('kmap', 'KBRD');
      PackKeyMap();
      Save(theFile);
      fKeymapView->fDirty = false;
      SetMenuEnable(MENU_SAVE, false);
    }
  else 
    {
      BusyFile();
      delete theFile;
      theFile = NULL;
    }
  
  if (theFile) 
    {
      if (fHaveFile)
	delete fMapFile;
      
      fMapFile = theFile;
      fHaveFile = true;
    }
}

//--------------------------------------------------------------------

bool
TKeymapWindow::OpenFile(entry_ref theRef, bool readOnly)
{
  uint32                openMode = B_READ_WRITE;
  BEntry                entry(&theRef);
  bool                  result;

  //printf("OpenFile- ENTER\n");
  if (readOnly) openMode = B_READ_ONLY;
  BFile*  theFile = new BFile(&theRef, openMode);
  
  //printf("OpenFile - calling SaveChanges\n");
  result = SaveChanges();
  if (result) 
    {
      //printf("OpenFile - calling CloseFile\n");
      CloseFile(false);
      OpenFile(theFile, readOnly);  
    }
  else
    {
      fOtherView->DeselectAll();
      fLastView->Select(fLastSelection);
      delete theFile;
      if (fKeymapView)
	fKeymapView->Pulse();
    }
  return result;
}

void
TKeymapWindow::OpenFile(BFile* theFile, bool readOnly)
{
  //printf("OpenFile - doing init check\n");
  if (theFile->InitCheck() == B_NO_ERROR)
    {  
      //theFile->GetTypeAndApp(&type, &creator);
      //if (type == 'kmap') {
      fMapFile = theFile;
      fHaveFile = true;
      //printf("Reading new data\n");
      ReadKeyMapData(theFile, &fKeyMap, &fKeyMapBufferSize, &fKeyMapBuffer);
  
      fKeymapView->fUndoFlag = false;
      fKeymapView->fReadOnly = readOnly;
      fKeymapView->fDirty = false;
      SetMenuEnable(MENU_UNDO, false);
      ExpandKeyMap();
      fKeymapView->SetExtKeyMap(&eKeyMap);
      fKeymapView->ReDraw();
    }
  else 
    {	
      BusyFile();
      delete theFile;
    }
}

bool
TKeymapWindow::ReadKeyMapData(BFile* file, struct key_map** keyMap, 
	int32* keymapBufferSize, char** keymapBuffer)
{
	off_t		size;
	status_t	result;
	BAlert		*myAlert;
	key_map		newKeymap;
	int32		newSize;
	char*		newBuffer;
	
	result = file->GetSize(&size);
	//printf("Result of GetSize is %s\n", strerror(result));
	if (result != B_NO_ERROR)
		goto BADFILE;
	
	if (size < sizeof(struct key_map) + 4) {
		//printf("file size %Ld smaller than keymap size %Ld\n", size, sizeof(struct key_map));
		goto BADFILE;
	}
	
	size = file->Read(&newKeymap, sizeof(key_map));
	if (size != sizeof(key_map))  {
		//printf("1st read fail (%Ld/%ld)\n", size, sizeof(key_map));
		goto BADFILE;
	}
	
	size = file->Read(&newSize, sizeof(int32));
	newSize = B_BENDIAN_TO_HOST_INT32(newSize);
	if (size != sizeof(int32) || newSize < 0 || newSize > SANE_BUFFER_SIZE) {
		PRINT(("2nd read fail, %ld (%ld/%ld)\n", newSize, size, sizeof(int32)));
		goto BADFILE;
	}
	
	//printf("Read - Reading buffer of length %ld\n", newSize);
	newBuffer = (char*)malloc(newSize);
	if (newBuffer == NULL)
		goto BADFILE;
		
	size = file->Read(newBuffer, newSize);
	if (size != newSize){
		free(newBuffer);
		//printf("3rd read fail (%Ld/%ld)\n", size, newSize);
		goto BADFILE;
	} else {
		*keymapBufferSize = newSize;
		memcpy(*keyMap, &newKeymap, sizeof(key_map));
		if (*keymapBuffer)
			free (*keymapBuffer);
		*keymapBuffer = newBuffer;
	}
	
	return true;
	
	BADFILE:
	//printf("OpenFile - bad file!\n");
	
	myAlert = new BAlert("Bad File!","This file is either not a keymap file or is corrupt", "Sorry");
	UpdateIfNeeded();
	myAlert->Go();
	
	return false;
}

//--------------------------------------------------------------------

bool TKeymapWindow::SaveChanges()
{
  char		save[32] = "Save changes?";
  BAlert	*myAlert;
  long		result;
  //printf("SaveChanges - ENTER\n");
  if ((fKeymapView) && (fKeymapView->fDirty)) 
    {
      UpdateIfNeeded();
      be_app->SetCursor(B_HAND_CURSOR);
      myAlert = new BAlert("",save, "Cancel", "No", "OK");
      myAlert->SetShortcut(0, 'c');
      myAlert->SetShortcut(1, 'n');
      myAlert->SetShortcut(2, 'o');
      result = myAlert->Go();
      if (fKeymapView)
	fKeymapView->Pulse();
      
      switch (result) 
	{
	case 0:
	  return false;
	case 1:
	  //printf("SaveChanges - closing file w/o save\n");
	  CloseFile(false);
	  break;
	case 2:
	  if (fHaveFile)
	    {
	      //printf("SaveChanges - closing file w/save\n");
	      CloseFile(true);
	    }
	  else 
	    {
	      fOpenPanel->Show();
	      return false;
	    }
	  break;
	}
    }
  else
    CloseFile(false);
  return true;
}

//--------------------------------------------------------------------

void TKeymapWindow::SetMenuEnable(long theItem, bool theState)
{
	BMenuItem *item = fMenuBar->FindItem(theItem);
	if (!item)
		return;
	item->SetEnabled(theState);
}

//--------------------------------------------------------------------

void TKeymapWindow::SetMenuItemMark(long theItem, char theState)
{
	BMenuItem *item = fMenuBar->FindItem(theItem);
	if (!item)
		return;
	item->SetMarked(theState);
}

//--------------------------------------------------------------------

void TKeymapWindow::LoadCurrentKeyMap() {
}

//--------------------------------------------------------------------

void 
TKeymapWindow::ExpandMap(char **list_key, int32 *list_offset, int32 count) 
{
  /* 
     list_offset is a set of pointers to int32s that represent offsets into
     the buffer containing the pascal strings associated with the keys.
     list_key is a pointer to a list of char*'s that should each point to
     the pascal string associated with the key. count is just the count man!
     */
  int    i;
  int32  offset;
  uint8  length;
  char*  keyString;
  
  //printf("Expandmap - ENTER\n");
  for (i = 0; i < count; i++)
    {
	  offset = B_BENDIAN_TO_HOST_INT32(list_offset[i]);
      // Read out and copy a single pascal-style key string.
      PRINT(("Expand map - offset for key %d is %d\n", i, offset));
      length = ((uchar*)fKeyMapBuffer)[offset];
      if (length <= 0) 
	{
	  keyString = (char*)malloc(2);
	  keyString[0] = 0;
	  length = 0;
	}
      else
	{
	  keyString = (char*)malloc(length + 1);
	  memcpy(keyString, &(fKeyMapBuffer[offset]), length + 1);
	}
      list_key[i] = keyString;
      PRINT(("ExpandMap - first key is \'%c\'\n", keyString[1]));
    }
  return;
}

//--------------------------------------------------------------------

void 
TKeymapWindow::PackMap(char **list_key, int32 *list_offset, int32 count) 
{
	/* list_key contains the pascal strings associated with the keys. 
	list_offset is the array of int32's which, on exit, should contain
	the values of the offsets of the strings within the key_map buffer.
	*/
	int i;
	uint8 length;
	
//	printf("TKeymapWindow::PackMap - ENTER, packing at %ld offset in buffer of size %ld\n",
//		fKeyMapBufferPos, fKeyMapBufferSize);

	for (i = 0; i < count; i++) {
		length = *(uint8*)(list_key[i]);

		list_offset[i] = B_HOST_TO_BENDIAN_INT32(fKeyMapBufferPos);
		
//		printf("** setting offset for key %d to %ld\n", i, fKeyMapBufferPos);
		
		if (fKeyMapBufferSize < fKeyMapBufferPos + length + 1) {
		
//			printf("** resizing buf to size %ld\n", fKeyMapBufferPos + 10 * (length + 1));
			
#if 0
			fKeyMapBuffer = (char*)realloc(fKeyMapBuffer,
				fKeyMapBufferPos + 10 * (length + 1));
			if (fKeyMapBuffer == NULL) {
				printf("NULL PTR!!!!!!!!!!!!1\n");
				break;
			}
			fKeyMapBufferSize += 10 * (length + 1);
#else
			// this might be allocating too much memory
			int32 size = fKeyMapBufferSize + 10 * (length + 1);
			char* temp = (char*)malloc(size);
			if (temp) {
				memcpy(temp, fKeyMapBuffer, size);
				free(fKeyMapBuffer);
				fKeyMapBuffer = temp;
				fKeyMapBufferSize = size;
			} else {
				printf("buffer is null\n");
				break;
			}
#endif
		}
		
//		printf("** copying data of length %d\n", length+1);
		memcpy(fKeyMapBuffer + fKeyMapBufferPos, list_key[i],  length+1);
		
//		printf("** done copying data\n");
		fKeyMapBufferPos += (length + 1);
//		printf("** buffer position now %ld\n", fKeyMapBufferPos);
	}
	
//	printf("TKeymapWindow::PackMap - EXIT\n");
}

//--------------------------------------------------------------------

void TKeymapWindow::ExpandKeyMap() 
{
  eKeyMap.version =           B_BENDIAN_TO_HOST_INT32(fKeyMap->version);
  eKeyMap.caps_key =          B_BENDIAN_TO_HOST_INT32(fKeyMap->caps_key);
  eKeyMap.scroll_key =        B_BENDIAN_TO_HOST_INT32(fKeyMap->scroll_key);
  eKeyMap.num_key =           B_BENDIAN_TO_HOST_INT32(fKeyMap->num_key);
  eKeyMap.left_shift_key =    B_BENDIAN_TO_HOST_INT32(fKeyMap->left_shift_key);
  eKeyMap.right_shift_key =   B_BENDIAN_TO_HOST_INT32(fKeyMap->right_shift_key);
  eKeyMap.left_command_key =  B_BENDIAN_TO_HOST_INT32(fKeyMap->left_command_key);
  eKeyMap.right_command_key = B_BENDIAN_TO_HOST_INT32(fKeyMap->right_command_key);
  eKeyMap.left_control_key =  B_BENDIAN_TO_HOST_INT32(fKeyMap->left_control_key);
  eKeyMap.right_control_key = B_BENDIAN_TO_HOST_INT32(fKeyMap->right_control_key);
  eKeyMap.left_option_key =   B_BENDIAN_TO_HOST_INT32(fKeyMap->left_option_key);
  eKeyMap.right_option_key =  B_BENDIAN_TO_HOST_INT32(fKeyMap->right_option_key);
  eKeyMap.menu_key =          B_BENDIAN_TO_HOST_INT32(fKeyMap->menu_key);
  eKeyMap.lock_settings =     B_BENDIAN_TO_HOST_INT32(fKeyMap->lock_settings);
  ExpandMap(eKeyMap.control_map,           fKeyMap->control_map,           128);
  ExpandMap(eKeyMap.option_caps_shift_map, fKeyMap->option_caps_shift_map, 128);
  ExpandMap(eKeyMap.option_caps_map,       fKeyMap->option_caps_map,       128);
  ExpandMap(eKeyMap.option_shift_map,      fKeyMap->option_shift_map,      128);
  ExpandMap(eKeyMap.option_map,            fKeyMap->option_map,            128);
  ExpandMap(eKeyMap.caps_shift_map,        fKeyMap->caps_shift_map,        128);
  ExpandMap(eKeyMap.caps_map,              fKeyMap->caps_map,              128);
  ExpandMap(eKeyMap.shift_map,             fKeyMap->shift_map,             128);
  ExpandMap(eKeyMap.normal_map,            fKeyMap->normal_map,            128);
  ExpandMap(eKeyMap.acute_dead_key,        fKeyMap->acute_dead_key,        32);
  ExpandMap(eKeyMap.grave_dead_key,        fKeyMap->grave_dead_key,        32);
  ExpandMap(eKeyMap.circumflex_dead_key,   fKeyMap->circumflex_dead_key,   32);
  ExpandMap(eKeyMap.dieresis_dead_key,     fKeyMap->dieresis_dead_key,     32);
  ExpandMap(eKeyMap.tilde_dead_key,        fKeyMap->tilde_dead_key,        32);
  eKeyMap.acute_tables =      B_BENDIAN_TO_HOST_INT32(fKeyMap->acute_tables);
  eKeyMap.grave_tables =      B_BENDIAN_TO_HOST_INT32(fKeyMap->grave_tables);
  eKeyMap.circumflex_tables = B_BENDIAN_TO_HOST_INT32(fKeyMap->circumflex_tables);
  eKeyMap.dieresis_tables =   B_BENDIAN_TO_HOST_INT32(fKeyMap->dieresis_tables);
  eKeyMap.tilde_tables =      B_BENDIAN_TO_HOST_INT32(fKeyMap->tilde_tables);
}

//--------------------------------------------------------------------

void TKeymapWindow::PackKeyMap() 
{
	PRINT(("TKeymapWindow::PackKeyMap - ENTER\n"));
	fKeyMapBufferPos = 0;
	
	fKeyMap->version =           B_HOST_TO_BENDIAN_INT32(eKeyMap.version);
	fKeyMap->caps_key =          B_HOST_TO_BENDIAN_INT32(eKeyMap.caps_key);
	fKeyMap->scroll_key =        B_HOST_TO_BENDIAN_INT32(eKeyMap.scroll_key);
	fKeyMap->num_key =           B_HOST_TO_BENDIAN_INT32(eKeyMap.num_key);
	fKeyMap->left_shift_key =    B_HOST_TO_BENDIAN_INT32(eKeyMap.left_shift_key);
	fKeyMap->right_shift_key =   B_HOST_TO_BENDIAN_INT32(eKeyMap.right_shift_key);
	fKeyMap->left_command_key =  B_HOST_TO_BENDIAN_INT32(eKeyMap.left_command_key);
	fKeyMap->right_command_key = B_HOST_TO_BENDIAN_INT32(eKeyMap.right_command_key);
	fKeyMap->left_control_key =  B_HOST_TO_BENDIAN_INT32(eKeyMap.left_control_key);
	fKeyMap->right_control_key = B_HOST_TO_BENDIAN_INT32(eKeyMap.right_control_key);
	fKeyMap->left_option_key =   B_HOST_TO_BENDIAN_INT32(eKeyMap.left_option_key);
	fKeyMap->right_option_key =  B_HOST_TO_BENDIAN_INT32(eKeyMap.right_option_key);
	fKeyMap->menu_key =          B_HOST_TO_BENDIAN_INT32(eKeyMap.menu_key);
	fKeyMap->lock_settings =     B_HOST_TO_BENDIAN_INT32(eKeyMap.lock_settings);
	
//	printf("TKeymapWindow::PackKeyMap - calling PackMap\n");
	
	PackMap(eKeyMap.control_map,           fKeyMap->control_map,           128);
	PackMap(eKeyMap.option_caps_shift_map, fKeyMap->option_caps_shift_map, 128);
	PackMap(eKeyMap.option_caps_map,       fKeyMap->option_caps_map,       128);
	PackMap(eKeyMap.option_shift_map,      fKeyMap->option_shift_map,      128);
	PackMap(eKeyMap.option_map,            fKeyMap->option_map,            128);
	PackMap(eKeyMap.caps_shift_map,        fKeyMap->caps_shift_map,        128);
	PackMap(eKeyMap.caps_map,              fKeyMap->caps_map,              128);
	PackMap(eKeyMap.shift_map,             fKeyMap->shift_map,             128);
	PackMap(eKeyMap.normal_map,            fKeyMap->normal_map,            128);
	PackMap(eKeyMap.acute_dead_key,        fKeyMap->acute_dead_key,        32);
	PackMap(eKeyMap.grave_dead_key,        fKeyMap->grave_dead_key,        32);
	PackMap(eKeyMap.circumflex_dead_key,   fKeyMap->circumflex_dead_key,   32);
	PackMap(eKeyMap.dieresis_dead_key,     fKeyMap->dieresis_dead_key,     32);
	PackMap(eKeyMap.tilde_dead_key,        fKeyMap->tilde_dead_key,        32);

//	printf("done packing\n");
	
	fKeyMap->acute_tables =      B_HOST_TO_BENDIAN_INT32(eKeyMap.acute_tables);
	fKeyMap->grave_tables =      B_HOST_TO_BENDIAN_INT32(eKeyMap.grave_tables);
	fKeyMap->circumflex_tables = B_HOST_TO_BENDIAN_INT32(eKeyMap.circumflex_tables);
	fKeyMap->dieresis_tables =   B_HOST_TO_BENDIAN_INT32(eKeyMap.dieresis_tables);
	fKeyMap->tilde_tables =      B_HOST_TO_BENDIAN_INT32(eKeyMap.tilde_tables);
}

void
TKeymapWindow::AddKeymaps()
{
  // Put the system keymaps in the system keymap list box. Put the user keymaps
  // in the user keymap list box. Add the current keymap to the user listbox.
  BPath path;
  // First, clear out existing stuff.
  EmptyList(fSystemEntryList, fSystemListView);
  EmptyList(fUserEntryList, fUserListView);

  // Add the read-only, system keymap files.
  if (find_directory(B_BEOS_ETC_DIRECTORY, &path) == B_NO_ERROR)
    {
      // append "settings/Keymap" to it.
      if (path.Append("Keymap") == B_NO_ERROR)
	{
	  PRINT(("SelectWindow::AddKeymaps - checking system dir\n"));
	  AddFilesToList(&path, fSystemListView, fSystemEntryList);
	}
    }

  // Add the default keymap file.
  BEntry entry;
  if (gDefaultKeymapFilePath != NULL && entry.SetTo(gDefaultKeymapFilePath) == B_NO_ERROR)
    {
      //printf("Adding default file to list.\n");
      // Add the entry to the listbox, etc.
      BEntry* defEntry = new BEntry(entry);
      fUserEntryList.AddItem(defEntry);
      BStringItem* item = new BStringItem(kDefaultKeymapName);
      fUserListView->AddItem(item);
      fUserListView->Select(0);
      fLastView = fUserListView;
      fOtherView = fSystemListView;
      fLastSelection = 0;
    }
  //else printf("Error setting to default file.\n");

  // Add the other user keymap files.
  if (GetSettingsPath(&path) == B_NO_ERROR)
    {
      PRINT(("SelectWindow::AddKeymaps - checking user dir\n"));
      AddFilesToList(&path, fUserListView, fUserEntryList);
    }  
}

void
TKeymapWindow::AddFilesToList(BPath* path, BListView* listView, BList& list)
{
  BDirectory directory(path->Path());
  BEntry     entry;
  directory.Rewind();
  //printf("%s\n", path->Path());
  while (directory.GetNextEntry(&entry) == B_NO_ERROR)
    {
      PRINT(("SelectWindow::AddFilesToList - found file\n"));
      BEntry* saveEntry = new BEntry(entry);
      AddFileToList(saveEntry, listView, list);
    }
  return;
}

void
TKeymapWindow::AddFileToList(BEntry* entry, BListView* listView, BList& list)
{
  attr_info info;
  char      name[2048];
  BNode     node(entry);

  // Add the entry to the list.
  list.AddItem(entry);

  // See if it has a "KeymapName" attribute.
  if (node.GetAttrInfo("KeymapName", &info) == B_NO_ERROR) 
    {
      if (name != NULL)
	{
	  node.ReadAttr("KeymapName", B_STRING_TYPE, 0, name, info.size);
	}
    }
  else
    {
      entry->GetName(name);
    }
  PRINT(("SelectWindow::AddFileToList name is %s\n", name));
  BStringItem* item = new BStringItem(name);
  listView->AddItem(item);
  return;
}

status_t
TKeymapWindow::GetSettingsPath(BPath* path)
{
  status_t result;

  result = find_directory(B_USER_SETTINGS_DIRECTORY, path, true);
  if (result == B_NO_ERROR)
    {
     // Create the Keymap settings subdirectory if necessary.
      BDirectory dir;
      if (dir.SetTo(path->Path()) == B_NO_ERROR)
	{
	  if (!(dir.Contains(kKeymapSettingsDirectory)))
	    {
	      BDirectory tempDir;
	      result = dir.CreateDirectory(kKeymapSettingsDirectory, &tempDir);
	    }
	  if (result == B_NO_ERROR)
	    {
	      result = path->Append("Keymap");
	    }
	}
    }
  //printf("Result is %s\n", strerror(result));
  return result;
}



void
TKeymapWindow::EmptyList(BList& list, BListView* listView)
{
  //Empty the BEntries from the list.
  for (int i = list.CountItems(); i > 0; i--)
    {
      BEntry* entry = (BEntry*)list.RemoveItem(i-1);
      delete entry;
    }

  // Empty the BStringItems from the listview.
  for (int i = listView->CountItems(); i > 0; i--)
    {
      BListItem *item = listView->RemoveItem(i-1);
      delete item;
    }
  return;
}
