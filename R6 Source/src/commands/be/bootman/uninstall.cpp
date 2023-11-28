#include <Alert.h>

#include "widget.h"
#include "window.h"

#include <string.h>

bool TWindow::ExecuteUninstallBootMenu(int32 depth)
{
	char ebuf[512], buffer[4 * 0x200 + 1];

	if (depth != 1) return true;

	// read SavedMBR
	int fd;
	status_t error;
	struct stat st;

	if (stat(SavedMBR, &st) < 0) {
		sprintf(ebuf, "%s doesn't exist! Please choose another file.", SavedMBR);
		(new BAlert("", ebuf, "Oops"))->Go();
		return false;
	}
	
	fd = open(SavedMBR, O_RDONLY);
	if (fd < 0) {
		sprintf(ebuf, "Unable to open the Master Boot Record save file %s (%s). "
"MBR restoration cannot continue.", SavedMBR, strerror(fd));
		(new BAlert("", ebuf, "Sorry"))->Go();
		return false;
	}
	
	error = read(fd, buffer, 4 * 0x200 + 1);
	close(fd);
	
	if (error < 0) {
		sprintf(ebuf, "Error reading Master Boot Record save file %s (%s). "
"MBR restoration cannot continue.", SavedMBR, strerror(error));
		(new BAlert("", ebuf, "Sorry"))->Go();
		return false;
	}

	if (error != 4*0x200) {
		sprintf(ebuf,
"Master Boot Record save file %s is corrupt. MBR restoration cannot continue.", SavedMBR);
		(new BAlert("", ebuf, "Sorry"))->Go();
		return false;
	}
	
	if (memcmp(buffer + 0x1be, MBR + 0x1be, 0x40)) {
		if ((new BAlert("",
"The partition table on the boot device %s doesn't match the partition table found in "
"the saved Master Boot Record. Do you still wish to proceed with the MBR restore process?",
		"Yes", "No"))->Go() == 1)
			goto err;
	} else {
		sprintf(ebuf,
"About to restore the Master Boot Record of %s from %s. Do you wish to continue?",
			BootDevice->Name(), SavedMBR);
		if ((new BAlert("", ebuf, "Yes", "No"))->Go() == 1) {
			(new BAlert("", "Master Boot Record *not* restored.", "Phew"))->Go();
			goto err;
		}
	}
	
	fd = open(BootDevice->Name(), O_WRONLY);
	if (fd < 0) {
		sprintf(ebuf, "Unable to open boot device %s (%s). "
"Master Boot Record restoration cannot continue.", BootDevice->Name(), strerror(fd));
		(new BAlert("", ebuf, "Sorry"))->Go();
		return false;
	}
	
	memcpy(buffer + 0x1be, MBR + 0x1be, 0x40);
	error = write_pos(fd, 0, buffer, 4 * 0x200);
	close(fd);

	if (error < 4 * 0x200) {
		sprintf(ebuf, "Error writing Master Boot Record to %s (%s).",
			BootDevice->Name(), strerror(error));
		(new BAlert("", ebuf, "Sorry"))->Go();
		return false;
	}

	/* Update cached MBR */
	memcpy(MBR, buffer, 4 * 0x200);

	return true;

err:
	close(fd);
	return false;
}

void TWindow::DisplayUninstallBootMenu(void)
{
	BRect r, f;
	
	f = box->Bounds();
	
	switch (depth) {
		case 0 :
			r = BRect(0.05, 0.05, 0.95, 0.6);
			box->AddChild(new TTextView(TRect(r, f),
					"Please locate the Master Boot Record (MBR) save file "
					"to restore from. This is the file that was created "
					"when the boot manager was first installed."));
			AddFileChooser(0.6, SavedMBR, B_OPEN_PANEL);

			break;
		case 1 :
		{
			char ebuf[1024];
sprintf(ebuf,
"The Master Boot Record of the boot device (%s) has been successfully "
"restored from %s.", BootDevice->Name(), SavedMBR);
			r = BRect(0.05, 0.05, 0.95, 0.95);
			box->AddChild(new TTextView(TRect(r,f), ebuf));
			break;
		}

	}

	if (depth < 1)
		AddButton(0.75, "Next", NAVIGATE, 1);
	else
		AddButton(0.75, "Done", B_QUIT_REQUESTED);
}
