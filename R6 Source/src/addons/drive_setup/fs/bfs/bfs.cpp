//--------------------------------------------------------------------
//	
//	bfs.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1997 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <MenuBar.h>
#include <Screen.h>

#include <Path.h>
#include <FindDirectory.h>

#include "bfs.h"


//====================================================================

bool ds_fs_id(partition_data *part, int32 dev, uint64 offset, int32 block_size)
{
	bool				result = FALSE;
	void                *ptr;
	disk_super_block	*sb;
	disk_super_block	*new_sb;
	uchar               *bitmap;   /* we also check the first few bits in the bitmap */

	ptr = sb = (disk_super_block *)malloc(16384);
	new_sb = (disk_super_block *)((char *)sb + 512);
	lseek(dev, (offset * block_size) + (part->offset * part->logical_block_size), 0);
	if (read(dev, sb, 16384) != 16384) {
		free(ptr);
		return FALSE;
	}

	if (new_sb->magic1 == SUPER_BLOCK_MAGIC1)
		sb = new_sb;
	
	if ((sb->magic1 == SUPER_BLOCK_MAGIC1) &&
		(sb->magic2 == SUPER_BLOCK_MAGIC2) &&
		(sb->magic3 == SUPER_BLOCK_MAGIC3)) {

		int num_bitmap_blocks, bsize, n, i;

		/* now check that the super block and bitmap blocks are allocated */
    	bsize = sb->block_size;
		bitmap = (uchar *)((char *)ptr + bsize);

	    num_bitmap_blocks  = sb->num_blocks / 8;
	    num_bitmap_blocks  = ((num_bitmap_blocks + bsize - 1) & ~(bsize - 1));
	    num_bitmap_blocks /= bsize;

		/* n == the number of blocks that absolutely must be set 
           in the bitmap.  the +1 is for the superblock.
        */
		n = 1 + num_bitmap_blocks;
		if (n > (16384 - bsize - 512))  /* make sure we don't go too far */
			n = 16384 - bsize - 512;
		
		for(i=0; i < n; i++) {
			/* check if the bit was supposed to have been set... */
			if ((bitmap[i/8] & (1 << (i%8))) == 0) {  
				result = FALSE;
				goto done;
			}
		}
		
		strcpy(part->file_system_short_name, "bfs");
		strcpy(part->file_system_long_name, "Be File System");
		strcpy(part->volume_name, sb->name);

		result = TRUE;
	}

done:
	free(ptr);
	return result;
}

//--------------------------------------------------------------------

void ds_fs_flags(drive_setup_fs_flags *flags)
{
	flags->can_initialize = TRUE;
	flags->has_options = TRUE;
}

//--------------------------------------------------------------------

void ds_fs_initialize(BMessage *msg)
{
	BRect		r;
	BWindow		*wind;
	InitWindow	*window;

	msg->FindPointer("window", (void **)&wind);
	r = wind->Frame();
	r.left += ((r.Width() - WIND_WIDTH) / 2);
	r.right = r.left + WIND_WIDTH;
	r.top += ((r.Height() - WIND_HEIGHT) / 2);
	r.bottom = r.top + WIND_HEIGHT;

	BScreen screen( wind );
	BRect screen_frame = screen.Frame();
	screen_frame.InsetBy(6, 6);
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
	window = new InitWindow(r, msg);
}


//====================================================================

InitWindow::InitWindow(BRect rect, BMessage *msg)
		   :BWindow(rect, "", B_MODAL_WINDOW, B_NOT_RESIZABLE)
{
	char			str[256];
	const char		*name;
	uint32			size;
	BBox			*box;
	BButton			*button;
	BFont			font;
	BMenu			*menu;
	BMenuItem		*item;
	BMessage		*message;
	BMenuField		*field;
	BRect			r;
	BStringView		*string;

	fMessage = msg;
	
	const char *tmpDevice;
	if ((msg->FindString("device", &tmpDevice) != B_NO_ERROR) ||
		(msg->FindInt32("block_size", &fBlockSize) != B_NO_ERROR)) {
		(new BAlert("", "Error in message data passed to bfs add-on.", "Sorry"))->Go();
		Run();
		PostMessage(new BMessage(M_CANCEL));
		return;
	}
	fDevice = tmpDevice;

	r = Frame();
	r.OffsetTo(0, 0);
	r.InsetBy(-1, -1);
	r.top--;
	AddChild(box = new BBox(r, "", B_FOLLOW_ALL, B_WILL_DRAW, B_FANCY_BORDER));

	r.Set(TITLE_H, TITLE_V, TITLE_H + TITLE_WIDTH, TITLE_V + 16);
	sprintf(str, "%s - %s", TITLE_TEXT, fDevice.String());
	string = new BStringView(r, "", str);
	box->AddChild(string);
	font = *be_bold_font;
	font.SetSize(11.0);
	string->SetFont(&font);

	r.Set(LINE1_H, LINE1_V, LINE1_H + LINE1_WIDTH, LINE1_V + 1);
	box->AddChild(new BBox(r, "", B_FOLLOW_ALL, B_WILL_DRAW, B_FANCY_BORDER));

	font = *be_bold_font;
	font.SetSize(10.0);
	r.Set(BLOCK_MENU_H, BLOCK_MENU_V,
		  BLOCK_MENU_H + BLOCK_MENU_WIDTH, BLOCK_MENU_V + 20);
	menu = new BPopUpMenu("");
	menu->SetFont(&font);
	size = fBlockSize;
	if (size == 512)
		size = 1024;
	fSize = max_c(size, 1024);
	while (size <= 4 * 1024) {
		message = new BMessage(M_MENU);
		message->AddInt32("block", size);
		sprintf(str, "%4d", size);
		menu->AddItem(item = new BMenuItem(str, message));
		if (size == fSize)
			item->SetMarked(TRUE);
		size *= 2;
	}
	field = new BMenuField(r, "", BLOCK_MENU_TEXT, menu);
	field->SetFont(&font);
	field->MenuBar()->SetFont(&font);
	box->AddChild(field);

	r.Set(VOLUME_NAME_H, VOLUME_NAME_V,
		  VOLUME_NAME_H + VOLUME_NAME_WIDTH, VOLUME_NAME_V + 16);
	if (fMessage->FindString("name", &name) == B_NO_ERROR)
		fVolumeName = new BTextControl(r, "", VOLUME_NAME_TEXT, name,
								   new BMessage(M_TEXT));
	else
		fVolumeName = new BTextControl(r, "", VOLUME_NAME_TEXT, "untitled",
								   new BMessage(M_TEXT));
	fVolumeName->SetDivider(font.StringWidth(VOLUME_NAME_TEXT) + 4);
	fVolumeName->SetFont(&font);
	((BTextView *)fVolumeName->ChildAt(0))->SetMaxBytes(B_OS_NAME_LENGTH - 1);
	box->AddChild(fVolumeName);
	
	r.Set(BUTTON_OK_H, BUTTON_OK_V,
		  BUTTON_OK_H + BUTTON_WIDTH, BUTTON_OK_V + BUTTON_HEIGHT);
	box->AddChild(new BButton(r, "", BUTTON_OK_TEXT, new BMessage(M_OK)));

	r.Set(BUTTON_CANCEL_H, BUTTON_CANCEL_V,
		  BUTTON_CANCEL_H + BUTTON_WIDTH, BUTTON_CANCEL_V + BUTTON_HEIGHT);
	box->AddChild(new BButton(r, "", BUTTON_CANCEL_TEXT, new BMessage(M_CANCEL)));

	Show();
}

//--------------------------------------------------------------------

void InitWindow::MessageReceived(BMessage *msg)
{
	bool			result = FALSE;
	int32			partition;
	char			str[256];
	char			temp[256];
	BWindow			*wind;
	uchar			*partition_code = NULL;

	switch (msg->what) {
		case M_OK:
			fMessage->FindInt32("partition", &partition);
			if (partition >= 0)
				strcpy(temp, "partition");
			else
				strcpy(temp, "device");
			sprintf(str, "Initializing will destroy all data on this %s.", temp);
			if ((new BAlert("", str, "Initialize", "Cancel"))->Go()) {
				return;
			}
			if (Initialize() != B_NO_ERROR) {
				sprintf(str, "Error initializing %s.", temp);
				(new BAlert("", str, "Bummer"))->Go();
				return;
			}
			fMessage->FindPointer("part_code", (void **)&partition_code);
			if (partition_code) *partition_code = 0xeb;
			result = TRUE;
			// fall through
		case M_CANCEL:
			fMessage->FindPointer("window", (void **)&wind);
			fMessage->AddPointer("part_window", this);
			fMessage->AddInt32("part_looper_thread", Thread());
			fMessage->AddBool("result", result);
			if (wind != NULL) {
				wind->PostMessage(fMessage);
			}
			delete fMessage;
			Quit();
			break;

		case M_MENU:
			msg->FindInt32("block", &fSize);
			break;

		default:
			BWindow::MessageReceived(msg);
	}
}

//--------------------------------------------------------------------

status_t InitWindow::Initialize(void)
{
	BPath		path;
	char		command[(B_OS_NAME_LENGTH + B_FILE_NAME_LENGTH) + 20];
	status_t	err;

	err = find_directory (B_BEOS_BIN_DIRECTORY, &path);
	if (err != B_OK)
		return err;
	sprintf(command, "mkbfs %s %d \"%s\"\n",
						fDevice.String(), fSize, fVolumeName->Text());
	path.Append (command);
	return system(path.Path());
}
