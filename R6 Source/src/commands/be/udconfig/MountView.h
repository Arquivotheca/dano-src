//
// Copyright 2000 Be, Incorporated.  All Rights Reserved.
//

#ifndef _MOUNT_VIEW_H_
#define _MOUNT_VIEW_H_

#include <View.h>
#include <List.h>

class BInvoker;
class BListView;
class BButton;

class MountView : public BView {
public:
	MountView(BRect frame, const char *name, uint32 resizingMode, uint32 flags, const char *fname, BInvoker *invoker);
	~MountView();

	status_t InitCheck();

	void AttachedToWindow();
	void AllAttached();
	void FrameResized(float width, float height);

	void MessageReceived(BMessage *msg);

private:
	bool EvaluateFile();
	void FreeDeviceList();
	void BuildDeviceList();
	void AddDevice(const char*);
	void RecurseDir(const char*);
	void UpdateVolumeList();
	void AddFileName(BRect &r);
	void AddVolumeList(BRect &r);
	void AddButtons(BRect &r);
	void HandleSelection();

	status_t ScanVolumes();
	static int32 build_device_list(void *mount_view);

	void MountVolumes();
	void MountThread();
	static int32 mount_selected_volumes(void *mount_view);

private:
	float fEm;
	status_t fErr;
	char *fFileName;
	char *fVolumeName;
	BInvoker *fDoneInvoker;

	thread_id fScanThread;
	thread_id fMountThread;

	BList fDeviceList;
	BList fMountList;
	BListView *fVolumeListView;
	BButton *fMountButton;
	BButton *fRescanButton;
};

#endif //_MOUNT_VIEW_H_
