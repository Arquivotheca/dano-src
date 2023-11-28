#include <Alert.h>
#include <Application.h>
#include <Box.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Path.h>
#include <RadioButton.h>
#include <Screen.h>
#include <ScrollView.h>
#include <TextControl.h>
#include <TextView.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <fs_info.h>

#include "widget.h"
#include "window.h"

extern unsigned char boot[];

status_t GetDriveID(const char *device, uchar *id)
{
	status_t err;
	int fd;
	assert(id);
	*id = 0xff;
	fd = open(device, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "error opening %s\n", device);
		return fd;
	}
	err = ioctl(fd, B_GET_BIOS_DRIVE_ID, id, sizeof(uchar));
	close(fd);
	if (err)
		fprintf(stderr, "error getting BIOS drive id for %s (%s)\n",
				device, strerror(err));
	return err;
}

Device *FindAnyBIOSID(Device *device, void *cookie)
{
	uchar id;
	GetDriveID(device->Name(), &id);
	if ((id >= 0x80) && (id != 0xff))
		return device;
	return NULL;
}

Device *FindBootDevice(Device *device, void *cookie)
{
	uchar id;
	GetDriveID(device->Name(), &id);
	if (id == 0x80) {
		*(Device **)cookie = device;
		printf("Found boot device: %s\n", device->Name());
		return device;
	}
	return NULL;
}

Partition *AddPartition(Partition *partition, void *cookie)
{
	uchar id;
	BList *list = (BList *)cookie;

	// skip empty partitions
	if (partition->Data()->hidden)
		return NULL;

	if (partition->GetDevice()->Removable()) {
		printf("Skipping removable media (%s).\n", partition->GetDevice()->Name());
		return NULL;
	}

	GetDriveID(partition->GetDevice()->Name(), &id);

	if (id == 0xff) {
		partition->Dump("Skipping partition on drive with invalid BIOS id\n");
		return NULL;
	}

	list->AddItem(new PListItem(partition));
	
	return NULL;
}

class Window : public BWindow
{
	bool quit_app;
	DeviceList *Devices;
	Device **BootDevice;
	sem_id sem;
public:
	Window(BRect f, DeviceList *devices, Device **bootdevice, sem_id sema);
	void MessageReceived(BMessage *);
	bool QuitRequested(void);
};

#define SELECT_BOOT_DEVICE 'boot'
#define CANCEL_WINDOW 'canw'

Device *AddToMenu(Device *device, void *cookie)
{
	device_geometry geometry;
	BMenu *menu = (BMenu *)cookie;
	BMessage *message;
	char text[128], str[16];
	int fd;

	if (strstr(device->Name(), "floppy")) return NULL;

	str[0] = 0;
	if ((fd = open(device->Name(), O_RDONLY)) >= 0) {
		if (ioctl(fd, B_GET_GEOMETRY, &geometry, sizeof(geometry)) == B_OK) {
			if (geometry.device_type == B_CD) {
				close(fd);
				return NULL;
			}
			uint64 size = (uint64)geometry.bytes_per_sector *
							geometry.sectors_per_track *
							geometry.cylinder_count *
							geometry.head_count;
			if (size >= 1024LL * 1024 * 1024 * 1024)
				sprintf(str, " (%.1f TB)", size / (1024.*1024.*1024.*1024.));
			else if (size >= 1024 * 1024 * 1024)
				sprintf(str, " (%.1f GB)", size / (1024.*1024.*1024.));
			else if (size >= 1024 * 1024)
				sprintf(str, " (%.1f MB)", size / (1024.*1024.));
			else
				sprintf(str, " (%.1f KB)", size / 1024.);
		}
		close(fd);
	}

	message = new BMessage(SELECT_BOOT_DEVICE);
	message->AddPointer("device", device);
	sprintf(text, "%s%s", device->Name(), str);
	menu->AddItem(new BMenuItem(text, message));

	return NULL;
}

void Window::MessageReceived(BMessage *message)
{
	if (message->what == SELECT_BOOT_DEVICE) {
		void *p;
		if (message->FindPointer("device", 0, &p) == B_OK)
			*BootDevice = (Device *)p;
	} else if (message->what == CANCEL_WINDOW) {
		*BootDevice = NULL;
		PostMessage(B_QUIT_REQUESTED);
	} else
		BWindow::MessageReceived(message);
}

Window::Window(BRect f, DeviceList *devices, Device **bootdevice, sem_id sema) :
		BWindow(f, "Be Boot Manager", B_TITLED_WINDOW, B_NOT_RESIZABLE),
		Devices(devices), BootDevice(bootdevice), sem(sema)
{
	*BootDevice = NULL;
	
	BView *root = new BView(Bounds(), NULL, B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	root->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(root);

	BRect r = Bounds(), s;
	r.left += 25; r.right -= 25;
	r.top += 10; r.bottom = r.top + 50;
	s = r; s.OffsetTo(0,0);
	BTextView *tview = new BTextView(r, NULL, s, 0, B_WILL_DRAW);
	tview->SetText(
"The program was unable to identify the hard drive the system boots from. "
"Please select it from the list below and click 'Okay'.");
	tview->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	tview->MakeEditable(false);
	root->AddChild(tview);

	BMenu *menu = new BMenu("<choose a boot device>");
	menu->SetLabelFromMarked(true);

	Devices->EachDevice(AddToMenu, menu);

	menu->SetTargetForItems(this);

	r.right = f.Width() - r.left;
	r.bottom = r.top + 20;
	r.OffsetBy(0, 60);
	BMenuField *field = new BMenuField(r, NULL, "", menu);
	field->SetDivider(0);
	root->AddChild(field);

	r.OffsetBy(0, 30);
	r.bottom = r.top + 20;
	r.right = r.left + 60;
	root->AddChild(new BButton(r, NULL, "Cancel", new BMessage(CANCEL_WINDOW)));

	r.OffsetBy(f.Width() - r.right - r.left, 0);
	root->AddChild(new BButton(r, NULL, "Ok", new BMessage(B_QUIT_REQUESTED)));

	Show();
}

bool Window::QuitRequested(void)
{
	release_sem(sem);
	return true;
}

void LocateBootDevice(DeviceList *Devices, Device **BootDevice)
{
	float w, h, sw, sh;
	w = 290; h = 135;
	sem_id sem = create_sem(0, "block");

	BRect f = BScreen().Frame();
	sw = f.Width(); sh = f.Height();
	f.left = (sw - w) / 2; f.right = (sw + w) / 2;
	f.top = (sh - h) / 2; f.bottom = (sh + h) / 2;

	new Window(f, Devices, BootDevice, sem);

	acquire_sem(sem);
}

void TWindow::AddRadio(float top, float height, uint32 what, 
		const char *text, bool set)
{
	BRect f = box->Bounds(), r;
	BRadioButton *v;
	
	r = BRect(0.05, top, 0.2, top + height);
	box->AddChild(v = new BRadioButton(TRect(r,f), NULL, "",
			new BMessage(what)));
	if (set) v->SetValue(1);

	r = BRect(0.2, top, 0.95, top + height);
	box->AddChild(new TTextView(TRect(r,f), text));
}


void TWindow::AddFileChooser(float top, char *default_name, file_panel_mode type)
{
	BRect f = box->Bounds(), r;
	BTextControl *v;
	BMessage *message;

	r = BRect(0.05, top, 0.65, top + 0.1);
	v = new BTextControl(TRect(r, f), "filename", NULL,
			default_name, NULL);
	v->SetDivider(0);

	message = new BMessage(UPDATE_FILENAME);
	message->AddPointer("ptr", default_name);
	v->SetMessage(message);

	message = new BMessage(UPDATE_FILENAME);
	message->AddPointer("ptr", default_name);
	v->SetModificationMessage(message);

	box->AddChild(v);

	r = BRect(0.7, top, 0.95, top + 0.1);
	box->AddChild(new TButton(TRect(r,f), "Select", BROWSE, (int32)type));
}

void TWindow::AddButton(float left, const char *label, uint32 what,
		int32 cookie, bool enabled)
{
	BRect r, f;
	f = Bounds();
	r = BRect(left, 0.85, left + 0.2, 0.95);
	root->AddChild(new TButton(TRect(r,f), label, what, cookie, enabled));
}

void TWindow::RecursiveRemove(BView *parent, BView *child)
{
	for (int32 i=child->CountChildren()-1;i>=0;i--) {
		BView *grandchild = child->ChildAt(i);
		RecursiveRemove(child, grandchild);
		child->RemoveChild(grandchild);
		delete grandchild;
	}
}

void TWindow::UpdateState(int32 dpath, int32 ddepth)
{
	static int32 pathStack[10], depthStack[10], topStack = -1;
	Lock();

	BRect f = Bounds(), r;

	if (dpath < 0) {
		path = pathStack[topStack];
		depth = depthStack[topStack];
		topStack--;
	} else if (dpath != 'mr.t') {
		topStack++;
		pathStack[topStack] = path;
		depthStack[topStack] = depth;

		if (!path && !dpath) {
			if (WhichRadio() == 0) {
				dpath = 1;
				ddepth = memcmp(MBR, boot, 0x80) ? 0 : 3;
			} else {
				dpath = 2;
				ddepth = 0;
			}
		}

		if (((path + dpath == 1) && !ExecuteInstallBootMenu(depth + ddepth)) ||
			((path + dpath == 2) && !ExecuteUninstallBootMenu(depth + ddepth))) {
			topStack--;
			Unlock();
			return;
		}

		path += dpath;
		depth += ddepth;
	}

	if (path) default_path = path;

	/* first remove everything */
	for (int32 i=root->CountChildren()-1;i>=0;i--) {
		BView *child = root->ChildAt(i);
		RecursiveRemove(root, child);
		root->RemoveChild(child);
		delete child;
	}
	
	assert(root->CountChildren() == 0);

	r = BRect(0.05, 0.05, 0.95, 0.8);
	root->AddChild(box = new BBox(TRect(r,f)));

	if (path)
		AddButton(0.05, "Previous", NAVIGATE, -1);

	if (!path) {
		AddRadio(0.15, 0.4, NO_ACTION, "Install Boot Menu\n"
			"Choose this option to install a boot menu, "
			"allowing you to select which operating system to boot when "
			"you turn on your computer.", (default_path == 1)
		);
		AddRadio(0.55, 0.4, NO_ACTION, "Uninstall Boot Menu\n"
			"Choose this option to remove the boot menu "
			"previously installed by this program.",
			(default_path != 1)
		);

		AddButton(0.75, "Next", NAVIGATE, 1, true);
	} else if (path == 1) {
		DisplayInstallBootMenu();
	} else if (path == 2) {
		DisplayUninstallBootMenu();
	}

	Unlock();
}

int32 TWindow::WhichRadio(void)
{
	int32 children = box->CountChildren(), which = 0;
	for (int32 i=0;i<children;i++) {
		BRadioButton *button;
		button = dynamic_cast<BRadioButton *>(box->ChildAt(i));
		if (button && ++which && button->Value())
			break;
	}
	return which - 1;
}

status_t TWindow::ReadMBR(void)
{
	int fd;
	status_t error;
	
	fd = open(BootDevice->Name(), O_RDONLY);
	if (fd < 0) return fd;
	error = read(fd, MBR, 4 * 0x200);
	close(fd);

	if (error < 0) return error;
	return (error < 4 * 0x200) ? B_ERROR : B_OK;
}

TWindow::TWindow(BRect frame, const char *title, window_type type, uint32 flags,
		char *default_file, char *beos_boot_path) :
		BWindow(frame, title, type, flags)
{
	Devices.RescanDevices(true);
	Devices.UpdateMountingInfo();

	/* make sure there is at least one valid drive id */
	if (Devices.EachDevice(FindAnyBIOSID, NULL) == false) {
		(new BAlert("",
			"No drives were assigned BIOS IDs. This commonly occurs "
			"in SCSI systems. Please check your SCSI BIOS settings and make"
			"sure the drives are mapped. You can install the BeOS boot "
			"manager at a later time by opening a Terminal window and "
			"typing \"bootman\". "
			"Boot manager application cannot continue.",
			"Sorry"))->Go();
		PostMessage(B_QUIT_REQUESTED);
		return;
	}

	/* make sure the beos boot path is there */
	if (beos_boot_path) {
		fs_info info;

		if (fs_stat_dev(dev_for_path(beos_boot_path), &info) != B_OK) {
			if ((new BAlert("",
"Unable to get information about the BeOS boot volume. "
"Would you like to continue installing the boot manager anyway? (You can "
"install the BeOS boot manager at a later time by opening a Terminal "
"window and typing \"bootman\".",
				"Yes", "No"))->Go() == 1) {
				PostMessage(B_QUIT_REQUESTED);
				return;
			}
		} else {
			uchar id = 0xff;

			GetDriveID(info.device_name, &id);

			if (id == 0xff) {
				char message[1024];

				sprintf(message,
"Unable to get the BIOS drive number for the BeOS boot volume (%s). %s "
"occur because BeOS is unable to distinguish between two disks with "
"identical partition maps. You will be unable to add this volume to "
"the boot manager until this problem has been resolved. Would you like to "
"continue installing the boot manager anyway? (You can install the BeOS boot "
"manager at a later time by opening a Terminal window and typing \"bootman\".",
beos_boot_path,
(!strncmp("/dev/disk/scsi/", info.device_name, 15)) ?
"Please check your SCSI BIOS settings and make sure the drive is mapped. "
"into the BIOS. This may also" :
"This may");

				if ((new BAlert("", message, "Yes", "No"))->Go() == 1) {
					PostMessage(B_QUIT_REQUESTED);
					return;
				}
			}
		}
	}

	/* find the boot device */
	BootDevice = NULL;
	Devices.EachDevice(FindBootDevice, &BootDevice);

	if (BootDevice == NULL) {
		LocateBootDevice(&Devices, &BootDevice);
		if (BootDevice == NULL) {
			PostMessage(B_QUIT_REQUESTED);
			return;
		}
	}
	
	if (ReadMBR() < 0) {
		(new BAlert("",
			"Unable to read from the boot device. "
			"Boot manager application cannot continue.",
			"Sorry"))->Go();
		PostMessage(B_QUIT_REQUESTED);
		return;
	}
	
	if (Devices.EachPartition(AddPartition, &Partitions) != NULL) {
		PostMessage(B_QUIT_REQUESTED);
		return;
	}
	
	root = new BView(Bounds(), NULL, B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	root->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(root);

	path = 0; depth = 0;
	strcpy(SavedMBR, default_file);
	timeout = 0;
	default_partition = 0;
	default_path = 1;
	
	UpdateState('mr.t', 0);
}

bool TWindow::QuitRequested(void)
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

void TWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case UPDATE_FILENAME :
			{
				char *ptr;
				BTextControl *v;
				if ((message->FindPointer("ptr", 0, (void **)&ptr) == B_OK) &&
					(message->FindPointer("source", 0, (void **)&v) == B_OK)) {
					strcpy(ptr, v->Text());
				}
			}
			break;

		case BROWSE :
			{
				file_panel_mode type;
				if (message->FindInt32("cookie", 0, (int32 *)&type) == B_OK) {
					panel = new BFilePanel(type, NULL, (const entry_ref *)NULL,
							0, false, NULL, NULL, true, true);
					BTextControl *v = dynamic_cast<BTextControl *>(root->FindView("filename"));
					assert(v);
					if (v) {
						char *text = strrchr(v->Text(), '/'), c;
						if (text) {
							if (type == B_SAVE_PANEL)
								panel->SetSaveText(text + 1);
							c = *(text+1);
							*(text+1) = 0;
							BDirectory dir(v->Text());
							panel->SetPanelDirectory(&dir);
							*(text+1) = c;
						}
					}
					panel->SetTarget(BMessenger(this));
					panel->Show();
				} else {
					assert(0);
				}
			}
			break;
		case B_REFS_RECEIVED :
			{
				BTextControl *v = dynamic_cast<BTextControl *>(root->FindView("filename"));
				assert(v);
				if (v) {
					entry_ref ref;
					message->FindRef("refs", 0, &ref);
					BEntry entry(&ref);
					BPath path;
					entry.GetPath(&path);
					v->SetText(path.Path());
				}
			}
			break;
		case B_SAVE_REQUESTED :
			{
				BTextControl *v = dynamic_cast<BTextControl *>(root->FindView("filename"));
				assert(v);
				if (v) {
					char filename[128];
					entry_ref ref;
					message->FindRef("directory", 0, &ref);
					BEntry entry(&ref);
					BPath path;
					entry.GetPath(&path);
					const char *name;
					message->FindString("name", 0, &name);
					sprintf(filename, "%s/%s", path.Path(), name);
					v->SetText(filename);
				}
			}
			break;
		case B_CANCEL :
			// cleans up for both B_REFS_RECEIVED and B_SAVE_REQUESTED?
			delete panel;
			break;

		case NO_ACTION :
			break;
		case NAVIGATE :
			{
				int32 cookie;
				
				if (message->FindInt32("cookie", 0, &cookie) == B_OK) {
					if (cookie < 0)
						UpdateState(-1,-1);
					else
						UpdateState(0, cookie);
				} else {
					assert(0);
				}
				break;
			}
		case SET_DEFAULT :
			{
				message->FindInt32("cookie", 0, &default_partition);
				break;
			}
		default:
			BWindow::MessageReceived(message);
	}
}
