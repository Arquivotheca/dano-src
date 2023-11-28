//--------------------------------------------------------------------
//	
//	dos.cpp
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

#include <drive_setup.h>

#include <Drivers.h>
#include <FindDirectory.h>
#include <MenuBar.h>
#include <Path.h>
#include <Screen.h>

#include "dos.h"

//====================================================================

static uint32 read32(uchar data[4])
{
	return data[3] << 24 | data[2] << 16 | data[1] << 8 | data[0];
}

static uint32 read16(uchar data[2])
{
	return data[1] << 8 | data[0];
}

//====================================================================

// scans for volume label in a block of directory entries, returning true
// if either at the end of the directory list or if a label was found
static bool find_label(uchar *block, int32 entries, char *label)
{
	for (;entries;entries--,block+=0x20) {
		if (block[0] == 0)	// end of list
			return true;
			
		if (block[0] == 0xe5)	// erased entry
			continue;
		
		if (block[0x0b] == 0xf)	// long file name entry
			continue;
			
		if ((block[0x0b] & 8) == 0)	// check volume label bit
			continue;

		memcpy(label, block, 11);
		label[11] = 0;

		return true;
	}
	return false;
}

//====================================================================

bool ds_fs_id(partition_data *part, int32 dev, uint64 offset, int32 block_size)
{
	bool	result = FALSE;
	uchar	*block;
	int		bps;
	uint64	base;

	block = (uchar *)malloc(2048);
	if (!block) return FALSE;

	/* block_size and part->logical_block_size are valid only when used with
	 * offset and part->offset; the actual block size of the device may
	 * differ. In the case of the dos file system, it makes sense to use the
	 * block size stored in the boot sector. */
	base = offset * block_size + part->offset * part->logical_block_size;
	lseek(dev, base, 0);

	if ((read(dev, block, 512) == 512) &&
		// check for initial jmp short
		((block[0] == 0xeb) || (block[0] == 0xe9)) &&
		// check bytes/sector is 512, 1024, or 2048
		((read16(block+0xb) == 512) || (read16(block+0xb) == 1024)|| (read16(block+0xb) == 2048)) &&
		// check media descriptor versus known types
		((block[0x15] == 0xF0) || (block[0x15] >= 0xf8)) &&
		// check root entries are a multiple of 0x10
		((block[0x11] & 0xf) == 0) &&
		// only check boot block signature on hard disks to work around
		// mtools bug (mformat doesn't write the signature)
		((block[0x15] != 0xf8) || (read16(block+0x1fe) == 0xaa55))) {

		/* NTFS has its own plugin */
		if (!memcmp(block+3, "NTFS    ", 8))
			return false;

		// ditto for HPFS
		if (!memcmp(block+3, "HPFS    ", 8) ||
		    !memcmp(block+3, "OS2 ", 4)) {
			// should do more checking for hpfs...
			sprintf(part->file_system_short_name, "hpfs");
			sprintf(part->file_system_long_name, "hpfs");
			part->volume_name[0] = 0;
			free(block);
			return true;
		}

		bps = read16(block+0xb);

		sprintf(part->file_system_short_name, "dos");
		sprintf(part->file_system_long_name, "dos");
		strcpy(part->volume_name, "no name    "); /* default name */
		result = true;

		int32 reserved_sectors = read16(block+0xe);
		int32 num_fats = block[0x10];
		int32 sectors_per_fat = read16(block+0x16);
		int32 sectors_per_cluster = block[0xd];

		if (block[0x26] == 0x29) {
			// volume label for dos4 boot records may be stored
			// in the bpb; only copy if it is non-blank
			int i;
			for (i=0;i<11;i++)
				if (block[0x2b+i] != ' ')
					break;
			if (i < 11) {
				memcpy(part->volume_name, block + 0x2b, 11);
				part->volume_name[11] = 0;
			}
		}

		if (sectors_per_fat == 0) {
			// fat32 volume labels stored as directory entries
			int32 cluster = read32(block+0x2c);
			uchar *dir = (uchar *)malloc(sectors_per_cluster * bps);
			sectors_per_fat = read32(block+0x24);
			// search a reasonable number of clusters
			for (int i=0;i<5;i++) {
				// read in the cluster
				lseek(dev, base + bps * (reserved_sectors + num_fats*sectors_per_fat + (cluster - 2)*sectors_per_cluster), 0);
				if (read(dev, dir, bps*sectors_per_cluster) != bps * sectors_per_cluster)
					break;
				if (find_label(dir, bps*sectors_per_cluster/0x20, part->volume_name))
					break;

				// look up next cluster entry in the fat
				lseek(dev, base + reserved_sectors*bps + cluster/0x80*0x200, 0);
				if (read(dev, block, 0x200) != 0x200)
					break;
				cluster = read32(block+4*(cluster%(0x200/4))) & 0xfffffff;

				// quit if reserved
				if (cluster >= 0xffffff0)
					break;
			}
			free(dir);
			
			goto bi;
		}

		// scan fat12 and fat16 root directory for label
		int32 root_entries = read16(block+0x11);
		lseek(dev, base + bps*(reserved_sectors + num_fats*sectors_per_fat), 0);
		while (root_entries) {
			if (read(dev, block, 0x200) != 0x200)
				goto bi;
			if (find_label(block, 0x10, part->volume_name))
				break;
			root_entries -= 0x10;
		}
	}

bi:	if (result) {
		// make the volume label prettier
		// IMPORTANT: this has to be the same name returned by the
		// file system addon or else renaming the volume will not
		// rename the link in /
		int i;
		for (i=0;i<11;i++)
			if ((part->volume_name[i] >= 'A') && (part->volume_name[i] <= 'Z'))
				part->volume_name[i] += 'a' - 'A';
		for (i=10;i>0;i--)
			if (part->volume_name[i] != ' ')
				break;
		part->volume_name[i+1] = 0;
	}

	free(block);
	return result;
}

//--------------------------------------------------------------------

void ds_fs_flags(drive_setup_fs_flags *flags)
{
	flags->can_initialize = FALSE;
	flags->has_options = FALSE;
}

//--------------------------------------------------------------------

void ds_fs_initialize(BMessage *msg)
{
	BRect		r;
	BWindow		*wind;
	InitWindow	*window;

	msg->FindPointer("window", (void **) &wind);
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

	const char *tmpDeviceName;
	if ((msg->FindString("device", &tmpDeviceName) != B_NO_ERROR) ||
		(msg->FindInt32("block_size", &fBlockSize) != B_NO_ERROR)) {
		(new BAlert("", "Error in message data passed to dos add-on.", "Sorry"))->Go();
		Run();
		PostMessage(new BMessage(M_CANCEL));
		return;
	}
	fDevice = tmpDeviceName;

	fMessage = msg;
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

	int fd;
	device_geometry geometry;
	off_t blocks;
	
	if ((fd = open(fDevice.String(), O_RDONLY)) < 0) {
		(new BAlert("error", "cannot open device\n", "oops"))->Go();
		Run();
		PostMessage(new BMessage(M_CANCEL));
		return;
	}
	if (ioctl(fd, B_GET_GEOMETRY, &geometry) < 0) {
		close(fd);
		(new BAlert("error", "cannot get device geometry\n", "oops"))->Go();
		Run();
		PostMessage(new BMessage(M_CANCEL));
		return;
	}
	close(fd);
	
	blocks = geometry.cylinder_count * geometry.sectors_per_track * geometry.head_count;
	
	message = new BMessage(M_MENU);
	message->AddInt32("fatbits", 0);
	if (blocks < 32768) {
		message = new BMessage(M_MENU);
		message->AddInt32("fatbits", 12);
		menu->AddItem(item = new BMenuItem("12", message));
		item->SetMarked(true);
		fSize = 12;
	}
	
	if (blocks < 4194304) {
		message = new BMessage(M_MENU);
		message->AddInt32("fatbits", 16);
		menu->AddItem(item = new BMenuItem("16", message));
		if (blocks > 32767) {
			item->SetMarked(true);
			fSize = 16;
		}
	}

	message = new BMessage(M_MENU);
	message->AddInt32("fatbits", 32);
	menu->AddItem(item = new BMenuItem("32", message));
	if (blocks > 4194304) {
		item->SetMarked(true);
		fSize = 32;
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
	((BTextView *)fVolumeName->ChildAt(0))->SetMaxBytes(11);
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
	int64			offset, blocks;
	uchar			code;
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
			if ((new BAlert("", str, "Initialize", "Cancel"))->Go())
				return;
			if (Initialize() != B_NO_ERROR) {
				sprintf(str, "Error initializing %s.", temp);
				(new BAlert("", str, "Bummer"))->Go();
				return;
			}
			result = TRUE;
			fMessage->FindInt64("offset", &offset);
			fMessage->FindInt64("blocks", &blocks);
			/* determine type code */
			if (fSize == 12) {
				code = 0x01;
			} else if (fSize == 16) {
				/* size < 32 MB */
				if (offset >= 8LL*1024*1024*(1024/512))
					code = 0x0e; /* FAT16 LBA */
				else if (blocks >= 32*1024*(1024/512))
					code = 0x06; /* FAT16 >= 32 MB */
				else
					code = 0x04; /* FAT16 < 32 MB */
			} else {
				if (offset >= 8LL*1024*1024*(1024/512))
					code = 0x0c; /* FAT32 LBA */
				else
					code = 0x0b; /* FAT32 CHS */
			}
			fMessage->FindPointer("part_code", (void **) &partition_code);
			if (partition_code) *partition_code = code;
			// fall through
		case M_CANCEL:
			fMessage->FindPointer("window", (void **) &wind);
			fMessage->AddPointer("part_window", (void *) this);
			fMessage->AddInt32("part_looper_thread", Thread());
			fMessage->AddBool("result", result);
			if (wind != NULL) {
				wind->PostMessage(fMessage);
			}
			delete fMessage;
			Quit();
			break;

		case M_MENU:
			msg->FindInt32("fatbits", &fSize);
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
	sprintf(command, "mkdos -n -f %d %s \"%s\"\n",
						fSize, fDevice.String(), fVolumeName->Text());
	path.Append (command);
	return system(path.Path());
}
