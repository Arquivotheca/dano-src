//--------------------------------------------------------------------
//	
//	Fonts.cpp
//
//	Written by: Pierre Raynaud-Richard
//	
//	Copyright 1996 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <Debug.h>
#include <stdio.h>
#include <OS.h>
#include "Fonts.h"
#include <fcntl.h>
#include <malloc.h>
#include <nvram.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <priv_syscalls.h>

//--------------------------------------------------------------------

int main()
{	
	TFontsApp	*myApp;

	myApp = new TFontsApp();
	myApp->Run();

	delete myApp;
	return B_NO_ERROR;
}

//--------------------------------------------------------------------

TFontsApp::TFontsApp():BApplication('FNTS')
{
	int             ref;
	BRect			r;
	BPoint			win_pos, win_size;
	screen_info 	screen_info;

	Hpref.count = 0;
	Hpref.used = 0;
	Hpref.prefs = 0L;
	
	get_screen_info(0, &screen_info);

	r.Set(BROWSER_WIND, TITLE_BAR_HEIGHT,
		  BROWSER_WIND + WIND_WIDTH,
		  TITLE_BAR_HEIGHT + WIND_HEIGHT);
	if ((ref = open(FONTS_DATA_PATH, 0)) >= 0) {
		read(ref, &win_pos, sizeof(BPoint));
		read(ref, &win_size, sizeof(BPoint));
		close(ref);
		if (screen_info.frame.Contains(win_pos)) 
			r.OffsetTo(win_pos);
		r.SetRightBottom(r.LeftTop()+win_size);
	}
	if ((ref = open(FONTS_SETTINGS_PATH, 0)) >= 0) {
		ReadPrefs(ref);
		close(ref);
	}
	UpdatePrefs();
	
	fWindow = new TFontsWindow(r, "Fonts");
	fWindow->Show();
}

//--------------------------------------------------------------------

void TFontsApp::ExpandPrefs() {
	long        count;
	fnt_prefs   *fnt;
	
	if (Hpref.count == 0)
		count = 64;
	else
		count = Hpref.count*2;
	fnt = (fnt_prefs*)malloc(count*sizeof(fnt_prefs));
	if (Hpref.prefs != 0L) {
		memcpy((char*)fnt, (char*)Hpref.prefs, Hpref.count*sizeof(fnt_prefs));
		free((char*)Hpref.prefs);
	}
	Hpref.prefs = fnt;
	Hpref.count = count;
}

//--------------------------------------------------------------------

void TFontsApp::AddPrefs(char *file_name, char *real_name,
						 char *family_name, char *style_name,
						 char state) {
	int        i, file, real;
	fnt_prefs  *fnt;
	
	for (i=0; i<Hpref.used; i++) {
		fnt = Hpref.prefs+i;
	// Check the file_name
		file = (strcmp(fnt->file_name, file_name) == 0);
    // Check the real_name
		real = (strcmp(fnt->real_name, real_name) == 0);
	// Not the good one	
		if (file == FALSE) continue;
	// Same file_name
		if (real == FALSE) {
		// But different real_name : overwrite the previous one
			strcpy(fnt->real_name, real_name);
			strcpy(fnt->family_name, family_name);
			strcpy(fnt->style_name, style_name);
			return;
		}
		else {
			fnt->state += 2;
			return;
		}
	}
	if (Hpref.used == Hpref.count) ExpandPrefs();
	fnt = Hpref.prefs+Hpref.used;
	strcpy(fnt->file_name, file_name);
	strcpy(fnt->real_name, real_name);
	strcpy(fnt->family_name, family_name);
	strcpy(fnt->style_name, style_name);
	fnt->state = state;
	Hpref.used++;
}

//--------------------------------------------------------------------

static int font_strcmp(const void* cs, const void* ct)
{
	return(strcmp(((fnt_prefs*)cs)->real_name, ((fnt_prefs*)ct)->real_name));
}

void TFontsApp::SortPrefs() {
	int        i;
	fnt_prefs  *fnt;
	
	for (i=0;i<Hpref.used;i++) {
		fnt = Hpref.prefs+i;
		if (fnt->state > FNT_DISABLE) {
			fnt->full_name[0] = ' ';
			if (fnt->state == FNT_ENABLE+2)
				fnt->full_name[1] = 0xa5;
			else
				fnt->full_name[1] = ' ';
		}
		else fnt->full_name[0] = '+';
	}
}

//--------------------------------------------------------------------

void TFontsApp::ReadPrefs(int ref) {
	long       i, num;
	fnt_prefs  pref;
	
	read(ref, &num, sizeof(long));
	for (i=0;i<num;i++) {
		read(ref, &pref, sizeof(fnt_prefs));
		AddPrefs(pref.file_name, pref.real_name,
				 pref.family_name, pref.style_name, pref.state);
	}
}

//--------------------------------------------------------------------

void TFontsApp::WritePrefs(int ref) {
	long      i, num;

	num = 0;
	for (i=0;i<Hpref.used;i++)
		if (Hpref.prefs[i].full_name[0] == ' ') num++;
	write(ref, &num, sizeof(long));
	for (i=0;i<Hpref.used;i++)
		if (Hpref.prefs[i].full_name[0] == ' ') {
			if (Hpref.prefs[i].full_name[1] == ' ')
				Hpref.prefs[i].state = FNT_DISABLE;
			else
				Hpref.prefs[i].state = FNT_ENABLE;
			write(ref, Hpref.prefs+i, sizeof(fnt_prefs));
		}
}

//--------------------------------------------------------------------

bool TFontsApp::GetTrueTypeName(BEntry *entry, char *name, char *family, char *style) 
{
	#define tag_NamingTable         0x6e616d65L        /* 'name' */

	typedef long sfnt_TableTag;

	typedef struct {
		sfnt_TableTag   tag;
		ulong           checkSum;
		ulong           offset;
		ulong           length;
	} sfnt_DirectoryEntry;

	typedef struct {
		int    version;                 /* 0x10000 (1.0) */
		ushort numOffsets;              /* number of tables */
		ushort searchRange;             /* (max2 <= numOffsets)*16 */
		ushort entrySelector;           /* log2 (max2 <= numOffsets) */
		ushort rangeShift;              /* numOffsets*16-searchRange*/
		sfnt_DirectoryEntry table[1];   /* table[numOffsets] */
	} sfnt_OffsetTable;

	typedef struct {
		ushort platformID;
		ushort specificID;
		ushort languageID;
		ushort nameID;
		ushort length;
		ushort offset;
	} sfnt_NameRecord;

	typedef struct {
		ushort format;
		ushort count;
		ushort stringOffset;
	/*  sfnt_NameRecord[count]  */
	} sfnt_NamingTable;

	ushort     	    	numNames;
  	ulong               cTables;
	ulong               seek, cur_seek;
  	sfnt_OffsetTable    offsetTable;
  	sfnt_DirectoryEntry table;
  	sfnt_NamingTable    namingTable;
  	sfnt_NameRecord     nameRecord;
	long                i;
	bool                Bname, Bfamily, Bstyle;
	BFile ttFile;
	entry_ref ref;

	if (entry->GetRef(&ref) != B_NO_ERROR)
	  return false;
	
	if (ttFile.SetTo(entry, O_RDONLY) != B_NO_ERROR)
		return false;

	if (ttFile.Read(&offsetTable,
					 sizeof(sfnt_OffsetTable)-sizeof(sfnt_DirectoryEntry)) !=
		(sizeof(sfnt_OffsetTable)-sizeof(sfnt_DirectoryEntry)))
		return false;

  	cTables = ReadLong(&offsetTable.numOffsets);

	Bname = Bfamily = Bstyle = FALSE;
	
  	for (i = 0; (i < cTables) && (i < 40); i++) {
		if ((ttFile.Read(&table, sizeof(table))) != sizeof(table))
			return false;
    
		if (ReadULong(&table.tag) == tag_NamingTable) {	
			seek = ReadULong(&table.offset);
			if (ttFile.Seek(seek, SEEK_SET) != seek) 
			  return false;
			if (ttFile.Read(&namingTable, sizeof(namingTable)) !=
				sizeof(namingTable))
				return false;
			numNames = ReadUShort(&namingTable.count);
			while (numNames--) {
				if (ttFile.Read(&nameRecord, sizeof(nameRecord)) !=
					sizeof(nameRecord)) return false;

				if ((ReadUShort(&nameRecord.platformID)) == 1) {
					cur_seek = ttFile.Seek(0, SEEK_CUR);
					if ((ReadUShort(&nameRecord.nameID)) == 1) {
						seek = ReadULong(&table.offset) +
							ReadUShort(&nameRecord.offset) +
								ReadUShort(&namingTable.stringOffset);
						if (ttFile.Seek(seek, SEEK_SET) != seek) 
						  return false;
						seek = ReadUShort(&nameRecord.length);
						if (seek > REAL_FONT_NAME_MAX_SIZE)
							seek = REAL_FONT_NAME_MAX_SIZE;
						if (ttFile.Read(family, seek) != seek) 
						  return false;				
						family[seek] = '\0';
						Bfamily = true;
					}
					else if ((ReadUShort(&nameRecord.nameID)) == 2) {
						seek = ReadULong(&table.offset) +
							ReadUShort(&nameRecord.offset) +
								ReadUShort(&namingTable.stringOffset);
						if (ttFile.Seek(seek, SEEK_SET) != seek) 
						  return false;
						seek = ReadUShort(&nameRecord.length);
						if (seek > REAL_FONT_NAME_MAX_SIZE)
							seek = REAL_FONT_NAME_MAX_SIZE;
						if (ttFile.Read(style, seek) != seek) return false;				
						style[seek] = '\0';
						Bstyle = true;
					}
					else if ((ReadUShort(&nameRecord.nameID)) == 4) {
						seek = ReadULong(&table.offset) +
							ReadUShort(&nameRecord.offset) +
								ReadUShort(&namingTable.stringOffset);
						if (ttFile.Seek(seek, SEEK_SET) != seek) return false;
						seek = ReadUShort(&nameRecord.length);
						if (seek > REAL_FONT_NAME_MAX_SIZE)
							seek = REAL_FONT_NAME_MAX_SIZE;
						if (ttFile.Read(name, seek) != seek) return false;				
						name[seek] = '\0';
						Bname = true;
					}
					if (Bname && Bstyle && Bfamily)
						return true;
					ttFile.Seek(cur_seek, SEEK_SET);
				}
			}
			return false;
		}
	}
	return false;
}

//--------------------------------------------------------------------

void TFontsApp::UpdatePrefs() {
	char         name[B_FILE_NAME_LENGTH+1];
	char         real_name[REAL_FONT_NAME_MAX_SIZE+1];
	char         style_name[REAL_FONT_NAME_MAX_SIZE+1];
	char         family_name[REAL_FONT_NAME_MAX_SIZE+1];
  	entry_ref    ref;

	get_ref_for_path(TT_FONTS_FOLDER, &ref);
	BDirectory dir(&ref);
	dir.Rewind();
	
	BEntry entry;
	while(dir.GetNextEntry(&entry, false) == B_NO_ERROR) {
		entry.GetName(name);
		int j;
		for (j=0; j<B_FILE_NAME_LENGTH; j++)
			if (name[j] == 0) 
			  break;
		if (j < 4) 
		  continue;
		// check if file has a .ttf suffix
		if ((name[j-4] != '.') ||
			(name[j-3] != 't') ||
			(name[j-2] != 't') ||
			(name[j-1] != 'f'))
			continue;
		if (!GetTrueTypeName(&entry, real_name, family_name, style_name))
			continue;

		AddPrefs(name, real_name, family_name, style_name, FNT_ENABLE+2);
	}
	qsort(Hpref.prefs, Hpref.used, sizeof(fnt_prefs), font_strcmp);
	SortPrefs();
}

//--------------------------------------------------------------------

void TFontsApp::AboutRequested()
{
	BAlert		*myAlert;

	myAlert = new BAlert("", "...by Pierre Raynaud-Richard", "Done");
	myAlert->Go();
}

//====================================================================

TFontsWindow::TFontsWindow(BRect rect, char *title)
:BWindow(rect, title, B_TITLED_WINDOW, NULL)
{
	BRect		r;

	r = Frame();
	r.OffsetTo(0, 0);
	Lock();
	fView = new TFontsView(r, "FontsView");
	AddChild(fView);
	Unlock();
	SetSizeLimits( 170, 410, 230, 2000);
}

//--------------------------------------------------------------------

void TFontsWindow::MessageReceived(BMessage* msg)
{
    uchar      *data;
	BListView  *list;
	
	switch(msg->what) {
	case CANCEL:
		((TFontsApp*)be_app)->SortPrefs();
		fView->fList->Invalidate();
		break;
		
	case DONE:
		PostMessage(B_QUIT_REQUESTED);
		break;
		/*		
	case LIST_SELECT:
		if (msg->FindObjectPtr("source", &list) == B_NO_ERROR) {
			data = (uchar*)list->ItemAt(msg->FindInt32("index"));
			if (data[1] == 0xa5)
				data[1] = 0x20;
			else
				data[1] = 0xa5;
			list->Invalidate();
		}
		break;
		*/
	default:
		BWindow::MessageReceived(msg);
		break;
	}
}

//--------------------------------------------------------------------

bool TFontsWindow::QuitRequested()
{
	int		ref;
	BRect	r;
	BPoint	win_pos, win_size;

	r = Frame();
	win_pos = r.LeftTop();
	win_size = r.RightBottom() - r.LeftTop();
	mkdir(SETTINGS_FOLDER, 0777);
	if ((ref = open(FONTS_DATA_PATH, O_CREAT | O_TRUNC)) >= 0) {
		write(ref, &win_pos, sizeof(BPoint));
		write(ref, &win_size, sizeof(BPoint));
		close(ref);
	}
	if ((ref = open(FONTS_SETTINGS_PATH, O_CREAT | O_TRUNC)) >= 0) {
		((TFontsApp*)be_app)->WritePrefs(ref);
		close(ref);
	}

	be_app->PostMessage(B_QUIT_REQUESTED);
	return TRUE;
}


//====================================================================

TFontsView::TFontsView(BRect rect, char *title)
		  :BView(rect, title, B_FOLLOW_ALL, B_WILL_DRAW |
											B_FRAME_EVENTS |
											B_FULL_UPDATE_ON_RESIZE)
{
	BRect		r;
	BButton     *but;   
	BScrollView *fScroll;
	menu_info	minfo;
	
	get_menu_info(&minfo);
	r.Set(10, 10,
		  rect.Width() - 10 - B_V_SCROLL_BAR_WIDTH,
		  rect.Height() - 28 - minfo.font_size);
	fList = new TListView(r);
	fList->SetInvocationMessage(new BMessage(LIST_SELECT));
	fScroll = new BScrollView("pipo", fList, B_FOLLOW_ALL, 0, FALSE, TRUE);
	AddChild(fScroll);

	r.Set(rect.Width()/2 - 75, rect.Height() - 18 - minfo.font_size,
		  rect.Width()/2 - 5, rect.Height() - 10);
	but = new BButton(r, "", "Cancel", new BMessage(CANCEL),
					  B_FOLLOW_BOTTOM | B_FOLLOW_H_CENTER);
	AddChild(but);
	
	r.Set(rect.Width()/2 + 5, rect.Height() - 18 - minfo.font_size,
		  rect.Width()/2 + 75, rect.Height() - 10);
	but = new BButton(r, "", "Done", new BMessage(DONE),
					  B_FOLLOW_BOTTOM | B_FOLLOW_H_CENTER);
	AddChild(but);
}

//--------------------------------------------------------------------

void TFontsView::AttachedToWindow()
{
	rgb_color	c;

	c.red = c.green = c.blue = 216;
	SetViewColor(c);
	RefreshList();
}

//--------------------------------------------------------------------

void TFontsView::Draw(BRect where)
{
	BRect	r;

	// Window shading
	r = Frame();
	SetHighColor(255, 255, 255);
	StrokeLine(BPoint(r.left, r.top), BPoint(r.right, r.top));
	StrokeLine(BPoint(r.left, r.top), BPoint(r.left, r.bottom));
	SetHighColor(136, 136, 136);
	StrokeLine(BPoint(r.right, r.top + 1), BPoint(r.right, r.bottom));
	StrokeLine(BPoint(r.left + 1, r.bottom), BPoint(r.right, r.bottom));
	SetDrawingMode(B_OP_OVER);
}

//--------------------------------------------------------------------

void TFontsView::RefreshList()
{
	int      i;

	for (i=0;i<((TFontsApp*)be_app)->Hpref.used;i++)
	  if (((TFontsApp*)be_app)->Hpref.prefs[i].full_name[0] == ' ') 
			fList->AddItem(new BStringItem(((TFontsApp*)be_app)->Hpref.prefs[i].full_name));
}

//--------------------------------------------------------------------

TListView::TListView(BRect rect)
		  :BListView(rect, "", B_SINGLE_SELECTION_LIST,B_FOLLOW_ALL) {

}







