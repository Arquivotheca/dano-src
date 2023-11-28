#ifndef _DOWNLOADWINDOW_H_
#define _DOWNLOADWINDOW_H_


#include "RList.h"
#include "MThread.h"
#include "VHTConnection.h"
#include <Window.h>
#include <View.h>
#include <Messenger.h>
#include <ListItem.h>
#include <File.h>
#include <Entry.h>

class DownloadWindow : public BWindow
{
public:
	DownloadWindow();

	virtual void	MessageReceived(BMessage *);
};

class DownloadView : public BView
{
public:
	DownloadView(BRect r);
	
	virtual void	AttachedToWindow();
	virtual void	FrameResized(float w, float h);
	//virtual void	MessageReceived();
};

class ProgressBar;

class DlListItem : public BListItem
{
public:
	DlListItem(int32 id);
	
			void SetTo(BMessage *,int32);
	virtual void Update(BView *, const BFont *);
	virtual void DrawItem(BView *owner, BRect itemRect, bool complete);
			void DrawProgress(BView *owner, BRect itemRect);
	int32		fID;
private:
	friend class DownloadWindow;

	entry_ref	fileRef;
	status_t	fStatus;
	
	off_t		totalBytes;			// total file size
	off_t		bytesCompleted;		// amount completed (w/ resumes)
	
	off_t		bytesDownloaded;	// bytes completed this session
	float		secondsElapsed;		// elapsed time (this session)
	static		ProgressBar		*pBar;
};


// synchronization
class DlItem;

class DownloadManager : public BLooper
{
public:
	DownloadManager();
	
	virtual void	MessageReceived(BMessage *msg);
	
	// when this looper runs it should open up the queue file
	// and set up the items
	
	// when it receives a message it should find the first waiting
	// item from the queue mark it as downloading and then begin
	// status change is reported to downloadWindow
	
	// download progress is reported to downloadWindow
	
	// when the thread quits it should message its status to
	// DlManager, dl manager then marks the items status
	// status change is reported to downloadWindow
	
	// cancel download(item)
	// defer (item), download(item), remove(item)
	void				UpdateStatus(DlItem *it,entry_ref *ref );
	void				CheckQueue();
	
	RList<DlItem *>		list;
	BMessenger			statusWindow;
	
	int32				fID;
	DlItem				*currentItem;
	bool				fPaused;
};

class DownloadThread;
class DlItem
{
public:
				DlItem(BLooper *, int32 id);
	virtual		~DlItem();
		int32	ID();
		void	Cancel();
	status_t	SetTo(entry_ref	*ref, bool resume = false);
	status_t	SetTo(BMessage *m, bool resume = false);
//	status_t	Status();
	
	status_t	BeginDownload(BDirectory *dir);
	
			
	enum {
		ITEM_DOWNLOADING = 1,
		ITEM_QUEUED,
		ITEM_DEFERRED,
		ITEM_COMPLETE,
		ITEM_CANCELED,
		ITEM_ERROR
	};
	
private:
	status_t	PerformDownload(BMessage *msg, BFile &file);
	
	friend class DownloadThread;
	friend class DownloadManager;
	
	
	//char		*initalName;
	entry_ref	fileRef;
	
	BMessage	downloadData; 		// replace w/ char *url;
	status_t	fStatus;
	int32		fID;
	
	off_t		totalBytes;			// total file size
	off_t		bytesCompleted;		// amount completed (w/ resumes)
	
	off_t		bytesDownloaded;	// bytes completed this session
	float		secondsElapsed;		// elapsed time (this session)

	DownloadThread		*netThread;
	BLooper				*const dLooper;
};

class	DownloadThread	: public MThread
{
public:
	DownloadThread(BFile &f, DlItem *inItem);
	virtual void	Cancel();
private:
	virtual void	LastCall(long);
	virtual long	Execute();
	BFile			file;
	VHTConnection	htconn;
	DlItem			*fDlItem;
};

#endif

