/******************************************************************************
//
//	File:		 3dmovApp.h
//
//	Description: Demo application using the 3d Kit.
//
//	Copyright 1996, Be Incorporated
//
//****************************************************************************/

#ifndef _3D_MOV_APP_H
#define _3D_MOV_APP_H

#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef _3D_MOV_WINDOW_H
#include "3dmovWindow.h"
#endif
#ifndef _POP_UP_MENU_H
#include <PopUpMenu.h>
#endif
#ifndef _MENU_ITEM_H
#include <MenuItem.h>
#endif
#ifndef _BITMAP_H
#include <Bitmap.h>
#endif
#ifndef _FILE_H
#include <File.h>
#endif
#ifndef _3D_LOOK_H
#include <3dLook.h>
#endif
//#ifndef _PLANB_DRIVER_H
//#include <planb_driver.h>
//#endif
#include "BeQTPlayer.h"
//#include "vidsource.h"
#include <Entry.h>

enum {
	FIRST_WINDOW = 1000,
	SECOND_WINDOW = 1001,
	THIRD_WINDOW = 1002,
	FOURTH_WINDOW = 1003,
	PAUSE_WINDOWS = 1004
};

class VideoSource {
 public:
	BBitmap	    *buffer;
	void		*cur_buffer;
	entry_ref	*myFile;
	bool		source_modified;
 
	VideoSource(int buffer_count, entry_ref *file);
	virtual       ~VideoSource();
	bool          CheckFile(entry_ref *file);
	virtual void  GetFirstFrame();
	virtual void  GetNextFrame();
};


class DefaultSource : public VideoSource {
public:
    DefaultSource();
};

#if 0
class TunerSource : public VideoSource
{
	public:
		TunerSource();	// add card indexes later...
		~TunerSource();
		virtual void GetFirstFrame();
		virtual void GetNextFrame();
		
	private:
		BTSVideoSource	*vidsrc;
};
#endif

class BitmapSource : public VideoSource {
public:
    int			channel_index;
    BBitmap		*my_bitmap;

    BitmapSource(entry_ref *pict, int channel_index);
};


#if 0
class RealTimeSource : public VideoSource {
    int              driver_id;
	planb_vid_info   init;
	area_id			 video_area;
	char*		 	 area;
	
public:
    RealTimeSource(entry_ref *driver);
    virtual       ~RealTimeSource();
	virtual void  GetFirstFrame();
    virtual void  GetNextFrame();
};
#endif


class QuickTimeSource : public VideoSource {
public:
    QuickTimeSource(entry_ref *movie);
    virtual       ~QuickTimeSource();
	virtual void  GetFirstFrame();
    virtual void  GetNextFrame();
	void          CopyBuffer();

	sem_id    sem, in;
	bool      flag;
	QTPlayer  *mPlayer;
    int       curBuf;
	long      mTime;
	long      offset;
    void      *buffers[2];
	void      *base;
	long      size, row_byte;
	long      line_count;
	BBitmap   *mBitmap;
	thread_id player;
};


class Z3dApplication : public BApplication {
public:
	bool			Pause;
	sem_id		windowSem;
	thread_id		poller;
	Z3dWindow	*myWindow[4];
	int			window_count;
	sem_id		video_sem;
	sem_id		sync_sem;
	long			entry_count;
	sem_id		entry_sem;
	long			exit_count;
	sem_id		exit_sem;
	long			alloc_lock;
	sem_id		alloc_sem;
	long			sync_count;
	long			video_lock;
	VideoSource	*video_list[15];
	long			map_ref_index[6+1+2+4];
	map_ref		map_ref_list[6+1+2+4];
	bool			map_ref_lock[6+1+2+4];
	bool		book_stable;
	bool		mapping_stable;
	bool		exit_status;
	
	Z3dApplication();
	~Z3dApplication();
	virtual void		AboutRequested(void);
	virtual void		MessageReceived(BMessage* msg);
	void			ActiveWindow(long index, long state);
	void			OpenVideoChannels();
	void			EnableSync();
	void			DisableSync();
	void			Sync();
	void			CloseVideoChannels();
	void			LockAlloc();
	void			UnlockAlloc();
	void			ClearChannel(int index);
	void			SelectVideoChannel(int index, entry_ref *tref);
	B3dWorld		*GetWorld(int index);
};

#endif















