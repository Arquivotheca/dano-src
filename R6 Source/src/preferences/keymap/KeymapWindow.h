/*--------------------------------------------------------------------
//	
//	KeymapWindow.h
//
//	Written by: Robert Polic
//	
//	Copyright 1994 Be, Inc. All Rights Reserved.
//	
//------------------------------------------------------------------*/

#ifndef KEYMAP_WINDOW_H
#define KEYMAP_WINDOW_H

#include <stdio.h>
#include <Alert.h>
#include <Application.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Menu.h>
#include <Window.h>
#include <ListView.h>
#include <List.h>

#include <FilePanel.h>
#include <Entry.h>
#include <Path.h>

#define SIDEPANELWIDTH 140
#define BUTTONBOXHEIGHT 35

#include "KeymapView.h"

/* this is a more easily - modifiable version of the keymap data.
   We read in the keymap and convert it to this, then convert it
   back when we save. 
   */
struct ext_keymap {
	uint32	version;
	uint32	caps_key;
	uint32	scroll_key;
	uint32	num_key;
	uint32	left_shift_key;
	uint32	right_shift_key;
	uint32	left_command_key;
	uint32	right_command_key;
	uint32	left_control_key;
	uint32	right_control_key;
	uint32	left_option_key;
	uint32	right_option_key;
	uint32	menu_key;
	uint32	lock_settings;
	char	*control_map[128];
	char	*option_caps_shift_map[128];
	char	*option_caps_map[128];
	char	*option_shift_map[128];
	char	*option_map[128];
	char	*caps_shift_map[128];
	char	*caps_map[128];
	char	*shift_map[128];
	char	*normal_map[128];
	char	*acute_dead_key[32];
	char	*grave_dead_key[32];
	char	*circumflex_dead_key[32];
	char	*dieresis_dead_key[32];
	char	*tilde_dead_key[32];
	uint32	acute_tables;
	uint32	grave_tables;
	uint32	circumflex_tables;
	uint32	dieresis_tables;
	uint32	tilde_tables;
};

enum menuIDs	{
	MENU_OPEN = 1,	
	MENU_SAVE,		
	MENU_SAVE_AS,	  
	MENU_SET_SYS,
	MENU_QUIT,
				 
	MENU_UNDO,		
	MENU_CLEAR,		
	MENU_SELECT_ALL,  

//	MENU_DVORAK,
//	MENU_QWERTY,	
	MENU_RESTORE,	
	MENU_FONT		  
//	MENU_SYMBOL
};

#define KEYMAP_MSG 'KMAP'
#define KEYMAP_NAME "Keymap_Name"

//====================================================================

class TKeymapView;

class TKeymapWindow : public BWindow 
{

public:
  bool			fHaveFile;
  BFile*		fMapFile;
  BMenuBar*		fMenuBar;
  font_family		fFontName;
  font_style		fFontStyle;
  
  TKeymapWindow(BEntry* entry, bool writableFlag, BRect frame, char *title);
  ~TKeymapWindow();
  
  virtual	bool 	QuitRequested();
  virtual void	MessageReceived(BMessage* theMessage);
  virtual	bool	FilterMessageDropped(BMessage *drop, BPoint pt, BView **target);
  virtual void	WindowActivated(bool activeFlag);
  virtual void	SaveRequested(entry_ref *directory, const char* name);
  
  void			BusyFile();
  void			CloseFile(bool SaveChanges);
  void                  OpenFile(BFile* file, bool readOnly);
  bool			OpenFile(entry_ref theRef, bool readOnly);
  bool			SaveChanges();
  void			SetMenuEnable(long theItem, bool theState);
  void			SetMenuItemMark(long theItem, char theState);
  
  void                  LoadCurrentKeyMap();
  void                  ExpandMap(char **list_key, int32 *list_offset, int32 count);
  void                  PackMap(char **list_key, int32 *list_offset, int32 count);
  void                  ExpandKeyMap();
  void                  PackKeyMap();
  
protected:
  virtual void        BuildMenuBar();
  virtual status_t    GetSettingsPath(BPath* path);
  virtual void        AddButtons();
  virtual void	      Save(BFile* mapFile);
  virtual void        AddKeymaps();
  virtual void        AddFilesToList(BPath* path, BListView* listView, BList& list); 
  virtual void        AddFileToList(BEntry* entry, BListView* listView, BList& list);
  virtual bool        ReadKeyMapData(BFile* file, struct key_map** keyMap, 
				     int32* keymapBufferSize, 
				     char** keymapBuffer);
  virtual void        EmptyList(BList& list, BListView* listView);
  BMenuItem	      *cutItem;
  BMenuItem	      *copyItem;
  BMenuItem	      *pasteItem;
  BFilePanel*         fOpenPanel;
  BFilePanel*         fSavePanel;
  BEntry*             fDefaultFileEntry;

  // UI Stuff in window
  BView*              fBackView;
  TKeymapView*        fKeymapView;
  BListView*          fSystemListView;
  BListView*          fUserListView;
  BList               fSystemEntryList;
  BList               fUserEntryList;
  int32               fLastSelection;
  BListView*          fLastView;
  BListView*          fOtherView;

  // Keymap data struct stuff
  struct key_map      *fKeyMap;
  char		      *fKeyMapBuffer;
  int32               fKeyMapBufferSize;
  int32               fKeyMapBufferPos;
  ext_keymap          eKeyMap;

};

#endif



