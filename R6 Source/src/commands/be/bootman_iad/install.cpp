#include "widget.h"
#include "window.h"

#include <Alert.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <ScrollView.h>

#include <string.h>

#include "boot.h"
#include "rescue.h"

int32 timeouts[] = { 0, (int32)(5 * 18.2), (int32)(10 * 18.2),
					(int32)(15 * 18.2) };

bool TWindow::ExecuteInstallBootMenu(int32 depth)
{
	int fd;
	status_t error;
	char ebuf[1024];

	if ((depth == 0) || (depth == 3)) {
		for (int32 i=0;i<Partitions.CountItems();i++) {
			PListItem *item = (PListItem *)(Partitions.ItemAt(i));
			const Partition *partition = item->partition;
			uchar id;
			
			GetDriveID(partition->GetDevice()->Name(), &id);

			if ((id == 0x80) && (partition->Offset() < 5) && partition->Blocks()) {
				sprintf(ebuf, "Partition #%ld on the boot device (%s) lies too close "
								"to the beginning of the disk and will interfere with "
								"the boot manager.",
								partition->Index(), partition->GetDevice()->Name());
				(new BAlert("", ebuf, "Sorry"))->Go();
				return false;
			}
		}
	}

	if (depth == 1) {
		/* save MBR to a file */
		char *p;

		p = strrchr(SavedMBR, '/');
		if (p) {
			*p = 0;
			create_directory(SavedMBR, 0777);
			*p = '/';
		}

		fd = creat(SavedMBR, 0644);
		if (fd < 0) {
			sprintf(ebuf,	"Error opening %s (%s).", SavedMBR, strerror(fd));
			(new BAlert("", ebuf, "Ok"))->Go();
			return false;
		}
		error = write(fd, MBR, 4*0x200);
		close(fd);

		if (error < 4*0x200) {
			sprintf(ebuf,	"Error saving old MBR to %s (%s).", SavedMBR, strerror(error));
			(new BAlert("", ebuf, "Ok"))->Go();
			return false;
		}
	}
	
	if (this->depth == 2) {
		/* make rescue disk */
		fd = open("/dev/disk/floppy/raw", O_WRONLY);
		if (fd < 0) {
			sprintf(ebuf,	"Error opening floppy drive (%s).",
							strerror(fd));
			(new BAlert("", ebuf, "Ok"))->Go();
			return false;
		}
		error = write(fd, rescue, 0x200);
		if (error == 0x200)
			error = write(fd, MBR, 4*0x200);
		close(fd);

		if (error < 4*0x200) {
			/* vyt: output better error message */
			sprintf(ebuf, 	"Error writing rescue disk (%s).",
							strerror(error));
			(new BAlert("", ebuf, "Ok"))->Go();
			return false;
		}

		(new BAlert("", "Rescue disk successfully made. Please remove the "
						"floppy from the drive.", "Ok"))->Go();
	}
	
	if (depth == 4) {
		int32 i;
		for (i=0;i<Partitions.CountItems();i++)
			if (((PListItem *)(Partitions.ItemAt(i)))->enabled)
				break;
		if (i == Partitions.CountItems()) {
			(new BAlert("", "At least one partition must be selected!", "Ok"))->Go();
			return false;
		}
		return true;
	}
	
	if (depth == 5) {
		/* set timeout */
		timeout = timeouts[WhichRadio()];
		return true;
	}

	if (depth == 6) {
		if ((new BAlert("",
"About to write the boot menu to disk. Are you sure you want to continue?",
"Yes", "No", NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go() == 1)
			return false;
	
		uchar scratch[4 * 0x200], *p, *q;

		/* build the stuff to write */
		memset(scratch, 0, sizeof(scratch));
		memcpy(scratch, boot, sizeof(boot) - 1);
		memcpy(scratch + 0x1be, MBR + 0x1be, 0x40);
		p = scratch + sizeof(boot) - 1;
		*(uint16 *)p = timeout; p += sizeof(uint16); /* timeout */
		int32 i,j,numitems;
		for (i=0,numitems=0;i<Partitions.CountItems();i++) {
			PListItem *item = (PListItem *)(Partitions.ItemAt(i));
			if (item->enabled) numitems++;
		}
		*(uint16 *)p = numitems; p += sizeof(uint16);
		for (i=j=0;i<Partitions.CountItems();i++) {
			PListItem *item = (PListItem *)(Partitions.ItemAt(i));
			if (item->enabled) {
				if (i == default_partition)
					break;
				j++;
			}
		}
		if (i == Partitions.CountItems()) j = 0;
		*(uint16 *)p = j; p += sizeof(uint16); /* default item */
		q = p + numitems * sizeof(uint16);
		const uchar colors[] = { 1, 4, 2, 3 };
		for (i=j=0;i<Partitions.CountItems();i++) {
			PListItem *pi = (PListItem *)(Partitions.ItemAt(i));
			if (pi->enabled == false) continue;
			*(uint16 *)p = (int)(q - scratch + 0x7c00); p += sizeof(uint16);
			*(q++) = strlen(pi->name) + 1;
			*(q++) = colors[j % sizeof(colors)];
			memcpy(q, pi->name, strlen(pi->name));
			q += strlen(pi->name);
			uchar bios_id;
			GetDriveID(pi->partition->GetDevice()->Name(), &bios_id);
			*(q++) = bios_id;
			*(uint32 *)q = (uint32)pi->partition->Offset(); q += sizeof(uint32);
			j++;
		}

		fd = open(BootDevice->Name(), O_WRONLY);
		if (fd < 0) {
			sprintf(ebuf, "Error opening boot device %s (%s). "
"Boot menu installation failed.", BootDevice->Name(), strerror(fd));
			(new BAlert("", ebuf, "Sorry"))->Go();
			return false;
		}
		
		error = write(fd, scratch, 4 * 0x200);
		close(fd);
		
		if (error < 4 * 0x200) {
			sprintf(ebuf, "Error writing boot menu to boot device %s (%s). "
"Boot menu installation failed.", BootDevice->Name(), strerror(error));
			(new BAlert("", ebuf, "Sorry"))->Go();
			return false;
		}

		/* update cached copy of MBR */		
		memcpy(MBR, scratch, 4 * 0x200);

		return true;
	}

	return true;
}

void TWindow::DisplayInstallBootMenu(void)
{
	BRect f, r;
	char ebuff[512];
	const char *NextLabel = "Next";
	
	f = box->Bounds();

	switch (depth) {
		case 0 :
			r = BRect(0.05, 0.05, 0.95, 0.74);
			sprintf(ebuff, "The Master Boot Record (MBR) of the boot device:\n"
					"\t%s\nwill now be "
					"saved to disk. Please select a file to save the MBR "
					"into.\n\n"
					"If something goes wrong with the installation or if you "
					"later wish to remove the boot menu, simply run the "
					"bootman program and choose the 'Uninstall' option.",
					BootDevice->Name());
			box->AddChild(new TTextView(TRect(r, f), ebuff));

			AddFileChooser(0.75, SavedMBR, B_SAVE_PANEL);

			break;
		case 1 :
			sprintf(ebuff,
			"The old Master Boot Record was successfully saved to %s.\n\n"
			"Would you like to create a rescue disk?  This is highly recommended. "
			"If something goes wrong with the installation, you can boot from the "
			"rescue disk and it will automatically restore the MBR", SavedMBR);
			r = BRect(0.05, 0.05, 0.95, 0.95);
			box->AddChild(new TTextView(TRect(r, f), ebuff));

			AddButton(0.5, "No", NAVIGATE, 2);
			NextLabel = "Yes";

			break;
		case 2 :
			r = BRect(0.05, 0.05, 0.95, 0.95);
			box->AddChild(new TTextView(TRect(r, f), 
"Please place a floppy disk in the drive.  It will be made "
"into a rescue disk, overwriting the original contents."));
			break;
		case 3 :
		{
			r = BRect(0.05, 0.05, 0.95, 0.45);
			box->AddChild(new TTextView(TRect(r, f),
"The following partitions were detected. "
"Please check the box next to the partitions to be included in the boot menu. "
"You can also set the names of the partitions as you would like them to appear "
"in the boot menu."));
			r = TRect(BRect(0.05, 0.45, 0.95, 0.95), f);
			r.right -= B_V_SCROLL_BAR_WIDTH;
			r.bottom -= B_H_SCROLL_BAR_HEIGHT;
			PView *p = new PView(r, "PListView", &Partitions);
			box->AddChild(new BScrollView(NULL, p, B_FOLLOW_LEFT | B_FOLLOW_TOP, 0, true, true));
			break;
		}
		case 4 :
		{
			r = BRect(0.05, 0.05, 0.95, 0.4);
			box->AddChild(new TTextView(TRect(r, f), 
"Please specify a default partition and a timeout. The boot menu will load the default "
"partition after the timeout unless you select another partition. You can also "
"have the boot menu wait indefinitely for you to select a partition."));

			BMenu *menu = new BMenu("");
			menu->SetLabelFromMarked(true);
			PListItem *item;
			item = (PListItem *)(Partitions.ItemAt(default_partition));
			if (!item || !(item->enabled))
				default_partition = -1;
			for (int32 i=0;i<Partitions.CountItems();i++) {
				PListItem *item = (PListItem *)(Partitions.ItemAt(i));
				if (item->enabled) {
					if (default_partition == -1)
						default_partition = i;
					BMessage *message;
					message = new BMessage(SET_DEFAULT);
					message->AddInt32("cookie", i);
					BMenuItem *mitem = new BMenuItem(item->name, message);
					if (i == default_partition)
						mitem->SetMarked(true);
					menu->AddItem(mitem);
				}
			}
			menu->SetTargetForItems(this);

			r = BRect(0.05, 0.425, 0.95, 0.525);
			BMenuField *field = new BMenuField(TRect(r,f), NULL,
					"Default Partition:", menu);
			field->SetDivider(field->StringWidth(field->Label()) + 4);
			box->AddChild(field);

			uint32 i;
			for (i=0;i<sizeof(timeouts)/sizeof(timeouts[0]);i++)
				if (timeouts[i] == timeout)
					break;
			AddRadio(0.58, 0.1, NO_ACTION, "Wait Indefinitely", (i == 0));
			AddRadio(0.68, 0.1, NO_ACTION, "Wait 5 Seconds", (i == 1));
			AddRadio(0.78, 0.1, NO_ACTION, "Wait 10 Seconds", (i == 2));
			AddRadio(0.88, 0.1, NO_ACTION, "Wait 15 Seconds", (i == 3));

			break;
		}
		case 5 :
		{
			char ebuf[1024], temp1[128], temp2[128];
			sprintf(ebuf, "About to write the following boot menu to the boot "
"disk (%s). Please verify the information below before continuing.\n\n", BootDevice->Name());

			for (int32 i=0;i<Partitions.CountItems();i++) {
				PListItem *item = (PListItem *)(Partitions.ItemAt(i));
				if (item->enabled == false) continue;
				sprintf(temp1, item->partition->GetDevice()->Name());
				*(strrchr(temp1, '/')+1) = 0;
				sprintf(temp2, "%s\t(%s%ld_%ld)\n", item->name,
						temp1, item->partition->GetSession()->Index(),
						item->partition->Index());
				strcat(ebuf, temp2);
			}

			r = BRect(0.05, 0.05, 0.95, 0.95);
			TTextView *view = new TTextView(TRect(r,f), ebuf);
			view->SetTabWidth(f.Width() * 0.2);
			box->AddChild(view);
			break;
		}

		case 6 :
		{
			r = BRect(0.05, 0.05, 0.95, 0.95);
			box->AddChild(new TTextView(TRect(r,f),
"The boot manager has been successfully installed on your system."));
			break;
		}
	}

	if (depth < 6)
		AddButton(0.75, NextLabel, NAVIGATE, 1);
	else
		AddButton(0.75, "Done", B_QUIT_REQUESTED);
}
