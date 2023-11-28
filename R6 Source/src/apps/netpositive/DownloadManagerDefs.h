// ===========================================================================
//	DownloadManagerDefs.h
//  Copyright 1999 by Be Incorporated.
// ===========================================================================

#ifndef DOWNLOADMANAGERDEFS
#define DOWNLOADMANAGERDEFS

#include <Window.h>
#include <Screen.h>
#include <stdio.h>
#include <ScrollView.h>
#include <TextControl.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <StringView.h>
#include <Bitmap.h>
#include <Region.h>
#include <Autolock.h>
#include <Directory.h>
#include <Path.h>
#include <FilePanel.h>
#include <NodeInfo.h>
#include <Alert.h>
#include <FindDirectory.h>
#include <StorageKit.h>
#include <assert.h>

#include "NPApp.h"
#include "Strings.h"
#include "Protocols.h"
#include "Cache.h"
#include "MessageWindow.h"
#include "URL.h"
#include "DownloadManager.h"

enum {
	kdUninitialized,
	kdDownloading,
	kdStopped,
	kdError,
	kdComplete,
	kdWaiting
};

const uint32	msg_DLButtonPressed		= 'Dlbp';
const uint32	msg_SelectAll			= 'DlSa';
const uint32	msg_TrashSelected		= 'Tnps';

#define kGrey					203
#define kWhite					255
#define kItemHeight				43
#define kIconLeft				5
#define kIconDown				6
#define kNameLeft				45
#define kStatusStringLeft		kNameLeft
#define kStatusLeft				135
#define kStopButtonDown			11
#define kStatusStringDown		29
#define kTextBaseline			15
#define kWindowHSize			300 + B_V_SCROLL_BAR_WIDTH
#define kMaxWindowWidth			600
#define kMinWindowWidth			275
#define	kBottomPortionHeight	B_H_SCROLL_BAR_HEIGHT
#define kTimeAveragingInterval	60
#define kDownloadMenuBarHeight	20

void FormatProgressStr(BString& str, long done, long beginBytes, long of, int bytesPerSec, bool printOf, bool printPercent, bool printRemaining, bool abbreviate);
void FormatBPSStr(BString& str, int bytesPerSec, long done, long of, bool printRemaining, bool abbreviate);
void ByteSizeStr(int size, BString& sizeStr);

class DownloadItemView;

class DownloadScrollView : public BScrollView {
public:
				DownloadScrollView(BRect rect);
virtual			~DownloadScrollView();
void			AddItem(BView *item);
void			RemoveItem(void *item);
void			RemoveItem(int32 index);
void			RemoveAll();
DownloadItemView*	ItemAt(int32 index);
int32			CountItems();
virtual	void	FrameResized(float new_width, float new_height);
virtual void	MessageReceived(BMessage *msg);
virtual void	AttachedToWindow();
void			Pulse(); 
virtual void	KeyDown(const char *bytes, int32 numBytes);
void			DragAllSelected(BPoint where, int32 indexOfIconDragged);
void			LaunchAllSelected();
void			DeleteAllSelected();
void			RetryAllStopped();
void			StopAll();
void			SelectAll();
bool			IsStillDownloading();
void			AddFilePanel(BFilePanel *panel);
void			RemoveFilePanel(BFilePanel *panel);

protected:
void			UpdateScrollBar();
	BList	mItemList;
	BList	mFilePanelList;
	BView*	mTargetView;
	int32	mNumberDownloads;
};

class ActivatedView : public BView {
public:
				ActivatedView(BRect frame, const char *name, uint32 resizingMode, uint32 flags);
virtual void	MouseDown(BPoint where);
};

class DownloadItemView : public BView {
public:
				DownloadItemView(UResourceImp *resource, const char *name, const char *downloadPath, int32 status, bool openWhenDone, bool wasMoved = false);
				DownloadItemView(BMessage *data);
virtual	status_t Archive(BMessage *data, bool deep = true) const;
virtual			~DownloadItemView();
virtual	void	MouseDown(BPoint where);
virtual	void	Draw(BRect updateRect);
virtual void	MessageReceived(BMessage *msg);
virtual void	AttachedToWindow();
void			Pulse();

long			Done() {return mDone > 0 ? mDone : 0;}
long			Of() {return mOf > 0 ? mOf : 0;}
int				BPS() {return mBPS > 0 ? mBPS : 0;}
int				Status() {return mStatus;}
void			StopDownload();
bool			Trash();
void			SetSelected(bool isSelected);
bool			IsSelected() {return mIsSelected;}
bool			FileExists(){return mFileExists;}
node_ref		GetNodeRef() { return mNodeRef; }
entry_ref		GetEntryRef() { return mEntryRef; }
BBitmap			*NewDragBitmap();
bool			Launch();
void			NewParentWidth(float newWidth);
bool			Lock();
void			Unlock();
bool			IsLocked();

protected:
bool			UpdateStatus();
void			FetchIcon();
void			UpdateIconPhase(float percentDone);
void			FinishFile();
void			Init();

	BString	mName;
	int32	mStatus;
	UResourceImp*	mResource;
	
	long		mDone;
	long		mOf;
	int			mBPS;
	bigtime_t	mStartTime;
	bigtime_t	mTotalTime;
	bigtime_t	mLastUpdate;
	bigtime_t	mLastClick;
	BMenuBar*	mMenuBar;
	
	BString		mURL;
	entry_ref	mEntryRef;
	node_ref	mNodeRef;
	bool		mOpenWhenDone;
	BBitmap*	mBigIcon;
	BBitmap*	mSmallIcon;
	int32		mFileIconPhase; //number of pixels in icon bar
	bool		mUpdateIcon;
	bool		mFileExists;
	bool		wasDragged;
	bool		mUpdate;
	bool		mIsSelected;
	bool		mFetchedIcon;
	bool		mButtonDown;

	float		mButtonLeft;
	BBitmap*	mRetryIcon;
	BBitmap*	mRetryPressedIcon;
	BBitmap*	mStopIcon;
	BBitmap*	mStopPressedIcon;
	BLocker		pulseLock;
};

class DownloadWindow : public BWindow {
public:
				DownloadWindow(BRect frame);
virtual			~DownloadWindow();
void			AddDownloadItem(DownloadItemView *item);
void			RemoveDownloadItem(DownloadItemView *item);
virtual void	MessageReceived(BMessage *msg);
virtual	void	MenusBeginning();
virtual bool	QuitRequested();
void			AddFilePanel(BFilePanel *panel);
virtual void	Zoom(BPoint origin, float width, float height);

	DownloadScrollView*	mMainView;
	ActivatedView*		mBottomView;
	BMenuBar*			mMenuBar;
	BMenu*				mDownloadsMenu;
	BMenu*				mFileMenu;
};

#endif
