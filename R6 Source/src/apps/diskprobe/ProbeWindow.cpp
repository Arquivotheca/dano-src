//--------------------------------------------------------------------
//	
//	ProbeWindow.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1998 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "DiskProbe.h"
#include "ProbeWindow.h"
#include "Header.h"
#include "Slider.h"
#include "Content.h"
#include "FindWindow.h"
#include "Attributes.h"

extern	BMessage	*print_settings;


//====================================================================

TProbeWindow::TProbeWindow(BRect rect, char *title, void *data,
				int32 type, prefs *prefs)
			:BWindow(rect, title, B_DOCUMENT_WINDOW, 0)
{
	char			str[256];
	int32			index = 0;
	int32			size[] = {9, 10, 12, 14, 18, 24, 36, 48, 72, 0};
	BMenu			*menu;
	BMenu			*sub_menu;
	BMenuItem		*item;
	BMessage		*msg;
	BRect			r;

	fType = type;
	fPrefs = prefs;
	fBlockSize = 512;
	fDeviceSize = 512;
	fBlock = 0;
	fBitmap = NULL;
	fBuffer = NULL;
	fUndoBuffer = NULL;
	fLength = 0;
	fAttributes = NULL;
	fAttrList = new BList();
	fWindowCount = 1;
	fSearch = NULL;
	fFile = NULL;
	fRef = NULL;

	r.Set(0, 0, 32767, 15);
	fMenuBar = new BMenuBar(r, "");
	menu = new BMenu("File");

	msg = new BMessage(M_NEW);
	menu->AddItem(item = new BMenuItem("New"B_UTF8_ELLIPSIS, msg, 'N'));
	item->SetTarget(be_app);
	fDeviceMenu = new BMenu("Open Device");
	ScanDir("/dev/disk", fDeviceMenu);
	menu->AddItem(fDeviceMenu);
	menu->AddItem(item = new BMenuItem("Open File",
					new BMessage(M_OPEN_FILE), 'O'));
	item->SetTarget(be_app);
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Page Setup"B_UTF8_ELLIPSIS,
					new BMessage(M_PRINT_SETUP)));
	menu->AddItem(new BMenuItem("Print"B_UTF8_ELLIPSIS,
					new BMessage(M_PRINT), 'P'));
	menu->AddSeparatorItem();
	menu->AddItem(item = new BMenuItem("About Disk Probe",
					new BMessage(B_ABOUT_REQUESTED)));
	item->SetTarget(be_app);
	menu->AddSeparatorItem();
	menu->AddItem(item = new BMenuItem("Quit",
					new BMessage(B_QUIT_REQUESTED), 'Q'));
	item->SetTarget(be_app);
	fMenuBar->AddItem(menu);

	menu = new BMenu("Edit");
	menu->AddItem(fUndo = new BMenuItem("Undo", new BMessage(M_UNDO), 'Z'));
	menu->AddSeparatorItem();
	menu->AddItem(fCopy = new BMenuItem("Copy", new BMessage(B_COPY), 'C'));
	fCopy->SetTarget(NULL, this);
	menu->AddItem(fPaste = new BMenuItem("Paste", new BMessage(B_PASTE), 'V'));
	fPaste->SetTarget(NULL, this);
	menu->AddItem(item = new BMenuItem("Select All",
					new BMessage(M_SELECT_ALL), 'A'));
	item->SetTarget(NULL, this);

	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Find"B_UTF8_ELLIPSIS, new BMessage(M_FIND), 'F'));
	menu->AddItem(fFindAgain = new BMenuItem("Find Again", new BMessage(M_FIND_AGAIN), 'G'));
	fMenuBar->AddItem(menu);


	menu = new BMenu("Block");
	menu->AddItem(fNext = new BMenuItem("Next", new BMessage(M_NEXT), B_RIGHT_ARROW));
	menu->AddItem(fPrevious = new BMenuItem("Previous", new BMessage(M_PREVIOUS), B_LEFT_ARROW));
	menu->AddItem(fLastMenu = new BMenuItem("Back", new BMessage(M_LAST), 'J'));
	fGoSelection = new BMenu("Selection");
	fGoSelection->AddItem(fGo = new BMenuItem("", new BMessage(M_GO), 'K'));
	fGoSelection->AddItem(fGoSwap = new BMenuItem("", new BMessage(M_GO_SWAP), 'L'));
	menu->AddItem(fGoSelection);
	menu->AddSeparatorItem();
	menu->AddItem(fWrite = new BMenuItem("Write", new BMessage(M_WRITE), 'S'));
	menu->AddSeparatorItem();
	fBookmarks = new BMenu("Bookmarks");
	fBookmarks->AddItem(new BMenuItem("Add", new BMessage(M_ADD_BOOKMARK), 'B'));
	menu->AddItem(fBookmarks);
	fMenuBar->AddItem(menu);

	menu = new BMenu("View");
	sub_menu = new BMenu("Base");
	sub_menu->SetRadioMode(true);
	msg = new BMessage(M_BASE);
	msg->AddInt32("base", B_DECIMAL);
	sub_menu->AddItem(item = new BMenuItem("Decimal", msg, 'D'));
	if (fPrefs->base == B_DECIMAL)
		item->SetMarked(true);
	msg = new BMessage(M_BASE);
	msg->AddInt32("base", B_HEX);
	sub_menu->AddItem(item = new BMenuItem("Hex", msg, 'H'));
	if (fPrefs->base == B_HEX)
		item->SetMarked(true);
	menu->AddItem(sub_menu);

	sub_menu = new BMenu("Block Size");
	sub_menu->SetRadioMode(true);
	msg = new BMessage(M_BLOCK_SIZE);
	msg->AddInt32("size", 512);
	sub_menu->AddItem(fDefaultSize = new BMenuItem("512", msg));
	fDefaultSize->SetMarked(true);
	msg = new BMessage(M_BLOCK_SIZE);
	msg->AddInt32("size", 0);
	sub_menu->AddItem(fBlockMenu = new BMenuItem("", msg));
	menu->AddItem(sub_menu);
	
	menu->AddSeparatorItem();
	sub_menu = new BMenu("Font Size");
	sub_menu->SetRadioMode(true);
	while (size[index]) {
		msg = new BMessage(M_FONT_SIZE);
		msg->AddInt32("font_size", size[index]);
		sprintf(str, "%li", size[index]);
		sub_menu->AddItem(item = new BMenuItem(str, msg));
		if (size[index++] == fPrefs->font_size)
			item->SetMarked(true);
	}
	sub_menu->AddSeparatorItem();
	msg = new BMessage(M_FONT_SIZE);
	msg->AddInt32("size", 0);
	sub_menu->AddItem(item = new BMenuItem("Fit", msg));
	if (fPrefs->font_size == 0)
		item->SetMarked(true);
	menu->AddItem(sub_menu);
	fMenuBar->AddItem(menu);

	Lock();
	AddChild(fMenuBar);

	r = fMenuBar->Frame();
	r.OffsetTo(0, r.bottom + 2);
	r.right = 32767;
	r.bottom = r.top + HEADER_HEIGHT;
	AddChild(fHeaderView = new THeaderView(r, fPrefs->base, fType));

	r = fHeaderView->Frame();
	r.OffsetTo(0, r.bottom);
	r.right = Bounds().right;
	r.bottom = r.top + SLIDER_HEIGHT;
	fSliderView = new TSliderView(r, 0, 1);
	AddChild(fSliderView);

	r = fSliderView->Frame();
	r.OffsetTo(0, r.bottom + 3);
	r.right = Bounds().right - B_V_SCROLL_BAR_WIDTH ;
	r.bottom = Bounds().bottom - B_H_SCROLL_BAR_HEIGHT;

	fContentView = new TContentView(r, NULL, 0, fPrefs->base, false, fPrefs->font_size);
	AddChild(fScrollView = new BScrollView("", fContentView, B_FOLLOW_ALL, 0, true, true));

	AddCommonFilter(new TFilter(this));
	Unlock();

	fZoom = CalcMaxSize();

	if (fType == T_FILE) {
		if (OpenFile((entry_ref *)data) != B_NO_ERROR) {
			PostMessage(B_CLOSE_REQUESTED);
			Hide();
		}
	}
	else if (fType == T_DEVICE)
		OpenDevice((char *)data);
	fContentView->MakeFocus(true);
	AddShortcut(B_RIGHT_ARROW, B_COMMAND_KEY, new BMessage(M_NEXT_KEY));
	AddShortcut(B_LEFT_ARROW, B_COMMAND_KEY, new BMessage(M_PREVIOUS_KEY));
}

//--------------------------------------------------------------------

TProbeWindow::~TProbeWindow(void)
{
	BMessage	msg(M_WINDOW_CLOSED);

	if (fBuffer)
		free(fBuffer);
	if (fUndoBuffer)
		free(fUndoBuffer);
	if (fSearch)
		free(fSearch);
	if (fBitmap)
		delete fBitmap;

	msg.AddInt32("kind", W_PROBE);
	msg.AddRect("frame", Frame());
	be_app->PostMessage(&msg);
}

//--------------------------------------------------------------------

void TProbeWindow::FrameResized(float width, float height)
{
	int32	x;
	int32	y;

	fContentView->GetSize(&x, &y);
	fScrollView->ScrollBar(B_HORIZONTAL)->SetRange(0, max_c(0,
				x - fScrollView->Bounds().Width() + B_V_SCROLL_BAR_WIDTH + 4));
	fScrollView->ScrollBar(B_HORIZONTAL)->SetProportion((fScrollView->Bounds().Width() -
					B_V_SCROLL_BAR_WIDTH) / x);
	fScrollView->ScrollBar(B_VERTICAL)->SetRange(0, max_c(0,
				y - fScrollView->Bounds().Height() + B_H_SCROLL_BAR_HEIGHT + 2));
	fScrollView->ScrollBar(B_VERTICAL)->SetProportion(fScrollView->Bounds().Height() / y);
	y = fContentView->LineHeight();
	fScrollView->ScrollBar(B_VERTICAL)->SetSteps(y,
		(int)(((fScrollView->Bounds().Height() - B_H_SCROLL_BAR_HEIGHT) / y)) * y - y);
}

//--------------------------------------------------------------------

void TProbeWindow::MenusBeginning(void)
{
	char		label[256];
	int32		finish;
	int32		start;
	off_t		block;
	off_t		swap_block;
	status_t	result;

	fUndo->SetEnabled(fContentView->Dirty());
	if (CurrentFocus() != fContentView) {
		fHeaderView->GetSelection(&start, &finish);
		fCopy->SetEnabled(start != finish);
		be_clipboard->Lock();
		fPaste->SetEnabled(be_clipboard->Data()->HasData("text/plain", B_MIME_TYPE));
		be_clipboard->Unlock();
	}
	else {
		fCopy->SetEnabled(fBytesRead);
		be_clipboard->Lock();
		fPaste->SetEnabled((be_clipboard->Data()->HasData("text/plain", B_MIME_TYPE)) &&
					(!fReadOnly) && (fBytesRead > 0));
		be_clipboard->Unlock();
	}

	result = GetSelection(&block, &swap_block);
	fGoSelection->SetEnabled(result);
	sprintf(label, "Native: 0x%Lx", block);
	fGo->SetLabel(label);
	fGo->SetEnabled(result & 1);
	sprintf(label, "Swapped: 0x%Lx", swap_block);
	fGoSwap->SetLabel(label);
	fGoSwap->SetEnabled(result & 2);

	fFindAgain->SetEnabled(fSearch);
	fNext->SetEnabled((fBlock + 1) < (fLength + fBlockSize - 1) / fBlockSize);
	fPrevious->SetEnabled(fBlock);
	fWrite->SetEnabled(fContentView->Dirty());
}

//--------------------------------------------------------------------

void TProbeWindow::MessageReceived(BMessage* msg)
{
	char		*str;
	char		*title;
	char		bookmark[256];
	float		large;
	float		max;
	float		min;
	float		small;
	float		value = 0;
	int32		finish;
	int32		index;
	int32		loop;
	int32		mods;
	int32		opcode;
	int32		start;
	uint32		type;
	BEntry		entry;
	BMenuItem	*item;
	BMessage	*message;
	BPath		path;
	BScreen		screen(B_MAIN_SCREEN_ID);
	BRect		screen_frame = screen.Frame();
	BRect		r;
	dev_t		dev;
	entry_ref	ref;
	ino_t		dir;
	off_t		block;
	off_t		block_swap;
	attr_info	info;
	key_info	keys;
	attribute	*attr;

	switch(msg->what) {
		case M_PRINT_SETUP:
			PrintSetup();
			break;

		case M_PRINT:
			Print();
			break;

		case M_UNDO:
			fContentView->GetSelection(&start, &finish);
			for (loop = 0; loop < (fBlockSize / 4); loop++) {
				index = ((int32 *)fBuffer)[loop];
				((int32 *)fBuffer)[loop] = ((int32 *)fUndoBuffer)[loop];
				((int32 *)fUndoBuffer)[loop] = index;
			}
			fContentView->SetBlock(fBuffer, fBytesRead, fReadOnly, true);
			fContentView->SetSelection(start, finish);
			break;

		case M_SELECT_ALL:
			break;

		case M_BASE:
			msg->FindInt32("base", &fPrefs->base);
			fHeaderView->SetBase(fPrefs->base);
			fContentView->SetBase(fPrefs->base);
			break;
	
		case M_FIND:
		case M_FIND_AGAIN:
			if (TFindWindow::fFindWindow == NULL) {
				new TFindWindow(fPrefs);
				TFindWindow::fFindWindow->Show();
			}
			else if (msg->what == M_FIND_AGAIN)
				TFindWindow::fFindWindow->FindAgain();
			else
				TFindWindow::fFindWindow->Activate();
			TFindWindow::fFindWindow->WindowGuess(this);
			break;

		case M_FONT_SIZE:
			msg->FindInt32("font_size", &fPrefs->font_size);
			fContentView->SetFontSize(fPrefs->font_size);
			SetLimits();
			break;

		case M_NEXT_KEY:
			get_key_info(&keys);
			if (!(keys.key_states[99 >> 3] & (1 << ((7 - 99) & 7))))
				break;
		case M_NEXT:
			if ((fBlock + 1) < (fLength + fBlockSize - 1) / fBlockSize)
				Read(fBlock + 1);
			break;

		case M_PREVIOUS_KEY:
			get_key_info(&keys);
			if (!(keys.key_states[97 >> 3] & (1 << ((7 - 97) & 7))))
				break;
		case M_PREVIOUS:
			if (fBlock != 0)
				Read(fBlock - 1);
			break;

		case M_LAST:
			start = fStartSel;
			finish = fEndSel;
			Read(fLast);
			fContentView->SetSelection(start, finish);
			break;

		case M_GO:
		case M_GO_SWAP:
			if (GetSelection(&block, &block_swap)) {
				if (msg->what == M_GO)
					Read(block);
				else
					Read(block_swap);
			}
			else
				beep();
			break;

		case M_WRITE:
			if (Write() >= 0)
				fContentView->SetDirty(false);
			break;

		case M_ADD_BOOKMARK:
			if (fPrefs->base == B_DECIMAL)
				sprintf(bookmark, "Block %Ld", fBlock);
			else
				sprintf(bookmark, "Block 0x%Lx", fBlock);
			if (!fBookmarks->FindItem(bookmark)) {
				if (fBookmarks->CountItems() == 1)
					fBookmarks->AddSeparatorItem();
				fContentView->GetSelection(&start, &finish);
				message = new BMessage(M_BOOKMARK);
				message->AddInt64("bookmark", fBlock);
				message->AddInt32("start", start);
				message->AddInt32("end", finish);
				index = fBookmarks->CountItems() - 2;
				if (index < 10)
					fBookmarks->AddItem(new BMenuItem(bookmark, message, '0' + index));
				else
					fBookmarks->AddItem(new BMenuItem(bookmark, message));
			}
			break;

		case M_BLOCK_SIZE:
			if (Prompt("Write changes before changing block size?") == B_NO_ERROR) {
				if (fBlockMenu->IsMarked()) {
					block = (fBlock * 512) / fDeviceSize;
					fBlockSize = fDeviceSize;
				}
				else {
					block = (fBlock * fDeviceSize) / 512;
					fBlockSize = 512;
				}
				fBuffer = (uchar *)realloc(fBuffer, fBlockSize);
				fUndoBuffer = (uchar *)realloc(fUndoBuffer, fBlockSize);
				fHeaderView->SetInfo(NULL, (fLength + fBlockSize - 1) / fBlockSize);
				Init(block);
			}
			else {
				fDefaultSize->SetMarked(fBlockSize == 512);
				fBlockMenu->SetMarked(fBlockSize == fDeviceSize);
			}
			break;

		case M_KEY:
			msg->FindInt32("raw", &index);
			get_key_info(&keys);
			if (!(keys.key_states[index >> 3] & (1 << ((7 - index) & 7))))
				break;
			if (msg->FindInt32("key", &index) == B_NO_ERROR) {
				msg->FindInt32("modifiers", &mods);
				if (!(mods & B_COMMAND_KEY)) {
					fScrollView->ScrollBar(B_VERTICAL)->GetSteps(&small, &large);
					value = fScrollView->ScrollBar(B_VERTICAL)->Value();
					fScrollView->ScrollBar(B_VERTICAL)->GetRange(&min, &max);
				}
				switch (index) {
					case B_HOME:
						if (mods & B_COMMAND_KEY)
							index = 0;
						else
							value = min;
						break;
					case B_END:
						if (mods & B_COMMAND_KEY)
							index = (fLength + fBlockSize - 1) / fBlockSize - 1;
						else
							value = max;
						break;
					case B_PAGE_UP:
						if (mods & B_COMMAND_KEY) {
							index = fBlock - 1;
							if (index < 0)
								index = 0;
						}
						else {
							value -= large;
							if (value < min)
								value = min;
						}
						break;
					case B_PAGE_DOWN:
						if (mods & B_COMMAND_KEY) {
							index = fBlock + 1;
							if (index >= (fLength + fBlockSize - 1) / fBlockSize)
								index = (fLength + fBlockSize - 1) / fBlockSize - 1;
						}
						else {
							value += large;
							if (value > max)
								value = max;
						}
						break;
				}
				if (mods & B_COMMAND_KEY) {
					if ((index >= 0) && (index != fBlock))
						Read(index);
				}
				else
					fScrollView->ScrollBar(B_VERTICAL)->SetValue(value);
			}
			break;

		case M_BOOKMARK:
			msg->FindInt64("bookmark", &block);
			msg->FindInt32("start", &start);
			msg->FindInt32("end", &finish);
			Read(block);
			fContentView->SetSelection(start, finish);
			break;

		case M_BLOCK:
			if (msg->HasInt64("block"))
				 msg->FindInt64("block", &block);
			else
				block = strtoull(fHeaderView->fPosition->Text(), NULL, fPrefs->base);
			if ((block < 0) || (block >= (fLength + fBlockSize - 1) / fBlockSize))
				beep();
			else
				Read(block);
			break;

		case M_ATTRIBUTE:
			index = 0;
			msg->FindInt32("type", (int32 *)&type);
			msg->FindString("name", (const char**) &str);
			while ((attr = (attribute *)(fAttrList->ItemAt(index++)))) {
				if ((type == attr->type) && !(strcmp(str, attr->name))) {
					attr->window->Activate(true);
					return;
				}
			}
			r.Set(Frame().left, Frame().top,
				  Frame().left + ATTRIBUTE_WIDTH, Frame().top + ATTRIBUTE_HEIGHT);
			r.OffsetBy(fWindowCount * 20, fWindowCount * 20);
			if ((r.left - 6) < screen_frame.left)
				r.OffsetTo(screen_frame.left + 8, r.top);
			if ((r.left + 20) > screen_frame.right)
				r.OffsetTo(6, r.top);
			if ((r.top - 26) < screen_frame.top)
				r.OffsetTo(r.left, screen_frame.top + 26);
			if ((r.top + 20) > screen_frame.bottom)
				r.OffsetTo(r.left, TITLE_BAR_HEIGHT);
			if (fFile->GetAttrInfo(str, &info) == B_NO_ERROR) {
				attr = (attribute *)malloc(sizeof(attribute));
				attr->type = type;
				strcpy(attr->name, str);
				attr->size = info.size;
				attr->data = malloc(attr->size);
				fFile->ReadAttr(str, type, 0, attr->data, attr->size);
				attr->window = new TAttributesWindow(r, attr, this, fReadOnly);
				fAttrList->AddItem(attr);
				fWindowCount++;
			}
			else
				beep();
			break;

		case M_FIND_IT:
			msg->FindString("find_string", (const char**) &str);
			if (fSearch)
				fSearch = (uchar *)realloc(fSearch, strlen(str) + 1);
			else
				fSearch = (uchar *)malloc(strlen(str) + 1);
			strcpy((char *)fSearch, str);
			msg->FindBool("case", &fPrefs->case_sensitive);
			msg->FindInt32("length", &fSearchLen);
			Activate(true);
			Search();
			break;

		case B_SIMPLE_DATA:
			if (msg->FindRef("refs", &ref) == B_NO_ERROR)
				OpenFile(&ref);
			break;

		case B_NODE_MONITOR:
			if (msg->FindInt32("opcode", &opcode) == B_NO_ERROR) {
				switch (opcode) {
					case B_ENTRY_REMOVED:
						sprintf(bookmark, "DiskProbe:  The file \"%s\" has just been deleted.  All changes will be lost when this window is closed.",
								fRef->name);
						(new BAlert("", bookmark, "OK", NULL, NULL,
							B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
						break;
			
					case B_ENTRY_MOVED:
						msg->FindString("name", (const char**) &str);
						msg->FindInt64("to directory", (int64*) &dir);
						msg->FindInt32("device", &dev);
						delete fRef;
						fRef = new entry_ref(dev, dir, str);
						entry.SetTo(fRef);
						entry.GetPath(&path);
						SetTitle(str);
						fHeaderView->SetInfo((char *)path.Path(), (fLength + fBlockSize - 1) / fBlockSize, false);
						index = 0;
						while ((attr = ((attribute*)(fAttrList->
							ItemAt(index++))))) {
							if (attr->window) {
								title = (char *)malloc(strlen(str) + 
								  strlen(attr->name) + 10);
								sprintf(title, "%s - %s", str, attr->name);
								attr->window->SetTitle(title);
								free(title);
							}
						}
						break;
			
					case B_STAT_CHANGED:
						sprintf(bookmark, "DiskProbe:  The file \"%s\" has just been modified by another application.  Would you like to reload the file?",
								fRef->name);
						if ((new BAlert("", bookmark, "No", "Yes", NULL,
							B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go()) {
							fContentView->SetDirty(false);
							fFile->GetSize(&fLength);
							fHeaderView->SetInfo(NULL, (fLength + fBlockSize - 1) / fBlockSize);
							Init(0);
						}
						break;
			
					case B_ATTR_CHANGED:
						index = 0;
						while ((attr = ((attribute*)(fAttrList->
						  ItemAt(index++))))) {
							if (attr->window) {
								sprintf(bookmark, "DiskProbe:  The file \"%s's\" attributes have just been modified by another application.  Would you like to close all attribute windows?",
								fRef->name);
								if ((new BAlert("", bookmark, "No", "Yes", NULL,
									B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go()) {
									index = 0;
									while ((attr = ((attribute*)(fAttrList->
									  ItemAt(index++))))) {
										if (attr->window) {
											if (attr->window->Lock()) {
												AttrQuit(false, attr->window);
												index = 0;
											}
										}
									}
								}
							}
						}
						while ((item = fAttributes->ItemAt(0))) {
							fAttributes->RemoveItem(item);
							delete item;
						}
						BuildAttrMenu();
						break;
				}
			}
			break;

		default:
			BWindow::MessageReceived(msg);
	}
}

//--------------------------------------------------------------------

bool TProbeWindow::QuitRequested(void)
{
	if (Prompt("Write changes before closing?") == B_NO_ERROR) {
		if (fType == T_FILE)
			return(CloseFile() == B_NO_ERROR);
		else if (fType == T_DEVICE)
			return(CloseDevice() == B_NO_ERROR);
		return true;
	}
	return false;
}

//--------------------------------------------------------------------

void TProbeWindow::WindowActivated(bool active)
{
	fContentView->Activated(active);
	BWindow::WindowActivated(active);
}

//--------------------------------------------------------------------

void TProbeWindow::Zoom(BPoint /* pos */, float /* x */, float /* y */)
{
	BRect		r;

	r = CalcMaxSize();

	if ((abs((int)(Frame().Width() - r.Width())) < 5) &&
		(abs((int)(Frame().Height() - r.Height())) < 5)) {
		r = fZoom;
		r.OffsetTo(Frame().LeftTop());
	}
	else
		fZoom = Frame();

	ResizeTo(r.Width(), r.Height());
	MoveTo(r.LeftTop());
}

//--------------------------------------------------------------------

void TProbeWindow::AttrQuit(bool save, TAttributesWindow *wind)
{
	int32		index = 0;
	attribute	*attr;

	while ((attr = ((attribute*)(fAttrList->ItemAt(index++))))) {
		if (attr->window == wind) {
			if (save)
				fFile->WriteAttr(attr->name, attr->type, 0, attr->data, attr->size);
			free(attr->data);
			fAttrList->RemoveItem(attr);
			free(attr);
			fWindowCount--;
			wind->Quit();
		}
	}
}

//--------------------------------------------------------------------

void TProbeWindow::BuildAttrMenu(void)
{
	char		label[256];
	char		name[B_FILE_NAME_LENGTH];
	int32		index = 0;
	int32		type[] = {0, 0};
	BMenuItem	*item;
	BMessage	*msg;
	attr_info	attribute;

	if (!fAttributes) {
		fAttributes = new BMenu("Attributes");
		fMenuBar->AddItem(fAttributes, 3);
	}

	fFile->RewindAttrs();
	while(fFile->GetNextAttrName(name) == B_NO_ERROR) {
		fFile->GetAttrInfo(name, &attribute);
		msg = new BMessage(M_ATTRIBUTE);
		msg->AddString("name", name);
		type[0] = attribute.type;
		msg->AddInt32("type", type[0]);
		type[0] = B_HOST_TO_BENDIAN_INT32(type[0]);
		sprintf(label, "[%s] '%s'", (const char*)type, name);
		fAttributes->AddItem(new BMenuItem(label, msg));
		index++;
	}
	if (!index) {
		fAttributes->AddItem(item = new BMenuItem("none", new BMessage(M_ATTRIBUTE)));
		item->SetEnabled(false);
	}
}

//--------------------------------------------------------------------

BRect TProbeWindow::CalcMaxSize(void)
{
	int32		x;
	int32		y;
	BRect		r;

	r = Frame();
	fContentView->GetSize(&x, &y);
	r.right = r.left + x + B_V_SCROLL_BAR_WIDTH;
	r.bottom = r.top + fScrollView->Frame().top + y + B_H_SCROLL_BAR_HEIGHT;

	BScreen screen(this);
	BRect screen_frame = screen.Frame();
	screen_frame.InsetBy(6, 6);
	screen_frame.top += TITLE_BAR_HEIGHT - 6;

	if (r.Width() > screen_frame.Width())
		r.right = r.left + screen_frame.Width();
	if (r.Height() > screen_frame.Height())
		r.bottom = r.top + screen_frame.Height();

	if (r.right > screen_frame.right) {
		r.left -= r.right - screen_frame.right;
		r.right = screen_frame.right;
	}
	if (r.bottom > screen_frame.bottom) {
		r.top -= r.bottom - screen_frame.bottom;
		r.bottom = screen_frame.bottom;
	}
	if (r.left < screen_frame.left) {
		r.right += screen_frame.left - r.left;
		r.left = screen_frame.left;
	}
	if (r.top < screen_frame.top) {
		r.bottom += screen_frame.top - r.top;
		r.top = screen_frame.top;
	}
	return r;
}

//--------------------------------------------------------------------

status_t TProbeWindow::CloseDevice(void)
{
	status_t	result;

	result = Prompt("Write changes before closing?");
	if (result == B_NO_ERROR) {
		close(fDevice);
		fDevice = 0;
	}
	return result;
}

//--------------------------------------------------------------------

status_t TProbeWindow::CloseFile(void)
{
	BMenuItem	*item;
	status_t	result;
	attribute	*attr;

	while ((attr = (attribute *)(fAttrList->ItemAt(0)))) {
		if (attr->window->QuitRequested() == false)
			return B_ERROR;
	}
	fWindowCount = 1;

	result = Prompt("Write changes before closing?");
	if (result == B_NO_ERROR) {
		if (fFile) {
			stop_watching(this);
			delete fFile;
		}
		fFile = NULL;
		if (fRef)
			delete fRef;
		fRef = NULL;

		if (fAttributes) {
			while ((item = fAttributes->ItemAt(0))) {
				fAttributes->RemoveItem(item);
				delete item;
			}
		}
	}
	return result;
}

//--------------------------------------------------------------------

status_t TProbeWindow::GetSelection(off_t *unswap, off_t *swap)
{
	int32		end;
	int32		loop;
	int32		start;
	status_t	result = 0;

	*unswap = 0;
	*swap = 0;
	if (fBytesRead) {
		fContentView->GetSelection(&start, &end);
		end = min_c(end - start + 1, (int32) sizeof(int64));
		for (loop = start; loop < start + end; loop++) {
			*unswap <<= 8;
			*unswap |= fBuffer[loop];
		}
		switch (end) {
			case 1:
			case sizeof(int16):
				*swap = B_SWAP_INT16(*unswap);
				break;
			case 3:
			case sizeof(int32):
				*swap = B_SWAP_INT32(*unswap);
				break;
			case 5:
			case 6:
			case 7:
			case sizeof(int64):
				*swap = B_SWAP_INT64(*unswap);
				break;
		}
		if ((*unswap >= 0) && (*unswap < (fLength + fBlockSize - 1) / fBlockSize))
			result |= 1;
		if ((*swap >= 0) && (*swap < (fLength + fBlockSize - 1) / fBlockSize))
			result |= 2;
	}
	return result;
}

//--------------------------------------------------------------------

void TProbeWindow::Init(int32 block)
{
	char		label[256];
	BMenuItem	*item;

	fSliderView->SetMax((fLength + fBlockSize - 1) / fBlockSize - 1, block);

	fBytesRead = Read(block);
	fHeaderView->SetIcon(fBitmap);
	SetLimits();

	fLast = -1;
	fLastMenu->SetEnabled(false);

	sprintf(label, "%Ld", fDeviceSize);
	fBlockMenu->SetLabel(label);
	while ((item = fBookmarks->ItemAt(1))) {
		fBookmarks->RemoveItem(item);
		delete item;
	}
}

//--------------------------------------------------------------------

void TProbeWindow::OpenDevice(char *device)
{
	char			error[256];
	device_geometry	geometry;
	BRect			r;
	status_t		result;

	if ((fDevice = open(device, O_RDWR)) >= 0) {
		if ((result = ioctl(fDevice, B_GET_GEOMETRY, &geometry)) == B_NO_ERROR) {
			fDeviceSize = geometry.bytes_per_sector;
			(fBlockMenu->IsMarked()) ? fBlockSize = fDeviceSize : fBlockSize = 512;
			fReadOnly = geometry.read_only;
			fBuffer = (uchar *)malloc(fBlockSize);
			fUndoBuffer = (uchar *)malloc(fBlockSize);
			fLength = geometry.sectors_per_track * geometry.cylinder_count *
						geometry.head_count * fDeviceSize;
			r.Set(0, 0, B_LARGE_ICON - 1, B_LARGE_ICON - 1);
			fBitmap = new BBitmap(r, B_COLOR_8_BIT);
			if (get_device_icon(device, fBitmap->Bits(), B_LARGE_ICON) != B_NO_ERROR) {
				delete fBitmap;
				fBitmap = NULL;
			}
			SetTitle(device);
			fHeaderView->SetInfo(device, (fLength + fBlockSize - 1) / fBlockSize);
			Init(0);
		}
		else
			close(fDevice);
	}
	else
		result = fDevice;

	if (result < 0) {
		sprintf(error, "Sorry, error trying to open device '%s' (%s).",
				device, strerror(errno));
		(new BAlert("", error, "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
		PostMessage(B_CLOSE_REQUESTED);
		Hide();
		fType = T_NONE;
		fDevice = 0;
	}
	SetLimits();
}

//--------------------------------------------------------------------

status_t TProbeWindow::OpenFile(entry_ref *ref)
{
	char		label[256];
	int32		cookie = 0;
	BEntry		entry;
	BFile		*file;
	BNodeInfo	*info;
	BPath		path;
	BRect		r;
	status_t	result = B_NO_ERROR;
	fs_info		vol_info;
	struct stat	file_info;

	file = new BFile(ref, O_RDWR);
	if(file->InitCheck() == EROFS) {
		delete file;
		file = new BFile(ref, O_RDONLY);
	}
	if ((file->InitCheck() == B_NO_ERROR) && (file->IsFile())) {
		if (fType == T_FILE)
			result = CloseFile();
		else if (fType == T_DEVICE)
			result = CloseDevice();
		if (result != B_NO_ERROR) {
			delete file;
			return B_ERROR;
		}

		fFile = file;
		fFile->GetNodeRef(&fNode);
		watch_node(&fNode, B_WATCH_ALL, this);
		fRef = new entry_ref(*ref);
		fType = T_FILE;
		fReadOnly = false;

		entry.SetTo(ref);
		entry.GetStat(&file_info);
		fDeviceSize = 512;
		while (_kstatfs_(-1, &cookie, -1, NULL, &vol_info) == B_NO_ERROR) {
			if (vol_info.dev == file_info.st_dev) {
				fDeviceSize = vol_info.block_size;
				fReadOnly = vol_info.flags & B_FS_IS_READONLY;
				break;
			}
		}
		(fBlockMenu->IsMarked()) ? fBlockSize = fDeviceSize : fBlockSize = 512;
		(fBuffer) ? fBuffer = (uchar *)realloc(fBuffer, fBlockSize) :
					fBuffer = (uchar *)malloc(fBlockSize);
		(fUndoBuffer) ? fUndoBuffer = (uchar *)realloc(fUndoBuffer, fBlockSize) :
					fUndoBuffer = (uchar *)malloc(fBlockSize);
		SetTitle(ref->name);
		entry.GetPath(&path);
		if (fBitmap)
			delete fBitmap;
		info = new BNodeInfo(fFile);
		r.Set(0, 0, B_LARGE_ICON - 1, B_LARGE_ICON - 1);
		fBitmap = new BBitmap(r, B_COLOR_8_BIT);
		info->GetTrackerIcon(fBitmap, B_LARGE_ICON);
		delete info;
		fFile->GetSize(&fLength);
		fHeaderView->SetInfo((char *)path.Path(), (fLength + fBlockSize - 1) / fBlockSize);
		Init(0);

		BuildAttrMenu();
		result = B_NO_ERROR;
	}
	else {
		if (file->InitCheck() == B_NO_ERROR)
			sprintf(label, "Sorry, only files and devices can be opened.");
		else
			sprintf(label, "Sorry, error trying to open file '%s' (%s).", ref->name, strerror(file->InitCheck()));
		(new BAlert("", label, "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
		delete file;
		result = B_ERROR;
	}
	return result;
}

//--------------------------------------------------------------------

void TProbeWindow::Print(void)
{
	if (print_settings == NULL) {
		if (PrintSetup() != B_NO_ERROR)
			return;
	}

	BPrintJob	printJob(Title());
	printJob.SetSettings(new BMessage(*print_settings));

	if (printJob.ConfigJob() == B_NO_ERROR) {
		BRect	pageRect = printJob.PrintableRect();
		pageRect.OffsetTo(B_ORIGIN);

		printJob.BeginJob();
		printJob.DrawView(fContentView, pageRect, BPoint(0.0, 0.0));
		printJob.SpoolPage();
		printJob.CommitJob();
	}
}

//--------------------------------------------------------------------

status_t TProbeWindow::PrintSetup(void)
{
	BPrintJob	print(Title());
	status_t	result = B_ERROR;

	if (print_settings)
		print.SetSettings(new BMessage(*print_settings));

	if ((result = print.ConfigPage()) == B_NO_ERROR) {
		delete print_settings;
		print_settings = print.Settings();
	}
	return result;
}

//--------------------------------------------------------------------

status_t TProbeWindow::Prompt(const char *prompt)
{
	int32		button;
	status_t	result = B_NO_ERROR;

	if (fContentView->Dirty()) {
		button = (new BAlert("", prompt, "Don't Write", "Cancel", "Write",
					B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
		switch (button) {
			case 0:
				fContentView->SetDirty(false);
				break;
			case 1:
				result = B_ERROR;
				break;
			case 2:
				if (Write() >= 0)
					fContentView->SetDirty(false);
				break;
		}
	}
	return result;
}

//--------------------------------------------------------------------

ssize_t TProbeWindow::Read(off_t block)
{
	char		error[256];
	off_t		last = fBlock;
	ssize_t		bytes = -1;

	if (Prompt("Write changes before reading next block?") == B_NO_ERROR) {
		if (fType == T_FILE) {
			fFile->Seek(block * fBlockSize, 0);
			bytes = fFile->Read(fBuffer, fBlockSize);
		}
		else if (fType == T_DEVICE) {
			lseek(fDevice, block * fBlockSize, 0);
			bytes = read(fDevice, fBuffer, fBlockSize);
		}
	}
	else
		return B_ERROR;

	if (fLast == -1)
		fLastMenu->SetEnabled(true);
	fLast = last;
	fContentView->GetSelection(&fStartSel, &fEndSel);

	if (bytes >= 0)
		fBytesRead = bytes;
	else {
		fBytesRead = 0;
		if (fPrefs->base == B_HEX)
			sprintf(error, "Error reading block 0x%Lx (%s [0x%x])", block, strerror(errno), errno);
		else
			sprintf(error, "Error reading block %Ld (%s [%d])", block, strerror(errno), errno);
		(new BAlert("", error, "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
	}
	fBlock = block;
	fHeaderView->SetBlock(fBlock);
	fContentView->SetBlock(fBuffer, fBytesRead, fReadOnly);
	memcpy(fUndoBuffer, fBuffer, fBytesRead);
	fScrollView->ScrollBar(B_VERTICAL)->SetValue(0);
	fSliderView->SetValue(fBlock);
	return bytes;
}

//--------------------------------------------------------------------

void TProbeWindow::ScanDir(const char *directory, BMenu *menu)
{
	const char			*name;
	BPath				path;
	BDirectory			dir;
	BEntry				entry;
	BMenuItem			*item;
	BMessage			*msg;

	dir.SetTo(directory);
	if (dir.InitCheck() == B_NO_ERROR) {
		dir.Rewind();
		while ((dir.GetNextEntry(&entry) >= 0) && (entry.GetPath(&path) == B_NO_ERROR)) {
			name = path.Path();
			if (entry.IsDirectory())
				ScanDir(name, menu);
			else if (!strstr(name, "rescan")) {
				msg = new BMessage(M_OPEN_DEVICE);
				msg->AddString("path", name);
				menu->AddItem(item = new BMenuItem(name, msg));
				item->SetTarget(be_app);
			}
		}
	}
}

//--------------------------------------------------------------------

void TProbeWindow::Search(void)
{
	bool		found = false;
	bool		wrap = false;
	uchar		*buffer;
	uchar		*s1;
	uchar		*s2;
	uchar		c1;
	uchar		c2;
	int32		begin;
	int32		end;
	int32		len;
	int32		loop;
	off_t		block;
	off_t		buffer_size;
	ssize_t		bytes_read = 0;
	BPoint		where;

	buffer_size = min_c(fLength, 65536);
	buffer = (uchar *)malloc(buffer_size);
	block = fBlock;
	fContentView->GetSelection(&begin, &end);
	begin++;
	if (begin == fBlockSize - 1) {
		begin = 0;
		block++;
	}
	for (;;) {
		fHeaderView->SetBlock(block);
		fSliderView->SetValue(block);
		if (fType == T_FILE) {
			fFile->Seek(block * fBlockSize, 0);
			bytes_read = fFile->Read(buffer, buffer_size);
		}
		else if (fType == T_DEVICE) {
			if (block * fBlockSize >= fLength)
				bytes_read = 0;
			else {
				lseek(fDevice, block * fBlockSize, 0);
				if (block * fBlockSize + buffer_size >= fLength)
					bytes_read = read(fDevice, buffer, fLength - (block * fBlockSize));
				else
					bytes_read = read(fDevice, buffer, buffer_size);
			}
		}

		if (bytes_read > 0) {
			s1 = &buffer[begin];
			for (loop = begin; loop < bytes_read - fSearchLen + 1; loop++) {
				if (fPrefs->case_sensitive) {
					if (*s1 == *fSearch) {
						s2 = fSearch;
						len = fSearchLen;
						while ((len) && (*s1++ == *s2++)) {
							len--;
						}
						if (!len) {
							found = true;
							break;
						}
						else
							s1 = &buffer[loop];
					}
				}
				else {
					c1 = *s1;
					if ((c1 >= 'a') && (c1 <= 'z'))
						c1 -= 'a' - 'A';
					c2 = *fSearch;
					if ((c2 >= 'a') && (c2 <= 'z'))
						c2 -= 'a' - 'A';
					if (c1 == c2) {
						s2 = fSearch;
						len = fSearchLen;
						while (len) {
							c1 = *s1++;
							if ((c1 >= 'a') && (c1 <= 'z'))
								c1 -= 'a' - 'A';
							c2 = *s2++;
							if ((c2 >= 'a') && (c2 <= 'z'))
								c2 -= 'a' - 'A';
							if (c2 != c1)
								break;
							len--;
						}
						if (!len) {
							found = true;
							break;
						}
						else
							s1 = &buffer[loop];
					}
				}
				s1++;
			} //for (loop...
			if (found) {
				len = s1 - buffer - fSearchLen;
				block += len / fBlockSize;
				if (block != fBlock) {
					if (Read(block) < 0)
						break;
				}
				fContentView->SetSelection(len % fBlockSize,
											len % fBlockSize + fSearchLen - 1);
				fContentView->Scroll(len % fBlockSize);
				break;
			}
			else if (bytes_read < buffer_size) {
				if (wrap) {
					beep();
					break;
				}
				wrap = true;
				block = 0;
			}
			else {
				block += buffer_size / fBlockSize;
				if ((wrap) && (block + (buffer_size / fBlockSize) >= fBlock))
					buffer_size = (fBlock - block) * fBlockSize;
			}
			if ((wrap) && (block >= fBlock)) {
				beep();
				break;
			}
			begin = 0;
			UpdateIfNeeded();
			fHeaderView->GetMouse(&where, (uint32 *)&end);
			if ((end) && (IsActive()) && (Bounds().Contains(where)))
				break;
		}
		else if (!wrap) {
			wrap = true;
			block = 0;
		}
		else
			break;
	}
	free(buffer);
	fHeaderView->SetBlock(fBlock);
	fSliderView->SetValue(fBlock);
}

//--------------------------------------------------------------------

void TProbeWindow::SetLimits(void)
{
	int32			x;
	int32			y;

	fContentView->GetSize(&x, &y);
	if (fPrefs->font_size)
		SetSizeLimits(248, x + B_V_SCROLL_BAR_WIDTH,
					100 + fSliderView->Frame().bottom,
					fScrollView->Frame().top + y + B_H_SCROLL_BAR_HEIGHT);
	else
		SetSizeLimits(250, 32767, 100 + fSliderView->Frame().bottom, 32767);
	if ((Frame().Width() > x + B_V_SCROLL_BAR_WIDTH) ||
		(Frame().Height() > fScrollView->Frame().top + y + B_H_SCROLL_BAR_HEIGHT))
		Zoom(BPoint(0, 0), 0, 0);
	FrameResized(0, 0);
}

//--------------------------------------------------------------------

void TProbeWindow::SetOffset(off_t offset)
{
	fHeaderView->SetOffset(offset, fBlock * fBlockSize + offset);
}

//--------------------------------------------------------------------

ssize_t TProbeWindow::Write(void)
{
	char		error[256];
	ssize_t		bytes = B_ERROR;

	if (fType == T_FILE) {
		fFile->Seek(fBlock * fBlockSize, 0);
		bytes = fFile->Write(fBuffer, fBytesRead);
	}
	else if (fType == T_DEVICE) {
		lseek(fDevice, fBlock * fBlockSize, 0);
		bytes = write(fDevice, fBuffer, fBytesRead);
	}
	if (bytes >= 0)
		memcpy(fUndoBuffer, fBuffer, fBytesRead);
	else {
		if (fPrefs->base == B_HEX)
			sprintf(error, "Error writing block 0x%Lx (%s [0x%x])", fBlock, strerror(errno), errno);
		else
			sprintf(error, "Error writing block %Ld (%s [%d])", fBlock, strerror(errno), errno);
		(new BAlert("", error, "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
	}
	return bytes;
}


//====================================================================

TFilter::TFilter(TProbeWindow *window)
		:BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE, B_KEY_DOWN)
{
	fWindow = window;
}

//--------------------------------------------------------------------

filter_result TFilter::Filter(BMessage *msg, BHandler **target)
{
	char		key;
	char		raw;
	char		*str;
	int32		mods;
	BMessage	message(M_KEY);

	if (msg->FindString("bytes", (const char**) &str) == B_NO_ERROR) {
		key = str[0];
		if ((key == B_PAGE_UP) || (key == B_PAGE_DOWN) ||
			(key == B_HOME) || (key == B_END)) {
			message.AddInt32("key", key);
			raw = msg->FindInt32("key");
			message.AddInt32("raw", raw);
			msg->FindInt32("modifiers", &mods);
			message.AddInt32("modifiers", mods);
			fWindow->PostMessage(&message, fWindow);
			return B_SKIP_MESSAGE;
		}
	}
	return B_DISPATCH_MESSAGE;
}
