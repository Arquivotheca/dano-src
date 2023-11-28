//--------------------------------------------------------------------
//	
//	apple.cpp
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
#include <MenuBar.h>
#include <Screen.h>
#include <ByteOrder.h>

#ifndef APPLE_H
#include "apple.h"
#endif


//====================================================================

bool ds_partition_id(uchar *sb, int32 size)
{
	if (B_BENDIAN_TO_HOST_INT16(((Block0 *)sb)->sbSig) == sbSIGWord)
		return TRUE;
	return FALSE;
}

//--------------------------------------------------------------------

char *ds_partition_name(void)
{
	return "apple";
}

//--------------------------------------------------------------------

status_t ds_get_nth_map(int32 dev, uchar *sb, uint64 block_num, int32 block_size,
						int32 index, partition_data *partition)
{
	int32		size;
	status_t	result = B_ERROR;
	Partition	*block;

    if (B_BENDIAN_TO_HOST_INT16(((Block0 *)sb)->sbSig) == sbSIGWord) {

	block = (Partition *)malloc(block_size);
	lseek(dev, block_num * block_size, 0);
	if ((read(dev, block, block_size) >= 0) && (B_BENDIAN_TO_HOST_INT16(((Block0 *)block)->sbSig) == sbSIGWord)) {
		size = B_BENDIAN_TO_HOST_INT16(((Block0 *)block)->sbBlkSize);
		if (size > block_size) {
			free(block);
			block = (Partition *)malloc(size);
		}
	}
	else
		goto exit;
	lseek(dev, (block_num * block_size) + (1 * size), 0);
	if ((read(dev, block, size) >= 0) && (B_BENDIAN_TO_HOST_INT16(block->pmSig) == pMapSIG) &&
		(index >= 0) && (index <= (B_BENDIAN_TO_HOST_INT32(block->pmMapBlkCnt) - 1))) {
		lseek(dev, (block_num * block_size) + ((1 + index) * size), 0);
		if ((read(dev, block, size) >= 0) && (B_BENDIAN_TO_HOST_INT16(block->pmSig) == pMapSIG)) {
			partition->blocks = B_BENDIAN_TO_HOST_INT32(block->pmPartBlkCnt);
			partition->offset = B_BENDIAN_TO_HOST_INT32(block->pmPyPartStart);
			partition->logical_block_size = size;
			memcpy(partition->partition_name, &block->pmPartName, sizeof(block->pmPartName));
			memcpy(partition->partition_type, &block->pmParType, sizeof(block->pmParType));
			if ((!strcmp((char *)block->pmParType, "Apple_MFS")) ||
				(!strcmp((char *)block->pmParType, "Apple_HFS")) ||
				(!strcmp((char *)block->pmParType, "Apple_UNIX_SVR2")) ||
				(!strcmp((char *)block->pmParType, "Apple_Unix_SVR2")) ||
				(!strcmp((char *)block->pmParType, "Apple_PRODOS")) ||
				(!strcmp((char *)block->pmParType, "Apple_Free")) ||
				(!strcmp((char *)block->pmParType, "Be_BFS")) ||
				(!strcmp((char *)block->pmParType, "BeOS")))
				partition->hidden = FALSE;
			else
				partition->hidden = TRUE;
			partition->partition_code = 0;
			result = B_NO_ERROR;
		}
	}
exit:
	free(block);
    }
	return result;
}

//--------------------------------------------------------------------

void ds_partition_flags(drive_setup_partition_flags *flags)
{
	flags->can_partition = TRUE;
	flags->can_repartition = FALSE;
}

//--------------------------------------------------------------------

status_t ds_update_map(int32 dev, int32 index, partition_data *partition)
{
	Partition	*block;
	status_t	result = B_NO_ERROR;

	block = (Partition *)malloc(partition->logical_block_size);
	lseek(dev, (index + 1) * partition->logical_block_size, 0);
	if ((read(dev, block, partition->logical_block_size) >= 0) &&
		(strcmp((char *)block->pmParType, "Be_BFS"))) {
		strncpy((char *)block->pmParType, "Be_BFS", sizeof(block->pmParType));
		lseek(dev, (index + 1) * partition->logical_block_size, 0);
		result = write(dev, block, partition->logical_block_size);
		if (result >= 0) {
			result = B_NO_ERROR;
			strcpy(partition->partition_type, "Be_BFS");
		}
	}
	free(block);
	return result;
}

//--------------------------------------------------------------------

void ds_partition(BMessage *msg)
{
	BRect		r;
	BWindow		*wind;
	ApplePartWindow	*window;

	msg->FindPointer("window", (void **) &wind);
	r = wind->Frame();
	r.left += ((r.Width() - WIND_WIDTH) / 2);
	r.right = r.left + WIND_WIDTH;
	r.top += ((r.Height() - WIND_HEIGHT) / 2);
	r.bottom = r.top + WIND_HEIGHT;

	BScreen		screen( wind );
	BRect		screen_frame = screen.Frame();
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
	window = new ApplePartWindow(r, msg);
}


//====================================================================

ApplePartWindow::ApplePartWindow(BRect rect, BMessage *msg)
				:BWindow(rect, "", B_MODAL_WINDOW, B_NOT_RESIZABLE)
{
	bool			is_apple = FALSE;
	const char		*str;
	char			type[256];
	int32			len;
	int32			result;
	BButton			*button;
	BFont			font;
	BMenu			*menu;
	BMessage		*message;
	BMenuField		*field;
	BRect			r;
	BStringView		*string;
	AppleBox		*box;
	device_geometry	geometry;

	fMessage = msg;
	if (msg->FindInt32("dev", &fDevice) == B_NO_ERROR) {
		msg->FindBool("read_only", &fReadOnly);
		msg->FindInt64("offset", (int64 *)&fOffset);
		ioctl(fDevice, B_GET_GEOMETRY, &geometry);
		fBlockSize = geometry.bytes_per_sector;
		msg->FindInt64("blocks", (int64 *)&fSize);
		fSize *= fBlockSize;
		lseek(fDevice, fOffset * fBlockSize, 0);
		result = read(fDevice, &fSB, sizeof(Block0));
		if ((result >= 0) && (B_BENDIAN_TO_HOST_INT16(fSB.sbSig) == sbSIGWord)) {
			is_apple = TRUE;
			fLogicalSize = B_BENDIAN_TO_HOST_INT16(fSB.sbBlkSize);
		}
		else
			fLogicalSize = BLOCK_SIZE;
	}
	else {
		(new BAlert("", "Error in message data passed to Apple add-on.", "Sorry"))->Go();
		Run();
		PostMessage(new BMessage(M_CANCEL));
		return;
	}

	r = Frame();
	r.OffsetTo(0, 0);
	r.InsetBy(-1, -1);
	r.top--;
	AddChild(box = new AppleBox(r));

	fMessage->FindString("device", &str);
	r.Set(TITLE_H, TITLE_V, TITLE_H + TITLE_WIDTH, TITLE_V + 16);
	sprintf(type, "%s - %s", str, TITLE_TEXT);
	string = new BStringView(r, "", type);
	box->AddChild(string);
	font = *be_bold_font;
	font.SetSize(12.0);
	string->SetFont(&font);

	font = *be_plain_font;
	font.SetSize(10.0);
	r.Set(LAYOUT_MENU_H, LAYOUT_MENU_V,
		  LAYOUT_MENU_H + LAYOUT_MENU_WIDTH, LAYOUT_MENU_V + 20);
	menu = new BPopUpMenu("Layout");
	menu->SetFont(&font);
	menu->SetRadioMode(FALSE);
	message = new BMessage(M_LAYOUT);
	message->AddInt32("type", 0);
	menu->AddItem(new BMenuItem(PART_100_BE, message));
	message = new BMessage(M_LAYOUT);
	message->AddInt32("type", 1);
	menu->AddItem(new BMenuItem(PART_100_HFS, message));
	message = new BMessage(M_LAYOUT);
	message->AddInt32("type", 2);
	menu->AddItem(new BMenuItem(PART_50, message));
	message = new BMessage(M_LAYOUT);
	message->AddInt32("type", 3);
	menu->AddItem(new BMenuItem(PART_25, message));
	field = new BMenuField(r, "", "", menu);
	field->SetDivider(0.0);
	field->SetFont(&font);
	field->MenuBar()->SetFont(&font);
	field->SetEnabled(!fReadOnly);
	box->AddChild(field);

	font.SetSize(11.0);
	r.Set(LIST_H + 2 + LABEL_PARTITION_H, LABEL_V, LABEL_FS_H, LABEL_V + 13);
	string = new BStringView(r, "", LABEL_PARTITION_TEXT);
	box->AddChild(string);
	string->SetFont(&font);

	r.Set(LIST_H + 2 + LABEL_FS_H, LABEL_V, LABEL_VOLUME_H, LABEL_V + 13);
	string = new BStringView(r, "", LABEL_FS_TEXT);
	box->AddChild(string);
	string->SetFont(&font);

	r.Set(LIST_H + 2 + LABEL_VOLUME_H, LABEL_V, LABEL_SIZE_H, LABEL_V + 13);
	string = new BStringView(r, "", LABEL_VOLUME_TEXT);
	box->AddChild(string);
	string->SetFont(&font);

	r.Set(LIST_H + 2 + LABEL_SIZE_H, LABEL_V, LIST_H + LIST_WIDTH, LABEL_V + 13);
	string = new BStringView(r, "", LABEL_SIZE_TEXT);
	box->AddChild(string);
	string->SetAlignment(B_ALIGN_RIGHT);
	string->SetFont(&font);

	r.Set(LIST_H, LIST_V, LIST_H + LIST_WIDTH, LIST_V + LIST_HEIGHT);
	fList = new TListView(r);
	fList->SetInvocationMessage(new BMessage(M_LIST_INVOKED));
	fList->SetSelectionMessage(new BMessage(M_LIST_SELECTED));
	box->AddChild(new BScrollView("", fList, B_FOLLOW_ALL, B_WILL_DRAW, FALSE, TRUE,
										B_FANCY_BORDER));

	font = *be_plain_font;
	font.SetSize(10.0);
	r.Set(PART_NAME_H, PART_NAME_V,
		  PART_NAME_H + PART_NAME_WIDTH, PART_NAME_V + 16);
	box->AddChild(fPartName = new BTextControl(r, "", PART_NAME_TEXT, "",
											   new BMessage(M_PART_NAME)));
	fPartName->SetFont(&font);
	fPartName->SetDivider(font.StringWidth(PART_NAME_TEXT) + 5);

	r.Set(PART_TYPE_H, PART_TYPE_V,
		  PART_TYPE_H + PART_TYPE_WIDTH, PART_TYPE_V + 16);
	box->AddChild(fPartType = new BTextControl(r, "", PART_TYPE_TEXT, "",
											   new BMessage(M_PART_TYPE)));
	fPartType->SetFont(&font);
	fPartType->SetDivider(font.StringWidth(PART_NAME_TEXT) + 5);

	r.Set(PART_MENU_H, PART_MENU_V,
		  PART_MENU_H + PART_MENU_WIDTH, PART_MENU_V + 20);
	fPartMenu = new BPopUpMenu("Type");
	fPartMenu->SetFont(&font);
	fPartMenu->SetRadioMode(FALSE);
	message = new BMessage(M_PART_MENU);
	message->AddString("name", "Apple");
	message->AddString("type", "Apple_partition_map");
	fPartMenu->AddItem(new BMenuItem("Partition Map", message));
	message = new BMessage(M_PART_MENU);
	message->AddString("name", "Maci_Driver");
	message->AddString("type", "Apple_Driver43");
	fPartMenu->AddItem(new BMenuItem("Driver", message));
	message = new BMessage(M_PART_MENU);
	message->AddString("name", "Be_BFS");
	message->AddString("type", "Be_BFS");
	fPartMenu->AddItem(new BMenuItem("BeOS", message));
	message = new BMessage(M_PART_MENU);
	message->AddString("name", "MacOS");
	message->AddString("type", "Apple_HFS");
	fPartMenu->AddItem(new BMenuItem("Apple HFS", message));
	message = new BMessage(M_PART_MENU);
	message->AddString("name", "extra");
	message->AddString("type", "Apple_Free");
	fPartMenu->AddItem(new BMenuItem("Apple Free", message));
	field = new BMenuField(r, "", "", fPartMenu);
	field->MenuBar()->SetFont(&font);
	field->SetDivider(0.0);
	box->AddChild(field);

	r.Set(SLIDER_H, SLIDER_V, SLIDER_H + SLIDER_WIDTH, SLIDER_V + SLIDER_HEIGHT);
	fSlider = new TSliderView(r, 0, fSize, 0);
	box->AddChild(fSlider);

	r.Set(BUTTON_UPDATE_H, BUTTON_UPDATE_V,
		  BUTTON_UPDATE_H + BUTTON_WIDTH, BUTTON_UPDATE_V + BUTTON_HEIGHT);
	box->AddChild(fUpdate = new BButton(r, "", BUTTON_UPDATE_TEXT, new BMessage(M_UPDATE)));
	fUpdate->SetEnabled(FALSE);

	r.Set(BUTTON_ITEM_H, BUTTON_ITEM_V,
		  BUTTON_ITEM_H + BUTTON_WIDTH, BUTTON_ITEM_V + BUTTON_HEIGHT);
	box->AddChild(fItem = new BButton(r, "", BUTTON_ITEM_TEXT, new BMessage(M_ITEM)));
	fItem->SetEnabled(FALSE);

	r.Set(BUTTON_REMOVE_H, BUTTON_REMOVE_V,
		  BUTTON_REMOVE_H + BUTTON_WIDTH, BUTTON_REMOVE_V + BUTTON_HEIGHT);
	box->AddChild(fRemove = new BButton(r, "", BUTTON_REMOVE_TEXT, new BMessage(M_REMOVE)));

	r.Set(BUTTON_ADD_H, BUTTON_ADD_V,
		  BUTTON_ADD_H + BUTTON_WIDTH, BUTTON_ADD_V + BUTTON_HEIGHT);
	box->AddChild(fAdd = new BButton(r, "", BUTTON_ADD_TEXT, new BMessage(M_ADD)));
	fAdd->SetEnabled(FALSE);

	r.Set(BUTTON_PARTITION_H, BUTTON_PARTITION_V,
		  BUTTON_PARTITION_H + BUTTON_WIDTH, BUTTON_PARTITION_V + BUTTON_HEIGHT);
	button = new BButton(r, "", BUTTON_PARTITION_TEXT, new BMessage(M_OK));
	button->MakeDefault(TRUE);
	if (fReadOnly)
		button->SetEnabled(FALSE);
	box->AddChild(button);

	r.Set(BUTTON_CANCEL_H, BUTTON_CANCEL_V,
		  BUTTON_CANCEL_H + BUTTON_WIDTH, BUTTON_CANCEL_V + BUTTON_HEIGHT);
	box->AddChild(new BButton(r, "", BUTTON_CANCEL_TEXT, new BMessage(M_CANCEL)));

	r.Set(BUTTON_REVERT_H, BUTTON_REVERT_V,
		  BUTTON_REVERT_H + BUTTON_WIDTH, BUTTON_REVERT_V + BUTTON_HEIGHT);
	box->AddChild(fRevert = new BButton(r, "", BUTTON_REVERT_TEXT, new BMessage(M_REVERT)));
	fRevert->SetEnabled(FALSE);

	if ((is_apple) && (fMessage->FindPointer("list", (void **) &fOrig) == B_NO_ERROR))
		UpdateList();
	else
		fOrig = FALSE;
	CheckFree();
	Show();
}

//--------------------------------------------------------------------

ApplePartWindow::~ApplePartWindow(void)
{
	ClearList();
}

//--------------------------------------------------------------------

void ApplePartWindow::MessageReceived(BMessage *msg)
{
	bool			result = FALSE;
	const char		*str;
	int32			type;
	int32			loop;
	BRect			r;
	BWindow			*wind;
	TListItem		*item;
	partition_data	*info;

	switch (msg->what) {
		case M_OK:
			result = (new BAlert("", "Changing the partition map may "\
"destroy all data on this disk.", "Proceed", "Cancel"))->Go();
			if (result)
				return;
			if (WriteMap() == B_NO_ERROR)
				result = TRUE;
			else {
				(new BAlert("", "Error writing partition map to disk.",
								"Bummer"))->Go();
				return;
			}
		case M_CANCEL:
			fMessage->FindPointer("window", (void **) &wind);
			fMessage->AddPointer("part_window", this);
			fMessage->AddInt32("part_looper_thread", Thread());
			fMessage->AddBool("result", result);
			if (wind != NULL) {
				wind->PostMessage(fMessage);
			}
			delete fMessage;
			Quit();
			break;

		case M_REVERT:
			ClearList();
			UpdateList();
			CheckFree();
			break;

		case M_ADD:
			info = (partition_data *)calloc(1,sizeof(partition_data));
			strcpy(info->partition_name, "Extra");
			strcpy(info->partition_type, "Apple_Free");
			fList->AddItem(new TListItem(info, fLogicalSize));
			fList->Select(fList->CountItems() - 1);
			CheckFree();
			break;
			
		case M_REMOVE:
			fList->RemoveItem(fList->CurrentSelection());
			CheckFree();
			break;

		case M_LIST_SELECTED:
			CheckFree();
			break;

		case M_PART_NAME:
		case M_PART_TYPE:
			fUpdate->SetEnabled(TRUE);
			fItem->SetEnabled(TRUE);
			break;

		case M_PART_MENU:
			msg->FindString("name", &str);
			fPartName->SetText(str);
			msg->FindString("type", &str);
			fPartType->SetText(str);
			fUpdate->SetEnabled(TRUE);
			fItem->SetEnabled(TRUE);
			break;

		case M_SLIDER:
			fUpdate->SetEnabled(TRUE);
			fItem->SetEnabled(TRUE);
			break;
			
		case M_LAYOUT:
			ClearList();
			CheckFree();
			info = (partition_data *)calloc(1,sizeof(partition_data));
			sprintf(info->partition_name, "Apple");
			sprintf(info->partition_type, "Apple_partition_map");
			info->blocks = 63;
			fList->AddItem(new TListItem(info, fLogicalSize));
			fFree -= (63 * fLogicalSize);

			info = (partition_data *)calloc(1,sizeof(partition_data));
			sprintf(info->partition_name, "Maci_Driver");
			sprintf(info->partition_type, "Apple_Driver43");
			info->blocks = 128;
			fList->AddItem(new TListItem(info, fLogicalSize));
			fFree -= (128 * fLogicalSize);

			msg->FindInt32("type", &type);
			switch (type) {
				case 0:
					info = (partition_data *)calloc(1,sizeof(partition_data));
					sprintf(info->partition_name, "Be_BFS");
					sprintf(info->partition_type, "Be_BFS");
					info->blocks = min_c(fFree / fLogicalSize, 0xffffffff);
					fList->AddItem(new TListItem(info, fLogicalSize));
					break;

				case 1:
					info = (partition_data *)calloc(1,sizeof(partition_data));
					sprintf(info->partition_name, "MacOS");
					sprintf(info->partition_type, "Apple_HFS");
					info->blocks = min_c(fFree / fLogicalSize, 0xffffffff);
					fList->AddItem(new TListItem(info, fLogicalSize));
					break;

				case 2:
					info = (partition_data *)calloc(1,sizeof(partition_data));
					sprintf(info->partition_name, "Be_BFS");
					sprintf(info->partition_type, "Be_BFS");
					info->blocks = min_c((fFree / fLogicalSize) / 2, 0xffffffff);
					fList->AddItem(new TListItem(info, fLogicalSize));
					fFree -= (info->blocks * fLogicalSize);

					info = (partition_data *)calloc(1,sizeof(partition_data));
					sprintf(info->partition_name, "MacOS");
					sprintf(info->partition_type, "Apple_HFS");
					info->blocks = min_c(fFree / fLogicalSize, 0xffffffff);
					fList->AddItem(new TListItem(info, fLogicalSize));
					break;

				case 3:
					for (loop = 0; loop < 4; loop++) {
						info = (partition_data *)calloc(1,sizeof(partition_data));
						strcpy(info->partition_name, "Extra");
						strcpy(info->partition_type, "Apple_Free");
						info->blocks = min_c((fFree / fLogicalSize) / 4, 0xffffffff);
						fList->AddItem(new TListItem(info, fLogicalSize));
					}
					break;
			}
			CheckFree();
			break;

		case M_UPDATE:
			item = (TListItem *)fList->ItemAt(fList->CurrentSelection());
			info = item->PartitionInfo();
			strcpy(info->partition_name, fPartName->Text());
			strcpy(info->partition_type, fPartType->Text());
			strcpy(info->file_system_short_name, "");
			strcpy(info->file_system_long_name, "");
			strcpy(info->volume_name, "");
			info->blocks = fSlider->Value() / fLogicalSize;
			r = fList->ItemFrame(fList->CurrentSelection());
			fList->Invalidate(r);
			fUpdate->SetEnabled(FALSE);
			fItem->SetEnabled(FALSE);
			CheckFree();
			break;

		case M_ITEM:
			item = (TListItem *)fList->ItemAt(fList->CurrentSelection());
			info = item->PartitionInfo();
			fPartName->SetText(info->partition_name);
			fPartType->SetText(info->partition_type);
			fSlider->SetRange(0, fFree + (info->blocks * fBlockSize));
			fSlider->SetValue(info->blocks * fBlockSize);
			fUpdate->SetEnabled(FALSE);
			fItem->SetEnabled(FALSE);
			break;

		default:
			BWindow::MessageReceived(msg);
	}
}

//--------------------------------------------------------------------

void ApplePartWindow::ClearList(void)
{
	TListItem	*item;

	Lock();
	for (;;) {
		item = (TListItem *)fList->FirstItem();
		if (item) {
			fList->RemoveItem((int32)0);
			delete item;
		}
		else
			break;
	}
	Unlock();
}

//--------------------------------------------------------------------

void ApplePartWindow::CheckFree(void)
{
	bool			enable;
	bool			revert = FALSE;
	int32			count;
	int32			loop;
	TListItem		*item;
	partition_data	*info;
	partition_data	*orig;

	fFree = fSize;
	count = fList->CountItems();
	if (((fOrig) && (count != fOrig->CountItems())) || ((!fOrig) && (count)))
		revert = TRUE;
	for (loop = 0; loop < count; loop++) {
		item = (TListItem *)fList->ItemAt(loop);
		info = item->PartitionInfo();
		fFree -= info->blocks * fBlockSize;

		if (fOrig) {
			orig = (partition_data *)fOrig->ItemAt(loop);
			if (orig) {
				if ((strcmp(orig->partition_name, info->partition_name)) ||
				   (strcmp(orig->partition_type, info->partition_type)) ||
				   (orig->blocks != info->blocks))
				revert = TRUE;
			}
			else
				revert = TRUE;
		}
		else
			revert = TRUE;
	}
	fAdd->SetEnabled((fFree >= 1024) && (!fReadOnly));
	enable = fList->CurrentSelection() >= 0;
	fRemove->SetEnabled((enable) && (!fReadOnly));
	fPartName->SetEnabled((enable) && (!fReadOnly));
	fPartType->SetEnabled((enable) && (!fReadOnly));
	fPartMenu->SetEnabled((enable) && (!fReadOnly));
	fSlider->SetEnabled((enable) && (!fReadOnly));
	fUpdate->SetEnabled(FALSE);
	fItem->SetEnabled(FALSE);
	if (!enable) {
		fPartName->SetText("");
		fPartType->SetText("");
	}
	else {
		item = (TListItem *)fList->ItemAt(fList->CurrentSelection());
		info = item->PartitionInfo();
		fPartName->SetText(info->partition_name);
		fPartType->SetText(info->partition_type);
		fSlider->SetRange(0, fFree + (info->blocks * fBlockSize));
		fSlider->SetValue(info->blocks * fBlockSize);
	}
	fRevert->SetEnabled((revert) && (!fReadOnly));
}

//--------------------------------------------------------------------

void ApplePartWindow::UpdateList(void)
{
	int32			loop;
	partition_data	*info;

	if (fOrig) {
		for (loop = 0; loop < fOrig->CountItems(); loop++) {
			info = (partition_data *)malloc(sizeof(partition_data));
			memcpy(info, fOrig->ItemAt(loop), sizeof(partition_data));
			fList->AddItem(new TListItem(info, fLogicalSize));
		}
	}
}

//--------------------------------------------------------------------

int32 ApplePartWindow::WriteMap(void)
{
	int32			block;
	int32			items;
	int32			part_items;
	int32			loop;
	int32			skip = -1;
	int64			free;
	uint64			offset;
	Block0			orig_sb;
	Block0			sb;
	Partition		orig_part;
	Partition		part;
	partition_data	*info;
	TListItem		*item;

	fFree = fSize - fLogicalSize;
	items = fList->CountItems();
	memset(&part, 0, sizeof(Partition));
	part.pmSig = B_HOST_TO_BENDIAN_INT16(pMapSIG);
	part.pmMapBlkCnt = B_HOST_TO_BENDIAN_INT32(items + 1);
	part.pmPyPartStart = B_HOST_TO_BENDIAN_INT32(1);
	part.pmPartBlkCnt = B_HOST_TO_BENDIAN_INT32(63);
	strcpy((char *)part.pmPartName, "Apple");
	strcpy((char *)part.pmParType, "Apple_partition_map");
	
	free = fFree / fLogicalSize;
	for (loop = 0; loop < items; loop++) {
		item = (TListItem *)fList->ItemAt(loop);
		info = item->PartitionInfo();
		free -= info->blocks;
		if ((skip < 0) && (!strcmp(info->partition_type, "Apple_partition_map"))) {
			strncpy((char *)part.pmPartName, info->partition_name, sizeof(part.pmPartName));
			part.pmPartBlkCnt = B_HOST_TO_BENDIAN_INT32(info->blocks);
			skip = loop;
			part.pmMapBlkCnt = B_HOST_TO_BENDIAN_INT32(B_BENDIAN_TO_HOST_INT32(part.pmMapBlkCnt)-1);
		}
	}
	if (free > 0)
		part.pmMapBlkCnt = B_HOST_TO_BENDIAN_INT32(B_BENDIAN_TO_HOST_INT32(part.pmMapBlkCnt)+1);
	if (B_BENDIAN_TO_HOST_INT32(part.pmPartBlkCnt) < B_BENDIAN_TO_HOST_INT32(part.pmMapBlkCnt))
		part.pmPartBlkCnt = part.pmMapBlkCnt;

	fFree -= B_BENDIAN_TO_HOST_INT32(part.pmPartBlkCnt) * fLogicalSize;
	offset = 1 + B_BENDIAN_TO_HOST_INT32(part.pmPartBlkCnt);
	part_items = B_BENDIAN_TO_HOST_INT32(part.pmMapBlkCnt);
	lseek(fDevice, (fOffset + 1) * fLogicalSize, 0);
	if (write(fDevice, &part, sizeof(Partition)) < 0)
		return B_ERROR;

	block = 2;
	for (loop = 0; loop < items; loop++) {
		if (loop != skip) {
			item = (TListItem *)fList->ItemAt(loop);
			info = item->PartitionInfo();
			if (info->blocks > (fFree / fLogicalSize))
				free = fFree / fLogicalSize;
			else
				free = info->blocks;
			fFree -= free * fLogicalSize;
			lseek(fDevice, (fOffset * fBlockSize) + (block * fLogicalSize), 0);
			if ((read(fDevice, &orig_part, sizeof(Partition)) >= 0) &&
				(B_BENDIAN_TO_HOST_INT16(orig_part.pmSig) == pMapSIG)) {
				orig_part.pmMapBlkCnt = B_HOST_TO_BENDIAN_INT32(part_items);
				orig_part.pmPyPartStart = B_HOST_TO_BENDIAN_INT32(offset);
				orig_part.pmPartBlkCnt = B_HOST_TO_BENDIAN_INT32(free);
				strncpy((char *)orig_part.pmPartName,
						info->partition_name, sizeof(orig_part.pmPartName));
				strncpy((char *)orig_part.pmParType,
						info->partition_type, sizeof(orig_part.pmParType));
				lseek(fDevice, (fOffset * fBlockSize) + (block * fLogicalSize), 0);
				if (write(fDevice, &orig_part, sizeof(Partition)) < 0)
					return B_ERROR;
			}
			else {
				memset(&part, 0, sizeof(Partition));
				part.pmSig = B_HOST_TO_BENDIAN_INT16(pMapSIG);
				part.pmMapBlkCnt = B_HOST_TO_BENDIAN_INT32(part_items);
				part.pmPyPartStart = B_HOST_TO_BENDIAN_INT32(offset);
				part.pmPartBlkCnt = B_HOST_TO_BENDIAN_INT32(free);
				strncpy((char *)part.pmPartName,
						info->partition_name, sizeof(part.pmPartName));
				strncpy((char *)part.pmParType,
						info->partition_type, sizeof(part.pmParType));
				lseek(fDevice, (fOffset * fBlockSize) + (block * fLogicalSize), 0);
				if (write(fDevice, &part, sizeof(Partition)) < 0)
					return B_ERROR;
			}
			offset += free;
			block++;
		}
	}

	if (fFree) {
		memset(&part, 0, sizeof(Partition));
		part.pmSig = B_HOST_TO_BENDIAN_INT16(pMapSIG);
		part.pmMapBlkCnt = B_HOST_TO_BENDIAN_INT32(part_items);
		part.pmPyPartStart = B_HOST_TO_BENDIAN_INT32(offset);
		part.pmPartBlkCnt = B_HOST_TO_BENDIAN_INT32(fFree / fLogicalSize);
		strcpy((char *)part.pmPartName, "Extra");
		strcpy((char *)part.pmParType, "Apple_Free");
		lseek(fDevice, (fOffset * fBlockSize) + (block * fLogicalSize), 0);
		if (write(fDevice, &part, sizeof(Partition)) < 0)
			return B_ERROR;
	}

	lseek(fDevice, fOffset * fBlockSize, 0);
	if (read(fDevice, &orig_sb, sizeof(Block0)) >= 0) {
		if ((B_BENDIAN_TO_HOST_INT16(orig_sb.sbSig) == sbSIGWord) &&
			(B_BENDIAN_TO_HOST_INT16(orig_sb.sbBlkSize) == fLogicalSize) &&
			(abs((int)(B_BENDIAN_TO_HOST_INT32(orig_sb.sbBlkCount) - (fSize / fLogicalSize))) <= 1))
			return B_NO_ERROR;
	}

	sb.sbSig = B_HOST_TO_BENDIAN_INT16(sbSIGWord);
	sb.sbBlkSize = B_HOST_TO_BENDIAN_INT16(fLogicalSize);
	sb.sbBlkCount = B_HOST_TO_BENDIAN_INT32(fSize / fLogicalSize);
	
	lseek(fDevice, fOffset * fBlockSize, 0);
	if (write(fDevice, &sb, sizeof(Block0)) < 0)
		return B_ERROR;
	return B_NO_ERROR;
}


//====================================================================

TListItem::TListItem(partition_data *info, int32 block_size)
		  :BListItem()
{
	fPartition = info;
	fBlockSize = block_size;
	fHeight = 0.0;
}

//--------------------------------------------------------------------

TListItem::~TListItem(void)
{
	delete fPartition;
}

//--------------------------------------------------------------------

void TListItem::DrawItem(BView *view, BRect where, bool complete)
{
	char			str[256];
	char			mag;
	float			size;

	if (IsSelected())
		view->SetHighColor(SELECT_COLOR, SELECT_COLOR, SELECT_COLOR);
	else
		view->SetHighColor(255, 255, 255);
	view->FillRect(where);
	view->SetHighColor(0, 0, 0);
	view->MovePenTo(where.left + LABEL_PARTITION_H + 2,
					where.top + fHeight - 2);
	fit_string(view, fPartition->partition_type, LABEL_FS_H - LABEL_PARTITION_H - 4);
	view->MovePenTo(where.left + LABEL_FS_H + 2,
					where.top + fHeight - 2);
	fit_string(view, fPartition->file_system_short_name, LABEL_VOLUME_H - LABEL_FS_H - 4);
	view->MovePenTo(where.left + LABEL_VOLUME_H + 2,
					where.top + fHeight - 2);
	fit_string(view, fPartition->volume_name, LABEL_SIZE_H - LABEL_VOLUME_H - 4);
	size = (fBlockSize * fPartition->blocks) / 1024;
	mag = 'K';
	if (size > 1024) {
		size /= 1024;
		mag = 'M';
		if (size > 1024) {
			size /= 1024;
			mag = 'G';
		}
	}
	sprintf(str, "%.1f %cB", size, mag);
	view->MovePenTo(LIST_WIDTH - view->StringWidth(str),
					where.top + fHeight - 2);
	view->DrawString(str);
		
	view->SetHighColor(192, 192, 192);
	view->SetHighColor(102, 152, 203);
	view->StrokeLine(BPoint(where.left, where.bottom),
					 BPoint(where.right, where.bottom));
	view->StrokeLine(BPoint(where.left + LABEL_FS_H, where.top),
					 BPoint(where.left + LABEL_FS_H, where.bottom));
	view->StrokeLine(BPoint(where.left + LABEL_VOLUME_H, where.top),
					 BPoint(where.left + LABEL_VOLUME_H, where.bottom));
	view->StrokeLine(BPoint(where.left + LABEL_SIZE_H, where.top),
					 BPoint(where.left + LABEL_SIZE_H, where.bottom));
	view->Sync();
}

//--------------------------------------------------------------------

void TListItem::Update(BView *view, const BFont *font)
{
	int32	items;

	BListItem::Update(view, font);
	if (!fHeight)
		fHeight = Height();
}

//--------------------------------------------------------------------

partition_data* TListItem::PartitionInfo(void)
{
	return fPartition;
}


//====================================================================

TListView::TListView(BRect rect)
		  :BListView(rect, "")
{
	BFont	font;

	font = *be_plain_font;
	font.SetSize(10.0);
	SetFont(&font);
	SetDrawingMode(B_OP_OVER);
}

//--------------------------------------------------------------------

void TListView::Draw(BRect where)
{
	SetHighColor(255, 255, 255);
	FillRect(where);
	BListView::Draw(where);
}


//====================================================================

AppleBox::AppleBox(BRect rect)
		 :BBox(rect, "", B_FOLLOW_ALL, B_WILL_DRAW, B_FANCY_BORDER)
{
}

//--------------------------------------------------------------------

void AppleBox::Draw(BRect where)
{
	BBox::Draw(where);

	SetHighColor(VIEW_COLOR - 48, VIEW_COLOR - 48, VIEW_COLOR - 48);
	StrokeLine(BPoint(H_LINE_H, LINE1_V),
			   BPoint(H_LINE_H + H_LINE_WIDTH, LINE1_V));
	StrokeLine(BPoint(H_LINE_H, LINE2_V),
			   BPoint(H_LINE_H + H_LINE_WIDTH, LINE2_V));
	SetHighColor(255, 255, 255);
	StrokeLine(BPoint(H_LINE_H, LINE1_V + 1),
			   BPoint(H_LINE_H + H_LINE_WIDTH, LINE1_V + 1));
	StrokeLine(BPoint(H_LINE_H, LINE2_V + 1),
			   BPoint(H_LINE_H + H_LINE_WIDTH, LINE2_V + 1));
	SetHighColor(0, 0, 0);
}


//====================================================================

TSliderView::TSliderView(BRect rect, uint64 min, uint64 max, uint64 current)
			:BView(rect, "", B_FOLLOW_ALL, B_WILL_DRAW | B_NAVIGABLE)
{
	rgb_color	c;

	c.red = c.green = c.blue = VIEW_COLOR;
	SetViewColor(c);

	fMin = min;
	fMax = max;
	fNew = current;
	fEnabled = FALSE;
	fMag = 1024 * 1024;

	fSlider = NULL;
}

//--------------------------------------------------------------------

TSliderView::~TSliderView(void)
{
	if (fSlider)
		delete fSlider;
}

//--------------------------------------------------------------------

void TSliderView::AttachedToWindow(void)
{
	BFont	font;
	BRect	r;

	font = *be_plain_font;
	font.SetSize(10.0);
	SetFont(&font);

	fWidth = font.StringWidth(PART_NAME_TEXT) + 1;
	fOffset = 52;
	r = Bounds();
	r.right -= fWidth + fOffset;
	fSlider = new BBitmap(r, B_COLOR_8_BIT, TRUE);
	fSlider->AddChild(fOffView = new BView(r, "", B_FOLLOW_ALL, B_WILL_DRAW));
}

//--------------------------------------------------------------------

void TSliderView::KeyDown(const char *key, int32 count)
{
	bool		change = FALSE;
	int64		size;
	BMessage	msg;

	switch (key[0]) {
		case B_LEFT_ARROW:
			size = -fMag;
			change = TRUE;
			break;

		case B_RIGHT_ARROW:
			size = fMag;
			change = TRUE;
			break;

		case B_DOWN_ARROW:
			size = -10 * fMag;
			change = TRUE;
			break;

		case B_UP_ARROW:
			size = 10 * fMag;
			change = TRUE;
			break;

		default:
			BView::KeyDown(key, count);
	}

	if ((fEnabled) && (change)) {
		if (size < 0) {
			if (abs((int)size) > fNew)
				fNew = fMin;
			else
				fNew += size;
		}
		else {
			fNew += size;
			if (fNew > fMax)
				fNew = fMax;
		}
		if (fSlider)
			DrawSlider();
		msg.what = M_SLIDER;
		msg.AddInt64("slider", fNew);
		Window()->PostMessage(&msg);
	}
}

//--------------------------------------------------------------------

void TSliderView::Draw(BRect where)
{
	char	str[256];
	BFont	font;
	BRect	r, dr;

	DrawSlider();

	if (fEnabled)
		SetHighColor(0, 0, 0);
	else
		SetHighColor(128, 128, 128);

	r = Bounds();
	font = *be_plain_font;
	font.SetSize(10.0);
	SetFont(&font);
	MovePenTo(SLIDER_TEXT_H, r.top + 20);
	DrawString(SLIDER_TEXT);

	DrawRange(FALSE);
}

//--------------------------------------------------------------------

void TSliderView::MakeFocus(bool focus)
{
	fFocus = focus;
	Draw(Bounds());
	BView::MakeFocus(focus);
}

//--------------------------------------------------------------------

void TSliderView::MouseDown(BPoint where)
{
	uint32		buttons;
	uint64		current;
	uint64		temp;
	float		unit;
	float		offset = 0.0;
	BFont		font;
	BMenuItem	*item;
	BMessage	msg;
	BPoint		point;
	BPoint		parent;
	BPopUpMenu	*menu;
	BRect		back;
	BRect		track;

	if (!fEnabled)
		return;
	MakeFocus(TRUE);

	back = Bounds();
	back.left += fWidth + 5;
	back.right -= fOffset + 5;
	back.top += 13; 
	back.bottom = back.top + 14;

	unit = back.Width() / (float)((fMax - fMin) / fMag);
	(fNew < fMin) ? temp = fMin : temp = fNew;
	(fNew > fMax) ? temp = fMax : temp = fNew;
	track.left = fWidth + (((temp - fMin) / fMag) * unit);
	track.top = back.top + 6;
	track.right = track.left + 10;
	track.bottom = track.top + 14;
	if (track.Contains(where))
		offset = where.x - track.left - 5;
	else if (!back.Contains(where)) {
		back = Bounds();
		back.left = back.right - fOffset + 4;
		back.top += 10;
		back.bottom = back.top + 12;
		if (back.Contains(where)) {
			menu = new BPopUpMenu("");
			menu->AddItem(item = new BMenuItem("Kilobytes", new BMessage(M_KB)));
			if (fMag == 1024)
				item->SetMarked(TRUE);
			menu->AddItem(item = new BMenuItem("Megabytes", new BMessage(M_MB)));
			if (fMag == 1024 * 1024)
				item->SetMarked(TRUE);
			menu->AddItem(item = new BMenuItem("Gigabytes", new BMessage(M_GB)));
			if (fMag == 1024 * 1024 * 1024)
				item->SetMarked(TRUE);
			ConvertToScreen(&back);
			font = *be_plain_font;
			font.SetSize(9.0);
			menu->SetFont(&font);
			item = menu->Go(back.LeftTop(), FALSE);
			if (item) {
				if (item->Command() == M_KB)
					fMag = 1024;
				else if (item->Command() == M_MB)
					fMag = 1024 * 1024;
				else
					fMag = 1024 * 1024 * 1024;
				DrawSlider();
				DrawRange(TRUE);
			}
			delete menu;
		}
		return;
	}

	current = fNew;
	do {
		GetMouse(&point, &buttons);
		parent = point;
		ConvertToParent(&parent);
		if (!Window()->Bounds().Contains(parent))
			temp = current;
		else {
			point.x -= offset;
			if (point.x < back.left)
				point.x = back.left;
			if (point.x > back.right)
				point.x = back.right;
			temp = (((point.x - back.left) * (1 / unit)) * fMag + fMin);
			if (temp > fMax)
				temp = fMax;
		}
		if (temp != fNew) {
			fNew = temp;
			DrawSlider();
		}
	} while (buttons);

	msg.what = M_SLIDER;
	msg.AddInt64("slider", fNew);
	Window()->PostMessage(&msg);
}

//--------------------------------------------------------------------

void TSliderView::DrawRange(bool erase)
{
	char	mag;
	char	str[256];
	BFont	font;
	BRect	r;

	r = Bounds();
	if ((erase) || (!fEnabled)) {
		SetHighColor(VIEW_COLOR, VIEW_COLOR, VIEW_COLOR);
		r.bottom = 10;
		FillRect(r);
	}
	if (fEnabled) {
		SetHighColor(0, 0, 0);

		font = *be_plain_font;
		font.SetSize(9.0);
		SetFont(&font);
		SetHighColor(0, 0, 0);
		MovePenTo(r.left + fWidth + 4, r.top + 10);
		if (fMag == 1024)
			mag = 'K';
		else if (fMag == 1024 * 1024)
			mag = 'M';
		else if (fMag == 1024 * 1024 * 1024)
			mag = 'G';
		sprintf(str, "%Ld %cB", fMin / fMag, mag);
		DrawString(str);

		sprintf(str, "%Ld %cB", fMax / fMag, mag);
		MovePenTo(r.right - fOffset - font.StringWidth(str) - 4, r.top + 10);
		DrawString(str);
	}
}

//--------------------------------------------------------------------

void TSliderView::DrawSlider(void)
{
	uint64	size;
	float	width;
	char	mag;
	char	str[256];
	BFont	font;
	BRect	r;

	font = *be_plain_font;
	font.SetSize(10);
	SetFont(&font);
	SetHighColor(VIEW_COLOR, VIEW_COLOR, VIEW_COLOR);
	r = Bounds();
	r.left = r.right - fOffset;
	r.top += 10;
	r.bottom = r.top + 12;
	FillRect(r);
	if (fEnabled) {
		SetHighColor(0, 0, 0);
		if (fMag == 1024)
			mag = 'K';
		else if (fMag == 1024 * 1024)
			mag = 'M';
		else if (fMag == 1024 * 1024 * 1024)
			mag = 'G';
		sprintf(str, "%Ld %cB", fNew / fMag, mag);
	}
	else {
		SetHighColor(128, 128, 128);
		sprintf(str, "");
	}
	MovePenTo(r.right - StringWidth(str), r.top + 10);
	DrawString(str);

	fSlider->Lock();
	r = fOffView->Bounds();
	fOffView->SetHighColor(VIEW_COLOR, VIEW_COLOR, VIEW_COLOR);
	fOffView->FillRect(r);
	r.bottom = r.top + 9;
	r.left += 4;
	r.right -= 4;
	fOffView->SetHighColor(184, 184, 184);
	fOffView->FillRect(r);
	fOffView->SetHighColor(255, 255, 255);
	fOffView->StrokeLine(BPoint(r.left + 1, r.bottom),
						 BPoint(r.right, r.bottom));
	fOffView->StrokeLine(BPoint(r.right, r.bottom),
						 BPoint(r.right, r.top));
	fOffView->SetHighColor(128, 128, 128);
	fOffView->StrokeLine(BPoint(r.left, r.top),
						 BPoint(r.right - 1, r.top));
	fOffView->StrokeLine(BPoint(r.left, r.top),
						 BPoint(r.left, r.bottom));

	if (fEnabled) {
		r.InsetBy(1, 1);
		width = r.Width();

		(fNew < fMin) ? size = fMin : size = fNew;
		(fNew > fMax) ? size = fMax : size = fNew;
		r.right = r.left + (((float)((size - fMin) / (float)(fMax - fMin))) *
							width);
		fOffView->SetHighColor(102, 152, 203);
		fOffView->FillRect(r);

		fOffView->SetHighColor(80, 80, 80);
		fOffView->StrokeLine(BPoint(r.right, r.top + 6),
							 BPoint(r.right - 5, r.top + 11));
		fOffView->StrokeLine(BPoint(r.right - 5, r.top + 11),
							 BPoint(r.right + 5, r.top + 11));
		fOffView->StrokeLine(BPoint(r.right + 5, r.top + 11),
							 BPoint(r.right, r.top + 6));

		fOffView->SetHighColor(0, 255, 0);
		fOffView->StrokeLine(BPoint(r.right, r.top + 7),
							 BPoint(r.right, r.top + 7));
		fOffView->StrokeLine(BPoint(r.right - 1, r.top + 8),
							 BPoint(r.right + 1, r.top + 8));
		fOffView->StrokeLine(BPoint(r.right - 2, r.top + 9),
							 BPoint(r.right + 2, r.top + 9));
		fOffView->StrokeLine(BPoint(r.right - 3, r.top + 10),
							 BPoint(r.right + 3, r.top + 10));

		if (fFocus) {
			fOffView->SetHighColor(0, 0, 229);
			fOffView->StrokeLine(BPoint(r.right - 5, r.top + 14),
								 BPoint(r.right + 5, r.top + 14));
			fOffView->SetHighColor(255, 255, 255);
			fOffView->StrokeLine(BPoint(r.right - 5, r.top + 15),
								 BPoint(r.right + 5, r.top + 15));
		}
		else {
			fOffView->SetHighColor(VIEW_COLOR, VIEW_COLOR, VIEW_COLOR);
			fOffView->StrokeLine(BPoint(r.right - 5, r.top + 14),
								 BPoint(r.right + 5, r.top + 14));
			fOffView->StrokeLine(BPoint(r.right - 5, r.top + 15),
								 BPoint(r.right + 5, r.top + 15));
		}
	}
	fOffView->Sync();
	fSlider->Unlock();

	r = Bounds();
	DrawBitmap(fSlider, BPoint(r.left + fWidth, r.top + 13));
}

//--------------------------------------------------------------------

void TSliderView::SetEnabled(bool enable)
{
	if (enable != fEnabled) {
		fEnabled = enable;
		if (fSlider)
			Draw(Bounds());
		if (!enable)
			SetFlags(Flags() & ~B_NAVIGABLE);
		else
			SetFlags(Flags() | B_NAVIGABLE);
	}
}

//--------------------------------------------------------------------

void TSliderView::SetRange(uint64 min, uint64 max)
{
	if ((min != fMin) || (max != fMax)) {
		fMin = min;
		fMax = max;

		DrawRange(TRUE);
		DrawSlider();
	}
}

//--------------------------------------------------------------------

void TSliderView::SetValue(uint64 value)
{
	if (fNew != value) {
		fNew = value;
		if (value < 1024 * 1024) {
			fMag = 1024;
			DrawRange(TRUE);
		}
		else {
			fMag = 1024 * 1024;
			DrawRange(TRUE);
		}
		if (fSlider)
			DrawSlider();
	}
}

//--------------------------------------------------------------------

uint64 TSliderView::Value(void)
{
	return fNew;
}


//====================================================================

void fit_string(BView *view, char *str, float width)
{
	char	*string;
	const char *src[1];
	char	*result[1];
	BFont	font;

	view->GetFont(&font);
	src[0] = str;
	string = (char*)malloc(strlen(str) + 16);
	result[0] = string;
	font.GetTruncatedStrings(src, 1, B_TRUNCATE_END, width, result);

	if (strlen(string))
		view->DrawString(string);
	free(string);
}
