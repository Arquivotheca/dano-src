#ifndef _BOOTMAN_WINDOW_H_
#define _BOOTMAN_WINDOW_H_

#include <Window.h>
#include <private/storage/DeviceMap.h>
#include <FilePanel.h>

#define BROWSE 'brow'
#define UPDATE_FILENAME 'upfn'
#define NAVIGATE 'navi'
#define SET_DEFAULT 'setd'

#define NO_ACTION '0x90'

class TWindow : public BWindow {
	int32 path, depth;	/* specifies current state */
	int32 default_path;

	char SavedMBR[B_FILE_NAME_LENGTH];
	int32 timeout, default_partition;
	uchar MBR[4 * 0x200];

	BList Partitions;
	DeviceList Devices;
	Device *BootDevice;

	status_t ReadMBR(void);
	bool ExecuteInstallBootMenu(int32 depth);
	void DisplayInstallBootMenu(void);
	bool ExecuteUninstallBootMenu(int32 depth);
	void DisplayUninstallBootMenu(void);
	void UpdateState(int32 path, int32 depth);
	void AddRadio(float top, float height, uint32 what, const char *text, bool set = false);
	void AddFileChooser(float top, char *filename,
			file_panel_mode type);
	void AddButton(float left, const char *label, uint32 what, 
			int32 cookie = 0, bool enabled = true);
	int32 WhichRadio(void);
	void RecursiveRemove(BView *parent, BView *child);

	BView *root, *box;
	BFilePanel *panel;
public:
	TWindow(BRect, const char *, window_type, uint32, char *, char *);

	virtual bool QuitRequested(void);
	virtual void MessageReceived(BMessage *);
};

status_t GetDriveID(const char *device, uchar *id);

#endif
